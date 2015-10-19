/*
 * This file is part of libdcadec.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "common.h"
#include "core_decoder.h"
#include "exss_parser.h"
#include "xll_decoder.h"
#include "fixed_math.h"

#define DCADEC_PACKET_CORE  0x01
#define DCADEC_PACKET_EXSS  0x02
#define DCADEC_PACKET_XLL   0x04

#define DCADEC_PACKET_FILTERED  0x100

struct dcadec_context {
    int flags;
    int packet;

    struct core_decoder *core;
    struct exss_parser *exss;
    struct xll_decoder *xll;

    bool core_residual_valid;

    int *dmix_sample_buffer;

    int nframesamples;
    int sample_rate;
    int bits_per_sample;
    int profile;
    int channel_mask;
    int *samples[SPEAKER_COUNT];
};

static const uint8_t dca2wav_norm[] = {
    WAVESPKR_FC,  WAVESPKR_FL,  WAVESPKR_FR,  WAVESPKR_SL,
    WAVESPKR_SR,  WAVESPKR_LFE, WAVESPKR_BC,  WAVESPKR_BL,
    WAVESPKR_BR,  WAVESPKR_SL,  WAVESPKR_SR,  WAVESPKR_FLC,
    WAVESPKR_FRC, WAVESPKR_TFL, WAVESPKR_TFC, WAVESPKR_TFR,
    WAVESPKR_LFE, WAVESPKR_FLC, WAVESPKR_FRC, WAVESPKR_TC,
    WAVESPKR_TFL, WAVESPKR_TFR, WAVESPKR_TBC, WAVESPKR_TBL,
    WAVESPKR_TBR, WAVESPKR_BC,  WAVESPKR_BL,  WAVESPKR_BR
};

static const uint8_t dca2wav_wide[] = {
    WAVESPKR_FC,  WAVESPKR_FL,  WAVESPKR_FR,  WAVESPKR_BL,
    WAVESPKR_BR,  WAVESPKR_LFE, WAVESPKR_BC,  WAVESPKR_BL,
    WAVESPKR_BR,  WAVESPKR_SL,  WAVESPKR_SR,  WAVESPKR_FLC,
    WAVESPKR_FRC, WAVESPKR_TFL, WAVESPKR_TFC, WAVESPKR_TFR,
    WAVESPKR_LFE, WAVESPKR_SL,  WAVESPKR_SR , WAVESPKR_TC,
    WAVESPKR_TFL, WAVESPKR_TFR, WAVESPKR_TBC, WAVESPKR_TBL,
    WAVESPKR_TBR, WAVESPKR_BC,  WAVESPKR_BL,  WAVESPKR_BR
};

#define DCADEC_LAYOUT_7POINT0_WIDE  \
    (SPEAKER_MASK_C  | SPEAKER_MASK_L  | SPEAKER_MASK_R |   \
     SPEAKER_MASK_Ls | SPEAKER_MASK_Rs |                    \
     SPEAKER_MASK_Lw | SPEAKER_MASK_Rw)

#define DCADEC_LAYOUT_7POINT1_WIDE  \
    (DCADEC_LAYOUT_7POINT0_WIDE | SPEAKER_MASK_LFE1)

static int reorder_samples(struct dcadec_context *dca, int **dca_samples, int dca_mask)
{
    int nchannels = 0;

    if (dca->flags & DCADEC_FLAG_NATIVE_LAYOUT) {
        for (int dca_ch = 0; dca_ch < SPEAKER_COUNT; dca_ch++) {
            if (dca_mask & (1U << dca_ch)) {
                if (!dca_samples[dca_ch])
                    return -DCADEC_EINVAL;
                dca->samples[nchannels++] = dca_samples[dca_ch];
            }
        }
        dca->channel_mask = dca_mask;
    } else {
        int wav_mask = 0;
        int *wav_samples[WAVESPKR_COUNT] = { NULL };
        const uint8_t *dca2wav;
        if (dca_mask == DCADEC_LAYOUT_7POINT0_WIDE ||
            dca_mask == DCADEC_LAYOUT_7POINT1_WIDE)
            dca2wav = dca2wav_wide;
        else
            dca2wav = dca2wav_norm;
        for (size_t dca_ch = 0; dca_ch < sizeof(dca2wav_norm); dca_ch++) {
            if (dca_mask & (1 << dca_ch)) {
                if (!dca_samples[dca_ch])
                    return -DCADEC_EINVAL;
                int wav_ch = dca2wav[dca_ch];
                if (!wav_samples[wav_ch]) {
                    wav_samples[wav_ch] = dca_samples[dca_ch];
                    wav_mask |= 1 << wav_ch;
                }
            }
        }
        for (int wav_ch = 0; wav_ch < WAVESPKR_COUNT; wav_ch++) {
            if (wav_mask & (1 << wav_ch)) {
                dca->samples[nchannels++] = wav_samples[wav_ch];
            }
        }
        dca->channel_mask = wav_mask;
    }

    return nchannels;
}

static void clip_samples(struct dcadec_context *dca, int nchannels)
{
    int nsamples = dca->nframesamples;

    switch (dca->bits_per_sample) {
    case 24:
        for (int ch = 0; ch < nchannels; ch++)
            for (int n = 0; n < nsamples; n++)
                dca->samples[ch][n] = clip23(dca->samples[ch][n]);
        break;
    case 16:
        for (int ch = 0; ch < nchannels; ch++)
            for (int n = 0; n < nsamples; n++)
                dca->samples[ch][n] = clip15(dca->samples[ch][n]);
        break;
    default:
        assert(0);
        break;
    }
}

static int down_mix_prim_chset(struct dcadec_context *dca, int **samples,
                               int nsamples, int *ch_mask, int *dmix_coeff)
{
    // No action if already 2.0. Remove LFE channel if 2.1.
    if ((*ch_mask & ~SPEAKER_MASK_LFE1) == (SPEAKER_MASK_L | SPEAKER_MASK_R)) {
        *ch_mask = SPEAKER_MASK_L | SPEAKER_MASK_R;
        return 0;
    }

    // Unless both KEEP_DMIX flags are set, perform 2.0 downmix only when
    // custom matrix is present
    if (!dmix_coeff && !(dca->flags & DCADEC_FLAG_KEEP_DMIX_6CH))
        return 0;

    // Reallocate downmix sample buffer
    if (ta_alloc_fast(dca, &dca->dmix_sample_buffer, 2 * nsamples, sizeof(int)) < 0)
        return -DCADEC_ENOMEM;

    memset(dca->dmix_sample_buffer, 0, 2 * nsamples * sizeof(int));

    int nchannels = dca_popcount(*ch_mask);

    // Perform downmix
    for (int spkr = 0, pos = 0; spkr < SPEAKER_COUNT; spkr++) {
        if (!(*ch_mask & (1U << spkr)))
            continue;

        for (int ch = 0; ch < 2; ch++) {
            int coeff;

            // Use custom matrix if present. Otherwise use default matrix that
            // covers all supported core audio channel arrangements.
            if (dmix_coeff) {
                coeff = dmix_coeff[ch * nchannels + pos];
            } else {
                switch (spkr) {
                case SPEAKER_C:
                case SPEAKER_Cs:
                    coeff = (nchannels == 1) ? 23170 : 16423;
                    break;
                case SPEAKER_L:
                    coeff = (ch == 0) ? 23170 : 0;
                    break;
                case SPEAKER_R:
                    coeff = (ch == 1) ? 23170 : 0;
                    break;
                case SPEAKER_Ls:
                    coeff = (ch == 0) ? 16423 : 0;
                    break;
                case SPEAKER_Rs:
                    coeff = (ch == 1) ? 16423 : 0;
                    break;
                default:
                    coeff = 0;
                    break;
                }
            }

            if (coeff) {
                int *src = samples[spkr];
                int *dst = dca->dmix_sample_buffer + ch * nsamples;
                for (int n = 0; n < nsamples; n++)
                    dst[n] += mul15(src[n], coeff);
            }
        }

        pos++;
    }

    samples[SPEAKER_L] = dca->dmix_sample_buffer;
    samples[SPEAKER_R] = dca->dmix_sample_buffer + nsamples;
    *ch_mask = SPEAKER_MASK_L | SPEAKER_MASK_R;
    return 0;
}

static int filter_core_frame(struct dcadec_context *dca)
{
    struct core_decoder *core = dca->core;

    dca->core_residual_valid = false;

    // Filter core frame
    int ret;
    if ((ret = core_filter(core, dca->flags)) < 0)
        return ret;

    // Downmix core channels to Lo/Ro
    if (dca->flags & DCADEC_FLAG_KEEP_DMIX_2CH) {
        int *coeff = NULL;
        if (core->prim_dmix_embedded && core->prim_dmix_type == DMIX_TYPE_LoRo)
            coeff = core->prim_dmix_coeff;
        if ((ret = down_mix_prim_chset(dca, core->output_samples, core->npcmsamples,
                                       &core->ch_mask, coeff)) < 0)
            return ret;
    }

    // Reorder sample buffer pointers
    if ((ret = reorder_samples(dca, core->output_samples, core->ch_mask)) <= 0)
        return -DCADEC_EINVAL;

    dca->nframesamples = core->npcmsamples;
    dca->sample_rate = core->output_rate;
    dca->bits_per_sample = 24;

    // Set profile
    if (core->ext_audio_mask & (EXSS_XBR | EXSS_XXCH | EXSS_X96))
        dca->profile = DCADEC_PROFILE_HD_HRA;
    else if (core->ext_audio_mask & (CSS_XXCH | CSS_XCH))
        dca->profile = DCADEC_PROFILE_DS_ES;
    else if (core->ext_audio_mask & CSS_X96)
        dca->profile = DCADEC_PROFILE_DS_96_24;
    else
        dca->profile = DCADEC_PROFILE_DS;

    // Perform clipping
    if (dca->flags & DCADEC_FLAG_KEEP_DMIX_2CH)
        clip_samples(dca, ret);

    return 0;
}

static int map_spkr_to_core_spkr(struct core_decoder *core, int spkr)
{
    if (core->ch_mask & (1U << spkr))
        return spkr;
    if (spkr == SPEAKER_Lss && (core->ch_mask & SPEAKER_MASK_Ls))
        return SPEAKER_Ls;
    if (spkr == SPEAKER_Rss && (core->ch_mask & SPEAKER_MASK_Rs))
        return SPEAKER_Rs;
    return -1;
}

static struct xll_chset *find_first_hier_dmix_chset(struct xll_decoder *xll)
{
    for_each_chset(xll, c)
        if (!c->primary_chset && c->dmix_embedded && c->hier_chset)
            return c;

    return NULL;
}

static struct xll_chset *find_next_hier_dmix_chset(struct xll_chset *c)
{
    struct xll_decoder *xll = c->decoder;

    if (c->hier_chset)
        while (++c < &xll->chset[xll->nchsets])
            if (!c->primary_chset && c->dmix_embedded && c->hier_chset)
                return c;

    return NULL;
}

struct downmix {
    int *samples[XLL_MAX_BANDS][XLL_MAX_CHSETS * XLL_MAX_CHANNELS];
    int *deci_history[XLL_MAX_CHSETS * XLL_MAX_CHANNELS];
};

static void undo_down_mix(struct xll_chset *c, struct downmix *dmix)
{
    struct xll_decoder *xll = c->decoder;
    int nsamples = xll->nframesamples;

    // Pre-scale by next channel set in hierarchy
    struct xll_chset *o = find_next_hier_dmix_chset(c);
    if (o) {
        int *coeff_ptr = c->dmix_coeff;
        for (int i = 0; i < c->dmix_m; i++) {
            int scale_inv = o->dmix_scale_inv[i];
            for (int j = 0; j < c->nchannels; j++) {
                int coeff = mul16(*coeff_ptr, scale_inv);
                *coeff_ptr++ = mul15(coeff, o->dmix_scale[c->dmix_m + j]);
            }
        }
    }

    // Undo downmix of preceding channels in frequency band 0
    int *coeff_ptr = c->dmix_coeff;
    for (int i = 0; i < c->dmix_m; i++) {
        for (int j = 0; j < c->nchannels; j++) {
            int coeff = *coeff_ptr++;
            if (coeff) {
                int *src = c->msb_sample_buffer[XLL_BAND_0][j];
                int *dst = dmix->samples[XLL_BAND_0][i];
                for (int k = 0; k < nsamples; k++)
                    dst[k] -= mul15(src[k], coeff);
            }
        }
    }

    // Undo downmix of preceding channels in frequency band 1
    if (c->nfreqbands > 1 && c->band_dmix_embedded[XLL_BAND_1]) {
        int *coeff_ptr = c->dmix_coeff;
        for (int i = 0; i < c->dmix_m; i++) {
            for (int j = 0; j < c->nchannels; j++) {
                int coeff = *coeff_ptr++;
                if (coeff) {
                    // Undo downmix of channel samples
                    int *src = c->msb_sample_buffer[XLL_BAND_1][j];
                    int *dst = dmix->samples[XLL_BAND_1][i];
                    for (int k = 0; k < nsamples; k++)
                        dst[k] -= mul15(src[k], coeff);

                    // Undo downmix of decimator history
                    src = c->deci_history[j];
                    dst = dmix->deci_history[i];
                    for (int k = 1; k < XLL_DECI_HISTORY; k++)
                        dst[k] -= mul15(src[k], coeff);
                }
            }
        }
    }
}

static void scale_down_mix(struct xll_chset *c, struct downmix *dmix)
{
    struct xll_decoder *xll = c->decoder;
    int nsamples = xll->nframesamples;

    // Pre-scale by next channel set in hierarchy
    struct xll_chset *o = find_next_hier_dmix_chset(c);
    if (o) {
        for (int i = 0; i < c->dmix_m; i++) {
            int scale = o->dmix_scale[i];
            c->dmix_scale[i] = mul15(c->dmix_scale[i], scale);
        }
    }

    // Scale down preceding channels in frequency band 0
    for (int i = 0; i < c->dmix_m; i++) {
        int scale = c->dmix_scale[i];
        if (scale != (1 << 15)) {
            int *buf = dmix->samples[XLL_BAND_0][i];
            for (int k = 0; k < nsamples; k++)
                buf[k] = mul15(buf[k], scale);
        }
    }

    // Scale down preceding channels in frequency band 1
    if (c->nfreqbands > 1 && c->band_dmix_embedded[XLL_BAND_1]) {
        for (int i = 0; i < c->dmix_m; i++) {
            int scale = c->dmix_scale[i];
            if (scale != (1 << 15)) {
                // Scale down channel samples
                int *buf = dmix->samples[XLL_BAND_1][i];
                for (int k = 0; k < nsamples; k++)
                    buf[k] = mul15(buf[k], scale);

                // Scale down decimator history
                buf = dmix->deci_history[i];
                for (int k = 1; k < XLL_DECI_HISTORY; k++)
                    buf[k] = mul15(buf[k], scale);
            }
        }
    }
}

static int validate_hd_ma_frame(struct dcadec_context *dca)
{
    struct xll_decoder *xll = dca->xll;
    struct xll_chset *p = &xll->chset[0];

    // Validate the first (primary) channel set
    if (!p->primary_chset)
        return -DCADEC_ENOSUP;

    if (!p->ch_mask_enabled && p->nchannels != 2)
        return -DCADEC_ENOSUP;

    if (p->storage_bit_res != 16 && p->storage_bit_res != 24)
        return -DCADEC_ENOSUP;

    if (p->pcm_bit_res > p->storage_bit_res)
        return -DCADEC_EINVAL;

    // Validate channel sets
    bool residual = false;
    for_each_active_chset(xll, c) {
        // Reject multiple primary channel sets
        if (c->primary_chset && c != p)
            return -DCADEC_ENOSUP;

        // Reject non-primary channel sets w/o channel mask
        if (!c->ch_mask_enabled && c != p)
            return -DCADEC_ENOSUP;

        // Reject parallel downmix
        if (!c->primary_chset && c->dmix_embedded && !c->hier_chset)
            return -DCADEC_ENOSUP;

        // For now, PCM characteristics of all channel sets must be the same
        if (c->freq != p->freq)
            return -DCADEC_ENOSUP;

        if (c->pcm_bit_res != p->pcm_bit_res)
            return -DCADEC_ENOSUP;

        if (c->storage_bit_res != p->storage_bit_res)
            return -DCADEC_ENOSUP;

        if (c->nfreqbands != p->nfreqbands)
            return -DCADEC_ENOSUP;

        // Reject sampling frequency modifier
        if (c->interpolate)
            return -DCADEC_ENOSUP;

        residual |= c->residual_encode != (1 << c->nchannels) - 1;
    }

    // Verify that core is compatible if there are residual encoded channel sets
    if (residual) {
        struct core_decoder *core = dca->core;
        if (!(dca->packet & DCADEC_PACKET_CORE))
            return -DCADEC_EINVAL;
        if (p->freq != core->sample_rate &&
            p->freq != core->sample_rate * 2)
            return -DCADEC_ENOSUP;
        if (xll->nframesamples != core->npcmblocks * NUM_PCMBLOCK_SAMPLES &&
            xll->nframesamples != core->npcmblocks * NUM_PCMBLOCK_SAMPLES * 2)
            return -DCADEC_EINVAL;
    }

    return 0;
}

static void force_lossy_output(struct core_decoder *core, struct xll_chset *c)
{
    // Clear all band data
    for (int band = 0; band < c->nfreqbands; band++)
        xll_clear_band_data(c, band);

    // Clear decimator history and scalable LSBs
    memset(c->deci_history, 0, sizeof(c->deci_history));
    memset(c->nscalablelsbs, 0, sizeof(c->nscalablelsbs));
    memset(c->bit_width_adjust, 0, sizeof(c->bit_width_adjust));

    // Replace non-residual encoded channels with lossy counterparts
    for (int ch = 0; ch < c->nchannels; ch++) {
        if (!(c->residual_encode & (1 << ch)))
            continue;
        int spkr = xll_map_ch_to_spkr(c, ch);
        if (spkr < 0)
            continue;
        int core_spkr = map_spkr_to_core_spkr(core, spkr);
        if (core_spkr < 0)
            continue;
        c->residual_encode &= ~(1 << ch);
    }
}

static int filter_residual_core_frame(struct dcadec_context *dca)
{
    struct core_decoder *core = dca->core;
    struct xll_decoder *xll = dca->xll;
    int flags = DCADEC_FLAG_CORE_BIT_EXACT | DCADEC_FLAG_KEEP_DMIX_6CH;

    // Double sampling frequency if needed
    if (xll->chset->freq == 96000 && core->sample_rate == 48000)
        flags |= DCADEC_FLAG_CORE_SYNTH_X96;

    // Filter core frame
    int ret;
    if ((ret = core_filter(core, flags)) < 0)
        return ret;

    // Force lossy downmixed output if this is the first core frame since
    // the last time history was cleared
    if (dca->core_residual_valid == false && xll->nchsets > 1) {
        for_each_chset(xll, c) {
            if (c < &xll->chset[xll->nactivechsets])
                force_lossy_output(core, c);
            if (!c->primary_chset)
                c->dmix_embedded = false;
        }
        xll->scalable_lsbs = false;
        xll->fixed_lsb_width = 0;
    }

    dca->core_residual_valid = true;
    return 0;
}

static int combine_residual_core_frame(struct dcadec_context *dca,
                                       struct xll_chset *c)
{
    struct core_decoder *core = dca->core;
    struct xll_decoder *xll = dca->xll;
    int nsamples = xll->nframesamples;

    if (c->freq != core->output_rate)
        return -DCADEC_ENOSUP;

    if (nsamples != core->npcmsamples)
        return -DCADEC_EINVAL;

    // See if this channel set is downmixed and find the next channel set in
    // hierarchy. If downmixed, undo core pre-scaling before combining with
    // residual (residual is not scaled).
    struct xll_chset *o = find_next_hier_dmix_chset(c);

    // Reduce core bit width and combine with residual
    for (int ch = 0; ch < c->nchannels; ch++) {
        if (c->residual_encode & (1 << ch))
            continue;

        int spkr = xll_map_ch_to_spkr(c, ch);
        if (spkr < 0)
            return -DCADEC_EINVAL;

        int core_spkr = map_spkr_to_core_spkr(core, spkr);
        if (core_spkr < 0)
            return -DCADEC_EINVAL;

        int shift = 24 - c->pcm_bit_res;
        // Account for LSB width
        if (xll->scalable_lsbs)
            shift += xll_get_lsb_width(c, XLL_BAND_0, ch);
        int round = shift > 0 ? 1 << (shift - 1) : 0;

        int *dst = c->msb_sample_buffer[XLL_BAND_0][ch];
        int *src = core->output_samples[core_spkr];
        if (o) {
            // Undo embedded core downmix pre-scaling
            int scale_inv = o->dmix_scale_inv[c->dmix_m + ch];
            for (int n = 0; n < nsamples; n++)
                dst[n] += clip23((mul16(src[n], scale_inv) + round) >> shift);
        } else {
            // No downmix scaling
            for (int n = 0; n < nsamples; n++)
                dst[n] += (src[n] + round) >> shift;
        }
    }

    return 0;
}

static int filter_hd_ma_frame(struct dcadec_context *dca)
{
    struct xll_decoder *xll = dca->xll;
    struct xll_chset *p = &xll->chset[0];
    int ret;

    // Filter core frame if present
    if (dca->packet & DCADEC_PACKET_CORE)
        if ((ret = filter_residual_core_frame(dca)) < 0)
            return ret;

    // Process frequency band 0 for active channel sets
    for_each_active_chset(xll, c) {
        xll_filter_band_data(c, XLL_BAND_0);

        // Check for residual encoded channel set
        if (c->residual_encode != (1 << c->nchannels) - 1)
            if ((ret = combine_residual_core_frame(dca, c)) < 0)
                return ret;

        // Assemble MSB and LSB parts after combining with core
        if (xll->scalable_lsbs)
            xll_assemble_msbs_lsbs(c, XLL_BAND_0);
    }

    // Process frequency band 1 for active channel sets
    if (xll->nfreqbands > 1) {
        for_each_active_chset(xll, c) {
            xll_filter_band_data(c, XLL_BAND_1);
            xll_assemble_msbs_lsbs(c, XLL_BAND_1);
        }
    }

    // Undo hierarchial downmix and apply scaling
    if (xll->nchsets > 1) {
        struct downmix dmix;
        int nchannels = 0;

        // Build channel vectors for all active channel sets
        for_each_active_chset(xll, c) {
            if (c->hier_chset) {
                for (int ch = 0; ch < c->nchannels; ch++) {
                    dmix.samples[XLL_BAND_0][nchannels] =
                        c->msb_sample_buffer[XLL_BAND_0][ch];
                    dmix.samples[XLL_BAND_1][nchannels] =
                        c->msb_sample_buffer[XLL_BAND_1][ch];
                    dmix.deci_history[nchannels] = c->deci_history[ch];
                    nchannels++;
                }
            }
        }

        // Walk through downmix embedded channel sets
        for (struct xll_chset *o = find_first_hier_dmix_chset(xll);
             o != NULL; o = find_next_hier_dmix_chset(o)) {
            if (o->dmix_m > nchannels)
                o->dmix_m = nchannels;
            if (o->dmix_m == nchannels) {
                scale_down_mix(o, &dmix);
                break;
            }
            undo_down_mix(o, &dmix);
        }
    }

    // Assemble frequency bands 0 and 1 for active channel sets
    if (xll->nfreqbands > 1) {
        for_each_active_chset(xll, c)
            if ((ret = xll_assemble_freq_bands(c)) < 0)
                return ret;
        // Double sampling frequency
        xll->nframesamples *= 2;
        p->freq *= 2;
    }

    // Output speaker map and channel mask
    int *spkr_map[SPEAKER_COUNT] = { NULL };
    int ch_mask = 0;

    // Fake up channel mask for primary channel set if needed for LtRt decoding
    if (!p->ch_mask_enabled) {
        if (p->nchannels == 2)
            p->ch_mask = SPEAKER_MASK_L | SPEAKER_MASK_R;
        else
            return -DCADEC_ENOSUP;
    }

    // Build the output speaker map
    for_each_active_chset(xll, c) {
        for (int ch = 0; ch < c->nchannels; ch++) {
            int spkr = xll_map_ch_to_spkr(c, ch);
            if (spkr < 0)
                return -DCADEC_EINVAL;
            if (spkr_map[spkr])
                return -DCADEC_EINVAL;
            spkr_map[spkr] = c->out_sample_buffer[ch];
        }
        ch_mask |= c->ch_mask;
    }

    // Normalize to regular 5.1 layout if downmixing
    if (dca->flags & DCADEC_FLAG_KEEP_DMIX_MASK) {
        if (ch_mask & SPEAKER_MASK_Lss) {
            spkr_map[SPEAKER_Ls] = spkr_map[SPEAKER_Lss];
            ch_mask = (ch_mask & ~SPEAKER_MASK_Lss) | SPEAKER_MASK_Ls;
        }
        if (ch_mask & SPEAKER_MASK_Rss) {
            spkr_map[SPEAKER_Rs] = spkr_map[SPEAKER_Rss];
            ch_mask = (ch_mask & ~SPEAKER_MASK_Rss) | SPEAKER_MASK_Rs;
        }
    }

    // Downmix primary channel set to Lo/Ro
    if (dca->flags & DCADEC_FLAG_KEEP_DMIX_2CH) {
        int *coeff = NULL;
        if (p->dmix_embedded && p->dmix_type == DMIX_TYPE_LoRo)
            coeff = p->dmix_coeff;
        if ((ret = down_mix_prim_chset(dca, spkr_map, xll->nframesamples,
                                       &ch_mask, coeff)) < 0)
            return ret;
    }

    // Reorder sample buffer pointers
    if ((ret = reorder_samples(dca, spkr_map, ch_mask)) <= 0)
        return -DCADEC_EINVAL;

    // Shift samples to account for storage bit width
    int shift = p->storage_bit_res - p->pcm_bit_res;
    if (shift > 0) {
        int nsamples = xll->nframesamples;
        for (int ch = 0; ch < ret; ch++)
            for (int n = 0; n < nsamples; n++)
                dca->samples[ch][n] *= 1 << shift;
    }

    dca->nframesamples = xll->nframesamples;
    dca->sample_rate = p->freq;
    dca->bits_per_sample = p->storage_bit_res;
    dca->profile = DCADEC_PROFILE_HD_MA;

    // Perform clipping
    if (dca->flags & DCADEC_FLAG_KEEP_DMIX_MASK)
        clip_samples(dca, ret);

    return 0;
}

DCADEC_API int dcadec_context_parse(struct dcadec_context *dca, uint8_t *data, size_t size)
{
    int ret;

    if (!dca || !data || size < 4 || ((uintptr_t)data & 3))
        return -DCADEC_EINVAL;

    dca->packet = 0;

    // Parse backward compatible core sub-stream
    if (DCA_MEM32NE(data) == DCA_32BE_C(SYNC_WORD_CORE)) {
        if (!dca->core)
            if (!(dca->core = ta_znew(dca, struct core_decoder)))
                return -DCADEC_ENOMEM;

        if ((ret = core_parse(dca->core, data, size, dca->flags, NULL)) < 0)
            return ret;

        dca->packet |= DCADEC_PACKET_CORE;

        // EXXS data must be aligned on 4-byte boundary by the caller
        size_t frame_size = DCA_ALIGN(dca->core->frame_size, 4);
        if (size - 4 > frame_size) {
            data += frame_size;
            size -= frame_size;
        }
    }

    // Parse extension sub-stream (EXSS)
    if (DCA_MEM32NE(data) == DCA_32BE_C(SYNC_WORD_EXSS)) {
        if (!dca->exss)
            if (!(dca->exss = ta_znew(dca, struct exss_parser)))
                return -DCADEC_ENOMEM;

        if ((ret = exss_parse(dca->exss, data, size)) < 0) {
            if (dca->flags & DCADEC_FLAG_STRICT)
                return ret;
        } else {
            dca->packet |= DCADEC_PACKET_EXSS;
        }
    }

    // Parse coding components in the first EXSS asset
    if (dca->packet & DCADEC_PACKET_EXSS) {
        struct exss_asset *asset = &dca->exss->assets[0];

        // Parse core component in EXSS
        if (!(dca->packet & DCADEC_PACKET_CORE) && (asset->extension_mask & EXSS_CORE)) {
            if (!dca->core)
                if (!(dca->core = ta_znew(dca, struct core_decoder)))
                    return -DCADEC_ENOMEM;

            if ((ret = core_parse(dca->core, data, size, dca->flags, asset)) < 0)
                return ret;

            dca->packet |= DCADEC_PACKET_CORE;
        }

        // Parse XLL component in EXSS
        if (!(dca->flags & DCADEC_FLAG_CORE_ONLY) && (asset->extension_mask & EXSS_XLL)) {
            if (!dca->xll) {
                if (!(dca->xll = ta_znew(dca, struct xll_decoder)))
                    return -DCADEC_ENOMEM;
                dca->xll->flags = dca->flags;
            }

            if ((ret = xll_parse(dca->xll, data, size, asset)) < 0) {
                if (dca->flags & DCADEC_FLAG_STRICT)
                    return ret;
            } else {
                dca->packet |= DCADEC_PACKET_XLL;
            }
        }
    }

    if (!dca->packet)
        return -DCADEC_ENOSYNC;

    // Parse core extensions in EXSS or backward compatible core sub-stream
    if (!(dca->flags & DCADEC_FLAG_CORE_ONLY) && (dca->packet & DCADEC_PACKET_CORE)) {
        struct exss_asset *asset = NULL;
        if (dca->packet & DCADEC_PACKET_EXSS)
            asset = &dca->exss->assets[0];
        if ((ret = core_parse_exss(dca->core, data, size, dca->flags, asset)) < 0)
            return ret;
    }

    return 0;
}

DCADEC_API struct dcadec_core_info *dcadec_context_get_core_info(struct dcadec_context *dca)
{
    if (dca && (dca->packet & DCADEC_PACKET_CORE))
        return core_get_info(dca->core);
    return NULL;
}

DCADEC_API void dcadec_context_free_core_info(struct dcadec_core_info *info)
{
    ta_free(info);
}

DCADEC_API struct dcadec_exss_info *dcadec_context_get_exss_info(struct dcadec_context *dca)
{
    if (dca) {
        if (dca->packet & DCADEC_PACKET_EXSS)
            return exss_get_info(dca->exss);
        if (dca->packet & DCADEC_PACKET_CORE)
            return core_get_info_exss(dca->core);
    }
    return NULL;
}

DCADEC_API void dcadec_context_free_exss_info(struct dcadec_exss_info *info)
{
    ta_free(info);
}

DCADEC_API int dcadec_context_filter(struct dcadec_context *dca, int ***samples,
                                     int *nsamples, int *channel_mask,
                                     int *sample_rate, int *bits_per_sample,
                                     int *profile)
{
    int ret;

    if (!dca)
        return -DCADEC_EINVAL;

    if (!(dca->packet & DCADEC_PACKET_FILTERED)) {
        if (dca->packet & DCADEC_PACKET_XLL) {
            if ((ret = validate_hd_ma_frame(dca)) < 0) {
                if (dca->flags & DCADEC_FLAG_STRICT)
                    return ret;
                if (!(dca->packet & DCADEC_PACKET_CORE))
                    return ret;
                if ((ret = filter_core_frame(dca)) < 0)
                    return ret;
            } else {
                if ((ret = filter_hd_ma_frame(dca)) < 0)
                    return ret;
            }
        } else if (dca->packet & DCADEC_PACKET_CORE) {
            if ((ret = filter_core_frame(dca)) < 0)
                return ret;
        } else {
            return -DCADEC_EINVAL;
        }
        dca->packet |= DCADEC_PACKET_FILTERED;
    }

    if (samples)
        *samples = dca->samples;
    if (nsamples)
        *nsamples = dca->nframesamples;
    if (channel_mask)
        *channel_mask = dca->channel_mask;
    if (sample_rate)
        *sample_rate = dca->sample_rate;
    if (bits_per_sample)
        *bits_per_sample = dca->bits_per_sample;
    if (profile)
        *profile = dca->profile;
    return 0;
}

DCADEC_API void dcadec_context_clear(struct dcadec_context *dca)
{
    if (dca) {
        core_clear(dca->core);
        xll_clear(dca->xll);
        dca->core_residual_valid = false;
    }
}

DCADEC_API struct dcadec_context *dcadec_context_create(int flags)
{
    struct dcadec_context *dca = ta_znew(NULL, struct dcadec_context);
    if (dca)
        dca->flags = flags;
    return dca;
}

DCADEC_API void dcadec_context_destroy(struct dcadec_context *dca)
{
    ta_free(dca);
}

DCADEC_API const char *dcadec_strerror(int errnum)
{
    static const char * const errors[] = {
        "Invalid argument",
        "Invalid bitstream format",
        "CRC check failed",
        "Bitstream navigation error",
        "Synchronization error",
        "Unsupported feature",
        "Memory allocation error",
        "PCM output overflow",
        "I/O error",
        "PCM output parameters changed"
    };

    if (errnum >= 0)
        return "No error";

    unsigned int err = -errnum - 1;
    if (err < dca_countof(errors))
        return errors[err];
    else
        return "Unspecified error";
}

DCADEC_API unsigned int dcadec_version(void)
{
    return DCADEC_VERSION;
}

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
#include "exss_parser.h"

// Table 8-5: Sample rate decoding
const uint32_t exss_sample_rates[16] = {
      8000,  16000,  32000,  64000,
    128000,  22050,  44100,  88200,
    176400, 352800,  12000,  24000,
     48000,  96000, 192000, 384000
};

static int count_chs_for_mask(int mask)
{
    return dca_popcount(mask) + dca_popcount(mask & SPEAKER_PAIR_ALL_2);
}

static void parse_xll_parameters(struct exss_asset *asset)
{
    struct exss_parser *exss = asset->parser;

    // Size of XLL data in extension substream
    asset->xll_size = bits_get(&exss->bits, exss->exss_size_nbits) + 1;
    // XLL sync word present flag
    asset->xll_sync_present = bits_get1(&exss->bits);
    if (asset->xll_sync_present) {
        // Peak bit rate smoothing buffer size
        bits_skip(&exss->bits, 4);
        // Number of bits for XLL decoding delay
        int xll_delay_nbits = bits_get(&exss->bits, 5) + 1;
        // Initial XLL decoding delay in frames
        asset->xll_delay_nframes = bits_get(&exss->bits, xll_delay_nbits);
        // Number of bytes offset to XLL sync
        asset->xll_sync_offset = bits_get(&exss->bits, exss->exss_size_nbits);
    } else {
        asset->xll_delay_nframes = 0;
        asset->xll_sync_offset = 0;
    }
}

static void parse_lbr_parameters(struct exss_asset *asset)
{
    struct exss_parser *exss = asset->parser;

    // Size of LBR component in extension substream
    asset->lbr_size = bits_get(&exss->bits, 14) + 1;
    // LBR sync word present flag
    if (bits_get1(&exss->bits))
        // LBR sync distance
        bits_skip(&exss->bits, 2);
}

static int parse_descriptor(struct exss_asset *asset)
{
    struct exss_parser *exss = asset->parser;
    int i, j;

    size_t descr_pos = exss->bits.index;

    // Size of audio asset descriptor in bytes
    size_t descr_size = bits_get(&exss->bits, 9) + 1;

    // Audio asset identifier
    asset->asset_index = bits_get(&exss->bits, 3);

    //
    // Per stream static metadata
    //

    if (exss->static_fields_present) {
        // Asset type descriptor presence
        if (bits_get1(&exss->bits))
            // Asset type descriptor
            bits_skip(&exss->bits, 4);

        // Language descriptor presence
        if (bits_get1(&exss->bits))
            // Language descriptor
            bits_skip(&exss->bits, 24);

        // Additional textual information presence
        if (bits_get1(&exss->bits)) {
            // Byte size of additional text info
            int text_size = bits_get(&exss->bits, 10) + 1;
            // Additional textual information string
            bits_skip(&exss->bits, text_size * 8);
        }

        // PCM bit resolution
        asset->pcm_bit_res = bits_get(&exss->bits, 5) + 1;

        // Maximum sample rate
        asset->max_sample_rate = exss_sample_rates[bits_get(&exss->bits, 4)];

        // Total number of channels
        asset->nchannels_total = bits_get(&exss->bits, 8) + 1;

        // One to one map channel to speakers
        asset->one_to_one_map_ch_to_spkr = bits_get1(&exss->bits);
        if (asset->one_to_one_map_ch_to_spkr) {
            // Embedded stereo flag
            if (asset->nchannels_total > 2)
                asset->embedded_stereo = bits_get1(&exss->bits);

            // Embedded 6 channels flag
            if (asset->nchannels_total > 6)
                asset->embedded_6ch = bits_get1(&exss->bits);

            // Speaker mask enabled flag
            asset->spkr_mask_enabled = bits_get1(&exss->bits);

            int spkr_mask_nbits = 0;
            if (asset->spkr_mask_enabled) {
                // Number of bits for speaker activity mask
                spkr_mask_nbits = (bits_get(&exss->bits, 2) + 1) << 2;
                // Loudspeaker activity mask
                asset->spkr_mask = bits_get(&exss->bits, spkr_mask_nbits);
            }

            // Number of speaker remapping sets
            int spkr_remap_nsets = bits_get(&exss->bits, 3);
            enforce(!spkr_remap_nsets || spkr_mask_nbits,
                    "Speaker mask disabled yet there are remapping sets");

            // Standard loudspeaker layout mask
            int nspeakers[8];
            for (i = 0; i < spkr_remap_nsets; i++)
                nspeakers[i] = count_chs_for_mask(bits_get(&exss->bits, spkr_mask_nbits));

            for (i = 0; i < spkr_remap_nsets; i++) {
                // Number of channels to be decoded for speaker remapping
                int nch_for_remaps = bits_get(&exss->bits, 5) + 1;
                for (j = 0; j < nspeakers[i]; j++) {
                    // Decoded channels to output speaker mapping mask
                    int remap_ch_mask = bits_get(&exss->bits, nch_for_remaps);
                    // Loudspeaker remapping codes
                    int ncodes = dca_popcount(remap_ch_mask);
                    bits_skip(&exss->bits, ncodes * 5);
                }
            }
        } else {
            asset->embedded_stereo = false;
            asset->embedded_6ch = false;
            asset->spkr_mask_enabled = false;
            asset->spkr_mask = 0;

            // Representation type
            asset->representation_type = bits_get(&exss->bits, 3);
        }
    }

    //
    // DRC, DNC and mixing metadata
    //

    // Dynamic range coefficient presence flag
    bool drc_present = bits_get1(&exss->bits);

    // Code for dynamic range coefficient
    if (drc_present)
        bits_skip(&exss->bits, 8);

    // Dialog normalization presence flag
    if (bits_get1(&exss->bits))
        // Dialog normalization code
        bits_skip(&exss->bits, 5);

    // DRC for stereo downmix
    if (drc_present && asset->embedded_stereo)
        bits_skip(&exss->bits, 8);

    // Mixing metadata presence flag
    if (exss->mix_metadata_enabled && bits_get1(&exss->bits)) {
        // External mixing flag
        bits_skip1(&exss->bits);

        // Post mixing / replacement gain adjustment
        bits_skip(&exss->bits, 6);

        // DRC prior to mixing
        if (bits_get(&exss->bits, 2) == 3)
            // Custom code for mixing DRC
            bits_skip(&exss->bits, 8);
        else
            // Limit for mixing DRC
            bits_skip(&exss->bits, 3);

        // Scaling type for channels of main audio
        // Scaling parameters of main audio
        if (bits_get1(&exss->bits))
            for (i = 0; i < exss->nmixoutconfigs; i++)
                bits_skip(&exss->bits, 6 * exss->nmixoutchs[i]);
        else
            bits_skip(&exss->bits, 6 * exss->nmixoutconfigs);

        int nchannels_dmix = asset->nchannels_total;
        if (asset->embedded_6ch)
            nchannels_dmix += 6;
        if (asset->embedded_stereo)
            nchannels_dmix += 2;
        for (i = 0; i < exss->nmixoutconfigs; i++) {
            for (j = 0; j < nchannels_dmix; j++) {
                // Mix output mask
                int mix_map_mask = bits_get(&exss->bits, exss->nmixoutchs[i]);
                // Mixing coefficients
                int nmixcoefs = dca_popcount(mix_map_mask);
                bits_skip(&exss->bits, 6 * nmixcoefs);
            }
        }
    }

    //
    // Decoder navigation data
    //

    // Coding mode for the asset
    asset->coding_mode = bits_get(&exss->bits, 2);

    // Coding components used in asset
    switch (asset->coding_mode) {
    case 0: // Coding mode that may contain multiple coding components
        asset->extension_mask = bits_get(&exss->bits, 12);
        if (asset->extension_mask & EXSS_CORE) {
            // Size of core component in extension substream
            asset->core_size = bits_get(&exss->bits, 14) + 1;
            // Core sync word present flag
            if (bits_get1(&exss->bits))
                // Core sync distance
                bits_skip(&exss->bits, 2);
        }
        if (asset->extension_mask & EXSS_XBR)
            // Size of XBR extension in extension substream
            asset->xbr_size = bits_get(&exss->bits, 14) + 1;
        if (asset->extension_mask & EXSS_XXCH)
            // Size of XXCH extension in extension substream
            asset->xxch_size = bits_get(&exss->bits, 14) + 1;
        if (asset->extension_mask & EXSS_X96)
            // Size of X96 extension in extension substream
            asset->x96_size = bits_get(&exss->bits, 12) + 1;
        if (asset->extension_mask & EXSS_LBR)
            parse_lbr_parameters(asset);
        if (asset->extension_mask & EXSS_XLL)
            parse_xll_parameters(asset);
        if (asset->extension_mask & EXSS_RSV1)
            bits_skip(&exss->bits, 16);
        if (asset->extension_mask & EXSS_RSV2)
            bits_skip(&exss->bits, 16);
        break;

    case 1: // Loss-less coding mode without CBR component
        asset->extension_mask = EXSS_XLL;
        parse_xll_parameters(asset);
        break;

    case 2: // Low bit rate mode
        asset->extension_mask = EXSS_LBR;
        parse_lbr_parameters(asset);
        break;

    case 3: // Auxiliary coding mode
        asset->extension_mask = 0;
        // Size of auxiliary coded data
        bits_skip(&exss->bits, 14);
        // Auxiliary codec identification
        bits_skip(&exss->bits, 8);
        // Aux sync word present flag
        if (bits_get1(&exss->bits))
            // Aux sync distance
            bits_skip(&exss->bits, 3);
        break;
    }

    if (asset->extension_mask & EXSS_XLL)
        // DTS-HD stream ID
        asset->hd_stream_id = bits_get(&exss->bits, 3);

    // One to one mixing flag
    // Per channel main audio scaling flag
    // Main audio scaling codes
    // Decode asset in secondary decoder flag
    // Revision 2 DRC metadata
    // Reserved
    // Zero pad
    return bits_seek(&exss->bits, descr_pos + descr_size * 8);
}

static int set_exss_offsets(struct exss_asset *asset)
{
    size_t offs = asset->asset_offset;
    size_t size = asset->asset_size;

    if (asset->extension_mask & EXSS_CORE) {
        asset->core_offset = offs;
        if (offs & 3 || asset->core_size > size)
            return -DCADEC_EBADREAD;
        offs += asset->core_size;
        size -= asset->core_size;
    }

    if (asset->extension_mask & EXSS_XBR) {
        asset->xbr_offset = offs;
        if (offs & 3 || asset->xbr_size > size)
            return -DCADEC_EBADREAD;
        offs += asset->xbr_size;
        size -= asset->xbr_size;
    }

    if (asset->extension_mask & EXSS_XXCH) {
        asset->xxch_offset = offs;
        if (offs & 3 || asset->xxch_size > size)
            return -DCADEC_EBADREAD;
        offs += asset->xxch_size;
        size -= asset->xxch_size;
    }

    if (asset->extension_mask & EXSS_X96) {
        asset->x96_offset = offs;
        if (offs & 3 || asset->x96_size > size)
            return -DCADEC_EBADREAD;
        offs += asset->x96_size;
        size -= asset->x96_size;
    }

    if (asset->extension_mask & EXSS_LBR) {
        asset->lbr_offset = offs;
        if (offs & 3 || asset->lbr_size > size)
            return -DCADEC_EBADREAD;
        offs += asset->lbr_size;
        size -= asset->lbr_size;
    }

    if (asset->extension_mask & EXSS_XLL) {
        asset->xll_offset = offs;
        if (offs & 3 || asset->xll_size > size)
            return -DCADEC_EBADREAD;
        offs += asset->xll_size;
        size -= asset->xll_size;
    }

    return 0;
}

int exss_parse(struct exss_parser *exss, uint8_t *data, size_t size)
{
    int i, j, ret;

    bits_init(&exss->bits, data, size);

    // Extension substream sync word
    bits_skip(&exss->bits, 32);

    // User defined bits
    bits_skip(&exss->bits, 8);

    // Extension substream index
    exss->exss_index = bits_get(&exss->bits, 2);

    // Flag indicating short or long header size
    bool wide_hdr = bits_get1(&exss->bits);

    // Extension substream header length
    size_t header_size = bits_get(&exss->bits, 8 + 4 * wide_hdr) + 1;

    // Check CRC
    if ((ret = bits_check_crc(&exss->bits, 32 + 8, header_size * 8)) < 0)
        return ret;

    exss->exss_size_nbits = 16 + 4 * wide_hdr;

    // Number of bytes of extension substream
    exss->exss_size = bits_get(&exss->bits, exss->exss_size_nbits) + 1;
    enforce(exss->exss_size <= size, "Invalid EXSS size");

    // Per stream static fields presence flag
    exss->static_fields_present = bits_get1(&exss->bits);
    if (exss->static_fields_present) {
        // Reference clock code
        bits_skip(&exss->bits, 2);

        // Extension substream frame duration
        bits_skip(&exss->bits, 3);

        // Timecode presence flag
        if (bits_get1(&exss->bits)) {
            // Timecode data
            bits_skip(&exss->bits, 32);
            bits_skip(&exss->bits, 4);
        }

        // Number of defined audio presentations
        exss->npresents = bits_get(&exss->bits, 3) + 1;

        // Number of audio assets in extension substream
        exss->nassets = bits_get(&exss->bits, 3) + 1;

        // Active extension substream mask for audio presentation
        int active_exss_mask[8];
        for (i = 0; i < exss->npresents; i++)
            active_exss_mask[i] = bits_get(&exss->bits, exss->exss_index + 1);

        // Active audio asset mask
        for (i = 0; i < exss->npresents; i++)
            for (j = 0; j <= exss->exss_index; j++)
                if (active_exss_mask[i] & (1 << j))
                    bits_skip(&exss->bits, 8);

        // Mixing metadata enable flag
        exss->mix_metadata_enabled = bits_get1(&exss->bits);
        if (exss->mix_metadata_enabled) {
            // Mixing metadata adjustment level
            bits_skip(&exss->bits, 2);

            // Number of bits for mixer output speaker activity mask
            int spkr_mask_nbits = (bits_get(&exss->bits, 2) + 1) << 2;

            // Number of mixing configurations
            exss->nmixoutconfigs = bits_get(&exss->bits, 2) + 1;

            // Speaker layout mask for mixer output channels
            for (i = 0; i < exss->nmixoutconfigs; i++)
                exss->nmixoutchs[i] = count_chs_for_mask(bits_get(&exss->bits, spkr_mask_nbits));
        }
    } else {
        exss->npresents = 1;
        exss->nassets = 1;
    }

    // Reject unsupported features for now
    if (exss->exss_index > 0 || exss->npresents != 1 || exss->nassets != 1)
        return -DCADEC_ENOSUP;

    // Reallocate assets
    if (ta_zalloc_fast(exss, &exss->assets, exss->nassets, sizeof(struct exss_asset)) < 0)
        return -DCADEC_ENOMEM;

    // Size of encoded asset data in bytes
    size_t offset = header_size;
    for (i = 0; i < exss->nassets; i++) {
        exss->assets[i].asset_offset = offset;
        exss->assets[i].asset_size = bits_get(&exss->bits, exss->exss_size_nbits) + 1;
        offset += exss->assets[i].asset_size;
        enforce(offset <= exss->exss_size, "EXSS asset out of bounds");
    }

    // Audio asset descriptor
    for (i = 0; i < exss->nassets; i++) {
        exss->assets[i].parser = exss;
        if ((ret = parse_descriptor(&exss->assets[i])) < 0)
            return ret;
        if ((ret = set_exss_offsets(&exss->assets[i])) < 0)
            return ret;
    }

    // Backward compatible core present
    // Backward compatible core substream index
    // Backward compatible core asset index
    // Reserved
    // Byte align
    // CRC16 of extension substream header
    return bits_seek(&exss->bits, header_size * 8);
}

struct dcadec_exss_info *exss_get_info(struct exss_parser *exss)
{
    struct dcadec_exss_info *info = ta_znew(NULL, struct dcadec_exss_info);
    if (!info)
        return NULL;

    struct exss_asset *asset = &exss->assets[0];

    info->nchannels = asset->nchannels_total;
    info->sample_rate = asset->max_sample_rate;
    info->bits_per_sample = asset->pcm_bit_res;

    if (asset->extension_mask & EXSS_XLL)
        info->profile = DCADEC_PROFILE_HD_MA;
    else if (asset->extension_mask & (EXSS_XBR | EXSS_XXCH | EXSS_X96))
        info->profile = DCADEC_PROFILE_HD_HRA;
    else if (asset->extension_mask & EXSS_LBR)
        info->profile = DCADEC_PROFILE_EXPRESS;
    else
        info->profile = DCADEC_PROFILE_UNKNOWN;

    info->embedded_stereo = asset->embedded_stereo;
    info->embedded_6ch = asset->embedded_6ch;

    if (asset->spkr_mask_enabled)
        info->spkr_mask = asset->spkr_mask;
    else if (asset->nchannels_total == 2)
        info->spkr_mask = SPEAKER_PAIR_LR;

    if (!asset->one_to_one_map_ch_to_spkr) {
        if (asset->representation_type == REPR_TYPE_LtRt)
            info->matrix_encoding = DCADEC_MATRIX_ENCODING_SURROUND;
        else if (asset->representation_type == REPR_TYPE_LhRh)
            info->matrix_encoding = DCADEC_MATRIX_ENCODING_HEADPHONE;
    }

    return info;
}

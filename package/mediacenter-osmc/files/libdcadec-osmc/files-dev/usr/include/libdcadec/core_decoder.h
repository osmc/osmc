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

#ifndef CORE_DECODER_H
#define CORE_DECODER_H

#include "bitstream.h"

#define MAX_CHANNELS            7
#define MAX_SUBBANDS            32
#define MAX_LFE_SAMPLES         16
#define MAX_SUBFRAMES           16
#define NUM_SUBBAND_SAMPLES     8
#define NUM_PCMBLOCK_SAMPLES    32
#define NUM_ADPCM_COEFFS        4
#define NUM_CODE_BOOKS          10

#define MAX_SUBBANDS_X96        64

struct core_decoder;
struct exss_asset;
struct dcadec_core_info;

struct x96_decoder {
    struct core_decoder *core;

    int     rev_no;
    bool    crc_present;
    int     nchannels;
    bool    high_res;
    int     subband_start;
    int     nsubbands[MAX_CHANNELS];
    int     joint_intensity_index[MAX_CHANNELS];
    int     scale_factor_sel[MAX_CHANNELS];
    int     bit_allocation_sel[MAX_CHANNELS];
    int     quant_index_sel[MAX_CHANNELS][NUM_CODE_BOOKS];

    bool    prediction_mode[MAX_CHANNELS][MAX_SUBBANDS_X96];
    int     prediction_vq_index[MAX_CHANNELS][MAX_SUBBANDS_X96];
    int     bit_allocation[MAX_CHANNELS][MAX_SUBBANDS_X96];
    int     scale_factors[MAX_CHANNELS][MAX_SUBBANDS_X96];
    int     joint_scale_sel[MAX_CHANNELS];
    int     joint_scale_factors[MAX_CHANNELS][MAX_SUBBANDS_X96];

    int     rand;

    int     *subband_buffer;
    int     *subband_samples[MAX_CHANNELS][MAX_SUBBANDS_X96];
};

struct core_decoder {
    struct bitstream    bits;

    bool    normal_frame;
    int     deficit_samples;
    bool    crc_present;
    int     npcmblocks;
    size_t  frame_size;
    int     audio_mode;
    int     sample_rate;
    int     bit_rate;
    bool    drc_present;
    bool    ts_present;
    bool    aux_present;
    int     ext_audio_type;
    bool    ext_audio_present;
    bool    sync_ssf;
    int     lfe_present;
    bool    predictor_history;
    bool    filter_perfect;
    int     source_pcm_res;
    bool    es_format;
    bool    sumdiff_front;
    bool    sumdiff_surround;

    int     nsubframes;
    int     nsubsubframes[MAX_SUBFRAMES];

    int     nchannels;
    int     ch_mask;

    bool    dmix_coeffs_present;
    bool    dmix_embedded;
    int     dmix_scale_inv;
    int     dmix_mask[MAX_CHANNELS];
    int     dmix_coeff[64];

    int     nsubbands[MAX_CHANNELS];
    int     subband_vq_start[MAX_CHANNELS];
    int     joint_intensity_index[MAX_CHANNELS];
    int     transition_mode_sel[MAX_CHANNELS];
    int     scale_factor_sel[MAX_CHANNELS];
    int     bit_allocation_sel[MAX_CHANNELS];
    int     quant_index_sel[MAX_CHANNELS][NUM_CODE_BOOKS];
    int     scale_factor_adj[MAX_CHANNELS][NUM_CODE_BOOKS];

    bool    prediction_mode[MAX_CHANNELS][MAX_SUBBANDS];
    int     prediction_vq_index[MAX_CHANNELS][MAX_SUBBANDS];
    int     bit_allocation[MAX_CHANNELS][MAX_SUBBANDS];
    int     transition_mode[MAX_SUBFRAMES][MAX_CHANNELS][MAX_SUBBANDS];
    int     scale_factors[MAX_CHANNELS][MAX_SUBBANDS][2];
    int     joint_scale_sel[MAX_CHANNELS];
    int     joint_scale_factors[MAX_CHANNELS][MAX_SUBBANDS];

    int                 *subband_buffer;
    int                 *subband_samples[MAX_CHANNELS][MAX_SUBBANDS];
    struct interpolator *subband_dsp[MAX_CHANNELS];
    struct idct_context *subband_dsp_idct;

    int     *lfe_samples;

    bool    prim_dmix_embedded;
    int     prim_dmix_type;
    int     prim_dmix_coeff[4 * 6];

    int     ext_audio_mask;

    size_t  xch_pos;

    bool    xxch_crc_present;
    int     xxch_mask_nbits;
    int     xxch_core_mask;
    int     xxch_spkr_mask;
    size_t  xxch_pos;

    struct x96_decoder  *x96_decoder;
    size_t              x96_pos;

    int     *output_buffer;
    int     *output_samples[SPEAKER_COUNT];
    int     output_history_lfe;

    int     npcmsamples;
    int     output_rate;

    int     filter_flags;
};

int core_parse(struct core_decoder *core, uint8_t *data, size_t size,
               int flags, struct exss_asset *asset);
int core_parse_exss(struct core_decoder *core, uint8_t *data, size_t size,
                    int flags, struct exss_asset *asset);
int core_filter(struct core_decoder *core, int flags);
void core_clear(struct core_decoder *core);
struct dcadec_core_info *core_get_info(struct core_decoder *core);
struct dcadec_exss_info *core_get_info_exss(struct core_decoder *core);

#endif

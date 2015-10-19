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

#ifndef XLL_DECODER_H
#define XLL_DECODER_H

#include "bitstream.h"

#define XLL_MAX_CHSETS      16
#define XLL_MAX_CHANNELS    8
#define XLL_MAX_BANDS       2

#define XLL_DECI_HISTORY    8

#define XLL_BAND_0  0
#define XLL_BAND_1  1

#define for_each_chset(xll, chs) \
    for (struct xll_chset *(chs) = (xll)->chset; \
         (chs) != &(xll)->chset[(xll)->nchsets]; (chs)++)

#define for_each_active_chset(xll, chs) \
    for (struct xll_chset *(chs) = (xll)->chset; \
         (chs) != &(xll)->chset[(xll)->nactivechsets]; (chs)++)

struct xll_decoder;
struct exss_asset;

struct xll_chset {
    struct xll_decoder  *decoder;

    int     nchannels;
    int     residual_encode;
    int     pcm_bit_res;
    int     storage_bit_res;
    int     freq;
    int     interpolate;
    int     replace_set_index;

    bool    primary_chset;
    bool    dmix_coeffs_present;
    bool    dmix_embedded;
    int     dmix_type;
    bool    hier_chset;
    int     dmix_m;
    int     *dmix_coeff;
    int     *dmix_scale;
    int     *dmix_scale_inv;
    bool    ch_mask_enabled;
    int     ch_mask;

    int     nfreqbands;
    int     nabits;

    bool    decor_enabled[XLL_MAX_BANDS];
    int     orig_order[XLL_MAX_BANDS][XLL_MAX_CHANNELS];
    int     decor_coeff[XLL_MAX_BANDS][XLL_MAX_CHANNELS / 2];

    int     adapt_pred_order[XLL_MAX_BANDS][XLL_MAX_CHANNELS];
    int     highest_pred_order[XLL_MAX_BANDS];
    int     fixed_pred_order[XLL_MAX_BANDS][XLL_MAX_CHANNELS];
    int     adapt_refl_coeff[XLL_MAX_BANDS][XLL_MAX_CHANNELS][16];

    bool    band_dmix_embedded[XLL_MAX_BANDS];

    size_t  lsb_section_size[XLL_MAX_BANDS];
    int     nscalablelsbs[XLL_MAX_BANDS][XLL_MAX_CHANNELS];
    int     bit_width_adjust[XLL_MAX_BANDS][XLL_MAX_CHANNELS];

    bool    seg_type;
    bool    rice_code_flag[XLL_MAX_CHANNELS];
    int     bitalloc_hybrid_linear[XLL_MAX_CHANNELS];
    int     bitalloc_part_a[XLL_MAX_CHANNELS];
    int     bitalloc_part_b[XLL_MAX_CHANNELS];
    int     nsamples_part_a[XLL_MAX_CHANNELS];

    int     deci_history[XLL_MAX_CHANNELS][XLL_DECI_HISTORY];

    int     *msb_sample_buffer[XLL_MAX_BANDS][XLL_MAX_CHANNELS];
    int     *lsb_sample_buffer[XLL_MAX_BANDS][XLL_MAX_CHANNELS];
    int     *out_sample_buffer[XLL_MAX_CHANNELS];

    int     *sample_buffer1;
    int     *sample_buffer2;
    int     *sample_buffer3;
};

struct xll_decoder {
    struct bitstream    bits;

    int     flags;

    size_t  frame_size;
    int     nchsets;
    int     nframesegs;
    int     nsegsamples_log2;
    int     nsegsamples;
    int     nframesamples;
    int     seg_size_nbits;
    int     band_crc_present;
    bool    scalable_lsbs;
    int     ch_mask_nbits;
    int     fixed_lsb_width;

    struct xll_chset    *chset;

    size_t  *navi;

    int     nfreqbands;
    int     nchannels;
    int     nactivechsets;

    int     hd_stream_id;

    uint8_t     *pbr_buffer;
    size_t      pbr_length;
    int         pbr_delay;
};

void xll_clear_band_data(struct xll_chset *chs, int band);
void xll_filter_band_data(struct xll_chset *chs, int band);
int xll_get_lsb_width(struct xll_chset *chs, int band, int ch);
void xll_assemble_msbs_lsbs(struct xll_chset *chs, int band);
int xll_assemble_freq_bands(struct xll_chset *chs);
int xll_map_ch_to_spkr(struct xll_chset *chs, int ch);
int xll_parse(struct xll_decoder *xll, uint8_t *data, size_t size, struct exss_asset *asset);
void xll_clear(struct xll_decoder *xll);

#endif

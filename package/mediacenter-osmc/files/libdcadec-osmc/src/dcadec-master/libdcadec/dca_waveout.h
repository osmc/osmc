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

#ifndef WAVEOUT_H
#define WAVEOUT_H

#include "dca_context.h"

/**@{*/
/** Write a mono WAV file for each native DTS channel */
#define DCADEC_WAVEOUT_FLAG_MONO    0x01

/** Clip instead of raising PCM overflow error */
#define DCADEC_WAVEOUT_FLAG_CLIP    0x02
/**@}*/

struct dcadec_waveout;

/**
 * Write the block of PCM samples to WAV file. The first call to this function
 * writes the WAV header. Subsequent calls must have audio parameters identical
 * to the first call, excluding nsamples parameter which can change between
 * calls.
 *
 * @param wave    Writer handle.
 *
 * @param samples   Array of pointers to planes containing PCM data for active
 *                  channels. Normally, channels must be ordered according to
 *                  WAVEFORMATEXTENSIBLE specification. However, when
 *                  DCADEC_WAVEOUT_FLAG_MONO is set, native DTS channel layout
 *                  should be provided.
 *
 * @param nsamples  Number of PCM samples in each plane.
 *
 * @param channel_mask  Bit mask indicating active channels. Number of bits set
 *                      to 1 indicates the total number of planes to write.
 *
 * @param sample_rate       Audio sample rate in Hz.
 *
 * @param bits_per_sample   Audio PCM resolution in bits.
 *
 * @return      0 on success, negative error code on failure. Positive return
 *              value indicates number of out-of-range PCM samples that were
 *              clipped.
 */
DCADEC_API int dcadec_waveout_write(struct dcadec_waveout *wave, int **samples,
                                    int nsamples, int channel_mask,
                                    int sample_rate, int bits_per_sample);
/**
 * Open WAV writer to file or standard output.
 *
 * @param name  Name of the file to be opened. Pass NULL to open standard
 *              output. When DCADEC_WAVEOUT_FLAG_MONO is set, name must be
 *              non-NULL and must include `%s' sub-string that will be replaced
 *              with DTS channel name.
 *
 * @param flags Any number of DCADEC_WAVEOUT_FLAG_* constants OR'ed together.
 *
 * @return      Writer handle on success, NULL on failure.
 */
DCADEC_API struct dcadec_waveout *dcadec_waveout_open(const char *name, int flags);

/**
 * Close WAV writer. This function updates the WAV header if possible.
 *
 * @param wave    Writer handle.
 */
DCADEC_API void dcadec_waveout_close(struct dcadec_waveout *wave);

#endif

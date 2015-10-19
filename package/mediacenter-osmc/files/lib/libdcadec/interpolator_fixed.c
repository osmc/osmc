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
#include "interpolator.h"
#include "idct.h"
#include "fixed_math.h"
#include "fir_fixed.h"

INTERPOLATE_LFE(lfe_fixed_fir)
{
    // Select decimation factor
    int dec_factor = 64 << dec_select;

    // Interpolation
    for (int i = 0; i < nsamples; i++) {
        // One decimated sample generates 64 or 128 interpolated ones
        for (int j = 0; j < dec_factor; j++) {
            // Clear accumulation
            int64_t res = INT64_C(0);

            // Accumulate
            for (int k = 0; k < 512 / dec_factor; k++)
                res += (int64_t)lfe_fir_64[k * dec_factor + j] *
                    lfe_samples[MAX_LFE_HISTORY + i - k];

            // Save interpolated samples
            pcm_samples[(i * dec_factor + j) << synth_x96] = clip23(norm23(res));
        }
    }

    // Update history
    for (int n = MAX_LFE_HISTORY - 1; n >= 0; n--)
        lfe_samples[n] = lfe_samples[nsamples + n];
}

INTERPOLATE_SUB(sub32_fixed)
{
    (void)subband_samples_hi;
    assert(subband_samples_hi == NULL);

    // Get history pointer
    int *history = dsp->history;

    // Select filter
    const int32_t *filter_coeff = perfect ?
        band_fir_perfect : band_fir_nonperfect;

    // Interpolation begins
    for (int sample = 0; sample < nsamples; sample++) {
        int i, j, k;

        // Load in one sample from each subband
        int input[32];
        for (i = 0; i < 32; i++)
            input[i] = subband_samples_lo[i][sample];

        // Inverse DCT
        int output[32];
        idct_perform32_fixed(input, output);

        // Store history
        for (i = 0, k = 31; i < 16; i++, k--) {
            history[     i] = clip23(output[i] - output[k]);
            history[16 + i] = clip23(output[i] + output[k]);
        }

        // One subband sample generates 32 interpolated ones
        for (i = 0; i < 16; i++) {
            // Clear accumulation
            int64_t res = INT64_C(0);

            // Accumulate
            for (j = 32; j < 512; j += 64)
                res += (int64_t)history[16 + i + j] * filter_coeff[i + j];
            res = round21(res);
            for (j =  0; j < 512; j += 64)
                res += (int64_t)history[     i + j] * filter_coeff[i + j];

            // Save interpolated samples
            pcm_samples[sample * 32 + i] = clip23(norm21(res));
        }

        for (i = 16, k = 15; i < 32; i++, k--) {
            // Clear accumulation
            int64_t res = INT64_C(0);

            // Accumulate
            for (j = 32; j < 512; j += 64)
                res += (int64_t)history[16 + k + j] * filter_coeff[i + j];
            res = round21(res);
            for (j =  0; j < 512; j += 64)
                res += (int64_t)history[     k + j] * filter_coeff[i + j];

            // Save interpolated samples
            pcm_samples[sample * 32 + i] = clip23(norm21(res));
        }

        // Shift history
        for (i = 511; i >= 32; i--)
            history[i] = history[i - 32];
    }
}

INTERPOLATE_SUB(sub64_fixed)
{
    (void)perfect;

    // Get history pointer
    int *history = dsp->history;

    // Interpolation begins
    for (int sample = 0; sample < nsamples; sample++) {
        int i, j, k;

        // Load in one sample from each subband
        int input[64];
        if (subband_samples_hi) {
            // Full 64 subbands, first 32 are residual coded
            for (i =  0; i < 32; i++)
                input[i] = subband_samples_lo[i][sample] + subband_samples_hi[i][sample];
            for (i = 32; i < 64; i++)
                input[i] = subband_samples_hi[i][sample];
        } else {
            // Only first 32 subbands
            for (i =  0; i < 32; i++)
                input[i] = subband_samples_lo[i][sample];
            for (i = 32; i < 64; i++)
                input[i] = 0;
        }

        // Inverse DCT
        int output[64];
        idct_perform64_fixed(input, output);

        // Store history
        for (i = 0, k = 63; i < 32; i++, k--) {
            history[     i] = clip23(output[i] - output[k]);
            history[32 + i] = clip23(output[i] + output[k]);
        }

        // One subband sample generates 64 interpolated ones
        for (i = 0; i < 32; i++) {
            // Clear accumulation
            int64_t res = INT64_C(0);

            // Accumulate
            for (j = 64; j < 1024; j += 128)
                res += (int64_t)history[32 + i + j] * band_fir_x96[i + j];
            res = round20(res);
            for (j =  0; j < 1024; j += 128)
                res += (int64_t)history[     i + j] * band_fir_x96[i + j];

            // Save interpolated samples
            pcm_samples[sample * 64 + i] = clip23(norm20(res));
        }

        for (i = 32, k = 31; i < 64; i++, k--) {
            // Clear accumulation
            int64_t res = INT64_C(0);

            // Accumulate
            for (j = 64; j < 1024; j += 128)
                res += (int64_t)history[32 + k + j] * band_fir_x96[i + j];
            res = round20(res);
            for (j =  0; j < 1024; j += 128)
                res += (int64_t)history[     k + j] * band_fir_x96[i + j];

            // Save interpolated samples
            pcm_samples[sample * 64 + i] = clip23(norm20(res));
        }

        // Shift history
        for (i = 1023; i >= 64; i--)
            history[i] = history[i - 64];
    }
}

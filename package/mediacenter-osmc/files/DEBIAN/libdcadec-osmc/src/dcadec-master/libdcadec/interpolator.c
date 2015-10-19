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

struct interpolator *interpolator_create(struct idct_context *parent, int flags)
{
    struct interpolator *dsp = ta_new(parent, struct interpolator);
    if (!dsp)
        return NULL;

    dsp->idct = parent;
    dsp->history = ta_znew_array_size(dsp,
        (flags & DCADEC_FLAG_CORE_BIT_EXACT) ? sizeof(int) : sizeof(double),
        (flags & DCADEC_FLAG_CORE_SYNTH_X96) ? 1024 : 512);
    if (!dsp->history) {
        ta_free(dsp);
        return NULL;
    }

    if (flags & DCADEC_FLAG_CORE_BIT_EXACT) {
        if (flags & DCADEC_FLAG_CORE_SYNTH_X96)
            dsp->interpolate = interpolate_sub64_fixed;
        else
            dsp->interpolate = interpolate_sub32_fixed;
    } else {
        if (flags & DCADEC_FLAG_CORE_SYNTH_X96)
            dsp->interpolate = interpolate_sub64_float;
        else
            dsp->interpolate = interpolate_sub32_float;
    }

    return dsp;
}

void interpolator_clear(struct interpolator *dsp)
{
    if (dsp)
        memset(dsp->history, 0, ta_get_size(dsp->history));
}

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

#ifndef IDCT_H
#define IDCT_H

struct core_decoder;

struct idct_context {
    double dct_a[8][8];
    double dct_b[8][7];

    double mod_a[16];
    double mod_b[ 8];
    double mod_c[32];

    double mod64_a[32];
    double mod64_b[16];
    double mod64_c[64];
};

struct idct_context *idct_init(struct core_decoder *parent);

void idct_perform32_float(const struct idct_context * restrict idct,
                          double * restrict input, double * restrict output);
void idct_perform64_float(const struct idct_context * restrict idct,
                          double * restrict input, double * restrict output);

void idct_perform32_fixed(int * restrict input, int * restrict output);
void idct_perform64_fixed(int * restrict input, int * restrict output);

#endif

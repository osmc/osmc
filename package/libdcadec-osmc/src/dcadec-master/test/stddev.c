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

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#define BUF 3000
#define HDR 44

int main(int argc, char **argv)
{
    uint8_t buf1[BUF], buf2[BUF], *p1, *p2;
    int64_t acc = 0, cnt = 0;
    size_t r1, r2, i;
    double ref = 0.0;

    if (argc != 4)
        goto fail;

    if (strcmp(argv[3], "?")) {
        char *p;
        ref = strtod(argv[3], &p);
        if (*p || p == argv[3])
            goto fail;
    }

    FILE *fp1 = fopen(argv[1], "rb");
    FILE *fp2 = fopen(argv[2], "rb");
    if (!fp1 || !fp2)
        goto fail;

    if (fread(buf1, HDR, 1, fp1) != 1 || fread(buf2, HDR, 1, fp2) != 1)
        goto fail;

    if (memcmp(buf1, buf2, HDR) || memcmp(buf1, "RIFF", 4))
        goto fail;

    int bps;
    switch (buf1[34] | (buf1[35] << 8)) {
    case 16:
        bps = 2;
        break;
    case 24:
        bps = 3;
        break;
    default:
        goto fail;
    }

    do {
        r1 = fread(buf1, 1, BUF, fp1);
        r2 = fread(buf2, 1, BUF, fp2);
        if (r1 != r2 || r1 % bps)
            goto fail;

        for (i = 0, p1 = buf1, p2 = buf2; i < r1 / bps; i++, p1 += bps, p2 += bps) {
            int64_t d;

            if (bps == 3) {
                uint32_t u1 = (p1[0] << 8) | (p1[1] << 16) | ((uint32_t)p1[2] << 24);
                uint32_t u2 = (p2[0] << 8) | (p2[1] << 16) | ((uint32_t)p2[2] << 24);
                int32_t v1 = (int32_t)u1 >> 8;
                int32_t v2 = (int32_t)u2 >> 8;
                d = v1 - v2;
            } else {
                int16_t v1 = p1[0] | (p1[1] << 8);
                int16_t v2 = p2[0] | (p2[1] << 8);
                d = v1 - v2;
            }

            acc += d * d;
        }

        cnt += i;
    } while (r1 == BUF);

    if (!cnt)
        goto fail;

    double var = (double)acc / cnt;
    double dev = sqrt(var);
    if (strcmp(argv[3], "?")) {
        if (fabs(dev - ref) > 0.1)
            goto fail;
        printf("%s: OK\n", argv[1]);
    } else {
        printf("%s: %f\n", argv[1], dev);
    }

    return 0;

fail:
    printf("%s: FAILED\n", argc > 1 ? argv[1] : "???");
    return 1;
}

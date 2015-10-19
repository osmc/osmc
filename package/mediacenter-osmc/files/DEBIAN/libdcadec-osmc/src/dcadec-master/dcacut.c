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
#include <stdbool.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

#include "libdcadec/dca_stream.h"

int main(int argc, char **argv)
{
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <input.dts> <output.dts> [first] [last]\n", argv[0]);
        return 1;
    }

    unsigned long first_packet = 0;
    unsigned long last_packet = ULONG_MAX;

    if (argc > 3)
        first_packet = strtoul(argv[3], NULL, 0);
    if (argc > 4)
        last_packet = strtoul(argv[4], NULL, 0);
    if (last_packet < first_packet) {
        fprintf(stderr, "Invalid packet range\n");
        return 1;
    }

    struct dcadec_stream *stream = dcadec_stream_open(argv[1], 0);
    if (!stream) {
        fprintf(stderr, "Couldn't open input file\n");
        return 1;
    }

    FILE *fp = fopen(argv[2], "wb");
    if (!fp) {
        fprintf(stderr, "Couldn't open output file\n");
        dcadec_stream_close(stream);
        return 1;
    }

    unsigned long packet_in = 0;
    unsigned long packet_out = 0;

    int ret;

    while (true) {
        uint8_t *packet;
        size_t size;

        ret = dcadec_stream_read(stream, &packet, &size);
        if (ret < 0) {
            fprintf(stderr, "Error %d reading packet\n", ret);
            break;
        }
        if (ret == 0)
            break;

        if (packet_in >= first_packet) {
            if (fwrite(packet, size, 1, fp) != 1) {
                fprintf(stderr, "Error %d writing packet\n", errno);
                ret = -1;
                break;
            }
            packet_out++;
        }

        if (++packet_in > last_packet)
            break;
    }

    if (packet_out) {
        fprintf(stderr, "Wrote %lu packets\n", packet_out);
    } else {
        fprintf(stderr, "Didn't write a single packet!\n");
        ret = -1;
    }

    fclose(fp);
    dcadec_stream_close(stream);
    return !!ret;
}

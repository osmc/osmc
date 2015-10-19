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
#include "bitstream.h"
#include "dca_frame.h"

#define SRC_OP(E) \
    uint16_t src_0 = DCA_16##E(_src[0]); \
    uint16_t src_1 = DCA_16##E(_src[1]); \
    uint16_t src_2 = DCA_16##E(_src[2]); \
    uint16_t src_3 = DCA_16##E(_src[3]); \
    uint16_t src_4 = DCA_16##E(_src[4]); \
    uint16_t src_5 = DCA_16##E(_src[5]); \
    uint16_t src_6 = DCA_16##E(_src[6]); \
    uint16_t src_7 = DCA_16##E(_src[7]);

#define DST_OP \
    _dst[0] = DCA_16BE((src_0 <<  2) | ((src_1 & 0x3fff) >> 12)); \
    _dst[1] = DCA_16BE((src_1 <<  4) | ((src_2 & 0x3fff) >> 10)); \
    _dst[2] = DCA_16BE((src_2 <<  6) | ((src_3 & 0x3fff) >>  8)); \
    _dst[3] = DCA_16BE((src_3 <<  8) | ((src_4 & 0x3fff) >>  6)); \
    _dst[4] = DCA_16BE((src_4 << 10) | ((src_5 & 0x3fff) >>  4)); \
    _dst[5] = DCA_16BE((src_5 << 12) | ((src_6 & 0x3fff) >>  2)); \
    _dst[6] = DCA_16BE((src_6 << 14) | ((src_7 & 0x3fff) >>  0));

DCADEC_API int dcadec_frame_convert_bitstream(uint8_t *dst, size_t *dst_size,
                                              const uint8_t *src, size_t src_size)
{
    const uint16_t *_src = (const uint16_t *)src;
    uint16_t *_dst = (uint16_t *)dst;
    size_t count;

    if ((uintptr_t)_dst & 3)
        return -DCADEC_EINVAL;

    if ((uintptr_t)_src & 1)
        _src = memcpy(_dst, _src, src_size);

    switch (DCA_MEM32NE(_src)) {
    case DCA_32BE_C(SYNC_WORD_CORE):
    case DCA_32BE_C(SYNC_WORD_EXSS):
        if (_src != _dst)
            memcpy(_dst, _src, src_size);
        *dst_size = src_size;
        return DCADEC_BITSTREAM_BE16;

    case DCA_32BE_C(SYNC_WORD_CORE_LE):
    case DCA_32BE_C(SYNC_WORD_EXSS_LE):
        count = (src_size + 1) / 2;
        while (count--)
            *_dst++ = dca_bswap16(*_src++);
        *dst_size = src_size;
        return DCADEC_BITSTREAM_LE16;

    case DCA_32BE_C(SYNC_WORD_CORE_BE14):
        count = (src_size + 15) / 16;
        while (count--) {
            SRC_OP(BE)
            DST_OP
            _src += 8;
            _dst += 7;
        }
        *dst_size = src_size - src_size / 8;
        return DCADEC_BITSTREAM_BE14;

    case DCA_32BE_C(SYNC_WORD_CORE_LE14):
        count = (src_size + 15) / 16;
        while (count--) {
            SRC_OP(LE)
            DST_OP
            _src += 8;
            _dst += 7;
        }
        *dst_size = src_size - src_size / 8;
        return DCADEC_BITSTREAM_LE14;

    default:
        return -DCADEC_ENOSYNC;
    }
}

#undef SRC_OP
#undef DST_OP

DCADEC_API int dcadec_frame_parse_header(const uint8_t *data, size_t *size)
{
    struct bitstream bits;
    uint8_t header[DCADEC_FRAME_HEADER_SIZE];
    size_t header_size, frame_size;

    int ret;
    if ((ret = dcadec_frame_convert_bitstream(header, &header_size,
                                              data, DCADEC_FRAME_HEADER_SIZE)) < 0)
        return ret;

    bits_init(&bits, header, header_size);

    switch (bits_get(&bits, 32)) {
    case SYNC_WORD_CORE: {
        bool normal_frame = bits_get1(&bits);
        int deficit_samples = bits_get(&bits, 5) + 1;
        if (normal_frame && deficit_samples != 32)
            return -DCADEC_ENOSYNC;
        bits_skip1(&bits);
        int npcmblocks = bits_get(&bits, 7) + 1;
        if (npcmblocks < 6)
            return -DCADEC_ENOSYNC;
        frame_size = bits_get(&bits, 14) + 1;
        if (frame_size < 96)
            return -DCADEC_ENOSYNC;
        if (ret & DCADEC_BITSTREAM_BE14)
            *size = frame_size + frame_size / 7;
        else
            *size = frame_size;
        return DCADEC_FRAME_TYPE_CORE;
    }

    case SYNC_WORD_EXSS: {
        bits_skip(&bits, 10);
        bool wide_hdr = bits_get1(&bits);
        bits_skip(&bits, 8 + 4 * wide_hdr);
        frame_size = bits_get(&bits, 16 + 4 * wide_hdr) + 1;
        if (frame_size < DCADEC_FRAME_HEADER_SIZE)
            return -DCADEC_ENOSYNC;
        if (frame_size & 3)
            return -DCADEC_ENOSYNC;
        *size = frame_size;
        return DCADEC_FRAME_TYPE_EXSS;
    }

    default:
        return -DCADEC_ENOSYNC;
    }
}

DCADEC_API size_t dcadec_frame_buffer_size(size_t size)
{
    size_t padding = -size & (DCADEC_FRAME_BUFFER_ALIGN - 1);
    if (padding < DCADEC_BUFFER_PADDING)
        padding = DCADEC_BUFFER_PADDING;
    if (padding > SIZE_MAX - size)
        padding = SIZE_MAX - size;
    return size + padding;
}

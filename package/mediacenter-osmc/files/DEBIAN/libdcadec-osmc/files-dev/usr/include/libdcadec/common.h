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

#ifndef COMMON_H
#define COMMON_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <assert.h>

#include "compiler.h"
#include "dca_context.h"
#include "ta.h"

#if AT_LEAST_GCC(3, 4)
#define dca_clz32(x)    __builtin_clz(x)
#define dca_clz64(x)    __builtin_clzll(x)
#else
static inline int dca_clz32(uint32_t x)
{
    int r = 0;

    assert(x);
    if (x & 0xffff0000) { x >>= 16; r |= 16; }
    if (x & 0x0000ff00) { x >>=  8; r |=  8; }
    if (x & 0x000000f0) { x >>=  4; r |=  4; }
    if (x & 0x0000000c) { x >>=  2; r |=  2; }
    if (x & 0x00000002) { x >>=  1; r |=  1; }

    return 31 - r;
}

static inline int dca_clz64(uint64_t x)
{
    int r = 0;

    assert(x);
    if (x & 0xffffffff00000000) { x >>= 32; r |= 32; }
    if (x & 0x00000000ffff0000) { x >>= 16; r |= 16; }
    if (x & 0x000000000000ff00) { x >>=  8; r |=  8; }
    if (x & 0x00000000000000f0) { x >>=  4; r |=  4; }
    if (x & 0x000000000000000c) { x >>=  2; r |=  2; }
    if (x & 0x0000000000000002) { x >>=  1; r |=  1; }

    return 63 - r;
}
#endif

#if (defined __GNUC__) && (defined __POPCNT__)
#define dca_popcount(x) __builtin_popcount(x)
#else
static inline int dca_popcount(uint32_t x)
{
    x -= (x >> 1) & 0x55555555;
    x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
    x = (x + (x >> 4)) & 0x0f0f0f0f;
    x += x >> 8;
    return (x + (x >> 16)) & 0x3f;
}
#endif

#define dca_countof(x)  (sizeof(x) / sizeof((x)[0]))

#if AT_LEAST_GCC(4, 8)
#define dca_bswap16(x)  __builtin_bswap16(x)
#else
static inline uint16_t dca_bswap16(uint16_t x)
{
    return (x << 8) | (x >> 8);
}
#endif

#if AT_LEAST_GCC(4, 3)
#define dca_bswap32(x)  __builtin_bswap32(x)
#define dca_bswap64(x)  __builtin_bswap64(x)
#else
static inline uint32_t dca_bswap32(uint32_t x)
{
    x = ((x & 0x00ff00ff) << 8) | ((x & 0xff00ff00) >> 8);
    return (x << 16) | (x >> 16);
}

static inline uint64_t dca_bswap64(uint64_t x)
{
    x = ((x & 0x00ff00ff00ff00ff) <<  8) | ((x & 0xff00ff00ff00ff00) >>  8);
    x = ((x & 0x0000ffff0000ffff) << 16) | ((x & 0xffff0000ffff0000) >> 16);
    return (x << 32) | (x >> 32);
}
#endif

#define DCA_BSWAP16_C(x)    ((((x) & 0x00ff)   <<  8) | (((x) & 0xff00)  >>  8))
#define DCA_BSWAP32_C(x)    ((DCA_BSWAP16_C(x) << 16) | (DCA_BSWAP16_C(x >> 16)))
#define DCA_BSWAP64_C(x)    ((DCA_BSWAP32_C(x) << 32) | (DCA_BSWAP32_C(x >> 32)))

#if HAVE_BIGENDIAN
#define DCA_16LE(x) dca_bswap16(x)
#define DCA_32LE(x) dca_bswap32(x)
#define DCA_64LE(x) dca_bswap64(x)
#define DCA_16BE(x) ((uint16_t)(x))
#define DCA_32BE(x) ((uint32_t)(x))
#define DCA_64BE(x) ((uint64_t)(x))
#define DCA_16LE_C(x)   DCA_BSWAP16_C(x)
#define DCA_32LE_C(x)   DCA_BSWAP32_C(x)
#define DCA_64LE_C(x)   DCA_BSWAP64_C(x)
#define DCA_16BE_C(x)   (x)
#define DCA_32BE_C(x)   (x)
#define DCA_64BE_C(x)   (x)
#else
#define DCA_16LE(x) ((uint16_t)(x))
#define DCA_32LE(x) ((uint32_t)(x))
#define DCA_64LE(x) ((uint64_t)(x))
#define DCA_16BE(x) dca_bswap16(x)
#define DCA_32BE(x) dca_bswap32(x)
#define DCA_64BE(x) dca_bswap64(x)
#define DCA_16LE_C(x)   (x)
#define DCA_32LE_C(x)   (x)
#define DCA_64LE_C(x)   (x)
#define DCA_16BE_C(x)   DCA_BSWAP16_C(x)
#define DCA_32BE_C(x)   DCA_BSWAP32_C(x)
#define DCA_64BE_C(x)   DCA_BSWAP64_C(x)
#endif

#define DCA_MIN(a, b)   ((a) < (b) ? (a) : (b))
#define DCA_MAX(a, b)   ((a) > (b) ? (a) : (b))

#define DCA_ALIGN(value, align) \
    (((value) + (align) - 1) & ~((align) - 1))

#define DCA_MEM16BE(data) \
    (((uint32_t)(data)[0] <<  8) | (data)[1])

#define DCA_MEM24BE(data) \
    (((uint32_t)(data)[0] << 16) | DCA_MEM16BE(&(data)[1]))

#define DCA_MEM32BE(data) \
    (((uint32_t)(data)[0] << 24) | DCA_MEM24BE(&(data)[1]))

#define DCA_MEM40BE(data) \
    (((uint64_t)(data)[0] << 32) | DCA_MEM32BE(&(data)[1]))

static inline uint32_t DCA_MEM32NE(const void *data)
{
    uint32_t res;
    memcpy(&res, data, sizeof(res));
    return res;
}

#ifdef _DEBUG
#define DCA_DEBUG(m) \
    fprintf(stderr, "%s+%d: %s\n", __FILE__, __LINE__, m)
#else
#define DCA_DEBUG(m)
#endif

#define enforce3(x, m, e) \
    do {                        \
        if (!(x)) {             \
            DCA_DEBUG(m);       \
            return -(e);        \
        }                       \
    } while (false)

#define enforce(x, m)   enforce3(x, m, DCADEC_EBADDATA)
#define enforce2(x, m)  enforce3(x, m, DCADEC_ENOSYNC)
#define require(x, m)   enforce3(x, m, DCADEC_ENOSUP)

#define DCADEC_FLAG_KEEP_DMIX_MASK  \
    (DCADEC_FLAG_KEEP_DMIX_2CH | DCADEC_FLAG_KEEP_DMIX_6CH)

enum WaveTag {
    TAG_RIFF    = 0x46464952,
    TAG_WAVE    = 0x45564157,
    TAG_data    = 0x61746164,
    TAG_fmt     = 0x20746d66
};

// WAVEFORMATEXTENSIBLE speakers
enum WaveSpeaker {
    WAVESPKR_FL,  WAVESPKR_FR,  WAVESPKR_FC,  WAVESPKR_LFE,
    WAVESPKR_BL,  WAVESPKR_BR,  WAVESPKR_FLC, WAVESPKR_FRC,
    WAVESPKR_BC,  WAVESPKR_SL,  WAVESPKR_SR,  WAVESPKR_TC,
    WAVESPKR_TFL, WAVESPKR_TFC, WAVESPKR_TFR, WAVESPKR_TBL,
    WAVESPKR_TBC, WAVESPKR_TBR,

    WAVESPKR_COUNT
};

// Table 6-22: Loudspeaker masks
enum SpeakerMask {
    SPEAKER_MASK_C      = 0x00000001,
    SPEAKER_MASK_L      = 0x00000002,
    SPEAKER_MASK_R      = 0x00000004,
    SPEAKER_MASK_Ls     = 0x00000008,
    SPEAKER_MASK_Rs     = 0x00000010,
    SPEAKER_MASK_LFE1   = 0x00000020,
    SPEAKER_MASK_Cs     = 0x00000040,
    SPEAKER_MASK_Lsr    = 0x00000080,
    SPEAKER_MASK_Rsr    = 0x00000100,
    SPEAKER_MASK_Lss    = 0x00000200,
    SPEAKER_MASK_Rss    = 0x00000400,
    SPEAKER_MASK_Lc     = 0x00000800,
    SPEAKER_MASK_Rc     = 0x00001000,
    SPEAKER_MASK_Lh     = 0x00002000,
    SPEAKER_MASK_Ch     = 0x00004000,
    SPEAKER_MASK_Rh     = 0x00008000,
    SPEAKER_MASK_LFE2   = 0x00010000,
    SPEAKER_MASK_Lw     = 0x00020000,
    SPEAKER_MASK_Rw     = 0x00040000,
    SPEAKER_MASK_Oh     = 0x00080000,
    SPEAKER_MASK_Lhs    = 0x00100000,
    SPEAKER_MASK_Rhs    = 0x00200000,
    SPEAKER_MASK_Chr    = 0x00400000,
    SPEAKER_MASK_Lhr    = 0x00800000,
    SPEAKER_MASK_Rhr    = 0x01000000,
    SPEAKER_MASK_Cl     = 0x02000000,
    SPEAKER_MASK_Ll     = 0x04000000,
    SPEAKER_MASK_Rl     = 0x08000000,
    SPEAKER_MASK_RSV1   = 0x10000000,
    SPEAKER_MASK_RSV2   = 0x20000000,
    SPEAKER_MASK_RSV3   = 0x40000000,
    SPEAKER_MASK_RSV4   = 0x80000000
};

// Table 6-22: Loudspeaker masks
enum Speaker {
    SPEAKER_C,    SPEAKER_L,    SPEAKER_R,    SPEAKER_Ls,
    SPEAKER_Rs,   SPEAKER_LFE1, SPEAKER_Cs,   SPEAKER_Lsr,
    SPEAKER_Rsr,  SPEAKER_Lss,  SPEAKER_Rss,  SPEAKER_Lc,
    SPEAKER_Rc,   SPEAKER_Lh,   SPEAKER_Ch,   SPEAKER_Rh,
    SPEAKER_LFE2, SPEAKER_Lw,   SPEAKER_Rw,   SPEAKER_Oh,
    SPEAKER_Lhs,  SPEAKER_Rhs,  SPEAKER_Chr,  SPEAKER_Lhr,
    SPEAKER_Rhr,  SPEAKER_Cl,   SPEAKER_Ll,   SPEAKER_Rl,
    SPEAKER_RSV1, SPEAKER_RSV2, SPEAKER_RSV3, SPEAKER_RSV4,

    SPEAKER_COUNT
};

// Table 7-1: Sync words
enum SyncWord {
    SYNC_WORD_CORE      = 0x7ffe8001,
    SYNC_WORD_CORE_LE   = 0xfe7f0180,
    SYNC_WORD_CORE_LE14 = 0xff1f00e8,
    SYNC_WORD_CORE_BE14 = 0x1fffe800,
    SYNC_WORD_REV1AUX   = 0x9a1105a0,
    SYNC_WORD_REV2AUX   = 0x7004c070,
    SYNC_WORD_XCH       = 0x5a5a5a5a,
    SYNC_WORD_XXCH      = 0x47004a03,
    SYNC_WORD_X96       = 0x1d95f262,
    SYNC_WORD_XBR       = 0x655e315e,
    SYNC_WORD_LBR       = 0x0a801921,
    SYNC_WORD_XLL       = 0x41a29547,
    SYNC_WORD_EXSS      = 0x64582025,
    SYNC_WORD_EXSS_LE   = 0x58642520,
    SYNC_WORD_CORE_EXSS = 0x02b09261,
};

// Table 7-10: Loudspeaker bit mask for speaker activity
enum SpeakerPair {
    SPEAKER_PAIR_C      = 0x0001,
    SPEAKER_PAIR_LR     = 0x0002,
    SPEAKER_PAIR_LsRs   = 0x0004,
    SPEAKER_PAIR_LFE1   = 0x0008,
    SPEAKER_PAIR_Cs     = 0x0010,
    SPEAKER_PAIR_LhRh   = 0x0020,
    SPEAKER_PAIR_LsrRsr = 0x0040,
    SPEAKER_PAIR_Ch     = 0x0080,
    SPEAKER_PAIR_Oh     = 0x0100,
    SPEAKER_PAIR_LcRc   = 0x0200,
    SPEAKER_PAIR_LwRw   = 0x0400,
    SPEAKER_PAIR_LssRss = 0x0800,
    SPEAKER_PAIR_LFE2   = 0x1000,
    SPEAKER_PAIR_LhsRhs = 0x2000,
    SPEAKER_PAIR_Chr    = 0x4000,
    SPEAKER_PAIR_LhrRhr = 0x8000,
    SPEAKER_PAIR_ALL_1  = 0x5199,
    SPEAKER_PAIR_ALL_2  = 0xae66
};

// Table 7-11: Representation type
enum RepresentationType {
    REPR_TYPE_LtRt  = 2,
    REPR_TYPE_LhRh  = 3
};

// Table 7-15: Core/extension mask
enum ExtensionMask {
    CSS_CORE    = 0x001,
    CSS_XXCH    = 0x002,
    CSS_X96     = 0x004,
    CSS_XCH     = 0x008,
    EXSS_CORE   = 0x010,
    EXSS_XBR    = 0x020,
    EXSS_XXCH   = 0x040,
    EXSS_X96    = 0x080,
    EXSS_LBR    = 0x100,
    EXSS_XLL    = 0x200,
    EXSS_RSV1   = 0x400,
    EXSS_RSV2   = 0x800
};

// Table 8-8: Downmix type
enum DownMixType {
    DMIX_TYPE_1_0,
    DMIX_TYPE_LoRo,
    DMIX_TYPE_LtRt,
    DMIX_TYPE_3_0,
    DMIX_TYPE_2_1,
    DMIX_TYPE_2_2,
    DMIX_TYPE_3_1,

    DMIX_TYPE_COUNT
};

#endif

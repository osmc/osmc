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

// Table 5-4: Audio channel arrangement
static const uint8_t audio_mode_nch[16] = {
    1, 2, 2, 2, 2, 3, 3, 4, 4, 5, 6, 6, 6, 7, 8, 8
};

// Table 5-5: Core audio sampling frequencies
static const int32_t sample_rates[16] = {
    0,      8000,   16000,  32000,
    0,      0,      11025,  22050,
    44100,  0,      0,      12000,
    24000,  48000,  0,      0
};

// Table 5-7: Rate parameter versus targeted bit-rate
static const int32_t bit_rates[32] = {
     32000,   56000,  64000,  96000, 112000, 128000, 192000, 224000,
    256000,  320000, 384000, 448000, 512000, 576000, 640000, 768000,
    960000, 1024000,1152000,1280000,1344000,1408000,1411200,1472000,
    1536000,     -1,     -1,     -1,     -1,     0,      -1,     -2
};

// Table 5-17: Quantization resolution of source PCM samples
static const uint8_t sample_res[8] = {
    16, 16, 20, 20, 0, 24, 24, 0
};

// Table 5-26: Selection of quantization levels and codebooks
static const int32_t quant_levels[32] = {
          0,       3,       5,     7,      9,     13,     17,      25,
         32,      64,     128,   256,    512,   1024,   2048,    4096,
       8192,   16384,   32768, 65536, 131072, 262144, 524288, 1048576,
    2097152, 4194304, 8388608,    -1,     -1,     -1,     -1,      -1
};

// Table 5-26: Selection of quantization levels and codebooks
static const uint8_t quant_index_sel_nbits[10] = {
    1, 2, 2, 2, 2, 3, 3, 3, 3, 3
};

// Table 5-27: Scale factor adjustment index
static const int32_t scale_factor_adj[4] = {
    4194304, 4718592, 5242880, 6029312
};

// Annex D.1.1 - 6 bit quantization (nominal 2,2 dB step)
static const int32_t scale_factors_6bit[64] = {
          1,       2,       2,       3,       3,       4,       6,       7,
         10,      12,      16,      20,      26,      34,      44,      56,
         72,      93,     120,     155,     200,     257,     331,     427,
        550,     708,     912,    1175,    1514,    1950,    2512,    3236,
       4169,    5370,    6918,    8913,   11482,   14791,   19055,   24547,
      31623,   40738,   52481,   67608,   87096,  112202,  144544,  186209,
     239883,  309030,  398107,  512861,  660693,  851138, 1096478, 1412538,
    1819701, 2344229, 3019952, 3890451, 5011872, 6456542, 8317638,       0
};

// Annex D.1.2 - 7 bit quantization (nominal 1,1 dB step)
static const int32_t scale_factors_7bit[128] = {
          1,       1,       2,       2,       2,       2,       3,       3,
          3,       4,       4,       5,       6,       7,       7,       8,
         10,      11,      12,      14,      16,      18,      20,      23,
         26,      30,      34,      38,      44,      50,      56,      64,
         72,      82,      93,     106,     120,     136,     155,     176,
        200,     226,     257,     292,     331,     376,     427,     484,
        550,     624,     708,     804,     912,    1035,    1175,    1334,
       1514,    1718,    1950,    2213,    2512,    2851,    3236,    3673,
       4169,    4732,    5370,    6095,    6918,    7852,    8913,   10116,
      11482,   13032,   14791,   16788,   19055,   21627,   24547,   27861,
      31623,   35892,   40738,   46238,   52481,   59566,   67608,   76736,
      87096,   98855,  112202,  127350,  144544,  164059,  186209,  211349,
     239883,  272270,  309030,  350752,  398107,  451856,  512861,  582103,
     660693,  749894,  851138,  966051, 1096478, 1244515, 1412538, 1603245,
    1819701, 2065380, 2344229, 2660725, 3019952, 3427678, 3890451, 4415704,
    5011872, 5688529, 6456542, 7328245, 8317638,       0,       0,       0
};

// Annex D.2.1 - Lossy quantization
static const int32_t step_size_lossy[32] = {
         0, 6710886, 4194304, 3355443, 2474639, 2097152, 1761608, 1426063,
    796918,  461373,  251658,  146801,   79692,   46137,   27263,   16777,
     10486,    5872,    3355,    1887,    1258,     713,     336,     168,
        84,      42,      21,      -1,      -1,      -1,      -1,      -1
};

// Annex D.2.2 - Lossless quantization
static const int32_t step_size_lossless[32] = {
         0, 4194304, 2097152, 1384120, 1048576, 696254, 524288, 348127,
    262144,  131072,   65431,   33026,   16450,   8208,   4100,   2049,
      1024,     512,     256,     128,      64,     32,     16,      8,
         4,       2,       1,      -1,      -1,     -1,     -1,     -1
};

// Annex D.3 - Scale factor for joint intensity coding
static const int32_t joint_scale_factors[129] = {
       3288,    3490,    3691,    3909,    4144,    4387,    4647,    4924,
       5218,    5528,    5855,    6199,    6568,    6963,    7374,    7810,
       8271,    8758,    9278,    9831,   10410,   11031,   11685,   12373,
      13103,   13883,   14705,   15578,   16500,   17482,   18514,   19613,
      20770,   22003,   23312,   24688,   26156,   27699,   29343,   31080,
      32925,   34871,   36943,   39133,   41448,   43906,   46506,   49258,
      52177,   55273,   58544,   62017,   65691,   69584,   73711,   78073,
      82703,   87602,   92795,   98289,  104111,  110285,  116820,  123740,
     131072,  138840,  147069,  155776,  165012,  174785,  185145,  196117,
     207735,  220042,  233086,  246894,  261523,  277017,  293434,  310823,
     329236,  348748,  369409,  391303,  414490,  439043,  465064,  492621,
     521805,  552725,  585475,  620170,  656920,  695843,  737073,  780745,
     827008,  876014,  927923,  982902, 1041144, 1102834, 1168181, 1237404,
    1310720, 1388382, 1470649, 1557790, 1650098, 1747876, 1851441, 1961147,
    2077355, 2200441, 2330825, 2468935, 2615232, 2770195, 2934335, 3108206,
    3292378, 3487463, 3694108, 3913000, 4144862, 4390455, 4650611, 4926176,
    5218066
};

// Annex D.6 - Block code books
static const uint8_t block_code_nbits[8] = {
    0, 7, 10, 12, 13, 15, 17, 19
};

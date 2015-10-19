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
#include "dca_waveout.h"

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#else
#include <unistd.h>
#endif

struct dcadec_waveout {
    FILE        *fp[SPEAKER_COUNT];
    uint64_t    size;
    uint8_t     *buffer;
    char        *pattern;
    int         flags;

    int         channel_mask;
    int         nchannels;
    int         sample_rate;
    int         bits_per_sample;
    int         bytes_per_sample;
    int         block_align;
};

static const char * const dca_speaker_names[] = {
    "C",    "L",    "R",    "Ls",
    "Rs",   "LFE",  "Cs",   "Lsr",
    "Rsr",  "Lss",  "Rss",  "Lc",
    "Rc",   "Lh",   "Ch",   "Rh",
    "LFE2", "Lw",   "Rw",   "Oh",
    "Lhs",  "Rhs",  "Chr",  "Lhr",
    "Rhr",  "Cl",   "Ll",   "Rl",
    "RSV1", "RSV2", "RSV3", "RSV4"
};

struct header_buf {
    uint8_t data[68];
    size_t  size;
};

static void write_u16(struct header_buf *buf, int v)
{
    assert(buf->size <= sizeof(buf->data) - 2);
    buf->data[buf->size + 0] = (v >> 0) & 0xff;
    buf->data[buf->size + 1] = (v >> 8) & 0xff;
    buf->size += 2;
}

static void write_u32(struct header_buf *buf, int v)
{
    assert(buf->size <= sizeof(buf->data) - 4);
    buf->data[buf->size + 0] = (v >>  0) & 0xff;
    buf->data[buf->size + 1] = (v >>  8) & 0xff;
    buf->data[buf->size + 2] = (v >> 16) & 0xff;
    buf->data[buf->size + 3] = (v >> 24) & 0xff;
    buf->size += 4;
}

static int write_header(struct dcadec_waveout *wave, FILE *fp)
{
    bool extensible = !(wave->flags & DCADEC_WAVEOUT_FLAG_MONO);

    struct header_buf buf;
    buf.size = 0;

    write_u32(&buf, TAG_RIFF);
    if (wave->size && wave->size <= UINT32_MAX - (36 + 24 * extensible))
        write_u32(&buf, (uint32_t)(wave->size + 36 + 24 * extensible));
    else
        write_u32(&buf, 0);

    write_u32(&buf, TAG_WAVE);

    write_u32(&buf, TAG_fmt);
    write_u32(&buf, 16 + 24 * extensible);

    // wFormatTag
    write_u16(&buf, extensible ? 0xfffe : 0x0001);

    // nChannels
    write_u16(&buf, extensible ? wave->nchannels : 1);

    // nSamplesPerSec
    write_u32(&buf, wave->sample_rate);

    // nAvgBytesPerSec
    write_u32(&buf, wave->sample_rate * wave->block_align);

    // nBlockAlign
    write_u16(&buf, wave->block_align);

    // wBitsPerSample
    write_u16(&buf, wave->bytes_per_sample << 3);

    if (extensible) {
        // cbSize
        write_u16(&buf, 22);

        // wValidBitsPerSample
        write_u16(&buf, wave->bits_per_sample);

        // dwChannelMask
        write_u32(&buf, wave->channel_mask);

        // SubFormat
        write_u32(&buf, 1);
        write_u32(&buf, 0x00100000);
        write_u32(&buf, 0xaa000080);
        write_u32(&buf, 0x719b3800);
    }

    write_u32(&buf, TAG_data);
    if (wave->size <= UINT32_MAX)
        write_u32(&buf, (uint32_t)wave->size);
    else
        write_u32(&buf, 0);

    if (fwrite(buf.data, buf.size, 1, fp) != 1)
        return -DCADEC_EIO;

    return 0;
}

static int write_data(struct dcadec_waveout *wave, FILE *fp,
                      int **samples, int nsamples, int nchannels)
{
    int limit = 1 << (wave->bits_per_sample - 1);
    int mask = ~((1 << wave->bits_per_sample) - 1);
    int bps = wave->bytes_per_sample;
    int nclipped = 0;

    uint8_t *dst = wave->buffer;
    for (int i = 0; i < nsamples; i++) {
        for (int j = 0; j < nchannels; j++) {
            int sample = samples[j][i];

            if ((sample + limit) & mask) {
                sample = (sample >> 31) ^ (limit - 1);
                nclipped++;
            }

            switch (bps) {
            case 4:
                dst[0] = (sample >>  0) & 0xff;
                dst[1] = (sample >>  8) & 0xff;
                dst[2] = (sample >> 16) & 0xff;
                dst[3] = (sample >> 24) & 0xff;
                break;
            case 3:
                dst[0] = (sample >>  0) & 0xff;
                dst[1] = (sample >>  8) & 0xff;
                dst[2] = (sample >> 16) & 0xff;
                break;
            case 2:
                dst[0] = (sample >>  0) & 0xff;
                dst[1] = (sample >>  8) & 0xff;
                break;
            case 1:
                dst[0] = (sample >>  0) & 0xff;
                break;
            default:
                return -DCADEC_EINVAL;
            }

            dst += bps;
        }
    }

    if (nclipped && !(wave->flags & DCADEC_WAVEOUT_FLAG_CLIP))
        return -DCADEC_EOVERFLOW;

    if (fwrite(wave->buffer, wave->block_align, nsamples, fp) != (size_t)nsamples)
        return -DCADEC_EIO;

    return nclipped;
}

DCADEC_API int dcadec_waveout_write(struct dcadec_waveout *wave, int **samples,
                                    int nsamples, int channel_mask,
                                    int sample_rate, int bits_per_sample)
{
    int ret, nclipped;

    if (nsamples == 0)
        return 0;
    if (nsamples < 0)
        return -DCADEC_EINVAL;
    if (!wave)
        return -DCADEC_EINVAL;
    if (!samples)
        return -DCADEC_EINVAL;
    if (!channel_mask)
        return -DCADEC_EINVAL;
    if (sample_rate < 8000 || sample_rate > 384000)
        return -DCADEC_EINVAL;
    if (bits_per_sample < 8 || bits_per_sample > 32)
        return -DCADEC_EINVAL;

    if (!wave->size) {
        wave->channel_mask = channel_mask;
        wave->nchannels = dca_popcount(channel_mask);
        wave->sample_rate = sample_rate;
        wave->bits_per_sample = bits_per_sample;
        wave->bytes_per_sample = (bits_per_sample + 7) >> 3;

        if (wave->flags & DCADEC_WAVEOUT_FLAG_MONO) {
            wave->block_align = wave->bytes_per_sample;
            for (int i = 0, j = 0; i < SPEAKER_COUNT; i++) {
                if (!(wave->channel_mask & (1U << i)))
                    continue;

                if (!wave->fp[j]) {
                    char name[1024];
                    sprintf(name, wave->pattern, dca_speaker_names[i]);
                    if (!(wave->fp[j] = fopen(name, "wb")))
                        return -DCADEC_EIO;
                }

                if ((ret = write_header(wave, wave->fp[j])) < 0)
                    return ret;

                j++;
            }
        } else {
            wave->block_align = wave->nchannels * wave->bytes_per_sample;
            if ((ret = write_header(wave, wave->fp[0])) < 0)
                return ret;
        }
    } else {
        if (channel_mask != wave->channel_mask)
            return -DCADEC_EOUTCHG;
        if (sample_rate != wave->sample_rate)
            return -DCADEC_EOUTCHG;
        if (bits_per_sample != wave->bits_per_sample)
            return -DCADEC_EOUTCHG;
    }

    if (ta_alloc_fast(wave, &wave->buffer, nsamples, wave->block_align) < 0)
        return -DCADEC_ENOMEM;

    if (wave->flags & DCADEC_WAVEOUT_FLAG_MONO) {
        nclipped = 0;
        for (int i = 0; i < wave->nchannels; i++) {
            if ((ret = write_data(wave, wave->fp[i], &samples[i], nsamples, 1)) < 0)
                return ret;
            nclipped += ret;
        }
    } else {
        if ((ret = write_data(wave, wave->fp[0], samples, nsamples, wave->nchannels)) < 0)
            return ret;
        nclipped = ret;
    }

    wave->size += nsamples * wave->block_align;
    return nclipped;
}

DCADEC_API struct dcadec_waveout *dcadec_waveout_open(const char *name, int flags)
{
    if (flags & DCADEC_WAVEOUT_FLAG_MONO) {
        if (!name || strlen(name) >= 1020)
            return NULL;
        char *p = strchr(name, '%');
        if (!p || p[1] != 's' || strchr(p + 2, '%'))
            return NULL;
    }

    struct dcadec_waveout *wave = ta_znew(NULL, struct dcadec_waveout);
    if (!wave)
        return NULL;

    if (name) {
        if (flags & DCADEC_WAVEOUT_FLAG_MONO) {
            if (!(wave->pattern = ta_strdup(wave, name)))
                goto fail;
        } else {
            if (!(wave->fp[0] = fopen(name, "wb")))
                goto fail;
        }
    } else {
        int fd;
#ifdef _WIN32
        if ((fd = _dup(STDOUT_FILENO)) < 0)
            goto fail;
        if (_setmode(fd, _O_BINARY) < 0) {
            _close(fd);
            goto fail;
        }
        if (!(wave->fp[0] = _fdopen(fd, "wb"))) {
            _close(fd);
            goto fail;
        }
#else
        if ((fd = dup(STDOUT_FILENO)) < 0)
            goto fail;
        if (!(wave->fp[0] = fdopen(fd, "wb"))) {
            close(fd);
            goto fail;
        }
#endif
    }

    wave->flags = flags;
    return wave;

fail:
    ta_free(wave);
    return NULL;
}

DCADEC_API void dcadec_waveout_close(struct dcadec_waveout *wave)
{
    if (!wave)
        return;

    for (int i = 0; i < SPEAKER_COUNT; i++) {
        if (wave->fp[i]) {
            if (wave->size && !fseeko(wave->fp[i], 0, SEEK_SET))
                write_header(wave, wave->fp[i]);
            fclose(wave->fp[i]);
        }
    }

    ta_free(wave);
}

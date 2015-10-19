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
#include "dca_stream.h"

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#else
#include <unistd.h>
#endif

#define BUFFER_ALIGN    4096

#define AUPR_HDR    UINT64_C(0x415550522D484452)
#define DTSHDHDR    UINT64_C(0x4454534844484452)
#define STRMDATA    UINT64_C(0x5354524D44415441)

#if (defined _WIN32)
#define DCA_FGETC   _fgetc_nolock
#elif (defined _BSD_SOURCE)
#define DCA_FGETC   fgetc_unlocked
#else
#define DCA_FGETC   fgetc
#endif

struct dcadec_stream {
    FILE    *fp;

    off_t   stream_size;
    off_t   stream_start;
    off_t   stream_end;

    bool        aupr_present;
    uint32_t    aupr_sample_rate;
    uint32_t    aupr_nframes;
    uint32_t    aupr_nframesamples;
    uint64_t    aupr_npcmsamples;
    uint32_t    aupr_ch_mask;
    uint32_t    aupr_ndelaysamples;

    uint8_t     *buffer;
    size_t      packet_size;
    uint32_t    backup_sync;

    bool    core_plus_exss;
};

// Check for DTS-HD container format. Such files have an extra `blackout'
// frame at the end that we don't wont to parse. Called only if the stream
// is seekable.
static int parse_hd_hdr(struct dcadec_stream *stream)
{
    uint64_t header[2];

    if (fread(header, sizeof(header), 1, stream->fp) != 1)
        return fseeko(stream->fp, 0, SEEK_SET);

    if (header[0] != DCA_64BE_C(DTSHDHDR))
        return fseeko(stream->fp, 0, SEEK_SET);

    while (true) {
        uint64_t size = DCA_64BE(header[1]);
        if (size > INT64_MAX)
            return -1;

        switch (header[0]) {
        case DCA_64BE_C(STRMDATA): {
            off_t pos = ftello(stream->fp);
            if (pos < 0)
                return -1;
            stream->stream_size = size;
            stream->stream_start = pos;
            stream->stream_end = pos + size;
            return 1;
        }

        case DCA_64BE_C(AUPR_HDR): {
            uint8_t data[21];

            if (size < sizeof(data))
                return -1;
            if (fread(data, sizeof(data), 1, stream->fp) != 1)
                return -1;
            if (fseeko(stream->fp, size - sizeof(data), SEEK_CUR) < 0)
                return -1;

            stream->aupr_present = true;

            // Sample rate in Hz
            stream->aupr_sample_rate = DCA_MEM24BE(&data[3]);

            // Number of frames
            stream->aupr_nframes = DCA_MEM32BE(&data[6]);

            // Number of PCM samples per frame
            stream->aupr_nframesamples = DCA_MEM16BE(&data[10]);

            // Number of PCM samples encoded
            stream->aupr_npcmsamples = DCA_MEM40BE(&data[12]);

            // EXSS channel mask
            stream->aupr_ch_mask = DCA_MEM16BE(&data[17]);

            // Codec delay in samples
            stream->aupr_ndelaysamples = DCA_MEM16BE(&data[19]);
            break;
        }

        default:
            if (fseeko(stream->fp, size, SEEK_CUR) < 0)
                return -1;
            break;
        }

        if (fread(header, sizeof(header), 1, stream->fp) != 1)
            return -1;
    }

    return -1;
}

static int parse_wav_hdr(struct dcadec_stream *stream)
{
    uint32_t header[2];

    if (fread(header, sizeof(header), 1, stream->fp) != 1)
        goto rewind;

    if (header[0] != DCA_32LE_C(TAG_RIFF))
        goto rewind;

    if (fread(header, sizeof(header[0]), 1, stream->fp) != 1)
        goto rewind;

    if (header[0] != DCA_32LE_C(TAG_WAVE))
        goto rewind;

    while (true) {
        if (fread(header, sizeof(header), 1, stream->fp) != 1)
            return -1;

        uint32_t size = DCA_32LE(header[1]);

        if (header[0] == DCA_32LE_C(TAG_data)) {
            off_t pos = ftello(stream->fp);
            if (pos < 0)
                return -1;
            if (size) {
                stream->stream_size = size;
                stream->stream_start = pos;
                stream->stream_end = pos + size;
            }
            return 1;
        }

        if (fseeko(stream->fp, size, SEEK_CUR) < 0)
            return -1;
    }

    return -1;

rewind:
    return fseeko(stream->fp, 0, SEEK_SET);
}

DCADEC_API struct dcadec_stream *dcadec_stream_open(const char *name, int flags)
{
    (void)flags;

    struct dcadec_stream *stream = ta_znew(NULL, struct dcadec_stream);
    if (!stream)
        return NULL;

    if (name) {
        if (!(stream->fp = fopen(name, "rb")))
            goto fail1;
    } else {
        int fd;
#ifdef _WIN32
        if ((fd = _dup(STDIN_FILENO)) < 0)
            goto fail1;
        if (_setmode(fd, _O_BINARY) < 0) {
            _close(fd);
            goto fail1;
        }
        if (!(stream->fp = _fdopen(fd, "rb"))) {
            _close(fd);
            goto fail1;
        }
#else
        if ((fd = dup(STDIN_FILENO)) < 0)
            goto fail1;
        if (!(stream->fp = fdopen(fd, "rb"))) {
            close(fd);
            goto fail1;
        }
#endif
    }

    if (!fseeko(stream->fp, 0, SEEK_END)) {
        off_t pos = ftello(stream->fp);
        if (pos > 0)
            stream->stream_size = pos;
        if (fseeko(stream->fp, 0, SEEK_SET) < 0)
            goto fail2;
        if (pos > 0) {
            int ret;
            if ((ret = parse_hd_hdr(stream)) < 0 ||
                (!ret && parse_wav_hdr(stream) < 0))
                goto fail2;
        }
    }

    if (!(stream->buffer = ta_zalloc_size(stream, BUFFER_ALIGN * 2)))
        goto fail2;

    return stream;

fail2:
    fclose(stream->fp);
fail1:
    ta_free(stream);
    return NULL;
}

DCADEC_API void dcadec_stream_close(struct dcadec_stream *stream)
{
    if (stream) {
        fclose(stream->fp);
        ta_free(stream);
    }
}

static uint8_t *prepare_packet_buffer(struct dcadec_stream *stream, size_t size)
{
    size_t old_size = ta_get_size(stream->buffer);
    size_t new_size = DCA_ALIGN(stream->packet_size + size, BUFFER_ALIGN);

    if (old_size < new_size) {
        uint8_t *buf = ta_realloc_size(stream, stream->buffer, new_size);
        if (buf) {
            memset(buf + old_size, 0, new_size - old_size);
            stream->buffer = buf;
        } else {
            return NULL;
        }
    }

    return stream->buffer + stream->packet_size;
}

static int read_frame(struct dcadec_stream *stream, uint32_t *sync_p)
{
    uint8_t header[DCADEC_FRAME_HEADER_SIZE], *buf;
    size_t frame_size;
    int ret;

    // Stop at position indicated by STRMDATA if known
    if (stream->stream_end > 0 && ftello(stream->fp) >= stream->stream_end)
        return 0;

    // Start with a backed up sync word. If there is none, advance one byte at
    // a time until proper sync word is read from the input byte stream.
    uint32_t sync = stream->backup_sync;
    while (sync != SYNC_WORD_CORE
        && sync != SYNC_WORD_EXSS
        && sync != SYNC_WORD_CORE_LE
        && sync != SYNC_WORD_EXSS_LE
        && sync != SYNC_WORD_CORE_LE14
        && sync != SYNC_WORD_CORE_BE14) {
        int c = DCA_FGETC(stream->fp);
        if (c == EOF)
            return 0;
        sync = (sync << 8) | c;
    }

    // Tried to read the second (EXSS) frame and it was core again. Back up
    // the sync word just read and return.
    if ((sync != SYNC_WORD_EXSS && sync != SYNC_WORD_EXSS_LE) && !sync_p) {
        stream->backup_sync = sync;
        return -DCADEC_ENOSYNC;
    }

    // Clear backed up sync word
    stream->backup_sync = 0;

    // Restore sync word
    header[0] = (sync >> 24) & 0xff;
    header[1] = (sync >> 16) & 0xff;
    header[2] = (sync >>  8) & 0xff;
    header[3] = (sync >>  0) & 0xff;

    // Read the frame header
    if (fread(header + 4, sizeof(header) - 4, 1, stream->fp) != 1)
        return 0;

    // Parse and validate the frame header
    if ((ret = dcadec_frame_parse_header(header, &frame_size)) < 0)
        return ret;

    // Reallocate packet buffer
    if (!(buf = prepare_packet_buffer(stream, dcadec_frame_buffer_size(frame_size))))
        return -DCADEC_ENOMEM;

    // Restore frame header
    memcpy(buf, header, sizeof(header));

    // Read the rest of the frame
    if (fread(buf + sizeof(header), frame_size - sizeof(header), 1, stream->fp) != 1)
        return 0;

    // Work around overread that occurs for 14-bit streams with excessive frame size
    if (sync == SYNC_WORD_CORE_LE14 || sync == SYNC_WORD_CORE_BE14)
        stream->backup_sync = DCA_MEM32BE(&buf[frame_size - 4]);

    // Convert the frame in place
    if ((ret = dcadec_frame_convert_bitstream(buf, &frame_size, buf, frame_size)) < 0)
        return ret;

    // Align frame size to 4-byte boundary
    stream->packet_size += DCA_ALIGN(frame_size, 4);

    if (sync_p)
        *sync_p = sync;
    return 1;
}

DCADEC_API int dcadec_stream_read(struct dcadec_stream *stream, uint8_t **data, size_t *size)
{
    uint32_t sync;
    int ret;

    // Loop until valid DTS core or standalone EXSS frame is read or EOF is
    // reached
    while (true) {
        ret = read_frame(stream, &sync);
        if (ret == 1)
            break;
        if (ret == 0)
            return ferror(stream->fp) ? -DCADEC_EIO : 0;
        if (ret < 0 && ret != -DCADEC_ENOSYNC)
            return ret;
    }

    // Check for EXSS that may follow core frame and try to concatenate both
    // frames into single packet
    if (sync == SYNC_WORD_CORE || sync == SYNC_WORD_CORE_LE) {
        ret = read_frame(stream, NULL);
        if (ret < 0 && ret != -DCADEC_ENOSYNC)
            return ret;
        // If the previous frame was core + EXSS, skip the incomplete (core
        // only) frame at end of file
        if (ret == 0 && stream->core_plus_exss)
            return 0;
        stream->core_plus_exss = (ret == 1);
    } else {
        stream->core_plus_exss = false;
    }

    *data = stream->buffer;
    *size = stream->packet_size;

    stream->packet_size = 0;
    return 1;
}

DCADEC_API int dcadec_stream_progress(struct dcadec_stream *stream)
{
    if (stream->stream_size > 0) {
        off_t pos = ftello(stream->fp);
        if (pos < stream->stream_start)
            return 0;
        if (pos >= stream->stream_start + stream->stream_size)
            return 100;
        return (int)((pos - stream->stream_start) * 100 / stream->stream_size);
    }
    return -1;
}

DCADEC_API struct dcadec_stream_info *dcadec_stream_get_info(struct dcadec_stream *stream)
{
    if (!stream || !stream->aupr_present)
        return NULL;
    struct dcadec_stream_info *info = ta_znew(NULL, struct dcadec_stream_info);
    if (!info)
        return NULL;

    info->stream_size = stream->stream_size;
    info->sample_rate = stream->aupr_sample_rate;
    info->nframes = stream->aupr_nframes;
    info->nframesamples = stream->aupr_nframesamples;
    info->npcmsamples = stream->aupr_npcmsamples;
    info->ch_mask = stream->aupr_ch_mask;
    info->ndelaysamples = stream->aupr_ndelaysamples;
    return info;
}

DCADEC_API void dcadec_stream_free_info(struct dcadec_stream_info *info)
{
    ta_free(info);
}

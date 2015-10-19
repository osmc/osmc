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

#ifndef DCA_STREAM_H
#define DCA_STREAM_H

#include "dca_context.h"

struct dcadec_stream;

struct dcadec_stream_info {
    uint64_t    stream_size;    /**< Size of encoded DTS stream data, in bytes */
    uint32_t    sample_rate;    /**< Audio sample rate in Hz */
    uint32_t    nframes;        /**< Total number of encoded frames */
    uint32_t    nframesamples;  /**< Number of PCM samples in each encoded frame */
    uint64_t    npcmsamples;    /**< Total number of PCM samples in original audio */
    uint32_t    ch_mask;        /**< Channel mask in EXSS format */
    uint32_t    ndelaysamples;  /**< Codec delay in PCM samples */
};

/**
 * Open DTS stream from file or standard input.
 *
 * @param name  Name of the file to be opened. Pass NULL to open standard input.
 *
 * @param flags Currently unused, should be 0.
 *
 * @return      Stream handle on success, NULL on failure.
 */
DCADEC_API struct dcadec_stream *dcadec_stream_open(const char *name, int flags);

/**
 * Close DTS stream.
 *
 * @param stream    Stream handle.
 */
DCADEC_API void dcadec_stream_close(struct dcadec_stream *stream);

/**
 * Establish synchronization and read the next packet from DTS stream.
 *
 * @param stream    Stream handle.
 *
 * @param data  Filled with pointer to packet data. This data is only
 *              valid until the next call to dcadec_stream_read() or
 *              dcadec_stream_close() functions. Packet data is padded
 *              with DCADEC_BUFFER_PADDING bytes at the end and can be
 *              directly passed to dcadec_context_parse() function.
 *
 * @param size  Filled with size of packet data, in bytes.
 *
 * @return      Positive value on success, 0 on EOF, negative error code on
 *              failure.
 */
DCADEC_API int dcadec_stream_read(struct dcadec_stream *stream, uint8_t **data, size_t *size);

/**
 * Return DTS stream progress percentage based on current file position.
 *
 * @param stream    Stream handle.
 *
 * @return      Progress value in range 0-100 on success, -1 on failure.
 */
DCADEC_API int dcadec_stream_progress(struct dcadec_stream *stream);

/**
 * Get audio properties information from DTS-HD container. For raw DTS streams
 * this function always fails.
 *
 * @param stream    Stream handle.
 *
 * @return      Pointer to information structure on success,
 *              NULL on failure. Returned data should be freed with
 *              dcadec_stream_free_info() function.
 */
DCADEC_API struct dcadec_stream_info *dcadec_stream_get_info(struct dcadec_stream *stream);

/**
 * Free audio properties information structure.
 *
 * @param info  Pointer to information structure.
 */
DCADEC_API void dcadec_stream_free_info(struct dcadec_stream_info *info);

#endif

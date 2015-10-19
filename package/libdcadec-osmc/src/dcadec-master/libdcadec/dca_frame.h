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

#ifndef DCA_FRAME_H
#define DCA_FRAME_H

#include "dca_context.h"

/**
 * Minimum size alignment, in bytes, of source and destination buffers that must
 * be passed to dcadec_frame_convert_bitstream().
 */
#define DCADEC_FRAME_BUFFER_ALIGN   16

/**
 * Minimum size of data buffer, in bytes, that must be passed to
 * dcadec_frame_parse_header().
 */
#define DCADEC_FRAME_HEADER_SIZE    16

/**@{*/
#define DCADEC_BITSTREAM_BE16   0
#define DCADEC_BITSTREAM_LE16   1
#define DCADEC_BITSTREAM_BE14   2
#define DCADEC_BITSTREAM_LE14   3
/**@}*/

/**@{*/
#define DCADEC_FRAME_TYPE_CORE  0   /**< Backward compatible DTS core */
#define DCADEC_FRAME_TYPE_EXSS  1   /**< Extension sub-stream (EXSS) */
/**@}*/

/**
 * Convert the raw input frame into native 16-bit big-endian format understood
 * by dcadec_context_parse(). Can operate in-place when destination buffer
 * is the same as source buffer. In all other cases destination and source
 * buffers must not overlap.
 *
 * @param dst       Pointer to destination buffer. Destination buffer size
 *                  must be no less than source buffer size, including
 *                  padding to the next multiple of DCADEC_FRAME_BUFFER_ALIGN.
 *                  Destination buffer must be aligned on 4-byte boundary in
 *                  memory.
 *
 * @param dst_size  Filled with resulting frame size after conversion, in bytes.
 *
 * @param src       Pointer to source buffer that must start with a valid sync
 *                  word. Source buffer size must be no less than src_size bytes
 *                  plus padding to the next multiple of
 *                  DCADEC_FRAME_BUFFER_ALIGN. Source buffer can have any
 *                  alignment in memory.
 *
 * @param src_size  Size of raw frame data prior to conversion, in bytes. Size
 *                  should not include padding.
 *
 * @return          Detected bitstream format on success, negative error code
 *                  on failure.
 */
DCADEC_API int dcadec_frame_convert_bitstream(uint8_t *dst, size_t *dst_size,
                                              const uint8_t *src, size_t src_size);

/**
 * Check that the passed data buffer starts with a valid sync word and satisfies
 * the basic requirements to be a valid DTS core or EXSS frame header.
 *
 * @param data      Pointer to data buffer that possibly contains the frame
 *                  header. The first DCADEC_FRAME_HEADER_SIZE bytes are read
 *                  from the buffer.
 *
 * @param size      Filled with raw frame size, in bytes. Frame size includes
 *                  sync word and header bytes.
 *
 * @return          Detected frame type on success, negative error code
 *                  on failure.
 */
DCADEC_API int dcadec_frame_parse_header(const uint8_t *data, size_t *size);

/**
 * Given the raw frame size returned by dcadec_frame_parse_header(), calculate
 * minimum required buffer size for performing bitstream format conversion and
 * (possibly) parsing the frame.
 *
 * It is recommended to use this function instead of calculating buffer size
 * manually based on DCADEC_FRAME_BUFFER_ALIGN and DCADEC_BUFFER_PADDING
 * constants since padding requirements may change in the future and older
 * headers might not reflect this.
 *
 * @param size      Raw frame size, in bytes.
 *
 * @return          Buffer size, in bytes, padded to the minimum value that
 *                  satisfies both dcadec_frame_convert_bitstream() alignment
 *                  requirement and dcadec_context_parse() padding requirement.
 */
DCADEC_API size_t dcadec_frame_buffer_size(size_t size);

#endif

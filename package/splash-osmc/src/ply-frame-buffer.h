/* ply-frame-buffer.h - framebuffer abstraction
 *
 * Copyright (C) 2007 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * Written By: Ray Strode <rstrode@redhat.com>
 */
#ifndef PLY_FRAME_BUFFER_H
#define PLY_FRAME_BUFFER_H

#include <stdbool.h>
#include <stdint.h>

#include "ply-utils.h"

typedef struct _ply_frame_buffer ply_frame_buffer_t;
typedef struct _ply_frame_buffer_area ply_frame_buffer_area_t;

struct _ply_frame_buffer_area
{
  long x;
  long y;
  unsigned long width;
  unsigned long height;
};

struct _ply_frame_buffer
{
  char *device_name;
  int   device_fd;

  char *map_address;
  size_t size;

  uint32_t *shadow_buffer;

  uint32_t red_bit_position;
  uint32_t green_bit_position;
  uint32_t blue_bit_position;
  uint32_t alpha_bit_position;

  uint32_t bits_for_red;
  uint32_t bits_for_green;
  uint32_t bits_for_blue;
  uint32_t bits_for_alpha;

  int32_t dither_red;
  int32_t dither_green;
  int32_t dither_blue;

  unsigned int bytes_per_pixel;
  unsigned int row_stride;

  ply_frame_buffer_area_t area;
  ply_frame_buffer_area_t area_to_flush;

  void (*flush)(ply_frame_buffer_t *buffer);

  int pause_count;
};

#define PLY_FRAME_BUFFER_COLOR_TO_PIXEL_VALUE(r,g,b,a)                        \
    (((uint8_t) (CLAMP (a * 255.0, 0.0, 255.0)) << 24)                        \
      | ((uint8_t) (CLAMP (r * 255.0, 0.0, 255.0)) << 16)                     \
      | ((uint8_t) (CLAMP (g * 255.0, 0.0, 255.0)) << 8)                      \
      | ((uint8_t) (CLAMP (b * 255.0, 0.0, 255.0))))

#ifndef PLY_HIDE_FUNCTION_DECLARATIONS
ply_frame_buffer_t *ply_frame_buffer_new (const char *device_name);
void ply_frame_buffer_free (ply_frame_buffer_t *buffer);
bool ply_frame_buffer_open (ply_frame_buffer_t *buffer);
void ply_frame_buffer_pause_updates (ply_frame_buffer_t *buffer);
bool ply_frame_buffer_unpause_updates (ply_frame_buffer_t *buffer);
bool ply_frame_buffer_device_is_open (ply_frame_buffer_t *buffer); 
char *ply_frame_buffer_get_device_name (ply_frame_buffer_t *buffer);
void ply_frame_buffer_set_device_name (ply_frame_buffer_t *buffer,
                                       const char     *device_name);
void ply_frame_buffer_close (ply_frame_buffer_t *buffer);
void ply_frame_buffer_get_size (ply_frame_buffer_t     *buffer,
                                ply_frame_buffer_area_t *size);
bool ply_frame_buffer_fill_with_color (ply_frame_buffer_t      *buffer,
                                       ply_frame_buffer_area_t  *area,
                                       double               red, 
                                       double               green,
                                       double               blue, 
                                       double               alpha);
bool ply_frame_buffer_fill_with_hex_color (ply_frame_buffer_t      *buffer,
                                           ply_frame_buffer_area_t *area,
                                           uint32_t                 hex_color);

bool ply_frame_buffer_fill_with_hex_color_at_opacity (ply_frame_buffer_t      *buffer,
                                                      ply_frame_buffer_area_t *area,
                                                      uint32_t                 hex_color,
                                                      double                   opacity);

bool ply_frame_buffer_fill_with_gradient (ply_frame_buffer_t      *buffer,
					  ply_frame_buffer_area_t *area,
					  uint32_t                 start,
					  uint32_t                 end);

bool ply_frame_buffer_fill_with_argb32_data (ply_frame_buffer_t      *buffer,
                                             ply_frame_buffer_area_t  *area,
                                             unsigned long        x,
                                             unsigned long        y,
                                             uint32_t            *data);
bool ply_frame_buffer_fill_with_argb32_data_at_opacity (ply_frame_buffer_t      *buffer,
                                                        ply_frame_buffer_area_t *area,
                                                        unsigned long            x,
                                                        unsigned long            y,
                                                        uint32_t                *data,
                                                        double                   opacity);

bool ply_frame_buffer_fill_with_argb32_data_with_clip (ply_frame_buffer_t      *buffer,
                                                       ply_frame_buffer_area_t  *area,
                                                       ply_frame_buffer_area_t  *clip,
                                                       unsigned long        x,
                                                       unsigned long        y,
                                                       uint32_t            *data);
bool ply_frame_buffer_fill_with_argb32_data_at_opacity_with_clip (ply_frame_buffer_t      *buffer,
                                                                  ply_frame_buffer_area_t *area,
                                                                  ply_frame_buffer_area_t *clip,
                                                                  unsigned long            x,
                                                                  unsigned long            y,
                                                                  uint32_t                *data,
                                                                  double                   opacity);

const char *ply_frame_buffer_get_bytes (ply_frame_buffer_t *buffer);


#endif

#endif /* PLY_FRAME_BUFFER_H */
/* vim: set ts=4 sw=4 expandtab autoindent cindent cino={.5s,(0: */

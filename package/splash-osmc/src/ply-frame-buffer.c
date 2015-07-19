/* ply-frame-buffer.c - framebuffer abstraction
 *
 * Copyright (C) 2006, 2007, 2008 Red Hat, Inc.
 *               2008 Charlie Brej <cbrej@cs.man.ac.uk>
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
 * Written by: Charlie Brej <cbrej@cs.man.ac.uk>
 *             Kristian Høgsberg <krh@redhat.com>
 *             Ray Strode <rstrode@redhat.com>
 */
#include "ply-frame-buffer.h"
//#include "ply-logger.h"

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <values.h>
#include <unistd.h>

#include <linux/fb.h>

#ifndef PLY_FRAME_BUFFER_DEFAULT_FB_DEVICE_NAME
#define PLY_FRAME_BUFFER_DEFAULT_FB_DEVICE_NAME "/dev/fb0"
#endif

static bool ply_frame_buffer_open_device (ply_frame_buffer_t  *buffer);
static void ply_frame_buffer_close_device (ply_frame_buffer_t *buffer);
static bool ply_frame_buffer_query_device (ply_frame_buffer_t *buffer);
static bool ply_frame_buffer_map_to_device (ply_frame_buffer_t *buffer);


static void ply_frame_buffer_add_area_to_flush_area (ply_frame_buffer_t     *buffer,
                                                     ply_frame_buffer_area_t *area);

static bool ply_frame_buffer_flush (ply_frame_buffer_t *buffer);

static void ply_frame_buffer_area_intersect (ply_frame_buffer_area_t *area1,
                                             ply_frame_buffer_area_t *area2,
                                             ply_frame_buffer_area_t *result);

static bool
ply_frame_buffer_open_device (ply_frame_buffer_t  *buffer)
{
  assert (buffer != NULL);
  assert (buffer->device_name != NULL);
  buffer->device_fd = open (buffer->device_name, O_RDWR);

  if (buffer->device_fd < 0)
    {
      return false;
    }

  return true;
}

static void
ply_frame_buffer_close_device (ply_frame_buffer_t *buffer)
{
  assert (buffer != NULL);

  if (buffer->map_address != MAP_FAILED)
    {
      munmap (buffer->map_address, buffer->size);
      buffer->map_address = MAP_FAILED;
    }

  if (buffer->device_fd >= 0)
    {
      close (buffer->device_fd);
      buffer->device_fd = -1;
    }
}

static void
flush_generic (ply_frame_buffer_t *buffer)
{
  unsigned long row, column;
  char *row_buffer;
  size_t bytes_per_row;
  unsigned long x1, y1, x2, y2;

  x1 = buffer->area_to_flush.x;
  y1 = buffer->area_to_flush.y;
  x2 = x1 + buffer->area_to_flush.width;
  y2 = y1 + buffer->area_to_flush.height;

  bytes_per_row = buffer->area_to_flush.width * buffer->bytes_per_pixel;
  row_buffer = malloc (buffer->row_stride * buffer->bytes_per_pixel);
  for (row = y1; row < y2; row++)
    {
      unsigned long offset;

      offset = row * buffer->row_stride * buffer->bytes_per_pixel + x1 * buffer->bytes_per_pixel;
      memcpy (buffer->map_address + offset, &buffer->shadow_buffer[row*buffer->area.width + x1],
              buffer->area_to_flush.width * buffer->bytes_per_pixel);
    }
  free (row_buffer);
}

static void
flush_xrgb32 (ply_frame_buffer_t *buffer)
{
  unsigned long x1, y1, x2, y2, y;
  char *dst, *src;

  x1 = buffer->area_to_flush.x;
  y1 = buffer->area_to_flush.y;
  x2 = x1 + buffer->area_to_flush.width;
  y2 = y1 + buffer->area_to_flush.height;

  dst = &buffer->map_address[(y1 * buffer->row_stride + x1) * 4];
  src = (char *) &buffer->shadow_buffer[y1 * buffer->area.width + x1];

  if (buffer->area_to_flush.width == buffer->row_stride)
    {
      memcpy (dst, src, buffer->area_to_flush.width * buffer->area_to_flush.height * 4);
      return;
    }

  for (y = y1; y < y2; y++)
    {
      memcpy (dst, src, buffer->area_to_flush.width * 4);
      dst += buffer->row_stride * 4;
      src += buffer->area.width * 4;
    }
}

static void
flush_xbgr32 (ply_frame_buffer_t *buffer)
{
  unsigned long x1, y1, x2, y2, x, y;
  char *dst, *src;

  x1 = buffer->area_to_flush.x;
  y1 = buffer->area_to_flush.y;
  x2 = x1 + buffer->area_to_flush.width;
  y2 = y1 + buffer->area_to_flush.height;

  for (y = y1; y < y2; y++)
    {
     dst = &buffer->map_address[(y * buffer->row_stride + x1) * 4];
     src = (char *) &buffer->shadow_buffer[y * buffer->area.width + x1];

     for (x = x1; x < x2; x++)
       {
         dst[0] = src[2];
         dst[1] = src[1];
         dst[2] = src[0];
         dst[3] = src[3];
         dst += 4;
         src += 4;
       }
    }
}

static void
flush_rgb16 (ply_frame_buffer_t *buffer)
{
  unsigned long x1, y1, x2, y2, x, y;
  unsigned short *dst; unsigned char *src;

  x1 = buffer->area_to_flush.x;
  y1 = buffer->area_to_flush.y;
  x2 = x1 + buffer->area_to_flush.width;
  y2 = y1 + buffer->area_to_flush.height;

  for (y = y1; y < y2; y++)
    {
     dst = (unsigned short *)&buffer->map_address[(y * buffer->row_stride + x1) * 2];
     src = (unsigned char *) &buffer->shadow_buffer[y * buffer->area.width + x1];

     for (x = x1; x < x2; x++)
       {
         *dst++ = (src[0]>>3) << 0 | (src[1]>>2) << 5 | (src[2]>>3) << 11;
         src += 4;
       }
    }
}

static const char const *p_visual(int visual)
{
  static const char const *visuals[] =
    {
      [FB_VISUAL_MONO01] = "FB_VISUAL_MONO01",
      [FB_VISUAL_MONO10] = "FB_VISUAL_MONO10",
      [FB_VISUAL_TRUECOLOR] = "FB_VISUAL_TRUECOLOR",
      [FB_VISUAL_PSEUDOCOLOR] = "FB_VISUAL_PSEUDOCOLOR",
      [FB_VISUAL_DIRECTCOLOR] = "FB_VISUAL_DIRECTCOLOR",
      [FB_VISUAL_STATIC_PSEUDOCOLOR] = "FB_VISUAL_STATIC_PSEUDOCOLOR",
      NULL
    };
  static char unknown[] = "invalid visual: -4294967295";

  if (visual < FB_VISUAL_MONO01 || visual > FB_VISUAL_STATIC_PSEUDOCOLOR)
    {
      sprintf(unknown, "invalid visual: %d", visual);
      return unknown;
    }

  return visuals[visual];
}

static bool
ply_frame_buffer_query_device (ply_frame_buffer_t *buffer)
{
  struct fb_var_screeninfo variable_screen_info;
  struct fb_fix_screeninfo fixed_screen_info;

  assert (buffer != NULL);
  assert (buffer->device_fd >= 0);

  if (ioctl (buffer->device_fd, FBIOGET_VSCREENINFO, &variable_screen_info) < 0)
    return false;

  if (ioctl(buffer->device_fd, FBIOGET_FSCREENINFO, &fixed_screen_info) < 0)
    return false;

  /* Normally the pixel is divided into channels between the color components.
   * Each channel directly maps to a color channel on the hardware.
   *
   * There are some odd ball modes that use an indexed palette instead.  In
   * those cases (pseudocolor, direct color, etc), the pixel value is just an
   * index into a lookup table of the real color values.
   *
   * We don't support that.
   */
  if (fixed_screen_info.visual != FB_VISUAL_TRUECOLOR)
    {
      int rc = -1;
      int i;
      int depths[] = {32, 24, 16, 0};

//      ply_trace("Visual was %s, trying to find usable mode.\n",
//                p_visual(fixed_screen_info.visual));

      for (i = 0; depths[i] != 0; i++)
        {
          variable_screen_info.bits_per_pixel = depths[i];
          variable_screen_info.activate |= FB_ACTIVATE_NOW | FB_ACTIVATE_FORCE;

          rc = ioctl(buffer->device_fd, FBIOPUT_VSCREENINFO, &variable_screen_info);
          if (rc >= 0)
            {
              if (ioctl(buffer->device_fd, FBIOGET_FSCREENINFO, &fixed_screen_info) < 0)
                return false;
              if (fixed_screen_info.visual == FB_VISUAL_TRUECOLOR)
                break;
            }
        }

      if (ioctl(buffer->device_fd, FBIOGET_VSCREENINFO, &variable_screen_info) < 0)
        return false;

      if (ioctl(buffer->device_fd, FBIOGET_FSCREENINFO, &fixed_screen_info) < 0)
        return false;
    }

  if (fixed_screen_info.visual != FB_VISUAL_TRUECOLOR ||
      variable_screen_info.bits_per_pixel < 16)
    {
//      ply_trace("Visual is %s; not using graphics\n",
//                p_visual(fixed_screen_info.visual));
      return false;
    }

  buffer->area.x = variable_screen_info.xoffset;
  buffer->area.y = variable_screen_info.yoffset;
  buffer->area.width = variable_screen_info.xres;
  buffer->area.height = variable_screen_info.yres;

  buffer->red_bit_position = variable_screen_info.red.offset;
  buffer->bits_for_red = variable_screen_info.red.length;

  buffer->green_bit_position = variable_screen_info.green.offset;
  buffer->bits_for_green = variable_screen_info.green.length;

  buffer->blue_bit_position = variable_screen_info.blue.offset;
  buffer->bits_for_blue = variable_screen_info.blue.length;

  buffer->alpha_bit_position = variable_screen_info.transp.offset;
  buffer->bits_for_alpha = variable_screen_info.transp.length;

  buffer->bytes_per_pixel = variable_screen_info.bits_per_pixel >> 3;
  buffer->row_stride = fixed_screen_info.line_length / buffer->bytes_per_pixel;
  buffer->size = buffer->area.height * buffer->row_stride * buffer->bytes_per_pixel;
  
  buffer->dither_red = 0;
  buffer->dither_green = 0;
  buffer->dither_blue = 0;

  if (buffer->bytes_per_pixel == 4 &&
      buffer->red_bit_position == 16 && buffer->bits_for_red == 8 &&
      buffer->green_bit_position == 8 && buffer->bits_for_green == 8 &&
      buffer->blue_bit_position == 0 && buffer->bits_for_blue == 8)
    buffer->flush = flush_xrgb32;
  else if (buffer->bytes_per_pixel == 4 &&
      buffer->red_bit_position == 0 && buffer->bits_for_red == 8 &&
      buffer->green_bit_position == 8 && buffer->bits_for_green == 8 &&
      buffer->blue_bit_position == 16 && buffer->bits_for_blue == 8)
    buffer->flush = flush_xbgr32;
  else if (buffer->bytes_per_pixel == 2 &&
      buffer->red_bit_position == 11 && buffer->bits_for_red == 5 &&
      buffer->green_bit_position == 5 && buffer->bits_for_green == 6 &&
      buffer->blue_bit_position == 0 && buffer->bits_for_blue == 5)
    buffer->flush = flush_rgb16;
  else
    buffer->flush = flush_generic;

  return true;
}

static bool
ply_frame_buffer_map_to_device (ply_frame_buffer_t *buffer)
{
  assert (buffer != NULL);
  assert (buffer->device_fd >= 0);
  assert (buffer->size > 0);

  buffer->map_address = mmap (NULL, buffer->size, PROT_WRITE,
                              MAP_SHARED, buffer->device_fd, 0);

  return buffer->map_address != MAP_FAILED;
}


static inline void 
ply_frame_buffer_set_value_at_pixel (ply_frame_buffer_t *buffer,
                                       int             x,
                                       int             y,
                                       uint32_t        pixel_value)
{

  buffer->shadow_buffer[y * buffer->area.width + x] = pixel_value;
}

static void
ply_frame_buffer_area_union (ply_frame_buffer_area_t *area1,
                             ply_frame_buffer_area_t *area2,
                             ply_frame_buffer_area_t *result)
{
  unsigned long x1, y1, x2, y2;

  if (area1->width == 0)
    {
      *result = *area2;
      return;
    }

  if (area2->width == 0)
    {
      *result = *area1;
      return;
    }

  x1 = area1->x + area1->width;
  y1 = area1->y + area1->height;
  x2 = area2->x + area2->width;
  y2 = area2->y + area2->height;

  result->x = MIN(area1->x, area2->x);
  result->y = MIN(area1->y, area2->y);
  result->width = MAX(x1, x2) - result->x;
  result->height = MAX(y1, y2) - result->y;
}

static void
ply_frame_buffer_add_area_to_flush_area (ply_frame_buffer_t      *buffer,
                                         ply_frame_buffer_area_t *area)
{
  ply_frame_buffer_area_t cropped_area;
  assert (buffer != NULL);
  assert (area != NULL);

  ply_frame_buffer_area_intersect (area, &buffer->area, &cropped_area);

  if (cropped_area.width == 0 || cropped_area.height == 0)
    return;

  ply_frame_buffer_area_union (&buffer->area_to_flush,
                               &cropped_area,
                               &buffer->area_to_flush);
}

static bool
ply_frame_buffer_flush (ply_frame_buffer_t *buffer)
{
  assert (buffer != NULL);

  if (buffer->pause_count > 0)
    return true;

  (*buffer->flush) (buffer);

  buffer->area_to_flush.x = buffer->area.width - 1;
  buffer->area_to_flush.y = buffer->area.height - 1;
  buffer->area_to_flush.width = 0; 
  buffer->area_to_flush.height = 0; 

  return true;
}

ply_frame_buffer_t *
ply_frame_buffer_new (const char *device_name)
{
  ply_frame_buffer_t *buffer;

  buffer = calloc (1, sizeof (ply_frame_buffer_t));

  if (device_name != NULL)
    buffer->device_name = strdup (device_name);
  else if (getenv ("FRAMEBUFFER") != NULL)
    buffer->device_name = strdup (getenv ("FRAMEBUFFER"));
  else
    buffer->device_name = 
      strdup (PLY_FRAME_BUFFER_DEFAULT_FB_DEVICE_NAME);

  buffer->map_address = MAP_FAILED;
  buffer->shadow_buffer = NULL;

  buffer->pause_count = 0;

  return buffer;
}

void
ply_frame_buffer_free (ply_frame_buffer_t *buffer)
{
  assert (buffer != NULL);

  if (ply_frame_buffer_device_is_open (buffer))
    ply_frame_buffer_close (buffer);

  free (buffer->device_name);
  free (buffer->shadow_buffer);
  free (buffer);
}

bool 
ply_frame_buffer_open (ply_frame_buffer_t *buffer)
{
  bool is_open;

  assert (buffer != NULL);

  is_open = false;

  if (!ply_frame_buffer_open_device (buffer))
    {
      goto out;
    }

  if (!ply_frame_buffer_query_device (buffer))
    {
      goto out;
    }

  if (!ply_frame_buffer_map_to_device (buffer))
    {
      goto out;
    }

  buffer->shadow_buffer =
    realloc (buffer->shadow_buffer, 4 * buffer->area.width * buffer->area.height);
  memset (buffer->shadow_buffer, 0, 4 * buffer->area.width * buffer->area.height);

  is_open = true;

out:

  if (!is_open)
    {
      int saved_errno;

      saved_errno = errno;
      ply_frame_buffer_close_device (buffer);
      errno = saved_errno;
    }

  return is_open;
}

void
ply_frame_buffer_pause_updates (ply_frame_buffer_t *buffer)
{
  assert (buffer != NULL);

  buffer->pause_count++;
}

bool
ply_frame_buffer_unpause_updates (ply_frame_buffer_t *buffer)
{
  assert (buffer != NULL);

  buffer->pause_count--;
  return ply_frame_buffer_flush (buffer);
}

bool 
ply_frame_buffer_device_is_open (ply_frame_buffer_t *buffer)
{
  assert (buffer != NULL);
  return buffer->device_fd >= 0 && buffer->map_address != MAP_FAILED;
}

char *
ply_frame_buffer_get_device_name (ply_frame_buffer_t *buffer)
{
  assert (buffer != NULL);
  assert (ply_frame_buffer_device_is_open (buffer));
  assert (buffer->device_name != NULL);

  return strdup (buffer->device_name);
}

void
ply_frame_buffer_set_device_name (ply_frame_buffer_t *buffer,
                                  const char     *device_name)
{
  assert (buffer != NULL);
  assert (!ply_frame_buffer_device_is_open (buffer));
  assert (device_name != NULL);
  assert (buffer->device_name != NULL);

  if (strcmp (buffer->device_name, device_name) != 0)
    {
      free (buffer->device_name);
      buffer->device_name = strdup (device_name);
    }
}

void 
ply_frame_buffer_close (ply_frame_buffer_t *buffer)
{
  assert (buffer != NULL);

  assert (ply_frame_buffer_device_is_open (buffer));
  ply_frame_buffer_close_device (buffer);

  buffer->bytes_per_pixel = 0;
  buffer->area.x = 0;
  buffer->area.y = 0;
  buffer->area.width = 0;
  buffer->area.height = 0;
}

void 
ply_frame_buffer_get_size (ply_frame_buffer_t     *buffer,
                           ply_frame_buffer_area_t *size)
{
  assert (buffer != NULL);
  assert (ply_frame_buffer_device_is_open (buffer));
  assert (size != NULL);

  *size = buffer->area;
}

static void
ply_frame_buffer_area_intersect (ply_frame_buffer_area_t *area1,
                                 ply_frame_buffer_area_t *area2,
                                 ply_frame_buffer_area_t *result)
{
  long x1, y1, x2, y2;
  long width, height;

  if (area1->width == 0)
    {
      *result = *area1;
      return;
    }

  if (area2->width == 0)
    {
      *result = *area2;
      return;
    }

  x1 = area1->x + area1->width;
  y1 = area1->y + area1->height;
  x2 = area2->x + area2->width;
  y2 = area2->y + area2->height;

  result->x = MAX(area1->x, area2->x);
  result->y = MAX(area1->y, area2->y);

  width = MIN(x1, x2) - result->x;
  height = MIN(y1, y2) - result->y;
  if (width <= 0 || height <= 0)
    {
      result->width = 0;
      result->height = 0;
    }
  else
    {
      result->width = width;
      result->height = height;
    }
}

bool
ply_frame_buffer_fill_with_gradient (ply_frame_buffer_t      *buffer,
                                     ply_frame_buffer_area_t *area,
                                     uint32_t                 start,
                                     uint32_t                 end)
{
/* The gradient produced is a linear interpolation of the two passed
 * in color stops: start and end.
 *
 * In order to prevent banding when the color stops are too close
 * together, or are stretched over too large an area, we slightly
 * perturb the intermediate colors as we generate them.
 *
 * Before we do this, we store the interpolated color values in a
 * fixed point number with lots of fractional bits.  This is so
 * we don't add noise after the values have been clamped to 8-bits
 *
 * We add random noise to all of the fractional bits of each color
 * channel and also NOISE_BITS worth of noise to the non-fractional
 * part of the color. By default NOISE_BITS is 1.
 *
 * We incorporate the noise by filling the bottom 24 bits of an
 * integer with random bits and then shifting the color channels
 * to the left such that the top 8 bits of the channel overlap
 * the noise by NOISE_BITS. E.g., if NOISE_BITS is 1, then the top
 * 7 bits of each channel won't overlap with the noise, and the 8th
 * bit + fractional bits will.  When the noise and color channel
 * are properly aligned, we add them together, drop the precision
 * of the resulting channels back to 8 bits and stuff the results
 * into a pixel in the frame buffer.
 */
#define NOISE_BITS 1
/* In the color stops, red is 8 bits starting at position 24
 * (since they're argb32 pixels).
 * We want to move those 8 bits such that the bottom NOISE_BITS
 * of them overlap the top of the 24 bits of generated noise.
 * Of course, green and blue are 8 bits away from red and each
 * other, respectively.
 */
#define RED_SHIFT (32 - (24 + NOISE_BITS))
#define GREEN_SHIFT (RED_SHIFT + 8)
#define BLUE_SHIFT (GREEN_SHIFT + 8)
#define NOISE_MASK (0x00ffffff)

/* Once, we've lined up the color channel we're interested in with
 * the noise, we need to mask out the other channels.
 */
#define COLOR_MASK (0xff << (24 - NOISE_BITS))

  uint32_t red, green, blue, red_step, green_step, blue_step, t, pixel;
  uint32_t x, y;
  ply_frame_buffer_area_t cropped_area;

  if (area == NULL)
    area = &buffer->area;

  ply_frame_buffer_area_intersect (area, &buffer->area, &cropped_area);

  red   = (start << RED_SHIFT) & COLOR_MASK;
  green = (start << GREEN_SHIFT) & COLOR_MASK;
  blue  = (start << BLUE_SHIFT) & COLOR_MASK;

  t = (end << RED_SHIFT) & COLOR_MASK;
  red_step = (int32_t) (t - red) / (int32_t) buffer->area.height;
  t = (end << GREEN_SHIFT) & COLOR_MASK;
  green_step = (int32_t) (t - green) / (int32_t) buffer->area.height;
  t = (end << BLUE_SHIFT) & COLOR_MASK;
  blue_step = (int32_t) (t - blue) / (int32_t) buffer->area.height;

  /* we use a fixed seed so that the dithering doesn't change on repaints
   * of the same area.
   */
  srand(100200);

/* FIXME: we assume RAND_MAX is at least 24 bits here, and it is on linux.
 * On some platforms it's only 16its though.  If that were true on linux,
 * then NOISE_BITS would get effectively ignored, since those bits would
 * always overlap with zeros.  We could fix it by running rand() twice
 * per channel generating 32-bits of noise, or by shifting the result of
 * rand() over 8 bits, such that the zeros would be overlapping with the
 * least significant fractional bits of the color channel instead.
 */
#define NOISE() (rand () & NOISE_MASK)

  for (y = buffer->area.y; y < buffer->area.y + buffer->area.height; y++)
    {
      if (cropped_area.y <= y && y < cropped_area.y + cropped_area.height)
        {
          for (x = cropped_area.x; x < cropped_area.x + cropped_area.width; x++)
            {
              pixel =
                  0xff000000 |
                  (((red   + NOISE ()) & COLOR_MASK) >> RED_SHIFT) |
                  (((green + NOISE ()) & COLOR_MASK) >> GREEN_SHIFT) |
                  (((blue  + NOISE ()) & COLOR_MASK) >> BLUE_SHIFT);

              buffer->shadow_buffer[y * buffer->area.width + x] = pixel;
            }
        }

      red += red_step;
      green += green_step;
      blue += blue_step;
    }

  ply_frame_buffer_add_area_to_flush_area (buffer, &cropped_area);

  return ply_frame_buffer_flush (buffer);
}


bool 
ply_frame_buffer_fill_with_argb32_data_with_clip (ply_frame_buffer_t      *buffer,
                                                   ply_frame_buffer_area_t *area,
                                                   ply_frame_buffer_area_t *clip,
                                                   unsigned long            x,
                                                   unsigned long            y,
                                                   uint32_t                *data)
{
  long row, column;
  ply_frame_buffer_area_t cropped_area;

  assert (buffer != NULL);
  assert (ply_frame_buffer_device_is_open (buffer));

  if (area == NULL)
    area = &buffer->area;

  ply_frame_buffer_area_intersect (area, &buffer->area, &cropped_area);

  if (clip)
    ply_frame_buffer_area_intersect (&cropped_area, clip, &cropped_area);

  if (cropped_area.width == 0 || cropped_area.height == 0)
    return true;

  x += cropped_area.x - area->x;
  y += cropped_area.y - area->y;

  for (row = y; row < y + cropped_area.height; row++)
    {
      for (column = x; column < x + cropped_area.width; column++)
        {
          uint32_t pixel_value;

          pixel_value = data[area->width * row + column];
          if ((pixel_value >> 24) == 0x00)
            continue;

          ply_frame_buffer_set_value_at_pixel (buffer,
                                                 cropped_area.x + (column - x),
                                                 cropped_area.y + (row - y),
                                                 pixel_value);

        }
    }

  ply_frame_buffer_add_area_to_flush_area (buffer, &cropped_area);

  return ply_frame_buffer_flush (buffer);
}

bool 
ply_frame_buffer_fill_with_argb32_data(ply_frame_buffer_t      *buffer,
                                                   ply_frame_buffer_area_t *area,
                                                   unsigned long            x,
                                                   unsigned long            y,
                                                   uint32_t                *data)
{
  long row, column;

  assert (buffer != NULL);
  assert (ply_frame_buffer_device_is_open (buffer));

  if (area == NULL)
    area = &buffer->area;


  for (row = y; row < y + area->height; row++)
    {
      for (column = x; column < x + area->width; column++)
        {
          buffer->shadow_buffer[(row-y) * buffer->area.width -x + column] = data[area->width * row + column];

        }
    }

  ply_frame_buffer_add_area_to_flush_area (buffer, area);

  return ply_frame_buffer_flush (buffer);
}
  
const char *
ply_frame_buffer_get_bytes (ply_frame_buffer_t *buffer)
{
  return (char *) buffer->shadow_buffer;
}

#ifdef PLY_FRAME_BUFFER_ENABLE_TEST

#include <math.h>
#include <stdio.h>
#include <sys/time.h>

static double
get_current_time (void)
{
  const double microseconds_per_second = 1000000.0;
  double timestamp;
  struct timeval now = { 0L, /* zero-filled */ };

  gettimeofday (&now, NULL);
  timestamp = ((microseconds_per_second * now.tv_sec) + now.tv_usec) /
               microseconds_per_second;

  return timestamp;
}

static void
animate_at_time (ply_frame_buffer_t *buffer,
                 double          time)
{
  int x, y;
  uint32_t *data;
  ply_frame_buffer_area_t area;

  ply_frame_buffer_get_size (buffer, &area);

  data = calloc (area.width * area.height, sizeof (uint32_t));

  for (y = 0; y < area.height; y++)
    {
      int blue_bit_position;
      uint8_t red, green, blue, alpha;

      blue_bit_position = (int) 64 * (.5 * sin (time) + .5) + (255 - 64);
      blue = rand () % blue_bit_position;
      for (x = 0; x < area.width; x++)
      {
        alpha = 0xff;
        red = (uint8_t) ((y / (area.height * 1.0)) * 255.0);
        green = (uint8_t) ((x / (area.width * 1.0)) * 255.0);

        red = green = (red + green + blue) / 3;

        data[y * area.width + x] = (alpha << 24) | (red << 16) | (green << 8) | blue;
      }
    }

  ply_frame_buffer_fill_with_argb32_data (buffer, NULL, 0, 0, data);
}

int
main (int    argc,
      char **argv)
{
  static unsigned int seed = 0;
  ply_frame_buffer_t *buffer;
  int exit_code;

  exit_code = 0;

  buffer = ply_frame_buffer_new (NULL);

  if (!ply_frame_buffer_open (buffer))
    {
      exit_code = errno;
      perror ("could not open frame buffer");
      return exit_code;
    }

  if (seed == 0)
    {
      seed = (int) get_current_time ();
      srand (seed);
    }

  while ("we want to see ad-hoc animations")
    {
      animate_at_time (buffer, get_current_time ());
    }

  ply_frame_buffer_close (buffer);
  ply_frame_buffer_free (buffer);

  return main (argc, argv);
}

#endif /* PLY_FRAME_BUFFER_ENABLE_TEST */

/* vim: set ts=4 sw=4 expandtab autoindent cindent cino={.5s,(0: */

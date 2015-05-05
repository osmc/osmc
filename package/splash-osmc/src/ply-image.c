/* ply-image.c - png file loader
 *
 * Copyright (C) 2006, 2007 Red Hat, Inc.
 * Copyright (C) 2003 University of Southern California
 * Copyright (c) 2014 Sam Nazarko
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
 * Some implementation taken from the cairo library.
 *
 * Written by: Charlie Brej <cbrej@cs.man.ac.uk>
 *             Kristian Høgsberg <krh@redhat.com>
 *             Ray Strode <rstrode@redhat.com>
 *             Carl D. Worth <cworth@cworth.org>
 */

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <math.h>

#include <png.h>

#include <linux/fb.h>
#include "ply-frame-buffer.h"

#define MIN(a,b) ((a) <= (b)? (a) : (b))
#define MAX(a,b) ((a) >= (b)? (a) : (b))

#define CLAMP(a,b,c) (MIN (MAX ((a), (b)), (c)))


typedef union
{
 uint32_t *as_pixels;
 png_byte *as_png_bytes;
 char *address;
} ply_image_layout_t;

typedef struct _ply_image
{
  char  *filename;
  FILE  *fp;

  ply_image_layout_t layout;
  size_t size;

  long width;
  long height;
} ply_image_t;


static bool ply_image_open_file (ply_image_t *image);
static void ply_image_close_file (ply_image_t *image);

static bool
ply_image_open_file (ply_image_t *image)
{
  assert (image != NULL);

  image->fp = fopen (image->filename, "r");

  if (image->fp == NULL)
    return false;
  return true;
}

static void
ply_image_close_file (ply_image_t *image)
{
  assert (image != NULL);

  if (image->fp == NULL)
    return;
  fclose (image->fp);
  image->fp = NULL;
}

ply_image_t *
ply_image_new (const char *filename)
{
  ply_image_t *image;

  assert (filename != NULL);

  image = calloc (1, sizeof (ply_image_t));

  image->filename = strdup (filename);
  image->fp = NULL;
  image->layout.address = NULL;
  image->size = -1;
  image->width = -1;
  image->height = -1;

  return image;
}

void
ply_image_free (ply_image_t *image)
{
  if (image == NULL)
    return;

  assert (image->filename != NULL);

  if (image->layout.address != NULL)
    {
      free (image->layout.address);
      image->layout.address = NULL;
    }

  free (image->filename);
  free (image);
}

static void
transform_to_argb32 (png_struct   *png,
                     png_row_info *row_info,
                     png_byte     *data)
{
  unsigned int i;

  for (i = 0; i < row_info->rowbytes; i += 4) 
  {
    uint8_t  red, green, blue, alpha;
    uint32_t pixel_value;

    red = data[i + 0];
    green = data[i + 1];
    blue = data[i + 2];
    alpha = data[i + 3];

    red = (uint8_t) CLAMP (((red / 255.0) * (alpha / 255.0)) * 255.0, 0, 255.0);
    green = (uint8_t) CLAMP (((green / 255.0) * (alpha / 255.0)) * 255.0,
                             0, 255.0);
    blue = (uint8_t) CLAMP (((blue / 255.0) * (alpha / 255.0)) * 255.0, 0, 255.0);

    pixel_value = (alpha << 24) | (red << 16) | (green << 8) | (blue << 0);
    memcpy (data + i, &pixel_value, sizeof (uint32_t));
  }
}

static void
transform_to_rgb32 (png_struct   *png,
                     png_row_info *row_info,
                     png_byte     *data)
{
  unsigned int i;

  for (i = 0; i < row_info->rowbytes; i += 4) 
  {
    uint8_t  red, green, blue, alpha;
    uint32_t pixel_value;

    red = data[i + 0];
    green = data[i + 1];
    blue = data[i + 2];
    alpha = data[i + 3];
    pixel_value = (alpha << 24) | (red << 16) | (green << 8) | (blue << 0);
    memcpy (data + i, &pixel_value, sizeof (uint32_t));
  }

}

bool
ply_image_load (ply_image_t *image)
{
  png_struct *png;
  png_info *info;
  png_uint_32 width, height, bytes_per_row, row;
  int bits_per_pixel, color_type, interlace_method;
  png_byte **rows;

  assert (image != NULL);

  if (!ply_image_open_file (image))
    return false;

  png = png_create_read_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  assert (png != NULL);

  info = png_create_info_struct (png);
  assert (info != NULL);

  png_init_io (png, image->fp);

  if (setjmp (png_jmpbuf (png)) != 0)
    {
      ply_image_close_file (image);
      return false;
    }

  png_read_info (png, info);
  png_get_IHDR (png, info,
                &width, &height, &bits_per_pixel,
                &color_type, &interlace_method, NULL, NULL);
  bytes_per_row = 4 * width;

  if (color_type == PNG_COLOR_TYPE_PALETTE)
    png_set_palette_to_rgb (png);

  if ((color_type == PNG_COLOR_TYPE_GRAY) && (bits_per_pixel < 8))
    png_set_expand_gray_1_2_4_to_8 (png);

  if (png_get_valid (png, info, PNG_INFO_tRNS))
    png_set_tRNS_to_alpha (png);

  if (bits_per_pixel == 16)
    png_set_strip_16 (png);

  if (bits_per_pixel < 8)
    png_set_packing (png);

  if ((color_type == PNG_COLOR_TYPE_GRAY)
      || (color_type == PNG_COLOR_TYPE_GRAY_ALPHA))
    png_set_gray_to_rgb (png);

  if (interlace_method != PNG_INTERLACE_NONE)
    png_set_interlace_handling (png);

  png_set_filler (png, 0xff, PNG_FILLER_AFTER);

  png_set_read_user_transform_fn (png, transform_to_rgb32);

  png_read_update_info (png, info);

  rows = malloc (height * sizeof (png_byte *));
  image->layout.address = malloc (height * bytes_per_row);

  for (row = 0; row < height; row++)
    rows[row] = &image->layout.as_png_bytes[row * bytes_per_row];

  png_read_image (png, rows);

  free (rows);
  png_read_end (png, info);
  ply_image_close_file (image);
  png_destroy_read_struct (&png, &info, NULL);

  image->width = width;
  image->height = height;

  return true;
}

uint32_t *
ply_image_get_data (ply_image_t *image)
{
  assert (image != NULL);

  return image->layout.as_pixels;
}

ssize_t
ply_image_get_size (ply_image_t *image)
{
  assert (image != NULL);

  return image->size;
}

long
ply_image_get_width (ply_image_t *image)
{
  assert (image != NULL);

  return image->width;
}

long
ply_image_get_height (ply_image_t *image)
{
  assert (image != NULL);

  return image->height;
}

ply_image_t *
ply_image_resize (ply_image_t *image,
                  long         width,
                  long         height)
{
  ply_image_t *new_image;
  int x, y;
  int old_x, old_y, old_width, old_height;
  float scale_x, scale_y;

  new_image = ply_image_new (image->filename);

  new_image->width = width;
  new_image->height = height;

  if (width % 16 != 0)
  {
    width = (width + (16 - width % 16));
  }

  new_image->layout.address = malloc (height * width * 4);

  old_width = ply_image_get_width (image);
  old_height = ply_image_get_height (image);

  scale_x = ((double) old_width) / width;
  scale_y = ((double) old_height) / height;

  for (y = 0; y < height; y++)
    {
      old_y = y * scale_y;
      for (x=0; x < width; x++)
        {
          old_x = x * scale_x;
          new_image->layout.as_pixels[x + y * width] = image->layout.as_pixels[old_x + old_y * old_width];
        }
    }
  return new_image;
}

ply_image_t *
ply_image_rotate (ply_image_t *image,
                  long         center_x,
                  long         center_y,
                  double       theta_offset)
{
  ply_image_t *new_image;
  int x, y;
  int old_x, old_y;
  int width;
  int height;

  width = ply_image_get_width (image);
  height = ply_image_get_height (image);

  new_image = ply_image_new (image->filename);

  new_image->layout.address = malloc (height * width * 4);
  new_image->width = width;
  new_image->height = height;

  for (y = 0; y < height; y++)
    {
      for (x = 0; x < width; x++)
        {
          double d;
          double theta;

          d = sqrt ((x - center_x) * (x - center_x) + (y - center_y) * (y - center_y));
          theta = atan2 (y - center_y, x - center_x);

          theta -= theta_offset;
          old_x = center_x + d * cos (theta);
          old_y = center_y + d * sin (theta);

          if (old_x < 0 || old_x >= width || old_y < 0 || old_y >= height)
            new_image->layout.as_pixels[x +y * width] = 0x00000000;
          else
            new_image->layout.as_pixels[x + y * width] = image->layout.as_pixels[old_x + old_y * width];
        }
    }
  return new_image;
}


#include "ply-frame-buffer.h"

#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <values.h>

#include <linux/kd.h>

#ifndef FRAMES_PER_SECOND
#define FRAMES_PER_SECOND 50
#endif

static int console_fd;

static bool
hide_cursor (void)
{
  static const char invisible_cursor[] = "\033[?25l\033[?1c";

  if (write (STDOUT_FILENO, invisible_cursor, 
             sizeof (invisible_cursor) - 1) != sizeof (invisible_cursor) - 1)
    return false;

  return true;
}

static void
animate_at_time (ply_frame_buffer_t *buffer,
                 ply_image_t        *image)
{
  ply_frame_buffer_area_t area;
  uint32_t *data;
  long width, height;

  data = ply_image_get_data (image);
  width = ply_image_get_width (image);
  height = ply_image_get_height (image);

  ply_frame_buffer_get_size (buffer, &area);
  area.x = (area.width / 2) - (width / 2);
  area.y = (area.height / 2) - (height / 2);
  area.width = width;
  area.height = height;


  ply_frame_buffer_pause_updates (buffer);
  ply_frame_buffer_fill_with_argb32_data(buffer, &area, 0, 0, data);
  ply_frame_buffer_unpause_updates (buffer);

}

int
main (int    argc,
      char **argv)
{
  ply_image_t *image;
  ply_frame_buffer_t *buffer;
  int exit_code;

  exit_code = 0;

  //hide_cursor ();

  if (argc == 1)
    image = ply_image_new ("/usr/splash.png");
  else
  {
	  int retcode = atoi(argv[1]);
	  switch (retcode)
	  {
			case 0: case 64: case 65:
			image = ply_image_new("/usr/splash.png");
			break;
			case 66: case 1:
			image = ply_image_new("/usr/splash_sad.png");
			case 100:
			image = ply_image_new("/usr/splash_update.png");
			default:
			image = ply_image_new("/usr/splash_sad.png"); /* Unknown error */
	  }
  }

  if (!ply_image_load (image))
    {
      exit_code = errno;
      perror ("could not load image");
      return exit_code;
    }

  console_fd = open ("/dev/tty0", O_RDWR);

  buffer = ply_frame_buffer_new (NULL);

  if (!ply_frame_buffer_open (buffer))
    {
      exit_code = errno;
      perror ("could not open framebuffer");
      return exit_code;
    }

  image = ply_image_resize(image, buffer->area.width, buffer->area.height);

  animate_at_time (buffer, image);

  ply_frame_buffer_close (buffer);
  ply_frame_buffer_free (buffer);

  ply_image_free (image);

  return exit_code;
}


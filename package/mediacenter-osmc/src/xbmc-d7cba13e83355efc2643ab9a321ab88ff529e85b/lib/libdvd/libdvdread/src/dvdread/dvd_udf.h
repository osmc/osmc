/*
 * This code is based on dvdudf by:
 *   Christian Wolff <scarabaeus@convergence.de>.
 *
 * Modifications by:
 *   Billy Biggs <vektor@dumbterm.net>.
 *   Björn Englund <d4bjorn@dtek.chalmers.se>.
 *
 * dvdudf: parse and read the UDF volume information of a DVD Video
 * Copyright (C) 1999 Christian Wolff for convergence integrated media
 * GmbH The author can be reached at scarabaeus@convergence.de, the
 * project's page is at http://linuxtv.org/dvd/
 *
 * This file is part of libdvdread.
 *
 * libdvdread is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * libdvdread is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with libdvdread; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef LIBDVDREAD_DVD_UDF_H
#define LIBDVDREAD_DVD_UDF_H

#include <inttypes.h>

#include "dvdread/dvd_reader.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Looks for a file on the UDF disc/imagefile and returns the block number
 * where it begins, or 0 if it is not found.  The filename should be an
 * absolute pathname on the UDF filesystem, starting with '/'.  For example,
 * '/VIDEO_TS/VTS_01_1.IFO'.  On success, filesize will be set to the size of
 * the file in bytes.
 */
uint32_t UDFFindFile( dvd_reader_t *device, char *filename, uint32_t *size );

void FreeUDFCache(void *cache);
int UDFGetVolumeIdentifier(dvd_reader_t *device,
			   char *volid, unsigned int volid_size);
int UDFGetVolumeSetIdentifier(dvd_reader_t *device,
			      uint8_t *volsetid, unsigned int volsetid_size);
void *GetUDFCacheHandle(dvd_reader_t *device);
void SetUDFCacheHandle(dvd_reader_t *device, void *cache);

#ifdef __cplusplus
};
#endif
#endif /* LIBDVDREAD_DVD_UDF_H */

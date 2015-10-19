/*
 * Copyright (C) 2000, 2001, 2002 Håkan Hjort <d95hjort@dtek.chalmers.se>.
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

#ifndef LIBDVDREAD_NAV_READ_H
#define LIBDVDREAD_NAV_READ_H

#include "nav_types.h"

/**
 * Parsing of NAV data, PCI and DSI parts.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Reads the PCI packet data pointed to into th pci struct.
 *
 * @param pci Pointer to the PCI data structure to be filled in.
 * @param bufffer Pointer to the buffer of the on disc PCI data.
 */
void navRead_PCI(pci_t *, unsigned char *);

/**
 * Reads the DSI packet data pointed to into dsi struct.
 *
 * @param dsi Pointer to the DSI data structure to be filled in.
 * @param bufffer Pointer to the buffer of the on disc DSI data.
 */
void navRead_DSI(dsi_t *, unsigned char *);

#ifdef __cplusplus
};
#endif
#endif /* LIBDVDREAD_NAV_READ_H */

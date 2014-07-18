/**
 *  Copyright (C) 2011-2012  Juho Vähä-Herttua
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 */

#ifndef BASE64_H
#define BASE64_H

typedef struct base64_s base64_t;

base64_t *base64_init(const char *charlist, int use_padding, int skip_spaces);

int base64_encoded_length(base64_t *base64, int srclen);

int base64_encode(base64_t *base64, char *dst, const unsigned char *src, int srclen);
int base64_decode(base64_t *base64, unsigned char **dst, const char *src, int srclen);

void base64_destroy(base64_t *base64);

#endif

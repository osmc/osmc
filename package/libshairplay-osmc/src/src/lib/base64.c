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

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "base64.h"

#define DEFAULT_CHARLIST "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"

#define BASE64_PADDING 0x40
#define BASE64_INVALID 0x80

struct base64_s {
	char charlist[65];
	char charmap[256];
	int charmap_inited;

	int use_padding;
	int skip_spaces;
};

static base64_t default_base64 = {DEFAULT_CHARLIST, "", 0, 1, 0};

static void
initialize_charmap(base64_t *base64)
{
	int i;

	memset(base64->charmap, BASE64_INVALID, sizeof(base64->charmap));
	for (i=0; i<64; i++) {
		base64->charmap[(int)base64->charlist[i]] = i;
	}
	base64->charmap['='] = BASE64_PADDING;
	base64->charmap_inited = 1;
}

base64_t *
base64_init(const char *charlist, int use_padding, int skip_spaces)
{
	base64_t *base64;
	int i;

	if (!charlist) {
		charlist = DEFAULT_CHARLIST;	
	}
	if (strlen(charlist) != 64) {
		return NULL;
	}
	for (i=0; i<64; i++) {
		switch (charlist[i]) {
		case '\r':
		case '\n':
		case '=':
			return NULL;
		}
	}

	base64 = calloc(1, sizeof(base64_t));
	if (!base64) {
		return NULL;
	}
	strncpy(base64->charlist, charlist, sizeof(base64->charlist)-1);
	base64->use_padding = use_padding;
	base64->skip_spaces = skip_spaces;

	return base64;
}

void
base64_destroy(base64_t *base64)
{
	free(base64);
}

int
base64_encoded_length(base64_t *base64, int srclen)
{
	if (!base64) {
		base64 = &default_base64;
	}

	if (base64->use_padding) {
		return ((srclen+2)/3*4)+1;
	} else {
		int strlen = 0;
		switch (srclen % 3) {
		case 2:
			strlen += 1;
		case 1:
			strlen += 2; 
		default:
			strlen += srclen/3*4;
			break;
		}
		return strlen+1;
	}
}

int
base64_encode(base64_t *base64, char *dst, const unsigned char *src, int srclen)
{
	int src_idx, dst_idx;
	int residue;

	if (!base64) {
		base64 = &default_base64;
	}

	residue = 0;
	for (src_idx=dst_idx=0; src_idx<srclen; src_idx++) {
		residue |= src[src_idx];

		switch (src_idx%3) {
		case 0:
			dst[dst_idx++] = base64->charlist[(residue>>2)%64];
			residue &= 0x03;
			break;
		case 1:
			dst[dst_idx++] = base64->charlist[residue>>4];
			residue &= 0x0f;
			break;
		case 2:
			dst[dst_idx++] = base64->charlist[residue>>6];
			dst[dst_idx++] = base64->charlist[residue&0x3f];
			residue = 0;
			break;
		}
		residue <<= 8;
	}

	/* Add padding */
	if (src_idx%3 == 1) {
		dst[dst_idx++] = base64->charlist[residue>>4];
		if (base64->use_padding) {
			dst[dst_idx++] = '=';
			dst[dst_idx++] = '=';
		}
	} else if (src_idx%3 == 2) {
		dst[dst_idx++] = base64->charlist[residue>>6];
		if (base64->use_padding) {
			dst[dst_idx++] = '=';
		}
	}
	dst[dst_idx] = '\0';
	return dst_idx;
}

int
base64_decode(base64_t *base64, unsigned char **dst, const char *src, int srclen)
{
	char *inbuf;
	int inbuflen;
	unsigned char *outbuf;
	int outbuflen;
	char *srcptr;
	int index;

	if (!base64) {
		base64 = &default_base64;
	}
	if (!base64->charmap_inited) {
		initialize_charmap(base64);
	}

	inbuf = malloc(srclen+4);
	if (!inbuf) {
		return -1;
	}
	memcpy(inbuf, src, srclen);
	inbuf[srclen] = '\0';

	/* Remove all whitespaces from inbuf */
	if (base64->skip_spaces) {
		int i, inbuflen = strlen(inbuf);
		for (i=0; i<inbuflen; i++) {
			if (inbuf[i] == '\0') {
				break;
			} else if (isspace(inbuf[i])) {
				memmove(inbuf+i, inbuf+i+1, inbuflen-i);
				inbuflen -= 1;
				i -= 1;
			}
		}
	}

	/* Add padding to inbuf if required */
	inbuflen = strlen(inbuf);
	if (!base64->use_padding) {
		if (inbuflen%4 == 1) {
			free(inbuf);
			return -2;
		}
		if (inbuflen%4 == 2) {
			inbuf[inbuflen] = '=';
			inbuf[inbuflen+1] = '=';
			inbuf[inbuflen+2] = '\0';
			inbuflen += 2;
		} else if (inbuflen%4 == 3) {
			inbuf[inbuflen] = '=';
			inbuf[inbuflen+1] = '\0';
			inbuflen += 1;
		}
	}

	/* Make sure data is divisible by 4 */
	if (inbuflen%4 != 0) {
		free(inbuf);
		return -3;
	}

	/* Calculate the output length without padding */
	outbuflen = inbuflen/4*3;
	if (inbuflen >= 4 && inbuf[inbuflen-1] == '=') {
		outbuflen -= 1;
		if (inbuf[inbuflen-2] == '=') {
			outbuflen -= 1;
		}
	}

	/* Allocate buffer for outputting data */
	outbuf = malloc(outbuflen);
	if (!outbuf) {
		free(inbuf);
		return -4;
	}

	index = 0;
	srcptr = inbuf;
	while (*srcptr) {
		unsigned char a = base64->charmap[(unsigned char)*(srcptr++)];
		unsigned char b = base64->charmap[(unsigned char)*(srcptr++)];
		unsigned char c = base64->charmap[(unsigned char)*(srcptr++)];
		unsigned char d = base64->charmap[(unsigned char)*(srcptr++)];

		if (a == BASE64_INVALID || b == BASE64_INVALID ||
		    c == BASE64_INVALID || d == BASE64_INVALID) {
			return -5;
		}
		if (a == BASE64_PADDING || b == BASE64_PADDING) {
			return -6;
		}

		/* Update the first byte */
		outbuf[index++] = (a << 2) | ((b & 0x30) >> 4);

		/* Update the second byte */
		if (c == BASE64_PADDING) {
			break;
		}
		outbuf[index++] = ((b & 0x0f) << 4) | ((c & 0x3c) >> 2);

		/* Update the third byte */
		if (d == BASE64_PADDING) {
			break;
		}
		outbuf[index++] = ((c & 0x03) << 6) | d;
	}
	if (index != outbuflen) {
		free(inbuf);
		free(outbuf);
		return -7;
	}

	free(inbuf);
	*dst = outbuf;
	return outbuflen;
}

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
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "rsapem.h"
#include "base64.h"

#define RSAPRIVHEADER "-----BEGIN RSA PRIVATE KEY-----"
#define RSAPRIVFOOTER "-----END RSA PRIVATE KEY-----"

struct rsapem_s {
	unsigned char *data;
	unsigned int datalen;
	unsigned int datapos;
};

rsapem_t *
rsapem_init(const char *pemstr)
{
	rsapem_t *rsapem=NULL;
	const char *header;
	const char *footer;
	base64_t *b64dec;
	unsigned char *data;
	int datalen;

	header = strstr(pemstr, RSAPRIVHEADER);
	footer = strstr(pemstr, RSAPRIVFOOTER);
	if (!header || !footer) {
		return NULL;
	}


	/* Base64 decode the whole input excluding header and footer */
	b64dec = base64_init(NULL, 0, 1);
	datalen = base64_decode(b64dec, &data, pemstr+sizeof(RSAPRIVHEADER),
	                        (footer-header)-sizeof(RSAPRIVHEADER));
	base64_destroy(b64dec);
	b64dec = NULL;
	
	if (datalen < 0) {
		return NULL;
	}

#ifdef RSAPEM_DEBUG
	{
		int i;
		printf("Decoded output:\n");
		for (i=0; i<datalen; i++) {
			printf("%02x", data[i]);
		}
		printf("\n");
	}
#endif

	/* Check that first 4 bytes are all valid */
	if (datalen < 4 || data[0] != 0x30 || data[1] != 0x82) {
		free(data);
		return NULL;
	} else if (((data[2] << 8) | data[3]) != datalen-4) {
		free(data);
		return NULL;
	}
	
	rsapem = calloc(1, sizeof(rsapem_t));
	if (!rsapem) {
		free(data);
		return NULL;
	}

	/* Initialize the data */
	rsapem->data = data;
	rsapem->datalen = datalen;
	rsapem->datapos = 4;

	data = NULL;
	datalen = rsapem_read_vector(rsapem, &data);
	if (datalen != 1 && data[0] != 0x00) {
		free(data);
		rsapem_destroy(rsapem);
		return NULL;
	}
	free(data);
	return rsapem;
}

void
rsapem_destroy(rsapem_t *rsapem)
{
	if (rsapem) {
		free(rsapem->data);
		free(rsapem);
	}
}

int
rsapem_read_vector(rsapem_t *rsapem, unsigned char **data)
{
	unsigned int length;
	unsigned char *ptr;

	if (rsapem->datalen-rsapem->datapos < 2) {
		return -1;
	}
	if (rsapem->data[rsapem->datapos] != 0x02) {
		return -2;
	}

	/* Read vector length */
	length = rsapem->data[rsapem->datapos+1];
	if (length <= 0x80) {
		rsapem->datapos += 2;
	} else if (length == 0x81) {
		if (rsapem->datalen-rsapem->datapos < 3) {
			return -3;
		}
		length = rsapem->data[rsapem->datapos+2];
		rsapem->datapos += 3;
	} else if (length == 0x82) {
		if (rsapem->datalen-rsapem->datapos < 4) {
			return -3;
		}
		length = (rsapem->data[rsapem->datapos+2] << 8) |
		          rsapem->data[rsapem->datapos+3];
		rsapem->datapos += 4;
	} else {
		return -3;
	}

	/* Check that we have enough data available */
	if (rsapem->datalen-rsapem->datapos < length) {
		return -4;
	}

	/* Allocate data buffer and read bytes */
	ptr = malloc(length);
	if (!ptr) {
		return -5;
	}
	memcpy(ptr, rsapem->data+rsapem->datapos, length);
	rsapem->datapos += length;

	/* Return buffer and length */
	*data = ptr;
	return length;
}


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
#include <assert.h>
#include <math.h>

#include "raop_buffer.h"
#include "raop_rtp.h"
#include "utils.h"

#include <stdint.h>
#include "crypto/crypto.h"
#include "alac/alac.h"

#define RAOP_BUFFER_LENGTH 32

typedef struct {
	/* Packet available */
	int available;

	/* RTP header */
	unsigned char flags;
	unsigned char type;
	unsigned short seqnum;
	unsigned int timestamp;
	unsigned int ssrc;

	/* Audio buffer of valid length */
	int audio_buffer_size;
	int audio_buffer_len;
	void *audio_buffer;
} raop_buffer_entry_t;

struct raop_buffer_s {
	/* AES key and IV */
	unsigned char aeskey[RAOP_AESKEY_LEN];
	unsigned char aesiv[RAOP_AESIV_LEN];

	/* ALAC decoder */
	ALACSpecificConfig alacConfig;
	alac_file *alac;

	/* First and last seqnum */
	int is_empty;
	unsigned short first_seqnum;
	unsigned short last_seqnum;

	/* RTP buffer entries */
	raop_buffer_entry_t entries[RAOP_BUFFER_LENGTH];

	/* Buffer of all audio buffers */
	int buffer_size;
	void *buffer;
};



static int
get_fmtp_info(ALACSpecificConfig *config, const char *fmtp)
{
	int intarr[12];
	char *original;
	char *strptr;
	int i;

	/* Parse fmtp string to integers */
	original = strptr = strdup(fmtp);
	for (i=0; i<12; i++) {
		if (strptr == NULL) {
			free(original);
			return -1;
		}
		intarr[i] = atoi(utils_strsep(&strptr, " "));
	}
	free(original);
	original = strptr = NULL;

	/* Fill the config struct */
	config->frameLength = intarr[1];
	config->compatibleVersion = intarr[2];
	config->bitDepth = intarr[3];
	config->pb = intarr[4];
	config->mb = intarr[5];
	config->kb = intarr[6];
	config->numChannels = intarr[7];
	config->maxRun = intarr[8];
	config->maxFrameBytes = intarr[9];
	config->avgBitRate = intarr[10];
	config->sampleRate = intarr[11];

	/* Validate supported audio types */
	if (config->bitDepth != 16) {
		return -2;
	}
	if (config->numChannels != 2) {
		return -3;
	}

	return 0;
}

static void
set_decoder_info(alac_file *alac, ALACSpecificConfig *config)
{
	unsigned char decoder_info[48];
	memset(decoder_info, 0, sizeof(decoder_info));

#define SET_UINT16(buf, value)do{\
	(buf)[0] = (unsigned char)((value) >> 8);\
	(buf)[1] = (unsigned char)(value);\
	}while(0)

#define SET_UINT32(buf, value)do{\
	(buf)[0] = (unsigned char)((value) >> 24);\
	(buf)[1] = (unsigned char)((value) >> 16);\
	(buf)[2] = (unsigned char)((value) >> 8);\
	(buf)[3] = (unsigned char)(value);\
	}while(0)

	/* Construct decoder info buffer */
	SET_UINT32(&decoder_info[24], config->frameLength);
	decoder_info[28] = config->compatibleVersion;
	decoder_info[29] = config->bitDepth;
	decoder_info[30] = config->pb;
	decoder_info[31] = config->mb;
	decoder_info[32] = config->kb;
	decoder_info[33] = config->numChannels;
	SET_UINT16(&decoder_info[34], config->maxRun);
	SET_UINT32(&decoder_info[36], config->maxFrameBytes);
	SET_UINT32(&decoder_info[40], config->avgBitRate);
	SET_UINT32(&decoder_info[44], config->sampleRate);
	alac_set_info(alac, (char *) decoder_info);
}

raop_buffer_t *
raop_buffer_init(const char *rtpmap,
                 const char *fmtp,
                 const unsigned char *aeskey,
                 const unsigned char *aesiv)
{
	raop_buffer_t *raop_buffer;
	int audio_buffer_size;
	ALACSpecificConfig *alacConfig;
	int i;

        assert(rtpmap);
	assert(fmtp);
	assert(aeskey);
	assert(aesiv);

	raop_buffer = calloc(1, sizeof(raop_buffer_t));
	if (!raop_buffer) {
		return NULL;
	}

	/* Parse fmtp information */
	alacConfig = &raop_buffer->alacConfig;
	if (get_fmtp_info(alacConfig, fmtp) < 0) {
		free(raop_buffer);
		return NULL;
	}

	/* Allocate the output audio buffers */
	audio_buffer_size = alacConfig->frameLength *
	                    alacConfig->numChannels *
	                    alacConfig->bitDepth/8;
	raop_buffer->buffer_size = audio_buffer_size *
	                           RAOP_BUFFER_LENGTH;
	raop_buffer->buffer = malloc(raop_buffer->buffer_size);
	if (!raop_buffer->buffer) {
		free(raop_buffer);
		return NULL;
	}
	for (i=0; i<RAOP_BUFFER_LENGTH; i++) {
		raop_buffer_entry_t *entry = &raop_buffer->entries[i];
		entry->audio_buffer_size = audio_buffer_size;
		entry->audio_buffer_len = 0;
		entry->audio_buffer = (char *)raop_buffer->buffer+i*audio_buffer_size;
	}

	/* Initialize ALAC decoder */
	raop_buffer->alac = create_alac(alacConfig->bitDepth,
	                                alacConfig->numChannels);
	if (!raop_buffer->alac) {
		free(raop_buffer->buffer);
		free(raop_buffer);
		return NULL;
	}
	set_decoder_info(raop_buffer->alac, alacConfig);

	/* Initialize AES keys */
	memcpy(raop_buffer->aeskey, aeskey, RAOP_AESKEY_LEN);
	memcpy(raop_buffer->aesiv, aesiv, RAOP_AESIV_LEN);

	/* Mark buffer as empty */
	raop_buffer->is_empty = 1;
	return raop_buffer;
}

void
raop_buffer_destroy(raop_buffer_t *raop_buffer)
{
	if (raop_buffer) {
		destroy_alac(raop_buffer->alac);
		free(raop_buffer->buffer);
		free(raop_buffer);
	}
}

const ALACSpecificConfig *
raop_buffer_get_config(raop_buffer_t *raop_buffer)
{
	assert(raop_buffer);

	return &raop_buffer->alacConfig;
}

static short
seqnum_cmp(unsigned short s1, unsigned short s2)
{
	return (s1 - s2);
}

int
raop_buffer_queue(raop_buffer_t *raop_buffer, unsigned char *data, unsigned short datalen, int use_seqnum)
{
	unsigned char packetbuf[RAOP_PACKET_LEN];
	unsigned short seqnum;
	raop_buffer_entry_t *entry;
	int encryptedlen;
	AES_CTX aes_ctx;
	int outputlen;

	assert(raop_buffer);

	/* Check packet data length is valid */
	if (datalen < 12 || datalen > RAOP_PACKET_LEN) {
		return -1;
	}

	/* Get correct seqnum for the packet */
	if (use_seqnum) {
		seqnum = (data[2] << 8) | data[3];
	} else {
		seqnum = raop_buffer->first_seqnum;
	}

	/* If this packet is too late, just skip it */
	if (!raop_buffer->is_empty && seqnum_cmp(seqnum, raop_buffer->first_seqnum) < 0) {
		return 0;
	}

	/* Check that there is always space in the buffer, otherwise flush */
	if (seqnum_cmp(seqnum, raop_buffer->first_seqnum+RAOP_BUFFER_LENGTH) >= 0) {
		raop_buffer_flush(raop_buffer, seqnum);
	}

	/* Get entry corresponding our seqnum */
	entry = &raop_buffer->entries[seqnum % RAOP_BUFFER_LENGTH];
	if (entry->available && seqnum_cmp(entry->seqnum, seqnum) == 0) {
		/* Packet resend, we can safely ignore */
		return 0;
	}

	/* Update the raop_buffer entry header */
	entry->flags = data[0];
	entry->type = data[1];
	entry->seqnum = seqnum;
	entry->timestamp = (data[4] << 24) | (data[5] << 16) |
	                   (data[6] << 8) | data[7];
	entry->ssrc = (data[8] << 24) | (data[9] << 16) |
	              (data[10] << 8) | data[11];
	entry->available = 1;

	/* Decrypt audio data */
	encryptedlen = (datalen-12)/16*16;
	AES_set_key(&aes_ctx, raop_buffer->aeskey, raop_buffer->aesiv, AES_MODE_128);
	AES_convert_key(&aes_ctx);
	AES_cbc_decrypt(&aes_ctx, &data[12], packetbuf, encryptedlen);
	memcpy(packetbuf+encryptedlen, &data[12+encryptedlen], datalen-12-encryptedlen);

	/* Decode ALAC audio data */
	outputlen = entry->audio_buffer_size;
	decode_frame(raop_buffer->alac, packetbuf, entry->audio_buffer, &outputlen);
	entry->audio_buffer_len = outputlen;

	/* Update the raop_buffer seqnums */
	if (raop_buffer->is_empty) {
		raop_buffer->first_seqnum = seqnum;
		raop_buffer->last_seqnum = seqnum;
		raop_buffer->is_empty = 0;
	}
	if (seqnum_cmp(seqnum, raop_buffer->last_seqnum) > 0) {
		raop_buffer->last_seqnum = seqnum;
	}
	return 1;
}

const void *
raop_buffer_dequeue(raop_buffer_t *raop_buffer, int *length, int no_resend)
{
	short buflen;
	raop_buffer_entry_t *entry;

	/* Calculate number of entries in the current buffer */
	buflen = seqnum_cmp(raop_buffer->last_seqnum, raop_buffer->first_seqnum)+1;

	/* Cannot dequeue from empty buffer */
	if (raop_buffer->is_empty || buflen <= 0) {
		return NULL;
	}

	/* Get the first buffer entry for inspection */
	entry = &raop_buffer->entries[raop_buffer->first_seqnum % RAOP_BUFFER_LENGTH];
	if (no_resend) {
		/* If we do no resends, always return the first entry */
	} else if (!entry->available) {
		/* Check how much we have space left in the buffer */
		if (buflen < RAOP_BUFFER_LENGTH) {
			/* Return nothing and hope resend gets on time */
			return NULL;
		}
		/* Risk of buffer overrun, return empty buffer */
	}

	/* Update buffer and validate entry */
	raop_buffer->first_seqnum += 1;
	if (!entry->available) {
		/* Return an empty audio buffer to skip audio */
		*length = entry->audio_buffer_size;
		memset(entry->audio_buffer, 0, *length);
		return entry->audio_buffer;
	}
	entry->available = 0;

	/* Return entry audio buffer */
	*length = entry->audio_buffer_len;
	entry->audio_buffer_len = 0;
	return entry->audio_buffer;
}

void
raop_buffer_handle_resends(raop_buffer_t *raop_buffer, raop_resend_cb_t resend_cb, void *opaque)
{
	raop_buffer_entry_t *entry;

	assert(raop_buffer);
	assert(resend_cb);

	if (seqnum_cmp(raop_buffer->first_seqnum, raop_buffer->last_seqnum) < 0) {
		int seqnum, count;

		for (seqnum=raop_buffer->first_seqnum; seqnum_cmp(seqnum, raop_buffer->last_seqnum)<0; seqnum++) {
			entry = &raop_buffer->entries[seqnum % RAOP_BUFFER_LENGTH];
			if (entry->available) {
				break;
			}
		}
		if (seqnum_cmp(seqnum, raop_buffer->first_seqnum) == 0) {
			return;
		}
		count = seqnum_cmp(seqnum, raop_buffer->first_seqnum);
		resend_cb(opaque, raop_buffer->first_seqnum, count);
	}
}

void
raop_buffer_flush(raop_buffer_t *raop_buffer, int next_seq)
{
	int i;

	assert(raop_buffer);

	for (i=0; i<RAOP_BUFFER_LENGTH; i++) {
		raop_buffer->entries[i].available = 0;
		raop_buffer->entries[i].audio_buffer_len = 0;
	}
	if (next_seq < 0 || next_seq > 0xffff) {
		raop_buffer->is_empty = 1;
	} else {
		raop_buffer->first_seqnum = next_seq;
		raop_buffer->last_seqnum = next_seq-1;
	}
}

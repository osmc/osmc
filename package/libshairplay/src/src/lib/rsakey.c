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
#include <stdint.h>
#include <assert.h>

#include "rsakey.h"
#include "rsapem.h"
#include "base64.h"
#include "crypto/crypto.h"

#define RSA_MIN_PADLEN 8
#define MAX_KEYLEN 512

struct rsakey_s {
	int keylen;             /* length of modulus in bytes */
	BI_CTX *bi_ctx;         /* bigint context */

	bigint *n;              /* modulus */
	bigint *e;              /* public exponent */
	bigint *d;              /* private exponent */

	int use_crt;            /* use chinese remainder theorem */
	bigint *p;              /* p as in m = pq */
	bigint *q;              /* q as in m = pq */
	bigint *dP;             /* d mod (p-1) */
	bigint *dQ;             /* d mod (q-1) */
	bigint *qInv;           /* q^-1 mod p */

	base64_t *base64;
};

rsakey_t *
rsakey_init(const unsigned char *modulus, int mod_len,
            const unsigned char *pub_exp, int pub_len,
            const unsigned char *priv_exp, int priv_len,
            /* Optional, used for crt optimization */
            const unsigned char *p, int p_len,
            const unsigned char *q, int q_len,
            const unsigned char *dP, int dP_len,
            const unsigned char *dQ, int dQ_len,
            const unsigned char *qInv, int qInv_len)
{
	rsakey_t *rsakey;
	int i;

	if (mod_len > MAX_KEYLEN) {
		return NULL;
	}

	rsakey = calloc(1, sizeof(rsakey_t));
	if (!rsakey) {
		return NULL;
	}
	rsakey->base64 = base64_init(NULL, 0, 0);
	if (!rsakey->base64) {
		free(rsakey);
		return NULL;
	}

	/* Initialize structure */
	for (i=0; !modulus[i] && i<mod_len; i++);
	rsakey->keylen = mod_len-i;
	rsakey->bi_ctx = bi_initialize();

	/* Import public and private keys */
	rsakey->n = bi_import(rsakey->bi_ctx, modulus, mod_len);
	rsakey->e = bi_import(rsakey->bi_ctx, pub_exp, pub_len);
	rsakey->d = bi_import(rsakey->bi_ctx, priv_exp, priv_len);

	if (p && q && dP && dQ && qInv) {
		/* Import crt optimization keys */
		rsakey->p = bi_import(rsakey->bi_ctx, p, p_len);
		rsakey->q = bi_import(rsakey->bi_ctx, q, q_len);
		rsakey->dP = bi_import(rsakey->bi_ctx, dP, dP_len);
		rsakey->dQ = bi_import(rsakey->bi_ctx, dQ, dQ_len);
		rsakey->qInv = bi_import(rsakey->bi_ctx, qInv, qInv_len);
	
		/* Set imported keys either permanent or modulo */
		bi_permanent(rsakey->dP);
		bi_permanent(rsakey->dQ);
		bi_permanent(rsakey->qInv);
		bi_set_mod(rsakey->bi_ctx, rsakey->p, BIGINT_P_OFFSET);
		bi_set_mod(rsakey->bi_ctx, rsakey->q, BIGINT_Q_OFFSET);

		rsakey->use_crt = 1;
	}

	/* Add keys to the bigint context */
	bi_set_mod(rsakey->bi_ctx, rsakey->n, BIGINT_M_OFFSET);
	bi_permanent(rsakey->e);
	bi_permanent(rsakey->d);
	return rsakey;
}

rsakey_t *
rsakey_init_pem(const char *pemstr)
{
	rsapem_t *rsapem;
	unsigned char *modulus=NULL; unsigned int mod_len=0;
	unsigned char *pub_exp=NULL; unsigned int pub_len=0;
	unsigned char *priv_exp=NULL; unsigned int priv_len=0;
	unsigned char *p=NULL; unsigned int p_len=0;
	unsigned char *q=NULL; unsigned int q_len=0;
	unsigned char *dP=NULL; unsigned int dP_len=0;
	unsigned char *dQ=NULL; unsigned int dQ_len=0;
	unsigned char *qInv=NULL; unsigned int qInv_len=0;
	rsakey_t *rsakey=NULL;

	rsapem = rsapem_init(pemstr);
	if (!rsapem) {
		return NULL;
	}

	/* Read public and private keys */
	mod_len = rsapem_read_vector(rsapem, &modulus);
	pub_len = rsapem_read_vector(rsapem, &pub_exp);
	priv_len = rsapem_read_vector(rsapem, &priv_exp);
	/* Read private keys for crt optimization */
	p_len = rsapem_read_vector(rsapem, &p);
	q_len = rsapem_read_vector(rsapem, &q);
	dP_len = rsapem_read_vector(rsapem, &dP);
	dQ_len = rsapem_read_vector(rsapem, &dQ);
	qInv_len = rsapem_read_vector(rsapem, &qInv);
	
	if (modulus && pub_exp && priv_exp) {
		/* Initialize rsakey value */
		rsakey = rsakey_init(modulus, mod_len, pub_exp, pub_len, priv_exp, priv_len,
				     p, p_len, q, q_len, dP, dP_len, dQ, dQ_len, qInv, qInv_len);
	}

	free(modulus);
	free(pub_exp);
	free(priv_exp);
	free(p);
	free(q);
	free(dP);
	free(dQ);
	free(qInv);
	rsapem_destroy(rsapem);
	return rsakey;
}

void
rsakey_destroy(rsakey_t *rsakey)
{
	if (rsakey) {
		bi_free_mod(rsakey->bi_ctx, BIGINT_M_OFFSET);
		bi_depermanent(rsakey->e);
		bi_depermanent(rsakey->d);
		bi_free(rsakey->bi_ctx, rsakey->e);
		bi_free(rsakey->bi_ctx, rsakey->d);

		if (rsakey->use_crt) {
			bi_free_mod(rsakey->bi_ctx, BIGINT_P_OFFSET);
			bi_free_mod(rsakey->bi_ctx, BIGINT_Q_OFFSET);
			bi_depermanent(rsakey->dP);
			bi_depermanent(rsakey->dQ);
			bi_depermanent(rsakey->qInv);
			bi_free(rsakey->bi_ctx, rsakey->dP);
			bi_free(rsakey->bi_ctx, rsakey->dQ);
			bi_free(rsakey->bi_ctx, rsakey->qInv);
		}
		bi_terminate(rsakey->bi_ctx);

		base64_destroy(rsakey->base64);
		free(rsakey);
	}
}

static bigint *
rsakey_modpow(rsakey_t *rsakey, bigint *msg)
{
	if (rsakey->use_crt) {
		return bi_crt(rsakey->bi_ctx, msg,
		              rsakey->dP, rsakey->dQ,
		              rsakey->p, rsakey->q, rsakey->qInv);
	} else {
		rsakey->bi_ctx->mod_offset = BIGINT_M_OFFSET;
		return bi_mod_power(rsakey->bi_ctx, msg, rsakey->d);
	}
}

int
rsakey_sign(rsakey_t *rsakey, char *dst, int dstlen, const char *b64digest,
            unsigned char *ipaddr, int ipaddrlen,
            unsigned char *hwaddr, int hwaddrlen)
{
	unsigned char buffer[MAX_KEYLEN];
	unsigned char *digest;
	int digestlen;
	int inputlen;
	bigint *bi_in;
	bigint *bi_out;
	int idx;

	assert(rsakey);

	if (dstlen < base64_encoded_length(rsakey->base64, rsakey->keylen)) {
		return -1;
	}

	/* Decode the base64 digest */
	digestlen = base64_decode(rsakey->base64, &digest, b64digest, strlen(b64digest));
	if (digestlen < 0) {
		return -2;
	}

	/* Calculate the input data length */
	inputlen = digestlen+ipaddrlen+hwaddrlen;
	if (inputlen > rsakey->keylen-3-RSA_MIN_PADLEN) {
		free(digest);
		return -3;
	}
	if (inputlen < 32) {
		/* Minimum size is 32 */
		inputlen = 32;
	}

	/* Construct the input buffer with padding */
	/* See RFC 3447 9.2 for more information */
	idx = 0;
	memset(buffer, 0, sizeof(buffer));
	buffer[idx++] = 0x00;
	buffer[idx++] = 0x01;
	memset(buffer+idx, 0xff, rsakey->keylen-inputlen-3);
	idx += rsakey->keylen-inputlen-3;
	buffer[idx++] = 0x00;
	memcpy(buffer+idx, digest, digestlen);
	idx += digestlen;
	memcpy(buffer+idx, ipaddr, ipaddrlen);
	idx += ipaddrlen;
	memcpy(buffer+idx, hwaddr, hwaddrlen);
	idx += hwaddrlen;

	/* Calculate the signature s = m^d (mod n) */
	bi_in = bi_import(rsakey->bi_ctx, buffer, rsakey->keylen);
	bi_out = rsakey_modpow(rsakey, bi_in);

	/* Encode and save the signature into dst */
	bi_export(rsakey->bi_ctx, bi_out, buffer, rsakey->keylen);
	base64_encode(rsakey->base64, dst, buffer, rsakey->keylen);

	free(digest);
	return 0;
}

/* Mask generation function with SHA-1 hash */
/* See RFC 3447 B.2.1 for more information */
static int
rsakey_mfg1(unsigned char *dst, int dstlen, const unsigned char *seed, int seedlen, int masklen)
{
	SHA1_CTX sha_ctx;
	int iterations;
	int dstpos;
	int i;

	iterations = (masklen+SHA1_SIZE-1)/SHA1_SIZE;
	if (dstlen < iterations*SHA1_SIZE) {
		return -1;
	}

	dstpos = 0;
	for (i=0; i<iterations; i++) {
		unsigned char counter[4];
		counter[0] = (i>>24)&0xff;
		counter[1] = (i>>16)&0xff;
		counter[2] = (i>>8)&0xff;
		counter[3] = i&0xff;

		SHA1_Init(&sha_ctx);
		SHA1_Update(&sha_ctx, seed, seedlen);
		SHA1_Update(&sha_ctx, counter, sizeof(counter));
		SHA1_Final(dst+dstpos, &sha_ctx);
		dstpos += SHA1_SIZE;
	}
	return masklen;
}

/* OAEP decryption with SHA-1 hash */
/* See RFC 3447 7.1.2 for more information */
int
rsakey_decrypt(rsakey_t *rsakey, unsigned char *dst, int dstlen, const char *b64input)
{
	unsigned char buffer[MAX_KEYLEN];
	unsigned char maskbuf[MAX_KEYLEN];
	unsigned char *input;
	int inputlen;
	bigint *bi_in;
	bigint *bi_out;
	int outlen;
	int i, ret;

	assert(rsakey);
	if (!dst || !b64input) {
		return -1;
	}

	memset(buffer, 0, sizeof(buffer));
	inputlen = base64_decode(rsakey->base64, &input, b64input, strlen(b64input));
	if (inputlen < 0 || inputlen > rsakey->keylen) {
		return -2;
	}
	memcpy(buffer+rsakey->keylen-inputlen, input, inputlen);
	free(input);
	input = NULL;

	/* Decrypt the input data m = c^d (mod n) */
	bi_in = bi_import(rsakey->bi_ctx, buffer, rsakey->keylen);
	bi_out = rsakey_modpow(rsakey, bi_in);

	memset(buffer, 0, sizeof(buffer));
	bi_export(rsakey->bi_ctx, bi_out, buffer, rsakey->keylen);

	/* First unmask seed in the buffer */
	ret = rsakey_mfg1(maskbuf, sizeof(maskbuf),
	                  buffer+1+SHA1_SIZE,
	                  rsakey->keylen-1-SHA1_SIZE,
	                  SHA1_SIZE);
	if (ret < 0) {
		return -3;
	}
	for (i=0; i<ret; i++) {
		buffer[1+i] ^= maskbuf[i];
	}

	/* Then unmask the actual message */
	ret = rsakey_mfg1(maskbuf, sizeof(maskbuf),
	                  buffer+1, SHA1_SIZE,
	                  rsakey->keylen-1-SHA1_SIZE);
	if (ret < 0) {
		return -4;
	}
	for (i=0; i<ret; i++) {
		buffer[1+SHA1_SIZE+i] ^= maskbuf[i];
	}

	/* Finally find the first data byte */
	for (i=1+2*SHA1_SIZE; i<rsakey->keylen && !buffer[i++];);

	/* Calculate real output length and return */
	outlen = rsakey->keylen-i;
	if (outlen > dstlen) {
		return -5;
	}
	memcpy(dst, buffer+i, outlen);
	return outlen;
}

int
rsakey_parseiv(rsakey_t *rsakey, unsigned char *dst, int dstlen, const char *b64input)
{
	unsigned char *tmpptr;
	int length;

	assert(rsakey);
	if (!dst || !b64input) {
		return -1;
	}

	length = base64_decode(rsakey->base64, &tmpptr, b64input, strlen(b64input));
	if (length < 0) {
		return -1;
	} else if (length > dstlen) {
		free(tmpptr);
		return -2;
	}

	memcpy(dst, tmpptr, length);
	free(tmpptr);
	return length;
}

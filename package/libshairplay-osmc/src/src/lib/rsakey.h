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

#ifndef RSAKEY_H
#define RSAKEY_H

typedef struct rsakey_s rsakey_t;

rsakey_t *rsakey_init(const unsigned char *modulus, int mod_len,
                      const unsigned char *pub_exp, int pub_len,
                      const unsigned char *priv_exp, int priv_len,
                      const unsigned char *p, int p_len,
                      const unsigned char *q, int q_len,
                      const unsigned char *dP, int dP_len,
                      const unsigned char *dQ, int dQ_len,
                      const unsigned char *qInv, int qInv_len);
rsakey_t *rsakey_init_pem(const char *pemstr);

int rsakey_sign(rsakey_t *rsakey, char *dst, int dstlen, const char *b64digest,
                unsigned char *ipaddr, int ipaddrlen,
                unsigned char *hwaddr, int hwaddrlen);

int rsakey_decrypt(rsakey_t *rsakey, unsigned char *dst, int dstlen, const char *b64input);
int rsakey_parseiv(rsakey_t *rsakey, unsigned char *dst, int dstlen, const char *b64input);

void rsakey_destroy(rsakey_t *rsakey);

#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "compat.h"
#include "utils.h"
#include "crypto/crypto.h"

void
digest_md5_to_hex(const unsigned char *md5buf, char *md5hex)
{
	int i;
	for (i=0; i<MD5_SIZE*2; i++) {
		int val = (i%2) ? md5buf[i/2]&0x0f : (md5buf[i/2]&0xf0)>>4;
		md5hex[i] = (val<10) ? '0'+val : 'a'+(val-10);
	}
}

void
digest_get_response(const char *username, const char *realm,
                    const char *password, const char *nonce,
                    const char *method, const char *uri,
                    char *response)
{
	MD5_CTX md5ctx;
	unsigned char md5buf[MD5_SIZE];
	char md5hex[MD5_SIZE*2];

	/* Calculate first inner MD5 hash */
	MD5_Init(&md5ctx);
	MD5_Update(&md5ctx, (const unsigned char *)username, strlen(username));
	MD5_Update(&md5ctx, (const unsigned char *)":", 1);
	MD5_Update(&md5ctx, (const unsigned char *)realm, strlen(realm));
	MD5_Update(&md5ctx, (const unsigned char *)":", 1);
	MD5_Update(&md5ctx, (const unsigned char *)password, strlen(password));
	MD5_Final(md5buf, &md5ctx);
	digest_md5_to_hex(md5buf, md5hex);

	/* Calculate second inner MD5 hash */
	MD5_Init(&md5ctx);
	MD5_Update(&md5ctx, (const unsigned char *)method, strlen(method));
	MD5_Update(&md5ctx, (const unsigned char *)":", 1);
	MD5_Update(&md5ctx, (const unsigned char *)uri, strlen(uri));
	MD5_Final(md5buf, &md5ctx);

	/* Calculate outer MD5 hash */
	MD5_Init(&md5ctx);
	MD5_Update(&md5ctx, (const unsigned char *)md5hex, sizeof(md5hex));
	MD5_Update(&md5ctx, (const unsigned char *)":", 1);
	MD5_Update(&md5ctx, (const unsigned char *)nonce, strlen(nonce));
	MD5_Update(&md5ctx, (const unsigned char *)":", 1);
	digest_md5_to_hex(md5buf, md5hex);
	MD5_Update(&md5ctx, (const unsigned char *)md5hex, sizeof(md5hex));
	MD5_Final(md5buf, &md5ctx);

	/* Store the final result to response */
	digest_md5_to_hex(md5buf, response);
}

void
digest_generate_nonce(char *result, int resultlen)
{
	MD5_CTX md5ctx;
	unsigned char md5buf[MD5_SIZE];
	char md5hex[MD5_SIZE*2];
	unsigned int time;

	SYSTEM_GET_TIME(time);

	MD5_Init(&md5ctx);
	MD5_Update(&md5ctx, (unsigned char *)&time, sizeof(time));
	MD5_Final(md5buf, &md5ctx);
	digest_md5_to_hex(md5buf, md5hex);

	memset(result, 0, resultlen);
	strncpy(result, md5hex, resultlen-1);
}

int
digest_is_valid(const char *our_realm, const char *password,
                const char *our_nonce, const char *method,
                const char *our_uri, const char *authorization)
{
	char *auth;
	char *current;
	char *value;
	int success;

	/* Get values from authorization */
	char *username = NULL;
	char *realm = NULL;
	char *nonce = NULL;
	char *uri = NULL;
	char *response = NULL;

	/* Buffer for our response */
	char our_response[MD5_SIZE*2+1];

	if (!authorization) {
		return 0;
	}
	current = auth = strdup(authorization);
	if (!auth) {
		return 0;
	}

	/* Check that the type is digest */
	if (strncmp("Digest", current, 6)) {
		free(auth);
		return 0;
	}
	current += 6;

	while ((value = utils_strsep(&current, ",")) != NULL) {
		char *first, *last;

		/* Find first and last characters */
		first = value;
		last = value+strlen(value)-1;

		/* Trim spaces from the value */
		while (*first == ' ' && first < last) first++;
		while (*last == ' ' && last > first) last--;

		/* Validate the last character */
		if (*last != '"') continue;
		else *last = '\0';

		/* Store value if it is relevant */
		if (!strncmp("username=\"", first, 10)) {
			username = first+10;
		} else if (!strncmp("realm=\"", first, 7)) {
			realm = first+7;
		} else if (!strncmp("nonce=\"", first, 7)) {
			nonce = first+7;
		} else if (!strncmp("uri=\"", first, 5)) {
			uri = first+5;
		} else if (!strncmp("response=\"", first, 10)) {
			response = first+10;
		}
	}

	if (!username || !realm || !nonce || !uri || !response) {
		free(auth);
		return 0;
	}
	if (strcmp(realm, our_realm) || strcmp(nonce, our_nonce) || strcmp(uri, our_uri)) {
		free(auth);
		return 0;
	}

	/* Calculate our response */
	memset(our_response, 0, sizeof(our_response));
	digest_get_response(username, realm, password, nonce,
	                    method, uri, our_response);
	success = !strcmp(response, our_response);
	free(auth);

	return success;
}



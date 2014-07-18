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
#include <assert.h>

#include "raop.h"
#include "raop_rtp.h"
#include "rsakey.h"
#include "digest.h"
#include "httpd.h"
#include "sdp.h"

#include "global.h"
#include "utils.h"
#include "netutils.h"
#include "logger.h"
#include "compat.h"

/* Actually 345 bytes for 2048-bit key */
#define MAX_SIGNATURE_LEN 512

/* Let's just decide on some length */
#define MAX_PASSWORD_LEN 64

/* MD5 as hex fits here */
#define MAX_NONCE_LEN 32

struct raop_s {
	/* Callbacks for audio */
	raop_callbacks_t callbacks;

	/* Logger instance */
	logger_t *logger;

	/* HTTP daemon and RSA key */
	httpd_t *httpd;
	rsakey_t *rsakey;

	/* Hardware address information */
	unsigned char hwaddr[MAX_HWADDR_LEN];
	int hwaddrlen;

	/* Password information */
	char password[MAX_PASSWORD_LEN+1];
};

struct raop_conn_s {
	raop_t *raop;
	raop_rtp_t *raop_rtp;

	unsigned char *local;
	int locallen;

	unsigned char *remote;
	int remotelen;

	char nonce[MAX_NONCE_LEN+1];
};
typedef struct raop_conn_s raop_conn_t;

static void *
conn_init(void *opaque, unsigned char *local, int locallen, unsigned char *remote, int remotelen)
{
	raop_conn_t *conn;

	conn = calloc(1, sizeof(raop_conn_t));
	if (!conn) {
		return NULL;
	}
	conn->raop = opaque;
	conn->raop_rtp = NULL;

	if (locallen == 4) {
		logger_log(conn->raop->logger, LOGGER_INFO,
		           "Local: %d.%d.%d.%d",
		           local[0], local[1], local[2], local[3]);
	} else if (locallen == 16) {
		logger_log(conn->raop->logger, LOGGER_INFO,
		           "Local: %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x",
		           local[0], local[1], local[2], local[3], local[4], local[5], local[6], local[7],
		           local[8], local[9], local[10], local[11], local[12], local[13], local[14], local[15]);
	}
	if (remotelen == 4) {
		logger_log(conn->raop->logger, LOGGER_INFO,
		           "Remote: %d.%d.%d.%d",
		           remote[0], remote[1], remote[2], remote[3]);
	} else if (remotelen == 16) {
		logger_log(conn->raop->logger, LOGGER_INFO,
		           "Remote: %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x",
		           remote[0], remote[1], remote[2], remote[3], remote[4], remote[5], remote[6], remote[7],
		           remote[8], remote[9], remote[10], remote[11], remote[12], remote[13], remote[14], remote[15]);
	}

	conn->local = malloc(locallen);
	assert(conn->local);
	memcpy(conn->local, local, locallen);

	conn->remote = malloc(remotelen);
	assert(conn->remote);
	memcpy(conn->remote, remote, remotelen);

	conn->locallen = locallen;
	conn->remotelen = remotelen;

	digest_generate_nonce(conn->nonce, sizeof(conn->nonce));
	return conn;
}

static void
conn_request(void *ptr, http_request_t *request, http_response_t **response)
{
	const char realm[] = "airplay";
	raop_conn_t *conn = ptr;
	raop_t *raop = conn->raop;

	http_response_t *res;
	const char *method;
	const char *cseq;
	const char *challenge;
	int require_auth = 0;

	method = http_request_get_method(request);
	cseq = http_request_get_header(request, "CSeq");
	if (!method || !cseq) {
		return;
	}

	res = http_response_init("RTSP/1.0", 200, "OK");

	/* We need authorization for everything else than OPTIONS request */
	if (strcmp(method, "OPTIONS") != 0 && strlen(raop->password)) {
		const char *authorization;

		authorization = http_request_get_header(request, "Authorization");
		if (authorization) {
			logger_log(conn->raop->logger, LOGGER_DEBUG, "Our nonce: %s", conn->nonce);
			logger_log(conn->raop->logger, LOGGER_DEBUG, "Authorization: %s", authorization);
		}
		if (!digest_is_valid(realm, raop->password, conn->nonce, method, http_request_get_url(request), authorization)) {
			char *authstr;
			int authstrlen;

			/* Allocate the authenticate string */
			authstrlen = sizeof("Digest realm=\"\", nonce=\"\"") + sizeof(realm) + sizeof(conn->nonce) + 1;
			authstr = malloc(authstrlen);

			/* Concatenate the authenticate string */
			memset(authstr, 0, authstrlen);
			strcat(authstr, "Digest realm=\"");
			strcat(authstr, realm);
			strcat(authstr, "\", nonce=\"");
			strcat(authstr, conn->nonce);
			strcat(authstr, "\"");

			/* Construct a new response */
			require_auth = 1;
			http_response_destroy(res);
			res = http_response_init("RTSP/1.0", 401, "Unauthorized");
			http_response_add_header(res, "WWW-Authenticate", authstr);
			free(authstr);
			logger_log(conn->raop->logger, LOGGER_DEBUG, "Authentication unsuccessful, sending Unauthorized");
		} else {
			logger_log(conn->raop->logger, LOGGER_DEBUG, "Authentication successful!");
		}
	}

	http_response_add_header(res, "CSeq", cseq);
	http_response_add_header(res, "Apple-Jack-Status", "connected; type=analog");

	challenge = http_request_get_header(request, "Apple-Challenge");
	if (!require_auth && challenge) {
		char signature[MAX_SIGNATURE_LEN];

		memset(signature, 0, sizeof(signature));
		rsakey_sign(raop->rsakey, signature, sizeof(signature), challenge,
		            conn->local, conn->locallen, raop->hwaddr, raop->hwaddrlen);
		http_response_add_header(res, "Apple-Response", signature);

		logger_log(conn->raop->logger, LOGGER_DEBUG, "Got challenge: %s", challenge);
		logger_log(conn->raop->logger, LOGGER_DEBUG, "Got response: %s", signature);
	}

	if (require_auth) {
		/* Do nothing in case of authentication request */
	} else if (!strcmp(method, "OPTIONS")) {
		http_response_add_header(res, "Public", "ANNOUNCE, SETUP, RECORD, PAUSE, FLUSH, TEARDOWN, OPTIONS, GET_PARAMETER, SET_PARAMETER");
	} else if (!strcmp(method, "ANNOUNCE")) {
		const char *data;
		int datalen;

		unsigned char aeskey[16];
		unsigned char aesiv[16];
		int aeskeylen, aesivlen;

		data = http_request_get_data(request, &datalen);
		if (data) {
			sdp_t *sdp;
			const char *remotestr, *rtpmapstr, *fmtpstr, *aeskeystr, *aesivstr;

			sdp = sdp_init(data, datalen);
			remotestr = sdp_get_connection(sdp);
			rtpmapstr = sdp_get_rtpmap(sdp);
			fmtpstr = sdp_get_fmtp(sdp);
			aeskeystr = sdp_get_rsaaeskey(sdp);
			aesivstr = sdp_get_aesiv(sdp);

			logger_log(conn->raop->logger, LOGGER_DEBUG, "connection: %s", remotestr);
			logger_log(conn->raop->logger, LOGGER_DEBUG, "rtpmap: %s", rtpmapstr);
			logger_log(conn->raop->logger, LOGGER_DEBUG, "fmtp: %s", fmtpstr);
			logger_log(conn->raop->logger, LOGGER_DEBUG, "rsaaeskey: %s", aeskeystr);
			logger_log(conn->raop->logger, LOGGER_DEBUG, "aesiv: %s", aesivstr);

			aeskeylen = rsakey_decrypt(raop->rsakey, aeskey, sizeof(aeskey), aeskeystr);
			aesivlen = rsakey_parseiv(raop->rsakey, aesiv, sizeof(aesiv), aesivstr);
			logger_log(conn->raop->logger, LOGGER_DEBUG, "aeskeylen: %d", aeskeylen);
			logger_log(conn->raop->logger, LOGGER_DEBUG, "aesivlen: %d", aesivlen);

			if (conn->raop_rtp) {
				/* This should never happen */
				raop_rtp_destroy(conn->raop_rtp);
				conn->raop_rtp = NULL;
			}
			conn->raop_rtp = raop_rtp_init(raop->logger, &raop->callbacks, remotestr, rtpmapstr, fmtpstr, aeskey, aesiv);
			if (!conn->raop_rtp) {
				logger_log(conn->raop->logger, LOGGER_ERR, "Error initializing the audio decoder");
				http_response_set_disconnect(res, 1);
			}
			sdp_destroy(sdp);
		}
	} else if (!strcmp(method, "SETUP")) {
		unsigned short remote_cport=0, remote_tport=0;
		unsigned short cport=0, tport=0, dport=0;
		const char *transport;
		char buffer[1024];
		int use_udp;

		transport = http_request_get_header(request, "Transport");
		assert(transport);

		logger_log(conn->raop->logger, LOGGER_INFO, "Transport: %s", transport);
		use_udp = strncmp(transport, "RTP/AVP/TCP", 11);
		if (use_udp) {
			char *original, *current, *tmpstr;

			current = original = strdup(transport);
			if (original) {
				while ((tmpstr = utils_strsep(&current, ";")) != NULL) {
					unsigned short value;
					int ret;

					ret = sscanf(tmpstr, "control_port=%hu", &value);
					if (ret == 1) {
						logger_log(conn->raop->logger, LOGGER_DEBUG, "Found remote control port: %hu", value);
						remote_cport = value;
					}
					ret = sscanf(tmpstr, "timing_port=%hu", &value);
					if (ret == 1) {
						logger_log(conn->raop->logger, LOGGER_DEBUG, "Found remote timing port: %hu", value);
						remote_tport = value;
					}
				}
			}
			free(original);
		}
		if (conn->raop_rtp) {
			raop_rtp_start(conn->raop_rtp, use_udp, remote_cport, remote_tport, &cport, &tport, &dport);
		} else {
			logger_log(conn->raop->logger, LOGGER_ERR, "RAOP not initialized at SETUP, playing will fail!");
			http_response_set_disconnect(res, 1);
		}

		memset(buffer, 0, sizeof(buffer));
		if (use_udp) {
			snprintf(buffer, sizeof(buffer)-1,
			         "RTP/AVP/UDP;unicast;mode=record;timing_port=%hu;events;control_port=%hu;server_port=%hu",
			         tport, cport, dport);
		} else {
			snprintf(buffer, sizeof(buffer)-1,
			         "RTP/AVP/TCP;unicast;interleaved=0-1;mode=record;server_port=%u",
			         dport);
		}
		logger_log(conn->raop->logger, LOGGER_INFO, "Responding with %s", buffer);
		http_response_add_header(res, "Transport", buffer);
		http_response_add_header(res, "Session", "DEADBEEF");
	} else if (!strcmp(method, "SET_PARAMETER")) {
		const char *content_type;
		const char *data;
		int datalen;

		content_type = http_request_get_header(request, "Content-Type");
		data = http_request_get_data(request, &datalen);
		if (!strcmp(content_type, "text/parameters")) {
			char *datastr;
			datastr = calloc(1, datalen+1);
			if (data && datastr && conn->raop_rtp) {
				memcpy(datastr, data, datalen);
				if (!strncmp(datastr, "volume: ", 8)) {
					float vol = 0.0;
					sscanf(datastr+8, "%f", &vol);
					raop_rtp_set_volume(conn->raop_rtp, vol);
				}
			} else if (!conn->raop_rtp) {
				logger_log(conn->raop->logger, LOGGER_WARNING, "RAOP not initialized at SET_PARAMETER volume");
			}
			free(datastr);
		} else if (!strcmp(content_type, "image/jpeg")) {
			logger_log(conn->raop->logger, LOGGER_INFO, "Got image data of %d bytes", datalen);
			if (conn->raop_rtp) {
				raop_rtp_set_coverart(conn->raop_rtp, data, datalen);
			} else {
				logger_log(conn->raop->logger, LOGGER_WARNING, "RAOP not initialized at SET_PARAMETER coverart");
			}
		} else if (!strcmp(content_type, "application/x-dmap-tagged")) {
			logger_log(conn->raop->logger, LOGGER_INFO, "Got metadata of %d bytes", datalen);
			if (conn->raop_rtp) {
				raop_rtp_set_metadata(conn->raop_rtp, data, datalen);
			} else {
				logger_log(conn->raop->logger, LOGGER_WARNING, "RAOP not initialized at SET_PARAMETER metadata");
			}
		}
	} else if (!strcmp(method, "FLUSH")) {
		const char *rtpinfo;
		int next_seq = -1;

		rtpinfo = http_request_get_header(request, "RTP-Info");
		if (rtpinfo) {
			logger_log(conn->raop->logger, LOGGER_INFO, "Flush with RTP-Info: %s", rtpinfo);
			if (!strncmp(rtpinfo, "seq=", 4)) {
				next_seq = strtol(rtpinfo+4, NULL, 10);
			}
		}
		if (conn->raop_rtp) {
			raop_rtp_flush(conn->raop_rtp, next_seq);
		} else {
			logger_log(conn->raop->logger, LOGGER_WARNING, "RAOP not initialized at FLUSH");
		}
	} else if (!strcmp(method, "TEARDOWN")) {
		http_response_add_header(res, "Connection", "close");
		if (conn->raop_rtp) {
			/* Destroy our RTP session */
			raop_rtp_stop(conn->raop_rtp);
			raop_rtp_destroy(conn->raop_rtp);
			conn->raop_rtp = NULL;
		}
	}
	http_response_finish(res, NULL, 0);

	logger_log(conn->raop->logger, LOGGER_DEBUG, "Handled request %s with URL %s", method, http_request_get_url(request));
	*response = res;
}

static void
conn_destroy(void *ptr)
{
	raop_conn_t *conn = ptr;

	if (conn->raop_rtp) {
		/* This is done in case TEARDOWN was not called */
		raop_rtp_destroy(conn->raop_rtp);
	}
	free(conn->local);
	free(conn->remote);
	free(conn);
}

raop_t *
raop_init(int max_clients, raop_callbacks_t *callbacks, const char *pemkey, int *error)
{
	raop_t *raop;
	httpd_t *httpd;
	rsakey_t *rsakey;
	httpd_callbacks_t httpd_cbs;

	assert(callbacks);
	assert(max_clients > 0);
	assert(max_clients < 100);
	assert(pemkey);

	/* Initialize the network */
	if (netutils_init() < 0) {
		return NULL;
	}

	/* Validate the callbacks structure */
	if (!callbacks->audio_init ||
	    !callbacks->audio_process ||
	    !callbacks->audio_destroy) {
		return NULL;
	}

	/* Allocate the raop_t structure */
	raop = calloc(1, sizeof(raop_t));
	if (!raop) {
		return NULL;
	}

	/* Initialize the logger */
	raop->logger = logger_init();

	/* Set HTTP callbacks to our handlers */
	memset(&httpd_cbs, 0, sizeof(httpd_cbs));
	httpd_cbs.opaque = raop;
	httpd_cbs.conn_init = &conn_init;
	httpd_cbs.conn_request = &conn_request;
	httpd_cbs.conn_destroy = &conn_destroy;

	/* Initialize the http daemon */
	httpd = httpd_init(raop->logger, &httpd_cbs, max_clients);
	if (!httpd) {
		free(raop);
		return NULL;
	}

	/* Copy callbacks structure */
	memcpy(&raop->callbacks, callbacks, sizeof(raop_callbacks_t));

	/* Initialize RSA key handler */
	rsakey = rsakey_init_pem(pemkey);
	if (!rsakey) {
		free(httpd);
		free(raop);
		return NULL;
	}

	raop->httpd = httpd;
	raop->rsakey = rsakey;

	return raop;
}

raop_t *
raop_init_from_keyfile(int max_clients, raop_callbacks_t *callbacks, const char *keyfile, int *error)
{
	raop_t *raop;
	char *pemstr;

	if (utils_read_file(&pemstr, keyfile) < 0) {
		return NULL;
	}
	raop = raop_init(max_clients, callbacks, pemstr, error);
	free(pemstr);
	return raop;
}

void
raop_destroy(raop_t *raop)
{
	if (raop) {
		raop_stop(raop);

		httpd_destroy(raop->httpd);
		rsakey_destroy(raop->rsakey);
		logger_destroy(raop->logger);
		free(raop);

		/* Cleanup the network */
		netutils_cleanup();
	}
}

int
raop_is_running(raop_t *raop)
{
	assert(raop);

	return httpd_is_running(raop->httpd);
}

void
raop_set_log_level(raop_t *raop, int level)
{
	assert(raop);

	logger_set_level(raop->logger, level);
}

void
raop_set_log_callback(raop_t *raop, raop_log_callback_t callback, void *cls)
{
	assert(raop);

	logger_set_callback(raop->logger, callback, cls);
}

int
raop_start(raop_t *raop, unsigned short *port, const char *hwaddr, int hwaddrlen, const char *password)
{
	assert(raop);
	assert(port);
	assert(hwaddr);

	/* Validate hardware address */
	if (hwaddrlen > MAX_HWADDR_LEN) {
		return -1;
	}

	memset(raop->password, 0, sizeof(raop->password));
	if (password) {
		/* Validate password */
		if (strlen(password) > MAX_PASSWORD_LEN) {
			return -1;
		}

		/* Copy password to the raop structure */
		strncpy(raop->password, password, MAX_PASSWORD_LEN);
	}

	/* Copy hwaddr to the raop structure */
	memcpy(raop->hwaddr, hwaddr, hwaddrlen);
	raop->hwaddrlen = hwaddrlen;

	return httpd_start(raop->httpd, port);
}

void
raop_stop(raop_t *raop)
{
	assert(raop);

	httpd_stop(raop->httpd);
}


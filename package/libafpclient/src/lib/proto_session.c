

/*
 *  sesssion.c
 *
 *  Copyright (C) 2007 Alex deVries
 *
 */
#include <stdlib.h>
#include <string.h>
#include "dsi.h"
#include "dsi_protocol.h"
#include "afp.h"
#include "utils.h"

int afp_getsessiontoken(struct afp_server * server, int type, 
	unsigned int timestamp, struct afp_token *outgoing_token, 
		struct afp_token * incoming_token)
{
	struct {
		struct dsi_header header __attribute__((__packed__));
		uint8_t command;
		uint8_t pad;
		uint16_t type;
		uint32_t  idlength;
	} __attribute__((__packed__)) * request;


	int ret=0;
	int offset=0;
	int datalen=0;
	char * data;
	int timelen=0;

	datalen=outgoing_token->length;

	request=malloc(sizeof(*request) +AFP_TOKEN_MAX_LEN +sizeof(timestamp));

	switch (type) {
	case kLoginWithTimeAndID:
	case kReconnWithTimeAndID: {
		uint32_t *p = (void *) (((unsigned int) request)+
			sizeof(*request));

		offset=sizeof(timestamp);
		*p = timestamp;
		timelen=sizeof(timestamp);
		}
		break;
	case kRecon1Login:
		break;
	case kLoginWithoutID:
	case kRecon1Refresh:
	case kRecon1ReconnectLogin:
		datalen=0;
		break;
	case kGetKerberosSessionKey:  /* We don't support it */
	case kReconnWithID:/* Deprecated. */
	case kLoginWithID: /* Deprecated. */
	default:
		ret=-1;
		free(request);
		goto error;
	}

	data=(void *) (((unsigned int) request)+sizeof(*request)+offset);
	request->idlength=htonl(datalen);
	request->pad=0;
	request->type=htons(type);
	dsi_setup_header(server, &request->header, DSI_DSICommand);
	request->command = afpGetSessionToken;
	memcpy(data,outgoing_token->data,datalen);

	ret = dsi_send(server, (char *)request, 
		sizeof(*request) + datalen + timelen,
		DSI_DEFAULT_TIMEOUT, afpGetSessionToken, 
		(void *) incoming_token);
	free(request);

	return 0;
error:
	return ret;
}

int afp_getsessiontoken_reply(struct afp_server *server, char *buf, 
	unsigned int size, void * other) {

	struct afp_token * token = other;
	struct {
		struct dsi_header header __attribute__((__packed__));
		uint32_t token_len;
	} * reply = (void *)buf;
	char * token_data = buf + sizeof(*reply);
	unsigned int token_len;

	if (ntohl(reply->header.length)<=sizeof(struct dsi_header)) {
		if (token) token->length=0;
		return 0;
	}

	token_len=ntohl(reply->token_len);
	if ((token_len>AFP_TOKEN_MAX_LEN) ||
	(ntohl(reply->header.length)-sizeof(struct dsi_header) < token_len))
		return -1;

	if (token) {
		memcpy(token->data,token_data,token_len);
		token->length=token_len;
	}

	return 0;
}

int afp_disconnectoldsession(struct afp_server * server, int type, 
	struct afp_token * token)
{
	struct {
		struct dsi_header header __attribute__((__packed__));
		uint8_t command;
		uint8_t pad;
		uint16_t type;
		uint32_t  idlength;
	} __attribute__((__packed__)) * request;
	char * token_data;
	int ret;

	if ((request=malloc(sizeof(*request) + AFP_TOKEN_MAX_LEN))==NULL)
		return -1;

	token_data  = request + sizeof(*request);

	request->type=htons(type);

	if (token->length>AFP_TOKEN_MAX_LEN) return -1;

	dsi_setup_header(server, &request->header, DSI_DSICommand);
	request->command = afpDisconnectOldSession;
	request->idlength=htonl(token->length);
	memcpy(token_data,token->data,token->length);

	ret = dsi_send(server, (char *)request, 
		sizeof(*request) + token->length,
		DSI_DEFAULT_TIMEOUT, afpDisconnectOldSession, NULL);

	free(request);

	return ret;

}

	

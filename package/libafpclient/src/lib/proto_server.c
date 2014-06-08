
/*
 *  proto_server.c
 *
 *  Copyright (C) 2006 Alex deVries
 *
 */
#include <stdlib.h>
#include <string.h>
#include "dsi.h"
#include "afp.h"
#include "utils.h"
#include "dsi_protocol.h"
#include "afp_protocol.h"
#include "codepage.h"
#include "afp_internal.h"

int afp_getsrvrparms(struct afp_server *server)
{
	struct {
		struct dsi_header dsi_header __attribute__((__packed__));
		uint8_t command;
	}  __attribute__((__packed__)) afp_getsrvrparms_request;

	dsi_setup_header(server,&afp_getsrvrparms_request.dsi_header,DSI_DSICommand);
	afp_getsrvrparms_request.command=afpGetSrvrParms;
	dsi_send(server, (char *) &afp_getsrvrparms_request,
		sizeof(afp_getsrvrparms_request),DSI_DEFAULT_TIMEOUT,
		afpGetSrvrParms,NULL);
	return 0;
}


int afp_getsrvrparms_reply(struct afp_server *server, char * msg, unsigned int size, void * ignore)
{
	struct {
		struct dsi_header header __attribute__((__packed__));
		uint32_t time __attribute__((__packed__));
		uint8_t numvolumes;
	}  __attribute__((__packed__)) *afp_getsrvparm_reply = (void *) msg;
	int i;
	char * p;
	struct afp_volume * newvolumes;

	if (size < sizeof(*afp_getsrvparm_reply)) {
		log_for_client(NULL,AFPFSD,LOG_WARNING,"getsrvparm_reply response too short\n");
		return -1;
	}

	server->connect_time = AD_DATE_TO_UNIX(afp_getsrvparm_reply->time);

	server->num_volumes=afp_getsrvparm_reply->numvolumes;

	newvolumes=malloc(afp_getsrvparm_reply->numvolumes * sizeof(struct afp_volume));

	memset(newvolumes,0,afp_getsrvparm_reply->numvolumes * sizeof(struct afp_volume));

	server->volumes=newvolumes;

	p=(char *) (&afp_getsrvparm_reply->numvolumes)+1;

	for (i=0;i<afp_getsrvparm_reply->numvolumes;i++) {
		struct afp_volume * vol;
		vol=&server->volumes[i];
		vol->flags=p[0];
		vol->server=server;
		p++;
		p+=copy_from_pascal(vol->volume_name,p,
			AFP_VOLUME_NAME_LEN)+1;

		/* Here's the logic; until we call openvol, we should
		 * first use the AFP version to figure out what the 
		 * format of the volume name is.  In openvol, we use
		 * the volume attributes bit (since we have it then).
		 */

		if (server->using_version->av_number < 30) 
			memcpy(vol->volume_name_printable,
				vol->volume_name,AFP_VOLUME_NAME_LEN);
		else 
			convert_utf8dec_to_utf8pre(vol->volume_name,
				strlen(vol->volume_name),
				vol->volume_name_printable,
				AFP_VOLUME_NAME_UTF8_LEN);
	}
	return 0;
}


int afp_getsrvrmsg_reply(struct afp_server *server, char *buf, unsigned int size, void * other)
{
	struct afp_getsrvrmsg_reply_packet {
		struct dsi_header dsi_header __attribute__((__packed__));
		uint16_t messagetype;
		uint16_t messagebitmap;
	}  __attribute__((__packed__)) * afp_getsrvrmsg_reply = (void *) buf;

	char * mesg = other, * src;

	if (size < sizeof(struct afp_getsrvrmsg_reply_packet)) {
		log_for_client(NULL,AFPFSD,LOG_WARNING,"getsrvrmsg response too short\n");
		return -1;
	}

	src=buf + (sizeof(struct afp_getsrvrmsg_reply_packet));

	if (ntohs(afp_getsrvrmsg_reply->messagebitmap) & AFP_GETSRVRMSG_UTF8)
		copy_from_pascal_two(mesg,src,200);
	else
		copy_from_pascal(mesg,src,200);

	return 0;

}

int afp_getsrvrmsg(struct afp_server *server, unsigned short messagetype,
	unsigned char utf8, unsigned char block, char * mesg) 
{
	int rc;
	struct afp_getsrvrmsg_request_packet {
		struct dsi_header dsi_header __attribute__((__packed__));
		uint8_t command;
		uint8_t pad;
		uint16_t messagetype __attribute__((__packed__));
		uint16_t messagebitmap __attribute__((__packed__));
	}  __attribute__((__packed__)) afp_getsrvrmsg_request;

	dsi_setup_header(server,&afp_getsrvrmsg_request.dsi_header,DSI_DSICommand);
	afp_getsrvrmsg_request.command=afpGetSrvrMsg;
	afp_getsrvrmsg_request.pad=0;
	afp_getsrvrmsg_request.messagetype=htons(messagetype);
	afp_getsrvrmsg_request.messagebitmap=
		htons( AFP_GETSRVRMSG_GETMSG | (utf8 ? AFP_GETSRVRMSG_UTF8:0)); 
		/* Get the message, and yes, we support UTF8 */
	rc=dsi_send(server, (char *) &afp_getsrvrmsg_request,
		sizeof(afp_getsrvrmsg_request),block,afpGetSrvrMsg,(void *) mesg);

	return rc;
}

int afp_zzzzz(struct afp_server *server)
{

	struct {
		struct dsi_header dsi_header __attribute__((__packed__));
		uint8_t command;
		uint8_t pad;
		uint32_t reserved;
	}  __attribute__((__packed__)) request;

	dsi_setup_header(server,&request.dsi_header,DSI_DSICommand);
	request.command=afpZzzzz;
	request.pad=0;
	request.reserved=0;
	return dsi_send(server, (char *) &request,
		sizeof(request),DSI_DEFAULT_TIMEOUT,afpZzzzz,NULL);
}


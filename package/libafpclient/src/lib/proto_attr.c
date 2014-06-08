/*
 *  attr.c
 *
 *  Copyright (C) 2006 Alex deVries
 *
 */

#include <string.h>
#include <stdlib.h>
#include "dsi.h"
#include "afp.h"
#include "utils.h"
#include "afp_protocol.h"
#include "dsi_protocol.h"

/* This is a new command, function 76.  There are currently no docs, so this 
 * can be nothing but a rough implementation.  It is possible to create
 * some sort of reverse-engineered parser for the return. */

int afp_newcommand76(struct afp_volume * volume, unsigned int dlen, char * data) 
{
	struct {
		struct dsi_header dsi_header __attribute__((__packed__));
		uint8_t command;
		uint8_t pad;
		uint16_t volid ;
	} __attribute__((__packed__)) *request_packet;
	struct afp_server * server=volume->server;
	unsigned int len = sizeof(*request_packet)+dlen;
	int ret;
	char * msg = malloc(len);
	if (!msg) {
		log_for_client(NULL,AFPFSD,LOG_WARNING,"Out of memory\n");
		return -1;
	};

	request_packet=(void *) msg;

	dsi_setup_header(server,&request_packet->dsi_header,DSI_DSICommand);
	request_packet->command=76;
	request_packet->pad=0;
	request_packet->volid=htons(volume->volid);

	char * p=msg+sizeof(*request_packet);

	memcpy(p, data,dlen);

	ret=dsi_send(server, msg, len,DSI_DEFAULT_TIMEOUT, 76 , NULL);
	free(msg);
	
	return ret;
}
int afp_listextattr(struct afp_volume * volume, 
	unsigned int dirid, unsigned short bitmap,
	char * pathname, struct afp_extattr_info * info) 
{
	struct {
		struct dsi_header dsi_header __attribute__((__packed__));
		uint8_t command;
		uint8_t pad;
		uint16_t volid ;
		uint32_t dirid ;
		uint16_t bitmap;
		uint16_t reqcount;
		uint32_t startindex;
		uint32_t maxreplysize;
	} __attribute__((__packed__)) *request_packet;
	struct afp_server * server=volume->server;
	unsigned int len = sizeof(*request_packet)+sizeof_path_header(server)+strlen(pathname);
	char * pathptr;
	int ret;
	char * msg = malloc(len);
	if (!msg) {
		log_for_client(NULL,AFPFSD,LOG_WARNING,"Out of memory\n");
		return -1;
	};
	pathptr = msg + (sizeof(*request_packet));
	request_packet=(void *) msg;

	dsi_setup_header(server,&request_packet->dsi_header,DSI_DSICommand);
	request_packet->command=afpListExtAttrs;
	request_packet->pad=0;
	request_packet->volid=htons(volume->volid);
	request_packet->dirid=htonl(dirid);
	request_packet->reqcount=0;
	request_packet->startindex=0;
	request_packet->bitmap=htons(bitmap);
	request_packet->maxreplysize=hton64(info->maxsize);
	copy_path(server,pathptr,pathname,strlen(pathname));
	unixpath_to_afppath(server,pathptr);

	ret=dsi_send(server, (char *) request_packet,len,DSI_DEFAULT_TIMEOUT, 
		afpListExtAttrs , (void *) info);
	free(msg);
	
	return ret;
}

int afp_listextattrs_reply(struct afp_server * server, char * buf, 
	unsigned int size, void * x)
{

	struct {
		struct dsi_header header __attribute__((__packed__));
		uint16_t reserved ;
		uint32_t datalength ;
		char * data ;
	} __attribute__((__packed__)) * reply = (void *) buf;
	struct afp_extattr_info * i = x;

	unsigned int len = min(i->maxsize,ntohl(reply->datalength));

	i->size=len;

	/* Todo: make sure we don't go past the end of the buffer */

	memcpy(&i->data,&reply->data,len) ;

	return 0;

}


int afp_getextattr(struct afp_volume * volume, unsigned int dirid,
	unsigned short bitmap, unsigned int replysize , 
	char * pathname,
	unsigned short namelen, char * name, struct afp_extattr_info * i)
{
	struct {
		struct dsi_header dsi_header __attribute__((__packed__));
		uint8_t command;
		char pad;
		uint16_t volid ;
		uint32_t dirid ;
		uint16_t bitmap ;
		uint64_t offset ;
		uint64_t reqcount;
		uint32_t replysize;
	} __attribute__((__packed__)) *request_packet;
	struct {
		uint16_t len;
		char * name ;
	} __attribute__((__packed__)) * req2;
	struct afp_server * server = volume->server;
	unsigned int len = sizeof(*request_packet)+
		sizeof_path_header(server)+strlen(pathname)
		+1+sizeof(unsigned int) + strlen(name);
	char * p,*p2;
	int ret;
	char * msg = malloc(len);
	if (!msg) {
		log_for_client(NULL,AFPFSD,LOG_WARNING,"Out of memory\n");
		return -1;
	};
	p= msg + (sizeof(*request_packet));
	request_packet=(void *) msg;

	dsi_setup_header(server,&request_packet->dsi_header,DSI_DSICommand);
	request_packet->command=afpGetExtAttr;
	request_packet->pad=0;
	request_packet->volid=htons(volume->volid);
	request_packet->dirid=htonl(dirid);
	request_packet->offset=hton64(0);
	request_packet->reqcount=hton64(0);
	request_packet->replysize=htonl(replysize);
	copy_path(server,p,pathname,strlen(pathname));
	unixpath_to_afppath(server,p);
	p2=p+sizeof_path_header(server)+strlen(pathname);
	if (((unsigned int ) p2) & 0x1) p2++;
	req2=(void *) p2;

	req2->len=htons(namelen);
	memcpy(&req2->name,name,namelen);

	len=(p2+namelen)-msg;

	ret=dsi_send(server, (char *) request_packet,len,DSI_DEFAULT_TIMEOUT, 
		afpDelete ,(void *) i);

	free(msg);
	
	return ret;
}

int afp_setextattr(struct afp_volume * volume, unsigned int dirid,
	unsigned short bitmap, uint64_t offset, char * pathname,
	unsigned short namelen, char * name, unsigned int attribdatalen,
	char * attribdata)
{
	struct {
		struct dsi_header dsi_header __attribute__((__packed__));
		uint8_t command;
		uint8_t pad;
		uint16_t volid ;
		uint32_t dirid ;
		uint16_t bitmap ;
		uint64_t offset ;
	} __attribute__((__packed__)) *request_packet;
	struct afp_server * server = volume->server;
	unsigned int len = sizeof(*request_packet)+sizeof_path_header(server)+strlen(pathname);
	char * pathptr;
	int ret;
	char * msg = malloc(len);
	if (!msg) {
		log_for_client(NULL,AFPFSD,LOG_WARNING,"Out of memory\n");
		return -1;
	};
	pathptr = msg + (sizeof(*request_packet));
	request_packet=(void *) msg;

	dsi_setup_header(server,&request_packet->dsi_header,DSI_DSICommand);
	request_packet->command=afpSetExtAttr;
	request_packet->pad=0;
	request_packet->volid=htons(volume->volid);
	request_packet->dirid=htonl(dirid);
	copy_path(server,pathptr,pathname,strlen(pathname));
	unixpath_to_afppath(server,pathptr);

	ret=dsi_send(server, (char *) request_packet,len,DSI_DEFAULT_TIMEOUT, 
		afpDelete ,NULL);

	free(msg);
	
	return ret;
}


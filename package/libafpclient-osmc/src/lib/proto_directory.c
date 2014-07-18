
/*
 *  directory.c
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
#include "afp_replies.h"

int afp_moveandrename(struct afp_volume *volume,
	unsigned int src_did, 
	unsigned int dst_did, 
	char * src_path, char * dst_path, char *new_name)
{
	struct {
		struct dsi_header dsi_header __attribute__((__packed__));
		uint8_t command;
		uint8_t pad;
		uint16_t volid ;
		uint32_t src_did;
		uint32_t dst_did ;
	} __attribute__((__packed__)) * request_packet;
	char * p;
	char * msg;
	struct afp_server * server = volume->server;
	unsigned int len;
	unsigned int dlen=0,slen=0,nlen=0;
	int ret;
	unsigned short header_len=sizeof_path_header(server);
	char null_path[255];

	if (dst_path==NULL) {
		null_path[0]='\0';
		dlen=0;
		dst_path=null_path;
	} else {
		dlen=strlen(dst_path);
	}

	if (src_path) slen=strlen(src_path);
	if (new_name) nlen=strlen(new_name);

	len = sizeof(*request_packet) + 
		(3* header_len) + 
                dlen + slen + nlen;

        if ((msg=malloc(len)) == NULL)
                return -1;

	request_packet=(void *) msg;

	dsi_setup_header(server,&request_packet->dsi_header,DSI_DSICommand);

	request_packet->command=afpMoveAndRename;
	request_packet->pad=0;
	request_packet->volid=htons(volume->volid);
	request_packet->src_did=htonl(src_did);
	request_packet->dst_did=htonl(dst_did);

	p=msg+sizeof(*request_packet);
	copy_path(server,p,src_path,slen);
	unixpath_to_afppath(server,p);
	p+=sizeof_path_header(server)+slen;
	
	copy_path(server,p,dst_path,dlen);
	unixpath_to_afppath(server,p);
	p+=sizeof_path_header(server)+dlen;

	copy_path(server,p,new_name,nlen);
	unixpath_to_afppath(server,p);

	ret=dsi_send(server, (char *) msg,len,DSI_DEFAULT_TIMEOUT,afpMoveAndRename,NULL);
	free(msg);
	return ret;

}
int afp_rename(struct afp_volume *volume,
	unsigned int dirid, 
	char * path_from, char * path_to) 
{
	struct {
		struct dsi_header dsi_header __attribute__((__packed__));
		uint8_t command;
		uint8_t pad;
		uint16_t volid;
		uint32_t dirid;
	} __attribute__((__packed__)) * request_packet;
	char * pathfromptr, *pathtoptr;
	char * msg;
	struct afp_server * server = volume->server;
	unsigned int len = sizeof(*request_packet) +
                sizeof_path_header(server) + strlen(path_from) + 
                sizeof_path_header(server) + strlen(path_to);
	int ret;

        if ((msg=malloc(len)) == NULL)
                return -1;

	request_packet=(void *) msg;

	dsi_setup_header(server,&request_packet->dsi_header,DSI_DSICommand);

	request_packet->command=afpRename;
	request_packet->pad=0;
	request_packet->volid=htons(volume->volid);
	request_packet->dirid=htonl(dirid);

	pathfromptr=msg+sizeof(*request_packet);
	copy_path(server,pathfromptr,path_from,strlen(path_from));
	unixpath_to_afppath(server,pathfromptr);

	pathtoptr=pathfromptr+sizeof_path_header(server) + strlen(path_from);

	copy_path(server,pathtoptr,path_to,strlen(path_to));
	unixpath_to_afppath(server,pathtoptr);

	ret=dsi_send(server, (char *) msg,len,DSI_DEFAULT_TIMEOUT,
		afpRename,NULL);
	free(msg);
	return ret;

}

int afp_createdir(struct afp_volume * volume, unsigned int dirid, const char * pathname, unsigned int *did_p)
{
	struct {
		struct dsi_header dsi_header __attribute__((__packed__));
		uint8_t command;
		uint8_t pad;
		uint16_t volid;
		uint32_t dirid;
	} __attribute__((__packed__)) * request_packet;
	char * pathptr;
	char * msg;
	struct afp_server * server = volume->server;
	unsigned int len = sizeof(*request_packet) +
                sizeof_path_header(server) + strlen(pathname);
	int ret;

        if ((msg=malloc(len)) == NULL)
                return -1;

        pathptr=msg+sizeof(*request_packet);
        request_packet = (void *) msg;

	dsi_setup_header(server,&request_packet->dsi_header,DSI_DSICommand);

	request_packet->command=afpCreateDir;
	request_packet->pad=0;
	request_packet->volid=htons(volume->volid);
	request_packet->dirid=htonl(dirid);

	copy_path(server,pathptr,pathname,strlen(pathname));
	unixpath_to_afppath(server,pathptr);

	ret=dsi_send(server, (char *) msg,len,DSI_DEFAULT_TIMEOUT,
		afpCreateDir,(void *)did_p);
	free(msg);
	return ret;

}

int afp_createdir_reply(struct afp_server * server, char * buf, unsigned int size, void * other) 
{

	/* We're actually just going to ignore the return bitmap and forkid. */
	struct {
                struct dsi_header header __attribute__((__packed__));
                uint16_t bitmap;
                uint16_t forkid;
        } __attribute__((__packed__)) * reply_packet = (void *) buf;
	unsigned short * dir_p = (void *) other;

	*dir_p = 0;
	if (reply_packet->header.return_code.error_code)
		return (reply_packet->header.return_code.error_code);

	if (size<sizeof(*reply_packet))
		return -1;

	return 0;
}

int afp_enumerate_reply(struct afp_server *server, char * buf, unsigned int size, void * other) 
{

	struct {
		struct dsi_header dsi_header __attribute__((__packed__));
		uint16_t filebitmap;
		uint16_t dirbitmap;
		uint16_t reqcount;
	} __attribute__((__packed__)) * reply = (void *) buf;

	struct {
		uint8_t size;
		uint8_t isdir;
	} __attribute__((__packed__)) * entry;
	char * p = buf + sizeof(*reply);
	int i;
	char  *max=buf+size;
	struct afp_file_info * filebase = NULL, *filecur=NULL, *prev=NULL;
	void **x = other;

	if (reply->dsi_header.return_code.error_code) {
		return reply->dsi_header.return_code.error_code;
	}

	if (size<sizeof(*reply)) {
		return -1;
	}

	for (i=0;i<ntohs(reply->reqcount);i++) {
		entry  = (void *) p;

		if (p>max) {
			return -1;
		}
		prev=filecur;
		if ((filecur=malloc(sizeof(struct afp_file_info)))==NULL) {
			return -1;
		}
		if (filebase==NULL) {
			filebase=filecur;
			prev=filecur;
		} else {
			prev->next=filecur;
		}

		parse_reply_block(server,p+sizeof(*entry),
			ntohs(entry->size),entry->isdir,
			ntohs(reply->filebitmap), 
			ntohs(reply->dirbitmap), 
			filecur);

		p+=entry->size;
	}

	*x=filebase;

	return 0;
}
int afp_enumerateext2_reply(struct afp_server *server, char * buf, unsigned int size, void * other) 
{

	struct {
		struct dsi_header dsi_header __attribute__((__packed__));
		uint16_t filebitmap;
		uint16_t dirbitmap;
		uint16_t reqcount;
	} __attribute__((__packed__)) * reply = (void *) buf;

	struct {
		uint16_t size;
		uint8_t isdir;
		uint8_t pad;
	} __attribute__((__packed__)) * entry;
	char * p = buf + sizeof(*reply);
	int i;
	char  *max=buf+size;
	struct afp_file_info * filebase = NULL, *filecur=NULL, *new_file=NULL;
	void ** x = other;

	if (reply->dsi_header.return_code.error_code) {
		return reply->dsi_header.return_code.error_code;
	}

	if (size<sizeof(*reply)) {
		return -1;
	}

	for (i=0;i<ntohs(reply->reqcount);i++) {

		if ((new_file=malloc(sizeof(struct afp_file_info)))==NULL) {
			return -1;
		}

		new_file->next=NULL;

		if (filecur) {
			filecur->next=new_file;
			filecur=new_file;
		} else {
			filebase=new_file;
			filecur=new_file;
		}

		entry = p;

		parse_reply_block(server,p+sizeof(*entry),
			ntohs(entry->size),entry->isdir,
			ntohs(reply->filebitmap), 
			ntohs(reply->dirbitmap), 
			filecur);
		p+=ntohs(entry->size);
	}

	*x=filebase;

	return 0;
}

int afp_enumerate(
	struct afp_volume * volume, 
	unsigned int dirid, 
	unsigned int filebitmap, unsigned int dirbitmap,
	unsigned short reqcount, 
	unsigned short startindex,
	char * pathname,
	struct afp_file_info ** file_p)
{
	struct {
		struct dsi_header dsi_header __attribute__((__packed__));
		uint8_t command;
		uint8_t pad;
		uint16_t volid;
		uint32_t dirid;
		uint16_t filebitmap;
		uint16_t dirbitmap;
		uint16_t reqcount;
		uint16_t startindex;
		uint16_t maxreplysize;
	} __attribute__((__packed__)) * afp_enumerate_request_packet;
	unsigned short len;
	char * data;
	int rc;
	struct afp_file_info * files = NULL;
	struct afp_server * server = volume->server;
	char * path;


	len = sizeof_path_header(server) + strlen(pathname) +
		sizeof(*afp_enumerate_request_packet);

	if ((data=malloc(len))==NULL)
		return -1;
	path = data+sizeof(*afp_enumerate_request_packet);


	afp_enumerate_request_packet = (void *) data;

	dsi_setup_header(server,&afp_enumerate_request_packet->dsi_header,DSI_DSICommand);

	afp_enumerate_request_packet->command=afpEnumerate;
	afp_enumerate_request_packet->pad=0;
	afp_enumerate_request_packet->volid=htons(volume->volid);
	afp_enumerate_request_packet->dirid=htonl(dirid);
	afp_enumerate_request_packet->filebitmap=htons(filebitmap);
	afp_enumerate_request_packet->dirbitmap=htons(dirbitmap);
	afp_enumerate_request_packet->reqcount=htons(reqcount);
	afp_enumerate_request_packet->startindex=htons(startindex);
	afp_enumerate_request_packet->maxreplysize=htons(5280);
	copy_path(server,path,pathname,strlen(pathname));
	unixpath_to_afppath(server,path);
	
	rc=dsi_send(server, (char *) data,len,DSI_DEFAULT_TIMEOUT,
		afpEnumerate,(void **) &files);
	*file_p = files;
	free(data);
	return rc;
}

int afp_enumerateext2(
	struct afp_volume * volume, 
	unsigned int dirid, 
	unsigned int filebitmap, unsigned int dirbitmap,
	unsigned short reqcount, 
	unsigned long startindex,
	char * pathname,
	struct afp_file_info ** file_p)
{
	struct {
		struct dsi_header dsi_header __attribute__((__packed__));
		uint8_t command;
		uint8_t pad;
		uint16_t volid;
		uint32_t dirid;
		uint16_t filebitmap;
		uint16_t dirbitmap;
		uint16_t reqcount;
		uint32_t startindex;
		uint32_t maxreplysize;
	} __attribute__((__packed__)) * afp_enumerateext2_request_packet;
	unsigned short len;
	char * data;
	int rc;
	struct afp_file_info * files = NULL;
	struct afp_server * server = volume->server;
	char * path;


	len = sizeof_path_header(server) + strlen(pathname) +
		sizeof(*afp_enumerateext2_request_packet);

	if ((data=malloc(len))==NULL)
		return -1;
	path = data+sizeof(*afp_enumerateext2_request_packet);


	afp_enumerateext2_request_packet = (void *) data;

	dsi_setup_header(server,&afp_enumerateext2_request_packet->dsi_header,DSI_DSICommand);

	afp_enumerateext2_request_packet->command=afpEnumerateExt2;
	afp_enumerateext2_request_packet->pad=0;
	afp_enumerateext2_request_packet->volid=htons(volume->volid);
	afp_enumerateext2_request_packet->dirid=htonl(dirid);
	afp_enumerateext2_request_packet->filebitmap=htons(filebitmap);
	afp_enumerateext2_request_packet->dirbitmap=htons(dirbitmap);
	afp_enumerateext2_request_packet->reqcount=htons(reqcount);
	afp_enumerateext2_request_packet->startindex=htonl(startindex);
	afp_enumerateext2_request_packet->maxreplysize=htonl(5280);
	copy_path(server,path,pathname,strlen(pathname));
	unixpath_to_afppath(server,path);

	
	rc=dsi_send(server, (char *) data,len,DSI_DEFAULT_TIMEOUT,
		afpEnumerateExt2,(void **) &files);

	*file_p = files;
	free(data);
	return rc;

}

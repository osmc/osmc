
/*
 *  volume.c
 *
 *  Copyright (C) 2006 Alex deVries
 *
 */

#include <string.h>
#include <stdlib.h>
#include "dsi.h"
#include "afp.h"
#include "utils.h"
#include "dsi_protocol.h"
#include "afp_protocol.h"
#include "afp_internal.h"
#include "codepage.h"

static int parse_volbitmap_reply(struct afp_server * server, 
		struct afp_volume * tmpvol, 
		unsigned short bitmap,char * msg,unsigned int size) 
{
	char * p = msg;
	unsigned short name_offset = 0;

#define checksize { if (p>msg+size) return -1; }

	if (bitmap & kFPVolAttributeBit) {
		unsigned short *attr = (void *) p;
		checksize
		tmpvol->attributes=ntohs(*attr);
		p+=2;
	}
	if (bitmap & kFPVolSignatureBit) {
		unsigned short *sig = (void *) p;
		tmpvol->signature=ntohs(*sig);
		p+=2;
	}
	if (bitmap & kFPVolCreateDateBit) {
		unsigned int *date = (void *) p;
		tmpvol->creation_date = AD_DATE_TO_UNIX(*date);
		p+=4;
	}
	if (bitmap & kFPVolModDateBit) {
		unsigned int *date = (void *) p;
		tmpvol->modification_date = AD_DATE_TO_UNIX(*date);
		p+=4;
	}
	if (bitmap & kFPVolBackupDateBit) {
		unsigned int *date = (void *) p;
		tmpvol->backup_date = AD_DATE_TO_UNIX(*date);
		p+=4;
	}
	if (bitmap & kFPVolIDBit) {
		unsigned short *discovered_volid = (void *) p;

		tmpvol->volid=ntohs(*discovered_volid);
		p+=2;
	}

	if (bitmap & kFPVolBytesFreeBit) {
		unsigned int * size = (void *) p;
		tmpvol->stat.f_bfree=ntohl(*size);
		p+=4;
	}
	if (bitmap & kFPVolBytesTotalBit) {
		unsigned int * size = (void *) p;
		tmpvol->stat.f_blocks=ntohl(*size);
		p+=4;
	}
	if (bitmap & kFPVolNameBit) {
		unsigned short * name_offset_p = (void *) p;
		name_offset = ntohs(*name_offset_p);
		p+=2;
	}
	if (bitmap & kFPVolExtBytesFreeBit) {
		uint64_t * size = (void *) p;
		tmpvol->stat.f_bfree=ntoh64(*size);
		p+=8;
	}
	if (bitmap & kFPVolExtBytesTotalBit) {
		uint64_t * size = (void *) p;
		tmpvol->stat.f_blocks=ntoh64(*size);
		p+=8;
	}
	if (bitmap & kFPVolBlockSizeBit) {
		unsigned int *size = (void *) p;
		tmpvol->stat.f_bsize=ntohl(*size);
		p+=4;
	}
	return 0;
}

int afp_volclose(struct afp_volume * volume)
{
	struct {
		struct dsi_header dsi_header __attribute__((__packed__));
		uint8_t command;
		uint8_t pad;
		uint16_t volid;
	}  __attribute__((__packed__)) request;
	dsi_setup_header(volume->server,&request.dsi_header,DSI_DSICommand);
	request.command=afpCloseVol;
	request.pad=0;
	request.volid=htons(volume->volid);
        return dsi_send(volume->server, (char *) &request,sizeof(request),
		DSI_DEFAULT_TIMEOUT,afpCloseVol,NULL);
}


int afp_volopen_reply(struct afp_server *server, char * buf, unsigned int size, void *other)
{
	struct {
		struct dsi_header header __attribute__((__packed__));
		uint16_t bitmap;
	}  __attribute__((__packed__)) * afp_volopen_reply_packet = (void *) buf;
	unsigned short bitmap;
	struct afp_volume **v_p = (void *) other;
	struct afp_volume * volume=*v_p;
	
	if (size < sizeof (*afp_volopen_reply_packet))
		return -1;

	bitmap=ntohs(afp_volopen_reply_packet->bitmap);

	if (parse_volbitmap_reply(server,volume,
		bitmap, buf+sizeof(*afp_volopen_reply_packet), 
			size-sizeof(*afp_volopen_reply_packet))!=0) 
		return -1;

	if (volume->attributes & kSupportsUTF8Names) {
		convert_utf8dec_to_utf8pre(volume->volume_name, 
			strlen(volume->volume_name),
			volume->volume_name_printable,
			AFP_VOLUME_NAME_UTF8_LEN);
	} else {
		memcpy(volume->volume_name_printable,
			volume->volume_name,AFP_VOLUME_NAME_LEN);
	}

	return 0;
}

int afp_volopen(struct afp_volume * volume, 
	unsigned short bitmap, char * password) 
{
	struct {
		struct dsi_header dsi_header __attribute__((__packed__));
		uint8_t command;
		uint8_t pad;
		uint16_t bitmap;
	}  __attribute__((__packed__)) * afp_volopen_request;
	char * msg, *volname, * password_ptr;
	unsigned int len = sizeof(*afp_volopen_request) + 
		strlen(volume->volume_name)+1;
	unsigned char len2;
	int ret;

	if (password) {
		len+=AFP_VOLPASS_LEN;
		/* This is undocumented, but put it on 
		an even boundary.  This is how netatalk works. */
		if (len & 0x1) len++;  
	}

	if ((msg=malloc(len))==NULL)
		return -1;
	afp_volopen_request = (void *) msg;

	dsi_setup_header(volume->server,&afp_volopen_request->dsi_header,DSI_DSICommand);
	afp_volopen_request->command=afpOpenVol;
	afp_volopen_request->pad=0;
	afp_volopen_request->bitmap=htons(bitmap);

	volname = msg + sizeof(*afp_volopen_request);
	copy_to_pascal(volname,volume->volume_name);

	if (password) {
		password_ptr=msg+len-AFP_VOLPASS_LEN;
		len2=strlen(password);
		if (len2>AFP_VOLPASS_LEN) len2=AFP_VOLPASS_LEN;
		memset(password_ptr,0,AFP_VOLPASS_LEN);
		memcpy(password_ptr,password,len2);
	}

	ret=dsi_send(volume->server, (char *) msg,len,
		DSI_DEFAULT_TIMEOUT,afpOpenVol,(void *) &volume);
	free(msg);
	return ret;

}


int afp_getvolparms_reply(struct afp_server *server, char * buf, unsigned int size,void * other)
{
	unsigned int bitmap;
	struct afp_volume *volume = (void *) other;

	struct {
		struct dsi_header dsi_header __attribute__((__packed__));
		uint16_t bitmap;
	}  __attribute__((__packed__)) * reply = (void *) buf;

	if (size < sizeof (*reply))
		return -1;

	bitmap = ntohs(reply->bitmap);

	if (!volume) {
		log_for_client(NULL,AFPFSD,LOG_WARNING,"I don't know what volume this is\n");
		return -1;
	}

	/* Go find the correct volume */

	if (parse_volbitmap_reply(server,volume,
		bitmap, buf+sizeof(*reply), 
			size-sizeof(*reply))!=0) 
		return -1;

	return 0;

}

int afp_flush(struct afp_volume * volume)
{
	struct {
		struct dsi_header dsi_header __attribute__((__packed__));
		uint8_t command;
		uint8_t pad;
		uint16_t volid __attribute__((__packed__));
	}  __attribute__((__packed__)) afp_flush_request;
	int ret;

	dsi_setup_header(volume->server,&afp_flush_request.dsi_header,DSI_DSICommand);
	afp_flush_request.command=afpFlush;
	afp_flush_request.pad=0;
	afp_flush_request.volid=htons(volume->volid);

	ret=dsi_send(volume->server, (char *) &afp_flush_request,
		sizeof(afp_flush_request), DSI_DEFAULT_TIMEOUT,
		afpFlush,(void *) volume);
	return ret;
}

int afp_getvolparms(struct afp_volume * volume,unsigned short bitmap) 
{
	struct {
		struct dsi_header dsi_header __attribute__((__packed__));
		uint8_t command;
		uint8_t pad;
		uint16_t volid __attribute__((__packed__));
		uint16_t bitmap __attribute__((__packed__));
	}  __attribute__((__packed__)) afp_getvolparms_request;
	int ret;

	dsi_setup_header(volume->server,&afp_getvolparms_request.dsi_header,DSI_DSICommand);
	afp_getvolparms_request.command=afpGetVolParms;
	afp_getvolparms_request.pad=0;
	afp_getvolparms_request.volid=htons(volume->volid);
	afp_getvolparms_request.bitmap=htons(bitmap);

	ret=dsi_send(volume->server, (char *) &afp_getvolparms_request,
		sizeof(afp_getvolparms_request), DSI_DEFAULT_TIMEOUT,
		afpGetVolParms,(void *) volume);
	return ret;
}


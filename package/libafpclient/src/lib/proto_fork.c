
/*
 *  fork.c
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

int afp_setforkparms(struct afp_volume * volume,
	unsigned short forkid, unsigned short bitmap, unsigned long len)
{

	/* The implementation here deserves some explanation.
	 * If the functin is called with an extended size, use 64 bits.
	 * otherwise, 32.
	 */
	struct {
		struct dsi_header dsi_header __attribute__((__packed__));
		uint8_t command;
		uint8_t pad;
		uint16_t forkid;
		uint16_t bitmap;
		union {
			uint64_t newlen64;
			struct {
				uint32_t newlen;
				uint32_t pad;

			} __attribute__((__packed__)) newlen32;
		} newlen;
	}  __attribute__((__packed__)) request_packet;

	unsigned int actual_len = sizeof(request_packet);


	dsi_setup_header(volume->server,&request_packet.dsi_header,DSI_DSICommand);
	request_packet.command=afpSetForkParms;
	request_packet.pad=0;  
	request_packet.forkid=htons(forkid);
	request_packet.bitmap=htons(bitmap);

	if (bitmap & ( kFPExtDataForkLenBit | kFPExtRsrcForkLenBit )) {
		/* Ah, so this is a long */
		request_packet.newlen.newlen64=htonl(len);
	} else {
		request_packet.newlen.newlen32.newlen=htonl(len);
		actual_len-=4;

	}

	return dsi_send(volume->server, (char *) &request_packet,
		actual_len,DSI_DEFAULT_TIMEOUT,afpSetForkParms,NULL);
}

int afp_closefork(struct afp_volume * volume,
	unsigned short forkid)
{
	struct {
		struct dsi_header dsi_header __attribute__((__packed__));
		uint8_t command;
		uint8_t pad;
		uint16_t forkid;
	}  __attribute__((__packed__)) request_packet;

	dsi_setup_header(volume->server,&request_packet.dsi_header,DSI_DSICommand);
	request_packet.command=afpCloseFork;
	request_packet.pad=0;  
	request_packet.forkid=htons(forkid);

	return dsi_send(volume->server, (char *) &request_packet,
		sizeof(request_packet),DSI_DEFAULT_TIMEOUT,afpFlushFork,NULL);
}


int afp_flushfork(struct afp_volume * volume,
	unsigned short forkid)
{
	struct {
		struct dsi_header dsi_header __attribute__((__packed__));
		uint8_t command;
		uint8_t pad;
		uint16_t forkid;
	}  __attribute__((__packed__)) request_packet;

	dsi_setup_header(volume->server,&request_packet.dsi_header,DSI_DSICommand);
	request_packet.command=afpFlushFork;
	request_packet.pad=0;  
	request_packet.forkid=htons(forkid);

	return dsi_send(volume->server, (char *) &request_packet,
		sizeof(request_packet),DSI_DEFAULT_TIMEOUT,afpFlushFork,NULL);
}



int afp_openfork_reply(struct afp_server *server, char * buf, unsigned int size, void * x)
{
	struct {
		struct dsi_header header __attribute__((__packed__));
		uint16_t bitmap;
		uint16_t forkid;
	}  __attribute__((__packed__)) * afp_openfork_reply_packet = (void *) buf;
	struct afp_file_info * fp=x;
	/* For convenience... */
	struct dsi_header * header = &afp_openfork_reply_packet->header;

	if ((header->return_code.error_code==kFPNoErr) || 
	 	(header->return_code.error_code==kFPDenyConflict)) {
		if (size < sizeof (*afp_openfork_reply_packet)) {
			log_for_client(NULL,AFPFSD,LOG_ERR,
				"openfork response is too short\n");
			return -1;
		}
		fp->forkid=ntohs(afp_openfork_reply_packet->forkid);
	}
	/* We end up ignoring the reply bitmap */


	return 0;
}

int afp_openfork(struct afp_volume * volume,
	unsigned char forktype,
	unsigned int dirid, 
	unsigned short accessmode,
	char * filename,
	struct afp_file_info * fp)
{
	struct {
		struct dsi_header dsi_header __attribute__((__packed__));
		uint8_t command;
		uint8_t forktype;
		uint16_t volid;
		uint32_t dirid ;
		uint16_t bitmap ;
		uint16_t accessmode; 
	}  __attribute__((__packed__)) * afp_openfork_request;
	char * msg;
	char *pathptr;
	struct afp_server * server = volume->server;
	unsigned int len = sizeof(*afp_openfork_request) + 
		sizeof_path_header(server) + strlen(filename);
	int ret;

	if ((msg=malloc(len)) == NULL) 
		return -1;

	pathptr=msg+sizeof(*afp_openfork_request);
	afp_openfork_request = (void *) msg;

	dsi_setup_header(server,&afp_openfork_request->dsi_header,DSI_DSICommand);
	afp_openfork_request->command=afpOpenFork;
	afp_openfork_request->forktype=forktype ? AFP_FORKTYPE_RESOURCE : AFP_FORKTYPE_DATA;
	afp_openfork_request->bitmap=0;  
	afp_openfork_request->volid=htons(volume->volid);
	afp_openfork_request->dirid=htonl(dirid);
	afp_openfork_request->accessmode=htons(accessmode);

	copy_path(server,pathptr,filename,strlen(filename));
	unixpath_to_afppath(server,pathptr);

	ret=dsi_send(server, (char *) msg,len,DSI_DEFAULT_TIMEOUT,
		afpOpenFork,(void *) fp);
	free(msg);
	return ret;
}


int afp_byterangelock(struct afp_volume * volume,
	unsigned char flag,
	unsigned short forkid, 
	uint32_t offset,
	uint32_t len, uint32_t *generated_offset) 
{
	struct {
		struct dsi_header dsi_header __attribute__((__packed__));
		uint8_t command;
		uint8_t flag;
		uint16_t forkid;
		uint32_t offset;
		uint32_t len;
	}  __attribute__((__packed__)) request;
	int rc;

	dsi_setup_header(volume->server,&request.dsi_header,DSI_DSICommand);
	request.command=afpByteRangeLock;
	request.flag=flag;
	request.forkid=htons(forkid);
	request.offset=htonl(offset);
	request.len=htonl(len);
	rc=dsi_send(volume->server, (char *) &request,
		sizeof(request),DSI_DEFAULT_TIMEOUT,
		afpByteRangeLock,(void *) generated_offset);
	return rc;
}

int afp_byterangelock_reply(struct afp_server *server, char * buf, unsigned int size, void * x)
{
	struct {
		struct dsi_header header __attribute__((__packed__));
		uint64_t offset;
	}  __attribute__((__packed__)) * reply = (void *) buf;
	uint32_t *offset=x;
	*offset=0;

	if (size>=sizeof(*reply)) 
		*offset=ntohl(reply->offset);

	return reply->header.return_code.error_code;
}

int afp_byterangelockext(struct afp_volume * volume,
	unsigned char flag,
	unsigned short forkid, 
	uint64_t offset,
	uint64_t len, uint64_t *generated_offset) 
{
	struct {
		struct dsi_header dsi_header __attribute__((__packed__));
		uint8_t command;
		uint8_t flag;
		uint16_t forkid;
		uint64_t offset;
		uint64_t len;
	}  __attribute__((__packed__)) request;
	int rc;

	dsi_setup_header(volume->server,&request.dsi_header,DSI_DSICommand);
	request.command=afpByteRangeLockExt;
	request.flag=flag;
	request.forkid=htons(forkid);
	request.offset=hton64(offset);
	request.len=hton64(len);
	rc=dsi_send(volume->server, (char *) &request,
		sizeof(request),DSI_DEFAULT_TIMEOUT,
		afpByteRangeLockExt,(void *) generated_offset);
	return rc;
}

int afp_byterangelockext_reply(struct afp_server *server, char * buf, unsigned int size, void * x)
{
	struct {
		struct dsi_header header __attribute__((__packed__));
		uint64_t offset;
	}  __attribute__((__packed__)) * reply = (void *) buf;
	uint64_t *offset=x;
	*offset=0;

	if (size>=sizeof(*reply)) 
		*offset=ntoh64(reply->offset);

	return reply->header.return_code.error_code;
}

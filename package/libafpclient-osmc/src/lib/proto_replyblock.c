/*
 *  reply_block.c
 *
 *  Copyright (C) 2006 Alex deVries
 *
 */

#include <string.h>
#include "dsi.h"
#include "afp.h"
#include "utils.h"
#include "afp_internal.h"


/* FIXME: should do bounds checking */
int parse_reply_block(struct afp_server *server, char * buf, 
	unsigned int size, unsigned char isdir, unsigned int filebitmap, 
	unsigned int dirbitmap, 
	struct afp_file_info * filecur) 
{

	unsigned short bitmap;
	char * p2;

	memset(filecur,0,sizeof(struct afp_file_info));

	filecur->isdir=isdir;
	p2=buf;

	if (isdir) bitmap=dirbitmap ; 
		else bitmap=filebitmap;

	if (bitmap & kFPAttributeBit) {
		unsigned short * attr = (void *) p2;
		filecur->attributes=ntohs(*attr);
		p2+=2;
	}
	if (bitmap & kFPParentDirIDBit) {
		unsigned int * did= (void *) p2;
		filecur->did=ntohl(*did);
		p2+=4;
	}
	if (bitmap & kFPCreateDateBit) {
		unsigned int * date= (void *) p2;
		filecur->creation_date=AD_DATE_TO_UNIX(*date);
		p2+=4;
	}
	if (bitmap & kFPModDateBit) {
		unsigned int * date= (void *) p2;
		filecur->modification_date=AD_DATE_TO_UNIX(*date);
		p2+=4;
	}
	if (bitmap & kFPBackupDateBit) {
		unsigned int * date= (void *) p2;
		filecur->backup_date=AD_DATE_TO_UNIX(*date);
		p2+=4;
	}
	if (bitmap & kFPFinderInfoBit) {
		memcpy(filecur->finderinfo,p2,32);
		p2+=32;
	}
	if (bitmap & kFPLongNameBit) {
		unsigned short *offset = (void *) p2;
		copy_from_pascal(filecur->name,buf+(ntohs(*offset)),AFP_MAX_PATH);
		p2+=2;
	}
	if (bitmap & kFPShortNameBit) {
		p2+=2;
	}
	if (bitmap & kFPNodeIDBit) {
		unsigned int * id = (void *) p2;
		filecur->fileid=ntohl(*id);
		p2+=4;
	}
	if (isdir) {
		if (bitmap & kFPOffspringCountBit) {
			unsigned short *offspring = (void *) p2;
			filecur->offspring=ntohs(*offspring);
			p2+=2;
		}
		if (bitmap & kFPOwnerIDBit) {
			unsigned int * owner= (void *) p2;
			filecur->unixprivs.uid=ntohl(*owner);
			p2+=4;
		}
		if (bitmap & kFPGroupIDBit) {
			unsigned int * group= (void *) p2;
			filecur->unixprivs.gid=ntohl(*group);
			p2+=4;
		}
		if (bitmap & kFPAccessRightsBit) {
			unsigned int * access= (void *) p2;
			filecur->accessrights=ntohl(*access);
			p2+=4;
		}
	} else {
		if (bitmap & kFPDataForkLenBit) {
			unsigned int * len = (void *) p2;
			filecur->size=ntohl(*len);
			p2+=4;
		}
		if (bitmap & kFPRsrcForkLenBit) {
			unsigned int  * size = (void *) p2;
			filecur->resourcesize=ntohl(*size);
			p2+=4;
		}
		if (bitmap & kFPExtDataForkLenBit) {
			unsigned long long * len = (void *) p2;
			filecur->size=ntoh64(*len);
			p2+=8;
		}
		if (bitmap & kFPLaunchLimitBit) {
			p2+=2;
		}
	}
	if (bitmap & kFPUTF8NameBit) {
		unsigned short *offset = (void *) p2;
		copy_from_pascal_two(filecur->name,buf+(ntohs(*offset))+4,
			AFP_MAX_PATH);
		p2+=2;
		p2+=4;
	}
	if (bitmap & kFPExtRsrcForkLenBit) {
			unsigned long long * size = (void *) p2;
			filecur->resourcesize=ntoh64(*size);
			p2+=8;
	}
	if (bitmap & kFPUnixPrivsBit) {
		struct afp_unixprivs *unixpriv = (void *) p2;

		filecur->unixprivs.uid=ntohl(unixpriv->uid);
		filecur->unixprivs.gid=ntohl(unixpriv->gid);
		filecur->unixprivs.permissions=ntohl(unixpriv->permissions);
		filecur->unixprivs.ua_permissions=ntohl(unixpriv->ua_permissions);
		p2+=sizeof(*unixpriv);
	}
		
	return 0;
}


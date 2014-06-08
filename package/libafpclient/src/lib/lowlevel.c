

/*
    lowlevel.c: some functions that abstract common operations; used
	so the same code can be used between meta and normal files 

    Copyright (C) 2008 Alex deVries <alexthepuffin@gmail.com>

    This program can be distributed under the terms of the GNU GPL.
    See the file COPYING.
*/

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#ifdef __linux__
#include <asm/fcntl.h>
#else
#include <fcntl.h>
#endif
#include "afp.h"
#include "afp_protocol.h"
#include "codepage.h"
#include "utils.h"
#include "did.h"
#include "users.h"

static void set_nonunix_perms(unsigned int * mode, struct afp_file_info *fp) 
{
	if (fp->isdir) 
		*mode = 0700 | S_IFDIR;
	else 
		*mode = 0600 | S_IFREG;
}

int ll_handle_unlocking(struct afp_volume * volume,unsigned short forkid,
	uint64_t offset, uint64_t sizetorequest)
{
	uint64_t generated_offset;
	int rc;

	if (volume->extra_flags & VOLUME_EXTRA_FLAGS_NO_LOCKING) 
		return 0;

	if (volume->server->using_version->av_number < 30) 
		rc=afp_byterangelock(volume,ByteRangeLock_Unlock,
				forkid,offset, sizetorequest,
				(uint32_t *) &generated_offset);
	else 
		rc=afp_byterangelockext(volume,ByteRangeLock_Unlock,
				forkid,offset, sizetorequest,&generated_offset);
	switch(rc) {
		case kFPNoErr:
			break;
		case kFPMiscErr:
		case kFPParamErr:
		case kFPRangeNotLocked:
		default:
			return -1;
	}
	return 0;
}

int ll_handle_locking(struct afp_volume * volume,unsigned short forkid, 
	uint64_t offset, uint64_t sizetorequest)
{

#define MAX_LOCKTRYCOUNT 10

	int rc=0;
	int try=0;
	uint64_t generated_offset;

	if (volume->extra_flags & VOLUME_EXTRA_FLAGS_NO_LOCKING) 
		return 0;

	while (try<MAX_LOCKTRYCOUNT) {
		try++;
		if (volume->server->using_version->av_number < 30) 
			rc=afp_byterangelock(volume,ByteRangeLock_Lock,
				forkid,offset, sizetorequest,
				(uint32_t *) &generated_offset);
		else 
			rc=afp_byterangelockext(volume,ByteRangeLock_Lock,
				forkid,offset, sizetorequest,&generated_offset);
		switch(rc) {
		case kFPNoErr:
			goto done;
		case kFPNoMoreLocks: /* Max num of locks on server */
		case kFPLockErr:  /*Some or all of the requested range is locked
				    by another user. */

			sleep(1);
			break;
		default:
			return -1;
		}
	}
done:

	return 0;
}




/* zero_file()
 *
 * This function will truncate the fork given to zero bytes in length.
 * This has been abstracted because there is some differences in the
 * expectation of Ext or not Ext. */

int ll_zero_file(struct afp_volume * volume, unsigned short forkid,
	unsigned int resource)
{
	unsigned int bitmap;
	int ret;

	/* The Airport Extreme 7.1.1 will crash if you send it
	 * DataForkLenBit.  Netatalk replies with an error if you
	 * send it ExtDataForkLenBit.  So we need to choose. */

	if ((volume->server->using_version->av_number < 30)  ||
		(volume->server->server_type==AFPFS_SERVER_TYPE_NETATALK))
		bitmap=(resource ? 
			kFPRsrcForkLenBit : kFPDataForkLenBit);
	else
		bitmap=(resource ? 
			kFPExtRsrcForkLenBit : kFPExtDataForkLenBit);

	ret=afp_setforkparms(volume,forkid,bitmap,0);
	switch (ret) {
		case kFPAccessDenied:
			ret=EACCES;
			break;
		case kFPVolLocked:
		case kFPLockErr:
			ret=EBUSY;
			break;
		case kFPDiskFull:
			ret=ENOSPC;
			break;
		case kFPBitmapErr:
		case kFPMiscErr:
		case kFPParamErr:
			ret=EIO;
			break;
		default:
			ret=0;
	}
	return ret;
}


/* get_directory_entry is used to abstract afp_getfiledirparms
 *  * because in AFP<3.0 there is only afp_getfileparms and afp_getdirparms.
 *   */

int ll_get_directory_entry(struct afp_volume * volume,
	char * basename,
	unsigned int dirid,
	unsigned int filebitmap, unsigned int dirbitmap,
	struct afp_file_info *p)
{
	int ret;
	char tmpname[AFP_MAX_PATH];

	memcpy(tmpname,p->basename,AFP_MAX_PATH);
	ret =afp_getfiledirparms(volume,dirid,
		filebitmap,dirbitmap,basename,p);
	memcpy(p->basename,tmpname,AFP_MAX_PATH);
	return ret;
}



int ll_open(struct afp_volume * volume, const char *path, int flags, 
	struct afp_file_info *fp)
{

/* FIXME:  doesn't handle create properly */

	int ret, dsi_ret,rc;
	int create_file=0;
	char converted_path[AFP_MAX_PATH];
	unsigned char aflags = AFP_OPENFORK_ALLOWREAD;

	if (flags & O_RDONLY) aflags|=AFP_OPENFORK_ALLOWREAD;
	if (flags & O_WRONLY) aflags|=AFP_OPENFORK_ALLOWWRITE;        
	if (flags & O_RDWR) 
		aflags |= (AFP_OPENFORK_ALLOWREAD | AFP_OPENFORK_ALLOWWRITE);


	if ((aflags&AFP_OPENFORK_ALLOWWRITE) & 
		(volume_is_readonly(volume))) {
		ret=EPERM;
		goto error;
	}

	/*
	   O_NOBLOCK - todo: it is unclear how to this in fuse.
	*/

	/* The following flags don't need any attention here:
	   O_ASYNC - not relevant for files
	   O_APPEND
	   O_NOATIME - we have no way to handle this anyway
	*/


	/*this will be used later for caching*/
#ifdef __linux__
	fp->sync=(flags & (O_SYNC | O_DIRECT));  
#else
	fp->sync=(flags & (O_SYNC));  
#endif

	/* See if we need to create the file  */
	if (aflags & AFP_OPENFORK_ALLOWWRITE) {
		if (create_file) {
			/* Create the file */
			if (flags & O_EXCL) {
				ret=EEXIST;
				goto error;
			}
			rc=afp_createfile(volume,kFPSoftCreate,fp->did,fp->basename);
		} 
	}

	if (
#ifdef __linux__
		(flags & O_LARGEFILE) && 
#endif
		(volume->server->using_version->av_number<30)) 
	{
		switch(ll_get_directory_entry(volume,fp->basename,fp->did,
			kFPParentDirIDBit|kFPNodeIDBit|
			(fp->resource ? kFPRsrcForkLenBit : kFPDataForkLenBit),
				0,fp)) {
		case kFPAccessDenied:
			ret=EACCES;
			goto error;
		case kFPObjectNotFound:
			ret=ENOENT;
			goto error;
		case kFPNoErr:
			break;
		case kFPBitmapErr:
		case kFPMiscErr:
		case kFPParamErr:
		default:
			ret=EIO;
			goto error;
		}

		if ((fp->resource ? (fp->resourcesize>=(AFP_MAX_AFP2_FILESIZE-1)) :
		( fp->size>=AFP_MAX_AFP2_FILESIZE-1))) {
	/* According to p.30, if the server doesn't support >4GB files
	   and the file being opened is >4GB, then resourcesize or size
	   will return 4GB.  How can it return 4GB in 32 its?  I 
	   suspect it actually returns 4GB-1.
	*/
			ret=EOVERFLOW;
			goto error;
		}
	}


try_again:
	dsi_ret=afp_openfork(volume,fp->resource?1:0,fp->did,
		aflags,fp->basename,fp);

	switch (dsi_ret) {
	case kFPAccessDenied:
		ret=EACCES;
		goto error;
	case kFPObjectNotFound:
		if ((flags & O_CREAT) && 
			(ml_creat(volume,path,0644)==0)) {
/* FIXME 0644 is just made up */
				goto try_again;
		} else {
			ret=ENOENT;
			goto error;
		}
	case kFPObjectLocked:
		ret=EROFS;
		goto error;
	case kFPObjectTypeErr:
		ret=EISDIR;
		goto error;
	case kFPParamErr:
		ret=EACCES;
		goto error;
	case kFPTooManyFilesOpen:
		ret=EMFILE;
		goto error;
	case kFPVolLocked:
	case kFPDenyConflict:
	case kFPMiscErr:
	case kFPBitmapErr:
	case -1:
		ret=EFAULT;
		goto error;
	case 0:
		ret=0;
		break;
	default:
		ret=EFAULT;
		goto error;
	}

	add_opened_fork(volume, fp);

	if ((flags & O_TRUNC) && (!create_file)) {

		/* This is the case where we want to truncate the 
		   the file and it already exists. */
		if ((ret=ll_zero_file(volume,fp->forkid,fp->resource)))
			goto error;
	}


out:
	return 0;

error:
	return -ret;
}


int ll_read(struct afp_volume * volume, 
	char *buf, size_t size, off_t offset,
	struct afp_file_info *fp, int * eof)
{
	int bytesleft=size;
	int totalsize=0;
	int ret=0;
	int rc;
	unsigned int bufsize=min(volume->server->rx_quantum,size);
	struct afp_rx_buffer buffer;
	size_t amount_copied;

	*eof=0;

	buffer.data = buf;
	buffer.maxsize=bufsize;
	buffer.size=0;
	/* Lock the range */
	if (ll_handle_locking(volume, fp->forkid,offset,size)) {
		/* There was an irrecoverable error when locking */
		ret=EBUSY;
		goto error;
	}

	if (volume->server->using_version->av_number < 30)
		rc=afp_read(volume, fp->forkid,offset,size,&buffer);
	else
		rc=afp_readext(volume, fp->forkid,offset,size,&buffer);

	if (ll_handle_unlocking(volume, fp->forkid,offset,size)) {
		/* Somehow, we couldn't unlock the range. */
		ret=EIO;
		goto error;
	}
	switch(rc) {
	case kFPAccessDenied:
		ret=EACCES;
		goto error;
	case kFPLockErr:
		ret=EBUSY;
		goto error;
	case kFPMiscErr:
	case kFPParamErr:
		ret=EIO;
		goto error;
	case kFPEOFErr:
		*eof=1;
		break;
	case kFPNoErr:
		break;
	}

	bytesleft-=buffer.size;
	totalsize+=buffer.size;
	return totalsize;
error:
	return -ret;

}




int ll_readdir(struct afp_volume * volume, const char *path, 
	struct afp_file_info **fb, int resource)
{
	struct afp_file_info * p, * filebase=NULL, *base, *last;
	unsigned short reqcount=20;  /* Get them in batches of 20 */
	unsigned long startindex=1;
	int rc=0, ret=0, exit=0;
	unsigned int filebitmap, dirbitmap;
	char basename[AFP_MAX_PATH];
	char converted_name[AFP_MAX_PATH];
	unsigned int dirid;

	if (invalid_filename(volume->server,path)) 
		return -ENAMETOOLONG;

	if (get_dirid(volume, path, basename, &dirid)<0)
		return -ENOENT;

	/* We need to handle length bits differently for AFP < 3.0 */

	filebitmap=kFPAttributeBit | kFPParentDirIDBit |
		kFPCreateDateBit | kFPModDateBit |
		kFPBackupDateBit|
		kFPNodeIDBit;
	dirbitmap=kFPAttributeBit | kFPParentDirIDBit |
		kFPCreateDateBit | kFPModDateBit |
		kFPBackupDateBit|
		kFPNodeIDBit | kFPOffspringCountBit|
		kFPOwnerIDBit|kFPGroupIDBit;
	if (volume->extra_flags & VOLUME_EXTRA_FLAGS_VOL_SUPPORTS_UNIX) {
		dirbitmap|=kFPUnixPrivsBit;
		filebitmap|=kFPUnixPrivsBit;
	}

	if (volume->attributes & kSupportsUTF8Names ) {
		dirbitmap|=kFPUTF8NameBit;
		filebitmap|=kFPUTF8NameBit;
	} else {
		dirbitmap|=kFPLongNameBit| kFPShortNameBit;
		filebitmap|=kFPLongNameBit| kFPShortNameBit;
	}
	if (volume->server->using_version->av_number<30) {
		filebitmap |=(resource ? kFPRsrcForkLenBit:kFPDataForkLenBit);
	} else {
		filebitmap |=(resource ? kFPRsrcForkLenBit:kFPExtDataForkLenBit);
	}

	while (!exit) {

/* FIXME: check AFP version */
		/* this function will allocate and generate a linked list 
		   of files */
		if (volume->server->using_version->av_number<30) {
			rc = afp_enumerate(volume,dirid,
				filebitmap, dirbitmap,reqcount,
				startindex,basename,&base);
		} else {
			rc = afp_enumerateext2(volume,dirid,
				filebitmap, dirbitmap,reqcount,
				startindex,basename,&base);
		}

		switch(rc) {
		case -1:
			ret=EIO;
			goto error;
		case 0:
		case kFPObjectNotFound:
			if (filebase==NULL) filebase=base;
			else last->next=base;
			for (p=base; p; p=p->next) {
				startindex++;
				last=p;
			}
			if (rc==kFPObjectNotFound) exit=1;
			break;
		case kFPAccessDenied:
			ret=EACCES;
			goto error;
		case kFPDirNotFound:
			ret=ENOENT;
			exit++;
			break;
		case kFPBitmapErr:
		case kFPMiscErr:
		case kFPObjectTypeErr:
		case kFPParamErr:
		case kFPCallNotSupported:
			ret=EIO;
			goto error;
		}
	}

	for (p=filebase; p; p=p->next) {
		/* Convert all the names back to precomposed */
		convert_path_to_unix(
			volume->server->path_encoding, 
			converted_name,p->name, AFP_MAX_PATH);
		startindex++;
	}

	if (volume->server->using_version->av_number<30) {
		for (p=filebase; p; p=p->next) {
			set_nonunix_perms(&p->unixprivs.permissions, p);
		}
	}

	*fb=filebase;

	return 0;
error:
	return -ret;

}


int ll_getattr(struct afp_volume * volume, const char *path, struct stat *stbuf,
	int resource)
{
	struct afp_file_info fp;
	unsigned int dirid;
	int rc;
	unsigned int filebitmap, dirbitmap;
	char basename[AFP_MAX_PATH];
	unsigned int creation_date;
	unsigned int modification_date;

	memset(stbuf, 0, sizeof(struct stat));

	if ((volume->server) && 
		(invalid_filename(volume->server,path)))
		return -ENAMETOOLONG;
 
	if (get_dirid(volume, path, basename, &dirid)<0) {
		return -ENOENT;
	}

	dirbitmap=kFPAttributeBit 
		| kFPCreateDateBit | kFPModDateBit|
		kFPNodeIDBit |
		kFPParentDirIDBit | kFPOffspringCountBit;
	filebitmap=kFPAttributeBit | 
		kFPCreateDateBit | kFPModDateBit |
		kFPNodeIDBit |
		kFPFinderInfoBit |
		kFPParentDirIDBit;

	if (volume->server->using_version->av_number < 30) {
		if (path[0]=='/' && path[1]=='\0') {
			/* This will sound odd, but when referring to /, AFP 2.x
			   clients check on a 'file' with the volume name. */
			snprintf(basename,AFP_MAX_PATH,"%s",
				volume->volume_name);
			dirid=1;
		}
		filebitmap |=(resource ? kFPRsrcForkLenBit:kFPDataForkLenBit);

	} else {
		filebitmap |=(resource ? kFPExtRsrcForkLenBit:kFPExtDataForkLenBit);
	}

	if (volume->extra_flags & VOLUME_EXTRA_FLAGS_VOL_SUPPORTS_UNIX) {
		dirbitmap|= kFPUnixPrivsBit;
		filebitmap|= kFPUnixPrivsBit;
	} else {
		dirbitmap|=kFPOwnerIDBit | kFPGroupIDBit;
	}

	rc=afp_getfiledirparms(volume,dirid,filebitmap,dirbitmap,
		(char *) basename,&fp);

	switch(rc) {
		
	case kFPAccessDenied:
		return -EACCES;
	case kFPObjectNotFound:
		return -ENOENT;
	case kFPNoErr:
		break;
	case kFPBitmapErr:
	case kFPMiscErr:
	case kFPParamErr:
	default:
		return -EIO;
	}

	if (volume->server->using_version->av_number>=30)
		stbuf->st_mode |= fp.unixprivs.permissions;
	else
		set_nonunix_perms(stbuf,&fp);

	stbuf->st_uid=fp.unixprivs.uid;
	stbuf->st_gid=fp.unixprivs.gid;

	if (translate_uidgid_to_client(volume,
		&stbuf->st_uid,&stbuf->st_gid)) {
		return -EIO;
	}
	if (stbuf->st_mode & S_IFDIR) {
		stbuf->st_nlink = fp.offspring +2;  
		stbuf->st_size = (fp.offspring *34) + 24;  
			/* This slight voodoo was taken from Mac OS X 10.2 */
	} else {
		stbuf->st_nlink = 1;
		stbuf->st_size = (resource ? fp.resourcesize : fp.size);
		stbuf->st_blksize = 4096;
		stbuf->st_blocks = (stbuf->st_size) / 4096;
	}

        if ((volume->server->using_version->av_number<30) && 
		(stbuf->st_mode & S_IFDIR)) {
		/* AFP 2.x doesn't give ctime and mtime for directories*/
		creation_date=volume->server->connect_time;
		modification_date=volume->server->connect_time;
	} else {
		creation_date=fp.creation_date;
		modification_date=fp.modification_date;
	}

#ifdef __linux__
	stbuf->st_ctim.tv_sec=creation_date;
	stbuf->st_mtim.tv_sec=modification_date;
#else
	stbuf->st_ctime=creation_date;
	stbuf->st_mtime=modification_date;
#endif

	return 0;

}


int ll_write(struct afp_volume * volume,
		const char *data, size_t size, off_t offset,
                  struct afp_file_info * fp, size_t * totalwritten)
 {

	int ret,err=0;
	uint64_t sizetowrite, ignored;
	uint32_t ignored32;
	unsigned char flags = 0;
	unsigned int max_packet_size=volume->server->tx_quantum;
	off_t o=0;
	*totalwritten=0;

	if (!fp) return -EBADF;

	/* Get a lock */
	if (ll_handle_locking(volume, fp->forkid,offset,size)) {
		/* There was an irrecoverable error when locking */
		ret=EBUSY;
		goto error;
	}

	ret=0;
	while (*totalwritten < size) {
		sizetowrite=max_packet_size;
		if ((size-*totalwritten)<max_packet_size)
			sizetowrite=size-*totalwritten;

		if (volume->server->using_version->av_number < 30) 
			ret=afp_write(volume, fp->forkid,
				offset+o,sizetowrite,
				(char *) data+o,&ignored32);
		else 
			ret=afp_writeext(volume, fp->forkid,
				offset+o,sizetowrite,
				(char *) data+o,&ignored);
		ret=0;
		*totalwritten+=sizetowrite;
		switch(ret) {
		case kFPAccessDenied:
			err=EACCES;
			goto error;
		case kFPDiskFull:
			err=ENOSPC;
			goto error;
		case kFPLockErr:
		case kFPMiscErr:
		case kFPParamErr:
			err=EINVAL;
			goto error;
		}
		o+=sizetowrite;
	}
	if (ll_handle_unlocking(volume, fp->forkid,offset,size)) {
		/* Somehow, we couldn't unlock the range. */
		ret=EIO;
		goto error;
	}
	return 0;

error:
	return -err;


}



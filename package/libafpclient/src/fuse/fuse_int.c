

/*

    fuse.c, FUSE interfaces for afpfs-ng

    Copyright (C) 2006 Alex deVries <alexthepuffin@gmail.com>

    Heavily modifed from the example code provided by:
    Copyright (C) 2001-2005  Miklos Szeredi <miklos@szeredi.hu>

    This program can be distributed under the terms of the GNU GPL.
    See the file COPYING.
*/

#define HAVE_ARCH_STRUCT_FLOCK

#define FUSE_USE_VERSION 25


#include "afp.h"

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#ifdef __linux__
#include <asm/fcntl.h>
#else
#include <fcntl.h>
#endif

#include <utime.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>
#include <sys/types.h>
#include <pwd.h>
#include <stdarg.h>

#include "dsi.h"
#include "afp_protocol.h"
#include "codepage.h"
#include "midlevel.h"
#include "fuse_error.h"

/* Uncomment the following line to enable full debugging: */
/*
#define LOG_FUSE_EVENTS 
*/

void log_fuse_event(enum loglevels loglevel, int logtype,
                    char *format, ...) {
#ifdef LOG_FUSE_EVENTS
		va_list ap;

		va_start(ap,format);
		vprintf(format,ap);
		va_end(ap);
#endif

}


static int fuse_readlink(const char * path, char *buf, size_t size)
{
	int ret;
	struct afp_volume * volume=
		(struct afp_volume *)
		((struct fuse_context *)(fuse_get_context()))->private_data;

	log_fuse_event(AFPFSD,LOG_DEBUG,"*** readlink of %s\n",path);

	ret=ml_readlink(volume,path,buf,size);

	if (ret==-EFAULT) {
		log_for_client(NULL,AFPFSD,LOG_WARNING,
		"Got some sort of internal error in afp_open for readlink\n");
	}


	return ret;

}

static int fuse_rmdir(const char *path)
{
	int ret;
	struct afp_volume * volume=
		(struct afp_volume *)
		((struct fuse_context *)(fuse_get_context()))->private_data;

	log_fuse_event(AFPFSD,LOG_DEBUG,"*** rmdir of %s\n",path);

	ret=ml_rmdir(volume,path);

	return ret;
}

static int fuse_unlink(const char *path)
{
	int ret;
	struct afp_volume * volume=
		(struct afp_volume *)
		((struct fuse_context *)(fuse_get_context()))->private_data;

	log_fuse_event(AFPFSD,LOG_DEBUG,"*** unlink of %s\n",path);

	ret=ml_unlink(volume,path);

	return ret;
}


static int fuse_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                         off_t offset, struct fuse_file_info *fi)
{
	(void) offset;
	(void) fi;
	struct afp_file_info * filebase = NULL, * p;
	int ret;
	struct afp_volume * volume=
		(struct afp_volume *)
		((struct fuse_context *)(fuse_get_context()))->private_data;

	log_fuse_event(AFPFSD,LOG_DEBUG,"*** readdir of %s\n",path);

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);

	ret=ml_readdir(volume,path,&filebase);

	if (ret) goto error;

	for (p=filebase;p;p=p->next) {
		filler(buf,p->name,NULL,0);
	}

	afp_ml_filebase_free(&filebase);

    return 0;

error:
	return ret;
}

static int fuse_mknod(const char *path, mode_t mode, dev_t dev)
{
	int ret=0;
	struct fuse_context * context = fuse_get_context();
	struct afp_volume * volume=
		(struct afp_volume *) context->private_data;

	log_fuse_event(AFPFSD,LOG_DEBUG,"*** mknod of %s\n",path);

	ret=ml_creat(volume,path,mode);

	return ret;
}


static int fuse_release(const char * path, struct fuse_file_info * fi)
{

	struct afp_file_info * fp = (void *) fi->fh;
	int ret=0;
	struct afp_volume * volume=
		(struct afp_volume *)
		((struct fuse_context *)(fuse_get_context()))->private_data;

	log_fuse_event(AFPFSD,LOG_DEBUG,"*** release of %s\n",path);

	ret=ml_close(volume,path,fp);

	if (ret<0) goto error;

	return ret;
error:
	free((void *) fi->fh);
	return ret;
}

static int fuse_open(const char *path, struct fuse_file_info *fi)
{

	struct afp_file_info * fp ;
	int ret;
	struct afp_volume * volume=
		(struct afp_volume *)
		((struct fuse_context *)(fuse_get_context()))->private_data;
	unsigned char flags = AFP_OPENFORK_ALLOWREAD;

	log_fuse_event(AFPFSD,LOG_DEBUG,
		"*** Opening path %s\n",path);

	ret = ml_open(volume,path,flags,&fp);

	if (ret==0) 
		fi->fh=(void *) fp;

	return ret;
}


static int fuse_write(const char * path, const char *data, 
	size_t size, off_t offset,
	struct fuse_file_info *fi)
{

	struct afp_file_info *fp = (struct afp_file_info *) fi->fh;
	int ret;
	struct fuse_context * context = fuse_get_context();
	struct afp_volume * volume=(void *) context->private_data;

	log_fuse_event(AFPFSD,LOG_DEBUG,
		"*** write of from %llu for %llu\n",
		(unsigned long long) offset,(unsigned long long) size);

	ret=ml_write(volume,path,data,size,offset,fp,
		context->uid, context->gid);


	return ret;

}


static int fuse_mkdir(const char * path, mode_t mode) 
{
	int ret;
	struct afp_volume * volume=
		(struct afp_volume *)
		((struct fuse_context *)(fuse_get_context()))->private_data;

	log_fuse_event(AFPFSD,LOG_DEBUG,"*** mkdir of %s\n",path);

	ret=ml_mkdir(volume,path,mode);

	return ret;

}
static int fuse_read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi)
{
	struct afp_file_info * fp;	
	int ret=0;
	struct afp_volume * volume=
		(struct afp_volume *)
		((struct fuse_context *)(fuse_get_context()))->private_data;
	int eof;
	size_t amount_read=0;

	if (!fi || !fi->fh) 
		return -EBADF;
	fp=(void *) fi->fh;

	while (1) {
		ret = ml_read(volume,path,buf+amount_read,size,offset,fp,&eof);
		if (ret<0) goto error;
		amount_read+=ret;
		if (eof) goto out;
		size-=ret;
		if (size==0) goto out;
		offset+=ret;
	}
out:
	return amount_read;

error:
	return ret;
}

static int fuse_chown(const char * path, uid_t uid, gid_t gid) 
{
	int ret;
	struct afp_volume * volume=
		(struct afp_volume *)
		((struct fuse_context *)(fuse_get_context()))->private_data;

	log_fuse_event(AFPFSD,LOG_DEBUG,"** chown\n");

	ret=ml_chown(volume,path,uid,gid);

	if (ret==-ENOSYS) {
		log_for_client(NULL,AFPFSD,LOG_WARNING,"chown unsupported\n");
	}

	return ret;
}

static int fuse_truncate(const char * path, off_t offset)
{
	int ret=0;
	struct afp_volume * volume=
		(struct afp_volume *)
		((struct fuse_context *)(fuse_get_context()))->private_data;

	log_fuse_event(AFPFSD,LOG_DEBUG,
		"** truncate\n");

	ret=ml_truncate(volume,path,offset);

	return ret;
}


static int fuse_chmod(const char * path, mode_t mode) 
{
	struct afp_volume * volume=
		(struct afp_volume *)
		((struct fuse_context *)(fuse_get_context()))->private_data;
	int ret;

	log_fuse_event(AFPFSD,LOG_DEBUG,
		"** chmod %s\n",path);
	ret=ml_chmod(volume,path,mode);

	switch (ret) {

	case -EPERM:
		log_for_client(NULL,AFPFSD,LOG_DEBUG,
			"You're not the owner of this file.\n");
		break;

	case -ENOSYS:
                log_for_client(NULL,AFPFSD,LOG_WARNING,"chmod unsupported or this mode is not possible with this server\n");
		break;
	case -EFAULT:
	log_for_client(NULL,AFPFSD,LOG_ERR,
	"You're mounting from a netatalk server, and I was trying to change "
	"permissions but you're setting some mode bits that aren't supported " 
	"by the server.  This is because this netatalk server is broken. \n"
	"This is because :\n"
	" - you haven't set -options=unix_priv in AppleVolumes.default\n"
	" - you haven't applied a patch which fixes chmod() to netatalk, or are using an \n"
	"   old version. See afpfs-ng docs.\n"
	" - maybe both\n"
	"It sucks, but I'm marking this volume as broken for 'extended' chmod modes.\n"
	"Allowed bits are: %o\n", AFP_CHMOD_ALLOWED_BITS_22);

		ret=0; /* Return anyway */
		break;
	}


	return ret;
}

static int fuse_utime(const char * path, struct utimbuf * timebuf)
{

	int ret=0;
	struct afp_volume * volume=
		(struct afp_volume *)
		((struct fuse_context *)(fuse_get_context()))->private_data;

	log_fuse_event(AFPFSD,LOG_DEBUG,
		"** utime\n");


	ret=ml_utime(volume,path,timebuf);

	return ret;
}

static void afp_destroy(void * ignore) 
{
	struct afp_volume * volume=
		(struct afp_volume *)
		((struct fuse_context *)(fuse_get_context()))->private_data;

	if (volume->mounted==AFP_VOLUME_UNMOUNTED) {
		log_for_client(NULL,AFPFSD,LOG_WARNING,"Skipping unmounting of the volume %s\n",volume->volume_name_printable);
		return;
	}
	if ((!volume) || (!volume->server)) return;

	/* We're just ignoring the results since there's nothing we could
	   do with them anyway.  */
	afp_unmount_volume(volume);

}

static int fuse_symlink(const char * path1, const char * path2) 
{

	struct afp_volume * volume=
		(struct afp_volume *)
		((struct fuse_context *)(fuse_get_context()))->private_data;
	int ret;

	ret=ml_symlink(volume,path1,path2);
	if ((ret==-EFAULT) || (ret==-ENOSYS)) {
		log_for_client(NULL,AFPFSD,LOG_WARNING,
		"Got some sort of internal error in when creating symlink\n");
	}

	return ret;
};

static int fuse_rename(const char * path_from, const char * path_to) 
{
	int ret;
	struct afp_volume * volume=
		(struct afp_volume *)
		((struct fuse_context *)(fuse_get_context()))->private_data;

	ret=ml_rename(volume,path_from, path_to);

	return ret;

}

static int fuse_statfs(const char *path, struct statvfs *stat)
{
	struct afp_volume * volume=
		(struct afp_volume *)
		((struct fuse_context *)(fuse_get_context()))->private_data;
	int ret;

	ret=ml_statfs(volume,path,stat);

	return ret;
}


static int fuse_getattr(const char *path, struct stat *stbuf)
{
	char * c;
	struct afp_volume * volume=
		(struct afp_volume *)
		((struct fuse_context *)(fuse_get_context()))->private_data;
	int ret;

	log_fuse_event(AFPFSD,LOG_DEBUG,"*** getattr of \"%s\"\n",path);

	/* Oddly, we sometimes get <dir1>/<dir2>/(null) for the path */

	if (!path) return -EIO;

	if ((c=strstr(path,"(null)"))) {
		/* We should fix this to make sure it is at the end */
		if (c>path) *(c-1)='\0';
	}

	ret=ml_getattr(volume,path,stbuf);

	return ret;
}


static struct afp_volume * global_volume;

#if FUSE_USE_VERSION < 26
static void *afp_init(void) {
#else 
static void *afp_init(void * o) {
#endif
	struct afp_volume * vol = global_volume;

	vol->priv=(void *)((struct fuse_context *)(fuse_get_context()))->fuse;
	/* Trigger the daemon that we've started */
	if (vol->priv) vol->mounted=1;
	pthread_cond_signal(&vol->startup_condition_cond);
	return (void *) vol;
}



static struct fuse_operations afp_oper = {
	.getattr	=fuse_getattr,
	.open	= fuse_open,
	.read	= fuse_read,
	.readdir	= fuse_readdir,
	.mkdir      = fuse_mkdir,
	.readlink = fuse_readlink,
	.rmdir	= fuse_rmdir,
	.unlink = fuse_unlink,
	.mknod  = fuse_mknod,
	.write = fuse_write,
	.release= fuse_release,
	.chmod=fuse_chmod,
	.symlink=fuse_symlink,
	.chown=fuse_chown,
	.truncate=fuse_truncate,
	.rename=fuse_rename,
	.utime=fuse_utime,
	.destroy=afp_destroy,
	.init=afp_init,
	.statfs=fuse_statfs,
};


int afp_register_fuse(int fuseargc, char *fuseargv[],struct afp_volume * vol)
{
	int ret;
	global_volume=vol;

	fuse_capture_stderr_start();

#if FUSE_USE_VERSION < 26
	ret=fuse_main(fuseargc, fuseargv, &afp_oper);
#else
	ret=fuse_main(fuseargc, fuseargv, &afp_oper,(void *) vol);
#endif

	return ret;
}



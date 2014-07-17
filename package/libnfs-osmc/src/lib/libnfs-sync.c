/*
   Copyright (C) 2010 by Ronnie Sahlberg <ronniesahlberg@gmail.com>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation; either version 2.1 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program; if not, see <http://www.gnu.org/licenses/>.
*/
/*
 * High level api to nfs filesystems
 */
#ifdef WIN32
#include "win32_compat.h"
#define DllExport
#else
#include <strings.h>
#include <unistd.h>
#include <sys/statvfs.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <net/if.h>
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#ifdef HAVE_SYS_SOCKIO_H
#include <sys/sockio.h>
#endif

#include "libnfs.h"
#include "libnfs-raw.h"
#include "libnfs-raw-mount.h"
#include "libnfs-raw-nfs.h"
#include "libnfs-private.h"

struct sync_cb_data {
       int is_finished;
       int status;
       uint64_t offset;
       void *return_data;
       int return_int;
};


static void wait_for_reply(struct rpc_context *rpc, struct sync_cb_data *cb_data)
{
	struct pollfd pfd;

	while (!cb_data->is_finished) {

		pfd.fd = rpc_get_fd(rpc);
		pfd.events = rpc_which_events(rpc);
		if (poll(&pfd, 1, -1) < 0) {
			rpc_set_error(rpc, "Poll failed");
			cb_data->status = -EIO;
			break;
		}
		if (rpc_service(rpc, pfd.revents) < 0) {
			rpc_set_error(rpc, "rpc_service failed");
			cb_data->status = -EIO;
			break;
		}
		if (rpc_get_fd(rpc) == -1) {
			rpc_set_error(rpc, "Socket closed\n");
			break;
		}
	}
}

static void wait_for_nfs_reply(struct nfs_context *nfs, struct sync_cb_data *cb_data)
{
	struct pollfd pfd;

	while (!cb_data->is_finished) {

		pfd.fd = nfs_get_fd(nfs);
		pfd.events = nfs_which_events(nfs);
		if (poll(&pfd, 1, -1) < 0) {
			nfs_set_error(nfs, "Poll failed");
			cb_data->status = -EIO;
			break;
		}
		if (nfs_service(nfs, pfd.revents) < 0) {
			nfs_set_error(nfs, "nfs_service failed");
			cb_data->status = -EIO;
			break;
		}
	}
}






/*
 * connect to the server and mount the export
 */
static void mount_cb(int status, struct nfs_context *nfs, void *data, void *private_data)
{
	struct sync_cb_data *cb_data = private_data;

	cb_data->is_finished = 1;
	cb_data->status = status;

	if (status < 0) {
		nfs_set_error(nfs, "mount/mnt call failed with \"%s\"", (char *)data);
		return;
	}
}

int nfs_mount(struct nfs_context *nfs, const char *server, const char *export)
{
	struct sync_cb_data cb_data;
	struct rpc_context *rpc = nfs_get_rpc_context(nfs);

	cb_data.is_finished = 0;

	if (nfs_mount_async(nfs, server, export, mount_cb, &cb_data) != 0) {
		nfs_set_error(nfs, "nfs_mount_async failed");
		return -1;
	}

	wait_for_nfs_reply(nfs, &cb_data);

	/* Dont want any more callbacks even if the socket is closed */
	rpc->connect_cb = NULL;

	return cb_data.status;
}


/*
 * stat()
 */
static void stat_cb(int status, struct nfs_context *nfs, void *data, void *private_data)
{
	struct sync_cb_data *cb_data = private_data;

	cb_data->is_finished = 1;
	cb_data->status = status;

	if (status < 0) {
		nfs_set_error(nfs, "stat call failed with \"%s\"", (char *)data);
		return;
	}

	memcpy(cb_data->return_data, data, sizeof(struct stat));
}

int nfs_stat(struct nfs_context *nfs, const char *path, struct stat *st)
{
	struct sync_cb_data cb_data;

	cb_data.is_finished = 0;
	cb_data.return_data = st;

	if (nfs_stat_async(nfs, path, stat_cb, &cb_data) != 0) {
		nfs_set_error(nfs, "nfs_stat_async failed");
		return -1;
	}

	wait_for_nfs_reply(nfs, &cb_data);

	return cb_data.status;
}




/*
 * open()
 */
static void open_cb(int status, struct nfs_context *nfs, void *data, void *private_data)
{
	struct sync_cb_data *cb_data = private_data;
	struct nfsfh *fh, **nfsfh;

	cb_data->is_finished = 1;
	cb_data->status = status;

	if (status < 0) {
		nfs_set_error(nfs, "open call failed with \"%s\"", (char *)data);
		return;
	}

	fh    = data;
	nfsfh = cb_data->return_data;
	*nfsfh = fh;
}

int nfs_open(struct nfs_context *nfs, const char *path, int mode, struct nfsfh **nfsfh)
{
	struct sync_cb_data cb_data;

	cb_data.is_finished = 0;
	cb_data.return_data = nfsfh;

	if (nfs_open_async(nfs, path, mode, open_cb, &cb_data) != 0) {
		nfs_set_error(nfs, "nfs_open_async failed");
		return -1;
	}

	wait_for_nfs_reply(nfs, &cb_data);

	return cb_data.status;
}




/*
 * pread()
 */
static void pread_cb(int status, struct nfs_context *nfs, void *data, void *private_data)
{
	struct sync_cb_data *cb_data = private_data;
	char *buffer;
	cb_data->is_finished = 1;
	cb_data->status = status;

	if (status < 0) {
		nfs_set_error(nfs, "pread call failed with \"%s\"", (char *)data);
		return;
	}

	buffer = cb_data->return_data;
	memcpy(buffer, (char *)data, status);
}

int nfs_pread(struct nfs_context *nfs, struct nfsfh *nfsfh, uint64_t offset, uint64_t count, char *buffer)
{
	struct sync_cb_data cb_data;

	cb_data.is_finished = 0;
	cb_data.return_data = buffer;

	if (nfs_pread_async(nfs, nfsfh, offset, count, pread_cb, &cb_data) != 0) {
		nfs_set_error(nfs, "nfs_pread_async failed");
		return -1;
	}

	wait_for_nfs_reply(nfs, &cb_data);

	return cb_data.status;
}

/*
 * read()
 */
int nfs_read(struct nfs_context *nfs, struct nfsfh *nfsfh, uint64_t count, char *buffer)
{
	return nfs_pread(nfs, nfsfh, nfs_get_current_offset(nfsfh), count, buffer);
}

/*
 * close()
 */
static void close_cb(int status, struct nfs_context *nfs, void *data, void *private_data)
{
	struct sync_cb_data *cb_data = private_data;
	cb_data->is_finished = 1;
	cb_data->status = status;

	if (status < 0) {
		nfs_set_error(nfs, "close call failed with \"%s\"", (char *)data);
		return;
	}
}

int nfs_close(struct nfs_context *nfs, struct nfsfh *nfsfh)
{
	struct sync_cb_data cb_data;

	cb_data.is_finished = 0;

	if (nfs_close_async(nfs, nfsfh, close_cb, &cb_data) != 0) {
		nfs_set_error(nfs, "nfs_close_async failed");
		return -1;
	}

	wait_for_nfs_reply(nfs, &cb_data);

	return cb_data.status;
}




/*
 * fstat()
 */
int nfs_fstat(struct nfs_context *nfs, struct nfsfh *nfsfh, struct stat *st)
{
	struct sync_cb_data cb_data;

	cb_data.is_finished = 0;
	cb_data.return_data = st;

	if (nfs_fstat_async(nfs, nfsfh, stat_cb, &cb_data) != 0) {
		nfs_set_error(nfs, "nfs_fstat_async failed");
		return -1;
	}

	wait_for_nfs_reply(nfs, &cb_data);

	return cb_data.status;
}


/*
 * pwrite()
 */
static void pwrite_cb(int status, struct nfs_context *nfs, void *data, void *private_data)
{
	struct sync_cb_data *cb_data = private_data;
	cb_data->is_finished = 1;
	cb_data->status = status;

	if (status < 0) {
		nfs_set_error(nfs, "pwrite call failed with \"%s\"", (char *)data);
		return;
	}
}

int nfs_pwrite(struct nfs_context *nfs, struct nfsfh *nfsfh, uint64_t offset, uint64_t count, char *buf)
{
	struct sync_cb_data cb_data;

	cb_data.is_finished = 0;

	if (nfs_pwrite_async(nfs, nfsfh, offset, count, buf, pwrite_cb, &cb_data) != 0) {
		nfs_set_error(nfs, "nfs_pwrite_async failed");
		return -1;
	}

	wait_for_nfs_reply(nfs, &cb_data);

	return cb_data.status;
}

/*
 * write()
 */
int nfs_write(struct nfs_context *nfs, struct nfsfh *nfsfh, uint64_t count, char *buf)
{
	return nfs_pwrite(nfs, nfsfh, nfs_get_current_offset(nfsfh), count, buf);
}


/*
 * fsync()
 */
static void fsync_cb(int status, struct nfs_context *nfs, void *data, void *private_data)
{
	struct sync_cb_data *cb_data = private_data;
	cb_data->is_finished = 1;
	cb_data->status = status;

	if (status < 0) {
		nfs_set_error(nfs, "fsync call failed with \"%s\"", (char *)data);
		return;
	}
}

int nfs_fsync(struct nfs_context *nfs, struct nfsfh *nfsfh)
{
	struct sync_cb_data cb_data;

	cb_data.is_finished = 0;

	if (nfs_fsync_async(nfs, nfsfh, fsync_cb, &cb_data) != 0) {
		nfs_set_error(nfs, "nfs_fsync_async failed");
		return -1;
	}

	wait_for_nfs_reply(nfs, &cb_data);

	return cb_data.status;
}




/*
 * ftruncate()
 */
static void ftruncate_cb(int status, struct nfs_context *nfs, void *data, void *private_data)
{
	struct sync_cb_data *cb_data = private_data;
	cb_data->is_finished = 1;
	cb_data->status = status;

	if (status < 0) {
		nfs_set_error(nfs, "ftruncate call failed with \"%s\"", (char *)data);
		return;
	}
}

int nfs_ftruncate(struct nfs_context *nfs, struct nfsfh *nfsfh, uint64_t length)
{
	struct sync_cb_data cb_data;

	cb_data.is_finished = 0;

	if (nfs_ftruncate_async(nfs, nfsfh, length, ftruncate_cb, &cb_data) != 0) {
		nfs_set_error(nfs, "nfs_ftruncate_async failed");
		return -1;
	}

	wait_for_nfs_reply(nfs, &cb_data);

	return cb_data.status;
}



/*
 * truncate()
 */
static void truncate_cb(int status, struct nfs_context *nfs, void *data, void *private_data)
{
	struct sync_cb_data *cb_data = private_data;
	cb_data->is_finished = 1;
	cb_data->status = status;

	if (status < 0) {
		nfs_set_error(nfs, "truncate call failed with \"%s\"", (char *)data);
		return;
	}
}

int nfs_truncate(struct nfs_context *nfs, const char *path, uint64_t length)
{
	struct sync_cb_data cb_data;

	cb_data.is_finished = 0;

	if (nfs_truncate_async(nfs, path, length, truncate_cb, &cb_data) != 0) {
		nfs_set_error(nfs, "nfs_ftruncate_async failed");
		return -1;
	}

	wait_for_nfs_reply(nfs, &cb_data);

	return cb_data.status;
}





/*
 * mkdir()
 */
static void mkdir_cb(int status, struct nfs_context *nfs, void *data, void *private_data)
{
	struct sync_cb_data *cb_data = private_data;
	cb_data->is_finished = 1;
	cb_data->status = status;

	if (status < 0) {
		nfs_set_error(nfs, "mkdir call failed with \"%s\"", (char *)data);
		return;
	}
}

int nfs_mkdir(struct nfs_context *nfs, const char *path)
{
	struct sync_cb_data cb_data;

	cb_data.is_finished = 0;

	if (nfs_mkdir_async(nfs, path, mkdir_cb, &cb_data) != 0) {
		nfs_set_error(nfs, "nfs_mkdir_async failed");
		return -1;
	}

	wait_for_nfs_reply(nfs, &cb_data);

	return cb_data.status;
}





/*
 * rmdir()
 */
static void rmdir_cb(int status, struct nfs_context *nfs, void *data, void *private_data)
{
	struct sync_cb_data *cb_data = private_data;
	cb_data->is_finished = 1;
	cb_data->status = status;

	if (status < 0) {
		nfs_set_error(nfs, "rmdir call failed with \"%s\"", (char *)data);
		return;
	}
}

int nfs_rmdir(struct nfs_context *nfs, const char *path)
{
	struct sync_cb_data cb_data;

	cb_data.is_finished = 0;

	if (nfs_rmdir_async(nfs, path, rmdir_cb, &cb_data) != 0) {
		nfs_set_error(nfs, "nfs_rmdir_async failed");
		return -1;
	}

	wait_for_nfs_reply(nfs, &cb_data);

	return cb_data.status;
}



/*
 * creat()
 */
static void creat_cb(int status, struct nfs_context *nfs, void *data, void *private_data)
{
	struct sync_cb_data *cb_data = private_data;
	struct nfsfh *fh, **nfsfh;

	cb_data->is_finished = 1;
	cb_data->status = status;

	if (status < 0) {
		nfs_set_error(nfs, "creat call failed with \"%s\"", (char *)data);
		return;
	}

	fh    = data;
	nfsfh = cb_data->return_data;
	*nfsfh = fh;
}

int nfs_creat(struct nfs_context *nfs, const char *path, int mode, struct nfsfh **nfsfh)
{
	struct sync_cb_data cb_data;

	cb_data.is_finished = 0;
	cb_data.return_data = nfsfh;

	if (nfs_creat_async(nfs, path, mode, creat_cb, &cb_data) != 0) {
		nfs_set_error(nfs, "nfs_creat_async failed");
		return -1;
	}

	wait_for_nfs_reply(nfs, &cb_data);

	return cb_data.status;
}

/*
 * mknod()
 */
static void mknod_cb(int status, struct nfs_context *nfs, void *data, void *private_data)
{
	struct sync_cb_data *cb_data = private_data;

	cb_data->is_finished = 1;
	cb_data->status = status;

	if (status < 0) {
		nfs_set_error(nfs, "mknod call failed with \"%s\"", (char *)data);
		return;
	}
}

int nfs_mknod(struct nfs_context *nfs, const char *path, int mode, int dev)
{
	struct sync_cb_data cb_data;

	cb_data.is_finished = 0;

	if (nfs_mknod_async(nfs, path, mode, dev, mknod_cb, &cb_data) != 0) {
		nfs_set_error(nfs, "nfs_creat_async failed");
		return -1;
	}

	wait_for_nfs_reply(nfs, &cb_data);

	return cb_data.status;
}


/*
 * unlink()
 */
static void unlink_cb(int status, struct nfs_context *nfs, void *data, void *private_data)
{
	struct sync_cb_data *cb_data = private_data;

	cb_data->is_finished = 1;
	cb_data->status = status;

	if (status < 0) {
		nfs_set_error(nfs, "unlink call failed with \"%s\"", (char *)data);
		return;
	}
}

int nfs_unlink(struct nfs_context *nfs, const char *path)
{
	struct sync_cb_data cb_data;

	cb_data.is_finished = 0;

	if (nfs_unlink_async(nfs, path, unlink_cb, &cb_data) != 0) {
		nfs_set_error(nfs, "nfs_unlink_async failed");
		return -1;
	}

	wait_for_nfs_reply(nfs, &cb_data);

	return cb_data.status;
}



/*
 * opendir()
 */
static void opendir_cb(int status, struct nfs_context *nfs, void *data, void *private_data)
{
	struct sync_cb_data *cb_data = private_data;
	struct nfsdir *dir, **nfsdir;

	cb_data->is_finished = 1;
	cb_data->status = status;

	if (status < 0) {
		nfs_set_error(nfs, "opendir call failed with \"%s\"", (char *)data);
		return;
	}

	dir     = data;
	nfsdir  = cb_data->return_data;
	*nfsdir = dir;
}

int nfs_opendir(struct nfs_context *nfs, const char *path, struct nfsdir **nfsdir)
{
	struct sync_cb_data cb_data;

	cb_data.is_finished = 0;
	cb_data.return_data = nfsdir;

	if (nfs_opendir_async(nfs, path, opendir_cb, &cb_data) != 0) {
		nfs_set_error(nfs, "nfs_opendir_async failed");
		return -1;
	}

	wait_for_nfs_reply(nfs, &cb_data);

	return cb_data.status;
}


/*
 * lseek()
 */
static void lseek_cb(int status, struct nfs_context *nfs, void *data, void *private_data)
{
	struct sync_cb_data *cb_data = private_data;

	cb_data->is_finished = 1;
	cb_data->status = status;

	if (status < 0) {
		nfs_set_error(nfs, "lseek call failed with \"%s\"", (char *)data);
		return;
	}

	if (cb_data->return_data != NULL) {
		memcpy(cb_data->return_data, data, sizeof(uint64_t));
	}
}

int nfs_lseek(struct nfs_context *nfs, struct nfsfh *nfsfh, uint64_t offset, int whence, uint64_t *current_offset)
{
	struct sync_cb_data cb_data;

	cb_data.is_finished = 0;
	cb_data.return_data = current_offset;

	if (nfs_lseek_async(nfs, nfsfh, offset, whence, lseek_cb, &cb_data) != 0) {
		nfs_set_error(nfs, "nfs_lseek_async failed");
		return -1;
	}

	wait_for_nfs_reply(nfs, &cb_data);

	return cb_data.status;
}



/*
 * statvfs()
 */
static void statvfs_cb(int status, struct nfs_context *nfs, void *data, void *private_data)
{
	struct sync_cb_data *cb_data = private_data;

	cb_data->is_finished = 1;
	cb_data->status = status;

	if (status < 0) {
		nfs_set_error(nfs, "statvfs call failed with \"%s\"", (char *)data);
		return;
	}

	memcpy(cb_data->return_data, data, sizeof(struct statvfs));
}

int nfs_statvfs(struct nfs_context *nfs, const char *path, struct statvfs *svfs)
{
	struct sync_cb_data cb_data;

	cb_data.is_finished = 0;
	cb_data.return_data = svfs;

	if (nfs_statvfs_async(nfs, path, statvfs_cb, &cb_data) != 0) {
		nfs_set_error(nfs, "nfs_statvfs_async failed");
		return -1;
	}

	wait_for_nfs_reply(nfs, &cb_data);

	return cb_data.status;
}





/*
 * readlink()
 */
static void readlink_cb(int status, struct nfs_context *nfs, void *data, void *private_data)
{
	struct sync_cb_data *cb_data = private_data;

	cb_data->is_finished = 1;
	cb_data->status = status;

	if (status < 0) {
		nfs_set_error(nfs, "readlink call failed with \"%s\"", (char *)data);
		return;
	}

	if (strlen(data) > (size_t)cb_data->return_int) {
		nfs_set_error(nfs, "Too small buffer for readlink");
		cb_data->status = -ENAMETOOLONG;
		return;
	}

	memcpy(cb_data->return_data, data, strlen(data)+1);
}

int nfs_readlink(struct nfs_context *nfs, const char *path, char *buf, int bufsize)
{
	struct sync_cb_data cb_data;

	cb_data.is_finished = 0;
	cb_data.return_data = buf;
	cb_data.return_int  = bufsize;

	if (nfs_readlink_async(nfs, path, readlink_cb, &cb_data) != 0) {
		nfs_set_error(nfs, "nfs_readlink_async failed");
		return -1;
	}

	wait_for_nfs_reply(nfs, &cb_data);

	return cb_data.status;
}



/*
 * chmod()
 */
static void chmod_cb(int status, struct nfs_context *nfs, void *data, void *private_data)
{
	struct sync_cb_data *cb_data = private_data;

	cb_data->is_finished = 1;
	cb_data->status = status;

	if (status < 0) {
		nfs_set_error(nfs, "chmod call failed with \"%s\"", (char *)data);
		return;
	}
}

int nfs_chmod(struct nfs_context *nfs, const char *path, int mode)
{
	struct sync_cb_data cb_data;

	cb_data.is_finished = 0;

	if (nfs_chmod_async(nfs, path, mode, chmod_cb, &cb_data) != 0) {
		nfs_set_error(nfs, "nfs_chmod_async failed");
		return -1;
	}

	wait_for_nfs_reply(nfs, &cb_data);

	return cb_data.status;
}




/*
 * fchmod()
 */
static void fchmod_cb(int status, struct nfs_context *nfs, void *data, void *private_data)
{
	struct sync_cb_data *cb_data = private_data;

	cb_data->is_finished = 1;
	cb_data->status = status;

	if (status < 0) {
		nfs_set_error(nfs, "fchmod call failed with \"%s\"", (char *)data);
		return;
	}
}

int nfs_fchmod(struct nfs_context *nfs, struct nfsfh *nfsfh, int mode)
{
	struct sync_cb_data cb_data;

	cb_data.is_finished = 0;

	if (nfs_fchmod_async(nfs, nfsfh, mode, fchmod_cb, &cb_data) != 0) {
		nfs_set_error(nfs, "nfs_fchmod_async failed");
		return -1;
	}

	wait_for_nfs_reply(nfs, &cb_data);

	return cb_data.status;
}




/*
 * chown()
 */
static void chown_cb(int status, struct nfs_context *nfs, void *data, void *private_data)
{
	struct sync_cb_data *cb_data = private_data;

	cb_data->is_finished = 1;
	cb_data->status = status;

	if (status < 0) {
		nfs_set_error(nfs, "chown call failed with \"%s\"", (char *)data);
		return;
	}
}

int nfs_chown(struct nfs_context *nfs, const char *path, int uid, int gid)
{
	struct sync_cb_data cb_data;

	cb_data.is_finished = 0;

	if (nfs_chown_async(nfs, path, uid, gid, chown_cb, &cb_data) != 0) {
		nfs_set_error(nfs, "nfs_chown_async failed");
		return -1;
	}

	wait_for_nfs_reply(nfs, &cb_data);

	return cb_data.status;
}

/*
 * fchown()
 */
static void fchown_cb(int status, struct nfs_context *nfs, void *data, void *private_data)
{
	struct sync_cb_data *cb_data = private_data;

	cb_data->is_finished = 1;
	cb_data->status = status;

	if (status < 0) {
		nfs_set_error(nfs, "fchown call failed with \"%s\"", (char *)data);
		return;
	}
}

int nfs_fchown(struct nfs_context *nfs, struct nfsfh *nfsfh, int uid, int gid)
{
	struct sync_cb_data cb_data;

	cb_data.is_finished = 0;

	if (nfs_fchown_async(nfs, nfsfh, uid, gid, fchown_cb, &cb_data) != 0) {
		nfs_set_error(nfs, "nfs_fchown_async failed");
		return -1;
	}

	wait_for_nfs_reply(nfs, &cb_data);

	return cb_data.status;
}



/*
 * utimes()
 */
static void utimes_cb(int status, struct nfs_context *nfs, void *data, void *private_data)
{
	struct sync_cb_data *cb_data = private_data;

	cb_data->is_finished = 1;
	cb_data->status = status;

	if (status < 0) {
		nfs_set_error(nfs, "utimes call failed with \"%s\"", (char *)data);
		return;
	}
}

int nfs_utimes(struct nfs_context *nfs, const char *path, struct timeval *times)
{
	struct sync_cb_data cb_data;

	cb_data.is_finished = 0;

	if (nfs_utimes_async(nfs, path, times, utimes_cb, &cb_data) != 0) {
		nfs_set_error(nfs, "nfs_utimes_async failed");
		return -1;
	}

	wait_for_nfs_reply(nfs, &cb_data);

	return cb_data.status;
}



/*
 * utime()
 */
static void utime_cb(int status, struct nfs_context *nfs, void *data, void *private_data)
{
	struct sync_cb_data *cb_data = private_data;

	cb_data->is_finished = 1;
	cb_data->status = status;

	if (status < 0) {
		nfs_set_error(nfs, "utime call failed with \"%s\"", (char *)data);
		return;
	}
}

int nfs_utime(struct nfs_context *nfs, const char *path, struct utimbuf *times)
{
	struct sync_cb_data cb_data;

	cb_data.is_finished = 0;

	if (nfs_utime_async(nfs, path, times, utime_cb, &cb_data) != 0) {
		nfs_set_error(nfs, "nfs_utimes_async failed");
		return -1;
	}

	wait_for_nfs_reply(nfs, &cb_data);

	return cb_data.status;
}




/*
 * access()
 */
static void access_cb(int status, struct nfs_context *nfs, void *data, void *private_data)
{
	struct sync_cb_data *cb_data = private_data;

	cb_data->is_finished = 1;
	cb_data->status = status;

	if (status < 0) {
		nfs_set_error(nfs, "access call failed with \"%s\"", (char *)data);
		return;
	}
}

int nfs_access(struct nfs_context *nfs, const char *path, int mode)
{
	struct sync_cb_data cb_data;

	cb_data.is_finished = 0;

	if (nfs_access_async(nfs, path, mode, access_cb, &cb_data) != 0) {
		nfs_set_error(nfs, "nfs_access_async failed");
		return -1;
	}

	wait_for_nfs_reply(nfs, &cb_data);

	return cb_data.status;
}



/*
 * symlink()
 */
static void symlink_cb(int status, struct nfs_context *nfs, void *data, void *private_data)
{
	struct sync_cb_data *cb_data = private_data;

	cb_data->is_finished = 1;
	cb_data->status = status;

	if (status < 0) {
		nfs_set_error(nfs, "symlink call failed with \"%s\"", (char *)data);
		return;
	}
}

int nfs_symlink(struct nfs_context *nfs, const char *oldpath, const char *newpath)
{
	struct sync_cb_data cb_data;

	cb_data.is_finished = 0;

	if (nfs_symlink_async(nfs, oldpath, newpath, symlink_cb, &cb_data) != 0) {
		nfs_set_error(nfs, "nfs_symlink_async failed");
		return -1;
	}

	wait_for_nfs_reply(nfs, &cb_data);

	return cb_data.status;
}



/*
 * rename()
 */
static void rename_cb(int status, struct nfs_context *nfs, void *data, void *private_data)
{
	struct sync_cb_data *cb_data = private_data;

	cb_data->is_finished = 1;
	cb_data->status = status;

	if (status < 0) {
		nfs_set_error(nfs, "rename call failed with \"%s\"", (char *)data);
		return;
	}
}

int nfs_rename(struct nfs_context *nfs, const char *oldpath, const char *newpath)
{
	struct sync_cb_data cb_data;

	cb_data.is_finished = 0;

	if (nfs_rename_async(nfs, oldpath, newpath, rename_cb, &cb_data) != 0) {
		nfs_set_error(nfs, "nfs_rename_async failed");
		return -1;
	}

	wait_for_nfs_reply(nfs, &cb_data);

	return cb_data.status;
}



/*
 * link()
 */
static void link_cb(int status, struct nfs_context *nfs, void *data, void *private_data)
{
	struct sync_cb_data *cb_data = private_data;

	cb_data->is_finished = 1;
	cb_data->status = status;

	if (status < 0) {
		nfs_set_error(nfs, "link call failed with \"%s\"", (char *)data);
		return;
	}
}

int nfs_link(struct nfs_context *nfs, const char *oldpath, const char *newpath)
{
	struct sync_cb_data cb_data;

	cb_data.is_finished = 0;

	if (nfs_link_async(nfs, oldpath, newpath, link_cb, &cb_data) != 0) {
		nfs_set_error(nfs, "nfs_link_async failed");
		return -1;
	}

	wait_for_nfs_reply(nfs, &cb_data);

	return cb_data.status;
}

void mount_getexports_cb(struct rpc_context *mount_context, int status, void *data, void *private_data)
{
	struct sync_cb_data *cb_data = private_data;
	exports export = *(exports *)data;

	cb_data->is_finished = 1;
	cb_data->status = status;
	cb_data->return_data = NULL;

	if (status != 0) {
		rpc_set_error(mount_context, "mount/export call failed with \"%s\"", (char *)data);
		return;
	}

	while (export != NULL) {
		exports new_export;

		new_export = malloc(sizeof(*new_export));
		memset(new_export, 0, sizeof(*new_export));
		new_export->ex_dir  = strdup(export->ex_dir);
		new_export->ex_next = cb_data->return_data;

		cb_data->return_data = new_export;

		export = export->ex_next;
	}
}

struct exportnode *mount_getexports(const char *server)
{
	struct sync_cb_data cb_data;
	struct rpc_context *rpc;


	cb_data.is_finished = 0;
	cb_data.return_data = NULL;

	rpc = rpc_init_context();
	if (mount_getexports_async(rpc, server, mount_getexports_cb, &cb_data) != 0) {
		rpc_destroy_context(rpc);
		return NULL;
	}

	wait_for_reply(rpc, &cb_data);
	rpc_destroy_context(rpc);

	return cb_data.return_data;
}

void mount_free_export_list(struct exportnode *exports)
{
	struct exportnode *tmp;

	while ((tmp = exports)) {
		exports = exports->ex_next;
		free(tmp->ex_dir);
		free(tmp);
	}
}



void free_nfs_srvr_list(struct nfs_server_list *srv)
{
	while (srv != NULL) {
		struct nfs_server_list *next = srv->next;

		free(srv->addr);
		free(srv);
		srv = next;
	}
}	     

struct nfs_list_data {
       int status;
       struct nfs_server_list *srvrs;
};

void callit_cb(struct rpc_context *rpc, int status, void *data _U_, void *private_data)
{
	struct nfs_list_data *srv_data = private_data;
	struct sockaddr *sin;
	char hostdd[16];
	struct nfs_server_list *srvr;

	if (status == RPC_STATUS_CANCEL) {
		return;
	}
	if (status != 0) {
		srv_data->status = -1;
		return;
	}

	sin = rpc_get_recv_sockaddr(rpc);
	if (sin == NULL) {
		rpc_set_error(rpc, "failed to get sockaddr in CALLIT callback");
		srv_data->status = -1;
		return;
	}

	if (getnameinfo(sin, sizeof(struct sockaddr_in), &hostdd[0], sizeof(hostdd), NULL, 0, NI_NUMERICHOST) < 0) {
		rpc_set_error(rpc, "getnameinfo failed in CALLIT callback");
		srv_data->status = -1;
		return;
	}
	
	/* check for dupes */
	for (srvr = srv_data->srvrs; srvr; srvr = srvr->next) {
		if (!strcmp(hostdd, srvr->addr)) {
			return;
		}
	}

	srvr = malloc(sizeof(struct nfs_server_list));
	if (srvr == NULL) {
		rpc_set_error(rpc, "Malloc failed when allocating server structure");	
		srv_data->status = -1;
		return;
	}

	srvr->addr = strdup(hostdd);
	if (srvr->addr == NULL) {
		rpc_set_error(rpc, "Strdup failed when allocating server structure");
		free(srvr);
		srv_data->status = -1;
		return;
	}

	srvr->next = srv_data->srvrs;
	srv_data->srvrs = srvr;
}

#ifdef WIN32

static int send_nfsd_probes(struct rpc_context *rpc, INTERFACE_INFO *InterfaceList, int numIfs, struct nfs_list_data *data)
{
  int i=0;

  for(i = 0; i < numIfs; i++) 
  {
    SOCKADDR *pAddress;
    char bcdd[16];
    unsigned long nFlags = 0;

    pAddress = (SOCKADDR *) & (InterfaceList[i].iiBroadcastAddress);

    if(pAddress->sa_family != AF_INET)
      continue;
		
    nFlags = InterfaceList[i].iiFlags;

    if (!(nFlags & IFF_UP)) 
    {
      continue;
    }

    if (nFlags & IFF_LOOPBACK) 
    {
      continue;
    }

    if (!(nFlags & IFF_BROADCAST)) 
    {
      continue;
    }

    if (getnameinfo(pAddress, sizeof(struct sockaddr_in), &bcdd[0], sizeof(bcdd), NULL, 0, NI_NUMERICHOST) < 0) 
    {
      continue;
    }

    if (rpc_set_udp_destination(rpc, bcdd, 111, 1) < 0) 
    {
      return -1;
    }

    if (rpc_pmap_callit_async(rpc, MOUNT_PROGRAM, 2, 0, NULL, 0, callit_cb, data) < 0) 
    {
      return -1;
    }
  }
  return 0;
}

struct nfs_server_list *nfs_find_local_servers(void)
{
  struct rpc_context *rpc;
  struct nfs_list_data data = {0, NULL};
  struct timeval tv_start, tv_current;
  int loop;
  struct pollfd pfd;
  INTERFACE_INFO InterfaceList[20];
  unsigned long nBytesReturned;
  int nNumInterfaces = 0;

  rpc = rpc_init_udp_context();
  if (rpc == NULL) 
  {
    return NULL;
  }

  if (rpc_bind_udp(rpc, "0.0.0.0", 0) < 0) 
  {
    rpc_destroy_context(rpc);
    return NULL;
  }

  if (WSAIoctl(rpc_get_fd(rpc), SIO_GET_INTERFACE_LIST, 0, 0, &InterfaceList, sizeof(InterfaceList), &nBytesReturned, 0, 0) == SOCKET_ERROR) 
  {
    return NULL;
  }

  nNumInterfaces = nBytesReturned / sizeof(INTERFACE_INFO);

  for (loop=0; loop<3; loop++) 
  {
    if (send_nfsd_probes(rpc, InterfaceList, nNumInterfaces, &data) != 0) 
    {
      rpc_destroy_context(rpc);
      return NULL;
    }

    win32_gettimeofday(&tv_start, NULL);
    for(;;) 
    {
      int mpt;

      pfd.fd = rpc_get_fd(rpc);
      pfd.events = rpc_which_events(rpc);

      win32_gettimeofday(&tv_current, NULL);
      mpt = 1000
      -    (tv_current.tv_sec *1000 + tv_current.tv_usec / 1000)
      +    (tv_start.tv_sec *1000 + tv_start.tv_usec / 1000);

      if (poll(&pfd, 1, mpt) < 0) 
      {
        free_nfs_srvr_list(data.srvrs);
        rpc_destroy_context(rpc);
        return NULL;
      }
      if (pfd.revents == 0) 
      {
        break;
      }
		
      if (rpc_service(rpc, pfd.revents) < 0) 
      {
        break;
      }
    }
  }

  rpc_destroy_context(rpc);

  if (data.status != 0) 
  {
    free_nfs_srvr_list(data.srvrs);
    return NULL;
  }
  return data.srvrs;
}
#else

static int send_nfsd_probes(struct rpc_context *rpc, struct ifconf *ifc, struct nfs_list_data *data)
{
	char *ptr;

	for (ptr =(char *)(ifc->ifc_buf); ptr < (char *)(ifc->ifc_buf) + ifc->ifc_len; ) {
		struct ifreq *ifr;
		char bcdd[16];

		ifr = (struct ifreq *)ptr;
#ifdef HAVE_SOCKADDR_LEN
		if (ifr->ifr_addr.sa_len > sizeof(struct sockaddr)) {
			ptr += sizeof(ifr->ifr_name) + ifr->ifr_addr.sa_len;
		} else {
			ptr += sizeof(ifr->ifr_name) + sizeof(struct sockaddr);
		}
#else
		ptr += sizeof(struct ifreq);
#endif

		if (ifr->ifr_addr.sa_family != AF_INET) {
			continue;
		}
		if (ioctl(rpc_get_fd(rpc), SIOCGIFFLAGS, ifr) < 0) {
			return -1;
		}
		if (!(ifr->ifr_flags & IFF_UP)) {
			continue;
		}
		if (ifr->ifr_flags & IFF_LOOPBACK) {
			continue;
		}
		if (!(ifr->ifr_flags & IFF_BROADCAST)) {
			continue;
		}
		if (ioctl(rpc_get_fd(rpc), SIOCGIFBRDADDR, ifr) < 0) {
			continue;
		}
		if (getnameinfo(&ifr->ifr_broadaddr, sizeof(struct sockaddr_in), &bcdd[0], sizeof(bcdd), NULL, 0, NI_NUMERICHOST) < 0) {
			continue;
		}
		if (rpc_set_udp_destination(rpc, bcdd, 111, 1) < 0) {
			return -1;
		}

		if (rpc_pmap_callit_async(rpc, MOUNT_PROGRAM, 2, 0, NULL, 0, callit_cb, data) < 0) {
			return -1;
		}
	}

	return 0;
}

struct nfs_server_list *nfs_find_local_servers(void)
{
	struct rpc_context *rpc;
	struct nfs_list_data data = {0, NULL};
	struct timeval tv_start, tv_current;
	struct ifconf ifc;
	int size, loop;
	struct pollfd pfd;

	rpc = rpc_init_udp_context();
	if (rpc == NULL) {
		return NULL;
	}

	if (rpc_bind_udp(rpc, "0.0.0.0", 0) < 0) {
		rpc_destroy_context(rpc);
		return NULL;
	}


	/* get list of all interfaces */
	size = sizeof(struct ifreq);
	ifc.ifc_buf = NULL;
	ifc.ifc_len = size;

	while(ifc.ifc_len > (size - sizeof(struct ifreq))) {
		size *= 2;

		free(ifc.ifc_buf);	
		ifc.ifc_len = size;
		ifc.ifc_buf = malloc(size);
		memset(ifc.ifc_buf, 0, size);
		if (ioctl(rpc_get_fd(rpc), SIOCGIFCONF, (caddr_t)&ifc) < 0) {
			rpc_destroy_context(rpc);
			free(ifc.ifc_buf);	
			return NULL;
		}
	}	

	for (loop=0; loop<3; loop++) {
		if (send_nfsd_probes(rpc, &ifc, &data) != 0) {
			rpc_destroy_context(rpc);
			free(ifc.ifc_buf);	
			return NULL;
		}

		gettimeofday(&tv_start, NULL);
		for(;;) {
			int mpt;

			pfd.fd = rpc_get_fd(rpc);
			pfd.events = rpc_which_events(rpc);

			gettimeofday(&tv_current, NULL);
			mpt = 1000
			-    (tv_current.tv_sec *1000 + tv_current.tv_usec / 1000)
			+    (tv_start.tv_sec *1000 + tv_start.tv_usec / 1000);

			if (poll(&pfd, 1, mpt) < 0) {
				free_nfs_srvr_list(data.srvrs);
				rpc_destroy_context(rpc);
				return NULL;
			}
			if (pfd.revents == 0) {
				break;
			}
		
			if (rpc_service(rpc, pfd.revents) < 0) {
				break;
			}
		}
	}

	free(ifc.ifc_buf);	
	rpc_destroy_context(rpc);

	if (data.status != 0) {
		free_nfs_srvr_list(data.srvrs);
		return NULL;
	}
	return data.srvrs;
}
#endif//WIN32

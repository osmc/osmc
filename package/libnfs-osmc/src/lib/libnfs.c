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
#include <sys/statvfs.h>
#include <utime.h>
#include <unistd.h>
#endif/*WIN32*/

#define _GNU_SOURCE

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "libnfs.h"
#include "libnfs-raw.h"
#include "libnfs-raw-mount.h"
#include "libnfs-raw-nfs.h"
#include "libnfs-private.h"

struct nfsdir {
       struct nfsdirent *entries;
       struct nfsdirent *current;
};

struct nfsfh {
       struct nfs_fh3 fh;
       int is_sync;
       uint64_t offset;
};

struct nfs_context {
       struct rpc_context *rpc;
       char *server;
       char *export;
       struct nfs_fh3 rootfh;
       uint64_t readmax;
       uint64_t writemax;
};

void nfs_free_nfsdir(struct nfsdir *nfsdir)
{
	while (nfsdir->entries) {
		struct nfsdirent *dirent = nfsdir->entries->next;
		if (nfsdir->entries->name != NULL) {
			free(nfsdir->entries->name);
		}
		free(nfsdir->entries);
		nfsdir->entries = dirent;
	}
	free(nfsdir);
}

struct nfs_cb_data;
typedef int (*continue_func)(struct nfs_context *nfs, struct nfs_cb_data *data);

struct nfs_cb_data {
       struct nfs_context *nfs;
       struct nfsfh *nfsfh;
       char *saved_path, *path;

       nfs_cb cb;
       void *private_data;

       continue_func continue_cb;
       void *continue_data;
       void (*free_continue_data)(void *);
       int continue_int;

       struct nfs_fh3 fh;

       /* for multi-read/write calls. */
       int error;
       int cancel;
       int num_calls;
       uint64_t start_offset, max_offset;
       char *buffer;
};

struct nfs_mcb_data {
       struct nfs_cb_data *data;
       uint64_t offset;
       uint64_t count;
};

static int nfs_lookup_path_async_internal(struct nfs_context *nfs, struct nfs_cb_data *data, struct nfs_fh3 *fh);


void nfs_set_auth(struct nfs_context *nfs, AUTH *auth)
{
	rpc_set_auth(nfs->rpc, auth);
}

int nfs_get_fd(struct nfs_context *nfs)
{
	return rpc_get_fd(nfs->rpc);
}

int nfs_queue_length(struct nfs_context *nfs)
{
	return rpc_queue_length(nfs->rpc);
}

int nfs_which_events(struct nfs_context *nfs)
{
	return rpc_which_events(nfs->rpc);
}

int nfs_service(struct nfs_context *nfs, int revents)
{
	return rpc_service(nfs->rpc, revents);
}

char *nfs_get_error(struct nfs_context *nfs)
{
	return rpc_get_error(nfs->rpc);
};

struct nfs_context *nfs_init_context(void)
{
	struct nfs_context *nfs;

	nfs = malloc(sizeof(struct nfs_context));
	if (nfs == NULL) {
		return NULL;
	}
	nfs->rpc = rpc_init_context();
	if (nfs->rpc == NULL) {
		free(nfs);
		return NULL;
	}

	nfs->server = NULL;
	nfs->export = NULL;

	nfs->rootfh.data.data_len = 0;
	nfs->rootfh.data.data_val = NULL;

	return nfs;
}

void nfs_destroy_context(struct nfs_context *nfs)
{
	rpc_destroy_context(nfs->rpc);
	nfs->rpc = NULL;

	if (nfs->server) {
		free(nfs->server);
		nfs->server = NULL;
	}

	if (nfs->export) {
		free(nfs->export);
		nfs->export = NULL;
	}

	if (nfs->rootfh.data.data_val != NULL) {
		free(nfs->rootfh.data.data_val);
		nfs->rootfh.data.data_val = NULL;
	}

	free(nfs);
}

void free_nfs_cb_data(struct nfs_cb_data *data)
{
	if (data->saved_path != NULL) {
		free(data->saved_path);
		data->saved_path = NULL;
	}

	if (data->continue_data != NULL) {
		data->free_continue_data(data->continue_data);
		data->continue_data = NULL;
	}

	if (data->fh.data.data_val != NULL) {
		free(data->fh.data.data_val);
		data->fh.data.data_val = NULL;
	}

	if (data->buffer != NULL) {
		free(data->buffer);
		data->buffer = NULL;
	}

	free(data);
}





static void nfs_mount_10_cb(struct rpc_context *rpc _U_, int status, void *command_data, void *private_data)
{
	struct nfs_cb_data *data = private_data;
	struct nfs_context *nfs = data->nfs;

	if (status == RPC_STATUS_ERROR) {
		data->cb(-EFAULT, nfs, command_data, data->private_data);
		free_nfs_cb_data(data);
		return;
	}
	if (status == RPC_STATUS_CANCEL) {
		data->cb(-EINTR, nfs, "Command was cancelled", data->private_data);
		free_nfs_cb_data(data);
		return;
	}

	data->cb(0, nfs, NULL, data->private_data);
	free_nfs_cb_data(data);
}

static void nfs_mount_9_cb(struct rpc_context *rpc, int status, void *command_data, void *private_data)
{
	struct nfs_cb_data *data = private_data;
	struct nfs_context *nfs = data->nfs;
	FSINFO3res *res = command_data;

	if (status == RPC_STATUS_ERROR) {
		data->cb(-EFAULT, nfs, command_data, data->private_data);
		free_nfs_cb_data(data);
		return;
	}
	if (status == RPC_STATUS_CANCEL) {
		data->cb(-EINTR, nfs, "Command was cancelled", data->private_data);
		free_nfs_cb_data(data);
		return;
	}

	nfs->readmax = res->FSINFO3res_u.resok.rtmax;
	nfs->writemax = res->FSINFO3res_u.resok.wtmax;

	if (rpc_nfs_getattr_async(rpc, nfs_mount_10_cb, &nfs->rootfh, data) != 0) {
		data->cb(-ENOMEM, nfs, command_data, data->private_data);
		free_nfs_cb_data(data);
		return;
	}
}

static void nfs_mount_8_cb(struct rpc_context *rpc, int status, void *command_data, void *private_data)
{
	struct nfs_cb_data *data = private_data;
	struct nfs_context *nfs = data->nfs;

	if (status == RPC_STATUS_ERROR) {
		data->cb(-EFAULT, nfs, command_data, data->private_data);
		free_nfs_cb_data(data);
		return;
	}
	if (status == RPC_STATUS_CANCEL) {
		data->cb(-EINTR, nfs, "Command was cancelled", data->private_data);
		free_nfs_cb_data(data);
		return;
	}

	if (rpc_nfs_fsinfo_async(rpc, nfs_mount_9_cb, &nfs->rootfh, data) != 0) {
		data->cb(-ENOMEM, nfs, command_data, data->private_data);
		free_nfs_cb_data(data);
		return;
	}
}


static void nfs_mount_7_cb(struct rpc_context *rpc, int status, void *command_data, void *private_data)
{
	struct nfs_cb_data *data = private_data;
	struct nfs_context *nfs = data->nfs;

	/* Dont want any more callbacks even if the socket is closed */
	rpc->connect_cb = NULL;

	if (status == RPC_STATUS_ERROR) {
		data->cb(-EFAULT, nfs, command_data, data->private_data);
		free_nfs_cb_data(data);
		return;
	}
	if (status == RPC_STATUS_CANCEL) {
		data->cb(-EINTR, nfs, "Command was cancelled", data->private_data);
		free_nfs_cb_data(data);
		return;
	}

	if (rpc_nfs_null_async(rpc, nfs_mount_8_cb, data) != 0) {
		data->cb(-ENOMEM, nfs, command_data, data->private_data);
		free_nfs_cb_data(data);
		return;
	}
}


static void nfs_mount_6_cb(struct rpc_context *rpc, int status, void *command_data, void *private_data)
{
	struct nfs_cb_data *data = private_data;
	struct nfs_context *nfs = data->nfs;
	mountres3 *res;

	if (status == RPC_STATUS_ERROR) {
		data->cb(-EFAULT, nfs, command_data, data->private_data);
		free_nfs_cb_data(data);
		return;
	}
	if (status == RPC_STATUS_CANCEL) {
		data->cb(-EINTR, nfs, "Command was cancelled", data->private_data);
		free_nfs_cb_data(data);
		return;
	}

	res = command_data;
	if (res->fhs_status != MNT3_OK) {
		rpc_set_error(rpc, "RPC error: Mount failed with error %s(%d) %s(%d)", mountstat3_to_str(res->fhs_status), res->fhs_status, strerror(-mountstat3_to_errno(res->fhs_status)), -mountstat3_to_errno(res->fhs_status));
		data->cb(mountstat3_to_errno(res->fhs_status), nfs, rpc_get_error(rpc), data->private_data);
		free_nfs_cb_data(data);
		return;
	}

	nfs->rootfh.data.data_len = res->mountres3_u.mountinfo.fhandle.fhandle3_len;
	nfs->rootfh.data.data_val = malloc(nfs->rootfh.data.data_len);
	if (nfs->rootfh.data.data_val == NULL) {
		rpc_set_error(rpc, "Out of memory. Could not allocate memory to store root filehandle");
		data->cb(-ENOMEM, nfs, rpc_get_error(rpc), data->private_data);
		free_nfs_cb_data(data);
		return;
	}
	memcpy(nfs->rootfh.data.data_val, res->mountres3_u.mountinfo.fhandle.fhandle3_val, nfs->rootfh.data.data_len);

	rpc_disconnect(rpc, "normal disconnect");
	if (rpc_connect_async(rpc, nfs->server, 2049, nfs_mount_7_cb, data) != 0) {
		data->cb(-ENOMEM, nfs, command_data, data->private_data);
		free_nfs_cb_data(data);
		return;
	}
	/* NFS TCP connections we want to autoreconnect after sessions are torn down (due to inactivity or error) */
	rpc_set_autoreconnect(rpc);
}


static void nfs_mount_5_cb(struct rpc_context *rpc, int status, void *command_data, void *private_data)
{
	struct nfs_cb_data *data = private_data;
	struct nfs_context *nfs = data->nfs;

	if (status == RPC_STATUS_ERROR) {
		data->cb(-EFAULT, nfs, command_data, data->private_data);
		free_nfs_cb_data(data);
		return;
	}
	if (status == RPC_STATUS_CANCEL) {
		data->cb(-EINTR, nfs, "Command was cancelled", data->private_data);
		free_nfs_cb_data(data);
		return;
	}

	if (rpc_mount_mnt_async(rpc, nfs_mount_6_cb, nfs->export, data) != 0) {
		data->cb(-ENOMEM, nfs, command_data, data->private_data);
		free_nfs_cb_data(data);
		return;
	}
}

static void nfs_mount_4_cb(struct rpc_context *rpc, int status, void *command_data, void *private_data)
{
	struct nfs_cb_data *data = private_data;
	struct nfs_context *nfs = data->nfs;

	/* Dont want any more callbacks even if the socket is closed */
	rpc->connect_cb = NULL;

	if (status == RPC_STATUS_ERROR) {
		data->cb(-EFAULT, nfs, command_data, data->private_data);
		free_nfs_cb_data(data);
		return;
	}
	if (status == RPC_STATUS_CANCEL) {
		data->cb(-EINTR, nfs, "Command was cancelled", data->private_data);
		free_nfs_cb_data(data);
		return;
	}

	if (rpc_mount_null_async(rpc, nfs_mount_5_cb, data) != 0) {
		data->cb(-ENOMEM, nfs, command_data, data->private_data);
		free_nfs_cb_data(data);
		return;
	}
}

static void nfs_mount_3_cb(struct rpc_context *rpc, int status, void *command_data, void *private_data)
{
	struct nfs_cb_data *data = private_data;
	struct nfs_context *nfs = data->nfs;
	uint32_t mount_port;

	if (status == RPC_STATUS_ERROR) {	
		data->cb(-EFAULT, nfs, command_data, data->private_data);
		free_nfs_cb_data(data);
		return;
	}
	if (status == RPC_STATUS_CANCEL) {
		data->cb(-EINTR, nfs, "Command was cancelled", data->private_data);
		free_nfs_cb_data(data);
		return;
	}

	mount_port = *(uint32_t *)command_data;
	if (mount_port == 0) {
		rpc_set_error(rpc, "RPC error. Mount program is not available on %s", nfs->server);
		data->cb(-ENOENT, nfs, command_data, data->private_data);
		free_nfs_cb_data(data);
		return;
	}

	rpc_disconnect(rpc, "normal disconnect");
	if (rpc_connect_async(rpc, nfs->server, mount_port, nfs_mount_4_cb, data) != 0) {
		data->cb(-ENOMEM, nfs, command_data, data->private_data);
		free_nfs_cb_data(data);
		return;
	}
}


static void nfs_mount_2_cb(struct rpc_context *rpc, int status, void *command_data, void *private_data)
{
	struct nfs_cb_data *data = private_data;
	struct nfs_context *nfs = data->nfs;

	if (status == RPC_STATUS_ERROR) {
		data->cb(-EFAULT, nfs, command_data, data->private_data);
		free_nfs_cb_data(data);
		return;
	}
	if (status == RPC_STATUS_CANCEL) {
		data->cb(-EINTR, nfs, "Command was cancelled", data->private_data);
		free_nfs_cb_data(data);
		return;
	}

	if (rpc_pmap_getport_async(rpc, MOUNT_PROGRAM, MOUNT_V3, IPPROTO_TCP, nfs_mount_3_cb, private_data) != 0) {
		data->cb(-ENOMEM, nfs, command_data, data->private_data);
		free_nfs_cb_data(data);
		return;
	}
}

static void nfs_mount_1_cb(struct rpc_context *rpc, int status, void *command_data, void *private_data)
{
	struct nfs_cb_data *data = private_data;
	struct nfs_context *nfs = data->nfs;

	/* Dont want any more callbacks even if the socket is closed */
	rpc->connect_cb = NULL;

	if (status == RPC_STATUS_ERROR) {
		data->cb(-EFAULT, nfs, command_data, data->private_data);
		free_nfs_cb_data(data);
		return;
	}
	if (status == RPC_STATUS_CANCEL) {
		data->cb(-EINTR, nfs, "Command was cancelled", data->private_data);
		free_nfs_cb_data(data);
		return;
	}

	if (rpc_pmap_null_async(rpc, nfs_mount_2_cb, data) != 0) {
		data->cb(-ENOMEM, nfs, command_data, data->private_data);
		free_nfs_cb_data(data);
		return;
	}
}

/*
 * Async call for mounting an nfs share and geting the root filehandle
 */
int nfs_mount_async(struct nfs_context *nfs, const char *server, const char *export, nfs_cb cb, void *private_data)
{
	struct nfs_cb_data *data;
	char *new_server, *new_export;

	data = malloc(sizeof(struct nfs_cb_data));
	if (data == NULL) {
		rpc_set_error(nfs->rpc, "out of memory. failed to allocate memory for nfs mount data");
		return -1;
	}
	memset(data, 0, sizeof(struct nfs_cb_data));
	new_server = strdup(server);
	new_export = strdup(export);
	if (nfs->server != NULL) {
		free(nfs->server);
	}
	nfs->server        = new_server;
	if (nfs->export != NULL) {
		free(nfs->export);
	}
	nfs->export        = new_export;
	data->nfs          = nfs;
	data->cb           = cb;
	data->private_data = private_data;

	if (rpc_connect_async(nfs->rpc, server, 111, nfs_mount_1_cb, data) != 0) {
		rpc_set_error(nfs->rpc, "Failed to start connection");
		free_nfs_cb_data(data);
		return -1;
	}

	return 0;
}



/*
 * Functions to first look up a path, component by component, and then finally call a specific function once
 * the filehandle for the final component is found.
 */
static void nfs_lookup_path_1_cb(struct rpc_context *rpc _U_, int status, void *command_data, void *private_data)
{
	struct nfs_cb_data *data = private_data;
	struct nfs_context *nfs = data->nfs;
	LOOKUP3res *res;

	if (status == RPC_STATUS_ERROR) {
		data->cb(-EFAULT, nfs, command_data, data->private_data);
		free_nfs_cb_data(data);
		return;
	}
	if (status == RPC_STATUS_CANCEL) {
		data->cb(-EINTR, nfs, "Command was cancelled", data->private_data);
		free_nfs_cb_data(data);
		return;
	}

	res = command_data;
	if (res->status != NFS3_OK) {
		rpc_set_error(nfs->rpc, "NFS: Lookup of %s failed with %s(%d)", data->saved_path, nfsstat3_to_str(res->status), nfsstat3_to_errno(res->status));
		data->cb(nfsstat3_to_errno(res->status), nfs, rpc_get_error(nfs->rpc), data->private_data);
		free_nfs_cb_data(data);
		return;
	}

	if (nfs_lookup_path_async_internal(nfs, data, &res->LOOKUP3res_u.resok.object) != 0) {
		rpc_set_error(nfs->rpc, "Failed to create lookup pdu");
		data->cb(-ENOMEM, nfs, rpc_get_error(nfs->rpc), data->private_data);
		free_nfs_cb_data(data);
		return;
	}
}

static int nfs_lookup_path_async_internal(struct nfs_context *nfs, struct nfs_cb_data *data, struct nfs_fh3 *fh)
{
	char *path, *str;

	while (*data->path == '/') {
	      data->path++;
	}

	path = data->path;
	str = strchr(path, '/');
	if (str != NULL) {
		*str = 0;
		data->path = str+1;
	} else {
		while (*data->path != 0) {
		      data->path++;
		}
	}

	if (*path == 0) {
		data->fh.data.data_len = fh->data.data_len;
		data->fh.data.data_val = malloc(data->fh.data.data_len);
		if (data->fh.data.data_val == NULL) {
			rpc_set_error(nfs->rpc, "Out of memory: Failed to allocate fh for %s", data->path);
			data->cb(-ENOMEM, nfs, rpc_get_error(nfs->rpc), data->private_data);
			free_nfs_cb_data(data);
			return -1;
		}
		memcpy(data->fh.data.data_val, fh->data.data_val, data->fh.data.data_len);
		data->continue_cb(nfs, data);
		return 0;
	}

	if (rpc_nfs_lookup_async(nfs->rpc, nfs_lookup_path_1_cb, fh, path, data) != 0) {
		rpc_set_error(nfs->rpc, "RPC error: Failed to send lookup call for %s", data->path);
		data->cb(-ENOMEM, nfs, rpc_get_error(nfs->rpc), data->private_data);
		free_nfs_cb_data(data);
		return -1;
	}
	return 0;
}

static int nfs_lookuppath_async(struct nfs_context *nfs, const char *path, nfs_cb cb, void *private_data, continue_func continue_cb, void *continue_data, void (*free_continue_data)(void *), int continue_int)
{
	struct nfs_cb_data *data;

	if (path[0] != '/') {
		rpc_set_error(nfs->rpc, "Pathname is not absulute %s", path);
		return -1;
	}

	data = malloc(sizeof(struct nfs_cb_data));
	if (data == NULL) {
		rpc_set_error(nfs->rpc, "out of memory: failed to allocate nfs_cb_data structure");
		return -1;
	}
	memset(data, 0, sizeof(struct nfs_cb_data));
	data->nfs                = nfs;
	data->cb                 = cb;
	data->continue_cb        = continue_cb;
	data->continue_data      = continue_data;
	data->free_continue_data = free_continue_data;
	data->continue_int       = continue_int;
	data->private_data       = private_data;
	data->saved_path         = strdup(path);
	if (data->saved_path == NULL) {
		rpc_set_error(nfs->rpc, "out of memory: failed to copy path string");
		free_nfs_cb_data(data);
		return -1;
	}
	data->path = data->saved_path;

	if (nfs_lookup_path_async_internal(nfs, data, &nfs->rootfh) != 0) {
		/* return 0 here since the callback will be invoked if there is a failure */
		return 0;
	}
	return 0;
}





/*
 * Async stat()
 */
static void nfs_stat_1_cb(struct rpc_context *rpc _U_, int status, void *command_data, void *private_data)
{
	GETATTR3res *res;
	struct nfs_cb_data *data = private_data;
	struct nfs_context *nfs = data->nfs;
	struct stat st;

	if (status == RPC_STATUS_ERROR) {
		data->cb(-EFAULT, nfs, command_data, data->private_data);
		free_nfs_cb_data(data);
		return;
	}
	if (status == RPC_STATUS_CANCEL) {
		data->cb(-EINTR, nfs, "Command was cancelled", data->private_data);
		free_nfs_cb_data(data);
		return;
	}

	res = command_data;
	if (res->status != NFS3_OK) {
		rpc_set_error(nfs->rpc, "NFS: GETATTR of %s failed with %s(%d)", data->saved_path, nfsstat3_to_str(res->status), nfsstat3_to_errno(res->status));
		data->cb(nfsstat3_to_errno(res->status), nfs, rpc_get_error(nfs->rpc), data->private_data);
		free_nfs_cb_data(data);
		return;
	}

        st.st_dev     = -1;
        st.st_ino     = res->GETATTR3res_u.resok.obj_attributes.fileid;
        st.st_mode    = res->GETATTR3res_u.resok.obj_attributes.mode;
	if (res->GETATTR3res_u.resok.obj_attributes.type == NF3DIR) {
		st.st_mode |= S_IFDIR ;
	}
	if (res->GETATTR3res_u.resok.obj_attributes.type == NF3REG) {
		st.st_mode |= S_IFREG ;
	}
        st.st_nlink   = res->GETATTR3res_u.resok.obj_attributes.nlink;
        st.st_uid     = res->GETATTR3res_u.resok.obj_attributes.uid;
        st.st_gid     = res->GETATTR3res_u.resok.obj_attributes.gid;
        st.st_rdev    = 0;
        st.st_size    = res->GETATTR3res_u.resok.obj_attributes.size;
#ifndef WIN32
        st.st_blksize = 4096;
        st.st_blocks  = res->GETATTR3res_u.resok.obj_attributes.size / 4096;
#endif//WIN32        
        st.st_atime   = res->GETATTR3res_u.resok.obj_attributes.atime.seconds;
        st.st_mtime   = res->GETATTR3res_u.resok.obj_attributes.mtime.seconds;
        st.st_ctime   = res->GETATTR3res_u.resok.obj_attributes.ctime.seconds;

	data->cb(0, nfs, &st, data->private_data);
	free_nfs_cb_data(data);
}

static int nfs_stat_continue_internal(struct nfs_context *nfs, struct nfs_cb_data *data)
{
	if (rpc_nfs_getattr_async(nfs->rpc, nfs_stat_1_cb, &data->fh, data) != 0) {
		rpc_set_error(nfs->rpc, "RPC error: Failed to send STAT GETATTR call for %s", data->path);
		data->cb(-ENOMEM, nfs, rpc_get_error(nfs->rpc), data->private_data);
		free_nfs_cb_data(data);
		return -1;
	}
	return 0;
}

int nfs_stat_async(struct nfs_context *nfs, const char *path, nfs_cb cb, void *private_data)
{
	if (nfs_lookuppath_async(nfs, path, cb, private_data, nfs_stat_continue_internal, NULL, NULL, 0) != 0) {
		rpc_set_error(nfs->rpc, "Out of memory: failed to start parsing the path components");
		return -1;
	}

	return 0;
}





/*
 * Async open()
 */
static void nfs_open_cb(struct rpc_context *rpc _U_, int status, void *command_data, void *private_data)
{
	ACCESS3res *res;
	struct nfs_cb_data *data = private_data;
	struct nfs_context *nfs = data->nfs;
	struct nfsfh *nfsfh;
	unsigned int nfsmode = 0;

	if (status == RPC_STATUS_ERROR) {
		data->cb(-EFAULT, nfs, command_data, data->private_data);
		free_nfs_cb_data(data);
		return;
	}
	if (status == RPC_STATUS_CANCEL) {
		data->cb(-EINTR, nfs, "Command was cancelled", data->private_data);
		free_nfs_cb_data(data);
		return;
	}

	res = command_data;
	if (res->status != NFS3_OK) {
		rpc_set_error(nfs->rpc, "NFS: ACCESS of %s failed with %s(%d)", data->saved_path, nfsstat3_to_str(res->status), nfsstat3_to_errno(res->status));
		data->cb(nfsstat3_to_errno(res->status), nfs, rpc_get_error(nfs->rpc), data->private_data);
		free_nfs_cb_data(data);
		return;
	}

	if (data->continue_int & O_WRONLY) {
		nfsmode |= ACCESS3_MODIFY;
	}
	if (data->continue_int & O_RDWR) {
		nfsmode |= ACCESS3_READ|ACCESS3_MODIFY;
	}
	if (!(data->continue_int & (O_WRONLY|O_RDWR))) {
		nfsmode |= ACCESS3_READ;
	}


	if (res->ACCESS3res_u.resok.access != nfsmode) {
		rpc_set_error(nfs->rpc, "NFS: ACCESS denied. Required access %c%c%c. Allowed access %c%c%c",
					nfsmode&ACCESS3_READ?'r':'-',
					nfsmode&ACCESS3_MODIFY?'w':'-',
					nfsmode&ACCESS3_EXECUTE?'x':'-',
					res->ACCESS3res_u.resok.access&ACCESS3_READ?'r':'-',
					res->ACCESS3res_u.resok.access&ACCESS3_MODIFY?'w':'-',
					res->ACCESS3res_u.resok.access&ACCESS3_EXECUTE?'x':'-');
		data->cb(-EACCES, nfs, rpc_get_error(nfs->rpc), data->private_data);
		free_nfs_cb_data(data);
		return;
	}

	nfsfh = malloc(sizeof(struct nfsfh));
	if (nfsfh == NULL) {
		rpc_set_error(nfs->rpc, "NFS: Failed to allocate nfsfh structure");
		data->cb(-ENOMEM, nfs, rpc_get_error(nfs->rpc), data->private_data);
		free_nfs_cb_data(data);
		return;
	}
	memset(nfsfh, 0, sizeof(struct nfsfh));

	if (data->continue_int & O_SYNC) {
		nfsfh->is_sync = 1;
	}

	/* steal the filehandle */
	nfsfh->fh.data.data_len = data->fh.data.data_len;
	nfsfh->fh.data.data_val = data->fh.data.data_val;
	data->fh.data.data_val = NULL;

	data->cb(0, nfs, nfsfh, data->private_data);
	free_nfs_cb_data(data);
}

static int nfs_open_continue_internal(struct nfs_context *nfs, struct nfs_cb_data *data)
{
	int nfsmode = 0;

	if (data->continue_int & O_WRONLY) {
		nfsmode |= ACCESS3_MODIFY;
	}
	if (data->continue_int & O_RDWR) {
		nfsmode |= ACCESS3_READ|ACCESS3_MODIFY;
	}
	if (!(data->continue_int & (O_WRONLY|O_RDWR))) {
		nfsmode |= ACCESS3_READ;
	}

	if (rpc_nfs_access_async(nfs->rpc, nfs_open_cb, &data->fh, nfsmode, data) != 0) {
		rpc_set_error(nfs->rpc, "RPC error: Failed to send OPEN ACCESS call for %s", data->path);
		data->cb(-ENOMEM, nfs, rpc_get_error(nfs->rpc), data->private_data);
		free_nfs_cb_data(data);
		return -1;
	}
	return 0;
}

int nfs_open_async(struct nfs_context *nfs, const char *path, int mode, nfs_cb cb, void *private_data)
{
	if (nfs_lookuppath_async(nfs, path, cb, private_data, nfs_open_continue_internal, NULL, NULL, mode) != 0) {
		rpc_set_error(nfs->rpc, "Out of memory: failed to start parsing the path components");
		return -1;
	}

	return 0;
}





/*
 * Async pread()
 */
static void nfs_pread_cb(struct rpc_context *rpc _U_, int status, void *command_data, void *private_data)
{
	struct nfs_cb_data *data = private_data;
	struct nfs_context *nfs = data->nfs;
	READ3res *res;

	if (status == RPC_STATUS_ERROR) {
		data->cb(-EFAULT, nfs, command_data, data->private_data);
		free_nfs_cb_data(data);
		return;
	}
	if (status == RPC_STATUS_CANCEL) {
		data->cb(-EINTR, nfs, "Command was cancelled", data->private_data);
		free_nfs_cb_data(data);
		return;
	}

	res = command_data;
	if (res->status != NFS3_OK) {
		rpc_set_error(nfs->rpc, "NFS: Read failed with %s(%d)", nfsstat3_to_str(res->status), nfsstat3_to_errno(res->status));
		data->cb(nfsstat3_to_errno(res->status), nfs, rpc_get_error(nfs->rpc), data->private_data);
		free_nfs_cb_data(data);
		return;
	}

	data->nfsfh->offset += res->READ3res_u.resok.count;
	data->cb(res->READ3res_u.resok.count, nfs, res->READ3res_u.resok.data.data_val, data->private_data);
	free_nfs_cb_data(data);
}

static void nfs_pread_mcb(struct rpc_context *rpc _U_, int status, void *command_data, void *private_data)
{
	struct nfs_mcb_data *mdata = private_data;
	struct nfs_cb_data *data = mdata->data;
	struct nfs_context *nfs = data->nfs;
	READ3res *res;

	data->num_calls--;

	if (status == RPC_STATUS_ERROR) {
		/* flag the failure but do not invoke callback until we have received all responses */
		data->error = 1;
	}
	if (status == RPC_STATUS_CANCEL) {
		/* flag the cancellation but do not invoke callback until we have received all responses */
		data->cancel = 1;
	}

	/* reassemble the data into the buffer */
	if (status == RPC_STATUS_SUCCESS) {
		res = command_data;
		if (res->status != NFS3_OK) {
			rpc_set_error(nfs->rpc, "NFS: Read failed with %s(%d)", nfsstat3_to_str(res->status), nfsstat3_to_errno(res->status));
			data->error = 1;
		} else  {
			if (res->READ3res_u.resok.count > 0) {
				memcpy(&data->buffer[mdata->offset - data->start_offset], res->READ3res_u.resok.data.data_val, res->READ3res_u.resok.count);
				if ((unsigned)data->max_offset < mdata->offset + res->READ3res_u.resok.count) {
					data->max_offset = mdata->offset + res->READ3res_u.resok.count;
				}
			}
		}
	}

	if (data->num_calls > 0) {
		/* still waiting for more replies */
		free(mdata);
		return;
	}

	if (data->error != 0) {
		data->cb(-EFAULT, nfs, command_data, data->private_data);
		free_nfs_cb_data(data);
		free(mdata);
		return;
	}
	if (data->cancel != 0) {
		data->cb(-EINTR, nfs, "Command was cancelled", data->private_data);
		free_nfs_cb_data(data);
		free(mdata);
		return;
	}

	data->nfsfh->offset = data->max_offset;
	data->cb(data->max_offset - data->start_offset, nfs, data->buffer, data->private_data);

	free_nfs_cb_data(data);
	free(mdata);
}

int nfs_pread_async(struct nfs_context *nfs, struct nfsfh *nfsfh, uint64_t offset, uint64_t count, nfs_cb cb, void *private_data)
{
	struct nfs_cb_data *data;

	data = malloc(sizeof(struct nfs_cb_data));
	if (data == NULL) {
		rpc_set_error(nfs->rpc, "out of memory: failed to allocate nfs_cb_data structure");
		return -1;
	}
	memset(data, 0, sizeof(struct nfs_cb_data));
	data->nfs          = nfs;
	data->cb           = cb;
	data->private_data = private_data;
	data->nfsfh        = nfsfh;

	nfsfh->offset = offset;

	if (count <= nfs_get_readmax(nfs)) {
		if (rpc_nfs_read_async(nfs->rpc, nfs_pread_cb, &nfsfh->fh, offset, count, data) != 0) {
			rpc_set_error(nfs->rpc, "RPC error: Failed to send READ call for %s", data->path);
			data->cb(-ENOMEM, nfs, rpc_get_error(nfs->rpc), data->private_data);
			free_nfs_cb_data(data);
			return -1;
		}
		return 0;
	}

	/* trying to read more than maximum server read size, we has to chop it up into smaller
	 * reads and collect into a reassembly buffer.
	 * we send all reads in parallell so that performance is still good.
	 */
	data->max_offset = offset;
	data->start_offset = offset;

	data->buffer = 	malloc(count);
	if (data->buffer == NULL) {
		rpc_set_error(nfs->rpc, "Out-Of-Memory: Failed to allocate reassembly buffer for %d bytes", (int)count);
		data->cb(-ENOMEM, nfs, rpc_get_error(nfs->rpc), data->private_data);
		free_nfs_cb_data(data);
		return -1;
	}

	while (count > 0) {
		uint64_t readcount = count;
		struct nfs_mcb_data *mdata;

		if (readcount > nfs_get_readmax(nfs)) {
			readcount = nfs_get_readmax(nfs);
		}

		mdata = malloc(sizeof(struct nfs_mcb_data));
		if (mdata == NULL) {
			rpc_set_error(nfs->rpc, "out of memory: failed to allocate nfs_mcb_data structure");
			return -1;
		}
		memset(mdata, 0, sizeof(struct nfs_mcb_data));
		mdata->data   = data;
		mdata->offset = offset;
		mdata->count  = readcount;
		if (rpc_nfs_read_async(nfs->rpc, nfs_pread_mcb, &nfsfh->fh, offset, readcount, mdata) != 0) {
			rpc_set_error(nfs->rpc, "RPC error: Failed to send READ call for %s", data->path);
			data->cb(-ENOMEM, nfs, rpc_get_error(nfs->rpc), data->private_data);
			free(mdata);
			return -1;
		}

		count               -= readcount;
		offset              += readcount;
		data->num_calls++;
	 }

	 return 0;
}

/*
 * Async read()
 */
int nfs_read_async(struct nfs_context *nfs, struct nfsfh *nfsfh, uint64_t count, nfs_cb cb, void *private_data)
{
	return nfs_pread_async(nfs, nfsfh, nfsfh->offset, count, cb, private_data);
}



/*
 * Async pwrite()
 */
static void nfs_pwrite_cb(struct rpc_context *rpc _U_, int status, void *command_data, void *private_data)
{
	struct nfs_cb_data *data = private_data;
	struct nfs_context *nfs = data->nfs;
	WRITE3res *res;

	if (status == RPC_STATUS_ERROR) {
		data->cb(-EFAULT, nfs, command_data, data->private_data);
		free_nfs_cb_data(data);
		return;
	}
	if (status == RPC_STATUS_CANCEL) {
		data->cb(-EINTR, nfs, "Command was cancelled", data->private_data);
		free_nfs_cb_data(data);
		return;
	}

	res = command_data;
	if (res->status != NFS3_OK) {
		rpc_set_error(nfs->rpc, "NFS: Write failed with %s(%d)", nfsstat3_to_str(res->status), nfsstat3_to_errno(res->status));
		data->cb(nfsstat3_to_errno(res->status), nfs, rpc_get_error(nfs->rpc), data->private_data);
		free_nfs_cb_data(data);
		return;
	}

	data->nfsfh->offset += res->WRITE3res_u.resok.count;
	data->cb(res->WRITE3res_u.resok.count, nfs, NULL, data->private_data);
	free_nfs_cb_data(data);
}

static void nfs_pwrite_mcb(struct rpc_context *rpc _U_, int status, void *command_data, void *private_data)
{
	struct nfs_mcb_data *mdata = private_data;
	struct nfs_cb_data *data = mdata->data;
	struct nfs_context *nfs = data->nfs;
	WRITE3res *res;

	data->num_calls--;

	if (status == RPC_STATUS_ERROR) {
		/* flag the failure but do not invoke callback until we have received all responses */
		data->error = 1;
	}
	if (status == RPC_STATUS_CANCEL) {
		/* flag the cancellation but do not invoke callback until we have received all responses */
		data->cancel = 1;
	}

	if (status == RPC_STATUS_SUCCESS) {
		res = command_data;
		if (res->status != NFS3_OK) {
			rpc_set_error(nfs->rpc, "NFS: Write failed with %s(%d)", nfsstat3_to_str(res->status), nfsstat3_to_errno(res->status));
			data->error = 1;
		} else  {
			if (res->WRITE3res_u.resok.count > 0) {
				if ((unsigned)data->max_offset < mdata->offset + res->WRITE3res_u.resok.count) {
					data->max_offset = mdata->offset + res->WRITE3res_u.resok.count;
				}
			}
		}
	}

	if (data->num_calls > 0) {
		/* still waiting for more replies */
		free(mdata);
		return;
	}

	if (data->error != 0) {
		data->cb(-EFAULT, nfs, command_data, data->private_data);
		free_nfs_cb_data(data);
		free(mdata);
		return;
	}
	if (data->cancel != 0) {
		data->cb(-EINTR, nfs, "Command was cancelled", data->private_data);
		free_nfs_cb_data(data);
		free(mdata);
		return;
	}

	data->nfsfh->offset = data->max_offset;
	data->cb(data->max_offset - data->start_offset, nfs, NULL, data->private_data);

	free_nfs_cb_data(data);
	free(mdata);
}


int nfs_pwrite_async(struct nfs_context *nfs, struct nfsfh *nfsfh, uint64_t offset, uint64_t count, char *buf, nfs_cb cb, void *private_data)
{
	struct nfs_cb_data *data;

	data = malloc(sizeof(struct nfs_cb_data));
	if (data == NULL) {
		rpc_set_error(nfs->rpc, "out of memory: failed to allocate nfs_cb_data structure");
		return -1;
	}
	memset(data, 0, sizeof(struct nfs_cb_data));
	data->nfs          = nfs;
	data->cb           = cb;
	data->private_data = private_data;
	data->nfsfh        = nfsfh;

	nfsfh->offset = offset;

	if (count <= nfs_get_writemax(nfs)) {
		if (rpc_nfs_write_async(nfs->rpc, nfs_pwrite_cb, &nfsfh->fh, buf, offset, count, nfsfh->is_sync?FILE_SYNC:UNSTABLE, data) != 0) {
			rpc_set_error(nfs->rpc, "RPC error: Failed to send WRITE call for %s", data->path);
			data->cb(-ENOMEM, nfs, rpc_get_error(nfs->rpc), data->private_data);
			free_nfs_cb_data(data);
			return -1;
		}
		return 0;
	}

	/* trying to write more than maximum server write size, we has to chop it up into smaller
	 * chunks.
	 * we send all writes in parallell so that performance is still good.
	 */
	data->max_offset = offset;
	data->start_offset = offset;

	while (count > 0) {
		uint64_t writecount = count;
		struct nfs_mcb_data *mdata;

		if (writecount > nfs_get_writemax(nfs)) {
			writecount = nfs_get_writemax(nfs);
		}

		mdata = malloc(sizeof(struct nfs_mcb_data));
		if (mdata == NULL) {
			rpc_set_error(nfs->rpc, "out of memory: failed to allocate nfs_mcb_data structure");
			return -1;
		}
		memset(mdata, 0, sizeof(struct nfs_mcb_data));
		mdata->data   = data;
		mdata->offset = offset;
		mdata->count  = writecount;

		if (rpc_nfs_write_async(nfs->rpc, nfs_pwrite_mcb, &nfsfh->fh, &buf[offset - data->start_offset], offset, writecount, nfsfh->is_sync?FILE_SYNC:UNSTABLE, mdata) != 0) {
			rpc_set_error(nfs->rpc, "RPC error: Failed to send WRITE call for %s", data->path);
			data->cb(-ENOMEM, nfs, rpc_get_error(nfs->rpc), data->private_data);
			free(mdata);
			return -1;
		}

		count               -= writecount;
		offset              += writecount;
		data->num_calls++;
	}

	return 0;
}

/*
 * Async write()
 */
int nfs_write_async(struct nfs_context *nfs, struct nfsfh *nfsfh, uint64_t count, char *buf, nfs_cb cb, void *private_data)
{
	return nfs_pwrite_async(nfs, nfsfh, nfsfh->offset, count, buf, cb, private_data);
}




/*
 * close
 */
 
int nfs_close_async(struct nfs_context *nfs, struct nfsfh *nfsfh, nfs_cb cb, void *private_data)
{
	if (nfsfh->fh.data.data_val != NULL){
		free(nfsfh->fh.data.data_val);
		nfsfh->fh.data.data_val = NULL;
	}
	free(nfsfh);

	cb(0, nfs, NULL, private_data);
	return 0;
};





/*
 * Async fstat()
 */
int nfs_fstat_async(struct nfs_context *nfs, struct nfsfh *nfsfh, nfs_cb cb, void *private_data)
{
	struct nfs_cb_data *data;

	data = malloc(sizeof(struct nfs_cb_data));
	if (data == NULL) {
		rpc_set_error(nfs->rpc, "out of memory: failed to allocate nfs_cb_data structure");
		return -1;
	}
	memset(data, 0, sizeof(struct nfs_cb_data));
	data->nfs          = nfs;
	data->cb           = cb;
	data->private_data = private_data;

	if (rpc_nfs_getattr_async(nfs->rpc, nfs_stat_1_cb, &nfsfh->fh, data) != 0) {
		rpc_set_error(nfs->rpc, "RPC error: Failed to send STAT GETATTR call for %s", data->path);
		data->cb(-ENOMEM, nfs, rpc_get_error(nfs->rpc), data->private_data);
		free_nfs_cb_data(data);
		return -1;
	}
	return 0;
}



/*
 * Async fsync()
 */
static void nfs_fsync_cb(struct rpc_context *rpc _U_, int status, void *command_data, void *private_data)
{
	struct nfs_cb_data *data = private_data;
	struct nfs_context *nfs = data->nfs;
	COMMIT3res *res;

	if (status == RPC_STATUS_ERROR) {
		data->cb(-EFAULT, nfs, command_data, data->private_data);
		free_nfs_cb_data(data);
		return;
	}
	if (status == RPC_STATUS_CANCEL) {
		data->cb(-EINTR, nfs, "Command was cancelled", data->private_data);
		free_nfs_cb_data(data);
		return;
	}

	res = command_data;
	if (res->status != NFS3_OK) {
		rpc_set_error(nfs->rpc, "NFS: Commit failed with %s(%d)", nfsstat3_to_str(res->status), nfsstat3_to_errno(res->status));
		data->cb(nfsstat3_to_errno(res->status), nfs, rpc_get_error(nfs->rpc), data->private_data);
		free_nfs_cb_data(data);
		return;
	}

	data->cb(0, nfs, NULL, data->private_data);
	free_nfs_cb_data(data);
}

int nfs_fsync_async(struct nfs_context *nfs, struct nfsfh *nfsfh, nfs_cb cb, void *private_data)
{
	struct nfs_cb_data *data;

	data = malloc(sizeof(struct nfs_cb_data));
	if (data == NULL) {
		rpc_set_error(nfs->rpc, "out of memory: failed to allocate nfs_cb_data structure");
		return -1;
	}
	memset(data, 0, sizeof(struct nfs_cb_data));
	data->nfs          = nfs;
	data->cb           = cb;
	data->private_data = private_data;

	if (rpc_nfs_commit_async(nfs->rpc, nfs_fsync_cb, &nfsfh->fh, data) != 0) {
		rpc_set_error(nfs->rpc, "RPC error: Failed to send COMMIT call for %s", data->path);
		data->cb(-ENOMEM, nfs, rpc_get_error(nfs->rpc), data->private_data);
		free_nfs_cb_data(data);
		return -1;
	}
	return 0;
}




/*
 * Async ftruncate()
 */
static void nfs_ftruncate_cb(struct rpc_context *rpc _U_, int status, void *command_data, void *private_data)
{
	struct nfs_cb_data *data = private_data;
	struct nfs_context *nfs = data->nfs;
	SETATTR3res *res;

	if (status == RPC_STATUS_ERROR) {
		data->cb(-EFAULT, nfs, command_data, data->private_data);
		free_nfs_cb_data(data);
		return;
	}
	if (status == RPC_STATUS_CANCEL) {
		data->cb(-EINTR, nfs, "Command was cancelled", data->private_data);
		free_nfs_cb_data(data);
		return;
	}

	res = command_data;
	if (res->status != NFS3_OK) {
		rpc_set_error(nfs->rpc, "NFS: Setattr failed with %s(%d)", nfsstat3_to_str(res->status), nfsstat3_to_errno(res->status));
		data->cb(nfsstat3_to_errno(res->status), nfs, rpc_get_error(nfs->rpc), data->private_data);
		free_nfs_cb_data(data);
		return;
	}

	data->cb(0, nfs, NULL, data->private_data);
	free_nfs_cb_data(data);
}

int nfs_ftruncate_async(struct nfs_context *nfs, struct nfsfh *nfsfh, uint64_t length, nfs_cb cb, void *private_data)
{
	struct nfs_cb_data *data;
	SETATTR3args args;

	data = malloc(sizeof(struct nfs_cb_data));
	if (data == NULL) {
		rpc_set_error(nfs->rpc, "out of memory: failed to allocate nfs_cb_data structure");
		return -1;
	}
	memset(data, 0, sizeof(struct nfs_cb_data));
	data->nfs          = nfs;
	data->cb           = cb;
	data->private_data = private_data;

	memset(&args, 0, sizeof(SETATTR3args));
	args.object.data.data_len = nfsfh->fh.data.data_len;
	args.object.data.data_val = nfsfh->fh.data.data_val;
	args.new_attributes.size.set_it = 1;
	args.new_attributes.size.set_size3_u.size = length;

	if (rpc_nfs_setattr_async(nfs->rpc, nfs_ftruncate_cb, &args, data) != 0) {
		rpc_set_error(nfs->rpc, "RPC error: Failed to send SETATTR call for %s", data->path);
		data->cb(-ENOMEM, nfs, rpc_get_error(nfs->rpc), data->private_data);
		free_nfs_cb_data(data);
		return -1;
	}
	return 0;
}


/*
 * Async truncate()
 */
static int nfs_truncate_continue_internal(struct nfs_context *nfs, struct nfs_cb_data *data)
{
	uint64_t offset = data->continue_int;
	struct nfsfh nfsfh;

	nfsfh.fh.data.data_val = data->fh.data.data_val;
	nfsfh.fh.data.data_len = data->fh.data.data_len;

	if (nfs_ftruncate_async(nfs, &nfsfh, offset, data->cb, data->private_data) != 0) {
		rpc_set_error(nfs->rpc, "RPC error: Failed to send SETATTR call for %s", data->path);
		data->cb(-ENOMEM, nfs, rpc_get_error(nfs->rpc), data->private_data);
		free_nfs_cb_data(data);
		return -1;
	}
	free_nfs_cb_data(data);
	return 0;
}

int nfs_truncate_async(struct nfs_context *nfs, const char *path, uint64_t length, nfs_cb cb, void *private_data)
{
	uint64_t offset;

	offset = length;

	if (nfs_lookuppath_async(nfs, path, cb, private_data, nfs_truncate_continue_internal, NULL, NULL, offset) != 0) {
		rpc_set_error(nfs->rpc, "Out of memory: failed to start parsing the path components");
		return -1;
	}

	return 0;
}




/*
 * Async mkdir()
 */
static void nfs_mkdir_cb(struct rpc_context *rpc _U_, int status, void *command_data, void *private_data)
{
	MKDIR3res *res;
	struct nfs_cb_data *data = private_data;
	struct nfs_context *nfs = data->nfs;
	char *str = data->continue_data;
	
	str = &str[strlen(str) + 1];

	if (status == RPC_STATUS_ERROR) {
		data->cb(-EFAULT, nfs, command_data, data->private_data);
		free_nfs_cb_data(data);
		return;
	}
	if (status == RPC_STATUS_CANCEL) {
		data->cb(-EINTR, nfs, "Command was cancelled", data->private_data);
		free_nfs_cb_data(data);
		return;
	}

	res = command_data;
	if (res->status != NFS3_OK) {
		rpc_set_error(nfs->rpc, "NFS: MKDIR of %s/%s failed with %s(%d)", data->saved_path, str, nfsstat3_to_str(res->status), nfsstat3_to_errno(res->status));
		data->cb(nfsstat3_to_errno(res->status), nfs, rpc_get_error(nfs->rpc), data->private_data);
		free_nfs_cb_data(data);
		return;
	}

	data->cb(0, nfs, NULL, data->private_data);
	free_nfs_cb_data(data);
}

static int nfs_mkdir_continue_internal(struct nfs_context *nfs, struct nfs_cb_data *data)
{
	char *str = data->continue_data;
	MKDIR3args args;

	str = &str[strlen(str) + 1];

	memset(&args, 0, sizeof(MKDIR3args));
	args.where.dir.data.data_len = data->fh.data.data_len;
	args.where.dir.data.data_val = data->fh.data.data_val;
	args.where.name = str;
	args.attributes.mode.set_it = 1;
	args.attributes.mode.set_mode3_u.mode = 0755;

	if (rpc_nfs_mkdir_async(nfs->rpc, nfs_mkdir_cb, &args, data) != 0) {
		rpc_set_error(nfs->rpc, "RPC error: Failed to send MKDIR call for %s", data->path);
		data->cb(-ENOMEM, nfs, rpc_get_error(nfs->rpc), data->private_data);
		free_nfs_cb_data(data);
		return -1;
	}
	return 0;
}

int nfs_mkdir_async(struct nfs_context *nfs, const char *path, nfs_cb cb, void *private_data)
{
	char *new_path;
	char *ptr;

	new_path = strdup(path);
	if (new_path == NULL) {
		rpc_set_error(nfs->rpc, "Out of memory, failed to allocate mode buffer for path");
		return -1;
	}

	ptr = strrchr(new_path, '/');
	if (ptr == NULL) {
		rpc_set_error(nfs->rpc, "Invalid path %s", path);
		return -1;
	}
	*ptr = 0;

	/* new_path now points to the parent directory,  and beyond the nul terminateor is the new directory to create */
	if (nfs_lookuppath_async(nfs, new_path, cb, private_data, nfs_mkdir_continue_internal, new_path, free, 0) != 0) {
		rpc_set_error(nfs->rpc, "Out of memory: failed to start parsing the path component");
		return -1;
	}

	return 0;
}





/*
 * Async rmdir()
 */
static void nfs_rmdir_cb(struct rpc_context *rpc _U_, int status, void *command_data, void *private_data)
{
	RMDIR3res *res;
	struct nfs_cb_data *data = private_data;
	struct nfs_context *nfs = data->nfs;
	char *str = data->continue_data;
	
	str = &str[strlen(str) + 1];

	if (status == RPC_STATUS_ERROR) {
		data->cb(-EFAULT, nfs, command_data, data->private_data);
		free_nfs_cb_data(data);
		return;
	}
	if (status == RPC_STATUS_CANCEL) {
		data->cb(-EINTR, nfs, "Command was cancelled", data->private_data);
		free_nfs_cb_data(data);
		return;
	}

	res = command_data;
	if (res->status != NFS3_OK) {
		rpc_set_error(nfs->rpc, "NFS: RMDIR of %s/%s failed with %s(%d)", data->saved_path, str, nfsstat3_to_str(res->status), nfsstat3_to_errno(res->status));
		data->cb(nfsstat3_to_errno(res->status), nfs, rpc_get_error(nfs->rpc), data->private_data);
		free_nfs_cb_data(data);
		return;
	}

	data->cb(0, nfs, NULL, data->private_data);
	free_nfs_cb_data(data);
}

static int nfs_rmdir_continue_internal(struct nfs_context *nfs, struct nfs_cb_data *data)
{
	char *str = data->continue_data;
	
	str = &str[strlen(str) + 1];

	if (rpc_nfs_rmdir_async(nfs->rpc, nfs_rmdir_cb, &data->fh, str, data) != 0) {
		rpc_set_error(nfs->rpc, "RPC error: Failed to send RMDIR call for %s", data->path);
		data->cb(-ENOMEM, nfs, rpc_get_error(nfs->rpc), data->private_data);
		free_nfs_cb_data(data);
		return -1;
	}
	return 0;
}

int nfs_rmdir_async(struct nfs_context *nfs, const char *path, nfs_cb cb, void *private_data)
{
	char *new_path;
	char *ptr;

	new_path = strdup(path);
	if (new_path == NULL) {
		rpc_set_error(nfs->rpc, "Out of memory, failed to allocate mode buffer for path");
		return -1;
	}

	ptr = strrchr(new_path, '/');
	if (ptr == NULL) {
		rpc_set_error(nfs->rpc, "Invalid path %s", path);
		return -1;
	}
	*ptr = 0;

	/* new_path now points to the parent directory,  and beyond the nul terminateor is the new directory to create */
	if (nfs_lookuppath_async(nfs, new_path, cb, private_data, nfs_rmdir_continue_internal, new_path, free, 0) != 0) {
		rpc_set_error(nfs->rpc, "Out of memory: failed to start parsing the path components");
		return -1;
	}

	return 0;
}




/*
 * Async creat()
 */
static void nfs_create_2_cb(struct rpc_context *rpc _U_, int status, void *command_data, void *private_data)
{
	LOOKUP3res *res;
	struct nfs_cb_data *data = private_data;
	struct nfs_context *nfs = data->nfs;
	struct nfsfh *nfsfh;
	char *str = data->continue_data;

	if (status == RPC_STATUS_ERROR) {
		data->cb(-EFAULT, nfs, command_data, data->private_data);
		free_nfs_cb_data(data);
		return;
	}
	if (status == RPC_STATUS_CANCEL) {
		data->cb(-EINTR, nfs, "Command was cancelled", data->private_data);
		free_nfs_cb_data(data);
		return;
	}

	str = &str[strlen(str) + 1];
	res = command_data;
	if (res->status != NFS3_OK) {
		rpc_set_error(nfs->rpc, "NFS: CREATE of %s/%s failed with %s(%d)", data->saved_path, str, nfsstat3_to_str(res->status), nfsstat3_to_errno(res->status));
		data->cb(nfsstat3_to_errno(res->status), nfs, rpc_get_error(nfs->rpc), data->private_data);

		return;
	}

	nfsfh = malloc(sizeof(struct nfsfh));
	if (nfsfh == NULL) {
		rpc_set_error(nfs->rpc, "NFS: Failed to allocate nfsfh structure");
		data->cb(-ENOMEM, nfs, rpc_get_error(nfs->rpc), data->private_data);
		free_nfs_cb_data(data);
		return;
	}
	memset(nfsfh, 0, sizeof(struct nfsfh));

	/* steal the filehandle */
	nfsfh->fh.data.data_len = data->fh.data.data_len;
	nfsfh->fh.data.data_val = data->fh.data.data_val;
	data->fh.data.data_val = NULL;

	data->cb(0, nfs, nfsfh, data->private_data);
	free_nfs_cb_data(data);
}



static void nfs_creat_1_cb(struct rpc_context *rpc _U_, int status, void *command_data, void *private_data)
{
	CREATE3res *res;
	struct nfs_cb_data *data = private_data;
	struct nfs_context *nfs = data->nfs;
	char *str = data->continue_data;

	if (status == RPC_STATUS_ERROR) {
		data->cb(-EFAULT, nfs, command_data, data->private_data);
		free_nfs_cb_data(data);
		return;
	}
	if (status == RPC_STATUS_CANCEL) {
		data->cb(-EINTR, nfs, "Command was cancelled", data->private_data);
		free_nfs_cb_data(data);
		return;
	}

	str = &str[strlen(str) + 1];
	res = command_data;
	if (res->status != NFS3_OK) {
		rpc_set_error(nfs->rpc, "NFS: CREATE of %s/%s failed with %s(%d)", data->saved_path, str, nfsstat3_to_str(res->status), nfsstat3_to_errno(res->status));
		data->cb(nfsstat3_to_errno(res->status), nfs, rpc_get_error(nfs->rpc), data->private_data);

		return;
	}

	if (rpc_nfs_lookup_async(nfs->rpc, nfs_create_2_cb, &data->fh, str, data) != 0) {
		rpc_set_error(nfs->rpc, "RPC error: Failed to send lookup call for %s/%s", data->saved_path, str);
		data->cb(-ENOMEM, nfs, rpc_get_error(nfs->rpc), data->private_data);
		free_nfs_cb_data(data);
		return;
	}
	return;
}

static int nfs_creat_continue_internal(struct nfs_context *nfs, struct nfs_cb_data *data)
{
	char *str = data->continue_data;
	CREATE3args args;

	str = &str[strlen(str) + 1];


	memset(&args, 0, sizeof(CREATE3args));
	args.where.dir.data.data_len = data->fh.data.data_len;
	args.where.dir.data.data_val = data->fh.data.data_val;
	args.where.name = str;
	args.how.mode = UNCHECKED;
	args.how.createhow3_u.obj_attributes.mode.set_it = 1;
	args.how.createhow3_u.obj_attributes.mode.set_mode3_u.mode = data->continue_int;

	if (rpc_nfs_create_async(nfs->rpc, nfs_creat_1_cb, &args, data) != 0) {
		rpc_set_error(nfs->rpc, "RPC error: Failed to send CREATE call for %s/%s", data->path, str);
		data->cb(-ENOMEM, nfs, rpc_get_error(nfs->rpc), data->private_data);
		free_nfs_cb_data(data);
		return -1;
	}
	return 0;
}

int nfs_creat_async(struct nfs_context *nfs, const char *path, int mode, nfs_cb cb, void *private_data)
{
	char *new_path;
	char *ptr;

	new_path = strdup(path);
	if (new_path == NULL) {
		rpc_set_error(nfs->rpc, "Out of memory, failed to allocate mode buffer for path");
		return -1;
	}

	ptr = strrchr(new_path, '/');
	if (ptr == NULL) {
		rpc_set_error(nfs->rpc, "Invalid path %s", path);
		return -1;
	}
	*ptr = 0;

	/* new_path now points to the parent directory,  and beyond the nul terminateor is the new directory to create */
	if (nfs_lookuppath_async(nfs, new_path, cb, private_data, nfs_creat_continue_internal, new_path, free, mode) != 0) {
		rpc_set_error(nfs->rpc, "Out of memory: failed to start parsing the path components");
		return -1;
	}

	return 0;
}




/*
 * Async unlink()
 */
static void nfs_unlink_cb(struct rpc_context *rpc _U_, int status, void *command_data, void *private_data)
{
	REMOVE3res *res;
	struct nfs_cb_data *data = private_data;
	struct nfs_context *nfs = data->nfs;
	char *str = data->continue_data;
	
	str = &str[strlen(str) + 1];

	if (status == RPC_STATUS_ERROR) {
		data->cb(-EFAULT, nfs, command_data, data->private_data);
		free_nfs_cb_data(data);
		return;
	}
	if (status == RPC_STATUS_CANCEL) {
		data->cb(-EINTR, nfs, "Command was cancelled", data->private_data);
		free_nfs_cb_data(data);
		return;
	}

	res = command_data;
	if (res->status != NFS3_OK) {
		rpc_set_error(nfs->rpc, "NFS: REMOVE of %s/%s failed with %s(%d)", data->saved_path, str, nfsstat3_to_str(res->status), nfsstat3_to_errno(res->status));
		data->cb(nfsstat3_to_errno(res->status), nfs, rpc_get_error(nfs->rpc), data->private_data);
		free_nfs_cb_data(data);
		return;
	}

	data->cb(0, nfs, NULL, data->private_data);
	free_nfs_cb_data(data);
}

static int nfs_unlink_continue_internal(struct nfs_context *nfs, struct nfs_cb_data *data)
{
	char *str = data->continue_data;
	
	str = &str[strlen(str) + 1];

	if (rpc_nfs_remove_async(nfs->rpc, nfs_unlink_cb, &data->fh, str, data) != 0) {
		rpc_set_error(nfs->rpc, "RPC error: Failed to send REMOVE call for %s", data->path);
		data->cb(-ENOMEM, nfs, rpc_get_error(nfs->rpc), data->private_data);
		free_nfs_cb_data(data);
		return -1;
	}
	return 0;
}

int nfs_unlink_async(struct nfs_context *nfs, const char *path, nfs_cb cb, void *private_data)
{
	char *new_path;
	char *ptr;

	new_path = strdup(path);
	if (new_path == NULL) {
		rpc_set_error(nfs->rpc, "Out of memory, failed to allocate mode buffer for path");
		return -1;
	}

	ptr = strrchr(new_path, '/');
	if (ptr == NULL) {
		rpc_set_error(nfs->rpc, "Invalid path %s", path);
		return -1;
	}
	*ptr = 0;

	/* new_path now points to the parent directory,  and beyond the nul terminateor is the new directory to create */
	if (nfs_lookuppath_async(nfs, new_path, cb, private_data, nfs_unlink_continue_internal, new_path, free, 0) != 0) {
		rpc_set_error(nfs->rpc, "Out of memory: failed to start parsing the path components");
		return -1;
	}

	return 0;
}


/*
 * Async mknod()
 */
struct mknod_cb_data {
       char *path;
       int mode;
       int major;
       int minor;
};

static void free_mknod_cb_data(void *ptr)
{
	struct mknod_cb_data *data = ptr;

	free(data->path);
	free(data);
}

static void nfs_mknod_cb(struct rpc_context *rpc _U_, int status, void *command_data, void *private_data)
{
	MKNOD3res *res;
	struct nfs_cb_data *data = private_data;
	struct nfs_context *nfs = data->nfs;
	char *str = data->continue_data;
	
	str = &str[strlen(str) + 1];

	if (status == RPC_STATUS_ERROR) {
		data->cb(-EFAULT, nfs, command_data, data->private_data);
		free_nfs_cb_data(data);
		return;
	}
	if (status == RPC_STATUS_CANCEL) {
		data->cb(-EINTR, nfs, "Command was cancelled", data->private_data);
		free_nfs_cb_data(data);
		return;
	}

	res = command_data;
	if (res->status != NFS3_OK) {
		rpc_set_error(nfs->rpc, "NFS: MKNOD of %s/%s failed with %s(%d)", data->saved_path, str, nfsstat3_to_str(res->status), nfsstat3_to_errno(res->status));
		data->cb(nfsstat3_to_errno(res->status), nfs, rpc_get_error(nfs->rpc), data->private_data);
		free_nfs_cb_data(data);
		return;
	}

	data->cb(0, nfs, NULL, data->private_data);
	free_nfs_cb_data(data);
}

static int nfs_mknod_continue_internal(struct nfs_context *nfs, struct nfs_cb_data *data)
{
	struct mknod_cb_data *cb_data = data->continue_data;
	char *str = cb_data->path;
	
	str = &str[strlen(str) + 1];

	if (rpc_nfs_mknod_async(nfs->rpc, nfs_mknod_cb, &data->fh, str, cb_data->mode, cb_data->major, cb_data->minor, data) != 0) {
		data->cb(-ENOMEM, nfs, rpc_get_error(nfs->rpc), data->private_data);
		free_nfs_cb_data(data);
		return -1;
	}
	return 0;
}

int nfs_mknod_async(struct nfs_context *nfs, const char *path, int mode, int dev, nfs_cb cb, void *private_data)
{
	char *ptr;
	struct mknod_cb_data *cb_data;

	cb_data = malloc(sizeof(struct mknod_cb_data));
	if (cb_data == NULL) {
		rpc_set_error(nfs->rpc, "Out of memory, failed to allocate mode buffer for cb data");
		return -1;
	}

	cb_data->path = strdup(path);
	if (cb_data->path == NULL) {
		rpc_set_error(nfs->rpc, "Out of memory, failed to allocate mode buffer for path");
		free(cb_data);		
		return -1;
	}

	ptr = strrchr(cb_data->path, '/');
	if (ptr == NULL) {
		rpc_set_error(nfs->rpc, "Invalid path %s", path);
		return -1;
	}
	*ptr = 0;

	cb_data->mode = mode;
	cb_data->major = major(dev);
	cb_data->minor = minor(dev);

	/* data->path now points to the parent directory,  and beyond the nul terminateor is the new directory to create */
	if (nfs_lookuppath_async(nfs, cb_data->path, cb, private_data, nfs_mknod_continue_internal, cb_data, free_mknod_cb_data, 0) != 0) {
		rpc_set_error(nfs->rpc, "Out of memory: failed to start parsing the path components");
		free_mknod_cb_data(cb_data);
		return -1;
	}

	return 0;
}

/*
 * Async opendir()
 */

/* ReadDirPlus Emulation Callback data */
struct rdpe_cb_data {
	int getattrcount;
	int status;
	struct nfs_cb_data *data;
};

/* ReadDirPlus Emulation LOOKUP Callback data */
struct rdpe_lookup_cb_data {
	struct rdpe_cb_data *rdpe_cb_data;
	struct nfsdirent *nfsdirent;
};

/* Workaround for servers lacking READDIRPLUS, use READDIR instead and a GETATTR-loop */
static void nfs_opendir3_cb(struct rpc_context *rpc _U_, int status, void *command_data, void *private_data)
{
	LOOKUP3res *res = command_data;
	struct rdpe_lookup_cb_data *rdpe_lookup_cb_data = private_data;
	struct rdpe_cb_data *rdpe_cb_data = rdpe_lookup_cb_data->rdpe_cb_data;
	struct nfs_cb_data *data = rdpe_cb_data->data;
	struct nfsdir *nfsdir = data->continue_data;
	struct nfs_context *nfs = data->nfs;
	struct nfsdirent *nfsdirent = rdpe_lookup_cb_data->nfsdirent;

	free(rdpe_lookup_cb_data);

	rdpe_cb_data->getattrcount--;

	if (status == RPC_STATUS_ERROR) {
		rdpe_cb_data->status = RPC_STATUS_ERROR;
	}
	if (status == RPC_STATUS_CANCEL) {
		rdpe_cb_data->status = RPC_STATUS_CANCEL;
	}
	if (status == RPC_STATUS_SUCCESS && res->status != NFS3_OK) {
		rdpe_cb_data->status = RPC_STATUS_ERROR;
	}
	if (status == RPC_STATUS_SUCCESS && res->status == NFS3_OK) {
		if (res->LOOKUP3res_u.resok.obj_attributes.attributes_follow) {
			fattr3 *attributes = &res->LOOKUP3res_u.resok.obj_attributes.post_op_attr_u.attributes;

			nfsdirent->type = attributes->type;
			nfsdirent->mode = attributes->mode;
			nfsdirent->size = attributes->size;

			nfsdirent->atime.tv_sec  = attributes->atime.seconds;
			nfsdirent->atime.tv_usec = attributes->atime.nseconds/1000;
			nfsdirent->mtime.tv_sec  = attributes->mtime.seconds;
			nfsdirent->mtime.tv_usec = attributes->mtime.nseconds/1000;
			nfsdirent->ctime.tv_sec  = attributes->ctime.seconds;
			nfsdirent->ctime.tv_usec = attributes->ctime.nseconds/1000;
		}
	}

	if (rdpe_cb_data->getattrcount == 0) {
		if (rdpe_cb_data->status != RPC_STATUS_SUCCESS) {
			data->cb(-ENOMEM, nfs, rpc_get_error(nfs->rpc), data->private_data);
			nfs_free_nfsdir(nfsdir);
		} else {
			data->cb(0, nfs, nfsdir, data->private_data);
		}
		free(rdpe_cb_data);

		data->continue_data = NULL;
		free_nfs_cb_data(data);
	}
}

static void nfs_opendir2_cb(struct rpc_context *rpc _U_, int status, void *command_data, void *private_data)
{
	READDIR3res *res = command_data;
	struct nfs_cb_data *data = private_data;
	struct nfs_context *nfs = data->nfs;
	struct nfsdir *nfsdir = data->continue_data;
	struct nfsdirent *nfsdirent;
	struct entry3 *entry;
	uint64_t cookie;
	struct rdpe_cb_data *rdpe_cb_data;
	
	if (status == RPC_STATUS_ERROR) {
		data->cb(-EFAULT, nfs, command_data, data->private_data);
		nfs_free_nfsdir(nfsdir);
		data->continue_data = NULL;
		free_nfs_cb_data(data);
		return;
	}

	if (status == RPC_STATUS_CANCEL) {
		data->cb(-EINTR, nfs, "Command was cancelled", data->private_data);
		nfs_free_nfsdir(nfsdir);
		data->continue_data = NULL;
		free_nfs_cb_data(data);
		return;
	}

	if (res->status != NFS3_OK) {
		rpc_set_error(nfs->rpc, "NFS: READDIR of %s failed with %s(%d)", data->saved_path, nfsstat3_to_str(res->status), nfsstat3_to_errno(res->status));
		data->cb(nfsstat3_to_errno(res->status), nfs, rpc_get_error(nfs->rpc), data->private_data);
		nfs_free_nfsdir(nfsdir);
		data->continue_data = NULL;
		free_nfs_cb_data(data);
		return;
	}

	entry =res->READDIR3res_u.resok.reply.entries;
	while (entry != NULL) {
		nfsdirent = malloc(sizeof(struct nfsdirent));
		if (nfsdirent == NULL) {
			data->cb(-ENOMEM, nfs, "Failed to allocate dirent", data->private_data);
			nfs_free_nfsdir(nfsdir);
			data->continue_data = NULL;
			free_nfs_cb_data(data);
			return;
		}
		memset(nfsdirent, 0, sizeof(struct nfsdirent));
		nfsdirent->name = strdup(entry->name);
		if (nfsdirent->name == NULL) {
			data->cb(-ENOMEM, nfs, "Failed to allocate dirent->name", data->private_data);
			nfs_free_nfsdir(nfsdir);
			data->continue_data = NULL;
			free_nfs_cb_data(data);
			return;
		}
		nfsdirent->inode = entry->fileid;

		nfsdirent->next  = nfsdir->entries;
		nfsdir->entries  = nfsdirent;

		cookie = entry->cookie;
		entry  = entry->nextentry;
	}

	if (res->READDIR3res_u.resok.reply.eof == 0) {
	     	if (rpc_nfs_readdir_async(nfs->rpc, nfs_opendir2_cb, &data->fh, cookie, res->READDIR3res_u.resok.cookieverf, 8192, data) != 0) {
			rpc_set_error(nfs->rpc, "RPC error: Failed to send READDIR call for %s", data->path);
			data->cb(-ENOMEM, nfs, rpc_get_error(nfs->rpc), data->private_data);
			nfs_free_nfsdir(nfsdir);
			data->continue_data = NULL;
			free_nfs_cb_data(data);
			return;
		}
		return;
	}

	/* steal the dirhandle */
	nfsdir->current = nfsdir->entries;

	rdpe_cb_data = malloc(sizeof(struct rdpe_cb_data));
	rdpe_cb_data->getattrcount = 0;
	rdpe_cb_data->status = RPC_STATUS_SUCCESS;
	rdpe_cb_data->data = data;
	for (nfsdirent = nfsdir->entries; nfsdirent; nfsdirent = nfsdirent->next) {
		struct rdpe_lookup_cb_data *rdpe_lookup_cb_data;

		rdpe_lookup_cb_data = malloc(sizeof(struct rdpe_lookup_cb_data));
		rdpe_lookup_cb_data->rdpe_cb_data = rdpe_cb_data;
		rdpe_lookup_cb_data->nfsdirent = nfsdirent;

		if (rpc_nfs_lookup_async(nfs->rpc, nfs_opendir3_cb, &data->fh, nfsdirent->name, rdpe_lookup_cb_data) != 0) {
			rpc_set_error(nfs->rpc, "RPC error: Failed to send READDIR LOOKUP call");

			/* if we have already commands in flight, we cant just stop, we have to wait for the
		 	 * commands in flight to complete
		 	 */
			if (rdpe_cb_data->getattrcount > 0) {
				rdpe_cb_data->status = RPC_STATUS_ERROR;
				free(rdpe_lookup_cb_data);
				return;
			}

			data->cb(-ENOMEM, nfs, rpc_get_error(nfs->rpc), data->private_data);
			nfs_free_nfsdir(nfsdir);
			data->continue_data = NULL;
			free_nfs_cb_data(data);
			free(rdpe_lookup_cb_data);
			free(rdpe_cb_data);
			return;
		}
		rdpe_cb_data->getattrcount++;
	}
}


static void nfs_opendir_cb(struct rpc_context *rpc _U_, int status, void *command_data, void *private_data)
{
	READDIRPLUS3res *res = command_data;
	struct nfs_cb_data *data = private_data;
	struct nfs_context *nfs = data->nfs;
	struct nfsdir *nfsdir = data->continue_data;
	struct entryplus3 *entry;
	uint64_t cookie;
	

	if (status == RPC_STATUS_ERROR || (status == RPC_STATUS_SUCCESS && res->status == NFS3ERR_NOTSUPP) ){
		cookieverf3 cv;

		if (rpc_nfs_readdir_async(nfs->rpc, nfs_opendir2_cb, &data->fh, 0, (char *)&cv, 8192, data) != 0) {
			rpc_set_error(nfs->rpc, "RPC error: Failed to send READDIR call for %s", data->path);
			data->cb(-ENOMEM, nfs, rpc_get_error(nfs->rpc), data->private_data);
			nfs_free_nfsdir(nfsdir);
			data->continue_data = NULL;
			free_nfs_cb_data(data);
			return;
		}
		return;
	}

	if (status == RPC_STATUS_CANCEL) {
		data->cb(-EINTR, nfs, "Command was cancelled", data->private_data);
		nfs_free_nfsdir(nfsdir);
		data->continue_data = NULL;
		free_nfs_cb_data(data);
		return;
	}

	if (res->status != NFS3_OK) {
		rpc_set_error(nfs->rpc, "NFS: READDIRPLUS of %s failed with %s(%d)", data->saved_path, nfsstat3_to_str(res->status), nfsstat3_to_errno(res->status));
		data->cb(nfsstat3_to_errno(res->status), nfs, rpc_get_error(nfs->rpc), data->private_data);
		nfs_free_nfsdir(nfsdir);
		data->continue_data = NULL;
		free_nfs_cb_data(data);
		return;
	}

	entry =res->READDIRPLUS3res_u.resok.reply.entries;
	while (entry != NULL) {
		struct nfsdirent *nfsdirent;

		nfsdirent = malloc(sizeof(struct nfsdirent));
		if (nfsdirent == NULL) {
			data->cb(-ENOMEM, nfs, "Failed to allocate dirent", data->private_data);
			nfs_free_nfsdir(nfsdir);
			data->continue_data = NULL;
			free_nfs_cb_data(data);
			return;
		}
		memset(nfsdirent, 0, sizeof(struct nfsdirent));
		nfsdirent->name = strdup(entry->name);
		if (nfsdirent->name == NULL) {
			data->cb(-ENOMEM, nfs, "Failed to allocate dirent->name", data->private_data);
			nfs_free_nfsdir(nfsdir);
			data->continue_data = NULL;
			free_nfs_cb_data(data);
			return;
		}
		nfsdirent->inode = entry->fileid;
		if (entry->name_attributes.attributes_follow) {
			nfsdirent->type = entry->name_attributes.post_op_attr_u.attributes.type;
			nfsdirent->mode = entry->name_attributes.post_op_attr_u.attributes.mode;
			nfsdirent->size = entry->name_attributes.post_op_attr_u.attributes.size;

			nfsdirent->atime.tv_sec  = entry->name_attributes.post_op_attr_u.attributes.atime.seconds;
			nfsdirent->atime.tv_usec = entry->name_attributes.post_op_attr_u.attributes.atime.nseconds/1000;
			nfsdirent->mtime.tv_sec  = entry->name_attributes.post_op_attr_u.attributes.mtime.seconds;
			nfsdirent->mtime.tv_usec = entry->name_attributes.post_op_attr_u.attributes.mtime.nseconds/1000;
			nfsdirent->ctime.tv_sec  = entry->name_attributes.post_op_attr_u.attributes.ctime.seconds;
			nfsdirent->ctime.tv_usec = entry->name_attributes.post_op_attr_u.attributes.ctime.nseconds/1000;
		}

		nfsdirent->next  = nfsdir->entries;
		nfsdir->entries  = nfsdirent;

		cookie = entry->cookie;
		entry  = entry->nextentry;
	}

	if (res->READDIRPLUS3res_u.resok.reply.eof == 0) {
	     	if (rpc_nfs_readdirplus_async(nfs->rpc, nfs_opendir_cb, &data->fh, cookie, res->READDIRPLUS3res_u.resok.cookieverf, 8192, data) != 0) {
			rpc_set_error(nfs->rpc, "RPC error: Failed to send READDIRPLUS call for %s", data->path);
			data->cb(-ENOMEM, nfs, rpc_get_error(nfs->rpc), data->private_data);
			nfs_free_nfsdir(nfsdir);
			data->continue_data = NULL;
			free_nfs_cb_data(data);
			return;
		}
		return;
	}

	/* steal the dirhandle */
	data->continue_data = NULL;
	nfsdir->current = nfsdir->entries;

	data->cb(0, nfs, nfsdir, data->private_data);
	free_nfs_cb_data(data);
}

static int nfs_opendir_continue_internal(struct nfs_context *nfs, struct nfs_cb_data *data)
{
	cookieverf3 cv;

	memset(cv, 0, sizeof(cookieverf3));
	if (rpc_nfs_readdirplus_async(nfs->rpc, nfs_opendir_cb, &data->fh, 0, (char *)&cv, 8192, data) != 0) {
		rpc_set_error(nfs->rpc, "RPC error: Failed to send READDIRPLUS call for %s", data->path);
		data->cb(-ENOMEM, nfs, rpc_get_error(nfs->rpc), data->private_data);
		free_nfs_cb_data(data);
		return -1;
	}
	return 0;
}

int nfs_opendir_async(struct nfs_context *nfs, const char *path, nfs_cb cb, void *private_data)
{
	struct nfsdir *nfsdir;

	nfsdir = malloc(sizeof(struct nfsdir));
	if (nfsdir == NULL) {
		rpc_set_error(nfs->rpc, "failed to allocate buffer for nfsdir");
		return -1;
	}
	memset(nfsdir, 0, sizeof(struct nfsdir));

	if (nfs_lookuppath_async(nfs, path, cb, private_data, nfs_opendir_continue_internal, nfsdir, free, 0) != 0) {
		rpc_set_error(nfs->rpc, "Out of memory: failed to start parsing the path components");
		return -1;
	}

	return 0;
}


struct nfsdirent *nfs_readdir(struct nfs_context *nfs _U_, struct nfsdir *nfsdir)
{
	struct nfsdirent *nfsdirent = nfsdir->current;

	if (nfsdir->current != NULL) {
		nfsdir->current = nfsdir->current->next;
	}
	return nfsdirent;
}


void nfs_closedir(struct nfs_context *nfs _U_, struct nfsdir *nfsdir)
{
	nfs_free_nfsdir(nfsdir);
}







/*
 * Async lseek()
 */
struct lseek_cb_data {
       struct nfs_context *nfs;
       struct nfsfh *nfsfh;
       uint64_t offset;
       nfs_cb cb;
       void *private_data;
};

static void nfs_lseek_1_cb(struct rpc_context *rpc _U_, int status, void *command_data, void *private_data)
{
	GETATTR3res *res;
	struct lseek_cb_data *data = private_data;
	struct nfs_context *nfs = data->nfs;

	if (status == RPC_STATUS_ERROR) {
		data->cb(-EFAULT, nfs, command_data, data->private_data);
		free(data);
		return;
	}
	if (status == RPC_STATUS_CANCEL) {
		data->cb(-EINTR, nfs, "Command was cancelled", data->private_data);
		free(data);
		return;
	}

	res = command_data;
	if (res->status != NFS3_OK) {
		rpc_set_error(nfs->rpc, "NFS: GETATTR failed with %s(%d)", nfsstat3_to_str(res->status), nfsstat3_to_errno(res->status));
		data->cb(nfsstat3_to_errno(res->status), nfs, rpc_get_error(nfs->rpc), data->private_data);
		free(data);
		return;
	}

	data->nfsfh->offset = data->offset + res->GETATTR3res_u.resok.obj_attributes.size;
	data->cb(0, nfs, &data->nfsfh->offset, data->private_data);
	free(data);
}

int nfs_lseek_async(struct nfs_context *nfs, struct nfsfh *nfsfh, uint64_t offset, int whence, nfs_cb cb, void *private_data)
{
	struct lseek_cb_data *data;

	if (whence == SEEK_SET) {
		nfsfh->offset = offset;
		cb(0, nfs, &nfsfh->offset, private_data);
		return 0;
	}
	if (whence == SEEK_CUR) {
		nfsfh->offset += offset;
		cb(0, nfs, &nfsfh->offset, private_data);
		return 0;
	}

	data = malloc(sizeof(struct lseek_cb_data));
	if (data == NULL) {
		rpc_set_error(nfs->rpc, "Out Of Memory: Failed to malloc lseek cb data");
		return -1;
	}

	data->nfs          = nfs;
	data->nfsfh        = nfsfh;
	data->offset       = offset;
	data->cb           = cb;
	data->private_data = private_data;

	if (rpc_nfs_getattr_async(nfs->rpc, nfs_lseek_1_cb, &nfsfh->fh, data) != 0) {
		rpc_set_error(nfs->rpc, "RPC error: Failed to send LSEEK GETATTR call");
		free(data);
		return -1;
	}
	return 0;
}




/*
 * Async statvfs()
 */
static void nfs_statvfs_1_cb(struct rpc_context *rpc _U_, int status, void *command_data, void *private_data)
{
	FSSTAT3res *res;
	struct nfs_cb_data *data = private_data;
	struct nfs_context *nfs = data->nfs;
	struct statvfs svfs;

	if (status == RPC_STATUS_ERROR) {
		data->cb(-EFAULT, nfs, command_data, data->private_data);
		free_nfs_cb_data(data);
		return;
	}
	if (status == RPC_STATUS_CANCEL) {
		data->cb(-EINTR, nfs, "Command was cancelled", data->private_data);
		free_nfs_cb_data(data);
		return;
	}

	res = command_data;
	if (res->status != NFS3_OK) {
		rpc_set_error(nfs->rpc, "NFS: FSSTAT of %s failed with %s(%d)", data->saved_path, nfsstat3_to_str(res->status), nfsstat3_to_errno(res->status));
		data->cb(nfsstat3_to_errno(res->status), nfs, rpc_get_error(nfs->rpc), data->private_data);
		free_nfs_cb_data(data);
		return;
	}

	svfs.f_bsize   = 4096;
	svfs.f_frsize  = 4096;
	svfs.f_blocks  = res->FSSTAT3res_u.resok.tbytes/4096;
	svfs.f_bfree   = res->FSSTAT3res_u.resok.fbytes/4096;
	svfs.f_bavail  = res->FSSTAT3res_u.resok.abytes/4096;
	svfs.f_files   = res->FSSTAT3res_u.resok.tfiles;
	svfs.f_ffree   = res->FSSTAT3res_u.resok.ffiles;
	svfs.f_favail  = res->FSSTAT3res_u.resok.afiles;
	svfs.f_fsid    = 0;
	svfs.f_flag    = 0;
	svfs.f_namemax = 256;

	data->cb(0, nfs, &svfs, data->private_data);
	free_nfs_cb_data(data);
}

static int nfs_statvfs_continue_internal(struct nfs_context *nfs, struct nfs_cb_data *data)
{
	if (rpc_nfs_fsstat_async(nfs->rpc, nfs_statvfs_1_cb, &data->fh, data) != 0) {
		rpc_set_error(nfs->rpc, "RPC error: Failed to send FSSTAT call for %s", data->path);
		data->cb(-ENOMEM, nfs, rpc_get_error(nfs->rpc), data->private_data);
		free_nfs_cb_data(data);
		return -1;
	}
	return 0;
}

int nfs_statvfs_async(struct nfs_context *nfs, const char *path, nfs_cb cb, void *private_data)
{
	if (nfs_lookuppath_async(nfs, path, cb, private_data, nfs_statvfs_continue_internal, NULL, NULL, 0) != 0) {
		rpc_set_error(nfs->rpc, "Out of memory: failed to start parsing the path components");
		return -1;
	}

	return 0;
}




/*
 * Async readlink()
 */
static void nfs_readlink_1_cb(struct rpc_context *rpc _U_, int status, void *command_data, void *private_data)
{
	READLINK3res *res;
	struct nfs_cb_data *data = private_data;
	struct nfs_context *nfs = data->nfs;

	if (status == RPC_STATUS_ERROR) {
		data->cb(-EFAULT, nfs, command_data, data->private_data);
		free_nfs_cb_data(data);
		return;
	}
	if (status == RPC_STATUS_CANCEL) {
		data->cb(-EINTR, nfs, "Command was cancelled", data->private_data);
		free_nfs_cb_data(data);
		return;
	}

	res = command_data;
	if (res->status != NFS3_OK) {
		rpc_set_error(nfs->rpc, "NFS: READLINK of %s failed with %s(%d)", data->saved_path, nfsstat3_to_str(res->status), nfsstat3_to_errno(res->status));
		data->cb(nfsstat3_to_errno(res->status), nfs, rpc_get_error(nfs->rpc), data->private_data);
		free_nfs_cb_data(data);
		return;
	}

	
	data->cb(0, nfs, res->READLINK3res_u.resok.data, data->private_data);
	free_nfs_cb_data(data);
}

static int nfs_readlink_continue_internal(struct nfs_context *nfs, struct nfs_cb_data *data)
{
	READLINK3args args;

	args.symlink.data.data_len = data->fh.data.data_len; 
	args.symlink.data.data_val = data->fh.data.data_val; 

	if (rpc_nfs_readlink_async(nfs->rpc, nfs_readlink_1_cb, &args, data) != 0) {
		rpc_set_error(nfs->rpc, "RPC error: Failed to send READLINK call for %s", data->path);
		data->cb(-ENOMEM, nfs, rpc_get_error(nfs->rpc), data->private_data);
		free_nfs_cb_data(data);
		return -1;
	}
	return 0;
}

int nfs_readlink_async(struct nfs_context *nfs, const char *path, nfs_cb cb, void *private_data)
{
	if (nfs_lookuppath_async(nfs, path, cb, private_data, nfs_readlink_continue_internal, NULL, NULL, 0) != 0) {
		rpc_set_error(nfs->rpc, "Out of memory: failed to start parsing the path components");
		return -1;
	}

	return 0;
}




/*
 * Async chmod()
 */
static void nfs_chmod_cb(struct rpc_context *rpc _U_, int status, void *command_data, void *private_data)
{
	struct nfs_cb_data *data = private_data;
	struct nfs_context *nfs = data->nfs;
	SETATTR3res *res;

	if (status == RPC_STATUS_ERROR) {
		data->cb(-EFAULT, nfs, command_data, data->private_data);
		free_nfs_cb_data(data);
		return;
	}
	if (status == RPC_STATUS_CANCEL) {
		data->cb(-EINTR, nfs, "Command was cancelled", data->private_data);
		free_nfs_cb_data(data);
		return;
	}

	res = command_data;
	if (res->status != NFS3_OK) {
		rpc_set_error(nfs->rpc, "NFS: SETATTR failed with %s(%d)", nfsstat3_to_str(res->status), nfsstat3_to_errno(res->status));
		data->cb(nfsstat3_to_errno(res->status), nfs, rpc_get_error(nfs->rpc), data->private_data);
		free_nfs_cb_data(data);
		return;
	}

	data->cb(0, nfs, NULL, data->private_data);
	free_nfs_cb_data(data);
}

static int nfs_chmod_continue_internal(struct nfs_context *nfs, struct nfs_cb_data *data)
{
	SETATTR3args args;

	memset(&args, 0, sizeof(SETATTR3args));
	args.object.data.data_len = data->fh.data.data_len;
	args.object.data.data_val = data->fh.data.data_val;
	args.new_attributes.mode.set_it = 1;
	args.new_attributes.mode.set_mode3_u.mode = data->continue_int;

	if (rpc_nfs_setattr_async(nfs->rpc, nfs_chmod_cb, &args, data) != 0) {
		rpc_set_error(nfs->rpc, "RPC error: Failed to send SETATTR call for %s", data->path);
		data->cb(-ENOMEM, nfs, rpc_get_error(nfs->rpc), data->private_data);
		free_nfs_cb_data(data);
		return -1;
	}
	return 0;
}


int nfs_chmod_async(struct nfs_context *nfs, const char *path, int mode, nfs_cb cb, void *private_data)
{
	if (nfs_lookuppath_async(nfs, path, cb, private_data, nfs_chmod_continue_internal, NULL, NULL, mode) != 0) {
		rpc_set_error(nfs->rpc, "Out of memory: failed to start parsing the path components");
		return -1;
	}

	return 0;
}

/*
 * Async fchmod()
 */
int nfs_fchmod_async(struct nfs_context *nfs, struct nfsfh *nfsfh, int mode, nfs_cb cb, void *private_data)
{
	struct nfs_cb_data *data;

	data = malloc(sizeof(struct nfs_cb_data));
	if (data == NULL) {
		rpc_set_error(nfs->rpc, "out of memory. failed to allocate memory for nfs mount data");
		return -1;
	}
	memset(data, 0, sizeof(struct nfs_cb_data));
	data->nfs          = nfs;
	data->cb           = cb;
	data->private_data = private_data;
	data->continue_int = mode;
	data->fh.data.data_len = nfsfh->fh.data.data_len;
	data->fh.data.data_val = malloc(data->fh.data.data_len);
	if (data->fh.data.data_val == NULL) {
		rpc_set_error(nfs->rpc, "Out of memory: Failed to allocate fh");
		free_nfs_cb_data(data);
		return -1;
	}
	memcpy(data->fh.data.data_val, nfsfh->fh.data.data_val, data->fh.data.data_len);

	if (nfs_chmod_continue_internal(nfs, data) != 0) {
		free_nfs_cb_data(data);
		return -1;
	}

	return 0;
}



/*
 * Async chown()
 */
static void nfs_chown_cb(struct rpc_context *rpc _U_, int status, void *command_data, void *private_data)
{
	struct nfs_cb_data *data = private_data;
	struct nfs_context *nfs = data->nfs;
	SETATTR3res *res;

	if (status == RPC_STATUS_ERROR) {
		data->cb(-EFAULT, nfs, command_data, data->private_data);
		free_nfs_cb_data(data);
		return;
	}
	if (status == RPC_STATUS_CANCEL) {
		data->cb(-EINTR, nfs, "Command was cancelled", data->private_data);
		free_nfs_cb_data(data);
		return;
	}

	res = command_data;
	if (res->status != NFS3_OK) {
		rpc_set_error(nfs->rpc, "NFS: SETATTR failed with %s(%d)", nfsstat3_to_str(res->status), nfsstat3_to_errno(res->status));
		data->cb(nfsstat3_to_errno(res->status), nfs, rpc_get_error(nfs->rpc), data->private_data);
		free_nfs_cb_data(data);
		return;
	}

	data->cb(0, nfs, NULL, data->private_data);
	free_nfs_cb_data(data);
}

struct nfs_chown_data {
       uid_t uid;
       gid_t gid;
};

static int nfs_chown_continue_internal(struct nfs_context *nfs, struct nfs_cb_data *data)
{
	SETATTR3args args;
	struct nfs_chown_data *chown_data = data->continue_data;

	memset(&args, 0, sizeof(SETATTR3args));
	args.object.data.data_len = data->fh.data.data_len;
	args.object.data.data_val = data->fh.data.data_val;
	if (chown_data->uid != (uid_t)-1) {
		args.new_attributes.uid.set_it = 1;
		args.new_attributes.uid.set_uid3_u.uid = chown_data->uid;
	}
	if (chown_data->gid != (gid_t)-1) {
		args.new_attributes.gid.set_it = 1;
		args.new_attributes.gid.set_gid3_u.gid = chown_data->gid;
	}

	if (rpc_nfs_setattr_async(nfs->rpc, nfs_chown_cb, &args, data) != 0) {
		rpc_set_error(nfs->rpc, "RPC error: Failed to send SETATTR call for %s", data->path);
		data->cb(-ENOMEM, nfs, rpc_get_error(nfs->rpc), data->private_data);
		free_nfs_cb_data(data);
		return -1;
	}
	return 0;
}


int nfs_chown_async(struct nfs_context *nfs, const char *path, int uid, int gid, nfs_cb cb, void *private_data)
{
	struct nfs_chown_data *chown_data;

	chown_data = malloc(sizeof(struct nfs_chown_data));
	if (chown_data == NULL) {
		rpc_set_error(nfs->rpc, "Failed to allocate memory for chown data structure");
		return -1;
	}

	chown_data->uid = uid;
	chown_data->gid = gid;

	if (nfs_lookuppath_async(nfs, path, cb, private_data, nfs_chown_continue_internal, chown_data, free, 0) != 0) {
		rpc_set_error(nfs->rpc, "Out of memory: failed to start parsing the path components");
		return -1;
	}

	return 0;
}


/*
 * Async fchown()
 */
int nfs_fchown_async(struct nfs_context *nfs, struct nfsfh *nfsfh, int uid, int gid, nfs_cb cb, void *private_data)
{
	struct nfs_cb_data *data;
	struct nfs_chown_data *chown_data;

	chown_data = malloc(sizeof(struct nfs_chown_data));
	if (chown_data == NULL) {
		rpc_set_error(nfs->rpc, "Failed to allocate memory for chown data structure");
		return -1;
	}

	chown_data->uid = uid;
	chown_data->gid = gid;


	data = malloc(sizeof(struct nfs_cb_data));
	if (data == NULL) {
		rpc_set_error(nfs->rpc, "out of memory. failed to allocate memory for fchown data");
		return -1;
	}
	memset(data, 0, sizeof(struct nfs_cb_data));
	data->nfs           = nfs;
	data->cb            = cb;
	data->private_data  = private_data;
	data->continue_data = chown_data;
	data->fh.data.data_len = nfsfh->fh.data.data_len;
	data->fh.data.data_val = malloc(data->fh.data.data_len);
	if (data->fh.data.data_val == NULL) {
		rpc_set_error(nfs->rpc, "Out of memory: Failed to allocate fh");
		free_nfs_cb_data(data);
		return -1;
	}
	memcpy(data->fh.data.data_val, nfsfh->fh.data.data_val, data->fh.data.data_len);


	if (nfs_chown_continue_internal(nfs, data) != 0) {
		free_nfs_cb_data(data);
		return -1;
	}

	return 0;
}





/*
 * Async utimes()
 */
static void nfs_utimes_cb(struct rpc_context *rpc _U_, int status, void *command_data, void *private_data)
{
	struct nfs_cb_data *data = private_data;
	struct nfs_context *nfs = data->nfs;
	SETATTR3res *res;

	if (status == RPC_STATUS_ERROR) {
		data->cb(-EFAULT, nfs, command_data, data->private_data);
		free_nfs_cb_data(data);
		return;
	}
	if (status == RPC_STATUS_CANCEL) {
		data->cb(-EINTR, nfs, "Command was cancelled", data->private_data);
		free_nfs_cb_data(data);
		return;
	}

	res = command_data;
	if (res->status != NFS3_OK) {
		rpc_set_error(nfs->rpc, "NFS: SETATTR failed with %s(%d)", nfsstat3_to_str(res->status), nfsstat3_to_errno(res->status));
		data->cb(nfsstat3_to_errno(res->status), nfs, rpc_get_error(nfs->rpc), data->private_data);
		free_nfs_cb_data(data);
		return;
	}

	data->cb(0, nfs, NULL, data->private_data);
	free_nfs_cb_data(data);
}

static int nfs_utimes_continue_internal(struct nfs_context *nfs, struct nfs_cb_data *data)
{
	SETATTR3args args;
	struct timeval *utimes_data = data->continue_data;

	memset(&args, 0, sizeof(SETATTR3args));
	args.object.data.data_len = data->fh.data.data_len;
	args.object.data.data_val = data->fh.data.data_val;
	if (utimes_data != NULL) {
		args.new_attributes.atime.set_it = SET_TO_CLIENT_TIME;
		args.new_attributes.atime.set_atime_u.atime.seconds  = utimes_data[0].tv_sec;
		args.new_attributes.atime.set_atime_u.atime.nseconds = utimes_data[0].tv_usec * 1000;
		args.new_attributes.mtime.set_it = SET_TO_CLIENT_TIME;
		args.new_attributes.mtime.set_mtime_u.mtime.seconds  = utimes_data[1].tv_sec;
		args.new_attributes.mtime.set_mtime_u.mtime.nseconds = utimes_data[1].tv_usec * 1000;
	} else {
		args.new_attributes.atime.set_it = SET_TO_SERVER_TIME;
		args.new_attributes.mtime.set_it = SET_TO_SERVER_TIME;
	}

	if (rpc_nfs_setattr_async(nfs->rpc, nfs_utimes_cb, &args, data) != 0) {
		rpc_set_error(nfs->rpc, "RPC error: Failed to send SETATTR call for %s", data->path);
		data->cb(-ENOMEM, nfs, rpc_get_error(nfs->rpc), data->private_data);
		free_nfs_cb_data(data);
		return -1;
	}
	return 0;
}


int nfs_utimes_async(struct nfs_context *nfs, const char *path, struct timeval *times, nfs_cb cb, void *private_data)
{
	struct timeval *new_times = NULL;

	if (times != NULL) {
		new_times = malloc(sizeof(struct timeval)*2);
		if (new_times == NULL) {
			rpc_set_error(nfs->rpc, "Failed to allocate memory for timeval structure");
			return -1;
		}

		memcpy(new_times, times, sizeof(struct timeval)*2);
	}

	if (nfs_lookuppath_async(nfs, path, cb, private_data, nfs_utimes_continue_internal, new_times, free, 0) != 0) {
		rpc_set_error(nfs->rpc, "Out of memory: failed to start parsing the path components");
		return -1;
	}

	return 0;
}

/*
 * Async utime()
 */
int nfs_utime_async(struct nfs_context *nfs, const char *path, struct utimbuf *times, nfs_cb cb, void *private_data)
{
	struct timeval *new_times = NULL;

	if (times != NULL) {
		new_times = malloc(sizeof(struct timeval)*2);
		if (new_times == NULL) {
			rpc_set_error(nfs->rpc, "Failed to allocate memory for timeval structure");
			return -1;
		}

		new_times[0].tv_sec  = times->actime;
		new_times[0].tv_usec = 0;
		new_times[1].tv_sec  = times->modtime;
		new_times[1].tv_usec = 0;
	}

	if (nfs_lookuppath_async(nfs, path, cb, private_data, nfs_utimes_continue_internal, new_times, free, 0) != 0) {
		rpc_set_error(nfs->rpc, "Out of memory: failed to start parsing the path components");
		return -1;
	}

	return 0;
}





/*
 * Async access()
 */
static void nfs_access_cb(struct rpc_context *rpc _U_, int status, void *command_data, void *private_data)
{
	ACCESS3res *res;
	struct nfs_cb_data *data = private_data;
	struct nfs_context *nfs = data->nfs;
	unsigned int nfsmode = 0;

	if (status == RPC_STATUS_ERROR) {
		data->cb(-EFAULT, nfs, command_data, data->private_data);
		free_nfs_cb_data(data);
		return;
	}
	if (status == RPC_STATUS_CANCEL) {
		data->cb(-EINTR, nfs, "Command was cancelled", data->private_data);
		free_nfs_cb_data(data);
		return;
	}

	res = command_data;
	if (res->status != NFS3_OK) {
		rpc_set_error(nfs->rpc, "NFS: ACCESS of %s failed with %s(%d)", data->saved_path, nfsstat3_to_str(res->status), nfsstat3_to_errno(res->status));
		data->cb(nfsstat3_to_errno(res->status), nfs, rpc_get_error(nfs->rpc), data->private_data);
		free_nfs_cb_data(data);
		return;
	}

	if (data->continue_int & R_OK) {
		nfsmode |= ACCESS3_READ;
	}
	if (data->continue_int & W_OK) {
		nfsmode |= ACCESS3_MODIFY;
	}
	if (data->continue_int & X_OK) {
		nfsmode |= ACCESS3_EXECUTE;
	}

	if (res->ACCESS3res_u.resok.access != nfsmode) {
		rpc_set_error(nfs->rpc, "NFS: ACCESS denied. Required access %c%c%c. Allowed access %c%c%c",
					nfsmode&ACCESS3_READ?'r':'-',
					nfsmode&ACCESS3_MODIFY?'w':'-',
					nfsmode&ACCESS3_EXECUTE?'x':'-',
					res->ACCESS3res_u.resok.access&ACCESS3_READ?'r':'-',
					res->ACCESS3res_u.resok.access&ACCESS3_MODIFY?'w':'-',
					res->ACCESS3res_u.resok.access&ACCESS3_EXECUTE?'x':'-');
		data->cb(-EACCES, nfs, rpc_get_error(nfs->rpc), data->private_data);
		free_nfs_cb_data(data);
		return;
	}

	data->cb(0, nfs, NULL, data->private_data);
	free_nfs_cb_data(data);
}

static int nfs_access_continue_internal(struct nfs_context *nfs, struct nfs_cb_data *data)
{
	int nfsmode = 0;

	if (data->continue_int & R_OK) {
		nfsmode |= ACCESS3_READ;
	}
	if (data->continue_int & W_OK) {
		nfsmode |= ACCESS3_MODIFY;
	}
	if (data->continue_int & X_OK) {
		nfsmode |= ACCESS3_EXECUTE;
	}

	if (rpc_nfs_access_async(nfs->rpc, nfs_access_cb, &data->fh, nfsmode, data) != 0) {
		rpc_set_error(nfs->rpc, "RPC error: Failed to send OPEN ACCESS call for %s", data->path);
		data->cb(-ENOMEM, nfs, rpc_get_error(nfs->rpc), data->private_data);
		free_nfs_cb_data(data);
		return -1;
	}
	return 0;
}

int nfs_access_async(struct nfs_context *nfs, const char *path, int mode, nfs_cb cb, void *private_data)
{
	if (nfs_lookuppath_async(nfs, path, cb, private_data, nfs_access_continue_internal, NULL, NULL, mode) != 0) {
		rpc_set_error(nfs->rpc, "Out of memory: failed to start parsing the path components");
		return -1;
	}

	return 0;
}



/*
 * Async symlink()
 */
struct nfs_symlink_data {
       char *oldpath;
       char *newpathparent;
       char *newpathobject;
};

static void free_nfs_symlink_data(void *mem)
{
	struct nfs_symlink_data *data = mem;

	if (data->oldpath != NULL) {
		free(data->oldpath);
	}
	if (data->newpathparent != NULL) {
		free(data->newpathparent);
	}
	if (data->newpathobject != NULL) {
		free(data->newpathobject);
	}
	free(data);
}

static void nfs_symlink_cb(struct rpc_context *rpc _U_, int status, void *command_data, void *private_data)
{
	SYMLINK3res *res;
	struct nfs_cb_data *data = private_data;
	struct nfs_context *nfs = data->nfs;
	struct nfs_symlink_data *symlink_data = data->continue_data;
	
	if (status == RPC_STATUS_ERROR) {
		data->cb(-EFAULT, nfs, command_data, data->private_data);
		free_nfs_cb_data(data);
		return;
	}
	if (status == RPC_STATUS_CANCEL) {
		data->cb(-EINTR, nfs, "Command was cancelled", data->private_data);
		free_nfs_cb_data(data);
		return;
	}

	res = command_data;
	if (res->status != NFS3_OK) {
		rpc_set_error(nfs->rpc, "NFS: SYMLINK %s/%s -> %s failed with %s(%d)", symlink_data->newpathparent, symlink_data->newpathobject, symlink_data->oldpath, nfsstat3_to_str(res->status), nfsstat3_to_errno(res->status));
		data->cb(nfsstat3_to_errno(res->status), nfs, rpc_get_error(nfs->rpc), data->private_data);
		free_nfs_cb_data(data);
		return;
	}

	data->cb(0, nfs, NULL, data->private_data);
	free_nfs_cb_data(data);
}

static int nfs_symlink_continue_internal(struct nfs_context *nfs, struct nfs_cb_data *data)
{
	struct nfs_symlink_data *symlink_data = data->continue_data;
	SYMLINK3args sa;

	memset(&sa, 0, sizeof(SYMLINK3args));
	sa.where.dir.data.data_len = data->fh.data.data_len;
	sa.where.dir.data.data_val = data->fh.data.data_val;
	sa.where.name = symlink_data->newpathobject;
	sa.symlink.symlink_attributes.mode.set_it = 1;
	sa.symlink.symlink_attributes.mode.set_mode3_u.mode = S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IWGRP|S_IXGRP|S_IROTH|S_IWOTH|S_IXOTH;
	sa.symlink.symlink_data = symlink_data->oldpath;

	if (rpc_nfs_symlink_async(nfs->rpc, nfs_symlink_cb, &sa, data) != 0) {
		rpc_set_error(nfs->rpc, "RPC error: Failed to send SYMLINK call for %s", data->path);
		data->cb(-ENOMEM, nfs, rpc_get_error(nfs->rpc), data->private_data);
		free_nfs_cb_data(data);
		return -1;
	}
	return 0;
}

int nfs_symlink_async(struct nfs_context *nfs, const char *oldpath, const char *newpath, nfs_cb cb, void *private_data)
{
	char *ptr;
	struct nfs_symlink_data *symlink_data;

	symlink_data = malloc(sizeof(struct nfs_symlink_data));
	if (symlink_data == NULL) {
		rpc_set_error(nfs->rpc, "Out of memory, failed to allocate buffer for symlink data");
		return -1;
	}
	memset(symlink_data, 0, sizeof(struct nfs_symlink_data));

	symlink_data->oldpath = strdup(oldpath);
	if (symlink_data->oldpath == NULL) {
		rpc_set_error(nfs->rpc, "Out of memory, failed to allocate buffer for oldpath");
		free_nfs_symlink_data(symlink_data);
		return -1;
	}

	symlink_data->newpathparent = strdup(newpath);
	if (symlink_data->newpathparent == NULL) {
		rpc_set_error(nfs->rpc, "Out of memory, failed to allocate mode buffer for new path");
		free_nfs_symlink_data(symlink_data);
		return -1;
	}

	ptr = strrchr(symlink_data->newpathparent, '/');
	if (ptr == NULL) {
		rpc_set_error(nfs->rpc, "Invalid path %s", oldpath);
		free_nfs_symlink_data(symlink_data);
		return -1;
	}
	*ptr = 0;
	ptr++;

	symlink_data->newpathobject = strdup(ptr);
	if (symlink_data->newpathobject == NULL) {
		rpc_set_error(nfs->rpc, "Out of memory, failed to allocate mode buffer for new path");
		free_nfs_symlink_data(symlink_data);
		return -1;
	}

	if (nfs_lookuppath_async(nfs, symlink_data->newpathparent, cb, private_data, nfs_symlink_continue_internal, symlink_data, free_nfs_symlink_data, 0) != 0) {
		rpc_set_error(nfs->rpc, "Out of memory: failed to start parsing the path components");
		return -1;
	}

	return 0;
}



/*
 * Async rename()
 */
struct nfs_rename_data {
       char *oldpath;
       char *oldobject;
       struct nfs_fh3 olddir;
       char *newpath;
       char *newobject;
       struct nfs_fh3 newdir;
};

static void free_nfs_rename_data(void *mem)
{
	struct nfs_rename_data *data = mem;

	if (data->oldpath != NULL) {
		free(data->oldpath);
	}
	if (data->olddir.data.data_val != NULL) {
		free(data->olddir.data.data_val);
	}
	if (data->newpath != NULL) {
		free(data->newpath);
	}
	if (data->newdir.data.data_val != NULL) {
		free(data->newdir.data.data_val);
	}
	free(data);
}

static void nfs_rename_cb(struct rpc_context *rpc _U_, int status, void *command_data, void *private_data)
{
	RENAME3res *res;
	struct nfs_cb_data *data = private_data;
	struct nfs_context *nfs = data->nfs;
	struct nfs_rename_data *rename_data = data->continue_data;
	
	if (status == RPC_STATUS_ERROR) {
		data->cb(-EFAULT, nfs, command_data, data->private_data);
		free_nfs_cb_data(data);
		return;
	}
	if (status == RPC_STATUS_CANCEL) {
		data->cb(-EINTR, nfs, "Command was cancelled", data->private_data);
		free_nfs_cb_data(data);
		return;
	}

	res = command_data;
	if (res->status != NFS3_OK) {
		rpc_set_error(nfs->rpc, "NFS: RENAME %s/%s -> %s/%s failed with %s(%d)", rename_data->oldpath, rename_data->oldobject, rename_data->newpath, rename_data->newobject, nfsstat3_to_str(res->status), nfsstat3_to_errno(res->status));
		data->cb(nfsstat3_to_errno(res->status), nfs, rpc_get_error(nfs->rpc), data->private_data);
		free_nfs_cb_data(data);
		return;
	}

	data->cb(0, nfs, NULL, data->private_data);
	free_nfs_cb_data(data);
}

static int nfs_rename_continue_2_internal(struct nfs_context *nfs, struct nfs_cb_data *data)
{
	struct nfs_rename_data *rename_data = data->continue_data;

	/* steal the filehandle */
	rename_data->newdir.data.data_len = data->fh.data.data_len;
	rename_data->newdir.data.data_val = data->fh.data.data_val;
	data->fh.data.data_val = NULL;

	if (rpc_nfs_rename_async(nfs->rpc, nfs_rename_cb, &rename_data->olddir, rename_data->oldobject, &rename_data->newdir, rename_data->newobject, data) != 0) {
		rpc_set_error(nfs->rpc, "RPC error: Failed to send RENAME call for %s", data->path);
		data->cb(-ENOMEM, nfs, rpc_get_error(nfs->rpc), data->private_data);
		free_nfs_cb_data(data);
		return -1;
	}
	return 0;
}


static int nfs_rename_continue_1_internal(struct nfs_context *nfs, struct nfs_cb_data *data)
{
	struct nfs_rename_data *rename_data = data->continue_data;

	/* steal the filehandle */
	rename_data->olddir.data.data_len = data->fh.data.data_len;
	rename_data->olddir.data.data_val = data->fh.data.data_val;
	data->fh.data.data_val = NULL;

	if (nfs_lookuppath_async(nfs, rename_data->newpath, data->cb, data->private_data, nfs_rename_continue_2_internal, rename_data, free_nfs_rename_data, 0) != 0) {
		rpc_set_error(nfs->rpc, "RPC error: Failed to send LOOKUP call for %s", rename_data->newpath);
		data->cb(-ENOMEM, nfs, rpc_get_error(nfs->rpc), data->private_data);
		free_nfs_cb_data(data);
		return -1;
	}
	data->continue_data = NULL;
	free_nfs_cb_data(data);

	return 0;
}


int nfs_rename_async(struct nfs_context *nfs, const char *oldpath, const char *newpath, nfs_cb cb, void *private_data)
{
	char *ptr;
	struct nfs_rename_data *rename_data;

	rename_data = malloc(sizeof(struct nfs_rename_data));
	if (rename_data == NULL) {
		rpc_set_error(nfs->rpc, "Out of memory, failed to allocate buffer for rename data");
		return -1;
	}
	memset(rename_data, 0, sizeof(struct nfs_rename_data));

	rename_data->oldpath = strdup(oldpath);
	if (rename_data->oldpath == NULL) {
		rpc_set_error(nfs->rpc, "Out of memory, failed to allocate buffer for oldpath");
		free_nfs_rename_data(rename_data);
		return -1;
	}
	ptr = strrchr(rename_data->oldpath, '/');
	if (ptr == NULL) {
		rpc_set_error(nfs->rpc, "Invalid path %s", oldpath);
		free_nfs_rename_data(rename_data);
		return -1;
	}
	*ptr = 0;
	ptr++;
	rename_data->oldobject = ptr;


	rename_data->newpath = strdup(newpath);
	if (rename_data->newpath == NULL) {
		rpc_set_error(nfs->rpc, "Out of memory, failed to allocate buffer for newpath");
		free_nfs_rename_data(rename_data);
		return -1;
	}
	ptr = strrchr(rename_data->newpath, '/');
	if (ptr == NULL) {
		rpc_set_error(nfs->rpc, "Invalid path %s", newpath);
		free_nfs_rename_data(rename_data);
		return -1;
	}
	*ptr = 0;
	ptr++;
	rename_data->newobject = ptr;


	if (nfs_lookuppath_async(nfs, rename_data->oldpath, cb, private_data, nfs_rename_continue_1_internal, rename_data, free_nfs_rename_data, 0) != 0) {
		rpc_set_error(nfs->rpc, "Out of memory: failed to start parsing the path components");
		return -1;
	}

	return 0;
}


/*
 * Async link()
 */
struct nfs_link_data {
       char *oldpath;
       struct nfs_fh3 oldfh;
       char *newpath;
       char *newobject;
       struct nfs_fh3 newdir;
};

static void free_nfs_link_data(void *mem)
{
	struct nfs_link_data *data = mem;

	if (data->oldpath != NULL) {
		free(data->oldpath);
	}
	if (data->oldfh.data.data_val != NULL) {
		free(data->oldfh.data.data_val);
	}
	if (data->newpath != NULL) {
		free(data->newpath);
	}
	if (data->newdir.data.data_val != NULL) {
		free(data->newdir.data.data_val);
	}
	free(data);
}

static void nfs_link_cb(struct rpc_context *rpc _U_, int status, void *command_data, void *private_data)
{
	LINK3res *res;
	struct nfs_cb_data *data = private_data;
	struct nfs_context *nfs = data->nfs;
	struct nfs_link_data *link_data = data->continue_data;
	
	if (status == RPC_STATUS_ERROR) {
		data->cb(-EFAULT, nfs, command_data, data->private_data);
		free_nfs_cb_data(data);
		return;
	}
	if (status == RPC_STATUS_CANCEL) {
		data->cb(-EINTR, nfs, "Command was cancelled", data->private_data);
		free_nfs_cb_data(data);
		return;
	}

	res = command_data;
	if (res->status != NFS3_OK) {
		rpc_set_error(nfs->rpc, "NFS: LINK %s -> %s/%s failed with %s(%d)", link_data->oldpath, link_data->newpath, link_data->newobject, nfsstat3_to_str(res->status), nfsstat3_to_errno(res->status));
		data->cb(nfsstat3_to_errno(res->status), nfs, rpc_get_error(nfs->rpc), data->private_data);
		free_nfs_cb_data(data);
		return;
	}

	data->cb(0, nfs, NULL, data->private_data);
	free_nfs_cb_data(data);
}

static int nfs_link_continue_2_internal(struct nfs_context *nfs, struct nfs_cb_data *data)
{
	struct nfs_link_data *link_data = data->continue_data;

	/* steal the filehandle */
	link_data->newdir.data.data_len = data->fh.data.data_len;
	link_data->newdir.data.data_val = data->fh.data.data_val;
	data->fh.data.data_val = NULL;

	if (rpc_nfs_link_async(nfs->rpc, nfs_link_cb, &link_data->oldfh, &link_data->newdir, link_data->newobject, data) != 0) {
		rpc_set_error(nfs->rpc, "RPC error: Failed to send LINK call for %s", data->path);
		data->cb(-ENOMEM, nfs, rpc_get_error(nfs->rpc), data->private_data);
		free_nfs_cb_data(data);
		return -1;
	}
	return 0;
}


static int nfs_link_continue_1_internal(struct nfs_context *nfs, struct nfs_cb_data *data)
{
	struct nfs_link_data *link_data = data->continue_data;

	/* steal the filehandle */
	link_data->oldfh.data.data_len = data->fh.data.data_len;
	link_data->oldfh.data.data_val = data->fh.data.data_val;
	data->fh.data.data_val = NULL;

	if (nfs_lookuppath_async(nfs, link_data->newpath, data->cb, data->private_data, nfs_link_continue_2_internal, link_data, free_nfs_link_data, 0) != 0) {
		rpc_set_error(nfs->rpc, "RPC error: Failed to send LOOKUP call for %s", link_data->newpath);
		data->cb(-ENOMEM, nfs, rpc_get_error(nfs->rpc), data->private_data);
		free_nfs_cb_data(data);
		return -1;
	}
	data->continue_data = NULL;
	free_nfs_cb_data(data);

	return 0;
}


int nfs_link_async(struct nfs_context *nfs, const char *oldpath, const char *newpath, nfs_cb cb, void *private_data)
{
	char *ptr;
	struct nfs_link_data *link_data;

	link_data = malloc(sizeof(struct nfs_link_data));
	if (link_data == NULL) {
		rpc_set_error(nfs->rpc, "Out of memory, failed to allocate buffer for link data");
		return -1;
	}
	memset(link_data, 0, sizeof(struct nfs_link_data));

	link_data->oldpath = strdup(oldpath);
	if (link_data->oldpath == NULL) {
		rpc_set_error(nfs->rpc, "Out of memory, failed to allocate buffer for oldpath");
		free_nfs_link_data(link_data);
		return -1;
	}

	link_data->newpath = strdup(newpath);
	if (link_data->newpath == NULL) {
		rpc_set_error(nfs->rpc, "Out of memory, failed to allocate buffer for newpath");
		free_nfs_link_data(link_data);
		return -1;
	}
	ptr = strrchr(link_data->newpath, '/');
	if (ptr == NULL) {
		rpc_set_error(nfs->rpc, "Invalid path %s", newpath);
		free_nfs_link_data(link_data);
		return -1;
	}
	*ptr = 0;
	ptr++;
	link_data->newobject = ptr;


	if (nfs_lookuppath_async(nfs, link_data->oldpath, cb, private_data, nfs_link_continue_1_internal, link_data, free_nfs_link_data, 0) != 0) {
		rpc_set_error(nfs->rpc, "Out of memory: failed to start parsing the path components");
		return -1;
	}

	return 0;
}


//qqq replace later with lseek()
uint64_t nfs_get_current_offset(struct nfsfh *nfsfh)
{
	return nfsfh->offset;
}



/*
 * Get the maximum supported READ3 size by the server
 */
uint64_t nfs_get_readmax(struct nfs_context *nfs)
{
	return nfs->readmax;
}

/*
 * Get the maximum supported WRITE3 size by the server
 */
uint64_t nfs_get_writemax(struct nfs_context *nfs)
{
	/* Some XDR libraries can not marshall PDUs bigger than this */
        if (nfs->writemax < 32768) {
		return nfs->writemax;
	}
	return 32768;
}

void nfs_set_error(struct nfs_context *nfs, char *error_string, ...)
{
        va_list ap;
	char *str = NULL;

        va_start(ap, error_string);
	str = malloc(1024);
	vsnprintf(str, 1024, error_string, ap);
	if (nfs->rpc->error_string != NULL) {
		free(nfs->rpc->error_string);
	}
	nfs->rpc->error_string = str;
    va_end(ap);
}



struct mount_cb_data {
       rpc_cb cb;
       void *private_data;
       char *server;
};

static void free_mount_cb_data(struct mount_cb_data *data)
{
	if (data->server != NULL) {
		free(data->server);
		data->server = NULL;
	}

	free(data);
}

static void mount_export_5_cb(struct rpc_context *rpc, int status, void *command_data, void *private_data)
{
	struct mount_cb_data *data = private_data;

	if (status == RPC_STATUS_ERROR) {	
		data->cb(rpc, -EFAULT, command_data, data->private_data);
		free_mount_cb_data(data);
		return;
	}
	if (status == RPC_STATUS_CANCEL) {
		data->cb(rpc, -EINTR, "Command was cancelled", data->private_data);
		free_mount_cb_data(data);
		return;
	}

	data->cb(rpc, 0, command_data, data->private_data);
	if (rpc_disconnect(rpc, "normal disconnect") != 0) {
		rpc_set_error(rpc, "Failed to disconnect\n");
	}
	free_mount_cb_data(data);
}

static void mount_export_4_cb(struct rpc_context *rpc, int status, void *command_data, void *private_data)
{
	struct mount_cb_data *data = private_data;

	/* Dont want any more callbacks even if the socket is closed */
	rpc->connect_cb = NULL;

	if (status == RPC_STATUS_ERROR) {	
		data->cb(rpc, -EFAULT, command_data, data->private_data);
		free_mount_cb_data(data);
		return;
	}
	if (status == RPC_STATUS_CANCEL) {
		data->cb(rpc, -EINTR, "Command was cancelled", data->private_data);
		free_mount_cb_data(data);
		return;
	}

	if (rpc_mount_export_async(rpc, mount_export_5_cb, data) != 0) {
		data->cb(rpc, -ENOMEM, command_data, data->private_data);
		free_mount_cb_data(data);
		return;
	}
}

static void mount_export_3_cb(struct rpc_context *rpc, int status, void *command_data, void *private_data)
{
	struct mount_cb_data *data = private_data;
	uint32_t mount_port;

	if (status == RPC_STATUS_ERROR) {	
		data->cb(rpc, -EFAULT, command_data, data->private_data);
		free_mount_cb_data(data);
		return;
	}
	if (status == RPC_STATUS_CANCEL) {
		data->cb(rpc, -EINTR, "Command was cancelled", data->private_data);
		free_mount_cb_data(data);
		return;
	}

	mount_port = *(uint32_t *)command_data;
	if (mount_port == 0) {
		rpc_set_error(rpc, "RPC error. Mount program is not available");
		data->cb(rpc, -ENOENT, command_data, data->private_data);
		free_mount_cb_data(data);
		return;
	}

	rpc_disconnect(rpc, "normal disconnect");
	if (rpc_connect_async(rpc, data->server, mount_port, mount_export_4_cb, data) != 0) {
		data->cb(rpc, -ENOMEM, command_data, data->private_data);
		free_mount_cb_data(data);
		return;
	}
}

static void mount_export_2_cb(struct rpc_context *rpc, int status, void *command_data, void *private_data)
{
	struct mount_cb_data *data = private_data;

	if (status == RPC_STATUS_ERROR) {
		data->cb(rpc, -EFAULT, command_data, data->private_data);
		free_mount_cb_data(data);
		return;
	}
	if (status == RPC_STATUS_CANCEL) {
		data->cb(rpc, -EINTR, "Command was cancelled", data->private_data);
		free_mount_cb_data(data);
		return;
	}

	if (rpc_pmap_getport_async(rpc, MOUNT_PROGRAM, MOUNT_V3, IPPROTO_TCP, mount_export_3_cb, private_data) != 0) {
		data->cb(rpc, -ENOMEM, command_data, data->private_data);
		free_mount_cb_data(data);
		return;
	}
}

static void mount_export_1_cb(struct rpc_context *rpc, int status, void *command_data, void *private_data)
{
	struct mount_cb_data *data = private_data;

	/* Dont want any more callbacks even if the socket is closed */
	rpc->connect_cb = NULL;

	if (status == RPC_STATUS_ERROR) {
		data->cb(rpc, -EFAULT, command_data, data->private_data);
		free_mount_cb_data(data);
		return;
	}
	if (status == RPC_STATUS_CANCEL) {
		data->cb(rpc, -EINTR, "Command was cancelled", data->private_data);
		free_mount_cb_data(data);
		return;
	}

	if (rpc_pmap_null_async(rpc, mount_export_2_cb, data) != 0) {
		data->cb(rpc, -ENOMEM, command_data, data->private_data);
		free_mount_cb_data(data);
		return;
	}
}

int mount_getexports_async(struct rpc_context *rpc, const char *server, rpc_cb cb, void *private_data)
{
	struct mount_cb_data *data;

	data = malloc(sizeof(struct mount_cb_data));
	if (data == NULL) {
		return -1;
	}
	memset(data, 0, sizeof(struct mount_cb_data));
	data->cb           = cb;
	data->private_data = private_data;
	data->server       = strdup(server);
	if (data->server == NULL) {
		free_mount_cb_data(data);
		return -1;
	}	
	if (rpc_connect_async(rpc, data->server, 111, mount_export_1_cb, data) != 0) {
		free_mount_cb_data(data);
		return -1;
	}

	return 0;
}

struct rpc_context *nfs_get_rpc_context(struct nfs_context *nfs)
{
	return nfs->rpc;
}

const char *nfs_get_server(struct nfs_context *nfs) {
	return nfs->server;
}

const char *nfs_get_export(struct nfs_context *nfs) {
	return nfs->export;
}

const struct nfs_fh3 *nfs_get_rootfh(struct nfs_context *nfs) {
      return &nfs->rootfh;
}

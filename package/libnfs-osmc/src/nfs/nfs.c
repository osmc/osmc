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

#ifdef WIN32
#include "win32_compat.h"
#else
#include <sys/stat.h>
#endif/*WIN32*/

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <rpc/rpc.h>
#include <rpc/xdr.h>
#include "libnfs.h"
#include "libnfs-raw.h"
#include "libnfs-private.h"
#include "libnfs-raw-nfs.h"

char *nfsstat3_to_str(int error)
{
	switch (error) {
	case NFS3_OK: return "NFS3_OK"; break;
	case NFS3ERR_PERM: return "NFS3ERR_PERM"; break;
	case NFS3ERR_NOENT: return "NFS3ERR_NOENT"; break;
	case NFS3ERR_IO: return "NFS3ERR_IO"; break;
	case NFS3ERR_NXIO: return "NFS3ERR_NXIO"; break;
	case NFS3ERR_ACCES: return "NFS3ERR_ACCES"; break;
	case NFS3ERR_EXIST: return "NFS3ERR_EXIST"; break;
	case NFS3ERR_XDEV: return "NFS3ERR_XDEV"; break;
	case NFS3ERR_NODEV: return "NFS3ERR_NODEV"; break;
	case NFS3ERR_NOTDIR: return "NFS3ERR_NOTDIR"; break;
	case NFS3ERR_ISDIR: return "NFS3ERR_ISDIR"; break;
	case NFS3ERR_INVAL: return "NFS3ERR_INVAL"; break;
	case NFS3ERR_FBIG: return "NFS3ERR_FBIG"; break;
	case NFS3ERR_NOSPC: return "NFS3ERR_NOSPC"; break;
	case NFS3ERR_ROFS: return "NFS3ERR_ROFS"; break;
	case NFS3ERR_MLINK: return "NFS3ERR_MLINK"; break;
	case NFS3ERR_NAMETOOLONG: return "NFS3ERR_NAMETOOLONG"; break;
	case NFS3ERR_NOTEMPTY: return "NFS3ERR_NOTEMPTY"; break;
	case NFS3ERR_DQUOT: return "NFS3ERR_DQUOT"; break;
	case NFS3ERR_STALE: return "NFS3ERR_STALE"; break;
	case NFS3ERR_REMOTE: return "NFS3ERR_REMOTE"; break;
	case NFS3ERR_BADHANDLE: return "NFS3ERR_BADHANDLE"; break;
	case NFS3ERR_NOT_SYNC: return "NFS3ERR_NOT_SYNC"; break;
	case NFS3ERR_BAD_COOKIE: return "NFS3ERR_BAD_COOKIE"; break;
	case NFS3ERR_NOTSUPP: return "NFS3ERR_NOTSUPP"; break;
	case NFS3ERR_TOOSMALL: return "NFS3ERR_TOOSMALL"; break;
	case NFS3ERR_SERVERFAULT: return "NFS3ERR_SERVERFAULT"; break;
	case NFS3ERR_BADTYPE: return "NFS3ERR_BADTYPE"; break;
	case NFS3ERR_JUKEBOX: return "NFS3ERR_JUKEBOX"; break;
	};
	return "unknown nfs error";
}

int nfsstat3_to_errno(int error)
{
	switch (error) {
	case NFS3_OK:             return 0; break;
	case NFS3ERR_PERM:        return -EPERM; break;
	case NFS3ERR_NOENT:       return -ENOENT; break;
	case NFS3ERR_IO:          return -EIO; break;
	case NFS3ERR_NXIO:        return -ENXIO; break;
	case NFS3ERR_ACCES:       return -EACCES; break;
	case NFS3ERR_EXIST:       return -EEXIST; break;
	case NFS3ERR_XDEV:        return -EXDEV; break;
	case NFS3ERR_NODEV:       return -ENODEV; break;
	case NFS3ERR_NOTDIR:      return -ENOTDIR; break;
	case NFS3ERR_ISDIR:       return -EISDIR; break;
	case NFS3ERR_INVAL:       return -EINVAL; break;
	case NFS3ERR_FBIG:        return -EFBIG; break;
	case NFS3ERR_NOSPC:       return -ENOSPC; break;
	case NFS3ERR_ROFS:        return -EROFS; break;
	case NFS3ERR_MLINK:       return -EMLINK; break;
	case NFS3ERR_NAMETOOLONG: return -ENAMETOOLONG; break;
	case NFS3ERR_NOTEMPTY:    return -EEXIST; break;
	case NFS3ERR_DQUOT:       return -ERANGE; break;
	case NFS3ERR_STALE:       return -EIO; break;
	case NFS3ERR_REMOTE:      return -EIO; break;
	case NFS3ERR_BADHANDLE:   return -EIO; break;
	case NFS3ERR_NOT_SYNC:    return -EIO; break;
	case NFS3ERR_BAD_COOKIE:  return -EIO; break;
	case NFS3ERR_NOTSUPP:     return -EINVAL; break;
	case NFS3ERR_TOOSMALL:    return -EIO; break;
	case NFS3ERR_SERVERFAULT: return -EIO; break;
	case NFS3ERR_BADTYPE:     return -EINVAL; break;
	case NFS3ERR_JUKEBOX:     return -EAGAIN; break;
	};
	return -ERANGE;
}


int rpc_nfs_null_async(struct rpc_context *rpc, rpc_cb cb, void *private_data)
{
	struct rpc_pdu *pdu;

	pdu = rpc_allocate_pdu(rpc, NFS_PROGRAM, NFS_V3, NFS3_NULL, cb, private_data, (xdrproc_t)xdr_void, 0);
	if (pdu == NULL) {
		rpc_set_error(rpc, "Out of memory. Failed to allocate pdu for nfs/null call");
		return -1;
	}

	if (rpc_queue_pdu(rpc, pdu) != 0) {
		rpc_set_error(rpc, "Out of memory. Failed to queue pdu for nfs/null call");
		rpc_free_pdu(rpc, pdu);
		return -2;
	}

	return 0;
}

int rpc_nfs_getattr_async(struct rpc_context *rpc, rpc_cb cb, struct nfs_fh3 *fh, void *private_data)
{
	struct rpc_pdu *pdu;
	GETATTR3args args;

	pdu = rpc_allocate_pdu(rpc, NFS_PROGRAM, NFS_V3, NFS3_GETATTR, cb, private_data, (xdrproc_t)xdr_GETATTR3res, sizeof(GETATTR3res));
	if (pdu == NULL) {
		rpc_set_error(rpc, "Out of memory. Failed to allocate pdu for nfs/null call");
		return -1;
	}

	args.object.data.data_len = fh->data.data_len; 
	args.object.data.data_val = fh->data.data_val; 

	if (xdr_GETATTR3args(&pdu->xdr, &args) == 0) {
		rpc_set_error(rpc, "XDR error: Failed to encode GETATTR3args");
		rpc_free_pdu(rpc, pdu);
		return -2;
	}

	if (rpc_queue_pdu(rpc, pdu) != 0) {
		rpc_set_error(rpc, "Out of memory. Failed to queue pdu for nfs/null call");
		rpc_free_pdu(rpc, pdu);
		return -3;
	}

	return 0;
}

int rpc_nfs_lookup_async(struct rpc_context *rpc, rpc_cb cb, struct nfs_fh3 *fh, char *name, void *private_data)
{
	struct rpc_pdu *pdu;
	LOOKUP3args args;

	pdu = rpc_allocate_pdu(rpc, NFS_PROGRAM, NFS_V3, NFS3_LOOKUP, cb, private_data, (xdrproc_t)xdr_LOOKUP3res, sizeof(LOOKUP3res));
	if (pdu == NULL) {
		rpc_set_error(rpc, "Out of memory. Failed to allocate pdu for nfs/lookup call");
		return -1;
	}

	args.what.dir.data.data_len = fh->data.data_len; 
	args.what.dir.data.data_val = fh->data.data_val; 
	args.what.name              = name;

	if (xdr_LOOKUP3args(&pdu->xdr, &args) == 0) {
		rpc_set_error(rpc, "XDR error: Failed to encode LOOKUP3args");
		rpc_free_pdu(rpc, pdu);
		return -2;
	}

	if (rpc_queue_pdu(rpc, pdu) != 0) {
		rpc_set_error(rpc, "Out of memory. Failed to queue pdu for nfs/lookup call");
		rpc_free_pdu(rpc, pdu);
		return -3;
	}

	return 0;
}


int rpc_nfs_access_async(struct rpc_context *rpc, rpc_cb cb, struct nfs_fh3 *fh, int access, void *private_data)
{
	struct rpc_pdu *pdu;
	ACCESS3args args;

	pdu = rpc_allocate_pdu(rpc, NFS_PROGRAM, NFS_V3, NFS3_ACCESS, cb, private_data, (xdrproc_t)xdr_ACCESS3res, sizeof(ACCESS3res));
	if (pdu == NULL) {
		rpc_set_error(rpc, "Out of memory. Failed to allocate pdu for nfs/access call");
		return -1;
	}

	args.object.data.data_len = fh->data.data_len;
	args.object.data.data_val = fh->data.data_val;
	args.access = access;

	if (xdr_ACCESS3args(&pdu->xdr, &args) == 0) {
		rpc_set_error(rpc, "XDR error: Failed to encode ACCESS3args");
		rpc_free_pdu(rpc, pdu);
		return -2;
	}

	if (rpc_queue_pdu(rpc, pdu) != 0) {
		rpc_set_error(rpc, "Out of memory. Failed to queue pdu for nfs/access call");
		rpc_free_pdu(rpc, pdu);
		return -3;
	}

	return 0;
}



int rpc_nfs_read_async(struct rpc_context *rpc, rpc_cb cb, struct nfs_fh3 *fh, uint64_t offset, uint64_t count, void *private_data)
{
	struct rpc_pdu *pdu;
	READ3args args;

	pdu = rpc_allocate_pdu(rpc, NFS_PROGRAM, NFS_V3, NFS3_READ, cb, private_data, (xdrproc_t)xdr_READ3res, sizeof(READ3res));
	if (pdu == NULL) {
		rpc_set_error(rpc, "Out of memory. Failed to allocate pdu for nfs/read call");
		return -1;
	}

	args.file.data.data_len = fh->data.data_len;
	args.file.data.data_val = fh->data.data_val;
	args.offset = offset;
	args.count = count;

	if (xdr_READ3args(&pdu->xdr, &args) == 0) {
		rpc_set_error(rpc, "XDR error: Failed to encode READ3args");
		rpc_free_pdu(rpc, pdu);
		return -2;
	}

	if (rpc_queue_pdu(rpc, pdu) != 0) {
		rpc_set_error(rpc, "Out of memory. Failed to queue pdu for nfs/read call");
		rpc_free_pdu(rpc, pdu);
		return -3;
	}

	return 0;
}


int rpc_nfs_write_async(struct rpc_context *rpc, rpc_cb cb, struct nfs_fh3 *fh, char *buf, uint64_t offset, uint64_t count, int stable_how, void *private_data)
{
	struct rpc_pdu *pdu;
	WRITE3args args;

	pdu = rpc_allocate_pdu(rpc, NFS_PROGRAM, NFS_V3, NFS3_WRITE, cb, private_data, (xdrproc_t)xdr_WRITE3res, sizeof(WRITE3res));
	if (pdu == NULL) {
		rpc_set_error(rpc, "Out of memory. Failed to allocate pdu for nfs/write call");
		return -1;
	}

	args.file.data.data_len = fh->data.data_len;
	args.file.data.data_val = fh->data.data_val;
	args.offset = offset;
	args.count  = count;
	args.stable = stable_how;;
	args.data.data_len = count;
	args.data.data_val = buf;

	if (xdr_WRITE3args(&pdu->xdr, &args) == 0) {
		rpc_set_error(rpc, "XDR error: Failed to encode WRITE3args");
		rpc_free_pdu(rpc, pdu);
		return -2;
	}

	if (rpc_queue_pdu(rpc, pdu) != 0) {
		rpc_set_error(rpc, "Out of memory. Failed to queue pdu for nfs/write call");
		rpc_free_pdu(rpc, pdu);
		return -3;
	}

	return 0;
}



int rpc_nfs_commit_async(struct rpc_context *rpc, rpc_cb cb, struct nfs_fh3 *fh, void *private_data)
{
	struct rpc_pdu *pdu;
	COMMIT3args args;

	pdu = rpc_allocate_pdu(rpc, NFS_PROGRAM, NFS_V3, NFS3_COMMIT, cb, private_data, (xdrproc_t)xdr_COMMIT3res, sizeof(COMMIT3res));
	if (pdu == NULL) {
		rpc_set_error(rpc, "Out of memory. Failed to allocate pdu for nfs/commit call");
		return -1;
	}

	args.file.data.data_len = fh->data.data_len;
	args.file.data.data_val = fh->data.data_val;
	args.offset = 0;
	args.count  = 0;

	if (xdr_COMMIT3args(&pdu->xdr, &args) == 0) {
		rpc_set_error(rpc, "XDR error: Failed to encode WRITE3args");
		rpc_free_pdu(rpc, pdu);
		return -2;
	}

	if (rpc_queue_pdu(rpc, pdu) != 0) {
		rpc_set_error(rpc, "Out of memory. Failed to queue pdu for nfs/commit call");
		rpc_free_pdu(rpc, pdu);
		return -3;
	}

	return 0;
}


int rpc_nfs_setattr_async(struct rpc_context *rpc, rpc_cb cb, SETATTR3args *args, void *private_data)
{
	struct rpc_pdu *pdu;

	pdu = rpc_allocate_pdu(rpc, NFS_PROGRAM, NFS_V3, NFS3_SETATTR, cb, private_data, (xdrproc_t)xdr_SETATTR3res, sizeof(SETATTR3res));
	if (pdu == NULL) {
		rpc_set_error(rpc, "Out of memory. Failed to allocate pdu for nfs/setattr call");
		return -1;
	}

	if (xdr_SETATTR3args(&pdu->xdr, args) == 0) {
		rpc_set_error(rpc, "XDR error: Failed to encode SETATTR3args");
		rpc_free_pdu(rpc, pdu);
		return -2;
	}

	if (rpc_queue_pdu(rpc, pdu) != 0) {
		rpc_set_error(rpc, "Out of memory. Failed to queue pdu for nfs/setattr call");
		rpc_free_pdu(rpc, pdu);
		return -3;
	}

	return 0;
}



int rpc_nfs_mkdir_async(struct rpc_context *rpc, rpc_cb cb, MKDIR3args *args, void *private_data)
{
	struct rpc_pdu *pdu;

	pdu = rpc_allocate_pdu(rpc, NFS_PROGRAM, NFS_V3, NFS3_MKDIR, cb, private_data, (xdrproc_t)xdr_MKDIR3res, sizeof(MKDIR3res));
	if (pdu == NULL) {
		rpc_set_error(rpc, "Out of memory. Failed to allocate pdu for nfs/setattr call");
		return -1;
	}

	if (xdr_MKDIR3args(&pdu->xdr, args) == 0) {
		rpc_set_error(rpc, "XDR error: Failed to encode MKDIR3args");
		rpc_free_pdu(rpc, pdu);
		return -2;
	}

	if (rpc_queue_pdu(rpc, pdu) != 0) {
		rpc_set_error(rpc, "Out of memory. Failed to queue pdu for nfs/mkdir call");
		rpc_free_pdu(rpc, pdu);
		return -3;
	}

	return 0;
}




int rpc_nfs_rmdir_async(struct rpc_context *rpc, rpc_cb cb, struct nfs_fh3 *fh, char *dir, void *private_data)
{
	struct rpc_pdu *pdu;
	RMDIR3args args;

	pdu = rpc_allocate_pdu(rpc, NFS_PROGRAM, NFS_V3, NFS3_RMDIR, cb, private_data, (xdrproc_t)xdr_RMDIR3res, sizeof(RMDIR3res));
	if (pdu == NULL) {
		rpc_set_error(rpc, "Out of memory. Failed to allocate pdu for nfs/rmdir call");
		return -1;
	}

	memset(&args, 0, sizeof(RMDIR3args));
	args.object.dir.data.data_len = fh->data.data_len;
	args.object.dir.data.data_val = fh->data.data_val;
	args.object.name = dir;

	if (xdr_RMDIR3args(&pdu->xdr, &args) == 0) {
		rpc_set_error(rpc, "XDR error: Failed to encode RMDIR3args");
		rpc_free_pdu(rpc, pdu);
		return -2;
	}

	if (rpc_queue_pdu(rpc, pdu) != 0) {
		rpc_set_error(rpc, "Out of memory. Failed to queue pdu for nfs/rmdir call");
		rpc_free_pdu(rpc, pdu);
		return -3;
	}

	return 0;
}



int rpc_nfs_create_async(struct rpc_context *rpc, rpc_cb cb, CREATE3args *args, void *private_data)
{
	struct rpc_pdu *pdu;

	pdu = rpc_allocate_pdu(rpc, NFS_PROGRAM, NFS_V3, NFS3_CREATE, cb, private_data, (xdrproc_t)xdr_CREATE3res, sizeof(CREATE3res));
	if (pdu == NULL) {
		rpc_set_error(rpc, "Out of memory. Failed to allocate pdu for nfs/create call");
		return -1;
	}

	if (xdr_CREATE3args(&pdu->xdr, args) == 0) {
		rpc_set_error(rpc, "XDR error: Failed to encode CREATE3args");
		rpc_free_pdu(rpc, pdu);
		return -2;
	}

	if (rpc_queue_pdu(rpc, pdu) != 0) {
		rpc_set_error(rpc, "Out of memory. Failed to queue pdu for nfs/create call");
		rpc_free_pdu(rpc, pdu);
		return -3;
	}

	return 0;
}



int rpc_nfs_mknod_async(struct rpc_context *rpc, rpc_cb cb, struct nfs_fh3 *fh, char *file, int mode, int major, int minor, void *private_data)
{
	struct rpc_pdu *pdu;
	MKNOD3args args;

	pdu = rpc_allocate_pdu(rpc, NFS_PROGRAM, NFS_V3, NFS3_MKNOD, cb, private_data, (xdrproc_t)xdr_MKNOD3res, sizeof(MKNOD3res));
	if (pdu == NULL) {
		rpc_set_error(rpc, "Out of memory. Failed to allocate pdu for nfs/mknod call");
		return -1;
	}

	memset(&args, 0, sizeof(MKNOD3args));
	args.where.dir.data.data_len = fh->data.data_len;
	args.where.dir.data.data_val = fh->data.data_val;
	args.where.name = file;
	switch (mode & S_IFMT) {
	case S_IFCHR:
		args.what.type = NF3CHR;
		args.what.mknoddata3_u.chr_device.dev_attributes.mode.set_it = 1;
		args.what.mknoddata3_u.chr_device.dev_attributes.mode.set_mode3_u.mode = mode & (S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IWGRP|S_IXGRP|S_IROTH|S_IWOTH|S_IXOTH);
		args.what.mknoddata3_u.chr_device.spec.specdata1 = major;
		args.what.mknoddata3_u.chr_device.spec.specdata2 = minor;
		break;
	case S_IFBLK:
		args.what.type = NF3BLK;
		args.what.mknoddata3_u.blk_device.dev_attributes.mode.set_it = 1;
		args.what.mknoddata3_u.blk_device.dev_attributes.mode.set_mode3_u.mode = mode & (S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IWGRP|S_IXGRP|S_IROTH|S_IWOTH|S_IXOTH);
		args.what.mknoddata3_u.blk_device.spec.specdata1 = major;
		args.what.mknoddata3_u.blk_device.spec.specdata2 = minor;
	case S_IFSOCK:
		args.what.type = NF3SOCK;
		args.what.mknoddata3_u.sock_attributes.mode.set_it = 1;
		args.what.mknoddata3_u.sock_attributes.mode.set_mode3_u.mode = mode & (S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IWGRP|S_IXGRP|S_IROTH|S_IWOTH|S_IXOTH);
		break;
	case S_IFIFO:
		args.what.type = NF3FIFO;
		args.what.mknoddata3_u.pipe_attributes.mode.set_it = 1;
		args.what.mknoddata3_u.pipe_attributes.mode.set_mode3_u.mode = mode & (S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IWGRP|S_IXGRP|S_IROTH|S_IWOTH|S_IXOTH);
		break;
	default:
		rpc_set_error(rpc, "Invalid file type for nfs/mknod call");
		rpc_free_pdu(rpc, pdu);
		return -1;
	}

	if (xdr_MKNOD3args(&pdu->xdr, &args) == 0) {
		rpc_set_error(rpc, "XDR error: Failed to encode MKNOD3args");
		rpc_free_pdu(rpc, pdu);
		return -2;
	}

	if (rpc_queue_pdu(rpc, pdu) != 0) {
		rpc_set_error(rpc, "Out of memory. Failed to queue pdu for nfs/mknod call");
		rpc_free_pdu(rpc, pdu);
		return -3;
	}

	return 0;
}


int rpc_nfs_remove_async(struct rpc_context *rpc, rpc_cb cb, struct nfs_fh3 *fh, char *file, void *private_data)
{
	struct rpc_pdu *pdu;
	REMOVE3args args;

	pdu = rpc_allocate_pdu(rpc, NFS_PROGRAM, NFS_V3, NFS3_REMOVE, cb, private_data, (xdrproc_t)xdr_REMOVE3res, sizeof(REMOVE3res));
	if (pdu == NULL) {
		rpc_set_error(rpc, "Out of memory. Failed to allocate pdu for nfs/remove call");
		return -1;
	}

	memset(&args, 0, sizeof(REMOVE3args));
	args.object.dir.data.data_len = fh->data.data_len;
	args.object.dir.data.data_val = fh->data.data_val;
	args.object.name = file;

	if (xdr_REMOVE3args(&pdu->xdr, &args) == 0) {
		rpc_set_error(rpc, "XDR error: Failed to encode REMOVE3args");
		rpc_free_pdu(rpc, pdu);
		return -2;
	}

	if (rpc_queue_pdu(rpc, pdu) != 0) {
		rpc_set_error(rpc, "Out of memory. Failed to queue pdu for nfs/remove call");
		rpc_free_pdu(rpc, pdu);
		return -3;
	}

	return 0;
}

int rpc_nfs_readdir_async(struct rpc_context *rpc, rpc_cb cb, struct nfs_fh3 *fh, uint64_t cookie, char *cookieverf, int count, void *private_data)
{
	struct rpc_pdu *pdu;
	READDIR3args args;

	pdu = rpc_allocate_pdu(rpc, NFS_PROGRAM, NFS_V3, NFS3_READDIR, cb, private_data, (xdrproc_t)xdr_READDIR3res, sizeof(READDIR3res));
	if (pdu == NULL) {
		rpc_set_error(rpc, "Out of memory. Failed to allocate pdu for nfs/readdir call");
		return -1;
	}

	memset(&args, 0, sizeof(READDIR3args));
	args.dir.data.data_len = fh->data.data_len;
	args.dir.data.data_val = fh->data.data_val;
	args.cookie = cookie;
	memcpy(&args.cookieverf, cookieverf, sizeof(cookieverf3)); 
	args.count = count;

	if (xdr_READDIR3args(&pdu->xdr, &args) == 0) {
		rpc_set_error(rpc, "XDR error: Failed to encode READDIR3args");
		rpc_free_pdu(rpc, pdu);
		return -2;
	}

	if (rpc_queue_pdu(rpc, pdu) != 0) {
		rpc_set_error(rpc, "Out of memory. Failed to queue pdu for nfs/readdir call");
		rpc_free_pdu(rpc, pdu);
		return -3;
	}

	return 0;
}

int rpc_nfs_readdirplus_async(struct rpc_context *rpc, rpc_cb cb, struct nfs_fh3 *fh, uint64_t cookie, char *cookieverf, int count, void *private_data)
{
	struct rpc_pdu *pdu;
	READDIRPLUS3args args;

	pdu = rpc_allocate_pdu(rpc, NFS_PROGRAM, NFS_V3, NFS3_READDIRPLUS, cb, private_data, (xdrproc_t)xdr_READDIRPLUS3res, sizeof(READDIRPLUS3res));
	if (pdu == NULL) {
		rpc_set_error(rpc, "Out of memory. Failed to allocate pdu for nfs/readdirplus call");
		return -1;
	}

	memset(&args, 0, sizeof(READDIRPLUS3args));
	args.dir.data.data_len = fh->data.data_len;
	args.dir.data.data_val = fh->data.data_val;
	args.cookie = cookie;
	memcpy(&args.cookieverf, cookieverf, sizeof(cookieverf3)); 
	args.dircount = count;
	args.maxcount = count;

	if (xdr_READDIRPLUS3args(&pdu->xdr, &args) == 0) {
		rpc_set_error(rpc, "XDR error: Failed to encode READDIRPLUS3args");
		rpc_free_pdu(rpc, pdu);
		return -2;
	}

	if (rpc_queue_pdu(rpc, pdu) != 0) {
		rpc_set_error(rpc, "Out of memory. Failed to queue pdu for nfs/readdirplus call");
		rpc_free_pdu(rpc, pdu);
		return -3;
	}

	return 0;
}

int rpc_nfs_fsstat_async(struct rpc_context *rpc, rpc_cb cb, struct nfs_fh3 *fh, void *private_data)
{
	struct rpc_pdu *pdu;
	FSSTAT3args args;

	pdu = rpc_allocate_pdu(rpc, NFS_PROGRAM, NFS_V3, NFS3_FSSTAT, cb, private_data, (xdrproc_t)xdr_FSSTAT3res, sizeof(FSSTAT3res));
	if (pdu == NULL) {
		rpc_set_error(rpc, "Out of memory. Failed to allocate pdu for nfs/fsstat call");
		return -1;
	}

	args.fsroot.data.data_len = fh->data.data_len; 
	args.fsroot.data.data_val = fh->data.data_val; 

	if (xdr_FSSTAT3args(&pdu->xdr, &args) == 0) {
		rpc_set_error(rpc, "XDR error: Failed to encode FSSTAT3args");
		rpc_free_pdu(rpc, pdu);
		return -2;
	}

	if (rpc_queue_pdu(rpc, pdu) != 0) {
		rpc_set_error(rpc, "Out of memory. Failed to queue pdu for nfs/fsstat call");
		rpc_free_pdu(rpc, pdu);
		return -3;
	}

	return 0;
}

int rpc_nfs_fsinfo_async(struct rpc_context *rpc, rpc_cb cb, struct nfs_fh3 *fh, void *private_data)
{
	struct rpc_pdu *pdu;
	FSINFO3args args;

	pdu = rpc_allocate_pdu(rpc, NFS_PROGRAM, NFS_V3, NFS3_FSINFO, cb, private_data, (xdrproc_t)xdr_FSINFO3res, sizeof(FSINFO3res));
	if (pdu == NULL) {
		rpc_set_error(rpc, "Out of memory. Failed to allocate pdu for nfs/fsinfo call");
		return -1;
	}

	args.fsroot.data.data_len = fh->data.data_len; 
	args.fsroot.data.data_val = fh->data.data_val; 

	if (xdr_FSINFO3args(&pdu->xdr, &args) == 0) {
		rpc_set_error(rpc, "XDR error: Failed to encode FSINFO3args");
		rpc_free_pdu(rpc, pdu);
		return -2;
	}

	if (rpc_queue_pdu(rpc, pdu) != 0) {
		rpc_set_error(rpc, "Out of memory. Failed to queue pdu for nfs/fsinfo call");
		rpc_free_pdu(rpc, pdu);
		return -3;
	}

	return 0;
}

int rpc_nfs_readlink_async(struct rpc_context *rpc, rpc_cb cb, READLINK3args *args, void *private_data)
{
	struct rpc_pdu *pdu;

	pdu = rpc_allocate_pdu(rpc, NFS_PROGRAM, NFS_V3, NFS3_READLINK, cb, private_data, (xdrproc_t)xdr_READLINK3res, sizeof(READLINK3res));
	if (pdu == NULL) {
		rpc_set_error(rpc, "Out of memory. Failed to allocate pdu for nfs/readlink call");
		return -1;
	}

	if (xdr_READLINK3args(&pdu->xdr, args) == 0) {
		rpc_set_error(rpc, "XDR error: Failed to encode READLINK3args");
		rpc_free_pdu(rpc, pdu);
		return -2;
	}

	if (rpc_queue_pdu(rpc, pdu) != 0) {
		rpc_set_error(rpc, "Out of memory. Failed to queue pdu for nfs/readlink call");
		rpc_free_pdu(rpc, pdu);
		return -3;
	}

	return 0;
}


int rpc_nfs_symlink_async(struct rpc_context *rpc, rpc_cb cb, SYMLINK3args *args, void *private_data)
{
	struct rpc_pdu *pdu;

	pdu = rpc_allocate_pdu(rpc, NFS_PROGRAM, NFS_V3, NFS3_SYMLINK, cb, private_data, (xdrproc_t)xdr_SYMLINK3res, sizeof(SYMLINK3res));
	if (pdu == NULL) {
		rpc_set_error(rpc, "Out of memory. Failed to allocate pdu for nfs/symlink call");
		return -1;
	}

	if (xdr_SYMLINK3args(&pdu->xdr, args) == 0) {
		rpc_set_error(rpc, "XDR error: Failed to encode SYMLINK3args");
		rpc_free_pdu(rpc, pdu);
		return -2;
	}

	if (rpc_queue_pdu(rpc, pdu) != 0) {
		rpc_set_error(rpc, "Out of memory. Failed to queue pdu for nfs/symlink call");
		rpc_free_pdu(rpc, pdu);
		return -3;
	}

	return 0;
}




int rpc_nfs_rename_async(struct rpc_context *rpc, rpc_cb cb, struct nfs_fh3 *olddir, char *oldname, struct nfs_fh3 *newdir, char *newname, void *private_data)
{
	struct rpc_pdu *pdu;
	RENAME3args args;

	pdu = rpc_allocate_pdu(rpc, NFS_PROGRAM, NFS_V3, NFS3_RENAME, cb, private_data, (xdrproc_t)xdr_RENAME3res, sizeof(RENAME3res));
	if (pdu == NULL) {
		rpc_set_error(rpc, "Out of memory. Failed to allocate pdu for nfs/rename call");
		return -1;
	}

	memset(&args, 0, sizeof(RENAME3args));
	args.from.dir.data.data_len = olddir->data.data_len;
	args.from.dir.data.data_val = olddir->data.data_val;
	args.from.name = oldname;
	args.to.dir.data.data_len = newdir->data.data_len;
	args.to.dir.data.data_val = newdir->data.data_val;
	args.to.name = newname;

	if (xdr_RENAME3args(&pdu->xdr, &args) == 0) {
		rpc_set_error(rpc, "XDR error: Failed to encode RENAME3args");
		rpc_free_pdu(rpc, pdu);
		return -2;
	}

	if (rpc_queue_pdu(rpc, pdu) != 0) {
		rpc_set_error(rpc, "Out of memory. Failed to queue pdu for nfs/rename call");
		rpc_free_pdu(rpc, pdu);
		return -3;
	}

	return 0;
}




int rpc_nfs_link_async(struct rpc_context *rpc, rpc_cb cb, struct nfs_fh3 *file, struct nfs_fh3 *newdir, char *newname, void *private_data)
{
	struct rpc_pdu *pdu;
	LINK3args args;

	pdu = rpc_allocate_pdu(rpc, NFS_PROGRAM, NFS_V3, NFS3_LINK, cb, private_data, (xdrproc_t)xdr_LINK3res, sizeof(LINK3res));
	if (pdu == NULL) {
		rpc_set_error(rpc, "Out of memory. Failed to allocate pdu for nfs/link call");
		return -1;
	}

	memset(&args, 0, sizeof(LINK3args));
	args.file.data.data_len = file->data.data_len;
	args.file.data.data_val = file->data.data_val;
	args.link.dir.data.data_len = newdir->data.data_len;
	args.link.dir.data.data_val = newdir->data.data_val;
	args.link.name = newname;

	if (xdr_LINK3args(&pdu->xdr, &args) == 0) {
		rpc_set_error(rpc, "XDR error: Failed to encode LINK3args");
		rpc_free_pdu(rpc, pdu);
		return -2;
	}

	if (rpc_queue_pdu(rpc, pdu) != 0) {
		rpc_set_error(rpc, "Out of memory. Failed to queue pdu for nfs/link call");
		rpc_free_pdu(rpc, pdu);
		return -3;
	}

	return 0;
}





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
#endif/*WIN32*/

#include <stdio.h>
#include <errno.h>
#include <rpc/rpc.h>
#include <rpc/xdr.h>
#include "libnfs.h"
#include "libnfs-raw.h"
#include "libnfs-private.h"
#include "libnfs-raw-mount.h"

int rpc_mount_null_async(struct rpc_context *rpc, rpc_cb cb, void *private_data)
{
	struct rpc_pdu *pdu;

	pdu = rpc_allocate_pdu(rpc, MOUNT_PROGRAM, MOUNT_V3, MOUNT3_NULL, cb, private_data, (xdrproc_t)xdr_void, 0);
	if (pdu == NULL) {
		rpc_set_error(rpc, "Out of memory. Failed to allocate pdu for mount/null call");
		return -1;
	}

	if (rpc_queue_pdu(rpc, pdu) != 0) {
		rpc_set_error(rpc, "Out of memory. Failed to queue pdu for mount/null call");
		rpc_free_pdu(rpc, pdu);
		return -1;
	}

	return 0;
}

int rpc_mount_mnt_async(struct rpc_context *rpc, rpc_cb cb, char *export, void *private_data)
{
	struct rpc_pdu *pdu;

	pdu = rpc_allocate_pdu(rpc, MOUNT_PROGRAM, MOUNT_V3, MOUNT3_MNT, cb, private_data, (xdrproc_t)xdr_mountres3, sizeof(mountres3));
	if (pdu == NULL) {
		rpc_set_error(rpc, "Out of memory. Failed to allocate pdu for mount/mnt call");
		return -1;
	}

	if (xdr_dirpath(&pdu->xdr, &export) == 0) {
		rpc_set_error(rpc, "XDR error. Failed to encode mount/mnt call");
		rpc_free_pdu(rpc, pdu);
		return -1;
	}

	if (rpc_queue_pdu(rpc, pdu) != 0) {
		rpc_set_error(rpc, "Out of memory. Failed to queue pdu for mount/mnt call");
		rpc_free_pdu(rpc, pdu);
		return -1;
	}

	return 0;
}

int rpc_mount_dump_async(struct rpc_context *rpc, rpc_cb cb, void *private_data)
{
	struct rpc_pdu *pdu;

	pdu = rpc_allocate_pdu(rpc, MOUNT_PROGRAM, MOUNT_V3, MOUNT3_DUMP, cb, private_data, (xdrproc_t)xdr_mountlist, sizeof(mountlist));
	if (pdu == NULL) {
		rpc_set_error(rpc, "Failed to allocate pdu for mount/dump");
		return -1;
	}

	if (rpc_queue_pdu(rpc, pdu) != 0) {
		rpc_set_error(rpc, "Failed to queue mount/dump pdu");
		rpc_free_pdu(rpc, pdu);
		return -1;
	}

	return 0;
}

int rpc_mount_umnt_async(struct rpc_context *rpc, rpc_cb cb, char *export, void *private_data)
{
	struct rpc_pdu *pdu;

	pdu = rpc_allocate_pdu(rpc, MOUNT_PROGRAM, MOUNT_V3, MOUNT3_UMNT, cb, private_data, (xdrproc_t)xdr_void, 0);
	if (pdu == NULL) {
		rpc_set_error(rpc, "Failed to allocate pdu for mount/umnt");
		return -1;
	}

	if (xdr_dirpath(&pdu->xdr, &export) == 0) {
		rpc_set_error(rpc, "failed to encode dirpath for mount/umnt");
		rpc_free_pdu(rpc, pdu);
		return -1;
	}

	if (rpc_queue_pdu(rpc, pdu) != 0) {
		rpc_set_error(rpc, "Failed to queue mount/umnt pdu");
		rpc_free_pdu(rpc, pdu);
		return -1;
	}

	return 0;
}

int rpc_mount_umntall_async(struct rpc_context *rpc, rpc_cb cb, void *private_data)
{
	struct rpc_pdu *pdu;

	pdu = rpc_allocate_pdu(rpc, MOUNT_PROGRAM, MOUNT_V3, MOUNT3_UMNTALL, cb, private_data, (xdrproc_t)xdr_void, 0);
	if (pdu == NULL) {
		rpc_set_error(rpc, "Failed to allocate pdu for mount/umntall");
		return -1;
	}

	if (rpc_queue_pdu(rpc, pdu) != 0) {
		rpc_set_error(rpc, "Failed to queue mount/umntall pdu");
		rpc_free_pdu(rpc, pdu);
		return -1;
	}

	return 0;
}

int rpc_mount_export_async(struct rpc_context *rpc, rpc_cb cb, void *private_data)
{
	struct rpc_pdu *pdu;

	pdu = rpc_allocate_pdu(rpc, MOUNT_PROGRAM, MOUNT_V3, MOUNT3_EXPORT, cb, private_data, (xdrproc_t)xdr_exports, sizeof(exports));
	if (pdu == NULL) {
		rpc_set_error(rpc, "Failed to allocate pdu for mount/export");
		return -1;
	}

	if (rpc_queue_pdu(rpc, pdu) != 0) {
		rpc_set_error(rpc, "Failed to queue mount/export pdu");
		rpc_free_pdu(rpc, pdu);
		return -1;
	}

	return 0;
}

char *mountstat3_to_str(int st)
{
	enum mountstat3 stat = st;

	char *str = "unknown mount stat";
	switch (stat) {
	case MNT3_OK:		  str="MNT3_OK"; break;
	case MNT3ERR_PERM:	  str="MNT3ERR_PERM"; break;
	case MNT3ERR_NOENT:	  str="MNT3ERR_NOENT"; break;
	case MNT3ERR_IO:	  str="MNT3ERR_IO"; break;
	case MNT3ERR_ACCES:	  str="MNT3ERR_ACCES"; break;
	case MNT3ERR_NOTDIR:	  str="MNT3ERR_NOTDIR"; break;
	case MNT3ERR_INVAL:	  str="MNT3ERR_INVAL"; break;
	case MNT3ERR_NAMETOOLONG: str="MNT3ERR_NAMETOOLONG"; break;
	case MNT3ERR_NOTSUPP:	  str="MNT3ERR_NOTSUPP"; break;
	case MNT3ERR_SERVERFAULT: str="MNT3ERR_SERVERFAULT"; break;
	}
	return str;
}


int mountstat3_to_errno(int st)
{
	enum mountstat3 stat = st;

	switch (stat) {
	case MNT3_OK:		  return 0; break;
	case MNT3ERR_PERM:	  return -EPERM; break;
	case MNT3ERR_NOENT:	  return -EPERM; break;
	case MNT3ERR_IO:	  return -EIO; break;
	case MNT3ERR_ACCES:	  return -EACCES; break;
	case MNT3ERR_NOTDIR:	  return -ENOTDIR; break;
	case MNT3ERR_INVAL:	  return -EINVAL; break;
	case MNT3ERR_NAMETOOLONG: return -E2BIG; break;
	case MNT3ERR_NOTSUPP:	  return -EINVAL; break;
	case MNT3ERR_SERVERFAULT: return -EIO; break;
	}
	return -ERANGE;
}




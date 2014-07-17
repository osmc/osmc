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


int rpc_nfsacl_null_async(struct rpc_context *rpc, rpc_cb cb, void *private_data)
{
	struct rpc_pdu *pdu;

	pdu = rpc_allocate_pdu(rpc, NFSACL_PROGRAM, NFSACL_V3, NFSACL3_NULL, cb, private_data, (xdrproc_t)xdr_void, 0);
	if (pdu == NULL) {
		rpc_set_error(rpc, "Out of memory. Failed to allocate pdu for nfsacl/null call");
		return -1;
	}

	if (rpc_queue_pdu(rpc, pdu) != 0) {
		rpc_set_error(rpc, "Out of memory. Failed to queue pdu for nfsacl/null call");
		rpc_free_pdu(rpc, pdu);
		return -2;
	}

	return 0;
}


int rpc_nfsacl_getacl_async(struct rpc_context *rpc, rpc_cb cb, struct nfs_fh3 *fh, uint32_t mask, void *private_data)
{
	struct rpc_pdu *pdu;
	GETACL3args args;

	pdu = rpc_allocate_pdu(rpc, NFSACL_PROGRAM, NFSACL_V3, NFSACL3_GETACL, cb, private_data, (xdrproc_t)xdr_GETACL3res, sizeof(GETACL3res));
	if (pdu == NULL) {
		rpc_set_error(rpc, "Out of memory. Failed to allocate pdu for nfsacl/getacl call");
		return -1;
	}

	args.dir.data.data_len = fh->data.data_len;
	args.dir.data.data_val = fh->data.data_val;
	args.mask = mask;	

	if (xdr_GETACL3args(&pdu->xdr, &args) == 0) {
		rpc_set_error(rpc, "XDR error: Failed to encode GETACL3args");
		rpc_free_pdu(rpc, pdu);
		return -2;
	}

	if (rpc_queue_pdu(rpc, pdu) != 0) {
		rpc_set_error(rpc, "Out of memory. Failed to queue pdu for nfsacl/getacl call");
		rpc_free_pdu(rpc, pdu);
		return -2;
	}

	return 0;
}

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
#include <sys/stat.h>
#include <rpc/rpc.h>
#include <rpc/xdr.h>
#include "libnfs.h"
#include "libnfs-raw.h"
#include "libnfs-private.h"
#include "libnfs-raw-rquota.h"



char *rquotastat_to_str(int error)
{
	switch (error) {
	case RQUOTA_OK:      return "RQUOTA_OK"; break;
	case RQUOTA_NOQUOTA: return "RQUOTA_NOQUOTA"; break;
	case RQUOTA_EPERM:   return "RQUOTA_EPERM"; break;
	};
	return "unknown rquota error";
}

int rquotastat_to_errno(int error)
{
	switch (error) {
	case RQUOTA_OK:           return 0; break;
	case RQUOTA_NOQUOTA:      return -ENOENT; break;
	case RQUOTA_EPERM:        return -EPERM; break;
	};
	return -ENOENT;
}


int rpc_rquota1_null_async(struct rpc_context *rpc, rpc_cb cb, void *private_data)
{
	struct rpc_pdu *pdu;

	pdu = rpc_allocate_pdu(rpc, RQUOTA_PROGRAM, RQUOTA_V1, RQUOTA1_NULL, cb, private_data, (xdrproc_t)xdr_void, 0);
	if (pdu == NULL) {
		rpc_set_error(rpc, "Out of memory. Failed to allocate pdu for rquota1/null call");
		return -1;
	}

	if (rpc_queue_pdu(rpc, pdu) != 0) {
		rpc_set_error(rpc, "Out of memory. Failed to queue pdu for rquota1/null call");
		rpc_free_pdu(rpc, pdu);
		return -2;
	}

	return 0;
}

int rpc_rquota1_getquota_async(struct rpc_context *rpc, rpc_cb cb, char *export, int uid, void *private_data)
{
	struct rpc_pdu *pdu;
	GETQUOTA1args args;

	pdu = rpc_allocate_pdu(rpc, RQUOTA_PROGRAM, RQUOTA_V1, RQUOTA1_GETQUOTA, cb, private_data, (xdrproc_t)xdr_GETQUOTA1res, sizeof(GETQUOTA1res));
	if (pdu == NULL) {
		rpc_set_error(rpc, "Out of memory. Failed to allocate pdu for rquota1/getquota call");
		return -1;
	}

	args.export = export;
	args.uid    = uid;

	if (xdr_GETQUOTA1args(&pdu->xdr, &args) == 0) {
		rpc_set_error(rpc, "XDR error: Failed to encode GETQUOTA1args");
		rpc_free_pdu(rpc, pdu);
		return -2;
	}

	if (rpc_queue_pdu(rpc, pdu) != 0) {
		rpc_set_error(rpc, "Out of memory. Failed to queue pdu for rquota1/getquota call");
		rpc_free_pdu(rpc, pdu);
		return -3;
	}

	return 0;
}

int rpc_rquota1_getactivequota_async(struct rpc_context *rpc, rpc_cb cb, char *export, int uid, void *private_data)
{
	struct rpc_pdu *pdu;
	GETQUOTA1args args;

	pdu = rpc_allocate_pdu(rpc, RQUOTA_PROGRAM, RQUOTA_V1, RQUOTA1_GETACTIVEQUOTA, cb, private_data, (xdrproc_t)xdr_GETQUOTA1res, sizeof(GETQUOTA1res));
	if (pdu == NULL) {
		rpc_set_error(rpc, "Out of memory. Failed to allocate pdu for rquota1/getactivequota call");
		return -1;
	}

	args.export = export;
	args.uid    = uid;

	if (xdr_GETQUOTA1args(&pdu->xdr, &args) == 0) {
		rpc_set_error(rpc, "XDR error: Failed to encode GETQUOTA1args");
		rpc_free_pdu(rpc, pdu);
		return -2;
	}

	if (rpc_queue_pdu(rpc, pdu) != 0) {
		rpc_set_error(rpc, "Out of memory. Failed to queue pdu for rquota1/getactivequota call");
		rpc_free_pdu(rpc, pdu);
		return -3;
	}

	return 0;
}


int rpc_rquota2_null_async(struct rpc_context *rpc, rpc_cb cb, void *private_data)
{
	struct rpc_pdu *pdu;

	pdu = rpc_allocate_pdu(rpc, RQUOTA_PROGRAM, RQUOTA_V2, RQUOTA2_NULL, cb, private_data, (xdrproc_t)xdr_void, 0);
	if (pdu == NULL) {
		rpc_set_error(rpc, "Out of memory. Failed to allocate pdu for rquota2/null call");
		return -1;
	}

	if (rpc_queue_pdu(rpc, pdu) != 0) {
		rpc_set_error(rpc, "Out of memory. Failed to queue pdu for rquota2/null call");
		rpc_free_pdu(rpc, pdu);
		return -2;
	}

	return 0;
}

int rpc_rquota2_getquota_async(struct rpc_context *rpc, rpc_cb cb, char *export, int type, int uid, void *private_data)
{
	struct rpc_pdu *pdu;
	GETQUOTA2args args;

	pdu = rpc_allocate_pdu(rpc, RQUOTA_PROGRAM, RQUOTA_V2, RQUOTA2_GETQUOTA, cb, private_data, (xdrproc_t)xdr_GETQUOTA1res, sizeof(GETQUOTA1res));
	if (pdu == NULL) {
		rpc_set_error(rpc, "Out of memory. Failed to allocate pdu for rquota2/getquota call");
		return -1;
	}

	args.export  = export;
	args.type    = type;
	args.uid     = uid;

	if (xdr_GETQUOTA2args(&pdu->xdr, &args) == 0) {
		rpc_set_error(rpc, "XDR error: Failed to encode GETQUOTA2args");
		rpc_free_pdu(rpc, pdu);
		return -2;
	}

	if (rpc_queue_pdu(rpc, pdu) != 0) {
		rpc_set_error(rpc, "Out of memory. Failed to queue pdu for rquota2/getquota call");
		rpc_free_pdu(rpc, pdu);
		return -3;
	}

	return 0;
}

int rpc_rquota2_getactivequota_async(struct rpc_context *rpc, rpc_cb cb, char *export, int type, int uid, void *private_data)
{
	struct rpc_pdu *pdu;
	GETQUOTA2args args;

	pdu = rpc_allocate_pdu(rpc, RQUOTA_PROGRAM, RQUOTA_V2, RQUOTA2_GETACTIVEQUOTA, cb, private_data, (xdrproc_t)xdr_GETQUOTA1res, sizeof(GETQUOTA1res));
	if (pdu == NULL) {
		rpc_set_error(rpc, "Out of memory. Failed to allocate pdu for rquota2/getactivequota call");
		return -1;
	}

	args.export  = export;
	args.type    = type;
	args.uid     = uid;

	if (xdr_GETQUOTA2args(&pdu->xdr, &args) == 0) {
		rpc_set_error(rpc, "XDR error: Failed to encode GETQUOTA2args");
		rpc_free_pdu(rpc, pdu);
		return -2;
	}

	if (rpc_queue_pdu(rpc, pdu) != 0) {
		rpc_set_error(rpc, "Out of memory. Failed to queue pdu for rquota2/getactivequota call");
		rpc_free_pdu(rpc, pdu);
		return -3;
	}

	return 0;
}

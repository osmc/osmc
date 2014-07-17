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
#include <rpc/rpc.h>
#include <rpc/xdr.h>
#include "libnfs.h"
#include "libnfs-raw.h"
#include "libnfs-private.h"
#include "libnfs-raw-portmap.h"


int rpc_pmap_null_async(struct rpc_context *rpc, rpc_cb cb, void *private_data)
{
	struct rpc_pdu *pdu;

	pdu = rpc_allocate_pdu(rpc, PMAP_PROGRAM, PMAP_V2, PMAP_NULL, cb, private_data, (xdrproc_t)xdr_void, 0);
	if (pdu == NULL) {
		rpc_set_error(rpc, "Out of memory. Failed to allocate pdu for portmap/null call");
		return -1;
	}

	if (rpc_queue_pdu(rpc, pdu) != 0) {
		rpc_set_error(rpc, "Out of memory. Failed to queue pdu for portmap/null call");
		rpc_free_pdu(rpc, pdu);
		return -1;
	}

	return 0;
}

int rpc_pmap_getport_async(struct rpc_context *rpc, int program, int version, int protocol, rpc_cb cb, void *private_data)
{
	struct rpc_pdu *pdu;
	struct pmap_mapping m;

	pdu = rpc_allocate_pdu(rpc, PMAP_PROGRAM, PMAP_V2, PMAP_GETPORT, cb, private_data, (xdrproc_t)xdr_int, sizeof(uint32_t));
	if (pdu == NULL) {
		rpc_set_error(rpc, "Out of memory. Failed to allocate pdu for portmap/getport call");
		return -1;
	}

	m.prog = program;
	m.vers = version;
	m.prot = protocol;
	m.port = 0;
	if (xdr_pmap_mapping(&pdu->xdr, &m) == 0) {
		rpc_set_error(rpc, "XDR error: Failed to encode data for portmap/getport call");
		rpc_free_pdu(rpc, pdu);
		return -1;
	}

	if (rpc_queue_pdu(rpc, pdu) != 0) {
		rpc_set_error(rpc, "Failed to queue portmap/getport pdu");
		rpc_free_pdu(rpc, pdu);
		return -1;
	}

	return 0;
}

int rpc_pmap_set_async(struct rpc_context *rpc, int program, int version, int protocol, int port, rpc_cb cb, void *private_data)
{
	struct rpc_pdu *pdu;
	struct pmap_mapping m;

	pdu = rpc_allocate_pdu(rpc, PMAP_PROGRAM, PMAP_V2, PMAP_SET, cb, private_data, (xdrproc_t)xdr_int, sizeof(uint32_t));
	if (pdu == NULL) {
		rpc_set_error(rpc, "Out of memory. Failed to allocate pdu for portmap/set call");
		return -1;
	}

	m.prog = program;
	m.vers = version;
	m.prot = protocol;
	m.port = port;
	if (xdr_pmap_mapping(&pdu->xdr, &m) == 0) {
		rpc_set_error(rpc, "XDR error: Failed to encode data for portmap/set call");
		rpc_free_pdu(rpc, pdu);
		return -1;
	}

	if (rpc_queue_pdu(rpc, pdu) != 0) {
		rpc_set_error(rpc, "Failed to queue portmap/set pdu");
		rpc_free_pdu(rpc, pdu);
		return -1;
	}

	return 0;
}

int rpc_pmap_unset_async(struct rpc_context *rpc, int program, int version, int protocol, int port, rpc_cb cb, void *private_data)
{
	struct rpc_pdu *pdu;
	struct pmap_mapping m;

	pdu = rpc_allocate_pdu(rpc, PMAP_PROGRAM, PMAP_V2, PMAP_UNSET, cb, private_data, (xdrproc_t)xdr_int, sizeof(uint32_t));
	if (pdu == NULL) {
		rpc_set_error(rpc, "Out of memory. Failed to allocate pdu for portmap/unset call");
		return -1;
	}

	m.prog = program;
	m.vers = version;
	m.prot = protocol;
	m.port = port;
	if (xdr_pmap_mapping(&pdu->xdr, &m) == 0) {
		rpc_set_error(rpc, "XDR error: Failed to encode data for portmap/unset call");
		rpc_free_pdu(rpc, pdu);
		return -1;
	}

	if (rpc_queue_pdu(rpc, pdu) != 0) {
		rpc_set_error(rpc, "Failed to queue portmap/unset pdu");
		rpc_free_pdu(rpc, pdu);
		return -1;
	}

	return 0;
}

int rpc_pmap_callit_async(struct rpc_context *rpc, int program, int version, int procedure, const char *data, int datalen, rpc_cb cb, void *private_data)
{
	struct rpc_pdu *pdu;
	struct pmap_call_args ca;

	pdu = rpc_allocate_pdu(rpc, PMAP_PROGRAM, PMAP_V2, PMAP_CALLIT, cb, private_data, (xdrproc_t)xdr_pmap_call_result, sizeof(pmap_call_result));
	if (pdu == NULL) {
		rpc_set_error(rpc, "Out of memory. Failed to allocate pdu for portmap/callit call");
		return -1;
	}

	ca.prog = program;
	ca.vers = version;
	ca.proc = procedure;
	ca.args.args_len = datalen;
	ca.args.args_val = data;

	if (xdr_pmap_call_args(&pdu->xdr, &ca) == 0) {
		rpc_set_error(rpc, "XDR error: Failed to encode data for portmap/callit call");
		rpc_free_pdu(rpc, pdu);
		return -1;
	}

	if (rpc_queue_pdu(rpc, pdu) != 0) {
		rpc_set_error(rpc, "Failed to queue portmap/callit pdu: %s", rpc_get_error(rpc));
		return -1;
	}

	return 0;
}

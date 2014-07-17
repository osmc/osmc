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
#ifndef MSG_DONTWAIT
#define MSG_DONTWAIT 0
#endif
#else
#include <strings.h>
#endif/*WIN32*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <rpc/rpc.h>
#include <rpc/xdr.h>
#include <rpc/rpc_msg.h>
#include "slist.h"
#include "libnfs.h"
#include "libnfs-raw.h"
#include "libnfs-private.h"

struct rpc_pdu *rpc_allocate_pdu(struct rpc_context *rpc, int program, int version, int procedure, rpc_cb cb, void *private_data, xdrproc_t xdr_decode_fn, int xdr_decode_bufsize)
{
	struct rpc_pdu *pdu;
	struct rpc_msg msg;

	if (rpc == NULL) {
		return NULL;
	}

	pdu = malloc(sizeof(struct rpc_pdu));
	if (pdu == NULL) {
		rpc_set_error(rpc, "Out of memory: Failed to allocate pdu structure");
		return NULL;
	}
	memset(pdu, 0, sizeof(struct rpc_pdu));
	pdu->xid                = rpc->xid++;
	pdu->cb                 = cb;
	pdu->private_data       = private_data;
	pdu->xdr_decode_fn      = xdr_decode_fn;
	pdu->xdr_decode_bufsize = xdr_decode_bufsize;

	xdrmem_create(&pdu->xdr, rpc->encodebuf, rpc->encodebuflen, XDR_ENCODE);
	if (rpc->is_udp == 0) {
		xdr_setpos(&pdu->xdr, 4); /* skip past the record marker */
	}

	memset(&msg, 0, sizeof(struct rpc_msg));
	msg.rm_xid = pdu->xid;
        msg.rm_direction = CALL;
	msg.rm_call.cb_rpcvers = RPC_MSG_VERSION;
	msg.rm_call.cb_prog = program;
	msg.rm_call.cb_vers = version;
	msg.rm_call.cb_proc = procedure;
	msg.rm_call.cb_cred = rpc->auth->ah_cred;
	msg.rm_call.cb_verf = rpc->auth->ah_verf;

	if (xdr_callmsg(&pdu->xdr, &msg) == 0) {
		rpc_set_error(rpc, "xdr_callmsg failed");
		xdr_destroy(&pdu->xdr);
		free(pdu);
		return NULL;
	}

	return pdu;
}

void rpc_free_pdu(struct rpc_context *rpc _U_, struct rpc_pdu *pdu)
{
	if (pdu->outdata.data != NULL) {
		free(pdu->outdata.data);
		pdu->outdata.data = NULL;
	}

	if (pdu->xdr_decode_buf != NULL) {
		xdr_free(pdu->xdr_decode_fn, pdu->xdr_decode_buf);
		free(pdu->xdr_decode_buf);
		pdu->xdr_decode_buf = NULL;
	}

	xdr_destroy(&pdu->xdr);

	free(pdu);
}


int rpc_queue_pdu(struct rpc_context *rpc, struct rpc_pdu *pdu)
{
	int size, recordmarker;

	size = xdr_getpos(&pdu->xdr);

	/* for udp we dont queue, we just send it straight away */
	if (rpc->is_udp != 0) {
		if (sendto(rpc->fd, rpc->encodebuf, size, MSG_DONTWAIT, rpc->udp_dest, sizeof(struct sockaddr_in)) < 0) {
			rpc_set_error(rpc, "Sendto failed with errno %s", strerror(errno));
			rpc_free_pdu(rpc, pdu);
			return -1;
		}
		SLIST_ADD_END(&rpc->waitpdu, pdu);
		return 0;
	}

	/* write recordmarker */
	xdr_setpos(&pdu->xdr, 0);
	recordmarker = (size - 4) | 0x80000000;
	xdr_int(&pdu->xdr, &recordmarker);

	pdu->outdata.size = size;
	pdu->outdata.data = malloc(pdu->outdata.size);
	if (pdu->outdata.data == NULL) {
		rpc_set_error(rpc, "Out of memory. Failed to allocate buffer for pdu\n");
		rpc_free_pdu(rpc, pdu);
		return -1;
	}

	memcpy(pdu->outdata.data, rpc->encodebuf, pdu->outdata.size);
	SLIST_ADD_END(&rpc->outqueue, pdu);

	return 0;
}

int rpc_get_pdu_size(char *buf)
{
	uint32_t size;

	size = ntohl(*(uint32_t *)buf);

	return (size & 0x7fffffff) + 4;
}

static int rpc_process_reply(struct rpc_context *rpc, struct rpc_pdu *pdu, XDR *xdr)
{
	struct rpc_msg msg;

	memset(&msg, 0, sizeof(struct rpc_msg));
	msg.acpted_rply.ar_verf = _null_auth;
	if (pdu->xdr_decode_bufsize > 0) {
		if (pdu->xdr_decode_buf != NULL) {
			free(pdu->xdr_decode_buf);
		}
		pdu->xdr_decode_buf = malloc(pdu->xdr_decode_bufsize);
		if (pdu->xdr_decode_buf == NULL) {
			rpc_set_error(rpc, "xdr_replymsg failed in portmap_getport_reply");
			pdu->cb(rpc, RPC_STATUS_ERROR, "Failed to allocate buffer for decoding of XDR reply", pdu->private_data);
			return 0;
		}
		memset(pdu->xdr_decode_buf, 0, pdu->xdr_decode_bufsize);
	}
	msg.acpted_rply.ar_results.where = pdu->xdr_decode_buf;
	msg.acpted_rply.ar_results.proc  = pdu->xdr_decode_fn;

	if (xdr_replymsg(xdr, &msg) == 0) {
		rpc_set_error(rpc, "xdr_replymsg failed in portmap_getport_reply");
		pdu->cb(rpc, RPC_STATUS_ERROR, "Message rejected by server", pdu->private_data);
		if (pdu->xdr_decode_buf != NULL) {
			free(pdu->xdr_decode_buf);
			pdu->xdr_decode_buf = NULL;
		}
		return 0;
	}
	if (msg.rm_reply.rp_stat != MSG_ACCEPTED) {
		pdu->cb(rpc, RPC_STATUS_ERROR, "RPC Packet not accepted by the server", pdu->private_data);
		return 0;
	}
	switch (msg.rm_reply.rp_acpt.ar_stat) {
	case SUCCESS:
		pdu->cb(rpc, RPC_STATUS_SUCCESS, pdu->xdr_decode_buf, pdu->private_data);
		break;
	case PROG_UNAVAIL:
		pdu->cb(rpc, RPC_STATUS_ERROR, "Server responded: Program not available", pdu->private_data);
		break;
	case PROG_MISMATCH:
		pdu->cb(rpc, RPC_STATUS_ERROR, "Server responded: Program version mismatch", pdu->private_data);
		break;
	case PROC_UNAVAIL:
		pdu->cb(rpc, RPC_STATUS_ERROR, "Server responded: Procedure not available", pdu->private_data);
		break;
	case GARBAGE_ARGS:
		pdu->cb(rpc, RPC_STATUS_ERROR, "Server responded: Garbage arguments", pdu->private_data);
		break;
	case SYSTEM_ERR:
		pdu->cb(rpc, RPC_STATUS_ERROR, "Server responded: System Error", pdu->private_data);
		break;
	default:
		pdu->cb(rpc, RPC_STATUS_ERROR, "Unknown rpc response from server", pdu->private_data);
		break;
	}

	return 0;
}

int rpc_process_pdu(struct rpc_context *rpc, char *buf, int size)
{
	struct rpc_pdu *pdu;
	XDR xdr;
	int pos, recordmarker = 0;
	unsigned int xid;
	char *reasbuf = NULL;

	memset(&xdr, 0, sizeof(XDR));

	xdrmem_create(&xdr, buf, size, XDR_DECODE);
	if (rpc->is_udp == 0) {
		if (xdr_int(&xdr, &recordmarker) == 0) {
			rpc_set_error(rpc, "xdr_int reading recordmarker failed");
			xdr_destroy(&xdr);
			return -1;
		}
		if (!(recordmarker&0x80000000)) {
			xdr_destroy(&xdr);
			if (rpc_add_fragment(rpc, buf+4, size-4) != 0) {
				rpc_set_error(rpc, "Failed to queue fragment for reassembly.");
				return -1;
			}
			return 0;
		}
	}

	/* reassembly */
	if (recordmarker != 0 && rpc->fragments != NULL) {
		struct rpc_fragment *fragment;
		uint64_t total = size - 4;
		char *ptr;

		xdr_destroy(&xdr);
		for (fragment = rpc->fragments; fragment; fragment = fragment->next) {
			total += fragment->size;
		}

		reasbuf = malloc(total);
		if (reasbuf == NULL) {
			rpc_set_error(rpc, "Failed to reassemble PDU");
			rpc_free_all_fragments(rpc);
			return -1;
		}
		ptr = reasbuf;
		for (fragment = rpc->fragments; fragment; fragment = fragment->next) {
			memcpy(ptr, fragment->data, fragment->size);
			ptr += fragment->size;
		}
		memcpy(ptr, buf + 4, size - 4);
		xdrmem_create(&xdr, reasbuf, total, XDR_DECODE);
		rpc_free_all_fragments(rpc);
	}

	pos = xdr_getpos(&xdr);
	if (xdr_int(&xdr, (int *)&xid) == 0) {
		rpc_set_error(rpc, "xdr_int reading xid failed");
		xdr_destroy(&xdr);
		if (reasbuf != NULL) {
			free(reasbuf);
		}
		return -1;
	}
	xdr_setpos(&xdr, pos);

	for (pdu=rpc->waitpdu; pdu; pdu=pdu->next) {
		if (pdu->xid != xid) {
			continue;
		}
		if (rpc->is_udp == 0 || rpc->is_broadcast == 0) {
			SLIST_REMOVE(&rpc->waitpdu, pdu);
		}
		if (rpc_process_reply(rpc, pdu, &xdr) != 0) {
			rpc_set_error(rpc, "rpc_procdess_reply failed");
		}
		xdr_destroy(&xdr);
		if (rpc->is_udp == 0 || rpc->is_broadcast == 0) {
			rpc_free_pdu(rpc, pdu);
		}
		if (reasbuf != NULL) {
			free(reasbuf);
		}
		return 0;
	}
	rpc_set_error(rpc, "No matching pdu found for xid:%d", xid);
	xdr_destroy(&xdr);
	if (reasbuf != NULL) {
		free(reasbuf);
	}
	return -1;
}


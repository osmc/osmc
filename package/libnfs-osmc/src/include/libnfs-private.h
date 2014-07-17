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
#include <rpc/xdr.h>
#include <rpc/auth.h>

struct rpc_fragment {
	struct rpc_fragment *next;
	uint64_t size;
	char *data;
};

struct rpc_context {
	int fd;
	int is_connected;

	char *error_string;

	rpc_cb connect_cb;
	void *connect_data;

	AUTH *auth;
	unsigned long xid;

       /* buffer used for encoding RPC PDU */
       char *encodebuf;
       int encodebuflen;

       struct rpc_pdu *outqueue;
       struct sockaddr_storage udp_src;
       struct rpc_pdu *waitpdu;

       int inpos;
       int insize;
       char *inbuf;

       /* special fields for UDP, which can sometimes be BROADCASTed */
       int is_udp;
       struct sockaddr *udp_dest;
       int is_broadcast;

       /* track the address we connect to so we can auto-reconnect on session failure */
       struct sockaddr_storage s;
       int auto_reconnect;

	/* fragment reassembly */
	struct rpc_fragment *fragments;
};

struct rpc_pdu {
	struct rpc_pdu *next;

	unsigned long xid;
	XDR xdr;

	int written;
	struct rpc_data outdata;

	rpc_cb cb;
	void *private_data;

	/* function to decode the xdr reply data and buffer to decode into */
	xdrproc_t xdr_decode_fn;
	caddr_t xdr_decode_buf;
	int xdr_decode_bufsize;
};

struct rpc_pdu *rpc_allocate_pdu(struct rpc_context *rpc, int program, int version, int procedure, rpc_cb cb, void *private_data, xdrproc_t xdr_decode_fn, int xdr_bufsize);
void rpc_free_pdu(struct rpc_context *rpc, struct rpc_pdu *pdu);
int rpc_queue_pdu(struct rpc_context *rpc, struct rpc_pdu *pdu);
int rpc_get_pdu_size(char *buf);
int rpc_process_pdu(struct rpc_context *rpc, char *buf, int size);
void rpc_error_all_pdus(struct rpc_context *rpc, char *error);

void rpc_set_error(struct rpc_context *rpc, char *error_string, ...);
void nfs_set_error(struct nfs_context *nfs, char *error_string, ...);

struct rpc_context *nfs_get_rpc_context(struct nfs_context *nfs);
const char *nfs_get_server(struct nfs_context *nfs);
const char *nfs_get_export(struct nfs_context *nfs);

/* we dont want to expose UDP to normal applications/users  this is private to libnfs to use exclusively for broadcast RPC */
int rpc_bind_udp(struct rpc_context *rpc, char *addr, int port);
int rpc_set_udp_destination(struct rpc_context *rpc, char *addr, int port, int is_broadcast);
struct rpc_context *rpc_init_udp_context(void);
struct sockaddr *rpc_get_recv_sockaddr(struct rpc_context *rpc);

void rpc_set_autoreconnect(struct rpc_context *rpc);
void rpc_unset_autoreconnect(struct rpc_context *rpc);

int rpc_add_fragment(struct rpc_context *rpc, char *data, uint64_t size);
void rpc_free_all_fragments(struct rpc_context *rpc);

const struct nfs_fh3 *nfs_get_rootfh(struct nfs_context *nfs);

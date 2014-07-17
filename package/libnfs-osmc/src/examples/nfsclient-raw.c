/* 
   Copyright (C) by Ronnie Sahlberg <ronniesahlberg@gmail.com> 2010
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, see <http://www.gnu.org/licenses/>.
*/

/* Example program using the lowlevel raw interface.
 * This allow accurate control of the exact commands that are being used.
 */

#ifdef WIN32
#include "win32_compat.h"
#else
#include <poll.h>
#endif
#define SERVER "10.1.1.27"
#define EXPORT "/shared"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libnfs.h"
#include "libnfs-raw.h"
#include "libnfs-raw-mount.h"
#include "libnfs-raw-nfs.h"
#include "libnfs-raw-rquota.h"

struct client {
       char *server;
       char *export;
       uint32_t mount_port;
       uint32_t rquota_port;
       int is_finished;
       struct nfs_fh3 rootfh;
};

void rquota_getquota_cb(struct rpc_context *rpc _U_, int status, void *data, void *private_data)
{
	struct client *client = private_data;
//	GETQUOTA1res *res = data;

	if (status == RPC_STATUS_ERROR) {
		printf("rquota/getquota call failed with \"%s\"\n", (char *)data);
		exit(10);
	}
	if (status != RPC_STATUS_SUCCESS) {
		printf("rquota/getquota call to server %s failed, status:%d\n", client->server, status);
		exit(10);
	}

	printf("rquota responded ok\n");
	client->is_finished = 1;
}

void rquota_connect_cb(struct rpc_context *rpc, int status, void *data _U_, void *private_data)
{
	struct client *client = private_data;

	if (status != RPC_STATUS_SUCCESS) {
		printf("connection to RPC.RQUOTAD on server %s failed\n", client->server);
		exit(10);
	}

	printf("Connected to RPC.RQUOTAD on %s:%d\n", client->server, client->rquota_port);
	printf("Send GETQUOTA request for uid 100\n");
	if (rpc_rquota1_getquota_async(rpc, rquota_getquota_cb, EXPORT, 100, client) != 0) {
		printf("Failed to send fsinfo request\n");
		exit(10);
	}
}

void acl_getacl_cb(struct rpc_context *rpc _U_, int status, void *data, void *private_data)
{
	struct client *client = private_data;
	GETACL3res *res = data;

	printf("Got NFSACL/GETACL reply\n");

	if (status == RPC_STATUS_SUCCESS) {
		printf("Got an ACL :  ACL status:%d\n", res->status);
		if (res->status == NFS3_OK) {
			int i;
			printf("ACL MASK 0x%08x\n", res->GETACL3res_u.resok.mask);
			printf("NUM ACE %d\n", res->GETACL3res_u.resok.ace_count);
			for (i=0; i<res->GETACL3res_u.resok.ace.ace_len; i++) {
				printf("Type:0x%08x\n", res->GETACL3res_u.resok.ace.ace_val[i].type);
				printf("ID:%d\n", res->GETACL3res_u.resok.ace.ace_val[i].id);
				printf("Perm:0x%08x\n", res->GETACL3res_u.resok.ace.ace_val[i].perm);
			}
		}
	}

	printf("Disconnect socket from nfs server\n");
	if (rpc_disconnect(rpc, "normal disconnect") != 0) {
		printf("Failed to disconnect socket to nfs\n");
		exit(10);
	}

	printf("Connect to RPC.RQUOTAD on %s:%d\n", client->server, client->rquota_port);
	if (rpc_connect_async(rpc, client->server, client->rquota_port, rquota_connect_cb, client) != 0) {
		printf("Failed to start connection\n");
		exit(10);
	}
}

void acl_null_cb(struct rpc_context *rpc _U_, int status, void *data, void *private_data)
{
	struct client *client = private_data;

	printf("Got NFSACL/NULL reply\n");
	printf("Get ACL for root handle\n");

	if (rpc_nfsacl_getacl_async(rpc, acl_getacl_cb, &client->rootfh, NFSACL_MASK_ACL_ENTRY|NFSACL_MASK_ACL_COUNT|NFSACL_MASK_ACL_DEFAULT_ENTRY|NFSACL_MASK_ACL_DEFAULT_COUNT, client) != 0) {
		printf("Failed to send getacl request\n");
		exit(10);
	}

}

void nfs_fsinfo_cb(struct rpc_context *rpc _U_, int status, void *data, void *private_data)
{
	struct client *client = private_data;
	FSINFO3res *res = data;

	if (status == RPC_STATUS_ERROR) {
		printf("nfs/fsinfo call failed with \"%s\"\n", (char *)data);
		exit(10);
	}
	if (status != RPC_STATUS_SUCCESS) {
		printf("nfs/fsinfo call to server %s failed, status:%d\n", client->server, status);
		exit(10);
	}

	printf("Got reply from server for NFS/FSINFO procedure.\n");
	printf("Read Max:%d\n", (int)res->FSINFO3res_u.resok.rtmax);
	printf("Write Max:%d\n", (int)res->FSINFO3res_u.resok.wtmax);

	printf("Send NFSACL/NULL request\n");
	if (rpc_nfsacl_null_async(rpc, acl_null_cb, client) != 0) {
		printf("Failed to send acl/null request\n");
		exit(10);
	}
}


void nfs_connect_cb(struct rpc_context *rpc, int status, void *data _U_, void *private_data)
{
	struct client *client = private_data;

	if (status != RPC_STATUS_SUCCESS) {
		printf("connection to RPC.MOUNTD on server %s failed\n", client->server);
		exit(10);
	}

	printf("Connected to RPC.NFSD on %s:%d\n", client->server, client->mount_port);
	printf("Send FSINFO request\n");
	if (rpc_nfs_fsinfo_async(rpc, nfs_fsinfo_cb, &client->rootfh, client) != 0) {
		printf("Failed to send fsinfo request\n");
		exit(10);
	}
}

void mount_mnt_cb(struct rpc_context *rpc, int status, void *data, void *private_data)
{
	struct client *client = private_data;
	mountres3 *mnt = data;

	if (status == RPC_STATUS_ERROR) {
		printf("mount/mnt call failed with \"%s\"\n", (char *)data);
		exit(10);
	}
	if (status != RPC_STATUS_SUCCESS) {
		printf("mount/mnt call to server %s failed, status:%d\n", client->server, status);
		exit(10);
	}

	printf("Got reply from server for MOUNT/MNT procedure.\n");
	client->rootfh.data.data_len = mnt->mountres3_u.mountinfo.fhandle.fhandle3_len;
        client->rootfh.data.data_val = malloc(client->rootfh.data.data_len);
	memcpy(client->rootfh.data.data_val, mnt->mountres3_u.mountinfo.fhandle.fhandle3_val, client->rootfh.data.data_len);

	printf("Disconnect socket from mountd server\n");
	if (rpc_disconnect(rpc, "normal disconnect") != 0) {
		printf("Failed to disconnect socket to mountd\n");
		exit(10);
	}

	printf("Connect to RPC.NFSD on %s:%d\n", client->server, 2049);
	if (rpc_connect_async(rpc, client->server, 2049, nfs_connect_cb, client) != 0) {
		printf("Failed to start connection\n");
		exit(10);
	}
}



void mount_export_cb(struct rpc_context *rpc, int status, void *data, void *private_data)
{
	struct client *client = private_data;
	exports export = *(exports *)data;

	if (status == RPC_STATUS_ERROR) {
		printf("mount null call failed with \"%s\"\n", (char *)data);
		exit(10);
	}
	if (status != RPC_STATUS_SUCCESS) {
		printf("mount null call to server %s failed, status:%d\n", client->server, status);
		exit(10);
	}

	printf("Got reply from server for MOUNT/EXPORT procedure.\n");
	while (export != NULL) {
	      printf("Export: %s\n", export->ex_dir);
	      export = export->ex_next;
	}
	printf("Send MOUNT/MNT command for %s\n", client->export);
	if (rpc_mount_mnt_async(rpc, mount_mnt_cb, client->export, client) != 0) {
		printf("Failed to send mnt request\n");
		exit(10);
	}
}

void mount_null_cb(struct rpc_context *rpc, int status, void *data, void *private_data)
{
	struct client *client = private_data;

	if (status == RPC_STATUS_ERROR) {
		printf("mount null call failed with \"%s\"\n", (char *)data);
		exit(10);
	}
	if (status != RPC_STATUS_SUCCESS) {
		printf("mount null call to server %s failed, status:%d\n", client->server, status);
		exit(10);
	}

	printf("Got reply from server for MOUNT/NULL procedure.\n");
	printf("Send MOUNT/EXPORT command\n");
	if (rpc_mount_export_async(rpc, mount_export_cb, client) != 0) {
		printf("Failed to send export request\n");
		exit(10);
	}
}

void mount_connect_cb(struct rpc_context *rpc, int status, void *data _U_, void *private_data)
{
	struct client *client = private_data;

	if (status != RPC_STATUS_SUCCESS) {
		printf("connection to RPC.MOUNTD on server %s failed\n", client->server);
		exit(10);
	}

	printf("Connected to RPC.MOUNTD on %s:%d\n", client->server, client->mount_port);
	printf("Send NULL request to check if RPC.MOUNTD is actually running\n");
	if (rpc_mount_null_async(rpc, mount_null_cb, client) != 0) {
		printf("Failed to send null request\n");
		exit(10);
	}
}


void pmap_getport2_cb(struct rpc_context *rpc, int status, void *data, void *private_data)
{
	struct client *client = private_data;

	if (status == RPC_STATUS_ERROR) {
		printf("portmapper getport call failed with \"%s\"\n", (char *)data);
		exit(10);
	}
       	if (status != RPC_STATUS_SUCCESS) {
		printf("portmapper getport call to server %s failed, status:%d\n", client->server, status);
		exit(10);
	}

	client->mount_port = *(uint32_t *)data;
	printf("GETPORT returned RPC.MOUNTD is on port:%d\n", client->mount_port);
	if (client->mount_port == 0) {
		printf("RPC.MOUNTD is not available on server : %s:%d\n", client->server, client->mount_port);
		exit(10);
	}		

	printf("Disconnect socket from portmap server\n");
	if (rpc_disconnect(rpc, "normal disconnect") != 0) {
		printf("Failed to disconnect socket to portmapper\n");
		exit(10);
	}

	printf("Connect to RPC.MOUNTD on %s:%d\n", client->server, client->mount_port);
	if (rpc_connect_async(rpc, client->server, client->mount_port, mount_connect_cb, client) != 0) {
		printf("Failed to start connection\n");
		exit(10);
	}
}

void pmap_getport1_cb(struct rpc_context *rpc, int status, void *data, void *private_data)
{
	struct client *client = private_data;

	if (status == RPC_STATUS_ERROR) {
		printf("portmapper getport call failed with \"%s\"\n", (char *)data);
		exit(10);
	}
       	if (status != RPC_STATUS_SUCCESS) {
		printf("portmapper getport call to server %s failed, status:%d\n", client->server, status);
		exit(10);
	}

	client->rquota_port = *(uint32_t *)data;
	printf("GETPORT returned RPC.RQUOTAD on port:%d\n", client->rquota_port);
	if (client->rquota_port == 0) {
		printf("RPC.RQUOTAD is not available on server : %s:%d\n", client->server, client->rquota_port);
//		exit(10);
	}		

	printf("Send getport request asking for MOUNT port\n");
	if (rpc_pmap_getport_async(rpc, MOUNT_PROGRAM, MOUNT_V3, IPPROTO_TCP, pmap_getport2_cb, client) != 0) {
		printf("Failed to send getport request\n");
		exit(10);
	}
}

void pmap_null_cb(struct rpc_context *rpc, int status, void *data, void *private_data)
{
	struct client *client = private_data;

	if (status == RPC_STATUS_ERROR) {
		printf("portmapper null call failed with \"%s\"\n", (char *)data);
		exit(10);
	}
	if (status != RPC_STATUS_SUCCESS) {
		printf("portmapper null call to server %s failed, status:%d\n", client->server, status);
		exit(10);
	}

	printf("Got reply from server for PORTMAP/NULL procedure.\n");
	printf("Send getport request asking for MOUNT port\n");
	if (rpc_pmap_getport_async(rpc, RQUOTA_PROGRAM, RQUOTA_V1, IPPROTO_TCP, pmap_getport1_cb, client) != 0) {
		printf("Failed to send getport request\n");
		exit(10);
	}
}

void pmap_connect_cb(struct rpc_context *rpc, int status, void *data _U_, void *private_data)
{
	struct client *client = private_data;

	printf("pmap_connect_cb    status:%d.\n", status);
	if (status != RPC_STATUS_SUCCESS) {
		printf("connection to portmapper on server %s failed\n", client->server);
		exit(10);
	}

	printf("Send NULL request to check if portmapper is actually running\n");
	if (rpc_pmap_null_async(rpc, pmap_null_cb, client) != 0) {
		printf("Failed to send null request\n");
		exit(10);
	}
}


int main(int argc _U_, char *argv[] _U_)
{
	struct rpc_context *rpc;
	struct pollfd pfd;
	struct client client;

	rpc = rpc_init_context();
	if (rpc == NULL) {
		printf("failed to init context\n");
		exit(10);
	}

	client.server = SERVER;
	client.export = EXPORT;
	client.is_finished = 0;
	if (rpc_connect_async(rpc, client.server, 111, pmap_connect_cb, &client) != 0) {
		printf("Failed to start connection\n");
		exit(10);
	}

	for (;;) {
		pfd.fd = rpc_get_fd(rpc);
		pfd.events = rpc_which_events(rpc);

		if (poll(&pfd, 1, -1) < 0) {
			printf("Poll failed");
			exit(10);
		}
		if (rpc_service(rpc, pfd.revents) < 0) {
			printf("rpc_service failed\n");
			break;
		}
		if (client.is_finished) {
			break;
		}
	}
	
	rpc_destroy_context(rpc);
	rpc=NULL;
	printf("nfsclient finished\n");
	return 0;
}

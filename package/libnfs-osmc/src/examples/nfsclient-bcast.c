/* 
   Copyright (C) by Ronnie Sahlberg <ronniesahlberg@gmail.com> 2011
   
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

/* Example program using the lowlevel raw broadcast interface.
 */

#include "config.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <net/if.h>
#include <netdb.h>
#include "libnfs.h"
#include "libnfs-raw.h"
#include "libnfs-raw-mount.h"
#include "libnfs-raw-portmap.h"

struct nfs_list_data {
       int status;
       struct nfs_server_list *srvrs;
};

void free_nfs_srvr_list(struct nfs_server_list *srv)
{
	while (srv != NULL) {
		struct nfs_server_list *next = srv->next;

		free(srv->addr);
		free(srv);
		srv = next;
	}
}	     

void pm_cb(struct rpc_context *rpc, int status, void *data _U_, void *private_data)
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

int main(int argc _U_, char *argv[] _U_)
{
	struct rpc_context *rpc;
	struct pollfd pfd;
	struct ifconf ifc;
	int size;
	struct timeval tv_start, tv_current;
	struct nfs_list_data data = {0, NULL};
	struct nfs_server_list *srvr;
	char *ptr;
	
	rpc = rpc_init_udp_context();
	if (rpc == NULL) {
		printf("failed to init context\n");
		exit(10);
	}

	if (rpc_bind_udp(rpc, "0.0.0.0", 0) < 0) {
		printf("failed to bind to udp %s\n", rpc_get_error(rpc));
		exit(10);
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
			printf("ioctl SIOCGIFCONF failed\n");
			exit(10);
		}
	}	

	for (ptr =(char *)ifc.ifc_buf; ptr < ((char *)ifc.ifc_buf) + ifc.ifc_len; ) {
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
			printf("ioctl DRBADDR failed\n");
			exit(10);
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
			printf("failed to set udp destination %s\n", rpc_get_error(rpc));
			exit(10);
		}

		if (rpc_pmap_callit_async(rpc, MOUNT_PROGRAM, 2, 0, NULL, 0, pm_cb, &data) < 0) {
			printf("Failed to set up callit function\n");
			exit(10);
		}
	}
	free(ifc.ifc_buf);	

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
			printf("Poll failed");
			exit(10);
		}
		if (pfd.revents == 0) {
			break;
		}
		
		if (rpc_service(rpc, pfd.revents) < 0) {
			printf("rpc_service failed with %s\n", rpc_get_error(rpc));
			break;
		}
	}

	for (srvr=data.srvrs; srvr; srvr = srvr->next) {
		printf("NFS SERVER @ %s\n", srvr->addr);
	}
	free_nfs_srvr_list(data.srvrs);

	rpc_destroy_context(rpc);
	rpc=NULL;
	return 0;
}

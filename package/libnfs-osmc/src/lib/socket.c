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
#include <unistd.h>
#include <poll.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netdb.h>
#endif/*WIN32*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <rpc/rpc.h>
#include <rpc/xdr.h>
#ifdef HAVE_SYS_FILIO_H
#include <sys/filio.h>
#endif
#ifdef HAVE_SYS_SOCKIO_H
#include <sys/sockio.h>
#endif
#include <sys/types.h>
#include "libnfs.h"
#include "libnfs-raw.h"
#include "libnfs-private.h"
#include "slist.h"

#ifdef WIN32
//has to be included after stdlib!!
#include "win32_errnowrapper.h"
#endif


static int rpc_reconnect_requeue(struct rpc_context *rpc);
static int rpc_connect_sockaddr_async(struct rpc_context *rpc, struct sockaddr_storage *s);

static void set_nonblocking(int fd)
{
	int v = 0;
#if defined(WIN32)
	long nonblocking=1;
	v = ioctlsocket(fd, FIONBIO,&nonblocking);
#else
	v = fcntl(fd, F_GETFL, 0);
        fcntl(fd, F_SETFL, v | O_NONBLOCK);
#endif //FIXME
}

int rpc_get_fd(struct rpc_context *rpc)
{
	return rpc->fd;
}

int rpc_which_events(struct rpc_context *rpc)
{
	int events = rpc->is_connected ? POLLIN : POLLOUT;

	if (rpc->is_udp != 0) {
		/* for udp sockets we only wait for pollin */
		return POLLIN;
	}

	if (rpc->outqueue) {
		events |= POLLOUT;
	}
	return events;
}

static int rpc_write_to_socket(struct rpc_context *rpc)
{
	int64_t count;

	if (rpc == NULL) {
		return -1;
	}
	if (rpc->fd == -1) {
		rpc_set_error(rpc, "trying to write but not connected");
		return -1;
	}

	while (rpc->outqueue != NULL) {
		int64_t total;

		total = rpc->outqueue->outdata.size;

#if defined(WIN32)
		count = send(rpc->fd, rpc->outqueue->outdata.data + rpc->outqueue->written, total - rpc->outqueue->written, 0);
#else
		count = write(rpc->fd, rpc->outqueue->outdata.data + rpc->outqueue->written, total - rpc->outqueue->written);
#endif
		if (count == -1) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				return 0;
			}
			rpc_set_error(rpc, "Error when writing to socket :%s(%d)", strerror(errno), errno);
			return -1;
		}

		rpc->outqueue->written += count;
		if (rpc->outqueue->written == total) {
			struct rpc_pdu *pdu = rpc->outqueue;

	       	    	SLIST_REMOVE(&rpc->outqueue, pdu);
			SLIST_ADD_END(&rpc->waitpdu, pdu);
		}
	}
	return 0;
}

static int rpc_read_from_socket(struct rpc_context *rpc)
{
	int available;
	int size;
	int pdu_size;
	int64_t count;

#if defined(WIN32)
	if (ioctlsocket(rpc->fd, FIONREAD, &available) != 0) {
#else
	if (ioctl(rpc->fd, FIONREAD, &available) != 0) {
#endif
		rpc_set_error(rpc, "Ioctl FIONREAD returned error : %d. Closing socket.", errno);
		return -1;
	}

	if (available == 0) {
		rpc_set_error(rpc, "Socket has been closed");
		return -1;
	}

	if (rpc->is_udp) {
		char *buf;
		socklen_t socklen = sizeof(rpc->udp_src);

		buf = malloc(available);
		if (buf == NULL) {
			rpc_set_error(rpc, "Failed to malloc buffer for recvfrom");
			return -1;
		}
		count = recvfrom(rpc->fd, buf, available, MSG_DONTWAIT, (struct sockaddr *)&rpc->udp_src, &socklen);
		if (count < 0) {
			rpc_set_error(rpc, "Failed recvfrom: %s", strerror(errno));
			free(buf);
		}
		if (rpc_process_pdu(rpc, buf, count) != 0) {
			rpc_set_error(rpc, "Invalid/garbage pdu received from server. Ignoring PDU");
			free(buf);
			return -1;
		}
		free(buf);
		return 0;
	}

	/* read record marker, 4 bytes at the beginning of every pdu */
	if (rpc->inbuf == NULL) {
		rpc->insize = 4;
		rpc->inbuf = malloc(rpc->insize);
		if (rpc->inbuf == NULL) {
			rpc_set_error(rpc, "Failed to allocate buffer for record marker, errno:%d. Closing socket.", errno);
			return -1;
		}
	}
	if (rpc->inpos < 4) {
		size = 4 - rpc->inpos;

#if defined(WIN32)
		count = recv(rpc->fd, rpc->inbuf + rpc->inpos, size, 0);
#else
		count = read(rpc->fd, rpc->inbuf + rpc->inpos, size);
#endif
		if (count == -1) {
			if (errno == EINTR) {
				return 0;
			}
			rpc_set_error(rpc, "Read from socket failed, errno:%d. Closing socket.", errno);
			return -1;
		}
		available  -= count;
		rpc->inpos += count;
	}

	if (available == 0) {
		return 0;
	}

	pdu_size = rpc_get_pdu_size(rpc->inbuf);
	if (rpc->insize < pdu_size) {
		unsigned char *buf;
		
		buf = malloc(pdu_size);
		if (buf == NULL) {
			rpc_set_error(rpc, "Failed to allocate buffer of %d bytes for pdu, errno:%d. Closing socket.", pdu_size, errno);
			return -1;
		}
		memcpy(buf, rpc->inbuf, rpc->insize);
		free(rpc->inbuf);
		rpc->inbuf  = buf;
		rpc->insize = rpc_get_pdu_size(rpc->inbuf);
	}

	size = available;
	if (size > rpc->insize - rpc->inpos) {
		size = rpc->insize - rpc->inpos;
	}

#if defined(WIN32)
	count = recv(rpc->fd, rpc->inbuf + rpc->inpos, size, 0);
#else
	count = read(rpc->fd, rpc->inbuf + rpc->inpos, size);
#endif
	if (count == -1) {
		if (errno == EINTR) {
			return 0;
		}
		rpc_set_error(rpc, "Read from socket failed, errno:%d. Closing socket.", errno);
		return -1;
	}
	available  -= count;
	rpc->inpos += count;

	if (rpc->inpos == rpc->insize) {
		if (rpc_process_pdu(rpc, rpc->inbuf, pdu_size) != 0) {
			rpc_set_error(rpc, "Invalid/garbage pdu received from server. Closing socket");
			return -1;
		}
		free(rpc->inbuf);
		rpc->inbuf  = NULL;
		rpc->insize = 0;
		rpc->inpos  = 0;
	}

	return 0;
}



int rpc_service(struct rpc_context *rpc, int revents)
{
	if (revents & POLLERR) {
#ifdef WIN32
		char err = 0;
#else
		int err = 0;
#endif
		socklen_t err_size = sizeof(err);

		if (getsockopt(rpc->fd, SOL_SOCKET, SO_ERROR,
				(char *)&err, &err_size) != 0 || err != 0) {
			if (err == 0) {
				err = errno;
			}
			rpc_set_error(rpc, "rpc_service: socket error "
					       "%s(%d).",
					       strerror(err), err);
		} else {
			rpc_set_error(rpc, "rpc_service: POLLERR, "
						"Unknown socket error.");
		}
		if (rpc->connect_cb != NULL) {
			rpc->connect_cb(rpc, RPC_STATUS_ERROR, rpc->error_string, rpc->connect_data);
		}
		return -1;
	}
	if (revents & POLLHUP) {
		rpc_set_error(rpc, "Socket failed with POLLHUP");
		if (rpc->connect_cb != NULL) {
			rpc->connect_cb(rpc, RPC_STATUS_ERROR, rpc->error_string, rpc->connect_data);
		}
		return -1;
	}

	if (rpc->is_connected == 0 && rpc->fd != -1 && revents&POLLOUT) {
		int err = 0;
		socklen_t err_size = sizeof(err);

		if (getsockopt(rpc->fd, SOL_SOCKET, SO_ERROR,
				(char *)&err, &err_size) != 0 || err != 0) {
			if (err == 0) {
				err = errno;
			}
			rpc_set_error(rpc, "rpc_service: socket error "
				  	"%s(%d) while connecting.",
					strerror(err), err);
			if (rpc->connect_cb != NULL) {
				rpc->connect_cb(rpc, RPC_STATUS_ERROR,
					NULL, rpc->connect_data);
			}
			return -1;
		}

		rpc->is_connected = 1;
		if (rpc->connect_cb != NULL) {
			rpc->connect_cb(rpc, RPC_STATUS_SUCCESS, NULL, rpc->connect_data);
		}
		return 0;
	}

	if (revents & POLLIN) {
		if (rpc_read_from_socket(rpc) != 0) {
		  	rpc_reconnect_requeue(rpc);
			return 0;
		}
	}

	if (revents & POLLOUT && rpc->outqueue != NULL) {
		if (rpc_write_to_socket(rpc) != 0) {
			rpc_set_error(rpc, "write to socket failed");
			return -1;
		}
	}

	return 0;
}

void rpc_set_autoreconnect(struct rpc_context *rpc)
{
	rpc->auto_reconnect = 1;
}

void rpc_unset_autoreconnect(struct rpc_context *rpc)
{
	rpc->auto_reconnect = 0;
}

static int rpc_connect_sockaddr_async(struct rpc_context *rpc, struct sockaddr_storage *s)
{
	int socksize;

	switch (s->ss_family) {
	case AF_INET:
		socksize = sizeof(struct sockaddr_in);
		rpc->fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		break;
	default:
		rpc_set_error(rpc, "Can not handle AF_FAMILY:%d", s->ss_family);
		return -1;
	}

	if (rpc->fd == -1) {
		rpc_set_error(rpc, "Failed to open socket");
		return -1;
	}


#if !defined(WIN32)
	/* Some systems allow you to set capabilities on an executable
	 * to allow the file to be executed with privilege to bind to
	 * privileged system ports, even if the user is not root.
	 *
	 * Opportunistically try to bind the socket to a low numbered
	 * system port in the hope that the user is either root or the
	 * executable has the CAP_NET_BIND_SERVICE.
	 *
	 * As soon as we fail the bind() with EACCES we know we will never
	 * be able to bind to a system port so we terminate the loop.
	 *
	 * On linux, use
	 *    sudo setcap 'cap_net_bind_service=+ep' /path/executable
	 * to make the executable able to bind to a system port.
	 */
	if (1) {
		static int port = 200;
		int i;
		int one = 1;

		setsockopt(rpc->fd, SOL_SOCKET, SO_REUSEADDR, (char *)&one, sizeof(one));

		for (i = 0; i < 500; i++) {
			struct sockaddr_in sin;

			if(++port > 700) port = 200;

			memset(&sin, 0, sizeof(sin));
			sin.sin_port        = htons(port);
			sin.sin_family      = AF_INET;
			sin.sin_addr.s_addr = 0;

			if (bind(rpc->fd, (struct sockaddr *)&sin, sizeof(struct sockaddr_in)) != 0 && errno != EACCES) {
				/* we didnt get EACCES, so try again */
				continue;
			}
			break;
		}
	}
#endif

	set_nonblocking(rpc->fd);

#if defined(WIN32)
	if (connect(rpc->fd, (struct sockaddr *)s, socksize) == 0 && errno != EINPROGRESS   )
#else
	if (connect(rpc->fd, (struct sockaddr *)s, socksize) != 0 && errno != EINPROGRESS) 
#endif
	{
	  rpc_set_error(rpc, "connect() to server failed. %s(%d)", strerror(errno), errno);
		return -1;
	}		

	return 0;
}	    

int rpc_connect_async(struct rpc_context *rpc, const char *server, int port, rpc_cb cb, void *private_data)
{
	struct sockaddr_in *sin = (struct sockaddr_in *)&rpc->s;

	if (rpc->fd != -1) {
		rpc_set_error(rpc, "Trying to connect while already connected");
		return -1;
	}

	if (rpc->is_udp != 0) {
		rpc_set_error(rpc, "Trying to connect on UDP socket");
		return -1;
	}

	rpc->auto_reconnect = 0;

	sin->sin_family = AF_INET;
	sin->sin_port   = htons(port);
	if (inet_pton(AF_INET, server, &sin->sin_addr) != 1) {
		rpc_set_error(rpc, "Not a valid server ip address");
		return -1;
	}


	switch (rpc->s.ss_family) {
	case AF_INET:
#ifdef HAVE_SOCKADDR_LEN
		sin->sin_len = sizeof(struct sockaddr_in);
#endif
		break;
	}

	rpc->connect_cb  = cb;
	rpc->connect_data = private_data;

	if (rpc_connect_sockaddr_async(rpc, &rpc->s) != 0) {
		return -1;
	}

	return 0;
}	    

int rpc_disconnect(struct rpc_context *rpc, char *error)
{
	rpc_unset_autoreconnect(rpc);

	if (rpc->fd != -1) {
#if defined(WIN32)
		closesocket(rpc->fd);
#else
		close(rpc->fd);
#endif
	}
	rpc->fd  = -1;

	rpc->is_connected = 0;

	rpc_error_all_pdus(rpc, error);

	return 0;
}

static void reconnect_cb(struct rpc_context *rpc, int status, void *data _U_, void *private_data)
{
	if (status != RPC_STATUS_SUCCESS) {
		rpc_error_all_pdus(rpc, "RPC ERROR: Failed to reconnect async");
		return;
	}

	rpc->is_connected = 1;
	rpc->connect_cb   = NULL;
}

/* disconnect but do not error all PDUs, just move pdus in-flight back to the outqueue and reconnect */
static int rpc_reconnect_requeue(struct rpc_context *rpc)
{
	struct rpc_pdu *pdu;

	if (rpc->fd != -1) {
#if defined(WIN32)
		closesocket(rpc->fd);
#else
		close(rpc->fd);
#endif
	}
	rpc->fd  = -1;

	rpc->is_connected = 0;

	/* socket is closed so we will not get any replies to any commands
	 * in flight. Move them all over from the waitpdu queue back to the out queue
	 */
	for (pdu=rpc->waitpdu; pdu; pdu=pdu->next) {
		SLIST_REMOVE(&rpc->waitpdu, pdu);
		SLIST_ADD(&rpc->outqueue, pdu);
		/* we have to re-send the whole pdu again */
		pdu->written = 0;
	}

	if (rpc->auto_reconnect != 0) {
		rpc->connect_cb  = reconnect_cb;

		if (rpc_connect_sockaddr_async(rpc, &rpc->s) != 0) {
			rpc_error_all_pdus(rpc, "RPC ERROR: Failed to reconnect async");
			return -1;
		}
	}

	return 0;
}


int rpc_bind_udp(struct rpc_context *rpc, char *addr, int port)
{
	struct addrinfo *ai = NULL;
	char service[6];

	if (rpc->is_udp == 0) {
		rpc_set_error(rpc, "Cant not bind UDP. Not UDP context");
		return -1;
	}

	sprintf(service, "%d", port);
	if (getaddrinfo(addr, service, NULL, &ai) != 0) {
		rpc_set_error(rpc, "Invalid address:%s. "
			"Can not resolv into IPv4/v6 structure.");
		return -1;
 	}

	switch(ai->ai_family) {
	case AF_INET:
		rpc->fd = socket(ai->ai_family, SOCK_DGRAM, 0);
		if (rpc->fd == -1) {
			rpc_set_error(rpc, "Failed to create UDP socket: %s", strerror(errno)); 
			freeaddrinfo(ai);
			return -1;
		}

		if (bind(rpc->fd, (struct sockaddr *)ai->ai_addr, sizeof(struct sockaddr_in)) != 0) {
			rpc_set_error(rpc, "Failed to bind to UDP socket: %s",strerror(errno)); 
			freeaddrinfo(ai);
			return -1;
		}
		break;
	default:
		rpc_set_error(rpc, "Can not handle UPD sockets of family %d yet", ai->ai_family);
		freeaddrinfo(ai);
		return -1;
	}

	freeaddrinfo(ai);

	return 0;
}

int rpc_set_udp_destination(struct rpc_context *rpc, char *addr, int port, int is_broadcast)
{
	struct addrinfo *ai = NULL;
	char service[6];

	if (rpc->is_udp == 0) {
		rpc_set_error(rpc, "Can not set destination sockaddr. Not UDP context");
		return -1;
	}

	sprintf(service, "%d", port);
	if (getaddrinfo(addr, service, NULL, &ai) != 0) {
		rpc_set_error(rpc, "Invalid address:%s. "
			"Can not resolv into IPv4/v6 structure.");
		return -1;
 	}

	if (rpc->udp_dest) {
		free(rpc->udp_dest);
		rpc->udp_dest = NULL;
	}
	rpc->udp_dest = malloc(ai->ai_addrlen);
	if (rpc->udp_dest == NULL) {
		rpc_set_error(rpc, "Out of memory. Failed to allocate sockaddr structure");
		return -1;
	}
	memcpy(rpc->udp_dest, ai->ai_addr, ai->ai_addrlen);
	freeaddrinfo(ai);

	rpc->is_broadcast = is_broadcast;
	setsockopt(rpc->fd, SOL_SOCKET, SO_BROADCAST, (char *)&is_broadcast, sizeof(is_broadcast));

	return 0;
}

struct sockaddr *rpc_get_recv_sockaddr(struct rpc_context *rpc)
{
	return (struct sockaddr *)&rpc->udp_src;
}

int rpc_queue_length(struct rpc_context *rpc)
{
	int i=0;
	struct rpc_pdu *pdu;

	for(pdu = rpc->outqueue; pdu; pdu = pdu->next) {
		i++;
	}
	for(pdu = rpc->waitpdu; pdu; pdu = pdu->next) {
		i++;
	}
	return i;
}

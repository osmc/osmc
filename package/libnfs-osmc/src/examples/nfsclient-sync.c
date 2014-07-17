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

/* Example program using the highlevel sync interface
 */
#ifdef WIN32
#include "win32_compat.h"
#else
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#endif
 
#define SERVER "10.1.1.27"
#define EXPORT "/VIRTUAL"
#define NFSFILE "/BOOKS/Classics/Dracula.djvu.truncated"
#define NFSFILER "/BOOKS/Classics/Dracula.djvu.renamed"
#define NFSFILEW "/BOOKS/Classics/foo"
#define NFSDIR "/BOOKS/Classics/"

#define _GNU_SOURCE

#if defined(WIN32)
#pragma comment(lib, "ws2_32.lib")
WSADATA wsaData;
#else
#include <sys/statvfs.h>
#include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "libnfs.h"
#include <rpc/rpc.h>            /* for authunix_create() */
#include "libnfs-raw.h"
#include "libnfs-raw-mount.h"

struct client {
       char *server;
       char *export;
       uint32_t mount_port;
       int is_finished;
};


void PrintServerList()
{
  struct nfs_server_list *srvrs;
  struct nfs_server_list *srv;

  srvrs = nfs_find_local_servers();

  for (srv=srvrs; srv; srv = srv->next)
  {
      printf("Found nfs server: %s\n", srv->addr);

  }
  free_nfs_srvr_list(srvrs);
}

char buf[3*1024*1024+337];

int main(int argc _U_, char *argv[] _U_)
{
	struct nfs_context *nfs;
	int i, ret;
	uint64_t offset;
	struct client client;
	struct stat st;
	struct nfsfh  *nfsfh;
	struct nfsdir *nfsdir;
	struct nfsdirent *nfsdirent;
	struct statvfs svfs;
	exports export, tmp;

#if defined(WIN32)
	if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
		printf("Failed to start Winsock2\n");
		exit(10);
	}
#endif

	client.server = SERVER;
	client.export = EXPORT;
	client.is_finished = 0;

  PrintServerList();

	export = mount_getexports(SERVER);
	if (export != NULL) {
		printf("exports on server %s\n", SERVER);
		tmp = export;
		while (tmp != NULL) {
		      printf("Export: %s\n", tmp->ex_dir);
		      tmp = tmp->ex_next;
		}

		mount_free_export_list(export);
	} else {
		printf("no exports on server %s\n", SERVER);
	}	

	nfs = nfs_init_context();
	if (nfs == NULL) {
		printf("failed to init context\n");
		exit(10);
	}

	ret = nfs_mount(nfs, client.server, client.export);
	if (ret != 0) {
 		printf("Failed to mount nfs share : %s\n", nfs_get_error(nfs));
		exit(10);
	}
	printf("mounted share successfully %s\n", nfs_get_error(nfs));


	ret = nfs_stat(nfs, NFSFILE, &st);
	if (ret != 0) {
		printf("Failed to stat(%s) %s\n", NFSFILE, nfs_get_error(nfs));
		exit(10);
	}
	printf("Mode %04o\n", st.st_mode);
	printf("Size %d\n", (int)st.st_size);
	printf("Inode %04o\n", (int)st.st_ino);

	ret = nfs_open(nfs, NFSFILE, O_RDONLY, &nfsfh);
	if (ret != 0) {
		printf("Failed to open(%s) %s\n", NFSFILE, nfs_get_error(nfs));
		exit(10);
	}

#if 0
	ret = nfs_read(nfs, nfsfh, 16, buf);
	if (ret < 0) {
		printf("Failed to pread(%s) %s\n", NFSFILE, nfs_get_error(nfs));
		exit(10);
	}
	printf("read %d bytes\n", ret);
	for (i=0;i<16;i++) {
		printf("%02x ", buf[i]&0xff);
	}
	printf("\n");
#endif
	ret = nfs_read(nfs, nfsfh, sizeof(buf), buf);
	if (ret < 0) {
		printf("Failed to pread(%s) %s\n", NFSFILE, nfs_get_error(nfs));
		exit(10);
	}
	printf("read %d bytes\n", ret);
	for (i=0;i<16;i++) {
		printf("%02x ", buf[i]&0xff);
	}
	printf("\n");
	ret = nfs_read(nfs, nfsfh, sizeof(buf), buf);
	if (ret < 0) {
		printf("Failed to pread(%s) %s\n", NFSFILE, nfs_get_error(nfs));
		exit(10);
	}
	printf("read %d bytes\n", ret);
	ret = nfs_read(nfs, nfsfh, sizeof(buf), buf);
	if (ret < 0) {
		printf("Failed to pread(%s) %s\n", NFSFILE, nfs_get_error(nfs));
		exit(10);
	}
	printf("read %d bytes\n", ret);
	ret = nfs_read(nfs, nfsfh, sizeof(buf), buf);
	if (ret < 0) {
		printf("Failed to pread(%s) %s\n", NFSFILE, nfs_get_error(nfs));
		exit(10);
	}
	printf("read %d bytes\n", ret);
	ret = nfs_read(nfs, nfsfh, sizeof(buf), buf);
	if (ret < 0) {
		printf("Failed to pread(%s) %s\n", NFSFILE, nfs_get_error(nfs));
		exit(10);
	}
	printf("read %d bytes\n", ret);
	ret = nfs_read(nfs, nfsfh, sizeof(buf), buf);
	if (ret < 0) {
		printf("Failed to pread(%s) %s\n", NFSFILE, nfs_get_error(nfs));
		exit(10);
	}
	printf("read %d bytes\n", ret);

	ret = (int)nfs_lseek(nfs, nfsfh, 0, SEEK_CUR, &offset);
	if (ret < 0) {
		printf("Failed to lseek(%s) %s\n", NFSFILE, nfs_get_error(nfs));
		exit(10);
	}
	printf("File position is %d\n", (int)offset);

	printf("seek to end of file\n");
	ret = (int)nfs_lseek(nfs, nfsfh, 0, SEEK_END, &offset);
	if (ret < 0) {
		printf("Failed to lseek(%s) %s\n", NFSFILE, nfs_get_error(nfs));
		exit(10);
	}
	printf("File position is %d\n", (int)offset);

	ret = nfs_fstat(nfs, nfsfh, &st);
	if (ret != 0) {
		printf("Failed to stat(%s) %s\n", NFSFILE, nfs_get_error(nfs));
		exit(10);
	}
	printf("Mode %04o\n", st.st_mode);
	printf("Size %d\n", (int)st.st_size);
	printf("Inode %04o\n", (int)st.st_ino);


	ret = nfs_close(nfs, nfsfh);
	if (ret < 0) {
		printf("Failed to close(%s): %s\n", NFSFILE, nfs_get_error(nfs));
		exit(10);
	}

	ret = nfs_opendir(nfs, NFSDIR, &nfsdir);
	if (ret != 0) {
		printf("Failed to open(%s) %s\n", NFSFILE, nfs_get_error(nfs));
		exit(10);
	}
	while((nfsdirent = nfs_readdir(nfs, nfsdir)) != NULL) {
	  char filename[1024];
		printf("Inode:%d Name:%s ", (int)nfsdirent->inode, nfsdirent->name);
		sprintf(filename, "%s/%s", NFSDIR, nfsdirent->name);
		ret = nfs_open(nfs, filename, O_RDONLY, &nfsfh);
		if (ret != 0) {
			printf("Failed to open(%s) %s\n", filename, nfs_get_error(nfs));
			exit(10);
		}
		ret = nfs_read(nfs, nfsfh, sizeof(buf), buf);
		if (ret < 0) {
			printf("Error reading file\n");
		}
		printf("Read %d bytes\n", ret);
		ret = nfs_close(nfs, nfsfh);
		if (ret < 0) {
			printf("Failed to close(%s): %s\n", NFSFILE, nfs_get_error(nfs));
			exit(10);
		}
	}
	nfs_closedir(nfs, nfsdir);


	ret = nfs_open(nfs, NFSFILEW, O_WRONLY, &nfsfh);
	if (ret != 0) {
		printf("Failed to open(%s) %s\n", NFSFILEW, nfs_get_error(nfs));
		exit(10);
	}
	ret = nfs_pwrite(nfs, nfsfh, 0, 16, buf);
	if (ret < 0) {
		printf("Failed to pwrite(%s) %s\n", NFSFILEW, nfs_get_error(nfs));
		exit(10);
	}
	ret = nfs_fsync(nfs, nfsfh);
	if (ret < 0) {
		printf("Failed to fsync(%s) %s\n", NFSFILEW, nfs_get_error(nfs));
		exit(10);
	}
	ret = nfs_close(nfs, nfsfh);
	if (ret < 0) {
		printf("Failed to close(%s) %s\n", NFSFILEW, nfs_get_error(nfs));
		exit(10);
	}


	ret = nfs_statvfs(nfs, NFSDIR, &svfs);
	if (ret < 0) {
		printf("Failed to statvfs(%s) %s\n", NFSDIR, nfs_get_error(nfs));
		exit(10);
	}
	printf("files %d/%d/%d\n", (int)svfs.f_files, (int)svfs.f_ffree, (int)svfs.f_favail);


	ret = nfs_access(nfs, NFSFILE, R_OK);
	if (ret != 0) {
		printf("Failed to access(%s) %s\n", NFSFILE, nfs_get_error(nfs));
	}

	/* become root */
	nfs_set_auth(nfs, authunix_create("Ronnies-Laptop", 0, 0, 0, NULL));

	ret = nfs_link(nfs, NFSFILE, NFSFILER);
	if (ret != 0) {
		printf("Failed to link(%s) %s\n", NFSFILE, nfs_get_error(nfs));
	}


	nfs_destroy_context(nfs);
	printf("nfsclient finished\n");
	return 0;
}


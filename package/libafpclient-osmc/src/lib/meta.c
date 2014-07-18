

/*

    Copyright (C) 2006 Alex deVries <alexthepuffin@gmail.com>

    This program can be distributed under the terms of the GNU GPL.
    See the file COPYING.
*/

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <asm/fcntl.h>
#include <utime.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

#include "afp.h"
#include "dsi.h"
#include "afp_protocol.h"
#include "utils.h"


int afp_meta_getattr(const char *path, struct stat *stbuf)
{
	if (is_apple(path)) 
	{
		log_for_client(NULL,AFPFSD,LOG_DEBUG,
			"Is apple\n");
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink=2;
		return 0;
	}

	resource=apple_translate(volume,path);

	return resource;
}


static int afp_meta_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                         off_t offset, struct fuse_file_info *fi)
{
	(void) offset;
	(void) fi;
	unsigned int dirid=0;
	struct afp_file_info * filebase = NULL, * p, *prev;
	unsigned short reqcount=20;  /* Get them in batches of 20 */
	unsigned long startindex=1;
	int rc=0, ret=0, exit=0;
	unsigned int filebitmap, dirbitmap;
	struct afp_volume * volume=
		(struct afp_volume *)
		((struct fuse_context *)(fuse_get_context()))->private_data;

}


static int afp_meta_open(const char *path, struct fuse_file_info *fi)
{

}


static int afp_meta_read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi)
{

}


static int afp_meta_truncate(const char * path, off_t offset)
{
}


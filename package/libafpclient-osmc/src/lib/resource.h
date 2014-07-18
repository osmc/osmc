#ifndef __RESOURCE_H_
#define __RESOURCE_H_

#define AFP_META_NONE 0
#define AFP_META_RESOURCE 1
#define AFP_META_APPLEDOUBLE 2
#define AFP_META_FINDERINFO 3
#define AFP_META_COMMENT 4
#define AFP_META_SERVER_ICON 5

#include <utime.h>

int appledouble_creat(struct afp_volume * volume, const char * path, mode_t mode);


int appledouble_truncate(struct afp_volume * volume, const char * path, int size) ;

int appledouble_getattr(struct afp_volume * volume,
        const char * path, struct stat *stbuf);


int appledouble_readdir(struct afp_volume * volume,
	const char *path, struct afp_file_info **base);

int appledouble_open(struct afp_volume * volume, const char * path, int flags,
        struct afp_file_info *newfp);

int appledouble_read(struct afp_volume * volume, struct afp_file_info *fp,
	char * buf, size_t size, off_t offset, size_t * amount_read,
	int * eof);

int appledouble_close(struct afp_volume * volume, struct afp_file_info * fp);

int appledouble_chmod(struct afp_volume * volume, const char * path, mode_t mode);

int appledouble_unlink(struct afp_volume * volume, const char *path);

int appledouble_mkdir(struct afp_volume * volume, const char * path, mode_t mode);

int appledouble_readlink(struct afp_volume * volume, const char * path, 
        char * buf, size_t size);

int appledouble_rmdir(struct afp_volume * volume, const char * path);

int appledouble_chown(struct afp_volume * volume, const char * path,
                uid_t uid, gid_t gid);

int appledouble_utime(struct afp_volume * volume, const char * path,
        struct utimbuf * timebuf);

int appledouble_symlink(struct afp_volume *vol, const char *path1, 
	const char *path2);

int appledouble_rename(struct afp_volume * volume, const char * path_from, 
        const char * path_to);




#endif


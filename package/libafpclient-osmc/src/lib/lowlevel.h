#ifndef __LOWLEVEL_H_
#define __LOWLEVEL_H_
int ll_get_directory_entry(struct afp_volume * volume,
        char * basename,
        unsigned int dirid,
        unsigned int filebitmap, unsigned int dirbitmap,
        struct afp_file_info *p);

int ll_readdir(struct afp_volume * volume, const char *path,
        struct afp_file_info **fb, int resource);
int ll_getattr(struct afp_volume * volume, const char *path, struct stat *stbuf,
        int resourcefork);

int ll_zero_file(struct afp_volume * volume, unsigned short forkid,
	unsigned int resource);

int ll_read(struct afp_volume * volume,
	char *buf, size_t size, off_t offset,
	struct afp_file_info *fp, int * eof);

int ll_handle_unlocking(struct afp_volume * volume,unsigned short forkid,
	uint64_t offset, uint64_t sizetorequest);

int ll_handle_locking(struct afp_volume * volume,unsigned short forkid,
	uint64_t offset, uint64_t sizetorequest);

int ll_write(struct afp_volume * volume,
	const char *data, size_t size, off_t offset,
	struct afp_file_info * fp, size_t * totalwritten);

int ll_open(struct afp_volume * volume, const char *path, int flags,
        struct afp_file_info *fp);

#endif


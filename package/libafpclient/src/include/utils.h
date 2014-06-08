#ifndef __UTILS_H_
#define __UTILS_H_
#include <stdio.h>

#include "afp.h"

#if BYTE_ORDER == BIG_ENDIAN
#define hton64(x)       (x)
#define ntoh64(x)       (x)
#else /* BYTE_ORDER == BIG_ENDIAN */
#define hton64(x)       ((u_int64_t) (htonl(((x) >> 32) & 0xffffffffLL)) | \
                         (u_int64_t) ((htonl(x) & 0xffffffffLL) << 32))
#define ntoh64(x)       (hton64(x))
#endif /* BYTE_ORDER == BIG_ENDIAN */

#define min(a,b) (((a)<(b)) ? (a) : (b))
#define max(a,b) (((a)>(b)) ? (a) : (b))



unsigned char unixpath_to_afppath(
        struct afp_server * server,
        char * buf);

unsigned char sizeof_path_header(struct afp_server * server);



unsigned char copy_from_pascal(char *dest, char *pascal,unsigned int max_len) ;
unsigned short copy_from_pascal_two(char *dest, char *pascal,unsigned int max_len);

unsigned char copy_to_pascal(char *dest, const char *src);
unsigned short copy_to_pascal_two(char *dest, const char *src);

void copy_path(struct afp_server * server, char * dest, const char * pathname, unsigned char len);


char * create_path(struct afp_server * server, char * pathname, unsigned short * len);


int invalid_filename(struct afp_server * server, const char * filename);

#endif

#ifndef _AFP_INTERNAL_H_
#define _AFP_INTERNAL_H_

#include "afp.h"

extern struct afp_versions afp_versions[];

/* From netatalk's adouble.h */
#define AD_DATE_DELTA         946684800
#define AD_DATE_FROM_UNIX(x)  (htonl((x) - AD_DATE_DELTA))
#define AD_DATE_TO_UNIX(x)    (ntohl(x) + AD_DATE_DELTA)

void add_file_by_name(struct afp_file_info ** base, const char *filename);

#endif


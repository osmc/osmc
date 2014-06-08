#ifndef __FUSE_ERROR_H_
#define __FUSE_ERROR_H_

#include "fuse_internal.h"

void report_fuse_errors(struct fuse_client * c);
void fuse_capture_stderr_start(void);

#endif


/* (c) 2017 Sam Nazarko
 * email@samnazarko.co.uk */

#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/utsname.h>

#include <stdio.h>
#include <string.h>

int uname(struct utsname *buf)
{
 int r;
 r = syscall(SYS_uname, buf);
 strcpy(buf->machine, TARGET_ARCH);
 return r;
}

 /*
 * checkmodifier.c
 *
 * (c) 2014-2015 Sam Nazarko
 * email@samnazarko.co.uk
 *
 * Return kernel shift_state (depressed modifier keys) bitmask from /dev/console. Requires root access.
 * 0 = no modifier keys
 * 1 = Left/Right Shift
 * 2 = Alt Gr (right ALT)
 * 4 = Left/Right Ctrl
 * 8 = Left Alt
 */

#include <sys/ioctl.h>
#include <asm/ioctls.h>
#include <stdio.h>
#include <fcntl.h>

int main(int argc, char *argv[])
{
    char shift_state;
    int fd = open("/dev/console", O_RDWR);
    if (! fd)
    	return 127;
    /*
     * From console_ioctl:
     *        TIOCLINUX, subcode=6
     *               argp  points  to  a char which is set to the value of the kernel 
     *               variable shift_state.  (Since 1.1.32.)
     */

    shift_state = 6;
    ioctl(fd, TIOCLINUX, &shift_state);
    close(fd);
    return shift_state;
}

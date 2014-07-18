#ifndef __DAEMON_H_
#define __DAEMON_H_


void rm_fd_and_signal(int fd);
void signal_main_thread(void);

int get_debug_mode(void);

int add_client(int fd);
int process_client_fds(fd_set * set, int max_fd, int ** onfd);
int fuse_unmount_volume(struct afp_volume * volume);
void fuse_forced_ending_hook(void);



#endif

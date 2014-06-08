/*
 *  daemon.c
 *
 *  Copyright (C) 2006 Alex deVries
 *
 */

#include <sys/types.h>
#include <sys/param.h>

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <utime.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/un.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdarg.h>
#include <getopt.h>
#include <signal.h>
#include <sys/socket.h>

#include "afp.h"

#include "dsi.h"
#include "afp_server.h"
#include "utils.h"
#include "daemon.h"
#include "commands.h"

#define MAX_ERROR_LEN 1024
#define STATUS_LEN 1024


#define MAX_CLIENT_RESPONSE 2048


static int debug_mode = 0;
static char commandfilename[PATH_MAX];

int get_debug_mode(void) 
{
	return debug_mode;
}

void fuse_forced_ending_hook(void)
{
	struct afp_server * s = get_server_base();
	struct afp_volume * volume;
	int i;

	for (s=get_server_base();s;s=s->next) {
		if (s->connect_state==SERVER_STATE_CONNECTED)
		for (i=0;i<s->num_volumes;i++) {
			volume=&s->volumes[i];
			if (volume->mounted==AFP_VOLUME_MOUNTED)
				log_for_client(NULL,AFPFSD,LOG_NOTICE,
					"Unmounting %s\n",volume->mountpoint);
			afp_unmount_volume(volume);
		}
	}
}

int fuse_unmount_volume(struct afp_volume * volume)
{
	if (volume->priv) {
		fuse_exit((struct fuse *)volume->priv);
		pthread_kill(volume->thread, SIGHUP);
		pthread_join(volume->thread,NULL);
	}
	return 0;
}


static int startup_listener(void) 
{
	int command_fd;
	struct sockaddr_un sa;
	int len;

	if ((command_fd=socket(AF_UNIX,SOCK_STREAM,0)) < 0) {
		goto error;
	}
	memset(&sa,0,sizeof(sa));
	sa.sun_family = AF_UNIX;

	strcpy(sa.sun_path,commandfilename);
	len = sizeof(sa.sun_family) + strlen(sa.sun_path)+1;

	if (bind(command_fd,(struct sockaddr *)&sa,len) < 0)  {
		perror("binding");
		close(command_fd);
		goto error;
	}

	listen(command_fd,5);  /* Just one at a time */

	return command_fd;

error:
	return -1;

}

void close_commands(int command_fd) 
{

	close(command_fd);
	unlink(commandfilename);
}

static void usage(void)
{
	printf("Usage: afpfsd [OPTION]\n"
"  -l, --logmethod    Either 'syslog' or 'stdout'"
"  -f, --foreground   Do not fork\n"
"  -d, --debug        Does not fork, logs to stdout\n"
"Version %s\n", AFPFS_VERSION);
}

static int remove_other_daemon(void)
{

	int sock;
	struct sockaddr_un servaddr;
        int len=0, ret;
        char incoming_buffer[MAX_CLIENT_RESPONSE];
        struct timeval tv;
        fd_set rds;
#define OUTGOING_PACKET_LEN 1
	char outgoing_buffer[OUTGOING_PACKET_LEN];

	if (access(commandfilename,F_OK)!=0) 
		goto doesnotexist; /* file doesn't even exist */

	if ((sock=socket(AF_UNIX,SOCK_STREAM,0))<0) {
		perror("Opening socket");
		goto error;
	}

	memset(&servaddr,0,sizeof(servaddr));
	servaddr.sun_family = AF_UNIX;
	strcpy(servaddr.sun_path,commandfilename);

	if ((connect(sock,(struct sockaddr*) &servaddr,
		sizeof(servaddr.sun_family) +
		sizeof(servaddr.sun_path))) <0) {
		goto dead;
	}

	/* Try writing to it */

	outgoing_buffer[0]=AFP_SERVER_COMMAND_PING;
	if (write(sock, outgoing_buffer,OUTGOING_PACKET_LEN)
		<OUTGOING_PACKET_LEN)
		goto dead;

	/* See if we get a response */
        memset(incoming_buffer,0,MAX_CLIENT_RESPONSE);
        FD_ZERO(&rds);
        FD_SET(sock,&rds);
	tv.tv_sec=10; tv.tv_usec=0;
	ret=select(sock+1,&rds,NULL,NULL,&tv);
	if (ret==0) {
		goto dead; /* Timeout */
	}
	if (ret<0) {
		goto error; /* some sort of select error */
	}

	/* Let's see if we got a sane message back */
	len=read(sock,incoming_buffer,MAX_CLIENT_RESPONSE);

	if (len<1)
		goto dead;

	/* Okay, the server is live */

	close(sock);
	return -1;

dead:
	close(sock);
	/* See if we can remove it */
	if (access(commandfilename,F_OK)==0) {
		if (unlink(commandfilename)!=0) {
			log_for_client(NULL, AFPFSD,LOG_NOTICE,
				"Cannot remove command file");
			return -1;
		}
	}

	return 0;

doesnotexist:
	return 0;

error:
	close(sock);
	return -1;
}


int main(int argc, char *argv[]) {

	int option_index=0;
	struct option long_options[] = {
		{"logmethod",1,0,'l'},
		{"foreground",0,0,'f'},
		{"debug",1,0,'d'},
		{0,0,0,0},
	};
	int new_log_method=LOG_METHOD_SYSLOG;
	int dofork=1;
	/* getopt_long()'s return is int; specifying the variable to contain
	 * this return value as char depends on endian-specific behavior,
	 * breaking utterly on big endian (i.e., PowerPC)
	 */
	int c;
	int optnum;
	int command_fd=-1;

	fuse_register_afpclient();

	if (init_uams()<0) return -1;


	while (1) {
		optnum++;
		c = getopt_long(argc,argv,"l:fdh",
			long_options,&option_index);
		if (c==-1) break;
		switch (c) {
			case 'l':
				if (strncmp(optarg,"stdout",6)==0) 	
					fuse_set_log_method(LOG_METHOD_STDOUT);
				else if (strncmp(optarg,"syslog",6)==0) 	
					fuse_set_log_method(LOG_METHOD_SYSLOG);
				else {
					printf("Unknown log method %s\n",optarg);
					usage();
				}
				break;
			case 'f':
				dofork=0;
				break;
			case 'd':
				dofork=0;
				debug_mode=1;
				new_log_method=LOG_METHOD_STDOUT;
				break;
			case 'h':
			default:
				usage();
				return -1;
		}
	}

	fuse_set_log_method(new_log_method);

	sprintf(commandfilename,"%s-%d",SERVER_FILENAME,(unsigned int) geteuid());

	if (remove_other_daemon()<0)  {
		log_for_client(NULL, AFPFSD,LOG_NOTICE,
			"Daemon is already running and alive\n");
		return -1;
	}

	
	if ((!dofork) || (fork()==0)) {

		if ((command_fd=startup_listener())<0)
			goto error;

		log_for_client(NULL, AFPFSD,LOG_NOTICE,
			"Starting up AFPFS version %s\n",AFPFS_VERSION);

		afp_main_loop(command_fd);
		close_commands(command_fd);
	}



	return 0;

error:
	printf("Could not start afpfsd\n");

	return -1;
}

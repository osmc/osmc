/*
 *  commands.c
 *
 *  Copyright (C) 2006 Alex deVries
 *
 */

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

#include "afp.h"
#include "dsi.h"
#include "afp_server.h"
#include "utils.h"
#include "daemon.h"
#include "uams_def.h"
#include "codepage.h"
#include "libafpclient.h"
#include "map_def.h"
#include "fuse_int.h"
#include "fuse_error.h"
#include "fuse_internal.h"

#ifdef __linux
#define FUSE_DEVICE "/dev/fuse"
#else
#define FUSE_DEVICE "/dev/fuse0"
#endif


static int fuse_log_method=LOG_METHOD_SYSLOG;

void trigger_exit(void);

static struct fuse_client * client_base = NULL;

struct afp_volume * global_volume;

static int volopen(struct fuse_client * c, struct afp_volume * volume);
static int process_command(struct fuse_client * c);
static struct afp_volume * mount_volume(struct fuse_client * c,
	struct afp_server * server, char * volname, char * volpassword) ;

void fuse_set_log_method(int new_method)
{
	fuse_log_method=new_method;
}


static int remove_client(struct fuse_client * toremove) 
{
	struct fuse_client * c, * prev=NULL;

	for (c=client_base;c;c=c->next) {
		if (c==toremove) {
			if (!prev) client_base=NULL;
			else prev->next=toremove->next;
			free(toremove);
			toremove=NULL;
			return 0;
		}
		prev=c;
	}
	return -1;
}

static int fuse_add_client(int fd) 
{
	struct fuse_client * c, *newc;

	if ((newc=malloc(sizeof(*newc)))==NULL) goto error;


	memset(newc,0,sizeof(*newc));
	newc->fd=fd;
	newc->next=NULL;
	if (client_base==NULL) client_base=newc;
	else {
		for (c=client_base;c->next;c=c->next);
		c->next=newc;

	}
	return 0;
error:
	return -1;
}

static int fuse_process_client_fds(fd_set * set, int max_fd)
{

	struct fuse_client * c;

	for (c=client_base;c;c=c->next) {
		if (FD_ISSET(c->fd,set)) {
			if (process_command(c)<0) return -1;
			return 1;
		}
	}
	return 0;

}

static int fuse_scan_extra_fds(int command_fd, fd_set *set, int * max_fd)
{

	struct sockaddr_un new_addr;
	socklen_t new_len = sizeof(struct sockaddr_un);
	int new_fd;



	if (FD_ISSET(command_fd,set)) {
		new_fd=accept(command_fd,(struct sockaddr *) &new_addr,&new_len);
		if (new_fd>=0) {
			fuse_add_client(new_fd);
			FD_SET(new_fd,set);
			if ((new_fd+1) > *max_fd) *max_fd=new_fd+1;
		}
	}

	switch (fuse_process_client_fds(set,*max_fd)) {
	case -1:
		{
			int i;
			FD_CLR(new_fd,set);
			for (i=*max_fd;i>=0;i--)
				if (FD_ISSET(i,set)) {
					*max_fd=i;
					break;
				}
		}

		(*max_fd)++;
		close(new_fd);
		goto out;
	case 1:
		goto out;
	}
	/* unknown fd */
	sleep(10);

	return 0;

out:
	return 1;
}

static void fuse_log_for_client(void * priv,
	enum loglevels loglevel, int logtype, const char *message) {
	int len = 0;
	struct fuse_client * c = priv;

	if (c) {
		len = strlen(c->client_string);
		snprintf(c->client_string+len,
			MAX_CLIENT_RESPONSE-len,
			message);
	} else {

		if (fuse_log_method & LOG_METHOD_SYSLOG)
			syslog(LOG_INFO, "%s", message);
		if (fuse_log_method & LOG_METHOD_STDOUT)
			printf("%s",message);
	}

}

struct start_fuse_thread_arg {
	struct afp_volume * volume;
	struct fuse_client * client;
	int wait;
	int fuse_result;
	int fuse_errno;
	int changeuid;
};

static void * start_fuse_thread(void * other) 
{
	int fuseargc=0;
	const char *fuseargv[200];
#define mountstring_len (AFP_SERVER_NAME_LEN+1+AFP_VOLUME_NAME_LEN+1)
	char mountstring[mountstring_len];
	struct start_fuse_thread_arg * arg = other;
	struct afp_volume * volume = arg->volume;
	struct fuse_client * c = arg->client;
	struct afp_server * server = volume->server;

	/* Check to see if we have permissions to access the mountpoint */

	snprintf(mountstring,mountstring_len,"%s:%s",
		server->server_name_printable,
			volume->volume_name_printable);
	fuseargc=0;
	fuseargv[0]=mountstring;
	fuseargc++;
	fuseargv[1]=volume->mountpoint;
	fuseargc++;
	if (get_debug_mode()) {
		fuseargv[fuseargc]="-d";
		fuseargc++;
	} else {
		fuseargv[fuseargc]="-f";
		fuseargc++;
	}
	
	if (arg->changeuid) {
		fuseargv[fuseargc]="-o";
		fuseargc++;
		fuseargv[fuseargc]="allow_other";
		fuseargc++;
	}


/* #ifdef USE_SINGLE_THREAD */
	fuseargv[fuseargc]="-s";
	fuseargc++;
/*
#endif
*/
	global_volume=volume; 

	arg->fuse_result= 
		afp_register_fuse(fuseargc, (char **) fuseargv,volume);

	arg->fuse_errno=errno;

	arg->wait=0;
	pthread_cond_signal(&volume->startup_condition_cond);

	log_for_client((void *) c,AFPFSD,LOG_WARNING,
		"Unmounting volume %s from %s\n",
		volume->volume_name_printable,
                volume->mountpoint);

	return NULL;
}

static int volopen(struct fuse_client * c, struct afp_volume * volume)
{
	char mesg[1024];
	unsigned int l = 0;	
	memset(mesg,0,1024);
	int rc=afp_connect_volume(volume,volume->server,mesg,&l,1024);

	log_for_client((void *) c,AFPFSD,LOG_ERR,mesg);

	return rc;

}


static unsigned char process_suspend(struct fuse_client * c)
{
	struct afp_server_suspend_request * req =(void *)c->incoming_string+1;
	struct afp_server * s;

	/* Find the server */
	if ((s=find_server_by_name(req->server_name))==NULL) {
		log_for_client((void *) c,AFPFSD,LOG_ERR,
			"%s is an unknown server\n",req->server_name);
		return AFP_SERVER_RESULT_ERROR;
	}

	if (afp_zzzzz(s)) 
		return AFP_SERVER_RESULT_ERROR;

	loop_disconnect(s);
	
	s->connect_state=SERVER_STATE_DISCONNECTED;
	log_for_client((void *) c,AFPFSD,LOG_NOTICE,
		"Disconnected from %s\n",req->server_name);
	return AFP_SERVER_RESULT_OKAY;
}


static int afp_server_reconnect_loud(struct fuse_client * c, struct afp_server * s) 
{
	char mesg[1024];
	unsigned int l = 2040;
	int rc;

	rc=afp_server_reconnect(s,mesg,&l,l);

	if (rc) 
                log_for_client((void *) c,AFPFSD,LOG_ERR,
                        "%s",mesg);
	return rc;


}


static unsigned char process_resume(struct fuse_client * c)
{
	struct afp_server_resume_request * req =(void *) c->incoming_string+1;
	struct afp_server * s;

	/* Find the server */
	if ((s=find_server_by_name(req->server_name))==NULL) {
		log_for_client((void *) c,AFPFSD,LOG_ERR,
			"%s is an unknown server\n",req->server_name);
		return AFP_SERVER_RESULT_ERROR;
	}

	if (afp_server_reconnect_loud(c,s)) 
	{
		log_for_client((void *) c,AFPFSD,LOG_ERR,
			"Unable to reconnect to %s\n",req->server_name);
		return AFP_SERVER_RESULT_ERROR;
	}
	log_for_client((void *) c,AFPFSD,LOG_NOTICE,
		"Resumed connection to %s\n",req->server_name);

	return AFP_SERVER_RESULT_OKAY;
	
}

static unsigned char process_unmount(struct fuse_client * c)
{
	struct afp_server_unmount_request * req;
	struct afp_server * s;
	struct afp_volume * v;
	int j=0;

	req=(void *) c->incoming_string+1;

	for (s=get_server_base();s;s=s->next) {
		for (j=0;j<s->num_volumes;j++) {
			v=&s->volumes[j];
			if (strcmp(v->mountpoint,req->mountpoint)==0) {
				goto found;
			}

		}
	}
	goto notfound;
found:
	if (v->mounted != AFP_VOLUME_MOUNTED ) {
		log_for_client((void *) c,AFPFSD,LOG_NOTICE,
			"%s was not mounted\n",v->mountpoint);
		return AFP_SERVER_RESULT_ERROR;
	}

	afp_unmount_volume(v);

	return AFP_SERVER_RESULT_OKAY;
notfound:
	log_for_client((void *)c,AFPFSD,LOG_WARNING,
		"afpfs-ng doesn't have anything mounted on %s.\n",req->mountpoint);
	return AFP_SERVER_RESULT_ERROR;


}

static unsigned char process_ping(struct fuse_client * c)
{
	log_for_client((void *)c,AFPFSD,LOG_INFO,
		"Ping!\n");
	return AFP_SERVER_RESULT_OKAY;
}

static unsigned char process_exit(struct fuse_client * c)
{
	log_for_client((void *)c,AFPFSD,LOG_INFO,
		"Exiting\n");
	trigger_exit();
	return AFP_SERVER_RESULT_OKAY;
}

static unsigned char process_status(struct fuse_client * c)
{
	struct afp_server * s;

	char text[40960];
	int len=40960;

	if ((c->incoming_size + 1)< sizeof(struct afp_server_status_request)) 
		return AFP_SERVER_RESULT_ERROR;

	afp_status_header(text,&len);

	log_for_client((void *)c,AFPFSD,LOG_INFO,text);

	s=get_server_base();

	for (s=get_server_base();s;s=s->next) {
		afp_status_server(s,text,&len);
		log_for_client((void *)c,AFPFSD,LOG_DEBUG,text);
	}

	return AFP_SERVER_RESULT_OKAY;

}

static int process_mount(struct fuse_client * c)
{
	struct afp_server_mount_request * req;
	struct afp_server  * s=NULL;
	struct afp_volume * volume;
	struct afp_connection_request conn_req;
	int ret;
	struct stat lstat;

	if ((c->incoming_size-1) < sizeof(struct afp_server_mount_request)) 
		goto error;

	req=(void *) c->incoming_string+1;

	/* Todo should check the existance and perms of the mount point */

	if ((ret=access(req->mountpoint,X_OK))!=0) {
		log_for_client((void *)c,AFPFSD,LOG_DEBUG,
			"Incorrect permissions on mountpoint %s: %s\n",
			req->mountpoint, strerror(errno));

		goto error;
	}

	if (stat(FUSE_DEVICE,&lstat)) {
		printf("Could not find %s\n",FUSE_DEVICE);
		goto error;
	}

	if (access(FUSE_DEVICE,R_OK | W_OK )!=0) {
		log_for_client((void *)c, AFPFSD,LOG_NOTICE, 
			"Incorrect permissions on %s, mode of device"
			" is %o, uid/gid is %d/%d.  But your effective "
			"uid/gid is %d/%d\n", 
				FUSE_DEVICE,lstat.st_mode, lstat.st_uid, 
				lstat.st_gid, 
				geteuid(),getegid());
		goto error;
	}

	log_for_client((void *)c,AFPFSD,LOG_NOTICE,
		"Mounting %s from %s on %s\n",
		(char *) req->url.servername, 
		(char *) req->url.volumename,req->mountpoint);

	memset(&conn_req,0,sizeof(conn_req));

	conn_req.url=req->url;
	conn_req.uam_mask=req->uam_mask;

	if ((s=afp_server_full_connect(c,&conn_req))==NULL) {
		signal_main_thread();
		goto error;
	}
	
	if ((volume=mount_volume(c,s,req->url.volumename,
		req->url.volpassword))==NULL) {
		goto error;
	}

	volume->extra_flags|=req->volume_options;

	volume->mapping=req->map;
	afp_detect_mapping(volume);

	snprintf(volume->mountpoint,255,req->mountpoint);

	/* Create the new thread and block until we get an answer back */
	{
		pthread_mutex_t mutex;
		struct timespec ts;
		struct timeval tv;
		int ret;
		struct start_fuse_thread_arg arg;
		memset(&arg,0,sizeof(arg));
		arg.client = c;
		arg.volume = volume;
		arg.wait = 1;
		arg.changeuid=req->changeuid;

		gettimeofday(&tv,NULL);
		ts.tv_sec=tv.tv_sec;
		ts.tv_sec+=5;
		ts.tv_nsec=tv.tv_usec*1000;
		pthread_mutex_init(&mutex,NULL);
		pthread_cond_init(&volume->startup_condition_cond,NULL);

		/* Kickoff a thread to see how quickly it exits.  If
		 * it exits quickly, we have an error and it failed. */

		pthread_create(&volume->thread,NULL,start_fuse_thread,&arg);

		if (arg.wait) ret = pthread_cond_timedwait(
				&volume->startup_condition_cond,&mutex,&ts);

		report_fuse_errors(c);
		
		switch (arg.fuse_result) {
		case 0:
		if (volume->mounted==AFP_VOLUME_UNMOUNTED) {
			/* Try and discover why */
			switch(arg.fuse_errno) {
			case ENOENT:
				log_for_client((void *)c,AFPFSD,LOG_ERR,
					"Permission denied, maybe a problem with the fuse device or mountpoint?\n");
				break;
			default:
				log_for_client((void *)c,AFPFSD,LOG_ERR,
					"Mounting of volume %s of server %s failed.\n", 
						volume->volume_name_printable, 
						volume->server->server_name_printable);
			}
			goto error;
		} else {
			log_for_client((void *)c,AFPFSD,LOG_NOTICE,
				"Mounting of volume %s of server %s succeeded.\n", 
					volume->volume_name_printable, 
					volume->server->server_name_printable);
			return 0;
		}
		break;
		case ETIMEDOUT:
			log_for_client((void *)c,AFPFSD,LOG_NOTICE,
				"Still trying.\n");
			return 0;
			break;
		default:
			volume->mounted=AFP_VOLUME_UNMOUNTED;
			log_for_client((void *)c,AFPFSD,LOG_NOTICE,
				"Unknown error %d, %d.\n", 
				arg.fuse_result,arg.fuse_errno);
			goto error;
		}

	}
	return AFP_SERVER_RESULT_OKAY;
error:
	if ((s) && (!something_is_mounted(s))) {
		afp_server_remove(s);
	}
	signal_main_thread();
	return AFP_SERVER_RESULT_ERROR;
}


static void * process_command_thread(void * other)
{

	struct fuse_client * c = other;
	int ret=0;
	char tosend[sizeof(struct afp_server_response) + MAX_CLIENT_RESPONSE];
	struct afp_server_response response;


	switch(c->incoming_string[0]) {
	case AFP_SERVER_COMMAND_MOUNT: 
		ret=process_mount(c);
		break;
	case AFP_SERVER_COMMAND_STATUS: 
		ret=process_status(c);
		break;
	case AFP_SERVER_COMMAND_UNMOUNT: 
		ret=process_unmount(c);
		break;
	case AFP_SERVER_COMMAND_SUSPEND: 
		ret=process_suspend(c);
		break;
	case AFP_SERVER_COMMAND_RESUME: 
		ret=process_resume(c);
		break;
	case AFP_SERVER_COMMAND_PING: 
		ret=process_ping(c);
		break;
	case AFP_SERVER_COMMAND_EXIT: 
		ret=process_exit(c);
		break;
	default:
		log_for_client((void *)c,AFPFSD,LOG_ERR,"Unknown command\n");
	}
	/* Send response */
	response.result=ret;
	response.len=strlen(c->client_string);

	bcopy(&response,tosend,sizeof(response));
	bcopy(c->client_string,tosend+sizeof(response),response.len);
	ret=write(c->fd,tosend,sizeof(response)+response.len);
	if (ret<0) {
		perror("Writing");
	}

	if ((!c) || (c->fd==0)) return NULL;
	rm_fd_and_signal(c->fd);
	close(c->fd);
	remove_client(c);

	return NULL;

}
static int process_command(struct fuse_client * c)
{
	int ret;
	int fd;

	ret=read(c->fd,&c->incoming_string,AFP_CLIENT_INCOMING_BUF);

	if (ret<=0) {
		perror("reading");
		goto out;
	}
	c->incoming_size=ret;

	pthread_t thread;
	pthread_create(&thread,NULL,process_command_thread,c);
	return 0;
out:
	fd=c->fd;
	c->fd=0;
	remove_client(c);
	close(fd);
	rm_fd_and_signal(fd);
	return 0;
}


static struct afp_volume * mount_volume(struct fuse_client * c,
	struct afp_server * server, char * volname, char * volpassword) 
{
	struct afp_volume * using_volume;

	using_volume = find_volume_by_name(server,volname);

	if (!using_volume) {
		log_for_client((void *) c,AFPFSD,LOG_ERR,
			"Volume %s does not exist on server %s.\n",volname,
			server->server_name_printable);
		if (server->num_volumes) {
			char names[1024];
			afp_list_volnames(server,names,1024);
			log_for_client((void *)c,AFPFSD,LOG_ERR,
				"Choose from: %s\n",names);
		}
		goto error;
	}

	if (using_volume->mounted==AFP_VOLUME_MOUNTED) {
		log_for_client((void *)c,AFPFSD,LOG_ERR,
			"Volume %s is already mounted on %s\n",volname,
			using_volume->mountpoint);
		goto error;
	}

	if (using_volume->flags & HasPassword) {
		bcopy(volpassword,using_volume->volpassword,AFP_VOLPASS_LEN);
		if (strlen(volpassword)<1) {
			log_for_client((void *) c,AFPFSD,LOG_ERR,"Volume password needed\n");
			goto error;
		}
	}  else memset(using_volume->volpassword,0,AFP_VOLPASS_LEN);

	if (volopen(c,using_volume)) {
		log_for_client((void *) c,AFPFSD,LOG_ERR,"Could not mount volume %s\n",volname);
		goto error;
	}

	using_volume->server=server;

	return using_volume;
error:
	return NULL;
}


static struct libafpclient client = {
	.unmount_volume = fuse_unmount_volume,
	.log_for_client = fuse_log_for_client,
	.forced_ending_hook =fuse_forced_ending_hook,
	.scan_extra_fds = fuse_scan_extra_fds};

int fuse_register_afpclient(void)
{
	libafpclient_register(&client);
	return 0;
}




#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <sys/un.h>
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <grp.h>

#include "config.h"
#include <afp.h>
#include "afp_server.h"
#include "uams_def.h"
#include "map_def.h"
#include "libafpclient.h"

#define default_uam "Cleartxt Passwrd"

#define MAX_OUTGOING_LENGTH 8192

#define AFPFSD_FILENAME "afpfsd"
#define DEFAULT_MOUNT_FLAGS (VOLUME_EXTRA_FLAGS_SHOW_APPLEDOUBLE|\
	VOLUME_EXTRA_FLAGS_NO_LOCKING | VOLUME_EXTRA_FLAGS_IGNORE_UNIXPRIVS)

static char outgoing_buffer[MAX_OUTGOING_LENGTH];
static int outgoing_len=0;
static unsigned int uid, gid=0;
static int changeuid=0;
static int changegid=0;
static char * thisbin;


static int start_afpfsd(void)
{
	char *argv[1];

	argv[0]=0;
	if (fork()==0) {
		char filename[PATH_MAX];
		if (changegid) {
			if (setegid(gid)) {
				perror("Changing gid");
				return -1;
			}
		}
		if (changeuid) {
			if (seteuid(uid)) {
				perror("Changing uid");
				return -1;
			}
		}
		snprintf(filename,PATH_MAX,AFPFSD_FILENAME);
		if (getenv("PATH")==NULL) {
			/* If we don't have an PATH set, it is probably 
			   becaue we are being called from mount, 
			   so go search for it */
			snprintf(filename, PATH_MAX,
				"/usr/local/bin/%s",AFPFSD_FILENAME);
			if (access(filename,X_OK)) {
				snprintf(filename, "/usr/bin/%s",
					AFPFSD_FILENAME);
				if (access(filename,X_OK)) {
					printf("Could not find server (%s)\n",
						filename);
					return -1;
				}
			}
		}
		
		if (execvp(filename,argv)) {
			if (errno==ENOENT) {
				/* Try the path of afp_client */
				char newpath[PATH_MAX];
				snprintf(newpath,PATH_MAX,"%s/%s",
					basename(thisbin),AFPFSD_FILENAME);
				if (execvp(newpath,argv)) {
					perror("Starting up afpfsd\n");
					return -1;
				}
			} else {
				perror("Starting up afpfsd");
				return -1;
			}
		}
		printf("done threading\n");
	}
	return 0;
}


static int daemon_connect(void) 
{
	int sock;
	struct sockaddr_un servaddr;
	char filename[PATH_MAX];
	unsigned char trying=2;

	if ((sock=socket(AF_UNIX,SOCK_STREAM,0)) < 0) {
		perror("Could not create socket\n");
		return -1;
	}
	memset(&servaddr,0,sizeof(servaddr));
	servaddr.sun_family = AF_UNIX;
	sprintf(filename,"%s-%d",SERVER_FILENAME,uid);

	strcpy(servaddr.sun_path,filename);

	while(trying) {
		if ((connect(sock,(struct sockaddr*) &servaddr,
			sizeof(servaddr.sun_family) + 
			sizeof(servaddr.sun_path))) >=0) 
				goto done;
		printf("The afpfs daemon does not appear to be running for uid %d, let me start it for you\n", uid);

		if (start_afpfsd()!=0) {
			printf("Error in starting up afpfsd\n");
			goto error;
		}
		if ((connect(sock,(struct sockaddr*) &servaddr,
			sizeof(servaddr.sun_family) + 
			sizeof(servaddr.sun_path))) >=0) 
				goto done;
		sleep(1);
		trying--;
	}
error:
	perror("Trying to startup afpfsd");
	return -1;

done:
	return sock;
}


static void usage(void) 
{
	printf(
"afp_client [command] [options]\n"
"    mount [mountopts] <server>:<volume> <mountpoint>\n"
"         mount options:\n"
"         -u, --user <username> : log in as user <username>\n"
"         -p, --pass <password> : use <password>\n"
"                           If password is '-', password will be hidden\n"
"         -o, --port <portnum> : connect using <portnum> instead of 548\n"
"         -V, --volumepassword <volpass> : use this volume password\n"
"         -v, --afpversion <afpversion> set the AFP version, eg. 3.1\n"
"         -a, --uam <uam> : use this authentication method, one of:\n"
"               \"No User Authent\", \"Cleartxt Passwrd\", \n"
"               \"Randnum Exchange\", \"2-Way Randnum Exchange\", \n"
"               \"DHCAST128\", \"Client Krb v2\", \"DHX2\" \n\n"
"         -m, --map <mapname> : use this uid/gid mapping method, one of:\n"
"               \"Common user directory\", \"Login ids\"\n"
"    status: get status of the AFP daemon\n\n"
"    unmount <mountpoint> : unmount\n\n"
"    suspend <servername> : terminates the connection to the server, but\n"
"                           maintains the mount.  For laptop suspend/resume\n"
"    resume  <servername> : resumes the server connection \n\n"
"    exit                 : unmounts all volumes and exits afpfsd\n"
);
 }


static int send_command(int sock, char * msg,int len) 
{

	return write(sock,msg,len);
}

static int do_exit(int argc,char **argv)
{
	outgoing_len=1;
	outgoing_buffer[0]=AFP_SERVER_COMMAND_EXIT;

	return 0;

}

static int do_status(int argc, char ** argv) 
{
        int c;
        int option_index=0;
	struct afp_server_status_request * req;
	int optnum;
	struct option long_options[] = {
		{"volume",1,0,'v'},
		{"server",1,0,'s'},
		{0,0,0,0},
	};

	outgoing_len=sizeof(struct afp_server_status_request)+1;
	req = (void *) outgoing_buffer+1;
	memset(outgoing_buffer,0,outgoing_len);
	outgoing_buffer[0]=AFP_SERVER_COMMAND_STATUS;

        while(1) {
		optnum++;
                c = getopt_long(argc,argv,"v:s:",
                        long_options,&option_index);
                if (c==-1) break;
                switch(c) {
                case 'v':
                        snprintf(req->volumename,AFP_VOLUME_NAME_LEN,
				"%s",optarg);
                        break;
                }
        }

        return 0;
}

static int do_resume(int argc, char ** argv) 
{
	struct afp_server_resume_request * req;
	outgoing_len=sizeof(struct afp_server_resume_request)+1;
	req = (void *) outgoing_buffer+1;
	if (argc<3) {
		usage();
		return -1;
	}

	memset(req,0,sizeof(*req));
	snprintf(req->server_name,AFP_SERVER_NAME_LEN,"%s",argv[2]);
	outgoing_buffer[0]=AFP_SERVER_COMMAND_RESUME;

	return 0;
}

static int do_suspend(int argc, char ** argv) 
{
	struct afp_server_suspend_request * req;
	outgoing_len=sizeof(struct afp_server_suspend_request)+1;
	req = (void *) outgoing_buffer+1;
	if (argc<3) {
		usage();
		return -1;
	}

	memset(req,0,sizeof(*req));
	snprintf(req->server_name,AFP_SERVER_NAME_LEN,"%s",argv[2]);
	outgoing_buffer[0]=AFP_SERVER_COMMAND_SUSPEND;

	return 0;
}

static int do_unmount(int argc, char ** argv) 
{
	struct afp_server_unmount_request * req;
	outgoing_len=sizeof(struct afp_server_unmount_request)+1;
	req = (void *) outgoing_buffer+1;
	if (argc<2) {
		usage();
		return -1;
	}

	memset(req,0,sizeof(*req));
	snprintf(req->mountpoint,255,"%s",argv[2]);
	outgoing_buffer[0]=AFP_SERVER_COMMAND_UNMOUNT;

	return 0;
}

static int do_mount(int argc, char ** argv) 
{
        int c;
        int option_index=0;
	struct afp_server_mount_request * req;
	int optnum;
	unsigned int uam_mask=default_uams_mask();

	struct option long_options[] = {
		{"afpversion",1,0,'v'},
		{"volumepassword",1,0,'V'},
		{"user",1,0,'u'},
		{"pass",1,0,'p'},
		{"port",1,0,'o'},
		{"uam",1,0,'a'},
		{"map",1,0,'m'},
		{0,0,0,0},
	};

	if (argc<4) {
		usage();
		return -1;
	}

	outgoing_len=sizeof(struct afp_server_mount_request)+1;
	req = (void *) outgoing_buffer+1;
	memset(outgoing_buffer,0,outgoing_len);
	outgoing_buffer[0]=AFP_SERVER_COMMAND_MOUNT;
	req->url.port=548;
	req->map=AFP_MAPPING_UNKNOWN;

        while(1) {
		optnum++;
                c = getopt_long(argc,argv,"a:u:m:o:p:v:V:",
                        long_options,&option_index);
                if (c==-1) break;
                switch(c) {
                case 'a':
			if (strcmp(optarg,"guest")==0) 
				uam_mask=UAM_NOUSERAUTHENT;
			else
				uam_mask=uam_string_to_bitmap(optarg);
                        break;
                case 'm':
			req->map=map_string_to_num(optarg);
                        break;
                case 'u':
                        snprintf(req->url.username,AFP_MAX_USERNAME_LEN,"%s",optarg);
                        break;
                case 'o':
                        req->url.port=strtol(optarg,NULL,10);
                        break;
                case 'p':
                        snprintf(req->url.password,AFP_MAX_PASSWORD_LEN,"%s",optarg);
                        break;
                case 'V':
                        snprintf(req->url.volpassword,9,"%s",optarg);
                        break;
                case 'v':
                        req->url.requested_version=strtol(optarg,NULL,10);
                        break;
                }
        }

	if (strcmp(req->url.password, "-") == 0) {
		char *p = getpass("AFP Password: ");
		if (p)
			snprintf(req->url.password,AFP_MAX_PASSWORD_LEN,"%s",p);
	}
	if (strcmp(req->url.volpassword, "-") == 0) {
		char *p = getpass("Password for volume: ");
		if (p)
			snprintf(req->url.volpassword,9,"%s",p);
	}

	optnum=optind+1;
	if (optnum>=argc) {
		printf("No volume or mount point specified\n");
		return -1;
	}
	if (sscanf(argv[optnum++],"%[^':']:%[^':']",
		req->url.servername,req->url.volumename)!=2) {
		printf("Incorrect server:volume specification\n");
		return -1;
	}
	if (uam_mask==0) {
		printf("Unknown UAM\n");
		return -1;
	}

	req->uam_mask=uam_mask;
	req->volume_options=DEFAULT_MOUNT_FLAGS;

	if (optnum>=argc) {
		printf("No mount point specified\n");
		return -1;
	}

	snprintf(req->mountpoint,255,"%s",argv[optnum++]);


        return 0;
}

static void mount_afp_usage(void)
{
	printf("Usage:\n     mount_afp [-o volpass=password] <afp url> <mountpoint>\n");
}

static int handle_mount_afp(int argc, char * argv[])
{
	struct afp_server_mount_request * req = (void *) outgoing_buffer+1;
	unsigned int uam_mask=default_uams_mask();
	char * urlstring, * mountpoint;
	char * volpass = NULL;
	int readonly=0;

	if (argc<2) {
		mount_afp_usage();
		return -1;
	}
	if (strncmp(argv[1],"-o",2)==0) {
		char * p = argv[2], *q;
		char command[256];
		struct passwd * passwd;
		struct group * group;
		
		do {
			memset(command,0,256);
			
			if ((q=strchr(p,','))) 
				strncpy(command,p,(q-p));
			else 
				strcpy(command,p);

			if (strncmp(command,"volpass=",8)==0) {
				p+=8;
				volpass=p;
			} else if (strncmp(command,"user=",5)==0) {
				p=command+5;
				if ((passwd=getpwnam(p))==NULL) {
					printf("Unknown user %s\n",p);
					return -1;
				}
				uid=passwd->pw_uid;
				if (geteuid()!=uid)
					changeuid=1;
			} else if (strncmp(command,"group=",6)==0) {
				p=command+6;
				if ((group=getgrnam(p))==NULL) {
					printf("Unknown group %s\n",p);
					return -1;
				}
				gid=group->gr_gid;
				changegid=1;
			} else if (strcmp(command,"rw")==0) {
				/* Don't do anything */
			} else if (strcmp(command,"ro")==0) {
				readonly=1;
			} else {
				printf("Unknown option %s, skipping\n",command);
			}
		

			if (q) p=q+1;
			else p=NULL;

		} while (p);

		urlstring=argv[3];
		mountpoint=argv[4];
	} else {
		urlstring=argv[1];
		mountpoint=argv[2];
	}


	outgoing_len=sizeof(struct afp_server_mount_request)+1;
	memset(outgoing_buffer,0,outgoing_len);

	afp_default_url(&req->url);

	req->changeuid=changeuid;

	req->volume_options|=DEFAULT_MOUNT_FLAGS;
	if (readonly) req->volume_options |= VOLUME_EXTRA_FLAGS_READONLY;
	req->uam_mask=uam_mask;

	outgoing_buffer[0]=AFP_SERVER_COMMAND_MOUNT;
	req->map=AFP_MAPPING_UNKNOWN;
	snprintf(req->mountpoint,255,"%s",mountpoint);
	if (afp_parse_url(&req->url,urlstring,0) !=0) 
	{
		printf("Could not parse URL\n");
		return -1;
	}
	if (strcmp(req->url.password,"-")==0) {
		char *p = getpass("AFP Password: ");
		if (p)
			snprintf(req->url.password,AFP_MAX_PASSWORD_LEN,"%s",p);
	}

	if (volpass && (strcmp(volpass,"-")==0)) {
		volpass  = getpass("Password for volume: ");
	}
	if (volpass)
		snprintf(req->url.volpassword,9,"%s",volpass);

        return 0;
}

static int prepare_buffer(int argc, char * argv[]) 
{

	if (argc<2) {
		usage();
		return -1;
	}
	if (strncmp(argv[1],"mount",5)==0) {
		return do_mount(argc,argv);
	} else if (strncmp(argv[1],"resume",6)==0) {
		return do_resume(argc,argv);
	} else if (strncmp(argv[1],"suspend",7)==0) {
		return do_suspend(argc,argv);

	} else if (strncmp(argv[1],"status",6)==0) {
		return do_status(argc,argv);

	} else if (strncmp(argv[1],"unmount",7)==0) {
		return do_unmount(argc,argv);
	} else if (strncmp(argv[1],"exit",4)==0) {
		return do_exit(argc,argv);

	} else {
		usage();
		return -1;
	}

	return 0;
}


int read_answer(int sock) {
	int len=0, expected_len=0, packetlen;
	char incoming_buffer[MAX_CLIENT_RESPONSE];
	char toprint[MAX_CLIENT_RESPONSE+200];
	struct timeval tv;
	fd_set rds,ords;
	int ret;
	struct afp_server_response * answer = (void *) incoming_buffer;

	memset(incoming_buffer,0,MAX_CLIENT_RESPONSE);

	FD_ZERO(&rds);
	FD_SET(sock,&rds);
	while (1) {
		tv.tv_sec=30; tv.tv_usec=0;
		ords=rds;
		ret=select(sock+1,&ords,NULL,NULL,&tv);
		if (ret==0) {
			printf("No response from server\n");
			return -1;
		}
		if (FD_ISSET(sock,&ords)) {
			packetlen=read(sock,incoming_buffer+len,MAX_CLIENT_RESPONSE-len);
			if (packetlen==0) {
				printf("Dropped connection\n");
				goto done;
			}
			if (len==0) {
				expected_len=((struct afp_server_response *) incoming_buffer)->len;
			}
			len+=packetlen;
			if (len==expected_len+sizeof(struct afp_server_response))
				
				goto done;
			if (ret<0) goto error;

		}
	}

done:
	memset(toprint,0,MAX_CLIENT_RESPONSE+200);
	snprintf(toprint,MAX_CLIENT_RESPONSE+200,"%s",incoming_buffer+sizeof(*answer));
	printf(toprint);
	return ((struct afp_server_response *) incoming_buffer)->result;

	return 0;
error:
	return -1;
}

int main(int argc, char *argv[]) 
{
	int sock;
	int ret;
	struct afp_volume volume;
	thisbin=argv[0];

	uid=((unsigned int) geteuid());

	volume.server=NULL;

	if (strstr(argv[0],"mount_afp")) {
		if (handle_mount_afp(argc,argv)<0)
		return -1;
	}
	else if (prepare_buffer(argc,argv)<0)
		return -1;

	if ((sock=daemon_connect()) < 0) 
		return -1;

	send_command(sock,outgoing_buffer,outgoing_len);


	ret=read_answer(sock);
	return ret;
}


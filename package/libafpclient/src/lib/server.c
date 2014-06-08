/*
 *  server.c
 *
 *  Copyright (C) 2007 Alex deVries
 *
 */

#include <string.h>
#include <time.h>

#include "afp.h"
#include "dsi.h"
#include "utils.h"
#include "uams_def.h"
#include "codepage.h"
#include "users.h"
#include "libafpclient.h"
#include "afp_internal.h"
#include "dsi.h"


struct afp_server * afp_server_complete_connection(
	void * priv,
	struct afp_server * server,
	struct sockaddr_in * address, unsigned char * versions,
		unsigned int uams, char * username, char * password, 
		unsigned int requested_version, unsigned int uam_mask)
{
	char loginmsg[AFP_LOGINMESG_LEN];
	int using_uam;
#define LOGIN_ERROR_MESG_LEN 1024
	char mesg[LOGIN_ERROR_MESG_LEN];
	unsigned int len=0;

	memset(loginmsg,0,AFP_LOGINMESG_LEN);

	server->requested_version=requested_version;
	memcpy(server->username,username,sizeof(server->username));
	memcpy(server->password,password,sizeof(server->password));

	add_fd_and_signal(server->fd);
	dsi_opensession(server);

	/* Figure out what version we're using */
	if (((server->using_version=
		pick_version(versions,requested_version))==NULL)) {
		log_for_client(priv,AFPFSD,LOG_ERR,
			"Server cannot handle AFP version %d\n",
			requested_version);
		goto error;
	}
	using_uam=pick_uam(uams,uam_mask);
	if (using_uam==-1) {
		log_for_client(priv,AFPFSD,LOG_ERR,
			"Could not pick a matching UAM.\n");
		goto error;
	}
	server->using_uam=using_uam;
		
	if (afp_server_login(server,mesg,&len,LOGIN_ERROR_MESG_LEN)) {
		log_for_client(priv,AFPFSD,LOG_ERR,
			"Login error: %s\n", mesg);
		goto error;
	}

	if (afp_getsrvrparms(server)) {
		log_for_client(priv,AFPFSD,LOG_ERR,
			"Could not get server parameters\n");
		goto error;
	}

	/* If we haven't gotten a proper date back, so set it to the connect time. */
	if (server->connect_time==3094168448) {
		struct timeval tv;
		gettimeofday(&tv,NULL);
		server->connect_time = tv.tv_sec;
	}

	afp_getsrvrmsg(server,AFPMESG_LOGIN,
		((server->using_version->av_number>=30)?1:0),
		DSI_DEFAULT_TIMEOUT,loginmsg);  /* block */
	if (strlen(loginmsg)>0) 
		log_for_client(priv,AFPFSD,LOG_NOTICE,
			"Login message: %s\n", loginmsg);

	memcpy(server->loginmesg,loginmsg, AFP_LOGINMESG_LEN);


	return server;
error:
	afp_server_remove(server);
	return NULL;

}


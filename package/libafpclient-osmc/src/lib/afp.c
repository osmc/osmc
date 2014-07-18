
/*
 *  afp.c
 *
 *  Copyright (C) 2006 Alex deVries
 *  Portions copyright (C) 2007 Derrik Pates
 *
 */



#include "afp.h"
#include <config.h>

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>

#include "afp_protocol.h"
#include "libafpclient.h"
#include "server.h"
#include "dsi.h"
#include "dsi_protocol.h"
#include "utils.h"
#include "afp_replies.h"
#include "afp_internal.h"
#include "did.h"
#include "forklist.h"
#include "codepage.h"

struct afp_versions      afp_versions[] = {
            { "AFPVersion 1.1", 11 },
            { "AFPVersion 2.0", 20 },
            { "AFPVersion 2.1", 21 },
            { "AFP2.2", 22 },
            { "AFPX03", 30 },
            { "AFP3.1", 31 },
            { "AFP3.2", 32 },
            { NULL, 0}
        };

static int afp_blank_reply(struct afp_server *server, char * buf, unsigned int size, void * ignored);

int (*afp_replies[])(struct afp_server * server,char * buf, unsigned int len, void * other) = {
	NULL, afp_byterangelock_reply, afp_blank_reply, NULL,
	afp_blank_reply, NULL, afp_createdir_reply, afp_blank_reply, /* 0 - 7 */
	afp_blank_reply, afp_enumerate_reply, afp_blank_reply, afp_blank_reply, 
	NULL, NULL, NULL, NULL,                       /* 8 - 15 */
	afp_getsrvrparms_reply, afp_getvolparms_reply, afp_login_reply, afp_login_reply,
	afp_blank_reply, afp_mapid_reply, afp_mapname_reply, afp_blank_reply,  /*16 - 23 */
	afp_volopen_reply, NULL, afp_openfork_reply, afp_read_reply,
	afp_blank_reply, afp_blank_reply, afp_blank_reply, afp_blank_reply,    /*24 - 31 */
	NULL, afp_write_reply, afp_getfiledirparms_reply, afp_blank_reply,
	afp_changepassword_reply, afp_getuserinfo_reply, afp_getsrvrmsg_reply, NULL,      /*32 - 39 */

	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,                       /*40 - 47 */
	afp_opendt_reply, afp_blank_reply, NULL, afp_geticon_reply,
	NULL, NULL, NULL, NULL,                       /*48 - 55 */
	afp_blank_reply, NULL, afp_getcomment_reply, afp_byterangelockext_reply,
	afp_readext_reply, afp_writeext_reply, 
	NULL, NULL,                       /*56 - 63 */
	afp_getsessiontoken_reply,afp_blank_reply, NULL, NULL,
	afp_enumerateext2_reply, NULL, NULL, NULL,    /*64 - 71 */
	afp_listextattrs_reply, NULL, NULL, NULL,
	afp_blank_reply, NULL, NULL, NULL,                       /*72 - 79 */

	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,

	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,

	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,

	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,

	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,

	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,

	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,

	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,

	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,

};


/* This is the simplest afp reply */
static int afp_blank_reply(struct afp_server *server, char * buf, unsigned int size, void * ignored)
{
	struct {
		struct dsi_header header __attribute__((__packed__));
	} * reply = (void *) buf;
	return reply->header.return_code.error_code;
}

/* Handle a reply packet */
int afp_reply(unsigned short subcommand, struct afp_server * server, void * other) 
{
	int ret=0;

	/* No AFP packet is valid if it is smaller than a DSI header. */

	if (server->data_read<sizeof(struct dsi_header))
		return -1;

	if (afp_replies[subcommand]) {
		ret=(*afp_replies[subcommand])(server,
			server->incoming_buffer,
			server->data_read, other);
	} else {
		log_for_client(NULL,AFPFSD,LOG_WARNING,
			"AFP subcommand %d not supported\n",subcommand);
	}
	return ret;
}


static struct afp_server * server_base=NULL;

int server_still_valid(struct afp_server * server) 
{
	struct afp_server * s = server_base;

	for (;s;s=s->next)
		if (s==server) return 1;
	return 0;
}

static void add_server(struct afp_server *newserver)
{
        newserver->next=server_base;
        server_base=newserver;
}

struct afp_server * get_server_base(void) 
{
	return server_base;
}

struct afp_server * find_server_by_signature(char * signature) 
{
	struct afp_server * s;

	for (s=get_server_base();s;s=s->next) {
		if (memcmp(s->signature,signature,AFP_SIGNATURE_LEN)==0) {
			return s;
		}
	}
	return NULL;
}

struct afp_server * find_server_by_name(char * name) 
{
	struct afp_server * s;
	for (s=get_server_base(); s; s=s->next) {
		if (strcmp(s->server_name_utf8,name)==0) return s;
		if (strcmp(s->server_name,name)==0) return s;
	}

	return NULL;
}

struct afp_server * find_server_by_address(struct sockaddr_in * address)
{
        struct afp_server *s;
	for (s=server_base;s;s=s->next) {
                if (bcmp(&s->address,address,sizeof(struct sockaddr_in))==0)
                        return s;
	}
        return NULL;
}

int something_is_mounted(struct afp_server * server)
{
	int i;


	for (i=0;i<server->num_volumes;i++) {
		if (server->volumes[i].mounted != AFP_VOLUME_UNMOUNTED ) 
			return 1;
	}
	return 0;
}

int afp_unmount_all_volumes(struct afp_server * server) 
{

        int i;
        for (i=0;i<server->num_volumes;i++) {
                if (server->volumes[i].mounted == AFP_VOLUME_MOUNTED) {
                        if (afp_unmount_volume(&server->volumes[i]))
                                return 1;
		}
        }
	return 0;
}


int afp_unmount_volume(struct afp_volume * volume)
{

	struct afp_server * server;
	unsigned char emergency=0;

	if (volume==NULL)
		return -1;

	server=volume->server;
	if (volume->mounted != AFP_VOLUME_MOUNTED) {
		return -1;
	}

	volume->mounted=AFP_VOLUME_UNMOUNTING;

	/* close the volume */

	afp_flush(volume);

	if (afp_volclose(volume)!=kFPNoErr) emergency=1;
	free_entire_did_cache(volume);
	remove_fork_list(volume);
	if (volume->dtrefnum) afp_closedt(server,volume->dtrefnum);
	volume->dtrefnum=0;

	if (libafpclient->unmount_volume)
		libafpclient->unmount_volume(volume);

	volume->mounted=AFP_VOLUME_UNMOUNTED;

	/* Figure out if this is the last volume of the server */

	if (something_is_mounted(server)) return 0;

	/* Logout */
	afp_logout(server,DSI_DONT_WAIT /* don't wait */);

	afp_server_remove(server);

	return -1;

}


void afp_free_server(struct afp_server ** sp)
{
	struct dsi_request * p, *next;
	struct afp_volume * volumes;
	struct afp_server * server;

	if (sp==NULL) return;
	
	server=*sp;

	if (!server) return;

	for (p=server->command_requests;p;) {
		log_for_client(NULL,AFPFSD,LOG_NOTICE,"FSLeft in queue: %p, id: %d command: %d\n",                p,p->requestid,p->subcommand);
		next=p->next;
		free(p);
		p=next;
	}

	volumes=server->volumes;

	loop_disconnect(server);

	if (server->incoming_buffer) free(server->incoming_buffer);
	if (server->attention_buffer) free(server->attention_buffer);
	if (volumes) free(volumes);

	free(server);

	*sp=NULL;
}

int afp_server_remove(struct afp_server *s) 
{
	
	struct dsi_request * p;
	struct afp_server *s2;
	for (p=s->command_requests;p;p=p->next) {
		pthread_cond_signal(&p->condition_cond);
	}

	if (s==server_base) {
		server_base=s->next;
		afp_free_server(&s);
		goto out;
	}

	for (s2=server_base;s2;s2=s2->next) {
		if (s==s2->next) {
			s2->next=s->next;
			afp_free_server(&s);
			goto out;
		}
	}
	return -1;
out:
	return 0;

}

struct afp_server * afp_server_init(struct sockaddr_in * address)
{
	struct afp_server * s;
	struct passwd *pw;

	if ((s = malloc(sizeof(*s)))==NULL) 
		return NULL;
	memset((void *) s, 0, sizeof(*s));
	s->exit_flag = 0;
	s->path_encoding=kFPUTF8Name;  /* This is a default */
	s->next=NULL;
	s->bufsize=2048;
	s->incoming_buffer=malloc(s->bufsize);

	s->attention_quantum=AFP_DEFAULT_ATTENTION_QUANTUM;
	s->attention_buffer=malloc(s->attention_quantum);
	s->attention_len=0;

	s->connect_state=SERVER_STATE_DISCONNECTED;
	memcpy(&s->address,address,sizeof(*address));

	/* FIXME this shouldn't be set here */
	pw=getpwuid(geteuid());
	memcpy(&s->passwd,pw,sizeof(struct passwd));
	return s;
}

static void setup_default_outgoing_token(struct afp_token * token)
{
	char foo[] = {0x54,0xc0,0x75,0xb0,0x15,0xe6,0x1c,0x13,
	0x86,0x75,0xd2,0xc2,0xfd,0x03,0x4e,0x3b};
	token->length=16;
	memcpy(token->data,foo,16);
}

static int resume_token(struct afp_server * server)
{

	struct afp_token outgoing_token;
	time_t now;
	int ret;

	/* Get the time */

	time(&now);

	/* Setup the outgoing token */
	setup_default_outgoing_token(&outgoing_token);

	ret=afp_getsessiontoken(server,kReconnWithTimeAndID,
		(unsigned int) now,
		&outgoing_token,&server->token);

	return ret;

}
static int setup_token(struct afp_server * server)
{

	time_t now;
	int ret;
	struct afp_token outgoing_token;

	/* Get the time */

	time(&now);

	/* Setup the outgoing token */
	setup_default_outgoing_token(&outgoing_token);

	ret=afp_getsessiontoken(server,kLoginWithTimeAndID,
		(unsigned int) now,
		&outgoing_token,&server->token);

	return ret;

}

int afp_server_login(struct afp_server *server, 
	char * mesg, unsigned int *l, unsigned int max) 
{
	int rc;

	rc=afp_dologin(server,server->using_uam,
		server->username,server->password);

	switch(rc) {
	case -1:
		*l+=snprintf(mesg,max-*l,
			"Could not find a valid UAM\n");
		goto error;
	case kFPAuthContinue:
		*l+=snprintf(mesg,max-*l,
			"Authentication method unsupported by AFPFS\n");
		goto error;
	case kFPBadUAM:
		*l+=snprintf(mesg,max-*l,
			"Specified UAM is unknown\n");
		goto error;
	case kFPBadVersNum:
		*l+=snprintf(mesg,max-*l,
			"Server does not support this AFP version\n");
	case kFPCallNotSupported:
	case kFPMiscErr:
		*l+=snprintf(mesg,max-*l,
			"Already logged in\n");
		break;
	case kFPNoServer:
		*l+=snprintf(mesg,max-*l,
			"Authentication server not responding\n");
		goto error;
	case kFPPwdExpiredErr:
	case kFPPwdNeedsChangeErr:
		*l+=snprintf(mesg,max-*l,
			"Warning: password needs changing\n");
		goto error;
	case kFPServerGoingDown:
		*l+=snprintf(mesg,max-*l,
			"Server going down, so I can't log you in.\n");
		goto error;
	case kFPUserNotAuth:
		*l+=snprintf(mesg,max-*l,
			"Authentication failed\n");
		goto error;
	case 0: break;
	default:
		*l+=snprintf(mesg,max-*l,
			"Unknown error, maybe username/passwd needed?\n");
		goto error;
	}

	if (server->flags & kSupportsReconnect) {
		/* Get the session */

		if (server->need_resume) {
			resume_token(server); 
			server->need_resume=0;
		} else {
			setup_token(server);
		}
	}

	return 0;
error:
	return 1;
}


struct afp_volume * find_volume_by_name(struct afp_server * server, 
	const char * volname)
{
	int i;
	struct afp_volume * using_volume=NULL;
	char converted_volname[AFP_VOLUME_NAME_LEN];

	memset(converted_volname,0,AFP_VOLUME_NAME_LEN);

	convert_utf8pre_to_utf8dec(volname,strlen(volname),
		converted_volname,AFP_VOLUME_NAME_LEN);

 	for (i=0;i<server->num_volumes;i++)  {
		if (strcmp(converted_volname,
			server->volumes[i].volume_name_printable)==0) 
		{
			using_volume=&server->volumes[i];
			goto out;
		}
	}
out:

	return using_volume;
}

int afp_connect_volume(struct afp_volume * volume, struct afp_server * server,
	char * mesg, unsigned int * l, unsigned int max)
{
	unsigned short bitmap=
			kFPVolAttributeBit|kFPVolSignatureBit|
			kFPVolCreateDateBit|kFPVolIDBit |
			kFPVolNameBit;
	char new_encoding;

	if (server->using_version->av_number>=30) 
		bitmap|= kFPVolNameBit|kFPVolBlockSizeBit;

	switch (afp_volopen(volume,bitmap,
		(strlen(volume->volpassword)>0) ? volume->volpassword : NULL)) 
	{
	case kFPAccessDenied:
		*l+=snprintf(mesg,max-*l,
			"Incorrect volume password\n");
		goto error;
	case kFPNoErr:
		break;
	case kFPBitmapErr:
	case kFPMiscErr:
	case kFPObjectNotFound:
	case kFPParamErr:
		*l+=snprintf(mesg,max-*l,
			"Could not open volume\n");
		goto error;
	}

	/* It is said that if a volume's encoding will be the same 
	 * the server's. */
	if (volume->attributes & kSupportsUTF8Names)
		new_encoding=kFPUTF8Name;
	else
		new_encoding=kFPLongName;

	if (new_encoding != server->path_encoding) {
		*l+=snprintf(mesg,max-*l,
			"Volume %s changes the server's encoding\n",
			volume->volume_name_printable);
	}

	server->path_encoding=new_encoding;

	if (volume->signature != AFP_VOL_FIXED) {
		*l+=snprintf(mesg,max-*l,
			"Volume %s does not support fixed directories\n",
			volume->volume_name_printable);
		afp_unmount_volume(volume);
		goto error;

	}

	if (server->using_version->av_number >=30) {
		if ((volume->server->server_type==AFPFS_SERVER_TYPE_NETATALK) &&
			(~ volume->attributes & kSupportsUnixPrivs)) {
			volume->extra_flags &=~VOLUME_EXTRA_FLAGS_VOL_SUPPORTS_UNIX;
		} else {
			volume->extra_flags |= VOLUME_EXTRA_FLAGS_VOL_SUPPORTS_UNIX;
		}
	} else {
		/* This is very odd, but AFP 2.x doesn't give timestamps for directories */

	}
	
	volume->mounted=AFP_VOLUME_MOUNTED;

	return 0;
error:
	return 1;
}

int afp_server_reconnect(struct afp_server * s, char * mesg,
	unsigned int *l, unsigned int max)
{
	int i;
	struct afp_volume * v;

        if (afp_server_connect(s,0))  {
		*l+=snprintf(mesg,max-*l,"Error resuming connection to %s\n",
			s->server_name_printable);
                return 1;
        }

        dsi_opensession(s);

	if(afp_server_login(s,mesg,l,max)) return 1;

         for (i=0;i<s->num_volumes;i++) {
                v=&s->volumes[i];
                if (strlen(v->mountpoint)) {
			if (afp_connect_volume(v,v->server,mesg,l,max))
				*l+=snprintf(mesg,max-*l,
                                        "Could not mount %s\n",
					v->volume_name_printable);
                }
        }

        return 0;
}


int afp_server_connect(struct afp_server *server, int full)
{
	int error = 0;
	struct timeval t1, t2;

	if ((server->fd= socket(PF_INET,SOCK_STREAM,IPPROTO_TCP)) < 0 ) {
		error = errno;
		goto error;
	}

	if (connect(server->fd,(struct sockaddr *) &server->address,sizeof(server->address)) < 0) {
		error = errno;
		goto error;
	}

	server->exit_flag=0;
	server->lastrequestid=0;
	server->connect_state=SERVER_STATE_CONNECTED;

	add_server(server);

	add_fd_and_signal(server->fd);

	if (!full) {
		return 0;
	}

	/* Get the status, and calculate the transmit time.  We use this to
	* calculate our rx quantum. */
	gettimeofday(&t1,NULL);
	if ((error=dsi_getstatus(server))!=0) 
		goto error;
	gettimeofday(&t2,NULL);

	if ((t2.tv_sec - t1.tv_sec) > 0)
		server->tx_delay= (t2.tv_sec - t1.tv_sec) * 1000;
	else
		server->tx_delay= (t2.tv_usec - t1.tv_usec) / 1000;

	/* Calculate the quantum based on our tx_delay and a threshold */
	/* For now, we'll just set a default */
	/* This is the default in 10.4.x where x > 7 */
	server->rx_quantum = 128 * 1024;


	return 0;
error:
	return -error;
}

struct afp_versions * pick_version(unsigned char *versions,
	unsigned char requested) 
{
	/* Pick the right version number.  This means either the 
	   one requested or the last one. Set both the number and the
	   string. */
	int i;
	char version_num=0;
	char found_version=0;
	struct afp_versions * p;
	char highest=0;

	if (requested)
		requested= min(requested,AFP_MAX_SUPPORTED_VERSION);

	for (i=0;versions[i] && (i<SERVER_MAX_VERSIONS);i++) {
		version_num=versions[i];
		highest=max(highest,version_num);
		if (versions[i]==requested) {
			found_version=1;
			break;
		}
	};

	if (!found_version)
		version_num=highest;

	for (p=afp_versions;p->av_name;p++) {
		if (p->av_number==version_num) {
			return p;
		}
	}
	return NULL;
}

int pick_uam(unsigned int uam2, unsigned int uam1)
{

	int i;
	for (i=15;i>=0;i--) {
		if ( ((1<<i)) & (uam2 & uam1)) return (1<<i);

	}
	return -1;
}

int afp_list_volnames(struct afp_server * server, char * names, int max)
{
	int len=0;
	int i;

	for (i=0;i<server->num_volumes;i++) {
		if (i<server->num_volumes-1) 
			len+=snprintf(names+len,max-len,"%s, ",
				server->volumes[i].volume_name_printable);
		else 
			len+=snprintf(names+len,max-len,"%s",
				server->volumes[i].volume_name_printable);
	}
	return len;
}



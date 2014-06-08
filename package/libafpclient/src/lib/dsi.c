
/*
 *  dsi.c
 *
 *  Copyright (C) 2006 Alex deVries
 *
 */


#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>
#include <signal.h>
#include <iconv.h>

#include "utils.h"
#include "dsi.h"
#include "afp.h"
#include "uams_def.h"
#include "dsi_protocol.h"
#include "libafpclient.h"
#include "afp_internal.h"
#include "afp_replies.h"

/* define this in order to get reams of DSI debugging information */
#undef DEBUG_DSI

static int dsi_remove_from_request_queue(struct afp_server *server,
	struct dsi_request *toremove);

/* This sets up a DSI header. */
void dsi_setup_header(struct afp_server * server, struct dsi_header * header, char command) 
{

	memset(header,0, sizeof(struct dsi_header));

	pthread_mutex_lock(&server->requestid_mutex);
	if (server->lastrequestid == 65535) server->lastrequestid = 0;
	else server->lastrequestid++;
	server->expectedrequestid = server->lastrequestid;

	pthread_mutex_unlock(&server->requestid_mutex);

	header->requestid = htons(server->lastrequestid);

	header->command = command;

}

int dsi_getstatus(struct afp_server * server)
{
#define GETSTATUS_BUF_SIZE 1024
	struct dsi_header  header;
	struct afp_rx_buffer rx;
	int ret;
	rx.data=malloc(GETSTATUS_BUF_SIZE);
	rx.maxsize=GETSTATUS_BUF_SIZE;
	rx.size=0;
	dsi_setup_header(server,&header,DSI_DSIGetStatus);
	/* We're intentionally ignoring the results */
	ret=dsi_send(server,(char *) &header,sizeof(struct dsi_header),20,
		0,(void *) &rx);

	free(rx.data);
	return ret;
}

int dsi_sendtickle(struct afp_server *server)
{
	struct dsi_header  header;
	dsi_setup_header(server,&header,DSI_DSITickle);
	dsi_send(server,(char *) &header,sizeof(struct dsi_header),1,
		DSI_DONT_WAIT,NULL);
	return 0;
}

int dsi_opensession(struct afp_server *server)
{
	struct {
		struct dsi_header dsi_header  __attribute__((__packed__));
		uint8_t flags;
		uint8_t length;
		uint32_t rx_quantum ;
	} __attribute__((__packed__)) dsi_opensession_header;

	dsi_setup_header(server,&dsi_opensession_header.dsi_header,DSI_DSIOpenSession);
	/* Advertize our rx quantum */
	dsi_opensession_header.flags=1; 
	dsi_opensession_header.length=sizeof(unsigned int);
	dsi_opensession_header.rx_quantum=htonl(server->attention_quantum);

	dsi_send(server,(char *) &dsi_opensession_header,
		sizeof(dsi_opensession_header),1,DSI_BLOCK_TIMEOUT,NULL);
	return 0;
}

static int check_incoming_dsi_message(struct afp_server * server, void * data, int size)
{
	struct dsi_header * header = (struct dsi_header *) data;

	if (size > sizeof(struct dsi_header)) {
		log_for_client(NULL,AFPFSD,LOG_WARNING,
			"DSI packet too small");
		return -1;
	}

	if (header->flags != DSI_REPLY) {
		log_for_client(NULL,AFPFSD,LOG_WARNING,
			"Got a non-DSI reply");
		return -1;
	}

	if (header->requestid < server->lastrequestid ) {
		log_for_client(NULL,AFPFSD,LOG_WARNING,
			"Got a requestid that was too low");
		return -1;
	}
	if (header->requestid > server->lastrequestid ) {
		log_for_client(NULL,AFPFSD,LOG_WARNING,
			"Got a requestid that was too high");
		return -1;
	}

	return 0;
}

static int dsi_remove_from_request_queue(struct afp_server *server,
	struct dsi_request *toremove)
{

	struct dsi_request *p, *prev=NULL;
	#ifdef DEBUG_DSI
	printf("*** removing %d, %s\n",toremove->requestid, 
		afp_get_command_name(toremove->subcommand));
	#endif
	if (!server_still_valid(server)) return -1;
	pthread_mutex_lock(&server->request_queue_mutex);
	for (p=server->command_requests;p;p=p->next) {
		if (p==toremove) {
			if (prev==NULL)
				server->command_requests=p->next;
			else
				prev->next = p->next;
			server->stats.requests_pending--;
			free(p);
			pthread_mutex_unlock(&server->request_queue_mutex);
			return 0;
		}
		prev=p;
	}

	pthread_mutex_unlock(&server->request_queue_mutex);
	#ifdef DEBUG_DSI
	printf("*** Never removed anything for %d, %s\n",toremove->requestid,
		afp_get_command_name(toremove->subcommand));
	#endif
	log_for_client(NULL,AFPFSD,LOG_WARNING,
		"Got an unknown reply for requestid %i\n",ntohs(toremove->requestid));
	return -1;
}


int dsi_send(struct afp_server *server, char * msg, int size,int wait,unsigned char subcommand, void ** other) 
{
	/* For wait:
	 * -1: wait forever
	 *  0: don't wait
	 * x>n: wait for N seconds */

	struct dsi_header  *header = (struct dsi_header *) msg;
	struct dsi_request * new_request, *p;
	int rc=0;
	struct timespec ts;
	struct timeval tv;
 	header->length=htonl(size-sizeof(struct dsi_header));

	if (!server_still_valid(server) || server->fd==0)
		return -1;

	afp_wait_for_started_loop();

	/* Add request to the queue */
	if (!(new_request=malloc(sizeof(struct dsi_request)))) {
		log_for_client(NULL,AFPFSD,LOG_ERR,
			"Could not allocate for new request\n");
		return -1;
	}
	memset(new_request,0,sizeof(struct dsi_request));
	new_request->requestid=server->lastrequestid;
	new_request->subcommand=subcommand;
	new_request->other=other;
	new_request->wait=wait;
	new_request->next=NULL;

	pthread_mutex_lock(&server->request_queue_mutex);
	if (server->command_requests==NULL) {
		server->command_requests=new_request;
	} else {
		for (p=server->command_requests;p->next;p=p->next);
		p->next=new_request;
	}
	server->stats.requests_pending++;
	pthread_mutex_unlock(&server->request_queue_mutex);

	pthread_cond_init(&new_request->condition_cond,NULL);

	if (server->connect_state==SERVER_STATE_DISCONNECTED) {
		char mesg[1024];
		unsigned int l=0; 
		/* Try and reconnect */

		afp_server_reconnect(server,mesg,&l,1024);


	}

	pthread_mutex_lock(&server->send_mutex);
	#ifdef DEBUG_DSI
	printf("*** Sending %d, %s\n",ntohs(header->requestid),
		afp_get_command_name(new_request->subcommand));
	#endif
	if (write(server->fd,msg,size)<0) {
		if ((errno==EPIPE) || (errno==EBADF)) {
			/* The server has closed the connection */
			server->connect_state=SERVER_STATE_DISCONNECTED;
			return -1;

		}
		perror("writing to server");
		rc=-1;
		pthread_mutex_unlock(&server->send_mutex);
		goto out;
	}
	server->stats.tx_bytes+=size;
	pthread_mutex_unlock(&server->send_mutex);

	int tmpwait=new_request->wait;
	#ifdef DEBUG_DSI
	printf("=== Waiting for response for %d %s\n",
		new_request->requestid,
		afp_get_command_name(new_request->subcommand));
	#endif
	if (tmpwait<0) {

		pthread_mutex_t     mutex = PTHREAD_MUTEX_INITIALIZER;
		pthread_mutex_lock(&mutex);

		/* Wait forever */
		#ifdef DEBUG_DSI
		printf("=== Waiting forever for %d, %s\n",
			new_request->requestid,
			afp_get_command_name(new_request->subcommand));
		#endif

		rc=pthread_cond_wait( 
			&new_request->condition_cond, 
				&mutex );
		pthread_mutex_unlock(&mutex);

	} else if (tmpwait>0) {
		pthread_mutex_t     mutex = PTHREAD_MUTEX_INITIALIZER;
		pthread_mutex_lock(&mutex);

		#ifdef DEBUG_DSI
		printf("=== Waiting for %d %s, for %ds\n",
			new_request->requestid,
			afp_get_command_name(new_request->subcommand),
			new_request->wait);
		#endif

		gettimeofday(&tv,NULL);
		ts.tv_sec=tv.tv_sec;
		ts.tv_sec+=new_request->wait;
		ts.tv_nsec=tv.tv_usec *1000;
		if (new_request->wait==0) {
			#ifdef DEBUG_DSI
			printf("=== Changing my mind, no longer waiting for %d\n",
				new_request->requestid);
			#endif
			pthread_mutex_unlock(&mutex);
			goto skip;
		}
		rc=pthread_cond_timedwait( 
			&new_request->condition_cond, 
			&mutex,&ts);
		pthread_mutex_unlock(&mutex);
		if (rc==ETIMEDOUT) {
/* FIXME: should handle this case properly */
			#ifdef DEBUG_DSI
			printf("=== Timedout for %d\n",
				new_request->requestid);
			#endif
			goto out;
		}
	} else {
		#ifdef DEBUG_DSI
		printf("=== Skipping wait altogether for %d\n",new_request->requestid);
		#endif
	}
	#ifdef DEBUG_DSI
	printf("=== Done waiting for %d %s, waiting for %ds,"
		" return %d, DSI return %d\n",
		new_request->requestid,
		afp_get_command_name(new_request->subcommand),
		new_request->wait, 
		rc,new_request->return_code);
	#endif
skip:
	rc=new_request->return_code;
out:
	dsi_remove_from_request_queue(server,new_request);
	return rc;
}

int dsi_command_reply(struct afp_server* server,unsigned short subcommand, void * other) {

	int ret = 0;

	if (server->data_read<sizeof(struct dsi_header)) {
		log_for_client(NULL,AFPFSD,LOG_WARNING,
		"Got a short reply command, I am just ignoring it. size: %d\n",server->data_read);
		return -1;
	}


	if (subcommand==0) {
		log_for_client(NULL,AFPFSD,LOG_WARNING,
			"Broken subcommand: %d\n",subcommand);
		return -1;
	}

	if ((subcommand==afpRead) || ( subcommand==afpReadExt)) {
		struct afp_rx_buffer * buf = other;
		#ifdef DEBUG_DSI
		printf("=== read() for afpRead, %d bytes\n",buf->maxsize-buf->size);
		#endif
		if ((ret=read(server->fd,buf->data+buf->size,
			buf->maxsize-buf->size))<0) {
			return -1;
		}
		server->stats.rx_bytes+=ret;
		if (ret==0) {
			return -1;
		}
		server->data_read+=ret;
	}

	ret = afp_reply(subcommand,server,other);
	return ret;
}


void dsi_opensession_reply(struct afp_server * server) {

	struct {
		uint8_t flags ;
		uint8_t length ;
		uint32_t tx_quantum;
	}  __attribute__((__packed__)) * dsi_opensession_header = (void *) 
		server->incoming_buffer + sizeof(struct dsi_header);

	server->tx_quantum = ntohl(dsi_opensession_header->tx_quantum);

}

static int dsi_parse_versions(struct afp_server * server, char * msg) 
{
	unsigned char num_versions = msg[0];
	int i,j=0;
	char * p;
	unsigned char len;
	char tmpversionname[33];
	struct afp_versions * tmpversion;	

	memset(server->versions,0, SERVER_MAX_VERSIONS);

	if (num_versions > SERVER_MAX_VERSIONS) num_versions = SERVER_MAX_VERSIONS;
	p=msg+1;
	for (i=0;i<num_versions;i++) {
		len=copy_from_pascal(tmpversionname,p,33)+1;
		for (tmpversion=afp_versions;tmpversion->av_name;tmpversion++) {
			if (strcmp(tmpversion->av_name,tmpversionname)==0) {
				server->versions[j]=tmpversion->av_number;
				j++;
				break;
			}
		}
		p+=len;
	}
	return 0;
}

static int dsi_parse_uams(struct afp_server * server, char * msg) 
{
	unsigned char num_uams = msg[0];
	unsigned char len;
	int i;
	char * p;
	char ua_name[AFP_UAM_LENGTH+1];

	server->supported_uams= 0;

	memset(ua_name,0,AFP_UAM_LENGTH+1);

	if (num_uams > SERVER_MAX_UAMS) num_uams = SERVER_MAX_UAMS;
	p=msg+1;
	for (i=0;i<num_uams;i++) {
		len=copy_from_pascal(ua_name,p,AFP_UAM_LENGTH)+1;
		server->supported_uams|=uam_string_to_bitmap(ua_name);
		p+=len;
	}
	
	return 0;

}


/* The parsing of the return for DSI GetStatus is the same as for 
 * AFP GetSrvrInfo (which we don't yet support) */

void dsi_getstatus_reply(struct afp_server * server) 
{
	/* Todo: check for buffer overruns */

	char * data, *p, *p2;
	int len;
	uint16_t * offset;

	/* This is the fixed portion */
	struct dsi_getstatus_header {
		struct dsi_header dsi __attribute__((__packed__));
		uint16_t machine_offset;
		uint16_t version_offset;
		uint16_t uams_offset;
		uint16_t icon_offset;
		uint16_t flags ;
	} __attribute__((__packed__)) * reply1 = (void *) server->incoming_buffer;

	struct reply2 {
		uint16_t signature_offset;
		uint16_t networkaddress_offset;
		uint16_t directoryservices_offset;
		uint16_t utf8servername_offset;
	} __attribute__((__packed__)) * reply2;

	if (server->data_read < (sizeof(*reply1) + sizeof(*reply2))) {
		log_for_client(NULL,AFPFSD,LOG_ERR,
			"Got incomplete data for getstatus\n");
		return ;
	}

	data = (char * ) server->incoming_buffer + sizeof(struct dsi_header);

	/* First, get the fixed portion */
	p=data + ntohs(reply1->machine_offset);
	copy_from_pascal(server->machine_type,p,AFP_MACHINETYPE_LEN);

	p=data + ntohs(reply1->version_offset);
	dsi_parse_versions(server, p);

	p=data + ntohs(reply1->uams_offset);
	dsi_parse_uams(server, p);

	if (ntohs(reply1->icon_offset)>0) {
		/* The icon and mask are optional */
		p=data + ntohs(reply1->icon_offset);
		memcpy(server->icon,p,256);
	}
	server->flags=ntohs(reply1->flags);

	p=(void *)((unsigned int) server->incoming_buffer + sizeof(*reply1));
	p+=copy_from_pascal(server->server_name,p,AFP_SERVER_NAME_LEN)+1;

	/* Now work our way through the variable bits */

        /* First, make sure we're on an even boundary */
        if (((uint64_t) p) & 0x1) p++;

	/* Get the signature */

	offset = (uint16_t *) p;
	memcpy(server->signature,
        	((void *) data)+ntohs(*offset),
		AFP_SIGNATURE_LEN);
	p+=2;

	/* The network addresses */
	if (server->flags & kSupportsTCP) {
		offset = (uint16_t *) p;
		/* We don't actually do anything with the network addresses,
		 * but if we did, it'd go here */
		p+=2;
	}
	/* The directory names */
	if (server->flags & kSupportsDirServices) {
		offset = (uint16_t *) p;
		/* We don't actually do anything with the directory names,
		 * but if we did, it'd go here */
		p+=2;
	}
	if (server->flags & kSupportsUTF8SrvrName) {

		/* And now the UTF8 server name */
		offset = (uint16_t *) p;

		p2=((void *) data)+ntohs(*offset);

		/* Skip the hint character */
		p2+=1;
		len=copy_from_pascal(server->server_name_utf8,p2,
			AFP_SERVER_NAME_UTF8_LEN);

		/* This is a workaround.  There's a bug in netatalk that in some
		 * circumstances puts the UTF8 servername off by one character */
		if (len==0) {
			p2++;
			len=copy_from_pascal(server->server_name_utf8,p2,
				AFP_SERVER_NAME_UTF8_LEN);
		}

		convert_utf8dec_to_utf8pre(server->server_name_utf8,
			strlen(server->server_name_utf8),
			server->server_name_printable, AFP_SERVER_NAME_UTF8_LEN);
	} else {
		/* We don't have a UTF8 servername, so let's make one */

		iconv_t cd;
		size_t inbytesleft = strlen(server->server_name);
		size_t outbytesleft = AFP_SERVER_NAME_UTF8_LEN;
		char * inbuf = server->server_name;
		char * outbuf = server->server_name_printable;

		if ((cd  = iconv_open("MACINTOSH","UTF-8")) == (iconv_t) -1)
			return;

		iconv(cd,&inbuf,&inbytesleft,
			&outbuf, &outbytesleft);
		iconv_close(cd);

	}
}


void dsi_incoming_closesession(struct afp_server *server)
{
	afp_unmount_all_volumes(server);
	loop_disconnect(server);
}

void dsi_incoming_tickle(struct afp_server * server) 
{
	struct dsi_header  header;

	dsi_setup_header(server,&header,DSI_DSITickle);
	dsi_send(server,(char *) &header,sizeof(struct dsi_header),0,
		DSI_DONT_WAIT,NULL);

}


void * dsi_incoming_attention(void * other)
{
	struct afp_server * server = other;
	struct {
		struct dsi_header header __attribute__((__packed__));
		uint16_t flags ;
	} __attribute__((__packed__)) *packet = (void *) server->attention_buffer;
	unsigned short flags;
	char mesg[AFP_LOGINMESG_LEN];
	unsigned char shutdown=0;
	unsigned char mins=0;
	unsigned char checkmessage=0;


	/* The logic here's undocumented.  If we get an attention packet and
	   there's no flag, then go check the message.  Also, go check the
	   the message if there is a flag and we have the AFPATTN_MESG flag.
	   Checked on netatalk 2.0.3. */

	/* It's a bit tough to find docs on this, but I found it at:

	http://web.archive.org/web/20010806173437/developer.apple.com/techpubs/macosx/Networking/AFPClient/AFPClient-15.html

	*/

	if (ntohl(packet->header.length)>=2) {
		flags=ntohs(packet->flags);

		if (flags&AFPATTN_MESG)
			checkmessage=1;
		if (flags&(AFPATTN_CRASH|AFPATTN_SHUTDOWN))
			shutdown=1;
		mins=flags & 0xff;
	} else {
		checkmessage=1;
	}

	if (checkmessage) {
		afp_getsrvrmsg(server,AFPMESG_SERVER,
			((server->using_version->av_number>=30)?1:0),
			DSI_DEFAULT_TIMEOUT,mesg); 
		if(bcmp(mesg,"The server is going down for maintenance.",41)==0)
			shutdown=1;
	}

	if (shutdown) {
		log_for_client(NULL,AFPFSD,LOG_ERR,
			"Got a shutdown notice in packet %d, going down in %d mins\n",ntohs(packet->header.requestid),mins);
		loop_disconnect(server);
		server->connect_state=SERVER_STATE_DISCONNECTED;
		return NULL;
	}
	return NULL;
}


struct dsi_request * dsi_find_request(struct afp_server *server,
	unsigned short request_id)
{

	struct dsi_request *p, *prev=NULL;

	pthread_mutex_lock(&server->request_queue_mutex);
	for (p=server->command_requests;p;p=p->next) {
		if (request_id==p->requestid) {
			pthread_mutex_unlock(&server->request_queue_mutex);
			return p;
		}
		prev=p;
	}
	pthread_mutex_unlock(&server->request_queue_mutex);

	return NULL;
}

int dsi_recv(struct afp_server * server) 
{
	struct dsi_header * header = (void *) server->incoming_buffer;
	struct dsi_request * request=NULL;
	int rc;
        int amount_to_read=0;
	int ret;
	unsigned char runt_packet=0;
	/* Make sure we have at least one  header */
	if ((amount_to_read=sizeof(struct dsi_header)-server->data_read)>0) {
		#ifdef DEBUG_DSI
		printf("<<< read() for dsi, %d bytes\n",amount_to_read);
		#endif
		ret = read(server->fd,server->incoming_buffer+server->data_read,
			amount_to_read);
		if (ret<0) {
			perror("dsi_recv");
			return -1;
		}
		if (ret==0) {
			return -1;
		}
		server->stats.rx_bytes+=ret;
		server->data_read+=ret;
		if ((server->data_read==sizeof(struct dsi_header)) && 
			(ntohl(header->length)==0)) {
			goto gotenough;
			}
		return 0;
			/* We'll get the rest of the packet next time */
	}
gotenough:
	/* At this point, we have just the header */
	/* Figure out what it is a reply to */
	request = dsi_find_request(server,ntohs(header->requestid));
	if (!request && (header->flags==DSI_REPLY)) {
		log_for_client(NULL,AFPFSD,LOG_ERR,
			"I have no idea what this is a reply to, id %d.\n",
			ntohs(header->requestid));
			runt_packet=1;
			server->stats.runt_packets++;
	}
	if (request) request->return_code=ntohl(header->return_code.error_code);

	/* If it is a read, read in as much as we can */
	if ((request) && 
		((request->subcommand==afpRead) || 
		(request->subcommand==afpReadExt))) {
		struct afp_rx_buffer * buf = request->other;
		int newmax=buf->maxsize-buf->size;

		if (((server->data_read==sizeof(struct dsi_header)) &&
			(ntohl(header->length)==0))) {
				server->data_read=0;
				goto out;
			}

		if ((!buf) || (!buf->maxsize)) {
			log_for_client(NULL,AFPFSD,LOG_ERR,
				"No buffer allocated for incoming data\n");
			return -1;
		}
		if (newmax>ntohl(header->length)-buf->size)
			newmax=ntohl(header->length)-buf->size;

		#ifdef DEBUG_DSI
		printf("<<< read() in response to a request, %d bytes\n",newmax);
		#endif
		ret = read(server->fd,buf->data+buf->size,
			newmax);
		if (ret<0) {
			return -1;
		}
		if (ret==0) {
			return -1;
		}
		server->stats.rx_bytes+=ret;
		buf->size+=ret;

		/* Check to see if we've read enough */
		if (ntohl(header->length)==buf->size) {
			char * tmpbuf;
			int size_to_copy=server->data_read
				-sizeof(struct dsi_header);
			if (size_to_copy==0) {
				server->data_read=0;
				goto out;
			} else if (size_to_copy<0) goto error;


			/* Okay, so there is a buffer we have to shift */
			if ((tmpbuf=malloc(size_to_copy))==NULL) {
				log_for_client(NULL,AFPFSD,LOG_ERR,
					"Problem allocating memory for dsi_recv of size %d",size_to_copy);
				goto error;
			}

			memcpy(tmpbuf,
				server->incoming_buffer+
				sizeof(struct dsi_header), 
				size_to_copy);
			memcpy(server->incoming_buffer,tmpbuf,
				size_to_copy);
			server->data_read=size_to_copy;
			free(tmpbuf);
		} else return 0;
	} else {
		/* Okay, so it isn't a response to an afpRead or afpReadExt */

		if (((server->data_read==sizeof(struct dsi_header)) &&
			(ntohl(header->length)==0))) 
				goto process_packet;

		amount_to_read=min(ntohl(header->length),server->bufsize);
		#ifdef DEBUG_DSI
		printf("<<< read() of rest of AFP, %d bytes\n",amount_to_read);
		#endif
		ret = read(server->fd, (void *)
		(((unsigned int) server->incoming_buffer)+server->data_read),
			amount_to_read);
		if (ret<0) return -1;
		if (ret==0) {
			return -1;
		}
		server->stats.rx_bytes+=ret;
		server->data_read+=ret;

		if (server->data_read<(ntohl(header->length)+sizeof(*header)))
			return 0;
	}
	if (runt_packet) 
		goto after_processing;

process_packet:
	/* At this point, we have a full DSI packet
	   (or, for an afpRead, just the header and the data's been
	    dumped in the preallocated buffer */
	#ifdef DEBUG_DSI
	printf("<<< Handling %d\n",ntohs(header->requestid));
	#endif

	switch (header->command) {

	case DSI_DSICloseSession:
		dsi_incoming_closesession(server);
		break;
	case DSI_DSIGetStatus:
		dsi_getstatus_reply(server);
		break;
	case DSI_DSIOpenSession:
		dsi_opensession_reply(server);
		break;
	case DSI_DSITickle:
		dsi_incoming_tickle(server);
		break;
	case DSI_DSIWrite:
	case DSI_DSICommand:
		if (!runt_packet) 
		dsi_command_reply(server, request->subcommand,request->other);
		break;
	case DSI_DSIAttention:
		{
			pthread_t thread;
			memcpy( server->attention_buffer,
				server->incoming_buffer,
				server->data_read);
			server->attention_len=server->data_read;
			pthread_create(&thread,NULL,
				dsi_incoming_attention,server);
		}
		break;
	default:
		log_for_client(NULL,AFPFSD,LOG_ERR,
			"Unknown DSI command %i\n",header->command);
		goto error;

	}
after_processing:
	if  (server->data_read==ntohl(header->length)+sizeof(*header)) {
		/* The most common case */
		server->data_read=0;
	} else {
		unsigned int size_to_copy=
			server->data_read-
			(ntohl(header->length)+sizeof(*header));
		char * tmpbuf;

		if (size_to_copy<ntohl(header->length)) {
			/* This could be replaced with memmove, as it handles
			 * overlaps */
			memcpy( server->incoming_buffer,
				server->incoming_buffer+ntohl(header->length),
				size_to_copy);
		} else {
			/* This is more complicated, we need an tmp buf */
			if ((tmpbuf=malloc(size_to_copy))==NULL) {
				log_for_client(NULL,AFPFSD,LOG_ERR,
					"Problem allocating memory for dsi_recv of size %d",size_to_copy);
				goto error;
			}

			memcpy(tmpbuf,
				server->incoming_buffer+ntohl(header->length),
				size_to_copy);

			memcpy(server->incoming_buffer,tmpbuf,size_to_copy);
			free(tmpbuf);
		}
		server->data_read-=size_to_copy;
	}
			
out:

	rc=ntohl(header->return_code.error_code);
	if (request) {
		#ifdef DEBUG_DSI
		printf("<<< Found request %d, %s\n",request->requestid,
			afp_get_command_name(request->subcommand));
		#endif
		if (request->wait) {
			#ifdef DEBUG_DSI
			printf("<<< Signalling %d, returning %d or %d\n",request->requestid,request->return_code,rc);
			#endif
			request->wait=0;
			pthread_cond_signal(&request->condition_cond);
		} else {
			dsi_remove_from_request_queue(server,request);
		}
	}
	return 0;
error:
	#ifdef DEBUG_DSI
	printf("returning from dsi_recv with an error\n");
	#endif
	return -1;


}



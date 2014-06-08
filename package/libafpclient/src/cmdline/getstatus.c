#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "afp.h"

static int getstatus(char * address_string, unsigned int port)
{

        struct afp_server *server;
        struct hostent *h;
        int ret;
        struct sockaddr_in address;
	int j, firsttime=0;
	char signature_string[AFP_SIGNATURE_LEN*2+1];
	struct afp_versions * tmpversion;

        if (afp_get_address(NULL,address_string, port, &address)>0) return -1;

        server=afp_server_init(&address);

        ret=afp_server_connect(server,1);
        if (ret<0) {
		perror("Connecting to server");
		return -1;
	}

        printf("Server name: %s\n",server->server_name_printable);
	printf("Machine type: %s\n",server->machine_type);
	printf("AFP versions: \n");

	for (j=0;j<SERVER_MAX_VERSIONS;j++) {
		for (tmpversion=afp_versions;tmpversion->av_name;tmpversion++) {
			if (tmpversion->av_number==server->versions[j]) {
				printf("     %s\n",tmpversion->av_name);
				break;
			}
		}
	}

	printf("UAMs:\n");
	for (j=1;j<0x100;j<<=1) {
		if (j & server->supported_uams) {
			printf("     %s\n", uam_bitmap_to_string(j));
			firsttime=1;
		}
	};

	for (j=0;j<AFP_SIGNATURE_LEN;j++)
		sprintf(signature_string+(j*2),"%02x",
			(unsigned int) ((char) server->signature[j]));


	printf("Signature: %s\n", signature_string);


	
out:
        free(server);
        return 0;
}

static void usage(void)
{
	printf("getstatus [afp_url|ipaddress[:port]]\n");
}

int main(int argc, char * argv[])
{
	unsigned int port = 548;
	struct afp_url url;
	char * servername = argv[1];
	pthread_t loop_thread;

	if (argc!=2) {
		usage();
		return -1;
	}

	/* Parse the argument */
	afp_default_url(&url);

	if (afp_parse_url(&url,argv[1],0)!=0) {
		char * p;
		/* This is not a url */
		if ((p=strchr(servername,':'))!=0) {
			/* we have a port */
			*p='\0';
			p++;
			if ((port=atoi(p))<=0) {
					printf("Could not understand port %s\n",p);
					usage();
					return -1;
			}
		}
	} else {
		servername = url.servername;
		port=url.port;
	}

	libafpclient_register(NULL);

	afp_main_quick_startup(NULL);

	if (getstatus(servername,port) == 0) {

	} else return -1;
}

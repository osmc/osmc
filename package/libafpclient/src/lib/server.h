#ifndef __SERVER_H_
#define __SERVER_H_
struct afp_server * connect_to_new_server(
	void * priv,
	struct sockaddr_in *address,
	unsigned char requested_version, 
	char * username, char * password);


struct afp_server * new_server(
	void * priv,
        struct sockaddr_in * address, unsigned char * versions,
                unsigned int uams, char * username, char * password,
                unsigned int requested_version, unsigned int uam_mask);

int server_login(void * priv, struct afp_server * server);


#endif

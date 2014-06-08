#ifndef __UAMS_H_
#define __UAMS_H_
int afp_dopasswd(struct afp_server *server,
                unsigned int uam, char * username,
                char * oldpasswd, char * newpasswd);
int afp_dologin(struct afp_server *server,
                unsigned int uam, char * username, char * passwd);
#endif

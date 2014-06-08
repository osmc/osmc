#ifndef CMDLINE_AFP_H_
#define CMDLINE_AFP_H_

int full_url;

int com_connect(char * arg);
int com_dir(char * arg);
int com_chmod(char * arg);
int com_put(char *filename);
int com_get (char *filename);
int com_view (char * arg);
int com_rename (char * arg);
int com_delete (char *arg);
int com_mkdir(char *arg);
int com_rmdir(char *arg);
int com_lcd(char * path);
int com_cd (char *path);
int com_touch(char *path);
int com_lpwd (char * ignore);
int com_pwd (char * ignore);
int com_passwd (char * ignore);
int com_status(char * arg);
int com_statvfs(char * arg);
int com_pass(char * arg);
int com_user(char * arg);
int com_disconnect(char * arg);

void cmdline_afp_exit(void);

int cmdline_afp_setup(int recursive, char * url_string);

#endif

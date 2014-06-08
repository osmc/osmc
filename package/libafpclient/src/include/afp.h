
#ifndef _AFP_H_
#define _AFP_H_

#include <arpa/inet.h>
#include <pthread.h>
#include <netdb.h>
#include <sys/statvfs.h>
#include <pwd.h>
#include <afp_protocol.h>
#include <libafpclient.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <netinet/in.h>


#define AFPFS_VERSION "0.8.1"

/* This is the maximum AFP version this library supports */
#define AFP_MAX_SUPPORTED_VERSION 32

/* afp_url is used to pass locations around */
struct afp_url {
	enum {TCPIP,AT} protocol;
	char username[AFP_MAX_USERNAME_LEN];
	char uamname[50];
	char password[AFP_MAX_PASSWORD_LEN];
	char servername[AFP_SERVER_NAME_UTF8_LEN];
	int port;
	char volumename[AFP_VOLUME_NAME_UTF8_LEN];
	char path[AFP_MAX_PATH];

	int requested_version;
	char zone[AFP_ZONE_LEN]; /* Only used for Appletalk */
	char volpassword[9];;
};

struct afp_token {
	unsigned int length;
	char data[AFP_TOKEN_MAX_LEN];
};

#define SERVER_MAX_VERSIONS 10
#define SERVER_MAX_UAMS 10

struct afp_rx_buffer {
	unsigned int size;
	unsigned int maxsize;
	char * data;
	int errorcode;
};


struct afp_file_info {
	unsigned short attributes;
	unsigned int did;
	unsigned int creation_date;
	unsigned int modification_date;
	unsigned int backup_date;
	unsigned int fileid;
	unsigned short offspring;
	char sync;
	char finderinfo[32];
	char name[AFP_MAX_PATH];
	char basename[AFP_MAX_PATH];
	char translated_name[AFP_MAX_PATH];
	struct afp_unixprivs unixprivs;
	unsigned int accessrights;
	struct afp_file_info * next;
	struct afp_file_info * largelist_next;
	unsigned char isdir;
	unsigned long long size;
	unsigned short resourcesize;
	unsigned int resource;
	unsigned short forkid;
	struct afp_icon * icon;
	int eof;
};


#define VOLUME_EXTRA_FLAGS_VOL_CHMOD_KNOWN 0x1
#define VOLUME_EXTRA_FLAGS_VOL_CHMOD_BROKEN 0x2
#define VOLUME_EXTRA_FLAGS_SHOW_APPLEDOUBLE 0x4
#define VOLUME_EXTRA_FLAGS_VOL_SUPPORTS_UNIX 0x8
#define VOLUME_EXTRA_FLAGS_NO_LOCKING 0x10
#define VOLUME_EXTRA_FLAGS_IGNORE_UNIXPRIVS 0x20
#define VOLUME_EXTRA_FLAGS_READONLY 0x40

#define AFP_VOLUME_UNMOUNTED 0
#define AFP_VOLUME_MOUNTED 1
#define AFP_VOLUME_UNMOUNTING 2

struct afp_volume {
	unsigned short volid;
	char flags;  /* This is from afpGetSrvrParms */
	unsigned short attributes; /* This is from VolOpen */
	unsigned short signature;  /* This is fixed or variable */
	unsigned int creation_date;
	unsigned int modification_date;
	unsigned int backup_date;
	struct statvfs stat;
	unsigned char mounted;
	char mountpoint[255];
	struct afp_server * server;
	char volume_name[AFP_VOLUME_NAME_LEN];
	char volume_name_printable[AFP_VOLUME_NAME_UTF8_LEN];
	unsigned short dtrefnum;
	char volpassword[AFP_VOLPASS_LEN];
	unsigned int extra_flags; /* This is an afpfs-ng specific field */

	/* Our directory ID cache */
	struct did_cache_entry * did_cache_base;
	pthread_mutex_t did_cache_mutex;

	/* Our journal of open forks */
	struct afp_file_info * open_forks;
	pthread_mutex_t open_forks_mutex;

	/* Used to trigger startup */
        pthread_cond_t  startup_condition_cond;

	struct {
		uint64_t hits;
		uint64_t misses;
		uint64_t expired;
		uint64_t force_removed;
	} did_cache_stats;

	void * priv;  /* This is a private structure for fuse/cmdline, etc */
	pthread_t thread; /* This is the per-volume thread */

	int mapping;

};

#define SERVER_STATE_CONNECTED 1
#define SERVER_STATE_DISCONNECTED 2

enum server_type{
	AFPFS_SERVER_TYPE_UNKNOWN,
	AFPFS_SERVER_TYPE_NETATALK,
	AFPFS_SERVER_TYPE_AIRPORT,
	AFPFS_SERVER_TYPE_MACINTOSH,
};

#define is_netatalk(x) ( (x)->machine_type == AFPFS_SERVER_TYPE_NETATALK )
#define is_airport(x) ( (x)->machine_type == AFPFS_SERVER_TYPE_AIRPORT )
#define is_macintosh(x) ( (x)->machine_type == AFPFS_SERVER_TYPE_MACINTOSH )



struct afp_versions {
        char        *av_name;
        int         av_number;
};
extern struct afp_versions afp_versions[];

struct afp_server {

	/* Our buffer sizes */
	unsigned int tx_quantum;
	unsigned int rx_quantum;

	unsigned int tx_delay;

	/* Connection information */
	struct sockaddr_in address;
	int fd;

	/* Some stats, for information only */
	struct {
		uint64_t runt_packets;
		uint64_t incoming_dsi;
		uint64_t rx_bytes;
		uint64_t tx_bytes;
		uint64_t requests_pending;
	} stats;

	/* General information */
	char server_name[AFP_SERVER_NAME_LEN];
	char server_name_utf8[AFP_SERVER_NAME_UTF8_LEN];
        char server_name_printable[AFP_SERVER_NAME_UTF8_LEN];

	char machine_type[17];
	char icon[256];
	char signature[16];
	unsigned short flags;
	int connect_state;
	enum server_type server_type;

	/* This is the time we connected */
	time_t connect_time;

	/* UAMs */
	unsigned int supported_uams;
	unsigned int using_uam;

	/* Authentication */
	char username[AFP_MAX_USERNAME_LEN];
	char password[AFP_MAX_PASSWORD_LEN];

	/* Session */
	struct afp_token token;
	char need_resume;

	/* Versions */
	unsigned char requested_version;
	unsigned char versions[SERVER_MAX_VERSIONS];
	struct afp_versions *using_version;

	/* Volumes */
	unsigned char num_volumes;
	struct afp_volume * volumes;

	void * dsi;
	unsigned int exit_flag;

	/* Our DSI request queue */
	pthread_mutex_t requestid_mutex;
	pthread_mutex_t request_queue_mutex;
	unsigned short lastrequestid;
	unsigned short expectedrequestid;
	struct dsi_request * command_requests;


	char loginmesg[200];
	char servermesg[200];
	char path_encoding;

	/* This is the data for the incoming buffer */
	char * incoming_buffer;
	int data_read;
	int bufsize;

	/* And this is for the outgoing queue */
	pthread_mutex_t send_mutex;

	/* This is for user mapping */
	struct passwd passwd;
	unsigned int server_uid, server_gid;
	int server_gid_valid;

	struct afp_server *next;

	/* These are for DSI attention packets */
	unsigned int attention_quantum;
	unsigned int attention_len;
	char * attention_buffer;

};

struct afp_extattr_info {
	unsigned int maxsize;
	unsigned int size;
	char data[1024];
};
struct afp_comment {
	unsigned int maxsize;
	unsigned int size;
	char *data;
};

struct afp_icon {
	unsigned int maxsize;
	unsigned int size;
	char *data;
};

#define AFP_DEFAULT_ATTENTION_QUANTUM 1024

void afp_unixpriv_to_stat(struct afp_file_info *fp,
	struct stat *stat);

int init_uams(void) ;

unsigned int find_uam_by_name(const char * name);
char * uam_bitmap_to_string(unsigned int bitmap);


char * get_uam_names_list(void);

unsigned int default_uams_mask(void);

struct afp_volume * find_volume_by_name(struct afp_server * server,
        const char * volname);

struct afp_connection_request {
        unsigned int uam_mask;
	struct afp_url url;
};

void afp_default_url(struct afp_url *url);
int afp_parse_url(struct afp_url * url, const char * toparse, int verbose);
void afp_print_url(struct afp_url * url);
int afp_url_validate(char * url_string, struct afp_url * valid_url);

int afp_list_volnames(struct afp_server * server, char * names, int max);

/* User mapping */
int afp_detect_mapping(struct afp_volume * volume);

/* These are some functions that help with simple status text generation */

int afp_status_header(char * text, int * len);
int afp_status_server(struct afp_server * s,char * text, int * len);


struct afp_server * afp_server_full_connect(void * priv, struct afp_connection_request * req);

void * just_end_it_now(void *other);
void add_fd_and_signal(int fd);
void loop_disconnect(struct afp_server *s);
void afp_wait_for_started_loop(void);


struct afp_versions * pick_version(unsigned char *versions,
	unsigned char requested) ;
int pick_uam(unsigned int u1, unsigned int u2);

int afp_server_login(struct afp_server *server,
        char * mesg, unsigned int *l, unsigned int max);


int afp_dologin(struct afp_server *server,
	unsigned int uam, char * username, char * passwd);

void afp_free_server(struct afp_server **server);

struct afp_server * afp_server_init(struct sockaddr_in * address);
int afp_get_address(void * priv, const char * hostname, unsigned int port,
	struct sockaddr_in * address);


int afp_main_loop(int command_fd);
int afp_main_quick_startup(pthread_t * thread);

int afp_server_destroy(struct afp_server *s) ;
int afp_server_reconnect(struct afp_server * s, char * mesg,
        unsigned int *l, unsigned int max);
int afp_server_connect(struct afp_server *s, int full);

struct afp_server * afp_server_complete_connection(
	void * priv,
	struct afp_server * server,
	struct sockaddr_in * address, unsigned char * versions,
	unsigned int uams, char * username, char * password,
	unsigned int requested_version, unsigned int uam_mask);

int afp_connect_volume(struct afp_volume * volume, struct afp_server * server,
	char * mesg, unsigned int * l, unsigned int max);
int something_is_mounted(struct afp_server * server);

int add_cache_entry(struct afp_file_info * file) ;
struct afp_file_info * get_cache_by_name(char * name);
struct afp_server * find_server_by_address(struct sockaddr_in * address);
struct afp_server * find_server_by_signature(char * signature);
struct afp_server * find_server_by_name(char * name);
int server_still_valid(struct afp_server * server);


struct afp_server * get_server_base(void);
int afp_server_remove(struct afp_server * server);

int afp_unmount_volume(struct afp_volume * volume);
int afp_unmount_all_volumes(struct afp_server * server);

#define volume_is_readonly(x) (((x)->attributes&kReadOnly) || \
	((x)->extra_flags & VOLUME_EXTRA_FLAGS_READONLY))

int afp_opendt(struct afp_volume *volume, unsigned short * refnum);

int afp_closedt(struct afp_server * server, unsigned short * refnum);

int afp_getcomment(struct afp_volume *volume, unsigned int did,
        const char * pathname, struct afp_comment * comment);

int afp_addcomment(struct afp_volume *volume, unsigned int did,
        const char * pathname, char * comment,uint64_t *size);

int afp_geticon(struct afp_volume * volume, unsigned int filecreator,
        unsigned int filetype, unsigned char icontype, 
	unsigned short length, struct afp_icon * icon);

/* Things you want to do to a server */

int afp_getsrvrmsg(struct afp_server *server, unsigned short messagetype,unsigned char utf8, unsigned char block, char * mesg);

int afp_login(struct afp_server *server, char * uaname,
        char * userauthinfo, unsigned int userauthinfo_len,
	struct afp_rx_buffer *rx);

int afp_changepassword(struct afp_server *server, char * uaname,
        char * userauthinfo, unsigned int userauthinfo_len,
	struct afp_rx_buffer *rx);

int afp_logincont(struct afp_server *server, unsigned short id,
        char * userauthinfo, unsigned int userauthinfo_len,
	struct afp_rx_buffer *rx);

int afp_getsessiontoken(struct afp_server * server, int type,
        unsigned int timestamp, struct afp_token *outgoing_token,
        struct afp_token * incoming_token);

int afp_getsrvrparms(struct afp_server *server);

int afp_logout(struct afp_server *server,unsigned char wait);

int afp_mapname(struct afp_server * server, unsigned char subfunction,
        char * name, unsigned int * id);

int afp_mapid(struct afp_server * server, unsigned char subfunction,
	unsigned int id, char *name);

int afp_getuserinfo(struct afp_server * server, int thisuser,
	unsigned int userid, unsigned short bitmap, 
	unsigned int *newuid, unsigned int *newgid);

int afp_zzzzz(struct afp_server *server);

int afp_volopen(struct afp_volume * volume, 
		unsigned short bitmap, char * password);

int afp_flush(struct afp_volume * volume);

int afp_getfiledirparms(struct afp_volume *volume, unsigned int did, 
	unsigned int filebitmap, unsigned int dirbitmap, const char * pathname,
	struct afp_file_info *fp);

int afp_enumerate(struct afp_volume * volume, 
	unsigned int dirid, 
	unsigned int filebitmap, unsigned int dirbitmap, 
        unsigned short reqcount,
        unsigned short startindex,
        char * path,
	struct afp_file_info ** file_p);

int afp_enumerateext2(struct afp_volume * volume, 
	unsigned int dirid, 
	unsigned int filebitmap, unsigned int dirbitmap, 
        unsigned short reqcount,
        unsigned long startindex,
        char * path,
	struct afp_file_info ** file_p);

int afp_openfork(struct afp_volume * volume,
        unsigned char forktype,
        unsigned int dirid,
        unsigned short accessmode,
        char * filename, 
	struct afp_file_info *fp);

int afp_read(struct afp_volume * volume, unsigned short forkid,
                uint32_t offset,
                uint32_t count, struct afp_rx_buffer * rx);

int afp_readext(struct afp_volume * volume, unsigned short forkid,
                uint64_t offset,
                uint64_t count, struct afp_rx_buffer * rx);

int afp_getvolparms(struct afp_volume * volume, unsigned short bitmap);


int afp_createdir(struct afp_volume * volume, unsigned int dirid, const char * pathname, unsigned int *did_p);

int afp_delete(struct afp_volume * volume,
        unsigned int dirid, char * pathname);


int afp_createfile(struct afp_volume * volume, unsigned char flag,
        unsigned int did, char * pathname);

int afp_write(struct afp_volume * volume, unsigned short forkid,
        uint32_t offset, uint32_t reqcount,
        char * data, uint32_t * written);

int afp_writeext(struct afp_volume * volume, unsigned short forkid,
        uint64_t offset, uint64_t reqcount,
        char * data, uint64_t * written);

int afp_flushfork(struct afp_volume * volume, unsigned short forkid);

int afp_closefork(struct afp_volume * volume, unsigned short forkid);
int afp_setfileparms(struct afp_volume * volume,
        unsigned int dirid, const char * pathname, unsigned short bitmap,
        struct afp_file_info *fp);
int afp_setfiledirparms(struct afp_volume * volume, 
        unsigned int dirid, const char * pathname, unsigned short bitmap,
        struct afp_file_info *fp);

int afp_setdirparms(struct afp_volume * volume,
        unsigned int dirid, const char * pathname, unsigned short bitmap,
        struct afp_file_info *fp);

int afp_volclose(struct afp_volume * volume);


int afp_setforkparms(struct afp_volume *volume,
        unsigned short forkid, unsigned short bitmap, unsigned long len);

int afp_byterangelock(struct afp_volume * volume,
        unsigned char flag,
        unsigned short forkid,
        uint32_t offset,
        uint32_t len, uint32_t *generated_offset);

int afp_byterangelockext(struct afp_volume * volume,
        unsigned char flag,
        unsigned short forkid,
        uint64_t offset,
        uint64_t len, uint64_t *generated_offset);

int afp_moveandrename(struct afp_volume *volume,
	unsigned int src_did,
	unsigned int dst_did,
	char * src_path, char * dst_path, char *new_name);

int afp_rename(struct afp_volume * volume,
        unsigned int dirid,
        char * path_from, char * path_to);

int afp_listextattr(struct afp_volume * volume,
        unsigned int dirid, unsigned short bitmap,
        char * pathname, struct afp_extattr_info * info);

/* This is a currently undocumented command */
int afp_newcommand76(struct afp_volume * volume, unsigned int dlen, char * data);

/* For debugging */
char * afp_get_command_name(char code);


#endif

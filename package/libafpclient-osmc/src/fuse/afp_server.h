#ifndef _AFP_SERVER_H_
#define _AFP_SERVER_H_

#define SERVER_FILENAME "/tmp/afp_server"

#define AFP_SERVER_COMMAND_MOUNT 1
#define AFP_SERVER_COMMAND_STATUS 2
#define AFP_SERVER_COMMAND_UNMOUNT 3
#define AFP_SERVER_COMMAND_SUSPEND 4
#define AFP_SERVER_COMMAND_RESUME 5
#define AFP_SERVER_COMMAND_PING 6
#define AFP_SERVER_COMMAND_EXIT 7

#define AFP_SERVER_RESULT_OKAY 1
#define AFP_SERVER_RESULT_ERROR 2
#define AFP_SERVER_RESULT_TRYING 3
#define AFP_SERVER_RESULT_WARNING 4

struct afp_server_resume_request {
	char server_name[AFP_SERVER_NAME_LEN];
};

struct afp_server_suspend_request {
	char server_name[AFP_SERVER_NAME_LEN];
};

struct afp_server_unmount_request {
	char mountpoint[255];
};

struct afp_server_mount_request {
	struct afp_url url;
	unsigned int uam_mask;
	char mountpoint[255];
	unsigned int volume_options;
	unsigned int map;
	int changeuid;
};

struct afp_server_status_request {
	char volumename[AFP_VOLUME_NAME_LEN];
	char servername[AFP_VOLUME_NAME_LEN];
};

struct afp_server_response {
	char result;
	unsigned int len;
};



#endif

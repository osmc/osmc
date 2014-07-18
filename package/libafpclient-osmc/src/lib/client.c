#include <afp.h>
#include <libafpclient.h>


struct libafpclient * libafpclient = NULL;

static struct libafpclient null_afpclient = {
	.unmount_volume = NULL,
	.log_for_client = stdout_log_for_client,
	.forced_ending_hook = NULL,
	.scan_extra_fds = NULL,
	.loop_started = NULL,
};


void libafpclient_register(struct libafpclient * tmpclient)
{
	if (tmpclient) 
		libafpclient=tmpclient;
	else 
		libafpclient=&null_afpclient;
}


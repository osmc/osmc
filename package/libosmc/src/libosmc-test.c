/*
 * libosmc.c
 * 
 * Copyright 2014 Sam Nazarko <email@samnazarko.co.uk>
 */


#include <stdio.h>
#include <unistd.h>
#include "libosmc.h"

int is_service_enabled(char *service_name)
{
	char *service_file_path;
	asprintf(&service_file_path, "%s%s%s", "/etc/systemd/system/multi-user.target.wants/", service_name, ".service"); 
	fflush(stdout); 
	if (access(service_file_path, F_OK) != -1)
	{
		return 1; 
	}
	else
	{
		return 0;
	}
	free(service_file_path); 
}

void disable_service(char *servicename)
{
	char *disable_cmd;
	asprintf(&disable_cmd, "%s%s", "systemctl disable ", servicename); 
	system(disable_cmd);
}

void enable_service(char *servicename)
{
	char *enable_cmd;
	asprintf(&enable_cmd, "%s%s", "systemctl enable ", servicename); 
	system(enable_cmd);
}

char *get_hostname()
{
	FILE *hostfile;
	char *hostname = NULL;
    size_t len = 0;
	hostfile = fopen("/etc/hostname", "r");
	if (hostfile == NULL)
		return NULL;
	if (getline(&hostname, &len, hostfile) != -1)
	{
		return hostname;
	}
	else
	{
		return NULL;
	}
}

int *set_hostname(char *hostname)
{
   FILE *hostfile; 
   hostfile = fopen("/etc/hostname", "w");
   if (hostfile == NULL)
	   return NULL;
   fwrite(hostname, 1, sizeof(hostname), hostfile);
   /* NB: update /etc/hosts in future */
   fclose(hostfile);
   return 0;
}


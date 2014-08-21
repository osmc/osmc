/*
 * services.c
 * 
 * Copyright 2014 Sam Nazarko <email@samnazarko.co.uk>
 */
 
#include <stdio.h>
#include <unistd.h>

int is_service_enabled(char *service_name)
{
	char *service_file_path;
	int result;
	asprintf(&service_file_path, "%s%s%s", "/etc/systemd/system/multi-user.target.wants/", service_name, ".service"); 
	fflush(stdout); 
	if (access(service_file_path, F_OK) != -1)
		result = 0;
	else
		result = 1;
	free(service_file_path);
	return result;
}

void disable_service(char *servicename)
{
	char *disable_cmd;
	asprintf(&disable_cmd, "%s%s", "systemctl disable ", servicename); 
	system(disable_cmd);
	free(disable_cmd);
}

void enable_service(char *servicename)
{
	char *enable_cmd;
	asprintf(&enable_cmd, "%s%s", "systemctl enable ", servicename); 
	system(enable_cmd);
	free(enable_cmd);
}

/*
 * identity.c
 * 
 * Copyright 2014 Sam Nazarko <email@samnazarko.co.uk>
 */

#include <stdio.h>
#include <unistd.h> 

char *get_hostname()
{
	FILE *hostfile;
	char *hostname = NULL;
	size_t len = 0;
	hostfile = fopen("/etc/hostname", "r");
	if (hostfile == NULL)
		return NULL;
	if (getline(&hostname, &len, hostfile) != -1)
		return hostname;
	else
		return NULL;
}

int *set_hostname(char *hostname)
{
   FILE *hostfile; 
   hostfile = fopen("/etc/hostname", "w");
   if (hostfile == NULL)
	   return -1;
   fwrite(hostname, 1, sizeof(hostname), hostfile);
   /* NB: update /etc/hosts in future */
   fclose(hostfile);
   return 0;
}

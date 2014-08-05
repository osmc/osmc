/*
 * libosmc.c
 * 
 * Copyright 2014 Sam Nazarko <email@samnazarko.co.uk>
 */


#include <stdio.h>
#include <unistd.h>

int is_service_enabled(char *service_name);
void disable_service(char *servicename);
void enable_service(char *servicename);
char *get_hostname(); 
int *set_hostname();

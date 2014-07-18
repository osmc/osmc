#include <string.h>
#include <stdlib.h>

static char *afp_strings[]= { "No User Authent",
        "Cleartxt Passwrd",
        "Randnum Exchange",
        "2-Way Randnum Exchange",
        "DHCAST128",
        "Client Krb v2",
        "DHX2",
        "Recon1",
        ""};


int uam_string_to_bitmap(char * name) 
{
	int i;
	for (i=0;strlen(afp_strings[i])>0;i++)
		if (strcasecmp(name,afp_strings[i])==0)
			return 1<<i;
	return 0;
}

char * uam_bitmap_to_string(unsigned int bitmap) 
{
	int i;
	for (i=15;i>=0;i--) 
		if (bitmap & (1<<i)) return afp_strings[i];
	return NULL;
}


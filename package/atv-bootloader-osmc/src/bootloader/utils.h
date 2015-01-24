#ifndef _UTILS_H_
#define _UTILS_H_

#include	"types.h"

void	msleep(int s);
void	sleep(int s);
int		mseconds(void);
int		seconds(void);
//
char*	strcpy(char *dest, const char *src);
char*	strncpy(char *dest, const char *src, size_t count);
int		strncmp(const char *cs, const char *ct, size_t count);
char*	strcat(char * dest, const char * src);
char*	strstr(const char * s1,const char * s2);
size_t	strlen(const char *s);
void*	memcpy(void * to, const void *from, size_t n);
void*	memset(void *s, int c,  size_t count);
int		memcmp(const void *cs, const void *ct, size_t count);

#endif

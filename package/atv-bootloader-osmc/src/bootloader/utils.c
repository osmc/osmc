#include "utils.h"

/*
 * time.c:
 * sleep.c:
 *
 * Copyright (c) 2007 Mythic-Beasts LTD ( http://www.mythic-beasts.com ),
 * All rights reserved. Written by James McKenzie <macmini@madingley.org>.
 *
 */
#include		"types.h"

/**********************************************************************/
void
msleep(int s)
{
	s += mseconds();

	while ( mseconds() < s);
}
/**********************************************************************/
void
sleep(int s)
{
	msleep(s * 1000);
}

/**********************************************************************/
/**********************************************************************/
static uint32_t		l0, h0;
static int			set = 0;

int
mseconds(void)
{
	uint32_t	l, q;
	uint32_t	h;
	int			r;

	if (!set) {
		asm __volatile__ ("rdtsc\n":"=a" (l0), "=d" (h0):);
		set++;
	}

	asm __volatile__ ("rdtsc\n":"=a" (l), "=d" (h):);


	r = (h - h0) << 12;

	q = l;
	q >>= 20;
	r += q;

	q = l0;
	q >>= 20;
	r -= q;

	return r;
}
/**********************************************************************/
int
seconds(void)
{
	return(mseconds() / 1000);
}

/**********************************************************************/
/**********************************************************************/
// these come from linux/arch/i386/lib/string.c
//  Copyright (C) 1991, 1992  Linus Torvalds
/**********************************************************************/
char* strcpy(char *dest, const char *src)
{
	char *tmp = dest;

	while ((*dest++ = *src++) != '\0')
		/* nothing */;
		
	return tmp;
}
/*
char * strcpy(char * dest, const char *src)
{
	int d0, d1, d2;
	__asm__ __volatile__(
       		"1:\tlodsb\n\t"
        	"stosb\n\t"
       		"testb %%al,%%al\n\t"
       		"jne 1b"
    		: "=&S" (d0), "=&D" (d1), "=&a" (d2)
     		:"0" (src),"1" (dest) : "memory");
	return dest;
}
*/
/**********************************************************************/
char* strncpy(char *dest, const char *src, size_t count)
{
	char *tmp = dest;

	while (count-- && (*dest++ = *src++) != '\0')
		/* nothing */;

	return tmp;
}
 /*
char *strncpy(char * dest, const char *src, size_t count)
{
	int d0, d1, d2, d3;
	asm volatile( "1:\tdecl %2\n\t"
		"js 2f\n\t"
		"lodsb\n\t"
		"stosb\n\t"
		"testb %%al,%%al\n\t"
		"jne 1b\n\t"
		"rep\n\t"
		"stosb\n"
		"2:"
		: "=&S" (d0), "=&D" (d1), "=&c" (d2), "=&a" (d3)
		:"0" (src),"1" (dest),"2" (count) : "memory");
	return dest;
}
*/
/**********************************************************************/
int strncmp(const char *cs, const char *ct, size_t count)
{
	register signed char __res = 0;

	while (count) {
		if ((__res = *cs - *ct++) != 0 || !*cs++)
			break;
		count--;
	}

	return __res;
}
/*
int strncmp(const char * cs, const char * ct, size_t count)
{
	int res;
	int d0, d1, d2;
	asm volatile( "1:\tdecl %3\n\t"
		"js 2f\n\t"
		"lodsb\n\t"
		"scasb\n\t"
		"jne 3f\n\t"
		"testb %%al,%%al\n\t"
		"jne 1b\n"
		"2:\txorl %%eax,%%eax\n\t"
		"jmp 4f\n"
		"3:\tsbbl %%eax,%%eax\n\t"
		"orb $1,%%al\n"
		"4:"
		:"=a" (res), "=&S" (d0), "=&D" (d1), "=&c" (d2)
		:"1" (cs),"2" (ct),"3" (count)
		:"memory");
	return res;
}
*/
/**********************************************************************/
char * strcat(char * dest, const char * src)
{
	char *tmp = dest;

	while (*dest)
		dest++;
	while ((*dest++ = *src++) != '\0')
		;

	return tmp;
}
/**********************************************************************/
char * strstr(const char * s1,const char * s2)
{
	int l1, l2;

	l2 = strlen(s2);
	if (!l2)
		return (char *) s1;
	l1 = strlen(s1);
	while (l1 >= l2) {
		l1--;
		if (!memcmp(s1,s2,l2))
			return (char *) s1;
		s1++;
	}
	return NULL;
}
/**********************************************************************/
size_t strlen(const char *s)
{
	const char *sc;

	for (sc = s; *sc != '\0'; ++sc)
		/* nothing */;
		
	return sc - s;
}
/*
size_t strlen(const char *s)
{
	int d0;
	register int __res;
	__asm__ __volatile__(
        	"repne\n\t"
	       	"scasb\n\t"
        	"notl %0\n\t"
       		"decl %0"
		:"=c" (__res), "=&D" (d0) :"1" (s),"a" (0), "0" (0xffffffffu));
	return __res;
}
*/
/**********************************************************************/
void* memcpy(void *dest, const void *src, size_t count)
{
	char *tmp = (char *) dest, *s = (char *) src;

	while (count--)
		*tmp++ = *s++;

	return dest;
}
/*
void * memcpy(void * to, const void * from, size_t n)
{
	int d0, d1, d2;
	__asm__ __volatile__(
       		"rep ; movsl\n\t"
       		"testb $2,%b4\n\t"
      		"je 1f\n\t"
      	 	"movsw\n"
      		"1:\ttestb $1,%b4\n\t"
     		"je 2f\n\t"
     		"movsb\n"
     	  	"2:"
     		: "=&c" (d0), "=&D" (d1), "=&S" (d2)
		:"0" (n/4), "q" (n),"1" ((long) to),"2" ((long) from)
	       	: "memory");
	return (to);
}
*/
/**********************************************************************/
void* memset(void *s, int c, size_t count)
{
	char *xs = (char *) s;

	while (count--)
		*xs++ = c;
		
	return s;
}
/*
void * memset(void *s, int c,  size_t count)
{
  	int d0, d1;
	__asm__ __volatile__(
	        "rep\n\t"
	        "stosb"
	        : "=&c" (d0), "=&D" (d1)
	        :"a" (c),"1" (s),"0" (count)
	        :"memory");
	return s;
}
*/
/**********************************************************************/
int memcmp(const void *cs,const void *ct, size_t count)
{
	const unsigned char *su1, *su2;
	int					res = 0;

	for( su1 = cs, su2 = ct; 0 < count; ++su1, ++su2, count--)
		if ((res = *su1 - *su2) != 0) break;
	return res;
}

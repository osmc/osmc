/*
Copyright (c) 2006 by Dan Kennedy.
Copyright (c) 2006 by Juliusz Chroboczek.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#ifndef WIN32

static int dummy ATTRIBUTE((unused));

#else
#include "win32_compat.h"
#include <errno.h>
#include <stdio.h>
#include < time.h >

#undef poll
#undef socket
#undef connect
#undef accept
#undef shutdown
#undef getpeername
#undef sleep
#undef inet_aton
#undef gettimeofday
#undef stat
#define assert(a)

/* Windows needs this header file for the implementation of inet_aton() */
#include <ctype.h>

int win32_inet_pton(int af, const char * src, void * dst)
{
  struct sockaddr_in sa;
  int len = sizeof(SOCKADDR);
  int ret = -1;
  int strLen = strlen(src) + 1;
#ifdef UNICODE
  wchar_t *srcNonConst = (wchar_t *)malloc(strLen*sizeof(wchar_t));
  memset(srcNonConst, 0, strLen);
  MultiByteToWideChar(CP_ACP, 0, src, -1, srcNonConst, strLen);
#else
  char *srcNonConst = (char *)malloc(strLen);
  memset(srcNonConst, 0, strLen);
  strncpy(srcNonConst, src, strLen);
#endif

  if( WSAStringToAddress(srcNonConst,af,NULL,(LPSOCKADDR)&sa,&len) == 0 )
  {
	  ret = 1;
  }
  else
  {
    if(	WSAGetLastError() == WSAEINVAL )
    {
	    ret = -1;
    }
  }
  free(srcNonConst);
  memcpy(dst, &sa.sin_addr, sizeof(struct in_addr));
  return ret;
}

int win32_poll(struct pollfd *fds, unsigned int nfds, int timo)
{
  struct timeval timeout, *toptr;
  fd_set ifds, ofds, efds, *ip, *op;
  unsigned int i;
  int  rc;

  // Set up the file-descriptor sets in ifds, ofds and efds. 
  FD_ZERO(&ifds);
  FD_ZERO(&ofds);
  FD_ZERO(&efds);
  for (i = 0, op = ip = 0; i < nfds; ++i) 
  {
    fds[i].revents = 0;
    if(fds[i].events & (POLLIN|POLLPRI)) 
    {
      ip = &ifds;
      FD_SET(fds[i].fd, ip);
    }
    if(fds[i].events & POLLOUT) 
    {
      op = &ofds;
      FD_SET(fds[i].fd, op);
    }
    FD_SET(fds[i].fd, &efds);
  } 

  // Set up the timeval structure for the timeout parameter
  if(timo < 0) 
  {
    toptr = 0;
  } 
  else 
  {
    toptr = &timeout;
    timeout.tv_sec = timo / 1000;
    timeout.tv_usec = (timo - timeout.tv_sec * 1000) * 1000;
  }

#ifdef DEBUG_POLL
  printf("Entering select() sec=%ld usec=%ld ip=%lx op=%lx\n",
  (long)timeout.tv_sec, (long)timeout.tv_usec, (long)ip, (long)op);
#endif
  rc = select(0, ip, op, &efds, toptr);
#ifdef DEBUG_POLL
  printf("Exiting select rc=%d\n", rc);
#endif

  if(rc <= 0)
    return rc;

  if(rc > 0) 
  {
    for (i = 0; i < nfds; ++i) 
    {
      int fd = fds[i].fd;
      if(fds[i].events & (POLLIN|POLLPRI) && FD_ISSET(fd, &ifds))
        fds[i].revents |= POLLIN;
      if(fds[i].events & POLLOUT && FD_ISSET(fd, &ofds))
        fds[i].revents |= POLLOUT;
      if(FD_ISSET(fd, &efds)) // Some error was detected ... should be some way to know.
        fds[i].revents |= POLLHUP;
#ifdef DEBUG_POLL
      printf("%d %d %d revent = %x\n", 
      FD_ISSET(fd, &ifds), FD_ISSET(fd, &ofds), FD_ISSET(fd, &efds), 
      fds[i].revents
      );
#endif
    }
  }
  return rc;
}

#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
  #define DELTA_EPOCH_IN_MICROSECS  11644473600000000Ui64
#else
  #define DELTA_EPOCH_IN_MICROSECS  11644473600000000ULL
#endif
 
struct timezone 
{
  int  tz_minuteswest; /* minutes W of Greenwich */
  int  tz_dsttime;     /* type of dst correction */
};
 
int win32_gettimeofday(struct timeval *tv, struct timezone *tz)
{
  FILETIME ft;
  unsigned __int64 tmpres = 0;
  static int tzflag;
 
  if (NULL != tv)
  {
    GetSystemTimeAsFileTime(&ft);
 
    tmpres |= ft.dwHighDateTime;
    tmpres <<= 32;
    tmpres |= ft.dwLowDateTime;
 
    /*converting file time to unix epoch*/
    tmpres -= DELTA_EPOCH_IN_MICROSECS; 
    tmpres /= 10;  /*convert into microseconds*/
    tv->tv_sec = (long)(tmpres / 1000000UL);
    tv->tv_usec = (long)(tmpres % 1000000UL);
  }
 
  if (NULL != tz)
  {
    if (!tzflag)
    {
      _tzset();
      tzflag++;
    }
    tz->tz_minuteswest = _timezone / 60;
    tz->tz_dsttime = _daylight;
  }
 
  return 0;
}

#endif

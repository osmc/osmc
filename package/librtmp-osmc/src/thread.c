/*  Thread compatibility glue
 *  Copyright (C) 2009 Howard Chu
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with RTMPDump; see the file COPYING.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "thread.h"
#include "librtmp/log.h"

#ifdef WIN32

#include <errno.h>

HANDLE
ThreadCreate(thrfunc *routine, void *args)
{
  HANDLE thd;

  thd = (HANDLE) _beginthread(routine, 0, args);
  if (thd == -1L)
    RTMP_LogPrintf("%s, _beginthread failed with %d\n", __FUNCTION__, errno);

  return thd;
}
#else
pthread_t
ThreadCreate(thrfunc *routine, void *args)
{
  pthread_t id = 0;
  pthread_attr_t attributes;
  int ret;

  pthread_attr_init(&attributes);
  pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED);

  ret =
    pthread_create(&id, &attributes, routine, args);
  if (ret != 0)
    RTMP_LogPrintf("%s, pthread_create failed with %d\n", __FUNCTION__, ret);

  return id;
}
#endif

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

#ifndef __THREAD_H__
#define __THREAD_H__ 1

#ifdef WIN32
#include <windows.h>
#include <process.h>
#define TFTYPE	void
#define TFRET()
#define THANDLE	HANDLE
#else
#include <pthread.h>
#define TFTYPE	void *
#define TFRET()	return 0
#define THANDLE pthread_t
#endif
typedef TFTYPE (thrfunc)(void *arg);

THANDLE ThreadCreate(thrfunc *routine, void *args);
#endif /* __THREAD_H__ */

/*
 * gettimeofday.h
 * Copyright (C) 2000-2003 Michel Lespinasse <walken@zoy.org>
 * Copyright (C) 1999-2000 Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
 *
 * This file is part of mpeg2dec, a free MPEG-2 video stream decoder.
 * See http://libmpeg2.sourceforge.net/ for updates.
 *
 * mpeg2dec is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * mpeg2dec is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef LIBMPEG2_GETTIMEOFDAY_H
#define LIBMPEG2_GETTIMEOFDAY_H

#if defined(HAVE_STRUCT_TIMEVAL) && defined(HAVE_GETTIMEOFDAY)
#if defined(TIME_WITH_SYS_TIME)
#include <sys/time.h>
#include <time.h>
#elif defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif
#elif defined(HAVE_SYS_TIMEB_H) && defined(HAVE_FTIME)

#define HAVE_GETTIMEOFDAY 1
#define CUSTOM_GETTIMEOFDAY 1

struct timeval {
    long tv_sec;
    long tv_usec;
};

void gettimeofday (struct timeval * tp, void * dummy);

#else
#undef HAVE_GETTIMEOFDAY
#endif

#endif /* LIBMPEG2_GETTIMEOFDAY_H */

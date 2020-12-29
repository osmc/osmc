/*
 * drivers/amlogic/media/video_processor/videosync/videosync.h
 *
 * Copyright (C) 2017 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */

#ifndef _VIDEOSYNC_H
#define _VIDEOSYNC_H

#define VIDEOSYNC_IOC_MAGIC  'P'
#define VIDEOSYNC_IOC_ALLOC_ID   _IOR(VIDEOSYNC_IOC_MAGIC, 0x00, int)
#define VIDEOSYNC_IOC_FREE_ID    _IOW(VIDEOSYNC_IOC_MAGIC, 0x01, int)
#define VIDEOSYNC_IOC_SET_FREERUN_MODE    _IOW(VIDEOSYNC_IOC_MAGIC, 0x02, int)
#define VIDEOSYNC_IOC_GET_FREERUN_MODE    _IOR(VIDEOSYNC_IOC_MAGIC, 0x03, int)
#define VIDEOSYNC_IOC_SET_OMX_VPTS _IOW(VIDEOSYNC_IOC_MAGIC, 0x04, unsigned int)
#define VIDEOSYNC_IOC_GET_OMX_VPTS _IOR(VIDEOSYNC_IOC_MAGIC, 0x05, unsigned int)
#define VIDEOSYNC_IOC_GET_OMX_VERSION \
    _IOR(VIDEOSYNC_IOC_MAGIC, 0x06, unsigned int)
#define VIDEOSYNC_IOC_SET_OMX_ZORDER \
    _IOW(VIDEOSYNC_IOC_MAGIC, 0x07, unsigned int)
#define VIDEOSYNC_IOC_SET_FIRST_FRAME_NOSYNC \
    _IOR(VIDEOSYNC_IOC_MAGIC, 0x08, unsigned int)
#define VIDEOSYNC_IOC_SET_VPAUSE \
    _IOW(VIDEOSYNC_IOC_MAGIC, 0x09, unsigned int)
#define VIDEOSYNC_IOC_SET_VMASTER \
    _IOW(VIDEOSYNC_IOC_MAGIC, 0x0a, unsigned int)
#define VIDEOSYNC_IOC_GET_VPTS \
    _IOR(VIDEOSYNC_IOC_MAGIC, 0x0b, unsigned int)
#define VIDEOSYNC_IOC_GET_PCRSCR \
    _IOR(VIDEOSYNC_IOC_MAGIC, 0x0c, unsigned int)

#endif


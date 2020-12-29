/*
 * Copyright (C) 2010 Amlogic Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */



#ifndef __AMVIDEOCAP_HEADHER_
#define __AMVIDEOCAP_HEADHER_
#define AMVIDEOCAP_IOC_MAGIC  'V'
#include <linux/videodev2.h>

#define CAP_FLAG_AT_CURRENT     0
#define CAP_FLAG_AT_TIME_WINDOW 1
#define CAP_FLAG_AT_END         2



/*
format see linux/ge2d/ge2d.h
like:
GE2D_FORMAT_S24_RGB
*/
#define ENDIAN_SHIFT        24
#define LITTLE_ENDIAN       (1 << ENDIAN_SHIFT)
#define FMT_S24_RGB             (LITTLE_ENDIAN|0x00200) /* 10_00_0_00_0_00 */
#define FMT_S16_RGB             (LITTLE_ENDIAN|0x00100) /* 01_00_0_00_0_00 */
#define FMT_S32_RGBA           (LITTLE_ENDIAN|0x00300) /* 11_00_0_00_0_00 */

#define COLOR_MAP_SHIFT        20
#define COLOR_MAP_MASK         (0xf << COLOR_MAP_SHIFT)
/* 16 bit */
#define COLOR_MAP_RGB565       (5 << COLOR_MAP_SHIFT)
/* 24 bit */
#define COLOR_MAP_RGB888       (0 << COLOR_MAP_SHIFT)
#define COLOR_MAP_BGR888       (5 << COLOR_MAP_SHIFT)
/* 32 bit */
#define COLOR_MAP_RGBA8888      (0 << COLOR_MAP_SHIFT)
#define COLOR_MAP_ARGB8888      (1 << COLOR_MAP_SHIFT)
#define COLOR_MAP_ABGR8888      (2 << COLOR_MAP_SHIFT)
#define COLOR_MAP_BGRA8888      (3 << COLOR_MAP_SHIFT)

/*16 bit*/
#define FORMAT_S16_RGB_565         (FMT_S16_RGB | COLOR_MAP_RGB565)
/*24 bit*/
#define FORMAT_S24_BGR             (FMT_S24_RGB | COLOR_MAP_BGR888)
#define FORMAT_S24_RGB             (FMT_S24_RGB | COLOR_MAP_RGB888)
/*32 bit*/
#define FORMAT_S32_ARGB        (FMT_S32_RGBA    | COLOR_MAP_ARGB8888)
#define FORMAT_S32_ABGR        (FMT_S32_RGBA    | COLOR_MAP_ABGR8888)
#define FORMAT_S32_BGRA        (FMT_S32_RGBA    | COLOR_MAP_BGRA8888)
#define FORMAT_S32_RGBA        (FMT_S32_RGBA    | COLOR_MAP_RGBA8888)

#define AMVIDEOCAP_IOW_SET_WANTFRAME_FORMAT             _IOW(AMVIDEOCAP_IOC_MAGIC, 0x01, int)
#define AMVIDEOCAP_IOW_SET_WANTFRAME_WIDTH              _IOW(AMVIDEOCAP_IOC_MAGIC, 0x02, int)
#define AMVIDEOCAP_IOW_SET_WANTFRAME_HEIGHT             _IOW(AMVIDEOCAP_IOC_MAGIC, 0x03, int)
#define AMVIDEOCAP_IOW_SET_WANTFRAME_TIMESTAMP_MS       _IOW(AMVIDEOCAP_IOC_MAGIC, 0x04, u64)
#define AMVIDEOCAP_IOW_SET_WANTFRAME_WAIT_MAX_MS        _IOW(AMVIDEOCAP_IOC_MAGIC, 0x05, u64)
#define AMVIDEOCAP_IOW_SET_WANTFRAME_AT_FLAGS           _IOW(AMVIDEOCAP_IOC_MAGIC, 0x06, int)


#define AMVIDEOCAP_IOR_GET_FRAME_FORMAT             _IOR(AMVIDEOCAP_IOC_MAGIC, 0x10, int)
#define AMVIDEOCAP_IOR_GET_FRAME_WIDTH              _IOR(AMVIDEOCAP_IOC_MAGIC, 0x11, int)
#define AMVIDEOCAP_IOR_GET_FRAME_HEIGHT             _IOR(AMVIDEOCAP_IOC_MAGIC, 0x12, int)
#define AMVIDEOCAP_IOR_GET_FRAME_TIMESTAMP_MS       _IOR(AMVIDEOCAP_IOC_MAGIC, 0x13, int)


#define AMVIDEOCAP_IOR_GET_SRCFRAME_FORMAT                  _IOR(AMVIDEOCAP_IOC_MAGIC, 0x20, int)
#define AMVIDEOCAP_IOR_GET_SRCFRAME_WIDTH                   _IOR(AMVIDEOCAP_IOC_MAGIC, 0x21, int)
#define AMVIDEOCAP_IOR_GET_SRCFRAME_HEIGHT                  _IOR(AMVIDEOCAP_IOC_MAGIC, 0x22, int)


#define AMVIDEOCAP_IOR_GET_STATE                    _IOR(AMVIDEOCAP_IOC_MAGIC, 0x31, int)
#define AMVIDEOCAP_IOW_SET_START_CAPTURE            _IOW(AMVIDEOCAP_IOC_MAGIC, 0x32, int)
#define AMVIDEOCAP_IOW_SET_CANCEL_CAPTURE           _IOW(AMVIDEOCAP_IOC_MAGIC, 0x33, int)

#define AMVIDEOCAP_IOR_SET_SRC_X                _IOR(AMVIDEOCAP_IOC_MAGIC, 0x40, int)
#define AMVIDEOCAP_IOR_SET_SRC_Y                _IOR(AMVIDEOCAP_IOC_MAGIC, 0x41, int)
#define AMVIDEOCAP_IOR_SET_SRC_WIDTH            _IOR(AMVIDEOCAP_IOC_MAGIC, 0x42, int)
#define AMVIDEOCAP_IOR_SET_SRC_HEIGHT           _IOR(AMVIDEOCAP_IOC_MAGIC, 0x43, int)

enum amvideocap_state {
    AMVIDEOCAP_STATE_INIT = 0,
    AMVIDEOCAP_STATE_ON_CAPTURE = 200,
    AMVIDEOCAP_STATE_FINISHED_CAPTURE = 300,
    AMVIDEOCAP_STATE_ERROR = 0xffff,
};
#endif//__AMVIDEOCAP_HEADHER_


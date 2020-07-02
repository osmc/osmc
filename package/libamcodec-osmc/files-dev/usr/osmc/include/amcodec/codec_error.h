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



/**
* @file codec_error.h
* @brief  Codec error type definitions
* @author Zhang Chen <chen.zhang@amlogic.com>
* @version 1.0.0
* @date 2011-02-24
*/
/* Copyright (C) 2007-2011, Amlogic Inc.
* All right reserved
*
*/

#ifndef CODEC_ERROR_H_
#define CODEC_ERROR_H_

#define C_PAE                               (0x01000000)

#define CODEC_ERROR_NONE                    ( 0)
#define CODEC_ERROR_INVAL                   (C_PAE | 1)
#define CODEC_ERROR_NOMEM                   (C_PAE | 2)
#define CODEC_ERROR_BUSY                    (C_PAE | 3)
#define CODEC_ERROR_IO                      (C_PAE | 4)
#define CODEC_ERROR_PARAMETER               (C_PAE | 5)
#define CODEC_ERROR_AUDIO_TYPE_UNKNOW       (C_PAE | 6)
#define CODEC_ERROR_VIDEO_TYPE_UNKNOW       (C_PAE | 7)
#define CODEC_ERROR_STREAM_TYPE_UNKNOW      (C_PAE | 8)
#define CODEC_ERROR_VDEC_TYPE_UNKNOW        (C_PAE | 9)

#define CODEC_ERROR_INIT_FAILED             (C_PAE | 10)
#define CODEC_ERROR_SET_BUFSIZE_FAILED      (C_PAE | 11)
#define CODEC_OPEN_HANDLE_FAILED            (C_PAE | 12)




#endif


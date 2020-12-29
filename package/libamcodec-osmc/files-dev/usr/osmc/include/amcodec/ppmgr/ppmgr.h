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
* @file ppmgr.h
* @brief  Porting from ppmgr driver for codec ioctl commands
* @author Tim Yao <timyao@amlogic.com>
* @version 1.0.0
* @date 2011-02-24
*/
/* Copyright (C) 2007-2011, Amlogic Inc.
* All right reserved
*
*/

#ifndef PPMGR_H
#define PPMGR_H

#define PPMGR_IOC_MAGIC  'P'
//#define PPMGR_IOC_2OSD0       _IOW(PPMGR_IOC_MAGIC, 0x00, unsigned int)
//#define PPMGR_IOC_ENABLE_PP _IOW(PPMGR_IOC_MAGIC,0X01,unsigned int)
//#define PPMGR_IOC_CONFIG_FRAME  _IOW(PPMGR_IOC_MAGIC,0X02,unsigned int)
#define PPMGR_IOC_GET_ANGLE  _IOR(PPMGR_IOC_MAGIC,0X03,unsigned long)
#define PPMGR_IOC_SET_ANGLE  _IOW(PPMGR_IOC_MAGIC,0X04,unsigned long)

#endif /* PPMGR_H */


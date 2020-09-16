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



#ifndef __AUDIODSP_UPDATE_FORMAT_H__
#define __AUDIODSP_UPDATE_FORMAT_H__

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <audio-dec.h>
#include <audiodsp.h>
#include <log-print.h>

void adec_reset_track_enable(int enable_flag);
void adec_reset_track(aml_audio_dec_t *audec);
int audiodsp_format_update(aml_audio_dec_t *audec);
void audiodsp_set_format_changed_flag(int val);

int audiodsp_get_pcm_left_len();

#endif
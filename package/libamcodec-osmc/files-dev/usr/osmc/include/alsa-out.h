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



#ifndef ALSA_OUT_H
#define ALSA_OUT_H

#define PCM_DEVICE_DEFAULT      "default"
//#define PCM_DEVICE_DEFAULT  "hw:0,2"
#define OUTPUT_BUFFER_SIZE      (8*1024)

typedef struct {
    pthread_t playback_tid;
    pthread_mutex_t playback_mutex;
    pthread_cond_t playback_cond;
    snd_pcm_t *handle;
    snd_pcm_format_t format;
    size_t bits_per_sample;
    size_t bits_per_frame;
    int buffer_size;
    unsigned int channelcount;
    unsigned int rate;
    int oversample;
    int realchanl;
    int flag;
    int stop_flag;
    int pause_flag;
    int wait_flag;
	unsigned char decode_buffer[OUTPUT_BUFFER_SIZE + 64];
	float staging_vol;
	float target_vol;
	float last_vol;
	float fade;
} alsa_param_t;
#endif

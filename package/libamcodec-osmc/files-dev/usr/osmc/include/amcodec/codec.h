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
* @file codec.h
* @brief  Function prototypes of codec lib
* @author Zhang Chen <chen.zhang@amlogic.com>
* @version 1.0.0
* @date 2011-02-24
*/
/* Copyright (C) 2007-2011, Amlogic Inc.
* All right reserved
*
*/
#ifndef CODEC_CTRL_H_
#define CODEC_CTRL_H_

#include "codec_type.h"
#include "codec_error.h"


int codec_init(codec_para_t *);
int codec_close(codec_para_t *);
void codec_close_audio_async(codec_para_t *pcodec);
void codec_audio_basic_init(void);
void codec_close_audio(codec_para_t *);
void codec_resume_audio(codec_para_t *, unsigned int);
int codec_reset(codec_para_t *);
int codec_init_sub(codec_para_t *);
int codec_open_sub_read(void);
int codec_close_sub(codec_para_t *);
int codec_close_sub_fd(CODEC_HANDLE);
int codec_reset_subtile(codec_para_t *pcodec);
int codec_poll_sub(codec_para_t *);
int codec_poll_sub_fd(CODEC_HANDLE, int);
int codec_get_sub_size(codec_para_t *);
int codec_get_sub_size_fd(CODEC_HANDLE);
int codec_read_sub_data(codec_para_t *, char *, unsigned int);
int codec_read_sub_data_fd(CODEC_HANDLE, char *, unsigned int);
int codec_write_sub_data(codec_para_t *, char *, unsigned int);
int codec_init_cntl(codec_para_t *);
int codec_close_cntl(codec_para_t *);
int codec_poll_cntl(codec_para_t *);
int codec_get_cntl_state(codec_para_t *);
int codec_get_cntl_vpts(codec_para_t *pcodec);
int codec_set_cntl_mode(codec_para_t *, unsigned int);
int codec_set_mode(codec_para_t *, unsigned int);
int codec_set_cntl_avthresh(codec_para_t *, unsigned int);
int codec_set_cntl_syncthresh(codec_para_t *pcodec, unsigned int syncthresh);
int codec_reset_audio(codec_para_t *pcodec);
int codec_set_audio_pid(codec_para_t *pcodec);
int codec_set_sub_id(codec_para_t *pcodec);
int codec_set_sub_type(codec_para_t *pcodec);
int codec_audio_reinit(codec_para_t *pcodec);
int codec_set_dec_reset(codec_para_t *pcodec);

int codec_write(codec_para_t *pcodec, void *buffer, int len);
int codec_checkin_pts(codec_para_t *pcodec, unsigned long pts);
int codec_get_vbuf_state(codec_para_t *, struct buf_status *);
int codec_get_abuf_state(codec_para_t *, struct buf_status *);
int codec_get_vdec_state(codec_para_t *, struct vdec_status *);
int codec_get_adec_state(codec_para_t *, struct adec_status *);

int codec_pause(codec_para_t *);
int codec_resume(codec_para_t *);
int codec_audio_search(codec_para_t *p);
int codec_set_mute(codec_para_t *p, int mute);
int codec_get_volume_range(codec_para_t *, int *min, int *max);
int codec_set_volume(codec_para_t *, float val);
int codec_get_volume(codec_para_t *, float *val);
int codec_set_lrvolume(codec_para_t *, float lvol, float rvol);
int codec_get_lrvolume(codec_para_t *, float *lvol, float* rvol);
int codec_get_mutesta(codec_para_t *);
int codec_set_volume_balance(codec_para_t *, int); /*left��0-100)right*/
int codec_swap_left_right(codec_para_t *);
int codec_left_mono(codec_para_t *p);
int codec_right_mono(codec_para_t *p);
int codec_stereo(codec_para_t *p);
int codec_lr_mix_set(codec_para_t *p, int enable);
int codec_get_soundtrack(codec_para_t *p, int* strack);
int codec_audio_automute(void *priv, int auto_mute);
int codec_audio_spectrum_switch(codec_para_t *p, int isStart, int interval);
int codec_audio_isready(codec_para_t *p);
int codec_audio_get_nb_frames(codec_para_t *p);
int codec_audio_set_audioinfo(codec_para_t *p);
int codec_pcmpara_Applied_get(codec_para_t *p, int *pfs, int *pch,int *lfepresent);

int codec_get_apts(codec_para_t *pcodec);
int codec_get_vpts(codec_para_t *pcodec);
int codec_get_pcrscr(codec_para_t *pcodec);
int codec_set_pcrscr(codec_para_t *pcodec, int val);
int codec_set_syncenable(codec_para_t *pcodec, int enable);
int codec_set_sync_audio_discont(codec_para_t *pcodec, int discontinue);
int codec_get_sync_audio_discont(codec_para_t *pcodec);
int codec_set_sync_video_discont(codec_para_t *pcodec, int discontinue);
int codec_get_sync_video_discont(codec_para_t *pcodec);
unsigned long codec_get_sync_audio_discont_diff(codec_para_t *pcodec);
unsigned long codec_get_sync_video_discont_diff(codec_para_t *pcodec);
int codec_set_sync_audio_discont_diff(codec_para_t *pcodec, unsigned long discontinue_diff);
int codec_set_sync_video_discont_diff(codec_para_t *pcodec, unsigned long discontinue_diff);
int codec_get_sub_num(codec_para_t *pcodec);
int codec_get_sub_info(codec_para_t *pcodec, subtitle_info_t *sub_info);

int codec_set_av_threshold(codec_para_t *pcodec, int threshold);

int codec_get_freerun_mode(codec_para_t *pcodec);
int codec_set_freerun_mode(codec_para_t *pcodec, unsigned int mode);

int codec_init_audio_utils(codec_para_t *pcodec);
int codec_release_audio_utils(codec_para_t *pcodec);
int codec_set_audio_resample_ena(codec_para_t *pcodec, unsigned long mode);
int codec_get_audio_resample_ena(codec_para_t *pcodec);
int codec_set_audio_resample_type(codec_para_t *pcodec, unsigned long type);

int codec_set_video_delay_limited_ms(codec_para_t *pcodec, int delay_ms);
int codec_get_video_delay_limited_ms(codec_para_t *pcodec, int *delay_ms);
int codec_set_audio_delay_limited_ms(codec_para_t *pcodec, int delay_ms);
int codec_get_audio_delay_limited_ms(codec_para_t *pcodec, int *delay_ms);
int codec_get_audio_cur_delay_ms(codec_para_t *pcodec, int *delay_ms);
int codec_get_video_cur_delay_ms(codec_para_t *pcodec, int *delay_ms);
int codec_get_video_cur_bitrate(codec_para_t *pcodec, int *bitrate);
int codec_get_audio_cur_bitrate(codec_para_t *pcodec, int *bitrate);

int codec_set_vsync_upint(codec_para_t *pcodec, unsigned int mode);

int codec_get_last_checkout_apts(codec_para_t* pcodec, unsigned long *apts);
int codec_get_last_checkin_apts(codec_para_t* pcodec, unsigned long *apts);
int codec_disalbe_slowsync(codec_para_t *pcodec, int disable_slowsync);
int codec_utils_set_video_position(int x, int y, int w, int h, int rotation);
int codec_amsub_read_outdata(codec_para_t *pcodec,amsub_info_t *amsub_info);
void codec_close_subtitle(codec_para_t *pcodec);
void codec_resume_subtitle(codec_para_t *pcodec, unsigned int has_sub);
#endif

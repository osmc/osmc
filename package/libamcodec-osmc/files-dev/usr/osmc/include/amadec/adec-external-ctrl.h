/**
 * \file adec-external-ctrl.h
 * \brief  Function prototypes of Audio Dec
 * \version 1.0.0
 * \date 2011-03-08
 */
/* Copyright (C) 2007-2011, Amlogic Inc.
 * All right reserved
 *
 */
#ifndef ADEC_EXTERNAL_H
#define ADEC_EXTERNAL_H
#ifndef ANDROID
#include <adec-macros.h>
#endif
#ifdef  __cplusplus
extern "C"
{
#endif

    int audio_decode_init(void **handle, arm_audio_info *pcodec);
    int audio_decode_start(void *handle);
    int audio_decode_pause(void *handle);
    int audio_decode_resume(void *handle);
    int audio_decode_stop(void *handle);
    int audio_decode_release(void **handle);
    int audio_decode_automute(void *, int);
    int audio_decode_set_mute(void *handle, int);
    int audio_decode_set_volume(void *, float);
    int audio_decode_get_volume(void *, float *);
    int audio_decode_set_pre_gain(void *, float);
    int audio_decode_get_pre_gain(void *, float *);
    int audio_decode_set_pre_mute(void *, uint);
    int audio_decode_get_pre_mute(void *, uint *);
    int audio_channels_swap(void *);
    int audio_channel_left_mono(void *);
    int audio_channel_right_mono(void *);
    int audio_channel_stereo(void *);
    int audio_output_muted(void *handle);
    int audio_dec_ready(void *handle);
    int audio_get_decoded_nb_frames(void *handle);

    int audio_decode_set_lrvolume(void *, float lvol, float rvol);
    int audio_decode_get_lrvolume(void *, float* lvol, float* rvol);
    int audio_set_av_sync_threshold(void *, int);
    int audio_get_soundtrack(void *, int*);
    int get_audio_decoder(void);
    int get_decoder_status(void *p, struct adec_status *adec);
    int audio_channel_lrmix_flag_set(void *, int enable);
    int audio_decpara_get(void *handle, int *pfs, int *pch,int *lfepresent);
    int audio_get_format_supported(int format);
    int audio_set_associate_enable(void *handle, unsigned int enable);
    int audio_send_associate_data(void *handle, uint8_t *buf, size_t size);
    int acodec_buffer_write(void *p,char* buffer, size_t bytes);
    int get_abuf_state(void *p,struct buf_status *buf);
	int checkin_pts(void *p, unsigned long pts);
	int acodec_get_apts(void *p,unsigned long *pts);
#ifdef  __cplusplus
}
#endif

#endif

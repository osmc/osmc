/**
 * \file audio-out.h
 * \brief  Definitiond Of Audio Out Structures
 * \version 1.0.0
 * \date 2011-03-08
 */
/* Copyright (C) 2007-2011, Amlogic Inc.
 * All right reserved
 *
 */
#ifndef AUDIO_OUT_H
#define AUDIO_OUT_H

#include <adec-macros.h>

ADEC_BEGIN_DECLS

struct aml_audio_dec;

typedef struct {
    void *private_data;
    void *private_data_raw;
    int (*init)(struct aml_audio_dec *);
    int (*start)(struct aml_audio_dec *);
    int (*pause)(struct aml_audio_dec *);
    int (*resume)(struct aml_audio_dec *);
    int (*stop)(struct aml_audio_dec *);
    unsigned long(*latency)(struct aml_audio_dec *);                    /* get latency in ms */
    int (*mute)(struct aml_audio_dec *, adec_bool_t);           /* 1: enable mute ; 0: disable mute */
    int (*set_volume)(struct aml_audio_dec *, float);
    int (*set_lrvolume)(struct aml_audio_dec *, float, float);
    int (*set_track_rate)(struct aml_audio_dec *, void *rate);
    int audio_out_raw_enable;
    float track_rate;
} audio_out_operations_t;

ADEC_END_DECLS

#endif

/**
 * \file audiodsp.h
 * \brief  Definitiond Of Audiodsp Types And Structures
 * \version 1.0.0
 * \date 2011-03-08
 */
/* Copyright (C) 2007-2011, Amlogic Inc.
 * All right reserved
 *
 */
#ifndef AUDIODSP_H
#define AUDIODSP_H

#include <adec-macros.h>
#include <adec-types.h>

ADEC_BEGIN_DECLS

#define DSP_DEV_NOD "/dev/audiodsp0"

//#ifdef ANDROID
#if 1
#define AUDIODSP_SET_FMT                        _IOW('a',1,long)
#define AUDIODSP_START                          _IOW('a',2,long)
#define AUDIODSP_STOP                           _IOW('a',3,long)
#define AUDIODSP_DECODE_START                   _IOW('a',4,long)
#define AUDIODSP_DECODE_STOP                    _IOW('a',5,long)
#define AUDIODSP_REGISTER_FIRMWARE          _IOW('a',6,long)
#define AUDIODSP_UNREGISTER_ALLFIRMWARE     _IOW('a',7,long)
#define AUDIODSP_SYNC_SET_APTS                     _IOW('a', 10, unsigned long)
#define AUDIODSP_WAIT_FORMAT                 _IOW('a',11,long)

#define AUDIODSP_SKIP_BYTES                     _IOW('a', 13, unsigned long)

#define AUDIODSP_GET_CHANNELS_NUM           _IOR('r',1,long)
#define AUDIODSP_GET_SAMPLERATE             _IOR('r',2,long)
#define AUDIODSP_GET_BITS_PER_SAMPLE            _IOR('r',3,long)
#define AUDIODSP_GET_PTS                        _IOR('r',4,long)
#define AUDIODSP_GET_DECODED_NB_FRAMES          _IOR('r',5,long)
#define AUDIODSP_GET_FIRST_PTS_FLAG             _IOR('r',6,long)
#define AUDIODSP_SYNC_GET_APTS                  _IOR('r',7,unsigned long)
#define AUDIODSP_SYNC_GET_PCRSCR                _IOR('r',8,unsigned long)
#define AUDIODSP_AUTOMUTE_ON                    _IOW('r',9,unsigned long)
#define AUDIODSP_AUTOMUTE_OFF                   _IOW('r',10,unsigned long)
#define AUDIODSP_GET_PCM_LEVEL                  _IOR('r',12,unsigned long)
#define AUDIODSP_SET_PCM_BUF_SIZE                 _IOW('r',13,long)
#define AMAUDIO_IOC_SET_RESAMPLE_ENA              _IOW('A', 0x19, unsigned long)
#define AMAUDIO_IOC_GET_RESAMPLE_ENA              _IOR('A', 0x1a, unsigned long)
#else

#define AUDIODSP_SET_FMT                        _IOW('a',1,sizeof(long))
#define AUDIODSP_START                          _IOW('a',2,sizeof(long))
#define AUDIODSP_STOP                           _IOW('a',3,sizeof(long))
#define AUDIODSP_DECODE_START                   _IOW('a',4,sizeof(long))
#define AUDIODSP_DECODE_STOP                    _IOW('a',5,sizeof(long))
#define AUDIODSP_REGISTER_FIRMWARE          _IOW('a',6,sizeof(long))
#define AUDIODSP_UNREGISTER_ALLFIRMWARE     _IOW('a',7,sizeof(long))
#define AUDIODSP_WAIT_FORMAT                 _IOW('a',11,long)


#define AUDIODSP_GET_CHANNELS_NUM           _IOR('r',1,sizeof(long))
#define AUDIODSP_GET_SAMPLERATE             _IOR('r',2,sizeof(long))
#define AUDIODSP_GET_BITS_PER_SAMPLE            _IOR('r',3,sizeof(long))
#define AUDIODSP_GET_PTS                        _IOR('r',4,sizeof(long))
#define AUDIODSP_GET_DECODED_NB_FRAMES          _IOR('r',5,sizeof(long))
#define AUDIODSP_GET_FIRST_PTS_FLAG             _IOR('r',6,sizeof(long))
#endif


#define MCODEC_FMT_MPEG123       (1<<0)
#define MCODEC_FMT_AAC          (1<<1)
#define MCODEC_FMT_AC3          (1<<2)
#define MCODEC_FMT_DTS              (1<<3)
#define MCODEC_FMT_FLAC         (1<<4)
#define MCODEC_FMT_COOK         (1<<5)
#define MCODEC_FMT_AMR          (1<<6)
#define MCODEC_FMT_RAAC         (1<<7)
#define MCODEC_FMT_ADPCM          (1<<8)
#define MCODEC_FMT_WMA       (1<<9)
#define MCODEC_FMT_PCM               (1<<10)
#define MCODEC_FMT_WMAPRO       (1<<11)
#define MCODEC_FMT_ALAC             (1<<12)
#define MCODEC_FMT_VORBIS               (1<<13)
#define MCODEC_FMT_AAC_LATM          (1<<14)
#define MCODEC_FMT_APE          (1<<15)
#define MCODEC_FMT_EAC3          (1<<16)

/*********************************************************************************************/
typedef struct dsp_operations dsp_operations_t;

struct dsp_operations {
    int dsp_file_fd;
    int dsp_on;
    unsigned long kernel_audio_pts;
    unsigned long last_audio_pts;
    unsigned long last_pts_valid;
    int (*dsp_read)(dsp_operations_t *dsp_ops, char *buffer, int len);                                        /* read pcm stream from dsp */
    int (*dsp_read_raw)(dsp_operations_t *dsp_ops, char *buffer, int len); /* read raw stream from dsp */
    unsigned long(*get_cur_pts)(dsp_operations_t *);
    unsigned long(*get_cur_pcrscr)(dsp_operations_t *);
    int (*set_cur_apts)(dsp_operations_t *dsp_ops, unsigned long apts);
    int (*set_skip_bytes)(dsp_operations_t *dsp_ops, unsigned int skip_bytes);

    int amstream_fd;
    void *audec;
};

typedef struct {
    int cmd;
    int fmt;
    int data_len;
    char *data;
} audiodsp_cmd_t;

typedef struct {
    int     id;
    int     fmt;
    char    name[64];
} firmware_s_t;


/************************************************************************************************/

int audiodsp_stream_read(dsp_operations_t *dsp_ops, char *buffer, int size);
unsigned long  audiodsp_get_pts(dsp_operations_t *dsp_ops);
unsigned long  audiodsp_get_pcrscr(dsp_operations_t *dsp_ops);
int audiodsp_set_apts(dsp_operations_t *dsp_ops, unsigned long apts);
int audiodsp_get_decoded_nb_frames(dsp_operations_t *dsp_ops);
int audiodsp_get_first_pts_flag(dsp_operations_t *dsp_ops);
int audiodsp_automute_on(dsp_operations_t *dsp_ops);
int audiodsp_automute_off(dsp_operations_t *dsp_ops);
int audiodsp_set_skip_bytes(dsp_operations_t* dsp_ops, unsigned int bytes);
ADEC_END_DECLS

#endif

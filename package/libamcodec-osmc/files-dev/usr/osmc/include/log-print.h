/**
 * \file log-print.h
 * \brief  Definitiond Of Print Functions
 * \version 1.0.0
 * \date 2011-03-08
 */
/* Copyright (C) 2007-2011, Amlogic Inc.
 * All right reserved
 *
 */
#ifndef LOG_PRINT_H
#define LOG_PRINT_H

#ifdef ANDROID
#include <android/log.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#define  LOG_TAG    "amadec"
#define adec_print(...) __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#else
#include <stdio.h>
#include <time.h>
#define PROPERTY_VALUE_MAX 124
#define LOG_DEFAULT  0
static int s_adec_debug_level = -1;
#define adec_print(f,s...) do{\
    if (s_adec_debug_level < 0) {\
        char * level=getenv("LOG_LEVEL");\
        if(level)\
          s_adec_debug_level=atoi(level);\
        else\
          s_adec_debug_level=0;\
    }\
    if (s_adec_debug_level > LOG_DEFAULT) {\
        struct timespec ts;\
        clock_gettime(CLOCK_MONOTONIC, &ts);\
        fprintf(stderr,"%d.%06d %s:%d " f, ts.tv_sec, ts.tv_nsec/1000, __FUNCTION__, __LINE__, ##s);\
    }\
} while(0)

#endif


#endif

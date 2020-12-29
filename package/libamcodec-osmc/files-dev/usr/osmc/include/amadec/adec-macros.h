/**
 * \file adec-macros.h
 * \brief  Some Macros for Audio Dec
 * \version 1.0.0
 * \date 2011-03-08
 */
/* Copyright (C) 2007-2011, Amlogic Inc.
 * All right reserved
 *
 */
#ifndef ADEC_MACROS_H
#define ADEC_MACROS_H

#include <stdint.h>

#ifdef  __cplusplus
#define ADEC_BEGIN_DECLS    extern "C" {
#define ADEC_END_DECLS  }
#else
#define ADEC_BEGIN_DECLS
#define ADEC_END_DECLS
#endif


#ifndef TRUE
#define TRUE    1
#endif

#ifndef FALSE
#define FALSE   0
#endif
#define PROPERTY_VALUE_MAX 124

typedef unsigned int    adec_bool_t;

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x)  (sizeof(x) / sizeof((x)[0]))
#endif

#define MAX(X, Y)    ((X) > (Y)) ? (X) : (Y)
#define MIN(X, Y)    ((X) < (Y)) ? (X) : (Y)

#endif

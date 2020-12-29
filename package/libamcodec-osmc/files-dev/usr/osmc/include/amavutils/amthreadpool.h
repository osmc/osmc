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



#ifndef AM_LIBPLAYER_THREAD_POOL
#define  AM_LIBPLAYER_THREAD_POOL
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>

#ifdef ANDROID
#define AMTHREADPOOL_SLEEP_US_MONOTONIC
///#define AMTHREADPOOL_DEBUG
#endif
int amthreadpool_thread_usleep_in(int us);
int amthreadpool_thread_usleep_in_monotonic(int us);
int amthreadpool_thread_wake(pthread_t pid);
int amthreadpool_pool_thread_cancel(pthread_t pid);
int amthreadpool_pool_thread_uncancel(pthread_t pid);
int amthreadpool_thread_cancel(pthread_t pid);
int amthreadpool_thread_uncancel(pthread_t pid);

int amthreadpool_pthread_create(pthread_t * newthread,
                                __const pthread_attr_t * attr,
                                void * (*start_routine)(void *),
                                void * arg);
int amthreadpool_pthread_create_name(pthread_t * newthread,
                                     __const pthread_attr_t * attr,
                                     void * (*start_routine)(void *),
                                     void * arg, const char *name);

int amthreadpool_pthread_join(pthread_t thid, void ** ret_val);
int amthreadpool_system_init(void);
int amthreadpool_system_dump_info(void);
int amthreadpool_on_requare_exit(pthread_t pid);



#ifdef AMTHREADPOOL_DEBUG
int amthreadpool_thread_usleep_debug(int us, const char *func, int line);
#define amthreadpool_thread_usleep(us)\
    amthreadpool_thread_usleep_debug(us,__FUNCTION__,__LINE__)
#else

#ifdef AMTHREADPOOL_SLEEP_US_MONOTONIC
#define amthreadpool_thread_usleep(us)\
    amthreadpool_thread_usleep_in_monotonic(us)
#else
#define amthreadpool_thread_usleep(us)\
    amthreadpool_thread_usleep_in(us)
#endif
#endif
#endif


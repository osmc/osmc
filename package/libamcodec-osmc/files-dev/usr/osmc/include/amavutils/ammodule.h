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



#ifndef ANDROID_INCLUDE_AMMODULE_H
#define ANDROID_INCLUDE_AMMODULE_H

#include <stdint.h>
#include <sys/cdefs.h>


#define MAKE_TAG_CONSTANT(A,B,C,D) (((A) << 24) | ((B) << 16) | ((C) << 8) | (D))

#define AMPLAYER_MODULE_TAG MAKE_TAG_CONSTANT('A', 'M', 'M', 'D')

#define AMPLAYER_MAKE_API_VERSION(maj,min) \
            ((((maj) & 0xff) << 8) | ((min) & 0xff))

#define AMPLAYER_API_MAIOR  (1)
#define AMPLAYER_API_MINOR  (0) /*diff,can load,but some functions may lost.*/
#define AMPLAYER_HAL_API_VERSION AMPLAYER_MAKE_API_VERSION(AMPLAYER_API_MAIOR, AMPLAYER_API_MINOR)

#define AMPLAYER_MODULE_API_VERSION(maj,min) AMPLAYER_MAKE_API_VERSION(maj,min)



struct ammodule_t;
struct ammodule_methods_t;
typedef struct ammodule_t {
    uint32_t tag;/*AMPLAYER_MODULE_TAG*/
    uint16_t module_api_version;
#define version_major module_api_version
    uint16_t hal_api_version;
#define version_minor hal_api_version
    const char *id;
    const char *name;/*a simple name.*/
    const char *author;
    const char *descript;/*functions..*/
    struct ammodule_methods_t* methods;
    void* dso;
    uint32_t reserved[32 - 7];

} ammodule_t;

typedef struct ammodule_methods_t {
    /** Open a specific device */
    int (*init)(const struct ammodule_t* module, int flags);
    int (*release)(const struct ammodule_t* module);
} ammodule_methods_t;

#define AMPLAYER_MODULE_INFO_SYM         AMMD
#define AMPLAYER_MODULE_INFO_SYM_AS_STR  "AMMD"

#ifdef __cplusplus
extern "C" {
#endif

    int ammodule_load_module(const char *modulename, const struct ammodule_t **module);
    int ammodule_open_module(struct ammodule_t *module);
    int  ammodule_simple_load_module(char* name);
    int ammodule_match_check(const char* allmodstr, const char* modname);

#ifdef __cplusplus
}
#endif
#endif


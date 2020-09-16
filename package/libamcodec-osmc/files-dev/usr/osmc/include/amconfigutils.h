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



#ifndef FF_CONFIGS_H__
#define FF_CONFIGS_H__

#define MAX_CONFIG 128
#define CONFIG_PATH_MAX    32
#define CONFIG_VALUE_MAX   92
#define CONFIG_VALUE_OFF   (CONFIG_PATH_MAX+4)
#ifdef  __cplusplus
extern "C" {
#endif

    int am_config_init(void);
    int am_getconfig(const char * path, char *val, const char * def);
    int am_setconfig(const char * path, const char *val);
    int am_setconfig_float(const char * path, float value);
    int am_getconfig_float(const char * path, float *value);
    int am_dumpallconfigs(void);
    int am_getconfig_bool(const char * path);
    int am_getconfig_bool_def(const char * path, int def);
    int am_getconfig_int_def(const char * path, int def);
    float am_getconfig_float_def(const char * path, float defvalue);
#ifndef ANDROID	
	int property_get(const char *key, char *value, const char *default_value);
#endif	
#ifdef  __cplusplus
}
#endif
#endif


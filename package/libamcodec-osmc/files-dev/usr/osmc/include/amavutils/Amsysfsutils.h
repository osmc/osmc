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




#ifndef AMSYSFS_UTILS_H
#define AMSYSFS_UTILS_H

#ifdef  __cplusplus
extern "C" {
#endif
    int amsysfs_set_sysfs_str(const char *path, const char *val);
    int amsysfs_get_sysfs_str(const char *path, char *valstr, int size);
    int amsysfs_set_sysfs_int(const char *path, int val);
    int amsysfs_get_sysfs_int(const char *path);
    int amsysfs_set_sysfs_int16(const char *path, int val);
    int amsysfs_get_sysfs_int16(const char *path);
    unsigned long amsysfs_get_sysfs_ulong(const char *path);
    void amsysfs_write_prop(const char* key, const char* value);
#ifdef  __cplusplus
}
#endif


#endif


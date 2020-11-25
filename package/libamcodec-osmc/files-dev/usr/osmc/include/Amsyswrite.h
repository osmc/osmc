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



#ifndef AMSYSWRITE_UTILS_H
#define AMSYSWRITE_UTILS_H

#ifdef  __cplusplus
extern "C" {
#endif
    int amSystemWriteGetProperty(const char* key, char* value);
    int amSystemWriteGetPropertyStr(const char* key, char* def, char* value);
    int amSystemWriteGetPropertyInt(const char* key, int def);
    long amSystemWriteGetPropertyLong(const char* key, long def);
    int amSystemWriteGetPropertyBool(const char* key, int def);
    void amSystemWriteSetProperty(const char* key, const char* value);
    int amSystemWriteReadSysfs(const char* path, char* value);
    int amSystemWriteReadNumSysfs(const char* path, char* value, int size);
    int amSystemWriteWriteSysfs(const char* path, char* value);

#if ANDROID_PLATFORM_SDK_VERSION >= 21 //5.0
    int amSystemControlSetNativeWindowRect(int x, int y, int w, int h);
#endif


#ifdef  __cplusplus
}
#endif


#endif

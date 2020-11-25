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




#ifndef AMDISPLAY_UTILS_H
#define AMDISPLAY_UTILS_H

#ifdef  __cplusplus
extern "C" {
#endif
    int amdisplay_utils_get_size(int *width, int *height);
    int amdisplay_utils_get_size_fb2(int *width, int *height);

    /*scale osd mode ,only support x1 x2*/
    int amdisplay_utils_set_scale_mode(int scale_wx, int scale_hx);

#ifdef  __cplusplus
}
#endif


#endif


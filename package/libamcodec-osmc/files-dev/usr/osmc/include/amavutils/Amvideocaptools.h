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



#ifndef AMVIDEOCAP_TOOLS_HEAD
#define AMVIDEOCAP_TOOLS_HEAD
#include "Amvideocap.h"
//fmt ignored,always RGB888 now
int amvideocap_capframe(char *buf, int size, int *w, int *h, int fmt_ignored, int at_end, int* ret_size, int fmt);
int amvideocap_capframe_with_rect(char *buf, int size, int src_rect_x, int src_rect_y, int *w, int *h, int fmt_ignored, int at_end, int* ret_size);
#endif


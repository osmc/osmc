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



#ifndef AMDRM_UTILS_H
#define AMDRM_UTILS_H
#include <stdint.h>
#ifdef  __cplusplus
extern "C" {
#endif

struct tvp_region
{
	uint64_t start;
	uint64_t end;
	int mem_flags;
};

#define TVP_MM_ENABLE_FLAGS_FOR_4K 0x02

extern int tvp_mm_enable(int flags);
extern int tvp_mm_disable(int flags);
extern int tvp_mm_get_mem_region(struct tvp_region* region, int region_size);
extern int tvp_mm_get_enable();
extern int free_cma_buffer(void);


#ifdef  __cplusplus
}
#endif

#endif


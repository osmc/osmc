/**
 *  Copyright (C) 2011-2012  Juho Vähä-Herttua
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 */

#ifndef RSAPEM_H
#define RSAPEM_H

typedef struct rsapem_s rsapem_t;

rsapem_t *rsapem_init(const char *pemstr);
int rsapem_read_vector(rsapem_t *rsapem, unsigned char **data);
void rsapem_destroy(rsapem_t *rsapem);

#endif

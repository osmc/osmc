

/*
    forklist.c: some functions which help record which forks were opened.

    Copyright (C) 2008 Alex deVries <alexthepuffin@gmail.com>

    This program can be distributed under the terms of the GNU GPL.
    See the file COPYING.
*/


#include "afp.h"

#include <stdlib.h>
#include <pthread.h>

void add_opened_fork(struct afp_volume * volume, struct afp_file_info * fp)
{

	pthread_mutex_lock(&volume->open_forks_mutex);

	fp->largelist_next=volume->open_forks;
	volume->open_forks=fp;

	pthread_mutex_unlock(&volume->open_forks_mutex);
}

void remove_opened_fork(struct afp_volume * volume, struct afp_file_info * fp)
{

	struct afp_file_info * p, * prev = NULL;

	pthread_mutex_lock(&volume->open_forks_mutex);

	for (p=volume->open_forks;p;p=p->largelist_next) 
	{
		if (p==fp) {
			if (prev) 
				prev->largelist_next=p->largelist_next;
			else
				volume->open_forks=p->largelist_next;
			goto done;
		}
		prev=p;
	}

done:
	pthread_mutex_unlock(&volume->open_forks_mutex);
}

void remove_fork_list(struct afp_volume * volume) 
{
	struct afp_file_info * p, * next;

	pthread_mutex_lock(&volume->open_forks_mutex);

	for (p=volume->open_forks;p;p=next) 
	{
		next=p->largelist_next;
		afp_flushfork(volume,p->forkid);
		afp_closefork(volume,p->forkid);

		volume->open_forks=p->largelist_next;
		free(p);
	}
	pthread_mutex_unlock(&volume->open_forks_mutex);
}


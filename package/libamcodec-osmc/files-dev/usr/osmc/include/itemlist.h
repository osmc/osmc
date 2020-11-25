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



#ifndef _PLAYER_ITEMLIST_H_
#define _PLAYER_ITEMLIST_H_

#include <list.h>
#include <pthread.h>


#define ITEMLIST_WITH_LOCK

struct item {
    struct list_head list;
    unsigned long item_data;
    unsigned long extdata[1];
    /*can be more space on alloc..*/
};

struct itemlist {
    struct list_head list;
#ifdef ITEMLIST_WITH_LOCK
    pthread_mutex_t list_mutex;
    int muti_threads_access;
#endif
    int item_count;
    int max_items;
    int item_ext_buf_size;
    int reject_same_item_data;
};
#ifdef ITEMLIST_WITH_LOCK
#define ITEM_LOCK(pitems)\
    do{if(pitems->muti_threads_access)\
        pthread_mutex_lock(&pitems->list_mutex);\
    }while(0);

#define ITEM_UNLOCK(pitems)\
    do{if(pitems->muti_threads_access)\
        pthread_mutex_unlock(&pitems->list_mutex);\
    }while(0);
#define ITEM_LOCK_INIT(pitems)\
    do{if(pitems->muti_threads_access)\
        pthread_mutex_init(&pitems->list_mutex,NULL);\
    }while(0);
#define ITEM_LOCK_DESTROY(pitems)\
        do{if(pitems->muti_threads_access)\
            pthread_mutex_destroy(&pitems->list_mutex);\
        }while(0);

#else
#define ITEM_LOCK(pitems)
#define ITEM_UNLOCK(pitems)
#define ITEM_LOCK_INIT(pitems)
#define ITEM_LOCK_DESTROY(pitems)

#endif


#define ITEM_EXT(item,n)    (((item)->extdata)[n])
#define ITEM_EXT64(item,n) (((int64_t*)((item)->extdata+1))[n])


#define NEXT_ITEM(item) (list_entry(item->list.next, struct item, list))
#define PREV_ITEM(item) list_entry(item->list.prev, struct item, list)

typedef int (*data_free_fun)(void *);
typedef int(*item_is_match_fun)(struct item *item, struct item *tomatchitem);
typedef int(*printitem_fun)(struct item *item);

int itemlist_init(struct itemlist *itemlist);
struct item * item_alloc(int ext);
void item_free(struct item *item);
int itemlist_deinit(struct itemlist *itemlist);

int  itemlist_del_item(struct itemlist *itemlist, struct item *item);
int  itemlist_del_item_locked(struct itemlist *itemlist, struct item *item);

int itemlist_add_tail(struct itemlist *itemlist, struct item *item);
struct item * itemlist_get_head(struct itemlist *itemlist);
struct item * itemlist_get_tail(struct itemlist *itemlist);
struct item * itemlist_peek_head(struct itemlist *itemlist);
struct item * itemlist_peek_tail(struct itemlist *itemlist);
struct item *  itemlist_get_match_item(struct itemlist *itemlist, unsigned long data);
struct item *  itemlist_find_match_item(struct itemlist *itemlist, unsigned long data);
int itemlist_del_match_data_item(struct itemlist *itemlist, unsigned long data);
int itemlist_have_match_data(struct itemlist *itemlist, unsigned long data);


int itemlist_clean(struct itemlist *itemlist, data_free_fun free_fun);
int itemlist_add_tail_data(struct itemlist *itemlist, unsigned long data);
int itemlist_add_tail_data_ext(struct itemlist *itemlist, unsigned long data, int extnum, unsigned long *extdata);

int itemlist_get_head_data(struct itemlist *itemlist, unsigned long *data);
int itemlist_get_tail_data(struct itemlist *itemlist, unsigned long *data);
int itemlist_peek_head_data(struct itemlist *itemlist, unsigned long *data);
int itemlist_peek_tail_data(struct itemlist *itemlist, unsigned long *data);
int itemlist_clean_data(struct itemlist *itemlist, data_free_fun free_fun);
struct item *  itemlist_find_match_item_ex(struct itemlist *itemlist, struct item *tomatch, item_is_match_fun match, int reveser);
int itemlist_item_insert(struct itemlist *itemlist, struct itemlist *position, struct itemlist *newitem, int flags);
int  itemlist_print(struct itemlist *itemlist, printitem_fun print);


#define FOR_EACH_ITEM_IN_ITEMLIST(__itemlist,__item)\
 ITEM_LOCK((__itemlist));{\
    struct list_head *llist, *tmplist;\
    list_for_each_safe(llist, tmplist, &(__itemlist)->list) \
    {\
       (__item) = list_entry(llist, struct item, list);\

#define FOR_ITEM_END(__itemlist)\
    }\
 }ITEM_UNLOCK((__itemlist))


#endif


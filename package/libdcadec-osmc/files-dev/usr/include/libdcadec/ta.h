/* Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef TA_H_
#define TA_H_

#include <stddef.h>
#include <stdbool.h>
#include <stdarg.h>

void *ta_alloc_size(void *ta_parent, size_t size);
void *ta_zalloc_size(void *ta_parent, size_t size);
void *ta_realloc_size(void *ta_parent, void *ptr, size_t size);
size_t ta_get_size(void *ptr);
void ta_free(void *ptr);
void ta_free_children(void *ptr);
bool ta_set_destructor(void *ptr, void (*destructor)(void *));
bool ta_set_parent(void *ptr, void *ta_parent);
void *ta_find_parent(void *ptr);
char *ta_strdup(void *ta_parent, const char *str);

static inline size_t ta_calc_array_size(size_t element_size, size_t count)
{
    if (count > (((size_t)-1) / element_size))
        return (size_t)-1;
    return element_size * count;
}

static inline int ta_zalloc_fast(void *ta_parent, void *ptr,
                                 size_t count, size_t element_size)
{
    void **_ptr = ptr;
    size_t old_size = ta_get_size(*_ptr);
    size_t new_size = ta_calc_array_size(element_size, count);

    if (old_size < new_size) {
        ta_free(*_ptr);
        if (!(*_ptr = ta_zalloc_size(ta_parent, new_size)))
            return -1;
        return 1;
    }

    return 0;
}

static inline int ta_alloc_fast(void *ta_parent, void *ptr,
                                size_t count, size_t element_size)
{
    void **_ptr = ptr;
    size_t old_size = ta_get_size(*_ptr);
    size_t new_size = ta_calc_array_size(element_size, count);

    if (old_size < new_size) {
        ta_free(*_ptr);
        if (!(*_ptr = ta_alloc_size(ta_parent, new_size)))
            return -1;
        return 1;
    }

    return 0;
}

#define ta_new(ta_parent, type)  (type *)ta_alloc_size(ta_parent, sizeof(type))
#define ta_znew(ta_parent, type) (type *)ta_zalloc_size(ta_parent, sizeof(type))

#define ta_new_array(ta_parent, type, count) \
    (type *)ta_alloc_size(ta_parent, ta_calc_array_size(sizeof(type), count))

#define ta_znew_array(ta_parent, type, count) \
    (type *)ta_zalloc_size(ta_parent, ta_calc_array_size(sizeof(type), count))

#define ta_new_array_size(ta_parent, element_size, count) \
    ta_alloc_size(ta_parent, ta_calc_array_size(element_size, count))

#define ta_znew_array_size(ta_parent, element_size, count) \
    ta_zalloc_size(ta_parent, ta_calc_array_size(element_size, count))

#define ta_realloc(ta_parent, ptr, type, count) \
    (type *)ta_realloc_size(ta_parent, ptr, ta_calc_array_size(sizeof(type), count))

#endif

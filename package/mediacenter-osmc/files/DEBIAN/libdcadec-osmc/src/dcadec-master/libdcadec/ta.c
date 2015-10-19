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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "compiler.h"
#include "ta.h"

// Note: the actual minimum alignment is dictated by malloc(). It doesn't
//       make sense to set this value higher than malloc's alignment.
#define MIN_ALIGN 16

struct ta_header {
    size_t size;                // size of the user allocation
    struct ta_header *prev;     // ring list containing siblings
    struct ta_header *next;
    struct ta_ext_header *ext;
};

union aligned_header {
    struct ta_header ta;
    // Make sure to satisfy typical alignment requirements
    void *align_ptr;
    int align_int;
    double align_d;
    long long align_ll;
    char align_min[(sizeof(struct ta_header) + MIN_ALIGN - 1) & ~(MIN_ALIGN - 1)];
};

#define PTR_TO_HEADER(ptr) (&((union aligned_header *)(ptr) - 1)->ta)
#define PTR_FROM_HEADER(h) ((void *)((union aligned_header *)(h) + 1))

#define MAX_ALLOC (((size_t)-1) - sizeof(union aligned_header))

// Needed for non-leaf allocations, or extended features such as destructors.
struct ta_ext_header {
    struct ta_header *header;  // points back to normal header
    struct ta_header children; // list of children, with this as sentinel
    void (*destructor)(void *);
};

// ta_ext_header.children.size is set to this
#define CHILDREN_SENTINEL ((size_t)-1)

static struct ta_header *get_header(void *ptr)
{
    return ptr ? PTR_TO_HEADER(ptr) : NULL;
}

static struct ta_ext_header *get_or_alloc_ext_header(void *ptr)
{
    struct ta_header *h = get_header(ptr);
    if (!h)
        return NULL;
    if (!h->ext) {
        h->ext = malloc(sizeof(struct ta_ext_header));
        if (!h->ext)
            return NULL;
        *h->ext = (struct ta_ext_header) {
            .header = h,
            .children = {
                .next = &h->ext->children,
                .prev = &h->ext->children,
                // Needed by ta_find_parent():
                .size = CHILDREN_SENTINEL,
                .ext = h->ext,
            },
        };
    }
    return h->ext;
}

/* Set the parent allocation of ptr. If parent==NULL, remove the parent.
 * Setting parent==NULL (with ptr!=NULL) always succeeds, and unsets the
 * parent of ptr. Operations ptr==NULL always succeed and do nothing.
 * Returns true on success, false on OOM.
 *
 * Warning: if ta_parent is a direct or indirect child of ptr, things will go
 *          wrong. The function will apparently succeed, but creates circular
 *          parent links, which are not allowed.
 */
bool ta_set_parent(void *ptr, void *ta_parent)
{
    struct ta_header *ch = get_header(ptr);
    if (!ch)
        return true;
    struct ta_ext_header *parent_eh = get_or_alloc_ext_header(ta_parent);
    if (ta_parent && !parent_eh) // do nothing on OOM
        return false;
    // Unlink from previous parent
    if (ch->next) {
        ch->next->prev = ch->prev;
        ch->prev->next = ch->next;
        ch->next = ch->prev = NULL;
    }
    // Link to new parent - insert at end of list (possibly orders destructors)
    if (parent_eh) {
        struct ta_header *children = &parent_eh->children;
        ch->next = children;
        ch->prev = children->prev;
        children->prev->next = ch;
        children->prev = ch;
    }
    return true;
}

/* Allocate size bytes of memory. If ta_parent is not NULL, this is used as
 * parent allocation (if ta_parent is freed, this allocation is automatically
 * freed as well). size==0 allocates a block of size 0 (i.e. returns non-NULL).
 * Returns NULL on OOM.
 */
void *ta_alloc_size(void *ta_parent, size_t size)
{
    if (size >= MAX_ALLOC)
        return NULL;
    struct ta_header *h = malloc(sizeof(union aligned_header) + size);
    if (!h)
        return NULL;
    *h = (struct ta_header) {.size = size};
    void *ptr = PTR_FROM_HEADER(h);
    if (!ta_set_parent(ptr, ta_parent)) {
        ta_free(ptr);
        return NULL;
    }
    return ptr;
}

/* Exactly the same as ta_alloc_size(), but the returned memory block is
 * initialized to 0.
 */
void *ta_zalloc_size(void *ta_parent, size_t size)
{
    if (size >= MAX_ALLOC)
        return NULL;
    struct ta_header *h = calloc(1, sizeof(union aligned_header) + size);
    if (!h)
        return NULL;
    *h = (struct ta_header) {.size = size};
    void *ptr = PTR_FROM_HEADER(h);
    if (!ta_set_parent(ptr, ta_parent)) {
        ta_free(ptr);
        return NULL;
    }
    return ptr;
}

/* Reallocate the allocation given by ptr and return a new pointer. Much like
 * realloc(), the returned pointer can be different, and on OOM, NULL is
 * returned.
 *
 * size==0 is equivalent to ta_free(ptr).
 * ptr==NULL is equivalent to ta_alloc_size(ta_parent, size).
 *
 * ta_parent is used only in the ptr==NULL case.
 *
 * Returns NULL if the operation failed.
 * NULL is also returned if size==0.
 */
void *ta_realloc_size(void *ta_parent, void *ptr, size_t size)
{
    if (size >= MAX_ALLOC)
        return NULL;
    if (!size) {
        ta_free(ptr);
        return NULL;
    }
    if (!ptr)
        return ta_alloc_size(ta_parent, size);
    struct ta_header *h = get_header(ptr);
    struct ta_header *old_h = h;
    if (h->size == size)
        return ptr;
    h = realloc(h, sizeof(union aligned_header) + size);
    if (!h)
        return NULL;
    h->size = size;
    if (h != old_h) {
        if (h->next) {
            // Relink siblings
            h->next->prev = h;
            h->prev->next = h;
        }
        if (h->ext) {
            // Relink children
            h->ext->header = h;
            h->ext->children.next->prev = &h->ext->children;
            h->ext->children.prev->next = &h->ext->children;
        }
    }
    return PTR_FROM_HEADER(h);
}

/* Return the allocated size of ptr. This returns the size parameter of the
 * most recent ta_alloc.../ta_realloc... call.
 * If ptr==NULL, return 0.
 */
size_t ta_get_size(void *ptr)
{
    struct ta_header *h = get_header(ptr);
    return h ? h->size : 0;
}

/* Free all allocations that (recursively) have ptr as parent allocation, but
 * do not free ptr itself.
 */
void ta_free_children(void *ptr)
{
    struct ta_header *h = get_header(ptr);
    struct ta_ext_header *eh = h ? h->ext : NULL;
    if (!eh)
        return;
    while (eh->children.next != &eh->children)
        ta_free(PTR_FROM_HEADER(eh->children.next));
}

/* Free the given allocation, and all of its direct and indirect children.
 */
void ta_free(void *ptr)
{
    struct ta_header *h = get_header(ptr);
    if (!h)
        return;
    if (h->ext && h->ext->destructor)
        h->ext->destructor(ptr);
    ta_free_children(ptr);
    if (h->next) {
        // Unlink from sibling list
        h->next->prev = h->prev;
        h->prev->next = h->next;
    }
    free(h->ext);
    free(h);
}

/* Set a destructor that is to be called when the given allocation is freed.
 * (Whether the allocation is directly freed with ta_free() or indirectly by
 * freeing its parent does not matter.) There is only one destructor. If an
 * destructor was already set, it's overwritten.
 *
 * The destructor will be called with ptr as argument. The destructor can do
 * almost anything, but it must not attempt to free or realloc ptr. The
 * destructor is run before the allocation's children are freed (also, before
 * their destructors are run).
 *
 * Returns false if ptr==NULL, or on OOM.
 */
bool ta_set_destructor(void *ptr, void (*destructor)(void *))
{
    struct ta_ext_header *eh = get_or_alloc_ext_header(ptr);
    if (!eh)
        return false;
    eh->destructor = destructor;
    return true;
}

/* Return the ptr's parent allocation, or NULL if there isn't any.
 *
 * Warning: this has O(N) runtime complexity with N sibling allocations!
 */
void *ta_find_parent(void *ptr)
{
    struct ta_header *h = get_header(ptr);
    if (!h || !h->next)
        return NULL;
    for (struct ta_header *cur = h->next; cur != h; cur = cur->next) {
        if (cur->size == CHILDREN_SENTINEL)
            return PTR_FROM_HEADER(cur->ext->header);
    }
    return NULL;
}

/* Return a copy of str.
 * Returns NULL on OOM.
 */
char *ta_strdup(void *ta_parent, const char *str)
{
    if (str) {
        size_t len = strlen(str) + 1;
        char *dup = ta_alloc_size(ta_parent, len);
        if (dup)
            memcpy(dup, str, len);
        return dup;
    }
    return NULL;
}

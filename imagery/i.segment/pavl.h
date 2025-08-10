/* Produced by texiweb from libavl.w. */

/* libavl - library for manipulation of binary trees.
   Copyright (C) 1998, 1999, 2000, 2001, 2002, 2004 Free Software
   Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301 USA.
 */

#ifndef PAVL_H
#define PAVL_H 1

#include <stddef.h>

/* Function types. */
typedef int pavl_comparison_func(const void *pavl_a, const void *pavl_b);
typedef void pavl_item_func(void *pavl_item);
typedef void *pavl_copy_func(void *pavl_item);

#ifndef LIBAVL_ALLOCATOR
#define LIBAVL_ALLOCATOR
/* Memory allocator. */
struct libavl_allocator {
    void *(*libavl_malloc)(size_t libavl_size);
    void (*libavl_free)(void *libavl_block);
};
#endif

/* Default memory allocator. */
extern struct libavl_allocator pavl_allocator_default;
void *pavl_malloc(size_t);
void pavl_free(void *);

/* Maximum PAVL height, unused. */
#ifndef PAVL_MAX_HEIGHT
#define PAVL_MAX_HEIGHT 32
#endif

/* Tree data structure. */
struct pavl_table {
    struct pavl_node *pavl_root;         /* Tree's root. */
    pavl_comparison_func *pavl_compare;  /* Comparison function. */
    struct libavl_allocator *pavl_alloc; /* Memory allocator. */
    size_t pavl_count;                   /* Number of items in tree. */
};

/* An PAVL tree node. */
struct pavl_node {
    struct pavl_node *pavl_link[2]; /* Subtrees. */
    struct pavl_node *pavl_parent;  /* Parent node. */
    void *pavl_data;                /* Pointer to data. */
    signed char pavl_balance;       /* Balance factor. */
};

/* PAVL traverser structure. */
struct pavl_traverser {
    struct pavl_table *pavl_table; /* Tree being traversed. */
    struct pavl_node *pavl_node;   /* Current node in tree. */
};

/* Table functions. */
struct pavl_table *pavl_create(pavl_comparison_func *,
                               struct libavl_allocator *);
struct pavl_table *pavl_copy(const struct pavl_table *, pavl_copy_func *,
                             pavl_item_func *, struct libavl_allocator *);
void pavl_destroy(struct pavl_table *, pavl_item_func *);
void **pavl_probe(struct pavl_table *, void *);
void *pavl_insert(struct pavl_table *, void *);
void *pavl_replace(struct pavl_table *, void *);
void *pavl_delete(struct pavl_table *, const void *);
void *pavl_find(const struct pavl_table *, const void *);
void pavl_assert_insert(struct pavl_table *, void *);
void *pavl_assert_delete(struct pavl_table *, void *);

#define pavl_count(table) ((size_t)(table)->pavl_count)

/* Table traverser functions. */
void pavl_t_init(struct pavl_traverser *, struct pavl_table *);
void *pavl_t_first(struct pavl_traverser *, struct pavl_table *);
void *pavl_t_last(struct pavl_traverser *, struct pavl_table *);
void *pavl_t_find(struct pavl_traverser *, struct pavl_table *, void *);
void *pavl_t_insert(struct pavl_traverser *, struct pavl_table *, void *);
void *pavl_t_copy(struct pavl_traverser *, const struct pavl_traverser *);
void *pavl_t_next(struct pavl_traverser *);
void *pavl_t_prev(struct pavl_traverser *);
void *pavl_t_cur(struct pavl_traverser *);
void *pavl_t_replace(struct pavl_traverser *, void *);

#endif /* pavl.h */

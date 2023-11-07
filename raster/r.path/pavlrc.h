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

#ifndef PAVLRC_H
#define PAVLRC_H 1

#include <stddef.h>

struct pavlrc {
    int row, col;
};

/* Function types. */
typedef void *pavlrc_copy_func(void *pavl_item);

#ifndef LIBAVL_ALLOCATOR
#define LIBAVL_ALLOCATOR
/* Memory allocator. */
struct libavl_allocator {
    void *(*libavl_malloc)(struct libavl_allocator *, size_t libavl_size);
    void (*libavl_free)(struct libavl_allocator *, void *libavl_block);
};
#endif

/* Default memory allocator. */
extern struct libavl_allocator pavlrc_allocator_default;
void *pavlrc_malloc(struct libavl_allocator *, size_t);
void pavlrc_free(struct libavl_allocator *, void *);

/* Maximum PAVL height, unused. */
#ifndef PAVL_MAX_HEIGHT
#define PAVL_MAX_HEIGHT 32
#endif

/* Tree data structure. */
struct pavlrc_table {
    struct pavlrc_node *pavl_root;       /* Tree's root. */
    struct libavl_allocator *pavl_alloc; /* Memory allocator. */
    size_t pavl_count;                   /* Number of items in tree. */
};

/* An PAVL tree node. */
struct pavlrc_node {
    struct pavlrc_node *pavl_link[2]; /* Subtrees. */
    struct pavlrc_node *pavl_parent;  /* Parent node. */
    struct pavlrc pavl_data;          /* data. */
    signed char pavl_balance;         /* Balance factor. */
};

/* PAVL traverser structure. */
struct pavlrc_traverser {
    struct pavlrc_table *pavl_table; /* Tree being traversed. */
    struct pavlrc_node *pavl_node;   /* Current node in tree. */
};

/* Table functions. */
struct pavlrc_table *pavlrc_create(struct libavl_allocator *);
struct pavlrc_table *pavlrc_copy(const struct pavlrc_table *,
                                 pavlrc_copy_func *, struct libavl_allocator *);
void pavlrc_destroy(struct pavlrc_table *);
struct pavlrc *pavlrc_probe(struct pavlrc_table *, struct pavlrc *);
struct pavlrc *pavlrc_insert(struct pavlrc_table *, struct pavlrc *);
struct pavlrc *pavlrc_replace(struct pavlrc_table *, struct pavlrc *);
struct pavlrc *pavlrc_delete(struct pavlrc_table *, struct pavlrc *);
struct pavlrc *pavlrc_find(const struct pavlrc_table *, const struct pavlrc *);
void pavlrc_assert_insert(struct pavlrc_table *, struct pavlrc *);
struct pavlrc *pavlrc_assert_delete(struct pavlrc_table *, struct pavlrc *);

#define pavlrc_count(table) ((size_t)(table)->pavlrc_count)

/* Table traverser functions. */
void pavlrc_t_init(struct pavlrc_traverser *, struct pavlrc_table *);
struct pavlrc *pavlrc_t_first(struct pavlrc_traverser *, struct pavlrc_table *);
struct pavlrc *pavlrc_t_last(struct pavlrc_traverser *, struct pavlrc_table *);
struct pavlrc *pavlrc_t_find(struct pavlrc_traverser *, struct pavlrc_table *,
                             struct pavlrc *);
struct pavlrc *pavlrc_t_insert(struct pavlrc_traverser *, struct pavlrc_table *,
                               struct pavlrc *);
struct pavlrc *pavlrc_t_copy(struct pavlrc_traverser *,
                             const struct pavlrc_traverser *);
struct pavlrc *pavlrc_t_next(struct pavlrc_traverser *);
struct pavlrc *pavlrc_t_prev(struct pavlrc_traverser *);
struct pavlrc *pavlrc_t_cur(struct pavlrc_traverser *);
struct pavlrc *pavlrc_t_replace(struct pavlrc_traverser *, struct pavlrc *);

#endif /* pavlrc.h */

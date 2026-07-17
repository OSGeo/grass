/****************************************************************
 *
 * MODULE:     v.generalize
 *
 * AUTHOR(S):  Daniel Bundala
 *
 * PURPOSE:    priority queue / binary max heap
 *
 *
 * SPDX-FileCopyrightText: 2002-2005 Other GRASS authors
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 ****************************************************************/

#ifndef PQ_H
#define PQ_H

#include <grass/vector.h>

/* None of the functions below tests overflows,
 * only extract_max treats underflow */

typedef struct {
    int items;
    double *key;
    int *value;
} binary_heap;

/* initializes new empty binary heap. Returns 1
 * on success, 0 otherwise */
int binary_heap_init(int size, binary_heap *bh);

/* frees the memory occupied by a heap */
void binary_heap_free(binary_heap *bh);

/* this function pushes (key, value) to the heap */
void binary_heap_push(double key, int value, binary_heap *bh);

/* passes the key of the element with the highest key and
 * deletes this key. Returns 1 on success, 0 on empty heap */
int binary_heap_extract_max(binary_heap *bh, int *value);

#endif

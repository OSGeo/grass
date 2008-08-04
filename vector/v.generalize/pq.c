
/****************************************************************
 *
 * MODULE:     v.generalize
 *
 * AUTHOR(S):  Daniel Bundala
 *
 * PURPOSE:    priority queue / binary heap
 *             
 *
 * COPYRIGHT:  (C) 2002-2005 by the GRASS Development Team
 *
 *             This program is free software under the
 *             GNU General Public License (>=v2).
 *             Read the file COPYING that comes with GRASS
 *             for details.
 *
 ****************************************************************/

#include "pq.h"

int binary_heap_init(int size, binary_heap * bh)
{
    bh->items = 0;
    bh->key = (double *)G_malloc(sizeof(double) * (size + 1));
    if (bh->key == NULL)
	return 0;
    bh->value = (int *)G_malloc(sizeof(int) * (size + 1));
    if (bh->value == NULL) {
	G_free(bh->key);
	return 0;
    }
    return 1;
}

void binary_heap_free(binary_heap * bh)
{
    G_free(bh->key);
    G_free(bh->value);
    return;
}

void binary_heap_push(double key, int value, binary_heap * bh)
{
    int i = ++(bh->items);

    while (i != 1 && key > bh->key[i / 2]) {
	bh->key[i] = bh->key[i / 2];
	bh->value[i] = bh->value[i / 2];
	i /= 2;
    }

    bh->key[i] = key;
    bh->value[i] = value;
    return;
}

int binary_heap_extract_max(binary_heap * bh, int *value)
{
    int n = bh->items;

    if (n == 0)
	return 0;
    *value = bh->value[1];


    bh->key[1] = bh->key[n];
    bh->value[1] = bh->value[n];

    int i = 1;
    double td;
    int tv;


    while (1) {
	int greater = i;
	int left = 2 * i;
	int right = 2 * i + 1;

	if (left < n && bh->key[left] > bh->key[i])
	    greater = left;
	if (right < n && bh->key[right] > bh->key[greater])
	    greater = right;

	if (greater == i)
	    break;

	td = bh->key[i];
	bh->key[i] = bh->key[greater];
	bh->key[greater] = td;

	tv = bh->value[i];
	bh->value[i] = bh->value[greater];
	bh->value[greater] = tv;

	i = greater;
    }

    bh->items--;
    return 1;
}

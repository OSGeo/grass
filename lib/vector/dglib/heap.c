/* LIBDGL -- a Directed Graph Library implementation
 * Copyright (C) 2002 Roberto Micarelli
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/* best view tabstop=4
 */
#include <stdio.h>
#include <stdlib.h>

#include "type.h"
#include "heap.h"



void dglHeapInit(dglHeap_s * pheap)
{
    pheap->index = 0;
    pheap->count = 0;
    pheap->block = 256;
    pheap->pnode = NULL;
}

void dglHeapFree(dglHeap_s * pheap, dglHeapCancelItem_fn pfnCancelItem)
{
    int iItem;

    if (pheap->pnode) {
	if (pfnCancelItem) {
	    for (iItem = 0; iItem <= pheap->index; iItem++) {
		pfnCancelItem(pheap, &pheap->pnode[iItem]);
	    }
	}
	free(pheap->pnode);
    }
    pheap->pnode = NULL;
}

int dglHeapInsertMin(dglHeap_s * pheap,
		     long key, unsigned char flags, dglHeapData_u value)
{
    long i;

    if (pheap->index >= pheap->count - 1) {
	pheap->count += pheap->block;
	if ((pheap->pnode =
	     realloc(pheap->pnode,
		     sizeof(dglHeapNode_s) * pheap->count)) == NULL)
	    return -1;
    }

    i = ++pheap->index;

    while (i != 1 && key < pheap->pnode[i / 2].key) {
	pheap->pnode[i] = pheap->pnode[i / 2];
	i /= 2;
    }

    pheap->pnode[i].key = key;
    pheap->pnode[i].flags = flags;
    pheap->pnode[i].value = value;

    return i;
}

int dglHeapExtractMin(dglHeap_s * pheap, dglHeapNode_s * pnoderet)
{
    dglHeapNode_s temp;
    long iparent, ichild;

    if (pheap->index == 0)
	return 0;		/* empty heap */

    *pnoderet = pheap->pnode[1];

    temp = pheap->pnode[pheap->index--];

    iparent = 1;
    ichild = 2;

    while (ichild <= pheap->index) {
	if (ichild < pheap->index &&
	    pheap->pnode[ichild].key > pheap->pnode[ichild + 1].key) {
	    ichild++;
	}
	if (temp.key <= pheap->pnode[ichild].key)
	    break;

	pheap->pnode[iparent] = pheap->pnode[ichild];
	iparent = ichild;
	ichild *= 2;
    }
    pheap->pnode[iparent] = temp;

    return 1;
}

int dglHeapInsertMax(dglHeap_s * pheap,
		     long key, unsigned char flags, dglHeapData_u value)
{
    long i;

    if (pheap->index >= pheap->count - 1) {
	pheap->count += pheap->block;
	if ((pheap->pnode =
	     realloc(pheap->pnode,
		     sizeof(dglHeapNode_s) * pheap->count)) == NULL)
	    return -1;
    }

    i = ++pheap->index;

    while (i != 1 && key > pheap->pnode[i / 2].key) {
	pheap->pnode[i] = pheap->pnode[i / 2];
	i /= 2;
    }

    pheap->pnode[i].key = key;
    pheap->pnode[i].flags = flags;
    pheap->pnode[i].value = value;

    return i;
}

int dglHeapExtractMax(dglHeap_s * pheap, dglHeapNode_s * pnoderet)
{
    dglHeapNode_s temp;
    long iparent, ichild;

    if (pheap->index == 0)
	return 0;		/* empty heap */

    *pnoderet = pheap->pnode[1];

    temp = pheap->pnode[pheap->index--];

    iparent = 1;
    ichild = 2;

    while (ichild <= pheap->index) {
	if (ichild < pheap->index &&
	    pheap->pnode[ichild].key < pheap->pnode[ichild + 1].key) {
	    ichild++;
	}
	if (temp.key >= pheap->pnode[ichild].key)
	    break;

	pheap->pnode[iparent] = pheap->pnode[ichild];
	iparent = ichild;
	ichild *= 2;
    }
    pheap->pnode[iparent] = temp;

    return 1;
}

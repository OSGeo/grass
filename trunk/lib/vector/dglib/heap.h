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

#ifndef _DGL_HEAP_H_
#define _DGL_HEAP_H_

typedef union _dglHeapData
{
    void *pv;
    int n;
    unsigned int un;
    long l;
    unsigned long ul;

} dglHeapData_u;


typedef struct _dglHeapNode
{
    long key;
    dglHeapData_u value;
    unsigned char flags;

} dglHeapNode_s;

typedef struct _dglHeap
{

    long index;			/* last node / number of current nodes (complete-binary-tree array representation ...) */
    long count;			/* number of allocated nodes in ->pnode array */
    long block;			/* allocation block size expressed in number of nodes */
    dglHeapNode_s *pnode;	/* the node-array */

} dglHeap_s;

extern void dglHeapInit(dglHeap_s * pheap);


typedef void (*dglHeapCancelItem_fn) (dglHeap_s * pheap,
				      dglHeapNode_s * pitem);
extern void dglHeapFree(dglHeap_s * pheap,
			dglHeapCancelItem_fn pfnCancelItem);

extern int dglHeapInsertMax(dglHeap_s * pheap,
			    long key,
			    unsigned char flags, dglHeapData_u value);

extern int dglHeapExtractMax(dglHeap_s * pheap, dglHeapNode_s * pnoderet);

extern int dglHeapInsertMin(dglHeap_s * pheap,
			    long key,
			    unsigned char flags, dglHeapData_u value);

extern int dglHeapExtractMin(dglHeap_s * pheap, dglHeapNode_s * pnoderet);

#endif

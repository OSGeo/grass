/* LIBDGL -- a Directed Graph Library implementation
 * SPDX-FileCopyrightText: 2002 Roberto Micarelli
 * SPDX-FileCopyrightText: Other GRASS authors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/* best view tabstop=4
 */

#ifndef _DGL_HEAP_H_
#define _DGL_HEAP_H_

typedef union _dglHeapData {
    void *pv;
    int n;
    unsigned int un;
    long l;
    unsigned long ul;

} dglHeapData_u;

typedef struct _dglHeapNode {
    long key;
    dglHeapData_u value;
    unsigned char flags;

} dglHeapNode_s;

typedef struct _dglHeap {

    long index; /* last node / number of current nodes (complete-binary-tree
                   array representation ...) */
    long count; /* number of allocated nodes in ->pnode array */
    long block; /* allocation block size expressed in number of nodes */
    dglHeapNode_s *pnode; /* the node-array */

} dglHeap_s;

extern void dglHeapInit(dglHeap_s *pheap);

typedef void (*dglHeapCancelItem_fn)(dglHeap_s *pheap, dglHeapNode_s *pitem);
extern void dglHeapFree(dglHeap_s *pheap, dglHeapCancelItem_fn pfnCancelItem);

extern int dglHeapInsertMax(dglHeap_s *pheap, long key, unsigned char flags,
                            dglHeapData_u value);

extern int dglHeapExtractMax(dglHeap_s *pheap, dglHeapNode_s *pnoderet);

extern int dglHeapInsertMin(dglHeap_s *pheap, long key, unsigned char flags,
                            dglHeapData_u value);

extern int dglHeapExtractMin(dglHeap_s *pheap, dglHeapNode_s *pnoderet);

#endif

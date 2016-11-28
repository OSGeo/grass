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

#ifndef _DGL_TREE_H_
#define _DGL_TREE_H_

#define USE_THREADED_AVL

#if defined(USE_THREADED_AVL)
#include "tavl.h"
#define avl_table tavl_table
#define avl_traverser tavl_traverser
#define avl_create tavl_create
#define avl_copy tavl_copy
#define avl_destroy tavl_destroy
#define avl_probe tavl_probe
#define avl_insert tavl_insert
#define avl_replace tavl_replace
#define avl_delete tavl_delete
#define avl_find tavl_find
#define avl_assert_insert tavl_assert_insert
#define avl_assert_delete tavl_assert_delete
#define avl_t_init tavl_t_init
#define avl_t_first tavl_t_first
#define avl_t_last tavl_t_last
#define avl_t_find tavl_t_find
#define avl_t_insert tavl_t_insert
#define avl_t_copy tavl_t_copy
#define avl_t_next tavl_t_next
#define avl_t_prev tavl_t_prev
#define avl_t_cur tavl_t_cur
#define avl_t_replace tavl_t_replace
#else
#include "avl.h"
#endif


extern void *dglTreeGetAllocator();

/*
 * Define a node as it is hosted in pNodeTree
 */
typedef struct _dglTreeNode
{
    long nKey;
    void *pv;
    void *pv2;
} dglTreeNode_s;
extern dglTreeNode_s *dglTreeNodeAlloc();
extern void dglTreeNodeCancel(void *pvNode, void *pvParam);
extern int dglTreeNodeCompare(const void *pvNodeA, const void *pvNodeB,
			      void *pvParam);
extern dglTreeNode_s *dglTreeNodeAdd(void *pvAVL, dglInt32_t nKey);


/*
 * Define a version-2 node as it is hosted in pNodeTree
 */
typedef struct _dglTreeNode2
{
    long nKey;
    void *pv;
    void *pv2;
    void *pv3;
} dglTreeNode2_s;
extern dglTreeNode2_s *dglTreeNode2Alloc();
extern void dglTreeNode2Cancel(void *pvNode, void *pvParam);
extern int dglTreeNode2Compare(const void *pvNodeA, const void *pvNodeB,
			       void *pvParam);
extern dglTreeNode2_s *dglTreeNode2Add(void *pvAVL, dglInt32_t nKey);


/*
 * Define a edge as it is hosted in pEdgeTree
 */
typedef struct _dglTreeEdge
{
    dglInt32_t nKey;
    void *pv;
} dglTreeEdge_s;
extern dglTreeEdge_s *dglTreeEdgeAlloc();
extern void dglTreeEdgeCancel(void *pvEdge, void *pvParam);
extern int dglTreeEdgeCompare(const void *pvEdgeA, const void *pvEdgeB,
			      void *pvParam);
extern dglTreeEdge_s *dglTreeEdgeAdd(void *pvAVL, dglInt32_t nKey);


/*
 * Define a dummy entry to 'touch' selected item with a dglInt32_t key
 * i.e. used to mark visited nodes in a greedy or tree-growing algorithm
 */
typedef struct _dglTreeTouchI32
{
    dglInt32_t nKey;
} dglTreeTouchI32_s;
extern dglTreeTouchI32_s *dglTreeTouchI32Alloc();
extern void dglTreeTouchI32Cancel(void *pvTouchI32, void *pvParam);
extern int dglTreeTouchI32Compare(const void *pvTouchI32A,
				  const void *pvTouchI32B, void *pvParam);
extern dglTreeTouchI32_s *dglTreeTouchI32Add(void *pvAVL, dglInt32_t nKey);


/*
 * Define a entry to maintain a predecessor/distance network in shortest-path computation
 */
typedef struct _dglTreePredist
{
    dglInt32_t nKey;
    dglInt32_t nFrom;
    dglInt32_t nDistance;
    dglInt32_t nCost;
    dglInt32_t *pnEdge;
    dglByte_t bFlags;
} dglTreePredist_s;
extern dglTreePredist_s *dglTreePredistAlloc();
extern void dglTreePredistCancel(void *pvPredist, void *pvParam);
extern int dglTreePredistCompare(const void *pvPredistA,
				 const void *pvPredistB, void *pvParam);
extern dglTreePredist_s *dglTreePredistAdd(void *pvAVL, dglInt32_t nKey);


/*
 * 32bit-key Node Prioritizer
 */
typedef struct _dglTreeNodePri32
{
    dglInt32_t nKey;
    dglInt32_t cnVal;
    dglInt32_t *pnVal;
} dglTreeNodePri32_s;
extern dglTreeNodePri32_s *dglTreeNodePri32Alloc();
extern void dglTreeNodePri32Cancel(void *pvNodePri32, void *pvParam);
extern int dglTreeNodePri32Compare(const void *pvNodePri32A,
				   const void *pvNodePri32B, void *pvParam);
extern dglTreeNodePri32_s *dglTreeNodePri32Add(void *pvAVL, dglInt32_t nKey);


/*
 * 32bit-key Edge Prioritizer
 */
typedef struct _dglTreeEdgePri32
{
    dglInt32_t nKey;
    dglInt32_t cnData;
    dglInt32_t *pnData;
} dglTreeEdgePri32_s;
extern dglTreeEdgePri32_s *dglTreeEdgePri32Alloc();
extern void dglTreeEdgePri32Cancel(void *pvEdgePri32, void *pvParam);
extern int dglTreeEdgePri32Compare(const void *pvEdgePri32A,
				   const void *pvEdgePri32B, void *pvParam);
extern dglTreeEdgePri32_s *dglTreeEdgePri32Add(void *pvAVL, dglInt32_t nKey);


#endif

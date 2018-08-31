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
#include <string.h>
#include <stdlib.h>

#include "type.h"
#include "tree.h"

/*
 * AVL Support for data type dglTreeNode_s
 * alloc
 * cancel
 * compare
 * add
 */
dglTreeNode_s *dglTreeNodeAlloc()
{
    dglTreeNode_s *pNode = (dglTreeNode_s *) malloc(sizeof(dglTreeNode_s));

    if (pNode)
	memset(pNode, 0, sizeof(dglTreeNode_s));
    return pNode;
}

void dglTreeNodeCancel(void *pvNode, void *pvParam)
{
    if (((dglTreeNode_s *) pvNode)->pv)
	free(((dglTreeNode_s *) pvNode)->pv);
    if (((dglTreeNode_s *) pvNode)->pv2)
	free(((dglTreeNode_s *) pvNode)->pv2);
    free(pvNode);
}

int dglTreeNodeCompare(const void *pvNodeA, const void *pvNodeB,
		       void *pvParam)
{
    if (((dglTreeNode_s *) pvNodeA)->nKey < ((dglTreeNode_s *) pvNodeB)->nKey)
	return -1;
    else if (((dglTreeNode_s *) pvNodeA)->nKey >
	     ((dglTreeNode_s *) pvNodeB)->nKey)
	return 1;
    else
	return 0;
}

dglTreeNode_s *dglTreeNodeAdd(void *pavl, dglInt32_t nKey)
{
    dglTreeNode_s *pnode;
    void **ppvret;

    if ((pnode = dglTreeNodeAlloc()) == NULL)
	return NULL;
    pnode->nKey = nKey;
    ppvret = avl_probe(pavl, pnode);
    if (*ppvret != pnode) {
	free(pnode);
	pnode = *ppvret;
    }
    return pnode;
}


/*
 * AVL Support for data type dglTreeNode2_s
 * alloc
 * cancel
 * compare
 * add
 */
dglTreeNode2_s *dglTreeNode2Alloc()
{
    dglTreeNode2_s *pNode2 =
	(dglTreeNode2_s *) malloc(sizeof(dglTreeNode2_s));
    if (pNode2)
	memset(pNode2, 0, sizeof(dglTreeNode2_s));
    return pNode2;
}

void dglTreeNode2Cancel(void *pvNode2, void *pvParam)
{
    if (((dglTreeNode2_s *) pvNode2)->pv)
	free(((dglTreeNode2_s *) pvNode2)->pv);
    if (((dglTreeNode2_s *) pvNode2)->pv2)
	free(((dglTreeNode2_s *) pvNode2)->pv2);
    if (((dglTreeNode2_s *) pvNode2)->pv3)
	free(((dglTreeNode2_s *) pvNode2)->pv3);
    free(pvNode2);
}

int dglTreeNode2Compare(const void *pvNode2A, const void *pvNode2B,
			void *pvParam)
{
    if (((dglTreeNode2_s *) pvNode2A)->nKey <
	((dglTreeNode2_s *) pvNode2B)->nKey)
	return -1;
    else if (((dglTreeNode2_s *) pvNode2A)->nKey >
	     ((dglTreeNode2_s *) pvNode2B)->nKey)
	return 1;
    else
	return 0;
}

dglTreeNode2_s *dglTreeNode2Add(void *pavl, dglInt32_t nKey)
{
    dglTreeNode2_s *pnode;
    void **ppvret;

    if ((pnode = dglTreeNode2Alloc()) == NULL)
	return NULL;
    pnode->nKey = nKey;
    ppvret = avl_probe(pavl, pnode);
    if (*ppvret != pnode) {
	free(pnode);
	pnode = *ppvret;
    }
    return pnode;
}


/*
 * AVL Support for data type dglTreeEdge_s
 * alloc
 * cancel
 * compare
 * add
 */
dglTreeEdge_s *dglTreeEdgeAlloc()
{
    dglTreeEdge_s *pEdge = (dglTreeEdge_s *) malloc(sizeof(dglTreeEdge_s));

    if (pEdge)
	memset(pEdge, 0, sizeof(dglTreeEdge_s));
    return pEdge;
}

void dglTreeEdgeCancel(void *pvEdge, void *pvParam)
{
    if (((dglTreeEdge_s *) pvEdge)->pv)
	free(((dglTreeEdge_s *) pvEdge)->pv);
    free(pvEdge);
}

int dglTreeEdgeCompare(const void *pvEdgeA, const void *pvEdgeB,
		       void *pvParam)
{
    if (((dglTreeEdge_s *) pvEdgeA)->nKey < ((dglTreeEdge_s *) pvEdgeB)->nKey)
	return -1;
    else if (((dglTreeEdge_s *) pvEdgeA)->nKey >
	     ((dglTreeEdge_s *) pvEdgeB)->nKey)
	return 1;
    else
	return 0;
}

dglTreeEdge_s *dglTreeEdgeAdd(void *pavl, dglInt32_t nKey)
{
    dglTreeEdge_s *pedge;
    void **ppvret;

    if ((pedge = dglTreeEdgeAlloc()) == NULL)
	return NULL;
    pedge->nKey = nKey;
    ppvret = avl_probe(pavl, pedge);
    if (*ppvret != pedge) {
	free(pedge);
	pedge = *ppvret;
    }
    return pedge;
}



/*
 * AVL Support for data type dglTreeTouchI32_s
 * alloc
 * cancel
 * compare
 * add
 */
dglTreeTouchI32_s *dglTreeTouchI32Alloc()
{
    dglTreeTouchI32_s *pTouchI32 =
	(dglTreeTouchI32_s *) malloc(sizeof(dglTreeTouchI32_s));
    pTouchI32->nKey = 0;
    return pTouchI32;
}

void dglTreeTouchI32Cancel(void *pvTouchI32, void *pvParam)
{
    free(pvTouchI32);
}

int dglTreeTouchI32Compare(const void *pvTouchI32A, const void *pvTouchI32B,
			   void *pvParam)
{
    if (((dglTreeTouchI32_s *) pvTouchI32A)->nKey <
	((dglTreeTouchI32_s *) pvTouchI32B)->nKey)
	return -1;
    else if (((dglTreeTouchI32_s *) pvTouchI32A)->nKey >
	     ((dglTreeTouchI32_s *) pvTouchI32B)->nKey)
	return 1;
    else
	return 0;
}

dglTreeTouchI32_s *dglTreeTouchI32Add(void *pavl, dglInt32_t nKey)
{
    dglTreeTouchI32_s *pnode;
    void **ppvret;

    if ((pnode = dglTreeTouchI32Alloc()) == NULL)
	return NULL;
    pnode->nKey = nKey;
    ppvret = avl_probe(pavl, pnode);
    if (*ppvret != pnode) {
	free(pnode);
	pnode = *ppvret;
    }
    return pnode;
}



/*
 * AVL Support for data type dglTreePredist_s
 * alloc
 * cancel
 * compare
 * add
 */
dglTreePredist_s *dglTreePredistAlloc()
{
    dglTreePredist_s *pPredist =
	(dglTreePredist_s *) malloc(sizeof(dglTreePredist_s));
    if (pPredist)
	memset(pPredist, 0, sizeof(dglTreePredist_s));
    return pPredist;
}

void dglTreePredistCancel(void *pvPredist, void *pvParam)
{
    free(pvPredist);
}

int dglTreePredistCompare(const void *pvPredistA, const void *pvPredistB,
			  void *pvParam)
{
    if (((dglTreePredist_s *) pvPredistA)->nKey <
	((dglTreePredist_s *) pvPredistB)->nKey)
	return -1;
    else if (((dglTreePredist_s *) pvPredistA)->nKey >
	     ((dglTreePredist_s *) pvPredistB)->nKey)
	return 1;
    else
	return 0;
}

dglTreePredist_s *dglTreePredistAdd(void *pavl, dglInt32_t nKey)
{
    dglTreePredist_s *pnode;
    void **ppvret;

    if ((pnode = dglTreePredistAlloc()) == NULL)
	return NULL;
    pnode->nKey = nKey;
    ppvret = avl_probe(pavl, pnode);
    if (*ppvret != pnode) {
	free(pnode);
	pnode = *ppvret;
    }
    return pnode;
}




/*
 * AVL Support for data type dglTreeNodePri32_s
 * alloc
 * cancel
 * compare
 * add
 */
dglTreeNodePri32_s *dglTreeNodePri32Alloc()
{
    dglTreeNodePri32_s *pNodePri32 =
	(dglTreeNodePri32_s *) malloc(sizeof(dglTreeNodePri32_s));
    if (pNodePri32)
	memset(pNodePri32, 0, sizeof(dglTreeNodePri32_s));
    return pNodePri32;
}

void dglTreeNodePri32Cancel(void *pvNodePri32, void *pvParam)
{
    free(pvNodePri32);
}

int dglTreeNodePri32Compare(const void *pvNodePri32A,
			    const void *pvNodePri32B, void *pvParam)
{
    if (((dglTreeNodePri32_s *) pvNodePri32A)->nKey <
	((dglTreeNodePri32_s *) pvNodePri32B)->nKey)
	return -1;
    else if (((dglTreeNodePri32_s *) pvNodePri32A)->nKey >
	     ((dglTreeNodePri32_s *) pvNodePri32B)->nKey)
	return 1;
    else
	return 0;
}

dglTreeNodePri32_s *dglTreeNodePri32Add(void *pavl, dglInt32_t nKey)
{
    dglTreeNodePri32_s *pnode;
    void **ppvret;

    if ((pnode = dglTreeNodePri32Alloc()) == NULL)
	return NULL;
    pnode->nKey = nKey;
    ppvret = avl_probe(pavl, pnode);
    if (*ppvret != pnode) {
	free(pnode);
	pnode = *ppvret;
    }
    return pnode;
}



/*
 * AVL Support for data type dglTreeEdgePri32_s
 * alloc
 * cancel
 * compare
 * add
 */
dglTreeEdgePri32_s *dglTreeEdgePri32Alloc()
{
    dglTreeEdgePri32_s *pEdgePri32 =
	(dglTreeEdgePri32_s *) malloc(sizeof(dglTreeEdgePri32_s));
    if (pEdgePri32)
	memset(pEdgePri32, 0, sizeof(dglTreeEdgePri32_s));
    return pEdgePri32;
}

void dglTreeEdgePri32Cancel(void *pvEdgePri32, void *pvParam)
{
    if (((dglTreeEdgePri32_s *) pvEdgePri32)->pnData) {
	free(((dglTreeEdgePri32_s *) pvEdgePri32)->pnData);
    }
    free(pvEdgePri32);
}

int dglTreeEdgePri32Compare(const void *pvEdgePri32A,
			    const void *pvEdgePri32B, void *pvParam)
{
    if (((dglTreeEdgePri32_s *) pvEdgePri32A)->nKey <
	((dglTreeEdgePri32_s *) pvEdgePri32B)->nKey)
	return -1;
    else if (((dglTreeEdgePri32_s *) pvEdgePri32A)->nKey >
	     ((dglTreeEdgePri32_s *) pvEdgePri32B)->nKey)
	return 1;
    else
	return 0;
}

dglTreeEdgePri32_s *dglTreeEdgePri32Add(void *pavl, dglInt32_t nKey)
{
    dglTreeEdgePri32_s *pnode;
    void **ppvret;

    if ((pnode = dglTreeEdgePri32Alloc()) == NULL)
	return NULL;
    pnode->nKey = nKey;
    ppvret = avl_probe(pavl, pnode);
    if (*ppvret != pnode) {
	free(pnode);
	pnode = *ppvret;
    }
    return pnode;
}




/*
 * Our AVL allocator
 */
static void *_tree_malloc(struct libavl_allocator *allocator,
			  size_t libavl_size)
{
    return malloc(libavl_size);
}

static void _tree_free(struct libavl_allocator *allocator, void *libavl_block)
{
    free(libavl_block);
}

static struct libavl_allocator _tree_allocator = {
    _tree_malloc, _tree_free
};

void *dglTreeGetAllocator()
{
    return &_tree_allocator;
}

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

/*
 * best view with tabstop=4
 */

/*
 * Build the depth-first spanning tree of 'pgraphIn' into 'pgraphOut'
 * - pgraphOut must have been previously initialized by the caller and is returned in TREE state
 * - I prefer using a iterative approach with a stack for 'waiting edges' instead of recursion: it's
 *   cheaper to stack 8 bytes for each edge than the whole function stack
 * - The visited network is passed by the caller because this function can be used for two purposes:
 *   1. generate a single spanning tree (dglDepthSpanning)
 *   2. part of a loop for generating connected-components of the graph (dglDepthComponents)
 */
int DGL_SPAN_DEPTHFIRST_SPANNING_FUNC(dglGraph_s * pgraphIn,
				      dglGraph_s * pgraphOut,
				      dglInt32_t nVertex,
				      void *pvVisited,
				      dglSpanClip_fn fnClip, void *pvClipArg)
{
    struct _stackItem
    {
	dglInt32_t *pnHead;
	dglInt32_t *pnEdge;
	int iWay;
    };

    struct _stackItem stackItem;
    struct _stackItem *pStackItem;

    dglInt32_t *pHead;
    dglInt32_t *pTail;
    dglInt32_t *pEdgeset;
    dglInt32_t *pEdge;
    long istack = 0;
    unsigned char *pstack = NULL;
    int nret;
    dglSpanClipInput_s clipInput;
    dglTreeNode_s findVisited;
    dglEdgesetTraverser_s laT;


    if ((pHead = dglGetNode(pgraphIn, nVertex)) == NULL) {
	pgraphIn->iErrno = DGL_ERR_HeadNodeNotFound;
	goto dfs_error;
    }

    /*
     * the simplest case is when vertex node is alone or has no outgoing edges, the result
     * of the spanning is a graph having only one node
     */
    if (DGL_NODE_STATUS(pHead) & DGL_NS_ALONE ||
	(!(DGL_NODE_STATUS(pHead) & DGL_NS_HEAD) &&
	 DGL_NODE_STATUS(pHead) & DGL_NS_TAIL)) {
	nret =
	    DGL_ADD_NODE_FUNC(pgraphOut, DGL_NODE_ID(pHead),
			      DGL_NODE_ATTR_PTR(pHead), 0);
	if (nret < 0) {
	    goto dfs_error;
	}
	return 0;
    }

    if ((DGL_NODE_STATUS(pHead) & DGL_NS_HEAD) || pgraphIn->Version == 3) {

	pEdgeset = _DGL_OUTEDGESET(pgraphIn, pHead);

	if (DGL_EDGESET_T_INITIALIZE_FUNC(pgraphIn, &laT, pEdgeset) < 0) {
	    goto dfs_error;
	}
	for (pEdge = DGL_EDGESET_T_FIRST_FUNC(&laT);
	     pEdge; pEdge = DGL_EDGESET_T_NEXT_FUNC(&laT)
	    ) {
	    stackItem.pnHead = pHead;
	    stackItem.pnEdge = pEdge;
	    stackItem.iWay = 0;
	    if ((pstack =
		 dgl_mempush(pstack, &istack, sizeof(stackItem),
			     &stackItem)) == NULL) {
		pgraphIn->iErrno = DGL_ERR_MemoryExhausted;
		goto dfs_error;
	    }
	}
	DGL_EDGESET_T_RELEASE_FUNC(&laT);

	if (pgraphIn->Version == 3) {
	    pEdgeset = _DGL_INEDGESET(pgraphIn, pHead);

	    if (DGL_EDGESET_T_INITIALIZE_FUNC(pgraphIn, &laT, pEdgeset) < 0) {
		goto dfs_error;
	    }
	    for (pEdge = DGL_EDGESET_T_FIRST_FUNC(&laT);
		 pEdge; pEdge = DGL_EDGESET_T_NEXT_FUNC(&laT)
		) {
		if (DGL_EDGE_STATUS(pEdge) & DGL_ES_DIRECTED)
		    continue;
		stackItem.pnHead = pHead;
		stackItem.pnEdge = pEdge;
		stackItem.iWay = 1;
		if ((pstack =
		     dgl_mempush(pstack, &istack, sizeof(stackItem),
				 &stackItem)) == NULL) {
		    pgraphIn->iErrno = DGL_ERR_MemoryExhausted;
		    goto dfs_error;
		}
	    }
	    DGL_EDGESET_T_RELEASE_FUNC(&laT);
	}

	if (dglTreeNodeAdd(pvVisited, DGL_NODE_ID(pHead)) == NULL) {
	    pgraphIn->iErrno = DGL_ERR_MemoryExhausted;
	    goto dfs_error;
	}
    }

    while ((pStackItem =
	    (struct _stackItem *)dgl_mempop(pstack, &istack,
					    sizeof(stackItem))) != NULL) {
	pHead = pStackItem->pnHead;
	pEdge = pStackItem->pnEdge;

	if (pStackItem->iWay == 0)
	    pTail = _DGL_EDGE_TAILNODE(pgraphIn, pEdge);
	else
	    pTail = _DGL_EDGE_HEADNODE(pgraphIn, pEdge);

	findVisited.nKey = DGL_NODE_ID(pTail);
	if (avl_find(pvVisited, &findVisited)) {	/* already visited */
	    continue;
	}

	if (fnClip) {
	    clipInput.pnNodeFrom = pHead;
	    clipInput.pnEdge = pEdge;
	    clipInput.pnNodeTo = pTail;
	    if (fnClip(pgraphIn, pgraphOut, &clipInput, NULL, pvClipArg))
		continue;
	}

	if (dglTreeNodeAdd(pvVisited, DGL_NODE_ID(pTail)) == NULL) {
	    pgraphIn->iErrno = DGL_ERR_MemoryExhausted;
	    goto dfs_error;
	}

	/* add this edge */
	nret = DGL_ADD_EDGE_FUNC(pgraphOut,
				 DGL_NODE_ID(pHead),
				 DGL_NODE_ID(pTail),
				 DGL_EDGE_COST(pEdge),
				 DGL_EDGE_ID(pEdge),
				 DGL_NODE_ATTR_PTR(pHead),
				 DGL_NODE_ATTR_PTR(pTail),
				 DGL_EDGE_ATTR_PTR(pEdge), 0);

	if (nret < 0) {
	    goto dfs_error;
	}

	if ((DGL_NODE_STATUS(pHead) & DGL_NS_HEAD) || pgraphIn->Version == 3) {

	    pEdgeset = _DGL_OUTEDGESET(pgraphIn, pTail);
	    if (DGL_EDGESET_T_INITIALIZE_FUNC(pgraphIn, &laT, pEdgeset) < 0) {
		goto dfs_error;
	    }
	    for (pEdge = DGL_EDGESET_T_FIRST_FUNC(&laT);
		 pEdge; pEdge = DGL_EDGESET_T_NEXT_FUNC(&laT)
		) {
		stackItem.pnHead = pTail;
		stackItem.pnEdge = pEdge;
		stackItem.iWay = 0;
		if ((pstack =
		     dgl_mempush(pstack, &istack, sizeof(stackItem),
				 &stackItem)) == NULL) {
		    pgraphIn->iErrno = DGL_ERR_MemoryExhausted;
		    goto dfs_error;
		}
	    }
	    DGL_EDGESET_T_RELEASE_FUNC(&laT);

	    if (pgraphIn->Version == 3) {
		pEdgeset = _DGL_INEDGESET(pgraphIn, pTail);
		if (DGL_EDGESET_T_INITIALIZE_FUNC(pgraphIn, &laT, pEdgeset) <
		    0) {
		    goto dfs_error;
		}
		for (pEdge = DGL_EDGESET_T_FIRST_FUNC(&laT);
		     pEdge; pEdge = DGL_EDGESET_T_NEXT_FUNC(&laT)
		    ) {
		    if (DGL_EDGE_STATUS(pEdge) & DGL_ES_DIRECTED)
			continue;
		    stackItem.pnHead = pTail;
		    stackItem.pnEdge = pEdge;
		    stackItem.iWay = 1;
		    if ((pstack =
			 dgl_mempush(pstack, &istack, sizeof(stackItem),
				     &stackItem)) == NULL) {
			pgraphIn->iErrno = DGL_ERR_MemoryExhausted;
			goto dfs_error;
		    }
		}
		DGL_EDGESET_T_RELEASE_FUNC(&laT);
	    }
	}
    }

    if (pstack)
	free(pstack);
    return 0;

  dfs_error:
    if (pstack)
	free(pstack);
    return -pgraphIn->iErrno;
}

/*
 * Use a edge prioritized, tree growing scheme (aka Prim algorithm) in order to
 * be appliable to both undirected graphs (minimum spanning tree - MST) and
 * digraphs (minimum arborescense tree - MAT)
 * The vertex argument is ignored in MST (when algorithm is applied to a
 * version 3 undirected graph).
 */
int DGL_SPAN_MINIMUM_SPANNING_FUNC(dglGraph_s * pgraphIn,
				   dglGraph_s * pgraphOut,
				   dglInt32_t nVertex,
				   dglSpanClip_fn fnClip, void *pvClipArg)
{
    dglInt32_t *pHead, *pTail, *pEdgeset, *pEdge;
    dglHeap_s FrontEdgeHeap;
    dglHeapData_u HeapData;
    dglHeapNode_s HeapItem;
    dglTreeNode_s *pPredistItem, findItem;
    dglEdgesetTraverser_s laT;
    int nret;

    dglHeapInit(&FrontEdgeHeap);

    if (pgraphIn->Version == 3) {	/* undirected: pick up the first node */
	dglNodeTraverser_s pT;

	DGL_NODE_T_INITIALIZE_FUNC(pgraphIn, &pT);
	pHead = DGL_NODE_T_FIRST_FUNC(&pT);
	DGL_NODE_T_RELEASE_FUNC(&pT);
    }
    else {			/* directed: pick up the arborescense origin */
	pHead = DGL_GET_NODE_FUNC(pgraphIn, nVertex);
    }

    if (pHead == NULL) {
	pgraphIn->iErrno = DGL_ERR_HeadNodeNotFound;
	goto mst_error;
    }

    if (DGL_NODE_STATUS(pHead) & DGL_NS_HEAD ||
	DGL_NODE_STATUS(pHead) & DGL_NS_ALONE) {

	if ((DGL_NODE_STATUS(pHead) & DGL_NS_HEAD) ||
	    (DGL_NODE_STATUS(pHead) & DGL_NS_ALONE) ||
	    pgraphIn->Version == 3) {
	    if (DGL_ADD_NODE_FUNC
		(pgraphOut, DGL_NODE_ID(pHead), DGL_NODE_ATTR_PTR(pHead),
		 0) < 0) {
		goto mst_error;
	    }

	    if (DGL_NODE_STATUS(pHead) & DGL_NS_ALONE) {
		dglHeapFree(&FrontEdgeHeap, NULL);
		return 0;
	    }

	    pEdgeset = _DGL_OUTEDGESET(pgraphIn, pHead);
	    if (DGL_EDGESET_T_INITIALIZE_FUNC(pgraphIn, &laT, pEdgeset) < 0) {
		goto mst_error;
	    }
	    for (pEdge = DGL_EDGESET_T_FIRST_FUNC(&laT);
		 pEdge; pEdge = DGL_EDGESET_T_NEXT_FUNC(&laT)
		) {
		HeapData.pv = pEdge;
		if (dglHeapInsertMin
		    (&FrontEdgeHeap, DGL_EDGE_COST(pEdge), 0, HeapData) < 0) {
		    pgraphIn->iErrno = DGL_ERR_HeapError;
		    goto mst_error;
		}
	    }
	    DGL_EDGESET_T_RELEASE_FUNC(&laT);
	    if (pgraphIn->Version == 3) {
		pEdgeset = _DGL_INEDGESET(pgraphIn, pHead);
		if (DGL_EDGESET_T_INITIALIZE_FUNC(pgraphIn, &laT, pEdgeset) <
		    0) {
		    goto mst_error;
		}
		for (pEdge = DGL_EDGESET_T_FIRST_FUNC(&laT);
		     pEdge; pEdge = DGL_EDGESET_T_NEXT_FUNC(&laT)
		    ) {
		    if (DGL_EDGE_STATUS(pEdge) & DGL_ES_DIRECTED)
			continue;
		    HeapData.pv = pEdge;
		    if (dglHeapInsertMin
			(&FrontEdgeHeap, DGL_EDGE_COST(pEdge), 1,
			 HeapData) < 0) {
			pgraphIn->iErrno = DGL_ERR_HeapError;
			goto mst_error;
		    }
		}
		DGL_EDGESET_T_RELEASE_FUNC(&laT);
	    }
	}
    }
    else {
	pgraphIn->iErrno = DGL_ERR_BadEdge;
	goto mst_error;
    }

    while (dglHeapExtractMin(&FrontEdgeHeap, &HeapItem) == 1) {
	pEdge = HeapItem.value.pv;

	if (HeapItem.flags == 0) {
	    if ((pHead = _DGL_EDGE_HEADNODE(pgraphIn, pEdge)) == NULL) {
		pgraphIn->iErrno = DGL_ERR_UnexpectedNullPointer;
		goto mst_error;
	    }
	    if ((pTail = _DGL_EDGE_TAILNODE(pgraphIn, pEdge)) == NULL) {
		pgraphIn->iErrno = DGL_ERR_UnexpectedNullPointer;
		goto mst_error;
	    }
	}
	else if (pgraphIn->Version == 3) {
	    if ((pTail = _DGL_EDGE_HEADNODE(pgraphIn, pEdge)) == NULL) {
		pgraphIn->iErrno = DGL_ERR_UnexpectedNullPointer;
		goto mst_error;
	    }
	    if ((pHead = _DGL_EDGE_TAILNODE(pgraphIn, pEdge)) == NULL) {
		pgraphIn->iErrno = DGL_ERR_UnexpectedNullPointer;
		goto mst_error;
	    }
	}
	else
	    continue;

	findItem.nKey = DGL_NODE_ID(pTail);

	if ((pPredistItem =
	     avl_find(pgraphOut->pNodeTree, &findItem)) != NULL) {
	    continue;
	}

	nret = DGL_ADD_EDGE_FUNC(pgraphOut,
				 DGL_NODE_ID(pHead),
				 DGL_NODE_ID(pTail),
				 DGL_EDGE_COST(pEdge),
				 DGL_EDGE_ID(pEdge),
				 DGL_NODE_ATTR_PTR(pHead),
				 DGL_NODE_ATTR_PTR(pTail),
				 DGL_EDGE_ATTR_PTR(pEdge), 0);

	if (nret < 0) {
	    goto mst_error;
	}

	pHead = pTail;

	if ((DGL_NODE_STATUS(pHead) & DGL_NS_HEAD) || pgraphIn->Version == 3) {
	    pEdgeset = _DGL_OUTEDGESET(pgraphIn, pHead);
	    if (DGL_EDGESET_T_INITIALIZE_FUNC(pgraphIn, &laT, pEdgeset) < 0) {
		goto mst_error;
	    }
	    for (pEdge = DGL_EDGESET_T_FIRST_FUNC(&laT);
		 pEdge; pEdge = DGL_EDGESET_T_NEXT_FUNC(&laT)
		) {
		HeapData.pv = pEdge;
		if (dglHeapInsertMin
		    (&FrontEdgeHeap, DGL_EDGE_COST(pEdge), 0, HeapData) < 0) {
		    pgraphIn->iErrno = DGL_ERR_HeapError;
		    goto mst_error;
		}
	    }
	    if (pgraphIn->Version == 3) {
		DGL_EDGESET_T_RELEASE_FUNC(&laT);
		pEdgeset = _DGL_OUTEDGESET(pgraphIn, pHead);
		if (DGL_EDGESET_T_INITIALIZE_FUNC(pgraphIn, &laT, pEdgeset) <
		    0) {
		    goto mst_error;
		}
		for (pEdge = DGL_EDGESET_T_FIRST_FUNC(&laT);
		     pEdge; pEdge = DGL_EDGESET_T_NEXT_FUNC(&laT)
		    ) {
		    if (DGL_EDGE_STATUS(pEdge) & DGL_ES_DIRECTED)
			continue;
		    HeapData.pv = pEdge;
		    if (dglHeapInsertMin
			(&FrontEdgeHeap, DGL_EDGE_COST(pEdge), 1,
			 HeapData) < 0) {
			pgraphIn->iErrno = DGL_ERR_HeapError;
			goto mst_error;
		    }
		}
		DGL_EDGESET_T_RELEASE_FUNC(&laT);
	    }
	}
    }
    dglHeapFree(&FrontEdgeHeap, NULL);
    return 0;

  mst_error:
    dglHeapFree(&FrontEdgeHeap, NULL);
    return -pgraphIn->iErrno;
}

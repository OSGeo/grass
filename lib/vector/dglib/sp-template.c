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
 * SHORTEST PATH CACHE
 * 
 * components:
 *   - start node id
 *   - visited network: a node is marked as visited when its departing
 *     edges have been added to the cache
 *   - predist network: node distances from start node
 *   - NodeHeap: holds unvisited nodes, the next node extracted is the
 *     unvisited node closest to SP start
 *
 * not all nodes in the predist network have been visited, SP from start
 * is known only for visited nodes
 * unvisited nodes can be reached, but not necessarily on the shortest
 * possible path
 * important for DGL_SP_CACHE_DISTANCE_FUNC and DGL_SP_CACHE_REPORT_FUNC
 */

#if !defined(DGL_DEFINE_TREE_PROCS) && !defined(DGL_DEFINE_FLAT_PROCS)

int DGL_SP_CACHE_INITIALIZE_FUNC(dglGraph_s * pgraph, dglSPCache_s * pCache,
				 dglInt32_t nStart)
{
    pCache->nStartNode = nStart;
    pCache->pvVisited = NULL;
    pCache->pvPredist = NULL;
    dglHeapInit(&pCache->NodeHeap);
    if ((pCache->pvVisited =
	 avl_create(dglTreeTouchI32Compare, NULL,
		    dglTreeGetAllocator())) == NULL)
	return -1;
    if ((pCache->pvPredist =
	 avl_create(dglTreePredistCompare, NULL,
		    dglTreeGetAllocator())) == NULL)
	return -1;
    return 0;
}

void DGL_SP_CACHE_RELEASE_FUNC(dglGraph_s * pgraph, dglSPCache_s * pCache)
{
    if (pCache->pvVisited)
	avl_destroy(pCache->pvVisited, dglTreeTouchI32Cancel);
    if (pCache->pvPredist)
	avl_destroy(pCache->pvPredist, dglTreePredistCancel);
    dglHeapFree(&pCache->NodeHeap, NULL);
}


static int DGL_SP_CACHE_DISTANCE_FUNC(dglGraph_s * pgraph,
				      dglSPCache_s * pCache,
				      dglInt32_t * pnDistance,
				      dglInt32_t nStart,
				      dglInt32_t nDestination)
{
    dglTreeTouchI32_s VisitedItem;
    dglTreePredist_s *pPredistItem, PredistItem;

    if (pCache->nStartNode != nStart) {
	pgraph->iErrno = DGL_ERR_HeadNodeNotFound;
	return -pgraph->iErrno;
    }

    VisitedItem.nKey = nDestination;
    if (avl_find(pCache->pvVisited, &VisitedItem) == NULL) {
	pgraph->iErrno = DGL_ERR_TailNodeNotFound;
	return -pgraph->iErrno;
    }

    PredistItem.nKey = nDestination;
    if ((pPredistItem = avl_find(pCache->pvPredist, &PredistItem)) == NULL) {
	pgraph->iErrno = DGL_ERR_UnexpectedNullPointer;
	return -pgraph->iErrno;
    }

    if (pnDistance)
	*pnDistance = pPredistItem->nDistance;
    return 0;
}

static dglSPReport_s *DGL_SP_CACHE_REPORT_FUNC(dglGraph_s * pgraph,
					       dglSPCache_s * pCache,
					       dglInt32_t nStart,
					       dglInt32_t nDestination)
{
    dglTreeTouchI32_s VisitedItem;
    dglTreePredist_s *pPredistItem, PredistItem;
    dglInt32_t *pEdge;
    dglInt32_t *pDestination;
    dglSPArc_s arc;
    long i, istack = 0;
    unsigned char *pstack = NULL;
    unsigned char *ppop;
    dglSPReport_s *pReport = NULL;

    if (pCache->nStartNode != nStart) {
	pgraph->iErrno = DGL_ERR_HeadNodeNotFound;
	return NULL;
    }

    VisitedItem.nKey = nDestination;
    if (avl_find(pCache->pvVisited, &VisitedItem) == NULL) {
	pgraph->iErrno = DGL_ERR_TailNodeNotFound;
	return NULL;
    }

    PredistItem.nKey = nDestination;
    if (avl_find(pCache->pvPredist, &PredistItem) == NULL) {
	pgraph->iErrno = DGL_ERR_UnexpectedNullPointer;
	return NULL;
    }

    for (PredistItem.nKey = nDestination,
	 pPredistItem = avl_find(pCache->pvPredist, &PredistItem);
	 pPredistItem;
	 PredistItem.nKey = pPredistItem->nFrom,
	 pPredistItem = avl_find(pCache->pvPredist, &PredistItem)
	) {
	if (pPredistItem->nFrom < 0) {
	    pgraph->iErrno = DGL_ERR_BadEdge;
	    goto spr_error;
	}

	pEdge = (dglInt32_t *) pPredistItem->pnEdge;

	if (pPredistItem->bFlags == 0) {
	    if (pgraph->Flags & DGL_GS_FLAT) {
		pDestination =
		    DGL_NODEBUFFER_SHIFT(pgraph,
					 DGL_EDGE_TAILNODE_OFFSET(pEdge));
	    }
	    else {
		pDestination =
		    DGL_GET_NODE_FUNC(pgraph,
				      DGL_EDGE_TAILNODE_OFFSET(pEdge));
	    }
	}
	else {
	    if (pgraph->Flags & DGL_GS_FLAT) {
		pDestination =
		    DGL_NODEBUFFER_SHIFT(pgraph,
					 DGL_EDGE_HEADNODE_OFFSET(pEdge));
	    }
	    else {
		pDestination =
		    DGL_GET_NODE_FUNC(pgraph,
				      DGL_EDGE_HEADNODE_OFFSET(pEdge));
	    }
	}

	if ((arc.pnEdge = DGL_EDGE_ALLOC(pgraph->EdgeAttrSize)) == NULL)
	    goto spr_error;
	arc.nFrom = pPredistItem->nFrom;
	arc.nTo = DGL_NODE_ID(pDestination);
	arc.nDistance = pPredistItem->nDistance;
	memcpy(arc.pnEdge, pEdge, DGL_EDGE_SIZEOF(pgraph->EdgeAttrSize));
	DGL_EDGE_COST(arc.pnEdge) = pPredistItem->nCost;

	if ((pstack =
	     dgl_mempush(pstack, &istack, sizeof(dglSPArc_s),
			 &arc)) == NULL) {
	    pgraph->iErrno = DGL_ERR_MemoryExhausted;
	    goto spr_error;
	}

	if (arc.nFrom == nStart)
	    break;
    }

    if (pPredistItem == NULL) {
	pgraph->iErrno = DGL_ERR_UnexpectedNullPointer;
	goto spr_error;
    }

    if ((pReport = malloc(sizeof(dglSPReport_s))) == NULL) {
	pgraph->iErrno = DGL_ERR_MemoryExhausted;
	goto spr_error;
    }
    memset(pReport, 0, sizeof(dglSPReport_s));

    pReport->cArc = istack;

    if ((pReport->pArc = malloc(sizeof(dglSPArc_s) * pReport->cArc)) == NULL) {
	pgraph->iErrno = DGL_ERR_MemoryExhausted;
	goto spr_error;
    }

    pReport->nDistance = 0;

    for (i = 0;
	 (ppop = dgl_mempop(pstack, &istack, sizeof(dglSPArc_s))) != NULL;
	 i++) {
	memcpy(&pReport->pArc[i], ppop, sizeof(dglSPArc_s));
	pReport->nDistance += DGL_EDGE_COST(pReport->pArc[i].pnEdge);
    }

    pReport->nStartNode = nStart;
    pReport->nDestinationNode = nDestination;

    if (pstack)
	free(pstack);

    return pReport;

  spr_error:
    if (pstack)
	free(pstack);
    if (pReport)
	dglFreeSPReport(pgraph, pReport);

    return NULL;
}
#endif

#if defined(DGL_DEFINE_TREE_PROCS) || defined(DGL_DEFINE_FLAT_PROCS)

#define __EDGELOOP_BODY_1(f) \
		if ( (f) == 0 ) { \
			pDestination = _DGL_EDGE_TAILNODE(pgraph, pEdge); \
		} \
		else { \
			pDestination = _DGL_EDGE_HEADNODE(pgraph, pEdge); \
		} \
		if ( !(DGL_NODE_STATUS(pDestination) & DGL_NS_TAIL) && pgraph->Version < 3) { \
			pgraph->iErrno = DGL_ERR_BadEdge; \
			goto sp_error; \
		} \
		clipOutput.nEdgeCost = DGL_EDGE_COST(pEdge); \
		if ( fnClip ) { \
			clipInput.pnPrevEdge 	= NULL; \
			clipInput.pnNodeFrom 	= pStart; \
			clipInput.pnEdge		= pEdge; \
			clipInput.pnNodeTo		= pDestination; \
			clipInput.nFromDistance = 0; \
			if ( fnClip( pgraph , & clipInput , & clipOutput , pvClipArg ) ) continue; \
		} \
		findPredist.nKey = DGL_NODE_ID(pDestination); \
		if ( (pPredistItem = avl_find( pCache->pvPredist, &findPredist)) == NULL ) { \
			if ( (pPredistItem = dglTreePredistAdd( pCache->pvPredist, DGL_NODE_ID(pDestination) )) == NULL ) { \
				pgraph->iErrno = DGL_ERR_MemoryExhausted; \
				goto sp_error; \
			} \
		} \
		else { \
			if ( pPredistItem->nDistance <= clipOutput.nEdgeCost ) { \
				continue; \
			} \
		} \
		pPredistItem->nFrom      = nStart; \
		pPredistItem->pnEdge     = pEdge; \
		pPredistItem->nCost      = clipOutput.nEdgeCost; \
		pPredistItem->nDistance  = clipOutput.nEdgeCost; \
		pPredistItem->bFlags	 = (f); \
		heapvalue.pv = pEdge; \
		if ( dglHeapInsertMin( & pCache->NodeHeap, pPredistItem->nDistance , f , heapvalue ) < 0 ) { \
			pgraph->iErrno = DGL_ERR_HeapError; \
			goto sp_error; \
		}

#define __EDGELOOP_BODY_2(f) \
			if ( (f) == 0 ) { \
				pDestination = _DGL_EDGE_TAILNODE(pgraph, pEdge); \
			} \
			else if ( pgraph->Version == 3 ) { \
				pDestination = _DGL_EDGE_HEADNODE(pgraph, pEdge); \
			} \
			if ( !(DGL_NODE_STATUS(pDestination) & DGL_NS_TAIL) && pgraph->Version < 3) { \
				pgraph->iErrno = DGL_ERR_BadEdge; \
				goto sp_error; \
			} \
			clipOutput.nEdgeCost = DGL_EDGE_COST(pEdge); \
			if ( fnClip ) { \
				clipInput.pnPrevEdge 	= pEdge_prev; \
				clipInput.pnNodeFrom 	= pStart; \
				clipInput.pnEdge		= pEdge; \
				clipInput.pnNodeTo		= pDestination; \
				clipInput.nFromDistance = fromDist; \
				if ( fnClip( pgraph , & clipInput , & clipOutput , pvClipArg ) ) continue; \
			} \
			findPredist.nKey = DGL_NODE_ID(pDestination); \
			if ( (pPredistItem = avl_find( pCache->pvPredist, &findPredist)) == NULL ) { \
				if ( (pPredistItem = dglTreePredistAdd( pCache->pvPredist, DGL_NODE_ID(pDestination) )) == NULL ) { \
					pgraph->iErrno = DGL_ERR_MemoryExhausted; \
					goto sp_error; \
				} \
			} \
			else { \
				if ( pPredistItem->nDistance <= fromDist + clipOutput.nEdgeCost ) { \
					continue; \
				} \
			} \
			pPredistItem->nFrom     = DGL_NODE_ID(pStart); \
			pPredistItem->pnEdge    = pEdge; \
			pPredistItem->nCost     = clipOutput.nEdgeCost; \
			pPredistItem->nDistance = fromDist + clipOutput.nEdgeCost; \
			pPredistItem->bFlags	= (f); \
			heapvalue.pv = pEdge; \
			if ( dglHeapInsertMin( & pCache->NodeHeap, pPredistItem->nDistance ,  f , heapvalue ) < 0 ) { \
				pgraph->iErrno = DGL_ERR_HeapError; \
				goto sp_error; \
			}

/*
 * Dijkstra Shortest Path 
 */
int DGL_SP_DIJKSTRA_FUNC(dglGraph_s * pgraph,
			 dglSPReport_s ** ppReport,
			 dglInt32_t * pDistance,
			 dglInt32_t nStart,
			 dglInt32_t nDestination,
			 dglSPClip_fn fnClip,
			 void *pvClipArg, dglSPCache_s * pCache)
{
    dglInt32_t *pStart;		/* pointer to the start node (pgraph->pNodeBuffer) */
    register dglInt32_t *pDestination;	/* temporary destination pointer */
    register dglInt32_t *pEdgeset;	/* pointer to the edge (pgraph->pEdgeBuffer) */
    register dglInt32_t *pEdge;	/* pointer to the to-edges in edge  */
    register dglInt32_t *pEdge_prev;	/* pointer to the previous edge in path */
    int nRet;
    dglEdgesetTraverser_s laT;

    dglSPCache_s spCache;
    int new_cache = 0;

    /*
     * shortest path distance temporary min heap
     */
    dglHeapData_u heapvalue;
    dglHeapNode_s heapnode;

    /*
     * shortest path visited network
     */
    dglTreeTouchI32_s *pVisitedItem, findVisited;

    /*
     * shortest path predecessor and distance network
     */
    dglTreePredist_s *pPredistItem, findPredist;

    /*
     * args to clip()
     */
    dglSPClipInput_s clipInput;
    dglSPClipOutput_s clipOutput;


    /*
     * Initialize the cache: initialize the heap and create temporary networks -
     * The use of a predist network for predecessor and distance has two important results:
     * 1) allows us not having to reset the whole graph status at each call;
     * 2) use of a stack memory area for temporary (and otherwise possibly thread-conflicting) states.
     * If a cache pointer was supplied, do not initialize it but try to get SP immediately.
     */
    if (pCache == NULL) {
	pCache = &spCache;
	DGL_SP_CACHE_INITIALIZE_FUNC(pgraph, pCache, nStart);
	new_cache = 1;
    }
    else {
	if (ppReport) {
	    if ((*ppReport =
		 DGL_SP_CACHE_REPORT_FUNC(pgraph, pCache, nStart,
					  nDestination)) != NULL) {
		return 1;
	    }
	}
	else {
	    if (DGL_SP_CACHE_DISTANCE_FUNC
		(pgraph, pCache, pDistance, nStart, nDestination) >= 0) {
		return 2;
	    }
	}
	if (pgraph->iErrno == DGL_ERR_HeadNodeNotFound) {
	    DGL_SP_CACHE_RELEASE_FUNC(pgraph, pCache);
	    DGL_SP_CACHE_INITIALIZE_FUNC(pgraph, pCache, nStart);
	    new_cache = 1;
	}
	else if (pgraph->iErrno != DGL_ERR_TailNodeNotFound) {
	    goto sp_error;
	}
    }

    /*
     * reset error status after using the cache
     */
    pgraph->iErrno = 0;

    if ((pStart = DGL_GET_NODE_FUNC(pgraph, nStart)) == NULL) {
	pgraph->iErrno = DGL_ERR_HeadNodeNotFound;
	goto sp_error;
    }

    if ((pDestination = DGL_GET_NODE_FUNC(pgraph, nDestination)) == NULL) {
	pgraph->iErrno = DGL_ERR_TailNodeNotFound;
	goto sp_error;
    }

    if ((DGL_NODE_STATUS(pStart) & DGL_NS_ALONE) ||
	(DGL_NODE_STATUS(pDestination) & DGL_NS_ALONE)) {
	goto sp_error;
    }

    if (!(DGL_NODE_STATUS(pStart) & DGL_NS_HEAD) && pgraph->Version < 3) {
	goto sp_error;
    }

    if (!(DGL_NODE_STATUS(pDestination) & DGL_NS_TAIL) && pgraph->Version < 3) {
	goto sp_error;
    }

    /* if we do not need a new cache, we just continue with the unvisited
     * nodes in the cache */
    if (new_cache) {
	/*
	 * now we inspect all edges departing from the start node
	 * - at each loop 'pedge' points to the edge in the edge buffer
	 * - we invoke the caller's clip() and eventually skip the edge (clip() != 0)
	 * - we insert a item in the predist network to set actual predecessor and distance
	 *   (there is no precedecessor at this stage) and actual distance from the starting node
	 *   (at this stage it equals the edge's cost)
	 * - we insert a item in the node min-heap (sorted on node distance), storing the offset of the
	 *   edge in the edge buffer.
	 * In the case of undirected graph (version 3) we inspect input edges as well.
	 */
	pEdgeset = _DGL_OUTEDGESET(pgraph, pStart);
	if (DGL_EDGESET_T_INITIALIZE_FUNC(pgraph, &laT, pEdgeset) < 0) {
	    goto sp_error;
	}
	for (pEdge = DGL_EDGESET_T_FIRST_FUNC(&laT);
	     pEdge; pEdge = DGL_EDGESET_T_NEXT_FUNC(&laT)
	    ) {
	    __EDGELOOP_BODY_1(0);
	}
	DGL_EDGESET_T_RELEASE_FUNC(&laT);

	if (pgraph->Version == 3) {
	    pEdgeset = _DGL_INEDGESET(pgraph, pStart);
	    if (DGL_EDGESET_T_INITIALIZE_FUNC(pgraph, &laT, pEdgeset) < 0) {
		goto sp_error;
	    }
	    for (pEdge = DGL_EDGESET_T_FIRST_FUNC(&laT);
		 pEdge; pEdge = DGL_EDGESET_T_NEXT_FUNC(&laT)
		) {
		if (DGL_EDGE_STATUS(pEdge) & DGL_ES_DIRECTED)
		    continue;
		__EDGELOOP_BODY_1(1);
	    }
	    DGL_EDGESET_T_RELEASE_FUNC(&laT);
	}
    }

    /*
     * Now we begin extracting nodes from the min-heap. Each node extracted is
     * the one that is actually closest to the SP start.
     */
    while (dglHeapExtractMin(&pCache->NodeHeap, &heapnode) == 1) {
	dglInt32_t fromDist;

	/*
	 * recover the stored edge pointer
	 */
	pEdge = heapnode.value.pv;

	/*
	 * the new relative head is the tail of the edge
	 * or the head of the edge if the traversal was reversed (undirected edge)
	 */
	if (heapnode.flags == 0) {
	    pStart = _DGL_EDGE_TAILNODE(pgraph, pEdge);	/* continue from previous tail */
	}
	else {
	    pStart = _DGL_EDGE_HEADNODE(pgraph, pEdge);	/* reversed head/tail */
	}

	/*
	 * We do not want to explore twice the same node as a relative starting point,
	 * that's the meaning of 'visited'. We mark actual start node as 'visited' by
	 * inserting it into the visited-network. If we find actual node in the network
	 * we just give up and continue looping. Otherwise we add actual node to the network.
	 */
	findVisited.nKey = DGL_NODE_ID(pStart);
	if ((pVisitedItem =
	     avl_find(pCache->pvVisited, &findVisited)) == NULL) {
	    if (dglTreeTouchI32Add(pCache->pvVisited, DGL_NODE_ID(pStart)) ==
		NULL) {
		pgraph->iErrno = DGL_ERR_MemoryExhausted;
		goto sp_error;
	    }
	}

	/*
	 * Give up with visited nodes now
	 */
	if (pVisitedItem) {
	    if (DGL_NODE_ID(pStart) == nDestination) {
		/* should not happen but does not harm
		 * this case should have been handled above */
		goto destination_found;
	    }
	    else
		continue;
	}

	/*
	 * If the node is not marked as having departing edges, then we are into a
	 * blind alley. Just give up this direction and continue looping.
	 * This only applies to v1 and v2 (digraphs)
	 */
	if (!(DGL_NODE_STATUS(pStart) & DGL_NS_HEAD) && pgraph->Version < 3) {
	    if (DGL_NODE_ID(pStart) == nDestination) {
		goto destination_found;
	    }
	    else
		continue;
	}

	/*
	 * save actual edge for later clip()
	 */
	pEdge_prev = pEdge;

	/*
	 * Recover the head node distance from the predist network
	 */
	findPredist.nKey = DGL_NODE_ID(pStart);
	if ((pPredistItem =
	     avl_find(pCache->pvPredist, &findPredist)) == NULL) {
	    pgraph->iErrno = DGL_ERR_UnexpectedNullPointer;
	    goto sp_error;
	}

	fromDist = pPredistItem->nDistance;

	/*
	 * Loop on departing edges:
	 * Scan the edgeset and loads pedge at each iteration with next-edge.
	 * iWay == DGL_EDGESET_T_WAY_OUT then pedge is a out arc (departing from node) else ot is a in arc.
	 * V1 has no in-degree support so iWay is always OUT, V2/3 have in-degree support.
	 *
	 * This loop needs to be done also when destination is found, otherwise
	 * the node is marked as visited but its departing edges are not added to the cache
	 * --> loose end, we might need these edges later on
	 */
	pEdgeset = _DGL_OUTEDGESET(pgraph, pStart);
	if (DGL_EDGESET_T_INITIALIZE_FUNC(pgraph, &laT, pEdgeset) < 0) {
	    goto sp_error;
	}
	for (pEdge = DGL_EDGESET_T_FIRST_FUNC(&laT);
	     pEdge; pEdge = DGL_EDGESET_T_NEXT_FUNC(&laT)
	    ) {
	    __EDGELOOP_BODY_2(0);
	}
	DGL_EDGESET_T_RELEASE_FUNC(&laT);

	if (pgraph->Version == 3) {
	    pEdgeset = _DGL_INEDGESET(pgraph, pStart);
	    if (DGL_EDGESET_T_INITIALIZE_FUNC(pgraph, &laT, pEdgeset) < 0) {
		goto sp_error;
	    }
	    for (pEdge = DGL_EDGESET_T_FIRST_FUNC(&laT);
		 pEdge; pEdge = DGL_EDGESET_T_NEXT_FUNC(&laT)
		) {
		if (DGL_EDGE_STATUS(pEdge) & DGL_ES_DIRECTED)
		    continue;
		__EDGELOOP_BODY_2(1);
	    }
	    DGL_EDGESET_T_RELEASE_FUNC(&laT);
	}

	/*
	 * Dijkstra algorithm ends when the destination node is extracted from
	 * the min distance heap, that means: no other path exist in the network giving
	 * a shortest output.
	 * If this happens we jump to the epilogue in order to build a path report and return.
	 */
	if (DGL_NODE_ID(pStart) == nDestination) {
	    goto destination_found;
	}
    }

  sp_error:
    if (pCache == &spCache) {
	DGL_SP_CACHE_RELEASE_FUNC(pgraph, pCache);
    }
    return -pgraph->iErrno;	/* == 0 path not found */

  destination_found:		/* path found - build a shortest path report or report the distance only */

    if (ppReport) {
	*ppReport =
	    DGL_SP_CACHE_REPORT_FUNC(pgraph, pCache, nStart, nDestination);
	if (*ppReport == NULL) {
	    nRet = -pgraph->iErrno;
	}
	else {
	    nRet = 1;
	}
    }
    else {
	if (DGL_SP_CACHE_DISTANCE_FUNC
	    (pgraph, pCache, pDistance, nStart, nDestination) < 0) {
	    nRet = -pgraph->iErrno;
	}
	else {
	    nRet = 2;
	}
    }
    if (pCache == &spCache) {
	DGL_SP_CACHE_RELEASE_FUNC(pgraph, pCache);
    }
    return nRet;
}

#endif

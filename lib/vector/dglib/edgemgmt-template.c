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
 * Add edge can be performed on TREE state graph. If the state is FLAT
 * return BadOnFlatGraph error.
 */
int DGL_ADD_EDGE_FUNC(dglGraph_s * pgraph,
		      dglInt32_t nHead,
		      dglInt32_t nTail,
		      dglInt32_t nCost,
		      dglInt32_t nEdge,
		      void *pvHeadAttr,
		      void *pvTailAttr, void *pvEdgeAttr, dglInt32_t nFlags)
{
    dglInt32_t *pHead;
    dglInt32_t *pTail;
    dglInt32_t *pEdgeset;
    dglInt32_t *pEdge;
    DGL_T_NODEITEM_TYPE *pHeadNodeItem;
    DGL_T_NODEITEM_TYPE *pTailNodeItem;

#if defined(_DGL_V2)
    dglInt32_t *pinEdgeset;
    dglTreeEdge_s *pEdgeItem;
    dglTreeEdge_s findEdge;
#endif

    if (pgraph->Flags & DGL_GS_FLAT) {
	pgraph->iErrno = DGL_ERR_BadOnFlatGraph;
	return -pgraph->iErrno;
    }

#ifdef DGL_STATS
    {
	clock_t clk = clock();
#endif

	if ((pHeadNodeItem =
	     DGL_T_NODEITEM_Add(pgraph->pNodeTree, nHead)) == NULL ||
	    (pTailNodeItem =
	     DGL_T_NODEITEM_Add(pgraph->pNodeTree, nTail)) == NULL) {
	    pgraph->iErrno = DGL_ERR_MemoryExhausted;
	    return -pgraph->iErrno;
	}

#ifdef DGL_STATS
	pgraph->clkNodeTree += clock() - clk;
	pgraph->cNodeTree++;
	pgraph->cNodeTree++;
    }
#endif

    if (DGL_T_NODEITEM_NodePTR(pHeadNodeItem) == NULL) {
	if ((pHead = DGL_NODE_ALLOC(pgraph->NodeAttrSize)) == NULL) {
	    pgraph->iErrno = DGL_ERR_MemoryExhausted;
	    return -1;
	}
	DGL_NODE_STATUS(pHead) = 0;
	DGL_T_NODEITEM_Set_NodePTR(pHeadNodeItem, pHead);
	pgraph->cNode++;
	pgraph->cHead++;
    }
    else {
	pHead = DGL_T_NODEITEM_NodePTR(pHeadNodeItem);
	if (!(DGL_NODE_STATUS(pHead) & DGL_NS_HEAD))
	    pgraph->cHead++;
    }

    if (DGL_T_NODEITEM_NodePTR(pTailNodeItem) == NULL) {
	if ((pTail = DGL_NODE_ALLOC(pgraph->NodeAttrSize)) == NULL) {
	    pgraph->iErrno = DGL_ERR_MemoryExhausted;
	    return -pgraph->iErrno;
	}
	DGL_NODE_STATUS(pTail) = 0;
	DGL_T_NODEITEM_Set_NodePTR(pTailNodeItem, pTail);
	pgraph->cNode++;
	pgraph->cTail++;
    }
    else {
	pTail = DGL_T_NODEITEM_NodePTR(pTailNodeItem);
	if (!(DGL_NODE_STATUS(pTail) & DGL_NS_TAIL))
	    pgraph->cTail++;
    }


    DGL_NODE_STATUS(pHead) |= DGL_NS_HEAD;
    DGL_NODE_STATUS(pTail) |= DGL_NS_TAIL;

    if (DGL_NODE_STATUS(pHead) & DGL_NS_ALONE) {
	DGL_NODE_STATUS(pHead) &= ~DGL_NS_ALONE;
	pgraph->cAlone--;
    }

    if (DGL_NODE_STATUS(pTail) & DGL_NS_ALONE) {
	DGL_NODE_STATUS(pTail) &= ~DGL_NS_ALONE;
	pgraph->cAlone--;
    }

    DGL_NODE_ID(pHead) = nHead;
    DGL_NODE_ID(pTail) = nTail;

    DGL_NODE_EDGESET_OFFSET(pHead) = -1;
    DGL_NODE_EDGESET_OFFSET(pTail) = -1;

    if (pvHeadAttr && pgraph->NodeAttrSize) {
	memcpy(DGL_NODE_ATTR_PTR(pHead), pvHeadAttr, pgraph->NodeAttrSize);
    }

    if (pvTailAttr && pgraph->NodeAttrSize) {
	memcpy(DGL_NODE_ATTR_PTR(pTail), pvTailAttr, pgraph->NodeAttrSize);
    }


    /*
       if ( DGL_T_NODEITEM_OutEdgesetPTR(pTailNodeItem) == NULL )
       {
       pEdgeset = DGL_EDGESET_ALLOC( 0 , pgraph->EdgeAttrSize );
       if ( pEdgeset == NULL ) {
       pgraph->iErrno = DGL_ERR_MemoryExhausted;
       return -pgraph->iErrno;
       }
       DGL_EDGESET_EDGECOUNT(pEdgeset) = 0;
       DGL_T_NODEITEM_Set_OutEdgesetPTR(pTailNodeItem,pEdgeset);
       }
     */

    if ((pEdgeset = DGL_T_NODEITEM_OutEdgesetPTR(pHeadNodeItem)) == NULL) {
	pEdgeset = DGL_EDGESET_ALLOC(1, pgraph->EdgeAttrSize);
	if (pEdgeset == NULL) {
	    pgraph->iErrno = DGL_ERR_MemoryExhausted;
	    return -pgraph->iErrno;
	}
	DGL_EDGESET_EDGECOUNT(pEdgeset) = 0;
	DGL_T_NODEITEM_Set_OutEdgesetPTR(pHeadNodeItem, pEdgeset);
    }
    else {
	pEdgeset = DGL_EDGESET_REALLOC(pEdgeset,
				       DGL_EDGESET_EDGECOUNT(pEdgeset) + 1,
				       pgraph->EdgeAttrSize);

	if (pEdgeset == NULL) {
	    pgraph->iErrno = DGL_ERR_MemoryExhausted;
	    return -pgraph->iErrno;
	}
	DGL_T_NODEITEM_Set_OutEdgesetPTR(pHeadNodeItem, pEdgeset);
    }

#if defined(_DGL_V2)
    /*
       if ( DGL_T_NODEITEM_InEdgesetPTR(pHeadNodeItem) == NULL )
       {
       pinEdgeset = DGL_EDGESET_ALLOC( 0 , pgraph->EdgeAttrSize );
       if ( pinEdgeset == NULL ) {
       pgraph->iErrno = DGL_ERR_MemoryExhausted;
       return -pgraph->iErrno;
       }
       DGL_EDGESET_EDGECOUNT(pinEdgeset) = 0;
       DGL_T_NODEITEM_Set_InEdgesetPTR(pHeadNodeItem,pinEdgeset);
       }
     */

    if ((pinEdgeset = DGL_T_NODEITEM_InEdgesetPTR(pTailNodeItem)) == NULL) {
	pinEdgeset = DGL_EDGESET_ALLOC(1, pgraph->EdgeAttrSize);
	if (pinEdgeset == NULL) {
	    pgraph->iErrno = DGL_ERR_MemoryExhausted;
	    return -pgraph->iErrno;
	}
	DGL_EDGESET_EDGECOUNT(pinEdgeset) = 0;
	DGL_T_NODEITEM_Set_InEdgesetPTR(pTailNodeItem, pinEdgeset);
    }
    else {
	pinEdgeset = DGL_EDGESET_REALLOC(pinEdgeset,
					 DGL_EDGESET_EDGECOUNT(pinEdgeset) +
					 1, pgraph->EdgeAttrSize);

	if (pinEdgeset == NULL) {
	    pgraph->iErrno = DGL_ERR_MemoryExhausted;
	    return -pgraph->iErrno;
	}
	DGL_T_NODEITEM_Set_InEdgesetPTR(pTailNodeItem, pinEdgeset);
    }

    /*
     * Set the edge-tree
     */
    findEdge.nKey = nEdge;

    if ((pEdgeItem = dglTreeEdgeAdd(pgraph->pEdgeTree, nEdge)) == NULL) {
	pgraph->iErrno = DGL_ERR_MemoryExhausted;
	return -pgraph->iErrno;
    }
    if (pEdgeItem->pv) {
	pgraph->iErrno = DGL_ERR_EdgeAlreadyExist;
	return -pgraph->iErrno;
    }
    if ((pEdgeItem->pv = DGL_EDGE_ALLOC(pgraph->EdgeAttrSize)) == NULL) {
	pgraph->iErrno = DGL_ERR_MemoryExhausted;
	return -pgraph->iErrno;
    }

    /*
     * assign edge id
     */
    pEdgeset[DGL_EDGESET_EDGECOUNT(pEdgeset) + 1] = nEdge;
    pinEdgeset[DGL_EDGESET_EDGECOUNT(pinEdgeset) + 1] = nEdge;
    ++DGL_EDGESET_EDGECOUNT(pEdgeset);
    ++DGL_EDGESET_EDGECOUNT(pinEdgeset);

    /*
       printf( "add edge: node %ld(%ld,%ld) -> %ld(%ld,%ld)\n",
       DGL_NODE_ID(pHead), DGL_EDGESET_EDGECOUNT(pEdgeset),0,
       DGL_NODE_ID(pTail), 0,DGL_EDGESET_EDGECOUNT(pinEdgeset));
     */


    pEdge = pEdgeItem->pv;
#endif

#if defined(_DGL_V1)
    pEdge =
	DGL_EDGESET_EDGE_PTR(pEdgeset, DGL_EDGESET_EDGECOUNT(pEdgeset),
			     pgraph->EdgeAttrSize);
    DGL_EDGESET_EDGECOUNT(pEdgeset)++;
#endif

    DGL_EDGE_HEADNODE_OFFSET(pEdge) = nHead;	/* will be an offset after flattening */
    DGL_EDGE_TAILNODE_OFFSET(pEdge) = nTail;	/* will be an offset after flattening */
    DGL_EDGE_COST(pEdge) = nCost;
    DGL_EDGE_ID(pEdge) = nEdge;

#if !defined(_DGL_V1)
    if (nFlags & DGL_ES_DIRECTED)
	DGL_EDGE_STATUS(pEdge) = DGL_ES_DIRECTED;
    else
	DGL_EDGE_STATUS(pEdge) = 0;
#endif

    pgraph->cEdge++;
    pgraph->nnCost += (dglInt64_t) nCost;

    if (pvEdgeAttr && pgraph->EdgeAttrSize) {
	memcpy(DGL_EDGE_ATTR_PTR(pEdge), pvEdgeAttr, pgraph->EdgeAttrSize);
    }

    /*
     * If requested add a cost-weighted entry into the edge prioritizer
     */
#if !defined(_DGL_V1)
    if (pgraph->nOptions & DGL_GO_EdgePrioritize_COST) {
	if (dgl_edge_prioritizer_add
	    (pgraph, DGL_EDGE_ID(pEdge), DGL_EDGE_COST(pEdge)) < 0) {
	    return -pgraph->iErrno;
	}
    }
#endif

    if (nFlags & DGL_STRONGCONNECT) {
	return DGL_ADD_EDGE_FUNC(pgraph, nTail, nHead, nCost, nEdge,
				 pvHeadAttr, pvTailAttr, pvEdgeAttr,
				 nFlags & ~DGL_STRONGCONNECT);
    }

    return 0;
}

int DGL_DEL_EDGE_FUNC(dglGraph_s * pgraph, dglInt32_t nEdge)
{
#if defined(_DGL_V1)
    pgraph->iErrno = DGL_ERR_NotSupported;
    return -pgraph->iErrno;
#else
    dglTreeEdge_s *pEdgeItem, findEdgeItem;
    dglInt32_t *pEdge;

    if (pgraph->Flags & DGL_GS_FLAT) {
	pgraph->iErrno = DGL_ERR_BadOnFlatGraph;
	return -pgraph->iErrno;
    }

    if (pgraph->pEdgeTree == NULL) {
	pgraph->iErrno = DGL_ERR_UnexpectedNullPointer;
	return -pgraph->iErrno;
    }

    findEdgeItem.nKey = nEdge;
    if ((pEdgeItem = avl_find(pgraph->pEdgeTree, &findEdgeItem)) == NULL) {
	pgraph->iErrno = DGL_ERR_EdgeNotFound;
	return -pgraph->iErrno;
    }

    pEdge = pEdgeItem->pv;

    if (DGL_DEL_NODE_INEDGE_FUNC
	(pgraph, DGL_EDGE_TAILNODE_OFFSET(pEdge), DGL_EDGE_ID(pEdge)) < 0) {
	return -pgraph->iErrno;
    }

    if (DGL_DEL_NODE_OUTEDGE_FUNC
	(pgraph, DGL_EDGE_HEADNODE_OFFSET(pEdge), DGL_EDGE_ID(pEdge)) < 0) {
	return -pgraph->iErrno;
    }


    /* prioritizer sync
     */
    if (pgraph->nOptions & DGL_GO_EdgePrioritize_COST) {
	if (dgl_edge_prioritizer_del
	    (pgraph, DGL_EDGE_ID(pEdge), DGL_EDGE_COST(pEdge)) < 0) {
	    return -pgraph->iErrno;
	}
    }
    /*
     */
    pgraph->cEdge--;
    pgraph->nnCost -= (dglInt64_t) DGL_EDGE_COST(pEdge);

    avl_delete(pgraph->pEdgeTree, pEdgeItem);
    dglTreeEdgeCancel(pEdgeItem, NULL);
    return 0;
#endif
}

dglInt32_t *DGL_GET_EDGE_FUNC(dglGraph_s * pgraph, dglInt32_t nEdge)
{
#if defined(_DGL_V1)
    pgraph->iErrno = DGL_ERR_NotSupported;
    return NULL;
#else
    register dglInt32_t top;	/* top of table */
    register dglInt32_t pos;	/* current position to compare */
    register dglInt32_t bot;	/* bottom of table */
    register dglInt32_t *pref;
    register int cwords;	/* size of a edge in words of 32 bit */
    register dglTreeEdge_s *ptreeEdge;
    dglTreeEdge_s findEdge;
    dglInt32_t id;

    pgraph->iErrno = 0;
    if (pgraph->Flags & DGL_GS_FLAT) {
	cwords = DGL_EDGE_WSIZE(pgraph->EdgeAttrSize);
	/*bot    = pgraph->iEdgeBuffer / DGL_EDGE_SIZEOF(pgraph->EdgeAttrSize); */
	bot = pgraph->cEdge;
	top = 0;
	pos = 0;
	pref = (dglInt32_t *) pgraph->pEdgeBuffer;

	/* perform a binary search
	 */
	while (top != bot) {
	    pos = top + (bot - top) / 2;
	    id = DGL_EDGE_ID(&pref[pos * cwords]);
	    if (id == nEdge) {
		break;
	    }
	    else if (nEdge < id) {
		bot = pos;
	    }
	    else if (nEdge > id) {
		top = pos + 1;
	    }
	}
	if (top == bot) {
	    return NULL;
	}
	return &pref[pos * cwords];
    }
    else {
	findEdge.nKey = nEdge;
	ptreeEdge = avl_find(pgraph->pEdgeTree, &findEdge);
	if (ptreeEdge && ptreeEdge->pv) {
	    return ptreeEdge->pv;
	}
	return NULL;
    }
#endif
}

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
 * Edge Traversing
 */
int DGL_EDGE_T_INITIALIZE_FUNC(dglGraph_s * pGraph, dglEdgeTraverser_s * pT,
			       dglEdgePrioritizer_s * pEP)
{
#if defined(_DGL_V1)
    pGraph->iErrno = DGL_ERR_NotSupported;
    return -pGraph->iErrno;
#else
    if (pGraph->Flags & DGL_GS_FLAT) {
	if (pEP && pEP->pvAVL) {
	    if ((pT->pvAVLT = malloc(sizeof(struct avl_traverser))) == NULL) {
		pGraph->iErrno = DGL_ERR_MemoryExhausted;
		return -pGraph->iErrno;
	    }
	    avl_t_init(pT->pvAVLT, pEP->pvAVL);
	    pT->pnEdge = NULL;
	    pT->pEdgePrioritizer = pEP;
	}
	else {
	    pT->pvAVLT = NULL;
	    pT->pnEdge = NULL;
	    pT->pEdgePrioritizer = NULL;
	}
    }
    else {
	if ((pT->pvAVLT = malloc(sizeof(struct avl_traverser))) == NULL) {
	    pGraph->iErrno = DGL_ERR_MemoryExhausted;
	    return -pGraph->iErrno;
	}
	if (pEP && pEP->pvAVL) {
	    avl_t_init(pT->pvAVLT, pEP->pvAVL);
	    pT->pnEdge = NULL;
	    pT->pEdgePrioritizer = pEP;
	}
	else {
	    avl_t_init(pT->pvAVLT, pGraph->pEdgeTree);
	    pT->pnEdge = NULL;
	    pT->pEdgePrioritizer = NULL;
	}
    }
    pT->pGraph = pGraph;
    return 0;
#endif
}

void DGL_EDGE_T_RELEASE_FUNC(dglEdgeTraverser_s * pT)
{
#if defined(_DGL_V1)
    pT->pGraph->iErrno = DGL_ERR_NotSupported;
#else
    if (pT->pvAVLT)
	free(pT->pvAVLT);
    pT->pvAVLT = NULL;
    pT->pnEdge = NULL;
    pT->pEdgePrioritizer = NULL;
#endif
}

dglInt32_t *DGL_EDGE_T_FIRST_FUNC(dglEdgeTraverser_s * pT)
{
#if defined(_DGL_V1)
    pT->pGraph->iErrno = DGL_ERR_NotSupported;
    return NULL;
#else
    dglGraph_s *pG = pT->pGraph;

    pT->pnEdge = NULL;
    if (pT->pvAVLT && pT->pEdgePrioritizer) {
	dglEdgePrioritizer_s *pPri = pT->pEdgePrioritizer;
	dglTreeEdgePri32_s *pItem;

	pItem = avl_t_first(pT->pvAVLT, pPri->pvAVL);
	if (pItem) {
	    /*
	       printf("edge_t_first: cEdge=%ld\n", pItem->cnData);
	     */
	    pPri->cEdge = pItem->cnData;
	    pPri->iEdge = 0;
	    if (pPri->iEdge < pPri->cEdge) {
		pT->pnEdge =
		    DGL_GET_EDGE_FUNC(pG, pItem->pnData[pPri->iEdge]);
		pPri->iEdge++;
	    }
	}
	pPri->pEdgePri32Item = pItem;
    }
    else if (pT->pvAVLT) {
	dglTreeEdge_s *pEdgeItem;

	if ((pEdgeItem = avl_t_first(pT->pvAVLT, pG->pEdgeTree)) == NULL) {
	    pT->pnEdge = NULL;
	}
	else {
	    pT->pnEdge = pEdgeItem->pv;
	}
    }
    else {
	if (pG->cEdge > 0)
	    pT->pnEdge = (dglInt32_t *) pG->pEdgeBuffer;
	else
	    pT->pnEdge = NULL;
    }
    return pT->pnEdge;
#endif
}

dglInt32_t *DGL_EDGE_T_NEXT_FUNC(dglEdgeTraverser_s * pT)
{
#if defined(_DGL_V1)
    pT->pGraph->iErrno = DGL_ERR_NotSupported;
    return NULL;
#else
    dglGraph_s *pG = pT->pGraph;

    pT->pnEdge = NULL;

    if (pT->pvAVLT && pT->pEdgePrioritizer) {
	dglEdgePrioritizer_s *pPri = pT->pEdgePrioritizer;
	dglTreeEdgePri32_s *pItem = pPri->pEdgePri32Item;

	if (pItem && pPri->iEdge < pPri->cEdge) {
	    pT->pnEdge = DGL_GET_EDGE_FUNC(pG, pItem->pnData[pPri->iEdge]);
	    pPri->iEdge++;
	}
	else {
	    if ((pItem = avl_t_next(pT->pvAVLT)) != NULL) {
		pPri->cEdge = pItem->cnData;
		pPri->iEdge = 0;
		if (pPri->iEdge < pPri->cEdge) {
		    pT->pnEdge =
			DGL_GET_EDGE_FUNC(pG, pItem->pnData[pPri->iEdge]);
		    pPri->iEdge++;
		}
	    }
	    pPri->pEdgePri32Item = pItem;
	}
    }
    else if (pT->pvAVLT) {
	dglTreeEdge_s *pItem;

	if ((pItem = avl_t_next(pT->pvAVLT)) != NULL) {
	    pT->pnEdge = pItem->pv;
	}
    }
    else {
	pT->pnEdge += DGL_NODE_WSIZE(pG->EdgeAttrSize);
	if (pT->pnEdge >= (dglInt32_t *) (pG->pEdgeBuffer + pG->iEdgeBuffer)) {
	    pT->pnEdge = NULL;
	}
    }
    return pT->pnEdge;
#endif
}


/*
 * Node Traversing
 */
int DGL_NODE_T_INITIALIZE_FUNC(dglGraph_s * pGraph, dglNodeTraverser_s * pT)
{
    if (pGraph->Flags & DGL_GS_FLAT) {
	pT->pnNode = NULL;
	pT->pvAVLT = NULL;
    }
    else {
	if ((pT->pvAVLT = malloc(sizeof(struct avl_traverser))) == NULL) {
	    pGraph->iErrno = DGL_ERR_MemoryExhausted;
	    return -pGraph->iErrno;
	}
	avl_t_init(pT->pvAVLT, pGraph->pNodeTree);
	pT->pnNode = NULL;
    }
    pT->pGraph = pGraph;
    return 0;
}

void DGL_NODE_T_RELEASE_FUNC(dglNodeTraverser_s * pT)
{
    if (pT->pvAVLT)
	free(pT->pvAVLT);
    pT->pvAVLT = NULL;
    pT->pnNode = NULL;
}

dglInt32_t *DGL_NODE_T_FIRST_FUNC(dglNodeTraverser_s * pT)
{
    DGL_T_NODEITEM_TYPE *pNodeItem;

    if (pT->pvAVLT) {
	if ((pNodeItem =
	     avl_t_first(pT->pvAVLT, pT->pGraph->pNodeTree)) == NULL)
	    pT->pnNode = NULL;
	else
	    pT->pnNode = DGL_T_NODEITEM_NodePTR(pNodeItem);
    }
    else {
	if (pT->pGraph->cNode > 0)
	    pT->pnNode = (dglInt32_t *) pT->pGraph->pNodeBuffer;
	else
	    pT->pnNode = NULL;
    }
    return pT->pnNode;
}

dglInt32_t *DGL_NODE_T_NEXT_FUNC(dglNodeTraverser_s * pT)
{
    DGL_T_NODEITEM_TYPE *pNodeItem;

    if (pT->pvAVLT) {
	if ((pNodeItem = avl_t_next(pT->pvAVLT)) == NULL)
	    pT->pnNode = NULL;
	else
	    pT->pnNode = DGL_T_NODEITEM_NodePTR(pNodeItem);
    }
    else {
	pT->pnNode += DGL_NODE_WSIZE(pT->pGraph->NodeAttrSize);
	if (pT->pnNode >=
	    (dglInt32_t *) (pT->pGraph->pNodeBuffer +
			    pT->pGraph->iNodeBuffer))
	    pT->pnNode = NULL;
    }
    return pT->pnNode;
}

dglInt32_t *DGL_NODE_T_FIND_FUNC(dglNodeTraverser_s * pT, dglInt32_t nNodeId)
{
    DGL_T_NODEITEM_TYPE *pNodeItem, findItem;

    if (pT->pvAVLT) {
	findItem.nKey = nNodeId;
	if ((pNodeItem =
	     avl_t_find(pT->pvAVLT, pT->pGraph->pNodeTree,
			&findItem)) == NULL)
	    pT->pnNode = NULL;
	else
	    pT->pnNode = DGL_T_NODEITEM_NodePTR(pNodeItem);
    }
    else {
	pT->pnNode = DGL_GET_NODE_FUNC(pT->pGraph, nNodeId);
    }
    return pT->pnNode;
}


/*
 * Edgeset Traversing
 */
int DGL_EDGESET_T_INITIALIZE_FUNC(dglGraph_s * pGraph,
				  dglEdgesetTraverser_s * pT,
				  dglInt32_t * pnEdgeset)
{
    pT->pGraph = pGraph;
    pT->pnEdgeset = pnEdgeset;
    pT->cEdge = (pnEdgeset) ? *pnEdgeset : 0;
    pT->iEdge = 0;
    return 0;
}


void DGL_EDGESET_T_RELEASE_FUNC(dglEdgesetTraverser_s * pT)
{
}

dglInt32_t *DGL_EDGESET_T_FIRST_FUNC(dglEdgesetTraverser_s * pT)
{
#if defined(_DGL_V2)
    dglInt32_t *pnOffset;
    dglTreeEdge_s *pEdgeItem, EdgeItem;
#endif

    if (pT->cEdge == 0)
	return NULL;
    pT->iEdge = 1;
#if defined(_DGL_V1)
    return pT->pnEdgeset + 1;
#endif
#if defined(_DGL_V2)
    pnOffset = pT->pnEdgeset + 1;
    if (pT->pGraph->Flags & DGL_GS_FLAT) {
	pT->pvCurrentItem = NULL;
	return DGL_EDGEBUFFER_SHIFT(pT->pGraph, *pnOffset);
    }
    else {
	EdgeItem.nKey = *pnOffset;
	if ((pEdgeItem = avl_find(pT->pGraph->pEdgeTree, &EdgeItem)) != NULL) {
	    pT->pvCurrentItem = pEdgeItem;
	    return pEdgeItem->pv;
	}
    }
#endif
    return NULL;
}


dglInt32_t *DGL_EDGESET_T_NEXT_FUNC(dglEdgesetTraverser_s * pT)
{
#if defined(_DGL_V2)
    dglInt32_t *pnOffset;
    dglTreeEdge_s *pEdgeItem, EdgeItem;
#endif

    if (pT->cEdge > 0 && pT->iEdge < pT->cEdge) {
#if defined(_DGL_V1)
	return DGL_EDGESET_EDGE_PTR(pT->pnEdgeset, pT->iEdge++,
				    pT->pGraph->EdgeAttrSize);
#endif
#if defined(_DGL_V2)
	pnOffset = pT->pnEdgeset + 1 + pT->iEdge++;
	if (pT->pGraph->Flags & DGL_GS_FLAT) {
	    return DGL_EDGEBUFFER_SHIFT(pT->pGraph, *pnOffset);
	}
	else {
	    EdgeItem.nKey = *pnOffset;
	    if ((pEdgeItem =
		 avl_find(pT->pGraph->pEdgeTree, &EdgeItem)) != NULL) {
		pT->pvCurrentItem = pEdgeItem;
		return pEdgeItem->pv;
	    }
	}
#endif
    }
    return NULL;
}


/*
 * Flatten the graph
 */
int DGL_FLATTEN_FUNC(dglGraph_s * pgraph)
{
    register DGL_T_NODEITEM_TYPE *ptreenode;

#if defined(_DGL_V2)
    register dglTreeEdge_s *ptreeEdge;
    int i;
#endif
    register dglInt32_t *pEdge;
    register dglInt32_t *pnode;
    register dglInt32_t *pnodescan;
    dglInt32_t *pOutEdgeset;
    dglInt32_t *pInEdgeset;
    int cOutEdgeset;
    int cInEdgeset;

    struct avl_traverser avlTraverser;

    if (pgraph->Flags & DGL_GS_FLAT) {
	pgraph->iErrno = DGL_ERR_BadOnFlatGraph;
	return -pgraph->iErrno;
    }

    pgraph->pNodeBuffer = NULL;	/* should be already */
    pgraph->iNodeBuffer = 0;
    pgraph->pEdgeBuffer = NULL;
    pgraph->iEdgeBuffer = 0;


#if defined(_DGL_V2)
    /*
       printf("flatten: traversing edges\n");
     */
    avl_t_init(&avlTraverser, pgraph->pEdgeTree);

    for (ptreeEdge = avl_t_first(&avlTraverser, pgraph->pEdgeTree);
	 ptreeEdge; ptreeEdge = avl_t_next(&avlTraverser)
	) {
	pEdge = ptreeEdge->pv;

	/*
	   printf( "flatten: add edge %ld to edge buffer\n", DGL_EDGE_ID(pEdge) );
	 */

	pgraph->pEdgeBuffer = realloc(pgraph->pEdgeBuffer,
				      pgraph->iEdgeBuffer +
				      DGL_EDGE_SIZEOF(pgraph->EdgeAttrSize)
	    );

	if (pgraph->pEdgeBuffer == NULL) {
	    pgraph->iErrno = DGL_ERR_MemoryExhausted;
	    return -pgraph->iErrno;
	}

	memcpy(pgraph->pEdgeBuffer + pgraph->iEdgeBuffer, pEdge,
	       DGL_EDGE_SIZEOF(pgraph->EdgeAttrSize));

	pgraph->iEdgeBuffer += DGL_EDGE_SIZEOF(pgraph->EdgeAttrSize);
    }
#endif

    /*
       printf("flatten: traversing nodes\n");
     */
    avl_t_init(&avlTraverser, pgraph->pNodeTree);

    for (ptreenode = avl_t_first(&avlTraverser, pgraph->pNodeTree);
	 ptreenode; ptreenode = avl_t_next(&avlTraverser)
	) {
	pnode = DGL_T_NODEITEM_NodePTR(ptreenode);
	pOutEdgeset = DGL_T_NODEITEM_OutEdgesetPTR(ptreenode);
	pInEdgeset = DGL_T_NODEITEM_InEdgesetPTR(ptreenode);

	if (!(DGL_NODE_STATUS(pnode) & DGL_NS_ALONE)) {
	    cOutEdgeset = (pOutEdgeset) ?
		DGL_EDGESET_SIZEOF(DGL_EDGESET_EDGECOUNT(pOutEdgeset),
				   pgraph->EdgeAttrSize) : sizeof(dglInt32_t);

#if !defined(_DGL_V1)
	    cInEdgeset = (pInEdgeset) ?
		DGL_EDGESET_SIZEOF(DGL_EDGESET_EDGECOUNT(pInEdgeset),
				   pgraph->EdgeAttrSize) : sizeof(dglInt32_t);
#else
	    cInEdgeset = 0;
#endif

	    pgraph->pEdgeBuffer = realloc(pgraph->pEdgeBuffer,
					  pgraph->iEdgeBuffer + cOutEdgeset +
					  cInEdgeset);

	    if (pgraph->pEdgeBuffer == NULL) {
		pgraph->iErrno = DGL_ERR_MemoryExhausted;
		return -pgraph->iErrno;
	    }

	    {
		dglInt32_t nDummy = 0;

		memcpy(pgraph->pEdgeBuffer + pgraph->iEdgeBuffer,
		       (pOutEdgeset) ? pOutEdgeset : &nDummy, cOutEdgeset);
#if !defined(_DGL_V1)
		memcpy(pgraph->pEdgeBuffer + pgraph->iEdgeBuffer +
		       cOutEdgeset, (pInEdgeset) ? pInEdgeset : &nDummy,
		       cInEdgeset);
#endif
	    }

	    DGL_NODE_EDGESET_OFFSET(pnode) = pgraph->iEdgeBuffer;

	    pgraph->iEdgeBuffer += cOutEdgeset + cInEdgeset;
	}

	pgraph->pNodeBuffer =
	    realloc(pgraph->pNodeBuffer,
		    pgraph->iNodeBuffer +
		    DGL_NODE_SIZEOF(pgraph->NodeAttrSize));

	if (pgraph->pNodeBuffer == NULL) {
	    pgraph->iErrno = DGL_ERR_MemoryExhausted;
	    return -pgraph->iErrno;
	}

	memcpy(pgraph->pNodeBuffer + pgraph->iNodeBuffer, pnode,
	       DGL_NODE_SIZEOF(pgraph->NodeAttrSize));
	pgraph->iNodeBuffer += DGL_NODE_SIZEOF(pgraph->NodeAttrSize);
    }

#if defined(_DGL_V2)
    if (pgraph->pEdgeTree) {
	avl_destroy(pgraph->pEdgeTree, dglTreeEdgeCancel);
	pgraph->pEdgeTree = NULL;
    }
#endif

    if (pgraph->pNodeTree) {
	avl_destroy(pgraph->pNodeTree, dglTreeNodeCancel);
	pgraph->pNodeTree = NULL;
    }

    pgraph->Flags |= DGL_GS_FLAT;	/* flattened */

    /*
     * convert node-id to node-offset
     */
    DGL_FOREACH_NODE(pgraph, pnodescan) {
	if (!(DGL_NODE_STATUS(pnodescan) & DGL_NS_ALONE)) {
	    pOutEdgeset =
		DGL_EDGEBUFFER_SHIFT(pgraph,
				     DGL_NODE_EDGESET_OFFSET(pnodescan));

#if defined(_DGL_V2)
	    for (i = 0; i < pOutEdgeset[0]; i++) {
		/*
		   printf("flatten: node %ld: scan out edge %ld/%ld - %ld\n", DGL_NODE_ID(pnodescan), i+1, pOutEdgeset[0], pOutEdgeset[i+1]);
		 */
		pEdge = DGL_GET_EDGE_FUNC(pgraph, pOutEdgeset[i + 1]);
		if (pEdge == NULL) {
		    pgraph->iErrno = DGL_ERR_UnexpectedNullPointer;
		    return -pgraph->iErrno;
		}
		/*
		   printf("         retrieved id %ld\n", DGL_EDGE_ID(pEdge) );
		 */
		pOutEdgeset[i + 1] = DGL_EDGEBUFFER_OFFSET(pgraph, pEdge);
	    }

	    pInEdgeset = pOutEdgeset + pOutEdgeset[0] + 1;

	    for (i = 0; i < pInEdgeset[0]; i++) {
		/*
		   printf("flatten: node %ld: scan in edge %ld/%ld - %ld\n",
		   DGL_NODE_ID(pnodescan), i+1, pInEdgeset[0], pInEdgeset[i+1]);
		 */
		pEdge = DGL_GET_EDGE_FUNC(pgraph, pInEdgeset[i + 1]);
		if (pEdge == NULL) {
		    pgraph->iErrno = DGL_ERR_UnexpectedNullPointer;
		    return -pgraph->iErrno;
		}
		/*
		   printf("         retrieved id %ld\n", DGL_EDGE_ID(pEdge) );
		 */
		pInEdgeset[i + 1] = DGL_EDGEBUFFER_OFFSET(pgraph, pEdge);
	    }
#endif

#if defined(_DGL_V2)
	    {
		int iEdge;

		DGL_FOREACH_EDGE(pgraph, pOutEdgeset, pEdge, iEdge)
#else
	    DGL_FOREACH_EDGE(pgraph, pOutEdgeset, pEdge)
#endif
	    {
		if ((pnode =
		     DGL_GET_NODE_FUNC(pgraph,
				       DGL_EDGE_HEADNODE_OFFSET(pEdge))) ==
		    NULL) {
		    pgraph->iErrno = DGL_ERR_HeadNodeNotFound;
		    return -pgraph->iErrno;
		}
		DGL_EDGE_HEADNODE_OFFSET(pEdge) =
		    DGL_NODEBUFFER_OFFSET(pgraph, pnode);

		if ((pnode =
		     DGL_GET_NODE_FUNC(pgraph,
				       DGL_EDGE_TAILNODE_OFFSET(pEdge))) ==
		    NULL) {
		    pgraph->iErrno = DGL_ERR_TailNodeNotFound;
		    return -pgraph->iErrno;
		}
		DGL_EDGE_TAILNODE_OFFSET(pEdge) =
		    DGL_NODEBUFFER_OFFSET(pgraph, pnode);
	    }
#if defined(_DGL_V2)
	}
#endif
    }
}

return 0;
}


int DGL_UNFLATTEN_FUNC(dglGraph_s * pgraph)
{
    register dglInt32_t *pHead;
    register dglInt32_t *pTail;
    register dglInt32_t *pEdge;
    register dglInt32_t *pEdgeset;
    int nret;

    if (!(pgraph->Flags & DGL_GS_FLAT)) {
	pgraph->iErrno = DGL_ERR_BadOnTreeGraph;
	return -pgraph->iErrno;
    }

    /*
     * unflag it now to avoid DGL_ADD_EDGE_FUNC() failure
     */
    pgraph->Flags &= ~DGL_GS_FLAT;
    pgraph->cNode = 0;
    pgraph->cEdge = 0;
    pgraph->cHead = 0;
    pgraph->cTail = 0;
    pgraph->cAlone = 0;
    pgraph->nnCost = (dglInt64_t) 0;

    if (pgraph->pNodeTree == NULL)
	pgraph->pNodeTree =
	    avl_create(DGL_T_NODEITEM_Compare, NULL, dglTreeGetAllocator());
    if (pgraph->pNodeTree == NULL) {
	pgraph->iErrno = DGL_ERR_MemoryExhausted;
	return -pgraph->iErrno;
    }
#if defined(_DGL_V1)
    pgraph->pEdgeTree = NULL;
#else
    if (pgraph->pEdgeTree == NULL)
	pgraph->pEdgeTree =
	    avl_create(dglTreeEdgeCompare, NULL, dglTreeGetAllocator());
    if (pgraph->pEdgeTree == NULL) {
	pgraph->iErrno = DGL_ERR_MemoryExhausted;
	return -pgraph->iErrno;
    }
#endif

    DGL_FOREACH_NODE(pgraph, pHead) {
	if (DGL_NODE_STATUS(pHead) & DGL_NS_HEAD) {
	    pEdgeset =
		DGL_EDGEBUFFER_SHIFT(pgraph, DGL_NODE_EDGESET_OFFSET(pHead));

#if defined(_DGL_V2)
	    {
		int iEdge;

		DGL_FOREACH_EDGE(pgraph, pEdgeset, pEdge, iEdge)
#else
	    DGL_FOREACH_EDGE(pgraph, pEdgeset, pEdge)
#endif
	    {
		pTail =
		    DGL_NODEBUFFER_SHIFT(pgraph,
					 DGL_EDGE_TAILNODE_OFFSET(pEdge));

		nret = DGL_ADD_EDGE_FUNC(pgraph,
					 DGL_NODE_ID(pHead),
					 DGL_NODE_ID(pTail),
					 DGL_EDGE_COST(pEdge),
					 DGL_EDGE_ID(pEdge),
					 DGL_NODE_ATTR_PTR(pHead),
					 DGL_NODE_ATTR_PTR(pTail),
					 DGL_EDGE_ATTR_PTR(pEdge), 0);

		if (nret < 0) {
		    goto error;
		}
	    }
#if defined(_DGL_V2)
	}
#endif
    }
    else
if (DGL_NODE_STATUS(pHead) & DGL_NS_ALONE) {
    nret = DGL_ADD_NODE_FUNC(pgraph,
			     DGL_NODE_ID(pHead), DGL_NODE_ATTR_PTR(pHead), 0);
    if (nret < 0) {
	goto error;
    }
}
}

	/* move away flat-state data
	 */
if (pgraph->pNodeBuffer)
    free(pgraph->pNodeBuffer);
if (pgraph->pEdgeBuffer)
    free(pgraph->pEdgeBuffer);
pgraph->pNodeBuffer = NULL;
pgraph->pEdgeBuffer = NULL;
return 0;

error:
if (pgraph->pNodeTree)
    avl_destroy(pgraph->pNodeTree, dglTreeNodeCancel);
if (pgraph->pEdgeTree)
    avl_destroy(pgraph->pEdgeTree, dglTreeEdgeCancel);
pgraph->pNodeTree = NULL;
pgraph->pEdgeTree = NULL;
pgraph->Flags |= DGL_GS_FLAT;
return nret;
}

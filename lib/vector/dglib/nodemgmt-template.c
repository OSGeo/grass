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

int DGL_ADD_NODE_FUNC(dglGraph_s * pgraph,
		      dglInt32_t nId, void *pvNodeAttr, dglInt32_t nFlags)
{
    DGL_T_NODEITEM_TYPE *pNodeItem;
    dglInt32_t *pnode;

    if (pgraph->Flags & DGL_GS_FLAT) {
	pgraph->iErrno = DGL_ERR_BadOnFlatGraph;
	return -pgraph->iErrno;
    }

    if ((pNodeItem = DGL_T_NODEITEM_Add(pgraph->pNodeTree, nId)) == NULL) {
	pgraph->iErrno = DGL_ERR_MemoryExhausted;
	return -pgraph->iErrno;
    }

    if (DGL_T_NODEITEM_NodePTR(pNodeItem) == NULL) {
	if ((pnode = DGL_NODE_ALLOC(pgraph->NodeAttrSize)) == NULL) {
	    pgraph->iErrno = DGL_ERR_MemoryExhausted;
	    return -pgraph->iErrno;
	}
	memset(pnode, 0, DGL_NODE_SIZEOF(pgraph->NodeAttrSize));
	DGL_NODE_ID(pnode) = nId;
	DGL_NODE_STATUS(pnode) = DGL_NS_ALONE;
	DGL_T_NODEITEM_Set_NodePTR(pNodeItem, pnode);
	pgraph->cNode++;
	pgraph->cAlone++;
    }
    else {
	/* node already exists */
	pgraph->iErrno = DGL_ERR_NodeAlreadyExist;
	return -pgraph->iErrno;
    }
    return 0;
}

#if !defined(_DGL_V1)
/*
 * Delete the link from the node's out-edgeset
 */
int DGL_DEL_NODE_OUTEDGE_FUNC(dglGraph_s * pgraph, dglInt32_t nNode,
			      dglInt32_t nEdge)
{
    DGL_T_NODEITEM_TYPE *pNodeItem, findNodeItem;
    dglInt32_t *pnEdgeset, *pnEdge, *pnNode;
    dglEdgesetTraverser_s t;

    findNodeItem.nKey = nNode;

    if ((pNodeItem = avl_find(pgraph->pNodeTree, &findNodeItem)) != NULL) {
	pnNode = DGL_T_NODEITEM_NodePTR(pNodeItem);
	if (DGL_NODE_STATUS(pnNode) == DGL_NS_ALONE) {
	    return 0;
	}
	if ((pnEdgeset = DGL_T_NODEITEM_OutEdgesetPTR(pNodeItem)) != NULL) {
	    if (DGL_EDGESET_T_INITIALIZE_FUNC(pgraph, &t, pnEdgeset) >= 0) {
		for (pnEdge = DGL_EDGESET_T_FIRST_FUNC(&t);
		     pnEdge; pnEdge = DGL_EDGESET_T_NEXT_FUNC(&t)
		    ) {
		    if (DGL_EDGE_ID(pnEdge) == nEdge) {
			register dglInt32_t *pnSet;
			register int i1, i2, c;

			c = pnEdgeset[0];

			if ((pnSet =
			     malloc(sizeof(dglInt32_t) * (c + 1))) == NULL) {
			    pgraph->iErrno = DGL_ERR_MemoryExhausted;
			    return -pgraph->iErrno;
			}

			for (i1 = 0, i2 = 0; i2 < c; i2++) {
			    if (pnEdgeset[1 + i2] != nEdge) {
				pnSet[1 + i1++] = pnEdgeset[1 + i2];
			    }
			}
			pnSet[0] = i1;

			free(pnEdgeset);
			DGL_T_NODEITEM_Set_OutEdgesetPTR(pNodeItem, pnSet);
			break;
		    }
		}
	    }
	}
	{			/* check alone status */
	    dglInt32_t *pIn, *pOut, *pNode;

	    pOut = DGL_T_NODEITEM_OutEdgesetPTR(pNodeItem);
	    pIn = DGL_T_NODEITEM_InEdgesetPTR(pNodeItem);
	    pNode = DGL_T_NODEITEM_NodePTR(pNodeItem);
	    if ((pOut == NULL || DGL_EDGESET_EDGECOUNT(pOut) == 0) &&
		(pIn == NULL || DGL_EDGESET_EDGECOUNT(pIn) == 0)
		) {
		if (DGL_NODE_STATUS(pNode) & DGL_NS_HEAD)
		    pgraph->cHead--;
		if (DGL_NODE_STATUS(pNode) & DGL_NS_TAIL)
		    pgraph->cTail--;
		DGL_NODE_STATUS(pNode) = DGL_NS_ALONE;
		pgraph->cAlone++;
	    }
	}
    }
    return 0;
}

int DGL_DEL_NODE_INEDGE_FUNC(dglGraph_s * pgraph, dglInt32_t nNode,
			     dglInt32_t nEdge)
{
    DGL_T_NODEITEM_TYPE *pNodeItem, findNodeItem;
    dglInt32_t *pnEdgeset, *pnEdge, *pnNode;
    dglEdgesetTraverser_s t;

    findNodeItem.nKey = nNode;

    if ((pNodeItem = avl_find(pgraph->pNodeTree, &findNodeItem)) != NULL) {
	pnNode = DGL_T_NODEITEM_NodePTR(pNodeItem);
	if (DGL_NODE_STATUS(pnNode) == DGL_NS_ALONE) {
	    return 0;
	}
	if ((pnEdgeset = DGL_T_NODEITEM_InEdgesetPTR(pNodeItem)) != NULL) {
	    if (DGL_EDGESET_T_INITIALIZE_FUNC(pgraph, &t, pnEdgeset) >= 0) {
		for (pnEdge = DGL_EDGESET_T_FIRST_FUNC(&t);
		     pnEdge; pnEdge = DGL_EDGESET_T_NEXT_FUNC(&t)
		    ) {
		    if (DGL_EDGE_ID(pnEdge) == nEdge) {
			register dglInt32_t *pnSet;
			register int i1, i2, c;

			c = pnEdgeset[0];

			if ((pnSet =
			     malloc(sizeof(dglInt32_t) * (c + 1))) == NULL) {
			    pgraph->iErrno = DGL_ERR_MemoryExhausted;
			    return -pgraph->iErrno;
			}

			for (i1 = 0, i2 = 0; i2 < c; i2++) {
			    if (pnEdgeset[1 + i2] != nEdge) {
				pnSet[1 + i1++] = pnEdgeset[1 + i2];
			    }
			}
			pnSet[0] = i1;

			free(pnEdgeset);
			DGL_T_NODEITEM_Set_InEdgesetPTR(pNodeItem, pnSet);
			break;
		    }
		}
	    }
	}
	{			/* check alone status */
	    dglInt32_t *pIn, *pOut, *pNode;

	    pOut = DGL_T_NODEITEM_OutEdgesetPTR(pNodeItem);
	    pIn = DGL_T_NODEITEM_InEdgesetPTR(pNodeItem);
	    pNode = DGL_T_NODEITEM_NodePTR(pNodeItem);
	    if ((pOut == NULL || DGL_EDGESET_EDGECOUNT(pOut) == 0) &&
		(pIn == NULL || DGL_EDGESET_EDGECOUNT(pIn) == 0)
		) {
		if (DGL_NODE_STATUS(pNode) & DGL_NS_HEAD)
		    pgraph->cHead--;
		if (DGL_NODE_STATUS(pNode) & DGL_NS_TAIL)
		    pgraph->cTail--;
		DGL_NODE_STATUS(pNode) = DGL_NS_ALONE;
		pgraph->cAlone++;
	    }
	}
    }
    return 0;
}
#endif

int DGL_DEL_NODE_FUNC(dglGraph_s * pgraph, dglInt32_t nNodeId)
{
#if defined(_DGL_V1)
    pgraph->iErrno = DGL_ERR_NotSupported;
    return -pgraph->iErrno;
#else
    DGL_T_NODEITEM_TYPE *pNodeItem, findNodeItem;
    dglInt32_t *pEdgeset;
    dglInt32_t *pnode;
    dglInt32_t *pEdge;
    dglEdgesetTraverser_s trav;

    dglTreeEdge_s *pEdgeItem;

    if (pgraph->Flags & DGL_GS_FLAT) {
	pgraph->iErrno = DGL_ERR_BadOnFlatGraph;
	return -pgraph->iErrno;
    }

    if (pgraph->pNodeTree == NULL) {
	pgraph->iErrno = DGL_ERR_UnexpectedNullPointer;
	return -pgraph->iErrno;
    }

    findNodeItem.nKey = nNodeId;
    if ((pNodeItem = avl_find(pgraph->pNodeTree, &findNodeItem)) == NULL) {
	pgraph->iErrno = DGL_ERR_NodeNotFound;
	return -pgraph->iErrno;
    }

    pnode = DGL_T_NODEITEM_NodePTR(pNodeItem);

    if (DGL_NODE_STATUS(pnode) & DGL_NS_ALONE)
	goto node_is_alone;

    pEdgeset = DGL_T_NODEITEM_OutEdgesetPTR(pNodeItem);

    if (DGL_EDGESET_T_INITIALIZE_FUNC(pgraph, &trav, pEdgeset) < 0)
	return -pgraph->iErrno;
    for (pEdge = DGL_EDGESET_T_FIRST_FUNC(&trav);
	 pEdge; pEdge = DGL_EDGESET_T_NEXT_FUNC(&trav)
	) {
	if (DGL_EDGE_TAILNODE_OFFSET(pEdge) != DGL_NODE_ID(pnode)) {
	    if (DGL_DEL_NODE_INEDGE_FUNC
		(pgraph, DGL_EDGE_TAILNODE_OFFSET(pEdge),
		 DGL_EDGE_ID(pEdge)) < 0) {
		return -pgraph->iErrno;
	    }
	}
	if ((pEdgeItem = trav.pvCurrentItem) != NULL) {
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
	}
    }
    DGL_EDGESET_T_RELEASE_FUNC(&trav);

    pEdgeset = DGL_T_NODEITEM_InEdgesetPTR(pNodeItem);

    if (DGL_EDGESET_T_INITIALIZE_FUNC(pgraph, &trav, pEdgeset) < 0)
	return -pgraph->iErrno;
    for (pEdge = DGL_EDGESET_T_FIRST_FUNC(&trav);
	 pEdge; pEdge = DGL_EDGESET_T_NEXT_FUNC(&trav)
	) {
	if (DGL_EDGE_HEADNODE_OFFSET(pEdge) != DGL_NODE_ID(pnode)) {
	    if (DGL_DEL_NODE_OUTEDGE_FUNC
		(pgraph, DGL_EDGE_HEADNODE_OFFSET(pEdge),
		 DGL_EDGE_ID(pEdge)) < 0) {
		return -pgraph->iErrno;
	    }
	}
	if ((pEdgeItem = trav.pvCurrentItem) != NULL) {
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
	}
    }
    DGL_EDGESET_T_RELEASE_FUNC(&trav);

    if (DGL_NODE_STATUS(pnode) & DGL_NS_HEAD)
	pgraph->cHead--;
    if (DGL_NODE_STATUS(pnode) & DGL_NS_TAIL)
	pgraph->cTail--;

  node_is_alone:
    if (DGL_NODE_STATUS(pnode) & DGL_NS_ALONE)
	pgraph->cAlone--;
    pgraph->cNode--;

    avl_delete(pgraph->pNodeTree, pNodeItem);
    DGL_T_NODEITEM_Cancel(pNodeItem, NULL);

    return 0;
#endif
}

dglInt32_t *DGL_GET_NODE_FUNC(dglGraph_s * pgraph, dglInt32_t nodeid)
{
    register dglInt32_t top;	/* top of table */
    register dglInt32_t pos;	/* current position to compare */
    register dglInt32_t bot;	/* bottom of table */
    register dglInt32_t *pref;
    register int cwords;	/* size of a node in words of 32 bit */
    register dglTreeNode_s *ptreenode;
    dglTreeNode_s findnode;
    dglInt32_t id;

    pgraph->iErrno = 0;
    if (pgraph->Flags & DGL_GS_FLAT) {
	cwords = DGL_NODE_WSIZE(pgraph->NodeAttrSize);
	/*bot    = pgraph->iNodeBuffer / DGL_NODE_SIZEOF(pgraph->NodeAttrSize); */
	bot = pgraph->cNode;
	top = 0;
	pos = 0;
	pref = (dglInt32_t *) pgraph->pNodeBuffer;

	/* perform a binary search
	 */
	while (top != bot) {
	    pos = top + (bot - top) / 2;
	    id = DGL_NODE_ID(&pref[pos * cwords]);
	    if (id == nodeid) {
		break;
	    }
	    else if (nodeid < id) {
		bot = pos;
	    }
	    else if (nodeid > id) {
		top = pos + 1;
	    }
	}
	if (top == bot) {
	    return NULL;
	}
	return &pref[pos * cwords];
    }
    else {
	findnode.nKey = nodeid;
	ptreenode = avl_find(pgraph->pNodeTree, &findnode);
	if (ptreenode && ptreenode->pv) {
	    return ptreenode->pv;
	}
	return NULL;
    }
}

/*
 * if graph is FLAT retrieve the edge area from the pEdgeBuffer
 * if graph is TREE retrieve the node from the pNodeTree avl and return pv field
 */
dglInt32_t *DGL_GET_NODE_OUTEDGESET_FUNC(dglGraph_s * pgraph,
					 dglInt32_t * pnode)
{
    DGL_T_NODEITEM_TYPE *ptreenode, findnode;
    dglInt32_t *pEdgeset;

    pgraph->iErrno = 0;

    if (pnode == NULL) {
	pgraph->iErrno = DGL_ERR_UnexpectedNullPointer;
	return NULL;
    }

    if (DGL_NODE_STATUS(pnode) & DGL_NS_ALONE) {
	pgraph->iErrno = DGL_ERR_NodeIsAComponent;
	return NULL;
    }

    if (pgraph->Flags & DGL_GS_FLAT) {
	pEdgeset =
	    DGL_EDGEBUFFER_SHIFT(pgraph, DGL_NODE_EDGESET_OFFSET(pnode));
	return pEdgeset;
    }
    else {
	findnode.nKey = DGL_NODE_ID(pnode);
	ptreenode = avl_find(pgraph->pNodeTree, &findnode);
	if (ptreenode && DGL_T_NODEITEM_OutEdgesetPTR(ptreenode)) {
	    return DGL_T_NODEITEM_OutEdgesetPTR(ptreenode);
	}
	return NULL;
    }
}

dglInt32_t *DGL_GET_NODE_INEDGESET_FUNC(dglGraph_s * pgraph,
					dglInt32_t * pnode)
{
#if defined(_DGL_V1)
    pgraph->iErrno = DGL_ERR_NotSupported;
    return NULL;
#endif

#if defined(_DGL_V2)
    DGL_T_NODEITEM_TYPE *ptreenode, findnode;
    dglInt32_t *pEdgeset;

    pgraph->iErrno = 0;

    if (pnode == NULL) {
	pgraph->iErrno = DGL_ERR_UnexpectedNullPointer;
	return NULL;
    }

    if (DGL_NODE_STATUS(pnode) & DGL_NS_ALONE) {
	pgraph->iErrno = DGL_ERR_NodeIsAComponent;
	return NULL;
    }

    if (pgraph->Flags & DGL_GS_FLAT) {
	pEdgeset =
	    DGL_EDGEBUFFER_SHIFT(pgraph, DGL_NODE_EDGESET_OFFSET(pnode));
	pEdgeset +=
	    DGL_EDGESET_WSIZE(DGL_EDGESET_EDGECOUNT(pEdgeset),
			      pgraph->EdgeAttrSize);
	return pEdgeset;
    }
    else {
	findnode.nKey = DGL_NODE_ID(pnode);
	ptreenode = avl_find(pgraph->pNodeTree, &findnode);
	if (ptreenode && DGL_T_NODEITEM_InEdgesetPTR(ptreenode)) {
	    return DGL_T_NODEITEM_InEdgesetPTR(ptreenode);
	}
	return NULL;
    }
#endif
}

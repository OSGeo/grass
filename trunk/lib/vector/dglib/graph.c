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

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#define DGL_V2 1

#include "type.h"
#include "tree.h"
#include "graph.h"
#include "graph_v1.h"
#if defined(DGL_V2)
#include "graph_v2.h"
#endif
#include "helpers.h"


void dglResetStats(dglGraph_s * pgraph)
{
#ifdef DGL_STATS
    pgraph->clkAddEdge = 0;
    pgraph->cAddEdge = 0;
    pgraph->clkNodeTree = 0;
    pgraph->cNodeTree = 0;
#endif
}

int dglInitialize(dglGraph_s * pGraph, dglByte_t Version,
		  dglInt32_t NodeAttrSize, dglInt32_t EdgeAttrSize,
		  dglInt32_t * pOpaqueSet)
{
    if (pGraph == NULL) {
	return -DGL_ERR_BadArgument;
    }
    switch (Version) {
    case 1:
#ifdef DGL_V2
    case 2:
    case 3:
#endif
	memset(pGraph, 0, sizeof(dglGraph_s));
	/*
	 * round attr size to the upper multiple of dglInt32_t size
	 */
	if (NodeAttrSize % sizeof(dglInt32_t))
	    NodeAttrSize +=
		(sizeof(dglInt32_t) - (NodeAttrSize % sizeof(dglInt32_t)));
	if (EdgeAttrSize % sizeof(dglInt32_t))
	    EdgeAttrSize +=
		(sizeof(dglInt32_t) - (EdgeAttrSize % sizeof(dglInt32_t)));
	pGraph->Version = Version;
	pGraph->NodeAttrSize = NodeAttrSize;
	pGraph->EdgeAttrSize = EdgeAttrSize;
	if (pOpaqueSet)
	    memcpy(&pGraph->aOpaqueSet, pOpaqueSet, sizeof(dglInt32_t) * 16);
#ifdef DGL_ENDIAN_BIG
	pGraph->Endian = DGL_ENDIAN_BIG;
#else
	pGraph->Endian = DGL_ENDIAN_LITTLE;
#endif
    }
    switch (Version) {
    case 1:
	if (dgl_initialize_V1(pGraph) < 0) {
	    return -pGraph->iErrno;
	}
	else
	    return 0;
#ifdef DGL_V2
    case 2:
    case 3:
	if (dgl_initialize_V2(pGraph) < 0) {
	    return -pGraph->iErrno;
	}
	else
	    return 0;
#endif
    }
    pGraph->iErrno = DGL_ERR_VersionNotSupported;
    return -pGraph->iErrno;
}

int dglRelease(dglGraph_s * pGraph)
{
    switch (pGraph->Version) {
    case 1:
	return dgl_release_V1(pGraph);
#ifdef DGL_V2
    case 2:
    case 3:
	return dgl_release_V2(pGraph);
#endif
    }
    pGraph->iErrno = DGL_ERR_BadVersion;
    return -pGraph->iErrno;
}

int dglUnflatten(dglGraph_s * pGraph)
{
    switch (pGraph->Version) {
    case 1:
	return dgl_unflatten_V1(pGraph);
#ifdef DGL_V2
    case 2:
    case 3:
	return dgl_unflatten_V2(pGraph);
#endif
    }
    pGraph->iErrno = DGL_ERR_BadVersion;
    return -pGraph->iErrno;
}


int dglFlatten(dglGraph_s * pGraph)
{
    switch (pGraph->Version) {
    case 1:
	return dgl_flatten_V1(pGraph);
#ifdef DGL_V2
    case 2:
    case 3:
	return dgl_flatten_V2(pGraph);
#endif
    }
    pGraph->iErrno = DGL_ERR_BadVersion;
    return -pGraph->iErrno;
}


dglInt32_t *dglGetNode(dglGraph_s * pGraph, dglInt32_t nNodeId)
{
    switch (pGraph->Version) {
    case 1:
	return dgl_get_node_V1(pGraph, nNodeId);
#ifdef DGL_V2
    case 2:
    case 3:
	return dgl_get_node_V2(pGraph, nNodeId);
#endif
    }
    pGraph->iErrno = DGL_ERR_BadVersion;
    return NULL;
}

dglInt32_t *dglNodeGet_OutEdgeset(dglGraph_s * pGraph, dglInt32_t * pnNode)
{
    if (pnNode) {
	switch (pGraph->Version) {
	case 1:
	    return dgl_getnode_outedgeset_V1(pGraph, pnNode);
#ifdef DGL_V2
	case 2:
	case 3:
	    return dgl_getnode_outedgeset_V2(pGraph, pnNode);
#endif
	}
	pGraph->iErrno = DGL_ERR_BadVersion;
	return NULL;
    }
    return NULL;
}

dglInt32_t *dglNodeGet_InEdgeset(dglGraph_s * pGraph, dglInt32_t * pnNode)
{
    if (pnNode) {
	switch (pGraph->Version) {
	case 1:
	    pGraph->iErrno = DGL_ERR_NotSupported;
	    return NULL;
#ifdef DGL_V2
	case 2:
	case 3:
	    return dgl_getnode_inedgeset_V2(pGraph, pnNode);
#endif
	}
	pGraph->iErrno = DGL_ERR_BadVersion;
	return NULL;
    }
    return NULL;
}



/*
 * Given that node id can be negative, only iErrno can report a error,
 * thus it is initialized to zero
 */
dglInt32_t dglNodeGet_Id(dglGraph_s * pGraph, dglInt32_t * pnNode)
{
    pGraph->iErrno = 0;
    if (pnNode) {
	switch (pGraph->Version) {
	case 1:
	    return DGL_NODE_ID_v1(pnNode);
#ifdef DGL_V2
	case 2:
	case 3:
	    return DGL_NODE_ID_v2(pnNode);
#endif
	}
	pGraph->iErrno = DGL_ERR_BadVersion;
	return 0;
    }
    pGraph->iErrno = DGL_ERR_UnexpectedNullPointer;
    return 0;
}


dglInt32_t dglNodeGet_Status(dglGraph_s * pGraph, dglInt32_t * pnNode)
{
    pGraph->iErrno = 0;
    if (pnNode) {
	switch (pGraph->Version) {
	case 1:
	    return DGL_NODE_STATUS_v1(pnNode);
#ifdef DGL_V2
	case 2:
	case 3:
	    return DGL_NODE_STATUS_v2(pnNode);
#endif
	}
	pGraph->iErrno = DGL_ERR_BadVersion;
	return 0;
    }
    pGraph->iErrno = DGL_ERR_UnexpectedNullPointer;
    return 0;
}


dglInt32_t *dglNodeGet_Attr(dglGraph_s * pGraph, dglInt32_t * pnNode)
{
    if (pnNode) {
	switch (pGraph->Version) {
	case 1:
	    return DGL_NODE_ATTR_PTR_v1(pnNode);
#ifdef DGL_V2
	case 2:
	case 3:
	    return DGL_NODE_ATTR_PTR_v2(pnNode);
#endif
	}
	pGraph->iErrno = DGL_ERR_BadVersion;
	return NULL;
    }
    pGraph->iErrno = DGL_ERR_UnexpectedNullPointer;
    return NULL;
}


void dglNodeSet_Attr(dglGraph_s * pGraph, dglInt32_t * pnNode,
		     dglInt32_t * pnAttr)
{
    if (pnNode) {
	switch (pGraph->Version) {
	case 1:
	    memcpy(DGL_NODE_ATTR_PTR_v1(pnNode), pnAttr,
		   pGraph->NodeAttrSize);
	    return;
#ifdef DGL_V2
	case 2:
	case 3:
	    memcpy(DGL_NODE_ATTR_PTR_v2(pnNode), pnAttr,
		   pGraph->NodeAttrSize);
	    return;
#endif
	}
	return;
    }
    return;
}

int dglNodeGet_InDegree(dglGraph_s * pGraph, dglInt32_t * pnNode)
{
#ifdef DGL_V2
    dglInt32_t *pinedgeset;
#endif

    pGraph->iErrno = 0;
    if (pnNode) {
	switch (pGraph->Version) {
	case 1:
	    pGraph->iErrno = DGL_ERR_NotSupported;
	    return 0;
#ifdef DGL_V2
	case 2:
	    if (DGL_NODE_STATUS_v2(pnNode) & DGL_NS_ALONE)
		return 0;
	    pinedgeset = dglNodeGet_InEdgeset(pGraph, pnNode);
	    if (pinedgeset)
		return DGL_EDGESET_EDGECOUNT_v2(pinedgeset);
	    return 0;
	case 3:
	    return dglNodeGet_Valence(pGraph, pnNode);
#endif
	}
	pGraph->iErrno = DGL_ERR_BadVersion;
	return 0;
    }
    pGraph->iErrno = DGL_ERR_UnexpectedNullPointer;
    return 0;
}


int dglNodeGet_OutDegree(dglGraph_s * pGraph, dglInt32_t * pnNode)
{
    dglInt32_t *poutedgeset;

    pGraph->iErrno = 0;
    if (pnNode) {
	switch (pGraph->Version) {
	case 1:
	    if (DGL_NODE_STATUS_v1(pnNode) & DGL_NS_ALONE)
		return 0;
	    poutedgeset = dglNodeGet_OutEdgeset(pGraph, pnNode);
	    if (poutedgeset)
		return DGL_EDGESET_EDGECOUNT_v1(poutedgeset);
	    return 0;
#ifdef DGL_V2
	case 2:
	    if (DGL_NODE_STATUS_v2(pnNode) & DGL_NS_ALONE)
		return 0;
	    poutedgeset = dglNodeGet_OutEdgeset(pGraph, pnNode);
	    if (poutedgeset)
		return DGL_EDGESET_EDGECOUNT_v2(poutedgeset);
	    return 0;
	case 3:
	    return dglNodeGet_Valence(pGraph, pnNode);
#endif
	}
	pGraph->iErrno = DGL_ERR_BadVersion;
	return 0;
    }
    pGraph->iErrno = DGL_ERR_UnexpectedNullPointer;
    return 0;
}


int dglNodeGet_Valence(dglGraph_s * pGraph, dglInt32_t * pnNode)
{
#ifdef DGL_V2
    dglInt32_t *poutedgeset;
    dglInt32_t *pinedgeset;
    int c;
#endif

    pGraph->iErrno = 0;
    if (pnNode) {
	switch (pGraph->Version) {
#ifdef DGL_V2
	case 3:
	    if (DGL_NODE_STATUS_v2(pnNode) & DGL_NS_ALONE)
		return 0;
	    poutedgeset = dglNodeGet_OutEdgeset(pGraph, pnNode);
	    pinedgeset = dglNodeGet_InEdgeset(pGraph, pnNode);
	    c = 0;
	    if (poutedgeset)
		c += DGL_EDGESET_EDGECOUNT_v2(poutedgeset);
	    if (pinedgeset)
		c += DGL_EDGESET_EDGECOUNT_v2(pinedgeset);
	    return c;
#endif
	}
	pGraph->iErrno = DGL_ERR_BadVersion;
	return 0;
    }
    pGraph->iErrno = DGL_ERR_UnexpectedNullPointer;
    return 0;
}



dglInt32_t dglEdgesetGet_EdgeCount(dglGraph_s * pGraph,
				   dglInt32_t * pnEdgeset)
{
    pGraph->iErrno = 0;
    if (pnEdgeset) {
	switch (pGraph->Version) {
	case 1:
	    return DGL_EDGESET_EDGECOUNT_v1(pnEdgeset);
#ifdef DGL_V2
	case 2:
	case 3:
	    return DGL_EDGESET_EDGECOUNT_v2(pnEdgeset);
#endif
	}
	pGraph->iErrno = DGL_ERR_BadVersion;
	return 0;
    }
    pGraph->iErrno = DGL_ERR_UnexpectedNullPointer;
    return 0;
}

dglInt32_t dglEdgeGet_Cost(dglGraph_s * pGraph, dglInt32_t * pnEdge)
{
    pGraph->iErrno = 0;
    if (pnEdge) {
	switch (pGraph->Version) {
	case 1:
	    return DGL_EDGE_COST_v1(pnEdge);
#ifdef DGL_V2
	case 2:
	case 3:
	    return DGL_EDGE_COST_v2(pnEdge);
#endif
	}
	pGraph->iErrno = DGL_ERR_BadVersion;
	return 0;
    }
    pGraph->iErrno = DGL_ERR_UnexpectedNullPointer;
    return 0;
}

dglInt32_t dglEdgeGet_Id(dglGraph_s * pGraph, dglInt32_t * pnEdge)
{
    pGraph->iErrno = 0;
    if (pnEdge) {
	switch (pGraph->Version) {
	case 1:
	    return DGL_EDGE_ID_v1(pnEdge);
#ifdef DGL_V2
	case 2:
	case 3:
	    return DGL_EDGE_ID_v2(pnEdge);
#endif
	}
	pGraph->iErrno = DGL_ERR_BadVersion;
	return 0;
    }
    pGraph->iErrno = DGL_ERR_UnexpectedNullPointer;
    return 0;
}

dglInt32_t *dglEdgeGet_Head(dglGraph_s * pGraph, dglInt32_t * pnEdge)
{
    pGraph->iErrno = 0;
    if (pnEdge) {
	switch (pGraph->Version) {
	case 1:
	    if (pGraph->Flags & DGL_GS_FLAT) {
		return DGL_NODEBUFFER_SHIFT_v1(pGraph,
					       DGL_EDGE_HEADNODE_OFFSET_v1
					       (pnEdge));
	    }
	    else {
		return dgl_get_node_V1(pGraph,
				       DGL_EDGE_HEADNODE_OFFSET_v1(pnEdge));
	    }
#ifdef DGL_V2
	case 2:
	case 3:
	    if (pGraph->Flags & DGL_GS_FLAT) {
		return DGL_NODEBUFFER_SHIFT_v2(pGraph,
					       DGL_EDGE_HEADNODE_OFFSET_v2
					       (pnEdge));
	    }
	    else {
		return dgl_get_node_V2(pGraph,
				       DGL_EDGE_HEADNODE_OFFSET_v2(pnEdge));
	    }
#endif
	}
	pGraph->iErrno = DGL_ERR_BadVersion;
	return NULL;
    }
    pGraph->iErrno = DGL_ERR_UnexpectedNullPointer;
    return NULL;
}

dglInt32_t *dglEdgeGet_Tail(dglGraph_s * pGraph, dglInt32_t * pnEdge)
{
    pGraph->iErrno = 0;
    if (pnEdge) {
	switch (pGraph->Version) {
	case 1:
	    if (pGraph->Flags & DGL_GS_FLAT) {
		return DGL_NODEBUFFER_SHIFT_v1(pGraph,
					       DGL_EDGE_TAILNODE_OFFSET_v1
					       (pnEdge));
	    }
	    else {
		return dgl_get_node_V1(pGraph,
				       DGL_EDGE_TAILNODE_OFFSET_v1(pnEdge));
	    }
#ifdef DGL_V2
	case 2:
	case 3:
	    if (pGraph->Flags & DGL_GS_FLAT) {
		return DGL_NODEBUFFER_SHIFT_v2(pGraph,
					       DGL_EDGE_TAILNODE_OFFSET_v2
					       (pnEdge));
	    }
	    else {
		return dgl_get_node_V2(pGraph,
				       DGL_EDGE_TAILNODE_OFFSET_v2(pnEdge));
	    }
#endif
	}
	pGraph->iErrno = DGL_ERR_BadVersion;
	return NULL;
    }
    pGraph->iErrno = DGL_ERR_UnexpectedNullPointer;
    return NULL;
}

dglInt32_t *dglEdgeGet_Attr(dglGraph_s * pGraph, dglInt32_t * pnEdge)
{
    pGraph->iErrno = 0;
    if (pnEdge) {
	switch (pGraph->Version) {
	case 1:
	    return DGL_EDGE_ATTR_PTR_v1(pnEdge);
#ifdef DGL_V2
	case 2:
	case 3:
	    return DGL_EDGE_ATTR_PTR_v2(pnEdge);
#endif
	}
	pGraph->iErrno = DGL_ERR_BadVersion;
	return NULL;
    }
    pGraph->iErrno = DGL_ERR_UnexpectedNullPointer;
    return NULL;
}

int dglEdgeSet_Attr(dglGraph_s * pGraph, dglInt32_t * pnAttr,
		    dglInt32_t * pnEdge)
{
    if (pnEdge) {
	switch (pGraph->Version) {
	case 1:
	    memcpy(DGL_EDGE_ATTR_PTR_v1(pnEdge), pnAttr,
		   pGraph->EdgeAttrSize);
	    return 0;
#ifdef DGL_V2
	case 2:
	case 3:
	    memcpy(DGL_EDGE_ATTR_PTR_v2(pnEdge), pnAttr,
		   pGraph->EdgeAttrSize);
	    return 0;
#endif
	}
	pGraph->iErrno = DGL_ERR_BadVersion;
	return -pGraph->iErrno;
    }
    pGraph->iErrno = DGL_ERR_UnexpectedNullPointer;
    return -pGraph->iErrno;
}



dglInt32_t *dglGetEdge(dglGraph_s * pGraph, dglInt32_t nEdgeId)
{
    switch (pGraph->Version) {
    case 1:
	return dgl_get_edge_V1(pGraph, nEdgeId);
	break;
#ifdef DGL_V2
    case 2:
    case 3:
	return dgl_get_edge_V2(pGraph, nEdgeId);
	break;
#endif
    }
    pGraph->iErrno = DGL_ERR_BadVersion;
    return NULL;
}

int dglDelEdge(dglGraph_s * pGraph, dglInt32_t nEdgeId)
{
    switch (pGraph->Version) {
    case 1:
	return dgl_del_edge_V1(pGraph, nEdgeId);
	break;
#ifdef DGL_V2
    case 2:
    case 3:
	return dgl_del_edge_V2(pGraph, nEdgeId);
	break;
#endif
    }
    pGraph->iErrno = DGL_ERR_BadVersion;
    return -pGraph->iErrno;
}

int dglAddEdge(dglGraph_s * pGraph,
	       dglInt32_t nHead,
	       dglInt32_t nTail, dglInt32_t nCost, dglInt32_t nEdge)
{
    int nRet;

#ifdef DGL_STATS
    clock_t clk;

    clk = clock();
    pGraph->cAddEdge++;
#endif
    switch (pGraph->Version) {
    case 1:
	nRet =
	    dgl_add_edge_V1(pGraph, nHead, nTail, nCost, nEdge, NULL, NULL,
			    NULL, 0);
	break;
#ifdef DGL_V2
    case 2:
    case 3:
	nRet =
	    dgl_add_edge_V2(pGraph, nHead, nTail, nCost, nEdge, NULL, NULL,
			    NULL, 0);
	break;
#endif
    default:
	pGraph->iErrno = DGL_ERR_BadVersion;
	nRet = -pGraph->iErrno;
	break;
    }
#ifdef DGL_STATS
    pGraph->clkAddEdge += clock() - clk;
#endif
    return nRet;
}

int dglAddEdgeX(dglGraph_s * pGraph,
		dglInt32_t nHead,
		dglInt32_t nTail,
		dglInt32_t nCost,
		dglInt32_t nEdge,
		void *pvHeadAttr,
		void *pvTailAttr, void *pvEdgeAttr, dglInt32_t nFlags)
{
    int nRet;

#ifdef DGL_STATS
    clock_t clk;

    clk = clock();
    pGraph->cAddEdge++;
#endif
    switch (pGraph->Version) {
    case 1:
	nRet =
	    dgl_add_edge_V1(pGraph, nHead, nTail, nCost, nEdge, pvHeadAttr,
			    pvTailAttr, pvEdgeAttr, nFlags);
	break;
#ifdef DGL_V2
    case 2:
    case 3:
	nRet =
	    dgl_add_edge_V2(pGraph, nHead, nTail, nCost, nEdge, pvHeadAttr,
			    pvTailAttr, pvEdgeAttr, nFlags);
	break;
#endif
    default:
	pGraph->iErrno = DGL_ERR_BadVersion;
	nRet = -pGraph->iErrno;
	break;
    }
#ifdef DGL_STATS
    pGraph->clkAddEdge += clock() - clk;
#endif
    return nRet;
}

int dglAddNode(dglGraph_s * pGraph,
	       dglInt32_t nNodeId, void *pvNodeAttr, dglInt32_t nFlags)
{
    int nRet;

    switch (pGraph->Version) {
    case 1:
	nRet = dgl_add_node_V1(pGraph, nNodeId, pvNodeAttr, nFlags);
	break;
#ifdef DGL_V2
    case 2:
    case 3:
	nRet = dgl_add_node_V2(pGraph, nNodeId, pvNodeAttr, nFlags);
	break;
#endif
    default:
	pGraph->iErrno = DGL_ERR_BadVersion;
	nRet = -pGraph->iErrno;
	break;
    }
    return nRet;
}

int dglDelNode(dglGraph_s * pGraph, dglInt32_t nNodeId)
{
    int nRet;

    switch (pGraph->Version) {
    case 1:
	nRet = dgl_del_node_V1(pGraph, nNodeId);
	break;
#ifdef DGL_V2
    case 2:
    case 3:
	nRet = dgl_del_node_V2(pGraph, nNodeId);
	break;
#endif
    default:
	pGraph->iErrno = DGL_ERR_BadVersion;
	nRet = -pGraph->iErrno;
	break;
    }
    return nRet;
}

int dglWrite(dglGraph_s * pGraph, int fd)
{
    int nRet;

    switch (pGraph->Version) {
    case 1:
	nRet = dgl_write_V1(pGraph, fd);
	break;
#ifdef DGL_V2
    case 2:
    case 3:
	nRet = dgl_write_V2(pGraph, fd);
	break;
#endif
    default:
	pGraph->iErrno = DGL_ERR_BadVersion;
	nRet = -pGraph->iErrno;
	break;
    }
    return nRet;
}

int dglRead(dglGraph_s * pGraph, int fd)
{
    dglByte_t bVersion;
    int nRet;

    if (read(fd, &bVersion, 1) != 1) {
	pGraph->iErrno = DGL_ERR_Read;
	nRet = -pGraph->iErrno;
    }
    else {
	switch (bVersion) {
	case 1:
	    nRet = dgl_read_V1(pGraph, fd);
	    break;
#ifdef DGL_V2
	case 2:
	case 3:
	    nRet = dgl_read_V2(pGraph, fd, bVersion);
	    break;
#endif
	default:
	    pGraph->iErrno = DGL_ERR_VersionNotSupported;
	    nRet = -pGraph->iErrno;
	    break;
	}
    }
    return nRet;
}


int dglShortestPath(dglGraph_s * pGraph,
		    dglSPReport_s ** ppReport,
		    dglInt32_t nStart,
		    dglInt32_t nDestination,
		    dglSPClip_fn fnClip,
		    void *pvClipArg, dglSPCache_s * pCache)
{
    int nRet;

    switch (pGraph->Version) {
    case 1:
	nRet =
	    dgl_dijkstra_V1(pGraph, ppReport, NULL, nStart, nDestination,
			    fnClip, pvClipArg, pCache);
	break;
#ifdef DGL_V2
    case 2:
    case 3:
	nRet =
	    dgl_dijkstra_V2(pGraph, ppReport, NULL, nStart, nDestination,
			    fnClip, pvClipArg, pCache);
	break;
#endif
    default:
	pGraph->iErrno = DGL_ERR_BadVersion;
	nRet = -pGraph->iErrno;
	break;
    }
    return nRet;
}


int dglShortestDistance(dglGraph_s * pGraph,
			dglInt32_t * pnDistance,
			dglInt32_t nStart,
			dglInt32_t nDestination,
			dglSPClip_fn fnClip,
			void *pvClipArg, dglSPCache_s * pCache)
{
    int nRet;

    switch (pGraph->Version) {
    case 1:
	nRet =
	    dgl_dijkstra_V1(pGraph, NULL, pnDistance, nStart, nDestination,
			    fnClip, pvClipArg, pCache);
	break;
#ifdef DGL_V2
    case 2:
    case 3:
	nRet =
	    dgl_dijkstra_V2(pGraph, NULL, pnDistance, nStart, nDestination,
			    fnClip, pvClipArg, pCache);
	break;
#endif
    default:
	pGraph->iErrno = DGL_ERR_BadVersion;
	nRet = -pGraph->iErrno;
	break;
    }
    return nRet;
}


int dglDepthSpanning(dglGraph_s * pgraphInput,
		     dglGraph_s * pgraphOutput,
		     dglInt32_t nVertexNode,
		     dglSpanClip_fn fnClip, void *pvClipArg)
{
    int nRet;
    void *pvVisited;

    if (dglGet_EdgeCount(pgraphInput) == 0) {	/* no span */
	pgraphInput->iErrno = 0;
	return 0;
    }

#ifndef DGL_V2
    if (pgraphInput->Version == 2) {
	pgraphInput->iErrno = DGL_ERR_BadVersion;
	return -pgraphInput->iErrno;
    }
#endif

    nRet = dglInitialize(pgraphOutput,
			 dglGet_Version(pgraphInput),
			 dglGet_NodeAttrSize(pgraphInput),
			 dglGet_EdgeAttrSize(pgraphInput),
			 dglGet_Opaque(pgraphInput));

    if (nRet < 0)
	return nRet;

    if ((pvVisited =
	 avl_create(dglTreeNodeCompare, NULL,
		    dglTreeGetAllocator())) == NULL) {
	pgraphInput->iErrno = DGL_ERR_MemoryExhausted;
	return -pgraphInput->iErrno;
    }

    switch (pgraphInput->Version) {
    case 1:
	nRet =
	    dgl_depthfirst_spanning_V1(pgraphInput, pgraphOutput, nVertexNode,
				       pvVisited, fnClip, pvClipArg);
	break;
#ifdef DGL_V2
    case 2:
    case 3:
	nRet =
	    dgl_depthfirst_spanning_V2(pgraphInput, pgraphOutput, nVertexNode,
				       pvVisited, fnClip, pvClipArg);
	break;
#endif
    default:
	pgraphInput->iErrno = DGL_ERR_BadVersion;
	nRet = -pgraphInput->iErrno;
	break;
    }

    avl_destroy(pvVisited, dglTreeNodeCancel);

    if (nRet < 0) {
	dglRelease(pgraphOutput);
    }

    return nRet;
}

int dglDepthComponents(dglGraph_s * pgraphInput,
		       dglGraph_s * pgraphComponents,
		       int cgraphComponents,
		       dglSpanClip_fn fnClip, void *pvClipArg)
{
    int i, nret = 0;
    dglTreeNode_s findVisited;
    void *pvVisited;
    dglInt32_t *pvertex, *pnode;

    if (dglGet_EdgeCount(pgraphInput) == 0) {	/* no span */
	pgraphInput->iErrno = 0;
	return 0;
    }

#ifndef DGL_V2
    if (pgraphInput->Version == 2 || pgraphInput->Version == 3) {
	pgraphInput->iErrno = DGL_ERR_BadVersion;
	return -pgraphInput->iErrno;
    }
#endif

    if ((pvVisited =
	 avl_create(dglTreeNodeCompare, NULL,
		    dglTreeGetAllocator())) == NULL) {
	pgraphInput->iErrno = DGL_ERR_MemoryExhausted;
	goto error;
    }

    /*
     * choose a vertex to start from
     */
    pvertex = NULL;
    {
	dglNodeTraverser_s pT;

	dglNode_T_Initialize(&pT, pgraphInput);
	for (pnode = dglNode_T_First(&pT); pnode; pnode = dglNode_T_Next(&pT)) {
	    switch (pgraphInput->Version) {
	    case 1:
		if (DGL_NODE_STATUS_v1(pnode) & DGL_NS_HEAD)
		    pvertex = pnode;
		break;
#ifdef DGL_V2
	    case 2:
	    case 3:
		if (DGL_NODE_STATUS_v2(pnode) & DGL_NS_HEAD)
		    pvertex = pnode;
		break;
#endif
	    }
	    if (pvertex)
		break;
	}
	dglNode_T_Release(&pT);
    }

    if (pvertex == NULL) {
	pgraphInput->iErrno = DGL_ERR_UnexpectedNullPointer;
	goto error;
    }

    for (i = 0; i < cgraphComponents && pvertex; i++) {
	nret = dglInitialize(&pgraphComponents[i],
			     dglGet_Version(pgraphInput),
			     dglGet_NodeAttrSize(pgraphInput),
			     dglGet_EdgeAttrSize(pgraphInput),
			     dglGet_Opaque(pgraphInput));

	if (nret < 0)
	    goto error;

	switch (pgraphInput->Version) {
	case 1:
	    nret =
		dgl_depthfirst_spanning_V1(pgraphInput, &pgraphComponents[i],
					   DGL_NODE_ID_v1(pvertex), pvVisited,
					   fnClip, pvClipArg);
	    if (nret < 0)
		goto error;
	    break;
#ifdef DGL_V2
	case 2:
	case 3:
	    nret =
		dgl_depthfirst_spanning_V2(pgraphInput, &pgraphComponents[i],
					   DGL_NODE_ID_v1(pvertex), pvVisited,
					   fnClip, pvClipArg);
	    if (nret < 0)
		goto error;
	    break;
#endif
	default:
	    pgraphInput->iErrno = DGL_ERR_BadVersion;
	    nret = -pgraphInput->iErrno;
	    goto error;
	}

	/*
	 * select next unvisited vertex
	 */
	pvertex = NULL;
	{
	    dglNodeTraverser_s pT;

	    dglNode_T_Initialize(&pT, pgraphInput);
	    for (pnode = dglNode_T_First(&pT); pnode;
		 pnode = dglNode_T_Next(&pT)) {
		switch (pgraphInput->Version) {
		case 1:
		    if (DGL_NODE_STATUS_v1(pnode) & DGL_NS_HEAD) {
			findVisited.nKey = DGL_NODE_ID_v1(pnode);
			if (avl_find(pvVisited, &findVisited) == NULL) {
			    pvertex = pnode;
			    break;
			}
		    }
		    break;
#ifdef DGL_V2
		case 2:
		case 3:
		    if (DGL_NODE_STATUS_v2(pnode) & DGL_NS_HEAD) {
			findVisited.nKey = DGL_NODE_ID_v2(pnode);
			if (avl_find(pvVisited, &findVisited) == NULL) {
			    pvertex = pnode;
			    break;
			}
		    }
		    break;
#endif
		}
		if (pvertex)
		    break;
	    }
	    dglNode_T_Release(&pT);
	}
    }

    avl_destroy(pvVisited, dglTreeNodeCancel);
    return i;

  error:
    avl_destroy(pvVisited, dglTreeNodeCancel);
    return nret;
}

int dglMinimumSpanning(dglGraph_s * pgraphInput,
		       dglGraph_s * pgraphOutput,
		       dglInt32_t nVertexNode,
		       dglSpanClip_fn fnClip, void *pvClipArg)
{
    int nRet;

    if (dglGet_EdgeCount(pgraphInput) == 0) {	/* no span */
	pgraphInput->iErrno = 0;
	return 0;
    }

    nRet = dglInitialize(pgraphOutput,
			 dglGet_Version(pgraphInput),
			 dglGet_NodeAttrSize(pgraphInput),
			 dglGet_EdgeAttrSize(pgraphInput),
			 dglGet_Opaque(pgraphInput));

    if (nRet < 0)
	return nRet;

    switch (pgraphInput->Version) {
    case 1:
	nRet =
	    dgl_minimum_spanning_V1(pgraphInput, pgraphOutput, nVertexNode,
				    fnClip, pvClipArg);
	break;
#ifdef DGL_V2
    case 2:
    case 3:
	nRet =
	    dgl_minimum_spanning_V2(pgraphInput, pgraphOutput, nVertexNode,
				    fnClip, pvClipArg);
	break;
#endif
    default:
	pgraphInput->iErrno = DGL_ERR_BadVersion;
	nRet = -pgraphInput->iErrno;
	break;
    }
    if (nRet < 0) {
	dglRelease(pgraphOutput);
    }
    return nRet;
}

void dglFreeSPReport(dglGraph_s * pgraph, dglSPReport_s * pSPReport)
{
    int iArc;

    if (pSPReport) {
	if (pSPReport->pArc) {
	    for (iArc = 0; iArc < pSPReport->cArc; iArc++) {
		if (pSPReport->pArc[iArc].pnEdge)
		    free(pSPReport->pArc[iArc].pnEdge);
	    }
	    free(pSPReport->pArc);
	}
	free(pSPReport);
    }
}

int dglInitializeSPCache(dglGraph_s * pGraph, dglSPCache_s * pCache)
{
    switch (pGraph->Version) {
    case 1:
	return dgl_sp_cache_initialize_V1(pGraph, pCache, 0);
#ifdef DGL_V2
    case 2:
    case 3:
	return dgl_sp_cache_initialize_V2(pGraph, pCache, 0);
#endif
    }
    pGraph->iErrno = DGL_ERR_BadVersion;
    return -pGraph->iErrno;
}

void dglReleaseSPCache(dglGraph_s * pGraph, dglSPCache_s * pCache)
{
    pGraph->iErrno = 0;
    switch (pGraph->Version) {
    case 1:
	dgl_sp_cache_release_V1(pGraph, pCache);
	break;
#ifdef DGL_V2
    case 2:
    case 3:
	dgl_sp_cache_release_V2(pGraph, pCache);
	break;
#endif
    }
    pGraph->iErrno = DGL_ERR_BadVersion;
}


int dglErrno(dglGraph_s * pgraph)
{
    return pgraph->iErrno;
}

char *dglStrerror(dglGraph_s * pgraph)
{
    switch (pgraph->iErrno) {
    case DGL_ERR_BadVersion:
	return "Bad Version";
    case DGL_ERR_BadNodeType:
	return "Bad Node Type";
    case DGL_ERR_MemoryExhausted:
	return "Memory Exhausted";
    case DGL_ERR_HeapError:
	return "Heap Error";
    case DGL_ERR_UndefinedMethod:
	return "Undefined Method";
    case DGL_ERR_Write:
	return "Write";
    case DGL_ERR_Read:
	return "Read";
    case DGL_ERR_NotSupported:
	return "Not Supported";
    case DGL_ERR_UnknownByteOrder:
	return "Unknown Byte Order";
    case DGL_ERR_NodeNotFound:
	return "Node Not Found";
    case DGL_ERR_HeadNodeNotFound:
	return "Head Node Not Found";
    case DGL_ERR_TailNodeNotFound:
	return "Tail Node Not Found";
    case DGL_ERR_BadEdge:
	return "Bad Edge";
    case DGL_ERR_BadOnFlatGraph:
	return "Operation Not Supported On Flat-State Graph";
    case DGL_ERR_BadOnTreeGraph:
	return "Operation Not Supported On Tree-State Graph";
    case DGL_ERR_TreeSearchError:
	return "Tree Search Error";
    case DGL_ERR_UnexpectedNullPointer:
	return "Unexpected Null Pointer";
    case DGL_ERR_VersionNotSupported:
	return "Version Not Supported";
    case DGL_ERR_EdgeNotFound:
	return "Edge Not Found";
    case DGL_ERR_NodeAlreadyExist:
	return "Node Already Exist";
    case DGL_ERR_NodeIsAComponent:
	return "Node Is A Component";
    case DGL_ERR_EdgeAlreadyExist:
	return "Edge Already Exist";
    case DGL_ERR_BadArgument:
	return "Bad Argument";
    }

    return "unknown graph error code";
}

/*
 * dglGraph_s hiders
 */
int dglGet_Version(dglGraph_s * pgraph)
{
    return pgraph->Version;
}
void dglSet_Version(dglGraph_s * pgraph, int nVersion)
{
    pgraph->Version = nVersion;
}

int dglGet_Endianess(dglGraph_s * pgraph)
{
    return pgraph->Endian;
}

int dglGet_NodeAttrSize(dglGraph_s * pgraph)
{
    return pgraph->NodeAttrSize;
}

int dglGet_EdgeAttrSize(dglGraph_s * pgraph)
{
    return pgraph->EdgeAttrSize;
}

int dglGet_NodeCount(dglGraph_s * pgraph)
{
    return pgraph->cNode;
}

int dglGet_HeadNodeCount(dglGraph_s * pgraph)
{
    return pgraph->cHead;
}

int dglGet_TailNodeCount(dglGraph_s * pgraph)
{
    return pgraph->cTail;
}

int dglGet_AloneNodeCount(dglGraph_s * pgraph)
{
    return pgraph->cAlone;
}

int dglGet_EdgeCount(dglGraph_s * pgraph)
{
    return pgraph->cEdge;
}

int dglGet_State(dglGraph_s * pgraph)
{
    return pgraph->Flags;
}

dglInt32_t *dglGet_Opaque(dglGraph_s * pgraph)
{
    return pgraph->aOpaqueSet;
}

void dglSet_Opaque(dglGraph_s * pgraph, dglInt32_t * pOpaque)
{
    memcpy(pgraph->aOpaqueSet, pOpaque, sizeof(dglInt32_t) * 16);
}

int dglGet_NodeSize(dglGraph_s * pgraph)
{
    switch (pgraph->Version) {
    case 1:
	return DGL_NODE_SIZEOF_v1(pgraph->NodeAttrSize);
#ifdef DGL_V2
    case 2:
    case 3:
	return DGL_NODE_SIZEOF_v2(pgraph->NodeAttrSize);
#endif
    }
    pgraph->iErrno = DGL_ERR_BadVersion;
    return -pgraph->iErrno;
}

int dglGet_EdgeSize(dglGraph_s * pgraph)
{
    switch (pgraph->Version) {
    case 1:
	return DGL_EDGE_SIZEOF_v1(pgraph->NodeAttrSize);
#ifdef DGL_V2
    case 2:
    case 3:
	return DGL_EDGE_SIZEOF_v2(pgraph->NodeAttrSize);
#endif
    }
    pgraph->iErrno = DGL_ERR_BadVersion;
    return -pgraph->iErrno;
}

dglInt64_t dglGet_Cost(dglGraph_s * pgraph)
{
    return pgraph->nnCost;
}

void dglSet_Cost(dglGraph_s * pgraph, dglInt64_t nnCost)
{
    pgraph->nnCost = nnCost;
}

dglInt32_t dglGet_Family(dglGraph_s * pgraph)
{
    return pgraph->nFamily;
}

void dglSet_Family(dglGraph_s * pgraph, dglInt32_t nFamily)
{
    pgraph->nFamily = nFamily;
}

dglInt32_t dglGet_Options(dglGraph_s * pgraph)
{
    return pgraph->nOptions;
}

void dglSet_Options(dglGraph_s * pgraph, dglInt32_t nOptions)
{
    pgraph->nOptions = nOptions;
}

dglEdgePrioritizer_s *dglGet_EdgePrioritizer(dglGraph_s * pGraph)
{
    return &pGraph->edgePrioritizer;
}

dglNodePrioritizer_s *dglGet_NodePrioritizer(dglGraph_s * pGraph)
{
    return &pGraph->nodePrioritizer;
}



/*
 * Node Traverse
 */
int dglNode_T_Initialize(dglNodeTraverser_s * pT, dglGraph_s * pGraph)
{
    switch (pGraph->Version) {
    case 1:
	return dgl_node_t_initialize_V1(pGraph, pT);
#ifdef DGL_V2
    case 2:
    case 3:
	return dgl_node_t_initialize_V2(pGraph, pT);
#endif
    }
    pGraph->iErrno = DGL_ERR_BadVersion;
    return -pGraph->iErrno;
}

void dglNode_T_Release(dglNodeTraverser_s * pT)
{
    switch (pT->pGraph->Version) {
    case 1:
	dgl_node_t_release_V1(pT);
	return;
#ifdef DGL_V2
    case 2:
    case 3:
	dgl_node_t_release_V2(pT);
	return;
#endif
    }
    pT->pGraph->iErrno = DGL_ERR_BadVersion;
}

dglInt32_t *dglNode_T_First(dglNodeTraverser_s * pT)
{
    switch (pT->pGraph->Version) {
    case 1:
	return dgl_node_t_first_V1(pT);
#ifdef DGL_V2
    case 2:
    case 3:
	return dgl_node_t_first_V2(pT);
#endif
    }
    pT->pGraph->iErrno = DGL_ERR_BadVersion;
    return NULL;
}

dglInt32_t *dglNode_T_Next(dglNodeTraverser_s * pT)
{
    switch (pT->pGraph->Version) {
    case 1:
	return dgl_node_t_next_V1(pT);
#ifdef DGL_V2
    case 2:
    case 3:
	return dgl_node_t_next_V2(pT);
#endif
    }
    pT->pGraph->iErrno = DGL_ERR_BadVersion;
    return NULL;
}

dglInt32_t *dglNode_T_Find(dglNodeTraverser_s * pT, dglInt32_t nNodeId)
{
    switch (pT->pGraph->Version) {
    case 1:
	return dgl_node_t_find_V1(pT, nNodeId);
#ifdef DGL_V2
    case 2:
    case 3:
	return dgl_node_t_find_V2(pT, nNodeId);
#endif
    }
    pT->pGraph->iErrno = DGL_ERR_BadVersion;
    return NULL;
}

/*
 * Edge Traverser
 */
int dglEdge_T_Initialize(dglEdgeTraverser_s * pT,
			 dglGraph_s * pGraph,
			 dglEdgePrioritizer_s * pEdgePrioritizer)
{
    switch (pGraph->Version) {
    case 1:
	return dgl_edge_t_initialize_V1(pGraph, pT, pEdgePrioritizer);
#ifdef DGL_V2
    case 2:
    case 3:
	return dgl_edge_t_initialize_V2(pGraph, pT, pEdgePrioritizer);
#endif
    }
    pGraph->iErrno = DGL_ERR_BadVersion;
    return -pGraph->iErrno;
}

void dglEdge_T_Release(dglEdgeTraverser_s * pT)
{
    switch (pT->pGraph->Version) {
    case 1:
	dgl_edge_t_release_V1(pT);
	return;
#ifdef DGL_V2
    case 2:
    case 3:
	dgl_edge_t_release_V2(pT);
	return;
#endif
    }
    pT->pGraph->iErrno = DGL_ERR_BadVersion;
}

dglInt32_t *dglEdge_T_First(dglEdgeTraverser_s * pT)
{
    switch (pT->pGraph->Version) {
    case 1:
	return dgl_edge_t_first_V1(pT);
#ifdef DGL_V2
    case 2:
    case 3:
	return dgl_edge_t_first_V2(pT);
#endif
    }
    pT->pGraph->iErrno = DGL_ERR_BadVersion;
    return NULL;
}

dglInt32_t *dglEdge_T_Next(dglEdgeTraverser_s * pT)
{
    switch (pT->pGraph->Version) {
    case 1:
	return dgl_edge_t_next_V1(pT);
#ifdef DGL_V2
    case 2:
    case 3:
	return dgl_edge_t_next_V2(pT);
#endif
    }
    pT->pGraph->iErrno = DGL_ERR_BadVersion;
    return NULL;
}


int dglEdgeset_T_Initialize(dglEdgesetTraverser_s * pT,
			    dglGraph_s * pGraph, dglInt32_t * pnEdgeset)
{
    switch (pGraph->Version) {
    case 1:
	return dgl_edgeset_t_initialize_V1(pGraph, pT, pnEdgeset);
#ifdef DGL_V2
    case 2:
    case 3:
	return dgl_edgeset_t_initialize_V2(pGraph, pT, pnEdgeset);
#endif
    }
    pGraph->iErrno = DGL_ERR_BadVersion;
    return -pGraph->iErrno;
}

void dglEdgeset_T_Release(dglEdgesetTraverser_s * pT)
{
}

dglInt32_t *dglEdgeset_T_First(dglEdgesetTraverser_s * pT)
{
    switch (pT->pGraph->Version) {
    case 1:
	return dgl_edgeset_t_first_V1(pT);
#ifdef DGL_V2
    case 2:
    case 3:
	return dgl_edgeset_t_first_V2(pT);
#endif
    }
    pT->pGraph->iErrno = DGL_ERR_BadVersion;
    return NULL;
}

dglInt32_t *dglEdgeset_T_Next(dglEdgesetTraverser_s * pT)
{
    switch (pT->pGraph->Version) {
    case 1:
	return dgl_edgeset_t_next_V1(pT);
#ifdef DGL_V2
    case 2:
    case 3:
	return dgl_edgeset_t_next_V2(pT);
#endif
    }
    pT->pGraph->iErrno = DGL_ERR_BadVersion;
    return NULL;
}


/*
 * chunked I/O
 */

#define __CIO_BEGIN	0
#define __CIO_W_HEADER 1
#define __CIO_W_NODEBUFFER 2
#define __CIO_W_EDGEBUFFER 3
#define __CIO_R_HEADER 4
#define __CIO_R_NODEBUFFER 5
#define __CIO_R_EDGEBUFFER 6
#define __CIO_END 7

int dglIOContextInitialize(dglGraph_s * pG, dglIOContext_s * pIO)
{
    pIO->pG = pG;
    pIO->nState = __CIO_BEGIN;
    pIO->cb = 0;
    pIO->ib = 0;
    pIO->pb = NULL;
    return 0;
}

void dglIOContextRelease(dglIOContext_s * pIO)
{
}

int dglWriteChunk(dglIOContext_s * pIO, dglWriteChunk_fn pfn, void *pv)
{
    unsigned char *pb;
    int cb;

    switch (pIO->nState) {
    case __CIO_BEGIN:
	pIO->pb = pIO->ab;
	pb = pIO->pb;
	memcpy(pb, &pIO->pG->Version, 1);
	pb += 1;		/* 1 */
	memcpy(pb, &pIO->pG->Endian, 1);
	pb += 1;		/* 2 */
	memcpy(pb, &pIO->pG->NodeAttrSize, 4);
	pb += 4;		/* 6 */
	memcpy(pb, &pIO->pG->EdgeAttrSize, 4);
	pb += 4;		/* 10 */
	memcpy(pb, &pIO->pG->aOpaqueSet, 64);
	pb += 64;		/* 74 */
	memcpy(pb, &pIO->pG->nOptions, 4);
	pb += 4;		/* 78 */
	memcpy(pb, &pIO->pG->nFamily, 4);
	pb += 4;		/* 82 */
	memcpy(pb, &pIO->pG->nnCost, 8);
	pb += 8;		/* 90 */
	memcpy(pb, &pIO->pG->cNode, 4);
	pb += 4;		/* 94 */
	memcpy(pb, &pIO->pG->cHead, 4);
	pb += 4;		/* 98 */
	memcpy(pb, &pIO->pG->cTail, 4);
	pb += 4;		/* 102 */
	memcpy(pb, &pIO->pG->cAlone, 4);
	pb += 4;		/* 106 */
	memcpy(pb, &pIO->pG->cEdge, 4);
	pb += 4;		/* 110 */
	memcpy(pb, &pIO->pG->iNodeBuffer, 4);
	pb += 4;		/* 114 */
	memcpy(pb, &pIO->pG->iEdgeBuffer, 4);
	pb += 4;		/* 118 */
	pIO->cb = 118;
	cb = pfn(pIO->pG, pIO->pb, pIO->cb, pv);
	if (cb >= 0) {
	    pIO->ib += cb;
	    if ((pIO->cb - pIO->ib) == 0) {
		pIO->ib = 0;
		pIO->cb = pIO->pG->iNodeBuffer;
		pIO->pb = pIO->pG->pNodeBuffer;
		pIO->nState = __CIO_W_NODEBUFFER;
	    }
	    else {
		pIO->nState = __CIO_W_HEADER;
	    }
	}
	return cb;
    case __CIO_W_HEADER:
	cb = pfn(pIO->pG, pIO->pb + pIO->ib, pIO->cb - pIO->ib, pv);
	if (cb > 0) {
	    pIO->ib += cb;
	    if ((pIO->cb - pIO->ib) == 0) {
		if (pIO->pG->iNodeBuffer > 0) {
		    pIO->ib = 0;
		    pIO->cb = pIO->pG->iNodeBuffer;
		    pIO->pb = pIO->pG->pNodeBuffer;
		    pIO->nState = __CIO_W_NODEBUFFER;
		}
		else if (pIO->pG->iEdgeBuffer > 0) {
		    pIO->ib = 0;
		    pIO->cb = pIO->pG->iEdgeBuffer;
		    pIO->pb = pIO->pG->pEdgeBuffer;
		    pIO->nState = __CIO_W_EDGEBUFFER;
		}
		else {
		    pIO->nState = __CIO_END;
		}
	    }
	    else {
		pIO->nState = __CIO_W_HEADER;
	    }
	}
	return cb;
    case __CIO_W_NODEBUFFER:
	cb = pfn(pIO->pG, pIO->pb + pIO->ib, pIO->cb - pIO->ib, pv);
	if (cb > 0) {
	    pIO->ib += cb;
	    if ((pIO->cb - pIO->ib) == 0) {
		if (pIO->pG->iEdgeBuffer > 0) {
		    pIO->ib = 0;
		    pIO->cb = pIO->pG->iEdgeBuffer;
		    pIO->pb = pIO->pG->pEdgeBuffer;
		    pIO->nState = __CIO_W_EDGEBUFFER;
		}
		else {
		    pIO->nState = __CIO_END;
		}
	    }
	}
	return cb;
    case __CIO_W_EDGEBUFFER:
	cb = pfn(pIO->pG, pIO->pb + pIO->ib, pIO->cb - pIO->ib, pv);
	if (cb > 0) {
	    pIO->ib += cb;
	    if ((pIO->cb - pIO->ib) == 0) {
		pIO->nState = __CIO_END;
	    }
	}
	return cb;
    case __CIO_END:
	pfn(pIO->pG, NULL, 0, pv);	/* notify end of graph */
	return 0;
    }
    return 0;
}

#ifndef MIN
#define MIN(x,y)	(((x)<(y))?x:y)
#endif

int dglReadChunk(dglIOContext_s * pIO, dglByte_t * pbChunk, int cbChunk)
{
    int i, c;
    unsigned char *pb;

    switch (pIO->nState) {
    case __CIO_BEGIN:
	pIO->cb = 118;
	pIO->ib = 0;
	pIO->pb = pIO->ab;

	c = MIN(cbChunk, 118);
	memcpy(pIO->pb, pbChunk, c);
	pIO->ib += c;
	if ((pIO->cb - pIO->ib) == 0)
	    goto init_nodebuffer;
	pIO->nState = __CIO_R_HEADER;
	return c;

    case __CIO_R_HEADER:
	c = MIN(cbChunk, pIO->cb - pIO->ib);
	memcpy(pIO->pb + pIO->ib, pbChunk, c);
	pIO->ib += c;
      init_nodebuffer:
	if ((pIO->cb - pIO->ib) == 0) {
	    pb = pIO->pb;
	    memcpy(&pIO->pG->Version, pb, 1);
	    pb += 1;		/* 1 */
	    memcpy(&pIO->pG->Endian, pb, 1);
	    pb += 1;		/* 2 */
	    memcpy(&pIO->pG->NodeAttrSize, pb, 4);
	    pb += 4;		/* 6 */
	    memcpy(&pIO->pG->EdgeAttrSize, pb, 4);
	    pb += 4;		/* 10 */
	    memcpy(&pIO->pG->aOpaqueSet, pb, 64);
	    pb += 64;		/* 74 */
	    memcpy(&pIO->pG->nOptions, pb, 4);
	    pb += 4;		/* 78 */
	    memcpy(&pIO->pG->nFamily, pb, 4);
	    pb += 4;		/* 82 */
	    memcpy(&pIO->pG->nnCost, pb, 8);
	    pb += 8;		/* 90 */
	    memcpy(&pIO->pG->cNode, pb, 4);
	    pb += 4;		/* 94 */
	    memcpy(&pIO->pG->cHead, pb, 4);
	    pb += 4;		/* 98 */
	    memcpy(&pIO->pG->cTail, pb, 4);
	    pb += 4;		/* 102 */
	    memcpy(&pIO->pG->cAlone, pb, 4);
	    pb += 4;		/* 106 */
	    memcpy(&pIO->pG->cEdge, pb, 4);
	    pb += 4;		/* 110 */
	    memcpy(&pIO->pG->iNodeBuffer, pb, 4);
	    pb += 4;		/* 114 */
	    memcpy(&pIO->pG->iEdgeBuffer, pb, 4);
	    pb += 4;		/* 118 */

	    pIO->fSwap = 0;
#ifdef DGL_ENDIAN_BIG
	    if (pIO->pG->Endian == DGL_ENDIAN_LITTLE)
		pIO->fSwap = 1;
#else
	    if (pIO->pG->Endian == DGL_ENDIAN_BIG)
		pIO->fSwap = 1;
#endif
	    if (pIO->fSwap) {
		dgl_swapInt32Bytes(&pIO->pG->NodeAttrSize);
		dgl_swapInt32Bytes(&pIO->pG->EdgeAttrSize);
		dgl_swapInt32Bytes(&pIO->pG->nOptions);
		dgl_swapInt32Bytes(&pIO->pG->nFamily);
		dgl_swapInt64Bytes(&pIO->pG->nnCost);
		dgl_swapInt32Bytes(&pIO->pG->cNode);
		dgl_swapInt32Bytes(&pIO->pG->cHead);
		dgl_swapInt32Bytes(&pIO->pG->cTail);
		dgl_swapInt32Bytes(&pIO->pG->cAlone);
		dgl_swapInt32Bytes(&pIO->pG->cEdge);
		dgl_swapInt32Bytes(&pIO->pG->iNodeBuffer);
		dgl_swapInt32Bytes(&pIO->pG->iEdgeBuffer);

		for (i = 0; i < 16; i++) {
		    dgl_swapInt32Bytes(&pIO->pG->aOpaqueSet[i]);
		}

#ifdef DGL_ENDIAN_BIG
		pIO->pG->Endian = DGL_ENDIAN_BIG;
#else
		pIO->pG->Endian = DGL_ENDIAN_LITTLE;
#endif
	    }

	    if (pIO->pG->iNodeBuffer > 0) {
		pIO->pG->pNodeBuffer = malloc(pIO->pG->iNodeBuffer);
		if (pIO->pG->pNodeBuffer == NULL) {
		    return -1;
		}
		pIO->cb = pIO->pG->iNodeBuffer;
		pIO->pb = pIO->pG->pNodeBuffer;
		pIO->ib = 0;
		pIO->nState = __CIO_R_NODEBUFFER;
	    }
	    else {
		goto init_edgebuffer;
	    }
	}
	return c;
    case __CIO_R_NODEBUFFER:
	c = MIN(cbChunk, pIO->cb - pIO->ib);
	memcpy(pIO->pb + pIO->ib, pbChunk, c);
	pIO->ib += c;
      init_edgebuffer:
	if ((pIO->cb - pIO->ib) == 0) {
	    if (pIO->pG->iEdgeBuffer > 0) {
		pIO->pG->pEdgeBuffer = malloc(pIO->pG->iEdgeBuffer);
		if (pIO->pG->pEdgeBuffer == NULL) {
		    return -1;
		}
		pIO->cb = pIO->pG->iEdgeBuffer;
		pIO->pb = pIO->pG->pEdgeBuffer;
		pIO->ib = 0;
		pIO->nState = __CIO_R_EDGEBUFFER;
	    }
	    else {
		pIO->nState = __CIO_END;
	    }
	}
	return c;
    case __CIO_R_EDGEBUFFER:
	c = MIN(cbChunk, pIO->cb - pIO->ib);
	memcpy(pIO->pb + pIO->ib, pbChunk, c);
	pIO->ib += c;
	if ((pIO->cb - pIO->ib) == 0) {
	    pIO->nState = __CIO_END;
	}
	return c;
    case __CIO_END:
	{
	    /* switch on FLAT bit */
	    pIO->pG->Flags |= DGL_GS_FLAT;

	    /* nodebuffer and edgebuffer are both arrays on 32 bit integer
	     * byte order swapping is straightforward
	     */
	    if (pIO->fSwap && pIO->pG->iNodeBuffer > 0) {
		int in, cn;
		dglInt32_t *pn;

		for (cn = pIO->pG->iNodeBuffer / sizeof(dglInt32_t),
		     pn = (dglInt32_t *) pIO->pG->pNodeBuffer,
		     in = 0; in < cn; in++) {
		    dgl_swapInt32Bytes(&pn[in]);
		}
	    }
	    if (pIO->fSwap && pIO->pG->iEdgeBuffer > 0) {
		int in, cn;
		dglInt32_t *pn;

		for (cn = pIO->pG->iEdgeBuffer / sizeof(dglInt32_t),
		     pn = (dglInt32_t *) pIO->pG->pEdgeBuffer,
		     in = 0; in < cn; in++) {
		    dgl_swapInt32Bytes(&pn[in]);
		}
	    }
	}
	return 0;
    default:
	return 0;
    }
}

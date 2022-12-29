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


#include "type.h"
#include "tree.h"
#include "graph.h"
#include "graph_v2.h"
#include "helpers.h"


/* Template expansion
 */
#include "v2-defs.h"
#include "sp-template.c"
#include "nodemgmt-template.c"
#include "edgemgmt-template.c"
#include "misc-template.c"


/* algorithms for TREE state
 */
#define DGL_DEFINE_TREE_PROCS 1
#include "v2-defs.h"
#include "sp-template.c"
#include "span-template.c"
#undef DGL_DEFINE_TREE_PROCS


/* algorithms for FLAT state
 */
#define DGL_DEFINE_FLAT_PROCS 1
#include "v2-defs.h"
#include "sp-template.c"
#include "span-template.c"
#undef DGL_DEFINE_FLAT_PROCS



int dgl_dijkstra_V2(dglGraph_s * pgraph,
		    dglSPReport_s ** ppReport,
		    dglInt32_t * pDistance,
		    dglInt32_t nStart,
		    dglInt32_t nDestination,
		    dglSPClip_fn fnClip,
		    void *pvClipArg, dglSPCache_s * pCache)
{
    if (pgraph->Flags & DGL_GS_FLAT) {
	return dgl_dijkstra_V2_FLAT(pgraph, ppReport, pDistance, nStart,
				    nDestination, fnClip, pvClipArg, pCache);
    }
    else {
	return dgl_dijkstra_V2_TREE(pgraph, ppReport, pDistance, nStart,
				    nDestination, fnClip, pvClipArg, pCache);
    }
}


int dgl_depthfirst_spanning_V2(dglGraph_s * pgraphIn,
			       dglGraph_s * pgraphOut,
			       dglInt32_t nVertex,
			       void *pvVisited,
			       dglSpanClip_fn fnClip, void *pvClipArg)
{
    if (pgraphIn->Flags & DGL_GS_FLAT) {
	return dgl_span_depthfirst_spanning_V2_FLAT(pgraphIn, pgraphOut,
						    nVertex, pvVisited,
						    fnClip, pvClipArg);
    }
    else {
	return dgl_span_depthfirst_spanning_V2_TREE(pgraphIn, pgraphOut,
						    nVertex, pvVisited,
						    fnClip, pvClipArg);
    }
}

int dgl_minimum_spanning_V2(dglGraph_s * pgraphIn,
			    dglGraph_s * pgraphOut,
			    dglInt32_t nVertex,
			    dglSpanClip_fn fnClip, void *pvClipArg)
{
    if (pgraphIn->Flags & DGL_GS_FLAT) {
	return dgl_span_minimum_spanning_V2_FLAT(pgraphIn, pgraphOut, nVertex,
						 fnClip, pvClipArg);
    }
    else {
	return dgl_span_minimum_spanning_V2_TREE(pgraphIn, pgraphOut, nVertex,
						 fnClip, pvClipArg);
    }
}


int dgl_initialize_V2(dglGraph_s * pgraph)
{
    if (pgraph->pNodeTree == NULL)
	pgraph->pNodeTree =
	    avl_create(dglTreeNode2Compare, NULL, dglTreeGetAllocator());
    if (pgraph->pNodeTree == NULL) {
	pgraph->iErrno = DGL_ERR_MemoryExhausted;
	return -pgraph->iErrno;
    }
    if (pgraph->pEdgeTree == NULL)
	pgraph->pEdgeTree =
	    avl_create(dglTreeEdgeCompare, NULL, dglTreeGetAllocator());
    if (pgraph->pEdgeTree == NULL) {
	pgraph->iErrno = DGL_ERR_MemoryExhausted;
	return -pgraph->iErrno;
    }
    return 0;
}

int dgl_release_V2(dglGraph_s * pgraph)
{
    pgraph->iErrno = 0;

    if (pgraph->pNodeTree)
	avl_destroy(pgraph->pNodeTree, dglTreeNodeCancel);
    if (pgraph->pEdgeTree)
	avl_destroy(pgraph->pEdgeTree, dglTreeEdgeCancel);
    if (pgraph->pNodeBuffer)
	free(pgraph->pNodeBuffer);
    if (pgraph->pEdgeBuffer)
	free(pgraph->pEdgeBuffer);
    if (pgraph->edgePrioritizer.pvAVL)
	avl_destroy(pgraph->edgePrioritizer.pvAVL, dglTreeEdgePri32Cancel);
    if (pgraph->nodePrioritizer.pvAVL)
	avl_destroy(pgraph->nodePrioritizer.pvAVL, dglTreeNodePri32Cancel);

    return 0;
}


int dgl_write_V2(dglGraph_s * pgraph, int fd)
{
    long nret, cnt, tot;

    pgraph->iErrno = 0;

    if (write(fd, &pgraph->Version, 1) != 1) {
	pgraph->iErrno = DGL_ERR_Write;
	return -pgraph->iErrno;
    }

    if (write(fd, &pgraph->Endian, 1) != 1) {
	pgraph->iErrno = DGL_ERR_Write;
	return -pgraph->iErrno;
    }

    if (write(fd, &pgraph->NodeAttrSize, sizeof(dglInt32_t)) !=
	sizeof(dglInt32_t)) {
	pgraph->iErrno = DGL_ERR_Write;
	return -pgraph->iErrno;
    }

    if (write(fd, &pgraph->EdgeAttrSize, sizeof(dglInt32_t)) !=
	sizeof(dglInt32_t)) {
	pgraph->iErrno = DGL_ERR_Write;
	return -pgraph->iErrno;
    }

    for (cnt = 0; cnt < 16; cnt++) {
	if (write(fd, &pgraph->aOpaqueSet[cnt], sizeof(dglInt32_t)) !=
	    sizeof(dglInt32_t)) {
	    pgraph->iErrno = DGL_ERR_Write;
	    return -pgraph->iErrno;
	}
    }

    if (write(fd, &pgraph->nnCost, sizeof(dglInt64_t)) != sizeof(dglInt64_t)) {
	pgraph->iErrno = DGL_ERR_Write;
	return -pgraph->iErrno;
    }

    if (write(fd, &pgraph->cNode, sizeof(dglInt32_t)) != sizeof(dglInt32_t)) {
	pgraph->iErrno = DGL_ERR_Write;
	return -pgraph->iErrno;
    }

    if (write(fd, &pgraph->cHead, sizeof(dglInt32_t)) != sizeof(dglInt32_t)) {
	pgraph->iErrno = DGL_ERR_Write;
	return -pgraph->iErrno;
    }

    if (write(fd, &pgraph->cTail, sizeof(dglInt32_t)) != sizeof(dglInt32_t)) {
	pgraph->iErrno = DGL_ERR_Write;
	return -pgraph->iErrno;
    }

    if (write(fd, &pgraph->cAlone, sizeof(dglInt32_t)) != sizeof(dglInt32_t)) {
	pgraph->iErrno = DGL_ERR_Write;
	return -pgraph->iErrno;
    }

    if (write(fd, &pgraph->cEdge, sizeof(dglInt32_t)) != sizeof(dglInt32_t)) {
	pgraph->iErrno = DGL_ERR_Write;
	return -pgraph->iErrno;
    }

    if (write(fd, &pgraph->iNodeBuffer, sizeof(dglInt32_t)) !=
	sizeof(dglInt32_t)) {
	pgraph->iErrno = DGL_ERR_Write;
	return -pgraph->iErrno;
    }

    if (write(fd, &pgraph->iEdgeBuffer, sizeof(dglInt32_t)) !=
	sizeof(dglInt32_t)) {
	pgraph->iErrno = DGL_ERR_Write;
	return -pgraph->iErrno;
    }

    for (tot = 0, cnt = pgraph->iNodeBuffer; tot < cnt; tot += nret) {
	if ((nret = write(fd, &pgraph->pNodeBuffer[tot], cnt - tot)) <= 0) {
	    pgraph->iErrno = DGL_ERR_Write;
	    return -pgraph->iErrno;
	}
    }

    for (tot = 0, cnt = pgraph->iEdgeBuffer; tot < cnt; tot += nret) {
	if ((nret = write(fd, &pgraph->pEdgeBuffer[tot], cnt - tot)) <= 0) {
	    pgraph->iErrno = DGL_ERR_Write;
	    return -pgraph->iErrno;
	}
    }

    return 0;
}


int dgl_read_V2(dglGraph_s * pgraph, int fd, int version)
{
    long nret, cnt, tot;
    dglByte_t Endian;
    dglInt32_t NodeAttrSize, EdgeAttrSize;
    int i, cn, fSwap;
    dglInt32_t *pn;

    if (read(fd, &Endian, 1) != 1) {
	pgraph->iErrno = DGL_ERR_Read;
	return -pgraph->iErrno;
    }

    fSwap = 0;
#ifdef DGL_ENDIAN_BIG
    if (Endian == DGL_ENDIAN_LITTLE)
	fSwap = 1;
#else
    if (Endian == DGL_ENDIAN_BIG)
	fSwap = 1;
#endif

    if (read(fd, &NodeAttrSize, sizeof(dglInt32_t)) != sizeof(dglInt32_t)) {
	pgraph->iErrno = DGL_ERR_Read;
	return -pgraph->iErrno;
    }
    if (fSwap)
	dgl_swapInt32Bytes(&NodeAttrSize);

    if (read(fd, &EdgeAttrSize, sizeof(dglInt32_t)) != sizeof(dglInt32_t)) {
	pgraph->iErrno = DGL_ERR_Read;
	return -pgraph->iErrno;
    }
    if (fSwap)
	dgl_swapInt32Bytes(&EdgeAttrSize);

    if ((nret =
	 dglInitialize(pgraph, version, NodeAttrSize, EdgeAttrSize,
		       NULL)) < 0) {
	return nret;
    }

    for (cnt = 0; cnt < 16; cnt++) {
	if ((nret =
	     read(fd, &pgraph->aOpaqueSet[cnt],
		  sizeof(dglInt32_t))) != sizeof(dglInt32_t)) {
	    pgraph->iErrno = DGL_ERR_Read;
	    return -pgraph->iErrno;
	}
	if (fSwap)
	    dgl_swapInt32Bytes(&pgraph->aOpaqueSet[cnt]);
    }

    if (read(fd, &pgraph->nnCost, sizeof(dglInt64_t)) != sizeof(dglInt64_t)) {
	pgraph->iErrno = DGL_ERR_Read;
	return -pgraph->iErrno;
    }
    if (fSwap)
	dgl_swapInt64Bytes(&pgraph->nnCost);

    if (read(fd, &pgraph->cNode, sizeof(dglInt32_t)) != sizeof(dglInt32_t)) {
	pgraph->iErrno = DGL_ERR_Read;
	return -pgraph->iErrno;
    }
    if (fSwap)
	dgl_swapInt32Bytes(&pgraph->cNode);

    if (read(fd, &pgraph->cHead, sizeof(dglInt32_t)) != sizeof(dglInt32_t)) {
	pgraph->iErrno = DGL_ERR_Read;
	return -pgraph->iErrno;
    }
    if (fSwap)
	dgl_swapInt32Bytes(&pgraph->cHead);

    if (read(fd, &pgraph->cTail, sizeof(dglInt32_t)) != sizeof(dglInt32_t)) {
	pgraph->iErrno = DGL_ERR_Read;
	return -pgraph->iErrno;
    }
    if (fSwap)
	dgl_swapInt32Bytes(&pgraph->cTail);

    if (read(fd, &pgraph->cAlone, sizeof(dglInt32_t)) != sizeof(dglInt32_t)) {
	pgraph->iErrno = DGL_ERR_Read;
	return -pgraph->iErrno;
    }
    if (fSwap)
	dgl_swapInt32Bytes(&pgraph->cAlone);

    if (read(fd, &pgraph->cEdge, sizeof(dglInt32_t)) != sizeof(dglInt32_t)) {
	pgraph->iErrno = DGL_ERR_Read;
	return -pgraph->iErrno;
    }
    if (fSwap)
	dgl_swapInt32Bytes(&pgraph->cEdge);

    if (read(fd, &pgraph->iNodeBuffer, sizeof(dglInt32_t)) !=
	sizeof(dglInt32_t)) {
	pgraph->iErrno = DGL_ERR_Read;
	return -pgraph->iErrno;
    }
    if (fSwap)
	dgl_swapInt32Bytes(&pgraph->iNodeBuffer);

    if (read(fd, &pgraph->iEdgeBuffer, sizeof(dglInt32_t)) !=
	sizeof(dglInt32_t)) {
	pgraph->iErrno = DGL_ERR_Read;
	return -pgraph->iErrno;
    }
    if (fSwap)
	dgl_swapInt32Bytes(&pgraph->iEdgeBuffer);

    if ((pgraph->pNodeBuffer = malloc(pgraph->iNodeBuffer)) == NULL) {
	pgraph->iErrno = DGL_ERR_MemoryExhausted;
	return -pgraph->iErrno;
    }

    if ((pgraph->pEdgeBuffer = malloc(pgraph->iEdgeBuffer)) == NULL) {
	free(pgraph->pNodeBuffer);
	pgraph->iErrno = DGL_ERR_MemoryExhausted;
	return -pgraph->iErrno;
    }

    for (tot = 0, cnt = pgraph->iNodeBuffer; tot < cnt; tot += nret) {
	if ((nret = read(fd, &pgraph->pNodeBuffer[tot], cnt - tot)) <= 0) {
	    free(pgraph->pNodeBuffer);
	    free(pgraph->pEdgeBuffer);
	    pgraph->iErrno = DGL_ERR_Read;
	    return -pgraph->iErrno;
	}
    }
    if (fSwap) {
	pn = (dglInt32_t *) pgraph->pNodeBuffer;
	cn = pgraph->iNodeBuffer / sizeof(dglInt32_t);
	for (i = 0; i < cn; i++) {
	    dgl_swapInt32Bytes(&pn[i]);
	}
    }

    for (tot = 0, cnt = pgraph->iEdgeBuffer; tot < cnt; tot += nret) {
	if ((nret = read(fd, &pgraph->pEdgeBuffer[tot], cnt - tot)) <= 0) {
	    free(pgraph->pNodeBuffer);
	    free(pgraph->pEdgeBuffer);
	    pgraph->iErrno = DGL_ERR_Read;
	    return -pgraph->iErrno;
	}
    }
    if (fSwap) {
	pn = (dglInt32_t *) pgraph->pEdgeBuffer;
	cn = pgraph->iEdgeBuffer / sizeof(dglInt32_t);
	for (i = 0; i < cn; i++) {
	    dgl_swapInt32Bytes(&pn[i]);
	}
    }

    pgraph->Flags |= 0x1;	/* flat-state */
    return 0;
}

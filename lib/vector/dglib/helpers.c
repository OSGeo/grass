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


#include <stdlib.h>
#include <string.h>

#include "type.h"
#include "tree.h"
#include "graph.h"
#include "helpers.h"

/*
 * helpers for parametric stack
 */
unsigned char *dgl_mempush(unsigned char *pstack, long *istack, long size,
			   void *pv)
{
    if (*istack == 0)
	pstack = NULL;
    pstack = realloc(pstack, size * (1 + *istack));
    if (pstack == NULL)
	return NULL;
    memcpy(&pstack[(*istack) * size], pv, size);
    (*istack)++;
    return pstack;
}

unsigned char *dgl_mempop(unsigned char *pstack, long *istack, long size)
{
    if (*istack == 0)
	return NULL;
    return &pstack[size * (--(*istack))];
}

void dgl_swapInt32Bytes(dglInt32_t * pn)
{
    unsigned char *pb = (unsigned char *)pn;

    pb[0] ^= pb[3];
    pb[3] ^= pb[0];
    pb[0] ^= pb[3];

    pb[1] ^= pb[2];
    pb[2] ^= pb[1];
    pb[1] ^= pb[2];
}

void dgl_swapInt64Bytes(dglInt64_t * pn)
{
    unsigned char *pb = (unsigned char *)pn;

    pb[0] ^= pb[7];
    pb[7] ^= pb[0];
    pb[0] ^= pb[7];

    pb[1] ^= pb[6];
    pb[6] ^= pb[1];
    pb[1] ^= pb[6];

    pb[2] ^= pb[5];
    pb[5] ^= pb[2];
    pb[2] ^= pb[5];

    pb[3] ^= pb[4];
    pb[4] ^= pb[3];
    pb[3] ^= pb[4];
}

/*
 * Keep the edge cost prioritizer in sync
 */
int dgl_edge_prioritizer_del(dglGraph_s * pG, dglInt32_t nId,
			     dglInt32_t nPriId)
{
    dglTreeEdgePri32_s findPriItem, *pPriItem;
    register int iEdge1, iEdge2;
    dglInt32_t *pnNew;

    if (pG->edgePrioritizer.pvAVL) {

	findPriItem.nKey = nPriId;
	pPriItem = avl_find(pG->edgePrioritizer.pvAVL, &findPriItem);

	if (pPriItem && pPriItem->pnData) {

	    pnNew = malloc(sizeof(dglInt32_t) * pPriItem->cnData);

	    if (pnNew == NULL) {
		pG->iErrno = DGL_ERR_MemoryExhausted;
		return -pG->iErrno;
	    }

	    for (iEdge1 = 0, iEdge2 = 0; iEdge2 < pPriItem->cnData; iEdge2++) {
		if (pPriItem->pnData[iEdge2] != nId) {
		    pnNew[iEdge1++] = pPriItem->pnData[iEdge2];
		}
	    }

	    free(pPriItem->pnData);
	    if (iEdge1 == 0) {
		free(pnNew);
		pPriItem->pnData = NULL;
		pPriItem->cnData = 0;
	    }
	    else {
		pPriItem->pnData = pnNew;
		pPriItem->cnData = iEdge1;
	    }
	}
    }
    return 0;
}

int dgl_edge_prioritizer_add(dglGraph_s * pG, dglInt32_t nId,
			     dglInt32_t nPriId)
{
    dglTreeEdgePri32_s *pPriItem;

    if (pG->edgePrioritizer.pvAVL == NULL) {
	pG->edgePrioritizer.pvAVL =
	    avl_create(dglTreeEdgePri32Compare, NULL, dglTreeGetAllocator());
	if (pG->edgePrioritizer.pvAVL == NULL) {
	    pG->iErrno = DGL_ERR_MemoryExhausted;
	    return -pG->iErrno;
	}
    }
    pPriItem = dglTreeEdgePri32Add(pG->edgePrioritizer.pvAVL, nPriId);
    if (pPriItem == NULL) {
	pG->iErrno = DGL_ERR_MemoryExhausted;
	return -pG->iErrno;
    }
    if (pPriItem->cnData == 0) {
	pPriItem->pnData = (dglInt32_t *) malloc(sizeof(dglInt32_t));
    }
    else {
	pPriItem->pnData =
	    (dglInt32_t *) realloc(pPriItem->pnData,
				   sizeof(dglInt32_t) * (pPriItem->cnData +
							 1));
    }
    if (pPriItem->pnData == NULL) {
	pG->iErrno = DGL_ERR_MemoryExhausted;
	return -pG->iErrno;
    }
    pPriItem->pnData[pPriItem->cnData] = nId;
    pPriItem->cnData++;
    return 0;
}

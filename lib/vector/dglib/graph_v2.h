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
 * best view tabstop=4
 */

#ifndef _DGL_dglGraph_s_V2_H_
#define _DGL_dglGraph_s_V2_H_

#ifdef DGL_STATS
#include <time.h>
#endif

/*
 * Node macros - addresses in a flat node
 */
#define DGL_IN_NODEID_v2					0
#define DGL_IN_STATUS_v2 					1
#define DGL_IN_EDGESET_OFFSET_v2 			2
#define DGL_IN_ATTR_v2					3
#define DGL_IN_SIZE_v2					DGL_IN_ATTR_v2

#define DGL_NODE_SIZEOF_v2( nattr  ) 	 	(sizeof( dglInt32_t ) * DGL_IN_SIZE_v2 + (nattr) )
#define DGL_NODE_WSIZE_v2( nattr )		(DGL_NODE_SIZEOF_v2( nattr ) / sizeof(dglInt32_t) )
#define DGL_NODE_ALLOC_v2( nattr )   		(malloc( DGL_NODE_SIZEOF_v2( nattr ) ) )

#define DGL_NODE_ID_v2(p)					((p)[DGL_IN_NODEID_v2])
#define DGL_NODE_STATUS_v2(p)				((p)[DGL_IN_STATUS_v2])
#define DGL_NODE_EDGESET_OFFSET_v2(p)		((p)[DGL_IN_EDGESET_OFFSET_v2])
#define DGL_NODE_ATTR_PTR_v2(p)				((p) + DGL_IN_ATTR_v2)

/*
 * Edgeset macros - addresses in a flat edge-area
 */
#define DGL_ILA_TOCNT_v2						0
#define DGL_ILA_SIZE_v2						1
#define DGL_ILA_TOARR_v2						DGL_ILA_SIZE_v2

#define DGL_EDGESET_SIZEOF_v2(C, lattr)  	(sizeof( dglInt32_t ) * ((C) + 1))
#define DGL_EDGESET_WSIZE_v2(C, lattr)		(DGL_EDGESET_SIZEOF_v2(C, lattr) / sizeof(dglInt32_t))
#define DGL_EDGESET_ALLOC_v2(C, lattr)   	(malloc(DGL_EDGESET_SIZEOF_v2(C, lattr)))
#define DGL_EDGESET_REALLOC_v2(P, C, lattr)	(realloc(P , DGL_EDGESET_SIZEOF_v2(C, lattr)))

#define DGL_EDGESET_EDGECOUNT_v2(p)			((p)[DGL_ILA_TOCNT_v2])
#define DGL_EDGESET_EDGEARRAY_PTR_v2(p)		((p) + DGL_ILA_TOARR_v2)
#define DGL_EDGESET_EDGE_PTR_v2(pgrp,p,i)	DGL_EDGEBUFFER_SHIFT_v2(pgrp,*((p) + DGL_ILA_TOARR_v2 + (i)))

/*
 * Edge macros - addresses in a flat edge
 */
#define DGL_IL_HEAD_OFFSET_v2					0
#define DGL_IL_TAIL_OFFSET_v2					1
#define DGL_IL_STATUS_v2						2
#define DGL_IL_COST_v2						3
#define DGL_IL_ID_v2							4
#define DGL_IL_ATTR_v2						5
#define DGL_IL_SIZE_v2						DGL_IL_ATTR_v2

#define DGL_EDGE_SIZEOF_v2( lattr ) 			(sizeof( dglInt32_t ) * DGL_IL_SIZE_v2 + (lattr))
#define DGL_EDGE_WSIZE_v2( lattr ) 			(DGL_EDGE_SIZEOF_v2( lattr ) / sizeof( dglInt32_t ))
#define DGL_EDGE_ALLOC_v2( lattr )  			(malloc( DGL_EDGE_SIZEOF_v2( lattr ) ))

#define DGL_EDGE_HEADNODE_OFFSET_v2(p)		((p)[DGL_IL_HEAD_OFFSET_v2])
#define DGL_EDGE_TAILNODE_OFFSET_v2(p)		((p)[DGL_IL_TAIL_OFFSET_v2])
#define DGL_EDGE_STATUS_v2(p)					((p)[DGL_IL_STATUS_v2])
#define DGL_EDGE_COST_v2(p)					((p)[DGL_IL_COST_v2])
#define DGL_EDGE_ID_v2(p)						((p)[DGL_IL_ID_v2])
#define DGL_EDGE_ATTR_PTR_v2(p)				((p) + DGL_IL_ATTR_v2)
#define DGL_EDGE_HEADNODE_ID_v2(pgrp,pl)		((pgrp->Flags&1)?\
												DGL_NODE_ID_v2(pgrp->pNodeBuffer+DGL_EDGE_HEADNODE_OFFSET_v2(pl)):\
												DGL_EDGE_HEADNODE_OFFSET_v2(pl))
#define DGL_EDGE_TAILNODE_ID_v2(pgrp,pl)		((pgrp->Flags&1)?\
												DGL_NODE_ID_v2(pgrp->pNodeBuffer+DGL_EDGE_TAILNODE_OFFSET_v2(pl)):\
												DGL_EDGE_TAILNODE_OFFSET_v2(pl))

/*
 * Scan a node buffer
 */
#define DGL_FOREACH_NODE_v2(pgrp,pn)			for((pn)=(dglInt32_t*)(pgrp)->pNodeBuffer;\
												(pgrp)->pNodeBuffer && (pn)<(dglInt32_t*)((pgrp)->pNodeBuffer+(pgrp)->iNodeBuffer);\
												(pn)+=DGL_NODE_WSIZE_v2((pgrp)->NodeAttrSize))
/*
 * Scan a edgeset
 */
#define DGL_FOREACH_EDGE_v2(pgrp,pla,pl,il)\
		for((il)=0, (pl)=DGL_EDGESET_EDGE_PTR_v2(pgrp,pla,il);\
			(il)<DGL_EDGESET_EDGECOUNT_v2(pla);\
			(il)++, (pl)=DGL_EDGESET_EDGE_PTR_v2(pgrp,pla,il))
/*
 * Node Buffer Utilities
 */
#define DGL_NODEBUFFER_SHIFT_v2(pgrp,o)		((dglInt32_t*)((pgrp)->pNodeBuffer + (o)))
#define DGL_NODEBUFFER_OFFSET_v2(pgrp,p)		((dglInt32_t)((dglByte_t *)p - (dglByte_t *)(pgrp)->pNodeBuffer))

/*
 * Edge Buffer Utilities
 */
#define DGL_EDGEBUFFER_SHIFT_v2(pgrp,o)		((dglInt32_t*)((pgrp)->pEdgeBuffer + (o)))
#define DGL_EDGEBUFFER_OFFSET_v2(pgrp,pl)		((dglInt32_t)((dglByte_t *)pl - (dglByte_t *)(pgrp)->pEdgeBuffer))




int dgl_add_edge_V2(dglGraph_s * pgraph,
		    dglInt32_t nHead,
		    dglInt32_t nTail,
		    dglInt32_t nCost,
		    dglInt32_t nEdge,
		    void *pvHeadAttr,
		    void *pvTailAttr, void *pvEdgeAttr, dglInt32_t nFlags);

int dgl_unflatten_V2(dglGraph_s * pgraph);
int dgl_flatten_V2(dglGraph_s * pgraph);
int dgl_initialize_V2(dglGraph_s * pgraph);
int dgl_release_V2(dglGraph_s * pgraph);
int dgl_write_V2(dglGraph_s * pgraph, int fd);
int dgl_read_V2(dglGraph_s * pgraph, int fd, int version);


int dgl_sp_cache_initialize_V2(dglGraph_s * pgraph, dglSPCache_s * pCache,
			       dglInt32_t nStart);
void dgl_sp_cache_release_V2(dglGraph_s * pgraph, dglSPCache_s * pCache);

int dgl_dijkstra_V2_TREE(dglGraph_s * pgraph,
			 dglSPReport_s ** ppReport,
			 dglInt32_t * pDistance,
			 dglInt32_t nStart,
			 dglInt32_t nDestination,
			 dglSPClip_fn fnClip,
			 void *pvClipArg, dglSPCache_s * pCache);
int dgl_dijkstra_V2_FLAT(dglGraph_s * pgraph,
			 dglSPReport_s ** ppReport,
			 dglInt32_t * pDistance,
			 dglInt32_t nStart,
			 dglInt32_t nDestination,
			 dglSPClip_fn fnClip,
			 void *pvClipArg, dglSPCache_s * pCache);
int dgl_dijkstra_V2(dglGraph_s * pgraph,
		    dglSPReport_s ** ppReport,
		    dglInt32_t * pDistance,
		    dglInt32_t nStart,
		    dglInt32_t nDestination,
		    dglSPClip_fn fnClip,
		    void *pvClipArg, dglSPCache_s * pCache);


int dgl_span_depthfirst_spanning_V2_TREE(dglGraph_s * pgraphIn,
					 dglGraph_s * pgraphOut,
					 dglInt32_t nVertex,
					 void *pvVisited,
					 dglSpanClip_fn fnClip,
					 void *pvClipArg);
int dgl_span_depthfirst_spanning_V2_FLAT(dglGraph_s * pgraphIn,
					 dglGraph_s * pgraphOut,
					 dglInt32_t nVertex,
					 void *pvVisited,
					 dglSpanClip_fn fnClip,
					 void *pvClipArg);
int dgl_depthfirst_spanning_V2(dglGraph_s * pgraphIn,
			       dglGraph_s * pgraphOut,
			       dglInt32_t nVertex,
			       void *pvVisited,
			       dglSpanClip_fn fnClip, void *pvClipArg);


int dgl_span_minimum_spanning_V2_TREE(dglGraph_s * pgraphIn,
				      dglGraph_s * pgraphOut,
				      dglInt32_t nVertex,
				      dglSpanClip_fn fnClip, void *pvClipArg);
int dgl_span_minimum_spanning_V2_FLAT(dglGraph_s * pgraphIn,
				      dglGraph_s * pgraphOut,
				      dglInt32_t nVertex,
				      dglSpanClip_fn fnClip, void *pvClipArg);
int dgl_minimum_spanning_V2(dglGraph_s * pgraphIn,
			    dglGraph_s * pgraphOut,
			    dglInt32_t nVertex,
			    dglSpanClip_fn fnClip, void *pvClipArg);


int dgl_add_node_V2(dglGraph_s * pgraph,
		    dglInt32_t nId, void *pvNodeAttr, dglInt32_t nFlags);
int dgl_del_node_outedge_V2(dglGraph_s * pgraph, dglInt32_t nNode,
			    dglInt32_t nEdge);
int dgl_del_node_inedge_V2(dglGraph_s * pgraph, dglInt32_t nNode,
			   dglInt32_t nEdge);
int dgl_del_node_V2(dglGraph_s * pgraph, dglInt32_t nId);
dglInt32_t *dgl_get_node_V2(dglGraph_s * pgraph, dglInt32_t nId);

dglInt32_t *dgl_get_edge_V2(dglGraph_s * pgraph, dglInt32_t nId);
int dgl_del_edge_V2(dglGraph_s * pgraph, dglInt32_t nId);

dglInt32_t *dgl_getnode_outedgeset_V2(dglGraph_s * pgraph,
				      dglInt32_t * pnode);
dglInt32_t *dgl_getnode_inedgeset_V2(dglGraph_s * pgraph, dglInt32_t * pnode);

/*
 * Node Traversing
 */
int dgl_node_t_initialize_V2(dglGraph_s * pGraph, dglNodeTraverser_s * pT);
void dgl_node_t_release_V2(dglNodeTraverser_s * pT);
dglInt32_t *dgl_node_t_first_V2(dglNodeTraverser_s * pT);
dglInt32_t *dgl_node_t_next_V2(dglNodeTraverser_s * pT);
dglInt32_t *dgl_node_t_find_V2(dglNodeTraverser_s * pT, dglInt32_t nId);


/*
 * Edgeset Traversing
 */
int dgl_edgeset_t_initialize_V2(dglGraph_s * pGraph,
				dglEdgesetTraverser_s * pTraverser,
				dglInt32_t * pnEdgeset);
void dgl_edgeset_t_release_V2(dglEdgesetTraverser_s * pTraverser);
dglInt32_t *dgl_edgeset_t_first_V2(dglEdgesetTraverser_s * pTraverser);
dglInt32_t *dgl_edgeset_t_next_V2(dglEdgesetTraverser_s * pTraverser);


int dgl_edge_t_initialize_V2(dglGraph_s * pGraph,
			     dglEdgeTraverser_s * pTraverser,
			     dglEdgePrioritizer_s * pEP);
void dgl_edge_t_release_V2(dglEdgeTraverser_s * pTraverser);
dglInt32_t *dgl_edge_t_first_V2(dglEdgeTraverser_s * pT);
dglInt32_t *dgl_edge_t_next_V2(dglEdgeTraverser_s * pT);


#endif

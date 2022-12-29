/*
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

#define _DGL_V1 	1
#undef  _DGL_V2

/*
 * Define function names
 */

#if defined(DGL_DEFINE_TREE_PROCS) || defined(DGL_DEFINE_FLAT_PROCS)
/* sp-template */
#undef DGL_SP_DIJKSTRA_FUNC
#undef DGL_SPAN_DEPTHFIRST_SPANNING_FUNC
#undef DGL_SPAN_MINIMUM_SPANNING_FUNC
#undef _DGL_OUTEDGESET
#undef _DGL_INEDGESET
#undef _DGL_EDGE_TAILNODE
#undef _DGL_EDGE_HEADNODE
#endif

/*
 * TREE version algorithms
 */
#if defined(DGL_DEFINE_TREE_PROCS)
/* sp-template */
#define DGL_SP_DIJKSTRA_FUNC				dgl_dijkstra_V1_TREE
/* span-template */
#define DGL_SPAN_DEPTHFIRST_SPANNING_FUNC	dgl_span_depthfirst_spanning_V1_TREE
#define DGL_SPAN_MINIMUM_SPANNING_FUNC	dgl_span_minimum_spanning_V1_TREE
/* portable actions */
#define _DGL_OUTEDGESET(pg,pn)			DGL_GET_NODE_OUTEDGESET_FUNC(pg,pn)
#define _DGL_INEDGESET(pg,pn)			DGL_GET_NODE_INEDGESET_FUNC(pg,pn)
#define _DGL_EDGE_HEADNODE(pg,pl)			DGL_GET_NODE_FUNC(pg, DGL_EDGE_HEADNODE_OFFSET(pl))
#define _DGL_EDGE_TAILNODE(pg,pl)			DGL_GET_NODE_FUNC(pg, DGL_EDGE_TAILNODE_OFFSET(pl))
#endif

/*
 * FLAT version algorithms
 */
#if defined(DGL_DEFINE_FLAT_PROCS)
/* sp-template */
#define DGL_SP_DIJKSTRA_FUNC				dgl_dijkstra_V1_FLAT
/* span-template */
#define DGL_SPAN_DEPTHFIRST_SPANNING_FUNC	dgl_span_depthfirst_spanning_V1_FLAT
#define DGL_SPAN_MINIMUM_SPANNING_FUNC	dgl_span_minimum_spanning_V1_FLAT
/* portable actions */
#define _DGL_OUTEDGESET(pg,pn)			DGL_EDGEBUFFER_SHIFT(pg, DGL_NODE_EDGESET_OFFSET(pn))
#define _DGL_INEDGESET(pg,pn)			((dglInt32_t*)(DGL_EDGEBUFFER_SHIFT(pg, DGL_NODE_EDGESET_OFFSET(pn))) + \
											*(dglInt32_t*)(DGL_EDGEBUFFER_SHIFT(pg, DGL_NODE_EDGESET_OFFSET(pn))) + 1)
#define _DGL_EDGE_HEADNODE(pg,pl)			DGL_NODEBUFFER_SHIFT(pg, DGL_EDGE_HEADNODE_OFFSET(pl))
#define _DGL_EDGE_TAILNODE(pg,pl)			DGL_NODEBUFFER_SHIFT(pg, DGL_EDGE_TAILNODE_OFFSET(pl))
#endif




#if !defined(DGL_DEFINE_TREE_PROCS) && !defined(DGL_DEFINE_FLAT_PROCS)

/* sp-template */
#define DGL_SP_CACHE_INITIALIZE_FUNC		dgl_sp_cache_initialize_V1
#define DGL_SP_CACHE_RELEASE_FUNC			dgl_sp_cache_release_V1
#define DGL_SP_CACHE_REPORT_FUNC			dgl_sp_cache_report_V1
#define DGL_SP_CACHE_DISTANCE_FUNC		dgl_sp_cache_distance_V1

/* nodemgmt-template */
#define DGL_ADD_NODE_FUNC					dgl_add_node_V1
#define DGL_DEL_NODE_FUNC					dgl_del_node_V1
#define DGL_GET_NODE_FUNC					dgl_get_node_V1
#define DGL_GET_NODE_OUTEDGESET_FUNC		dgl_getnode_outedgeset_V1

/* edgemgmt-template */
#define DGL_ADD_EDGE_FUNC					dgl_add_edge_V1
#define DGL_GET_EDGE_FUNC					dgl_get_edge_V1
#define DGL_DEL_EDGE_FUNC					dgl_del_edge_V1

/* misc-template */
#define DGL_EDGE_T_INITIALIZE_FUNC		dgl_edge_t_initialize_V1
#define DGL_EDGE_T_RELEASE_FUNC			dgl_edge_t_release_V1
#define DGL_EDGE_T_FIRST_FUNC				dgl_edge_t_first_V1
#define DGL_EDGE_T_NEXT_FUNC				dgl_edge_t_next_V1
#define DGL_NODE_T_INITIALIZE_FUNC		dgl_node_t_initialize_V1
#define DGL_NODE_T_RELEASE_FUNC			dgl_node_t_release_V1
#define DGL_NODE_T_FIRST_FUNC				dgl_node_t_first_V1
#define DGL_NODE_T_NEXT_FUNC				dgl_node_t_next_V1
#define DGL_NODE_T_FIND_FUNC				dgl_node_t_find_V1
#define DGL_EDGESET_T_INITIALIZE_FUNC		dgl_edgeset_t_initialize_V1
#define DGL_EDGESET_T_RELEASE_FUNC		dgl_edgeset_t_release_V1
#define DGL_EDGESET_T_FIRST_FUNC			dgl_edgeset_t_first_V1
#define DGL_EDGESET_T_NEXT_FUNC			dgl_edgeset_t_next_V1
#define DGL_FLATTEN_FUNC					dgl_flatten_V1
#define DGL_UNFLATTEN_FUNC				dgl_unflatten_V1



/*
 *
 */


/* Node
 */
#define DGL_NODE_ALLOC					DGL_NODE_ALLOC_v1
#define DGL_NODE_SIZEOF					DGL_NODE_SIZEOF_v1
#define DGL_NODE_WSIZE					DGL_NODE_WSIZE_v1
#define DGL_NODE_STATUS					DGL_NODE_STATUS_v1
#define DGL_NODE_ID						DGL_NODE_ID_v1
#define DGL_NODE_ATTR_PTR					DGL_NODE_ATTR_PTR_v1
#define DGL_NODE_EDGESET_OFFSET			DGL_NODE_EDGESET_OFFSET_v1

/* Edge
 */
#define DGL_EDGE_ALLOC					DGL_EDGE_ALLOC_v1
#define DGL_EDGE_SIZEOF					DGL_EDGE_SIZEOF_v1
#define DGL_EDGE_WSIZE					DGL_EDGE_WSIZE_v1
#define DGL_EDGE_STATUS(p)				0
#define DGL_EDGE_COST						DGL_EDGE_COST_v1
#define DGL_EDGE_ID						DGL_EDGE_ID_v1
#define DGL_EDGE_ATTR_PTR					DGL_EDGE_ATTR_PTR_v1
#define DGL_EDGE_HEADNODE_OFFSET			DGL_EDGE_HEADNODE_OFFSET_v1
#define DGL_EDGE_TAILNODE_OFFSET			DGL_EDGE_TAILNODE_OFFSET_v1

/* Edgeset
 */
#define DGL_ILA_TOARR						DGL_ILA_TOARR_v1
#define DGL_EDGESET_OFFSET				DGL_EDGESET_OFFSET_v1
#define DGL_EDGESET_EDGEARRAY_PTR		DGL_EDGESET_EDGEARRAY_PTR_v1
#define DGL_EDGESET_EDGECOUNT			DGL_EDGESET_EDGECOUNT_v1
#define DGL_EDGESET_EDGE_PTR				DGL_EDGESET_EDGE_PTR_v1
#define DGL_EDGESET_ALLOC				DGL_EDGESET_ALLOC_v1
#define DGL_EDGESET_REALLOC				DGL_EDGESET_REALLOC_v1
#define DGL_EDGESET_SIZEOF				DGL_EDGESET_SIZEOF_v1
#define DGL_EDGESET_WSIZE				DGL_EDGESET_WSIZE_v1

/* Misc
 */
#define DGL_NODEBUFFER_SHIFT				DGL_NODEBUFFER_SHIFT_v1
#define DGL_NODEBUFFER_OFFSET				DGL_NODEBUFFER_OFFSET_v1
#define DGL_EDGEBUFFER_SHIFT				DGL_EDGEBUFFER_SHIFT_v1
#define DGL_EDGEBUFFER_OFFSET				DGL_EDGEBUFFER_OFFSET_v1

#define DGL_FOREACH_NODE					DGL_FOREACH_NODE_v1
#define DGL_FOREACH_EDGE					DGL_FOREACH_EDGE_v1


/*
 * Tree-node portability
 */
#define DGL_T_NODEITEM_TYPE					dglTreeNode_s
#define DGL_T_NODEITEM_NodePTR(p)				((p)->pv)
#define DGL_T_NODEITEM_Set_NodePTR(p,ptr)		((p)->pv=(ptr))
#define DGL_T_NODEITEM_OutEdgesetPTR(p)		((p)->pv2)
#define DGL_T_NODEITEM_Set_OutEdgesetPTR(p,ptr)	((p)->pv2=(ptr))
#define DGL_T_NODEITEM_InEdgesetPTR(p)		NULL
#define DGL_T_NODEITEM_Set_InEdgesetPTR(p,ptr)
#define DGL_T_NODEITEM_Compare				dglTreeNodeCompare
#define DGL_T_NODEITEM_Cancel					dglTreeNodeCancel
#define DGL_T_NODEITEM_Add					dglTreeNodeAdd
#define DGL_T_NODEITEM_Alloc					dglTreeNodeAlloc



#endif

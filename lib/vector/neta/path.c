/*!
   \file vector/neta/path.c

   \brief Network Analysis library - shortest path

   Shortest paths from a set of nodes.

   (C) 2009-2010 by Daniel Bundala, and the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Daniel Bundala (Google Summer of Code 2009)
   \author Markus Metz
 */

#include <stdio.h>
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>
#include <grass/dgl/graph.h>
#include <grass/neta.h>

/*!
   \brief Computes shortest paths to every node from nodes in "from".

   Array "dst" contains the cost of the path or -1 if the node is not
   reachable. Prev contains edges from predecessor along the shortest
   path.

   \param graph input graph
   \param from list of 'from' positions
   \param[out] dst array of costs to reach nodes
   \param[out] prev array of edges from predecessor along the shortest path

   \return 0 on success
   \return -1 on failure
 */
int NetA_distance_from_points(dglGraph_s *graph, struct ilist *from,
			      int *dst, dglInt32_t **prev)
{
    int i, nnodes;
    dglHeap_s heap;
    int have_node_costs;
    dglInt32_t ncost;

    nnodes = dglGet_NodeCount(graph);
    dglEdgesetTraverser_s et;

    /* initialize costs and edge list */
    for (i = 1; i <= nnodes; i++) {
	dst[i] = -1;
	prev[i] = NULL;
    }

    ncost = 0;
    have_node_costs = dglGet_NodeAttrSize(graph);

    dglHeapInit(&heap);

    for (i = 0; i < from->n_values; i++) {
	int v = from->value[i];

	if (dst[v] == 0)
	    continue;		/* ignore duplicates */
	dst[v] = 0;		/* make sure all from nodes are processed first */
	dglHeapData_u heap_data;

	heap_data.ul = v;
	dglHeapInsertMin(&heap, 0, ' ', heap_data);
    }
    while (1) {
	dglInt32_t v, dist;
	dglHeapNode_s heap_node;
	dglHeapData_u heap_data;
	dglInt32_t *edge;
	dglInt32_t *node;

	if (!dglHeapExtractMin(&heap, &heap_node))
	    break;
	v = heap_node.value.ul;
	dist = heap_node.key;
	if (dst[v] < dist)
	    continue;

	node = dglGetNode(graph, v);

	if (have_node_costs && prev[v]) {
	    memcpy(&ncost, dglNodeGet_Attr(graph, node),
		   sizeof(ncost));
	    if (ncost > 0)
		dist += ncost;
	    /* do not go through closed nodes */
	    if (ncost < 0)
		continue;
	}

	dglEdgeset_T_Initialize(&et, graph,
				dglNodeGet_OutEdgeset(graph, node));

	for (edge = dglEdgeset_T_First(&et); edge;
	     edge = dglEdgeset_T_Next(&et)) {
	    dglInt32_t *to = dglEdgeGet_Tail(graph, edge);
	    dglInt32_t to_id = dglNodeGet_Id(graph, to);
	    dglInt32_t d = dglEdgeGet_Cost(graph, edge);

	    if (dst[to_id] < 0 || dst[to_id] > dist + d) {
		dst[to_id] = dist + d;
		prev[to_id] = edge;
		heap_data.ul = to_id;
		dglHeapInsertMin(&heap, dist + d, ' ', heap_data);
	    }
	}

	dglEdgeset_T_Release(&et);
    }

    dglHeapFree(&heap, NULL);

    return 0;
}

/*!
   \brief Computes shortest paths from every node to nodes in "to".

   Array "dst" contains the cost of the path or -1 if the node is not
   reachable. Nxt contains edges from successor along the shortest
   path. This method does reverse search starting with "to" nodes and 
   going backward.

   \param graph input graph
   \param to list of 'to' positions
   \param[out] dst array of costs to reach nodes
   \param[out] nxt array of edges from successor along the shortest path

   \return 0 on success
   \return -1 on failure
 */
int NetA_distance_to_points(dglGraph_s *graph, struct ilist *to,
			      int *dst, dglInt32_t **nxt)
{
    int i, nnodes;
    dglHeap_s heap;
    dglEdgesetTraverser_s et;
    int have_node_costs;
    dglInt32_t ncost;

    nnodes = dglGet_NodeCount(graph);

    /* initialize costs and edge list */
    for (i = 1; i <= nnodes; i++) {
	dst[i] = -1;
	nxt[i] = NULL;
    }

    if (graph->Version < 2) {
	G_warning("Directed graph must be version 2 or 3 for NetA_distance_to_points()");
	return -1;
    }

    ncost = 0;
    have_node_costs = dglGet_NodeAttrSize(graph);

    dglHeapInit(&heap);

    for (i = 0; i < to->n_values; i++) {
	int v = to->value[i];

	if (dst[v] == 0)
	    continue;		/* ignore duplicates */
	dst[v] = 0;		/* make sure all to nodes are processed first */
	dglHeapData_u heap_data;

	heap_data.ul = v;
	dglHeapInsertMin(&heap, 0, ' ', heap_data);
    }
    while (1) {
	dglInt32_t v, dist;
	dglHeapNode_s heap_node;
	dglHeapData_u heap_data;
	dglInt32_t *edge;
	dglInt32_t *node;

	if (!dglHeapExtractMin(&heap, &heap_node))
	    break;
	v = heap_node.value.ul;
	dist = heap_node.key;
	if (dst[v] < dist)
	    continue;

	node = dglGetNode(graph, v);

	if (have_node_costs && nxt[v]) {
	    memcpy(&ncost, dglNodeGet_Attr(graph, node),
		   sizeof(ncost));
	    if (ncost > 0)
		dist += ncost;
	    /* do not go through closed nodes */
	    if (ncost < 0)
		continue;
	}

	dglEdgeset_T_Initialize(&et, graph,
				dglNodeGet_InEdgeset(graph, node));

	for (edge = dglEdgeset_T_First(&et); edge;
	     edge = dglEdgeset_T_Next(&et)) {
	    dglInt32_t *from = dglEdgeGet_Head(graph, edge);
	    dglInt32_t from_id = dglNodeGet_Id(graph, from);
	    dglInt32_t d = dglEdgeGet_Cost(graph, edge);

	    if (dst[from_id] < 0 || dst[from_id] > dist + d) {
		dst[from_id] = dist + d;
		nxt[from_id] = edge;
		heap_data.ul = from_id;
		dglHeapInsertMin(&heap, dist + d, ' ', heap_data);
	    }
	}

	dglEdgeset_T_Release(&et);
    }

    dglHeapFree(&heap, NULL);

    return 0;
}

/*!
   \brief Find a path (minimum number of edges) from 'from' to 'to' 
   using only edges flagged as valid in 'edges'. Edge costs are not 
   considered. Closed nodes are not traversed.

   Precisely, edge with id I is used if edges[abs(i)] == 1. List
   stores the indices of lines on the path. The method returns the 
   number of edges or -1 if no path exists.

   \param graph input graph
   \param from 'from' position
   \param to 'to' position
   \param edges array of edges indicating whether an edge should be used
   \param[out] list list of edges

   \return number of edges
   \return -1 on failure
 */
int NetA_find_path(dglGraph_s * graph, int from, int to, int *edges,
		   struct ilist *list)
{
    dglInt32_t **prev, *queue;
    dglEdgesetTraverser_s et;
    char *vis;
    int begin, end, cur, nnodes;
    int have_node_costs;
    dglInt32_t ncost;

    nnodes = dglGet_NodeCount(graph);
    prev = (dglInt32_t **) G_calloc(nnodes + 1, sizeof(dglInt32_t *));
    queue = (dglInt32_t *) G_calloc(nnodes + 1, sizeof(dglInt32_t));
    vis = (char *)G_calloc(nnodes + 1, sizeof(char));
    if (!prev || !queue || !vis) {
	G_fatal_error(_("Out of memory"));
	return -1;
    }
    Vect_reset_list(list);

    ncost = 0;
    have_node_costs = dglGet_NodeAttrSize(graph);

    begin = 0;
    end = 1;
    vis[from] = 'y';
    queue[0] = from;
    prev[from] = NULL;
    while (begin != end) {
	dglInt32_t vertex = queue[begin++];
	dglInt32_t *edge, *node;

	if (vertex == to)
	    break;

	/* do not go through closed nodes */
	if (have_node_costs && prev[vertex]) {
	    memcpy(&ncost, dglNodeGet_Attr(graph, dglEdgeGet_Tail(graph, edge)),
		   sizeof(ncost));
	    if (ncost < 0)
		continue;
	}

	node = dglGetNode(graph, vertex);

	dglEdgeset_T_Initialize(&et, graph,
				dglNodeGet_OutEdgeset(graph, node));
	for (edge = dglEdgeset_T_First(&et); edge;
	     edge = dglEdgeset_T_Next(&et)) {
	    dglInt32_t edge_id = labs(dglEdgeGet_Id(graph, edge));
	    dglInt32_t node_id =
		dglNodeGet_Id(graph, dglEdgeGet_Tail(graph, edge));
	    if (edges[edge_id] && !vis[node_id]) {
		vis[node_id] = 'y';
		prev[node_id] = edge;
		queue[end++] = node_id;
	    }
	}
	dglEdgeset_T_Release(&et);
    }
    G_free(queue);
    if (!vis[to]) {
	G_free(prev);
	G_free(vis);
	return -1;
    }

    cur = to;
    while (prev[cur] != NULL) {
	Vect_list_append(list, labs(dglEdgeGet_Id(graph, prev[cur])));
	cur = dglNodeGet_Id(graph, dglEdgeGet_Head(graph, prev[cur]));
    }

    G_free(prev);
    G_free(vis);
    return list->n_values;
}

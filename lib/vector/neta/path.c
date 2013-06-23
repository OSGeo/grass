/*!
   \file vector/neta/path.c

   \brief Network Analysis library - shortest path

   Shortest paths from a set of nodes.

   (C) 2009-2010 by Daniel Bundala, and the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Daniel Bundala (Google Summer of Code 2009)
 */

#include <stdio.h>
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>
#include <grass/dgl/graph.h>
#include <grass/neta.h>

/*!
   \brief Computes shortests paths to every node from nodes in "from".

   Array "dst" contains the length of the path or -1 if the node is not
   reachable. Prev contains edges from predecessor along the shortest
   path.

   \param graph input graph
   \param from list of 'from' positions
   \param dst list of 'to' positions
   \param[out] prev list of edges from predecessor along the shortest path

   \return 0 on success
   \return -1 on failure
 */
int NetA_distance_from_points(dglGraph_s *graph, struct ilist *from,
			      int *dst, dglInt32_t **prev)
{
    int i, nnodes;
    dglHeap_s heap;

    nnodes = dglGet_NodeCount(graph);
    dglEdgesetTraverser_s et;

    /* initialize costs and edge list */
    for (i = 1; i <= nnodes; i++) {
	dst[i] = -1;
	prev[i] = NULL;
    }

    dglHeapInit(&heap);

    for (i = 0; i < from->n_values; i++) {
	int v = from->value[i];

	if (dst[v] == 0)
	    continue;		/*ingore duplicates */
	dst[v] = 0;		/* make sure all from nodes are processed first */
	dglHeapData_u heap_data;

	heap_data.ul = v;
	dglHeapInsertMin(&heap, 0, ' ', heap_data);
    }
    while (1) {
	dglInt32_t v, dist;
	dglHeapNode_s heap_node;
	dglHeapData_u heap_data;

	if (!dglHeapExtractMin(&heap, &heap_node))
	    break;
	v = heap_node.value.ul;
	dist = heap_node.key;
	if (dst[v] < dist)
	    continue;

	dglInt32_t *edge;

	dglEdgeset_T_Initialize(&et, graph,
				dglNodeGet_OutEdgeset(graph,
						      dglGetNode(graph, v)));

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
   \brief Find a path (minimum number of edges) from 'from' to 'to' using only edges in 'edges'.

   Precisely, edge with id I is used if edges[abs(i)] == 1. List
   stores the indices of lines on the path. Method return number of
   edges or -1 if no path exist.

   \param graph input graph
   \param from 'from' position
   \param to 'to' position
   \param edges list of available edges
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

    nnodes = dglGet_NodeCount(graph);
    prev = (dglInt32_t **) G_calloc(nnodes + 1, sizeof(dglInt32_t *));
    queue = (dglInt32_t *) G_calloc(nnodes + 1, sizeof(dglInt32_t));
    vis = (char *)G_calloc(nnodes + 1, sizeof(char));
    if (!prev || !queue || !vis) {
	G_fatal_error(_("Out of memory"));
	return -1;
    }
    Vect_reset_list(list);

    begin = 0;
    end = 1;
    vis[from] = 'y';
    queue[0] = from;
    prev[from] = NULL;
    while (begin != end) {
	dglInt32_t vertex = queue[begin++];

	if (vertex == to)
	    break;
	dglInt32_t *edge, *node = dglGetNode(graph, vertex);

	dglEdgeset_T_Initialize(&et, graph,
				dglNodeGet_OutEdgeset(graph, node));
	for (edge = dglEdgeset_T_First(&et); edge;
	     edge = dglEdgeset_T_Next(&et)) {
	    dglInt32_t id = abs(dglEdgeGet_Id(graph, edge));
	    dglInt32_t to =
		dglNodeGet_Id(graph, dglEdgeGet_Tail(graph, edge));
	    if (edges[id] && !vis[to]) {
		vis[to] = 'y';
		prev[to] = edge;
		queue[end++] = to;
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
	Vect_list_append(list, abs(dglEdgeGet_Id(graph, prev[cur])));
	cur = dglNodeGet_Id(graph, dglEdgeGet_Head(graph, prev[cur]));
    }

    G_free(prev);
    G_free(vis);
    return list->n_values;
}

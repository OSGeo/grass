/*!
   \file vector/neta/flow.c

   \brief Network Analysis library - flow in graph

   Computes the length of the shortest path between all pairs of nodes
   in the network.

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

dglInt32_t sign(dglInt32_t x)
{
    if (x >= 0)
	return 1;
    return -1;
}

/*!
   \brief Get max flow from source to sink.

   Array flow stores flow for each edge. Negative flow corresponds to a
   flow in opposite direction. The function assumes that the edge costs
   correspond to edge capacities.

   \param graph input graph
   \param source_list list of sources
   \param sink_list list of sinks
   \param[out] flow max flows

   \return number of flows
   \return -1 on failure
 */
int NetA_flow(dglGraph_s * graph, struct ilist *source_list,
	      struct ilist *sink_list, int *flow)
{
    int nnodes, nlines, i;
    dglEdgesetTraverser_s et;
    dglInt32_t *queue;
    dglInt32_t **prev;
    char *is_source, *is_sink;
    int begin, end, total_flow;
    int have_node_costs;
    dglInt32_t ncost;

    nnodes = dglGet_NodeCount(graph);
    nlines = dglGet_EdgeCount(graph) / 2;	/*each line corresponds to two edges. One in each direction */
    queue = (dglInt32_t *) G_calloc(nnodes + 3, sizeof(dglInt32_t));
    prev = (dglInt32_t **) G_calloc(nnodes + 3, sizeof(dglInt32_t *));
    is_source = (char *)G_calloc(nnodes + 3, sizeof(char));
    is_sink = (char *)G_calloc(nnodes + 3, sizeof(char));
    if (!queue || !prev || !is_source || !is_sink) {
	G_fatal_error(_("Out of memory"));
	return -1;
    }

    for (i = 0; i < source_list->n_values; i++)
	is_source[source_list->value[i]] = 1;
    for (i = 0; i < sink_list->n_values; i++)
	is_sink[sink_list->value[i]] = 1;

    for (i = 0; i <= nlines; i++)
	flow[i] = 0;

    ncost = 0;
    have_node_costs = dglGet_NodeAttrSize(graph);

    total_flow = 0;
    while (1) {
	dglInt32_t node, edge_id, min_residue;
	int found = -1;

	begin = end = 0;
	for (i = 0; i < source_list->n_values; i++)
	    queue[end++] = source_list->value[i];

	for (i = 1; i <= nnodes; i++) {
	    prev[i] = NULL;
	}
	while (begin != end && found == -1) {
	    dglInt32_t vertex = queue[begin++];
	    dglInt32_t *edge, *node = dglGetNode(graph, vertex);

	    dglEdgeset_T_Initialize(&et, graph,
				    dglNodeGet_OutEdgeset(graph, node));
	    for (edge = dglEdgeset_T_First(&et); edge;
		 edge = dglEdgeset_T_Next(&et)) {
		dglInt32_t cap = dglEdgeGet_Cost(graph, edge);
		dglInt32_t id = dglEdgeGet_Id(graph, edge);
		dglInt32_t to =
		    dglNodeGet_Id(graph, dglEdgeGet_Tail(graph, edge));
		if (!is_source[to] && prev[to] == NULL &&
		    cap > sign(id) * flow[abs(id)]) {
		    prev[to] = edge;
		    if (is_sink[to]) {
			found = to;
			break;
		    }
		    /* do not go through closed nodes */
		    if (have_node_costs) {
			memcpy(&ncost, dglNodeGet_Attr(graph, dglEdgeGet_Tail(graph, edge)),
			       sizeof(ncost));
		    }
		    if (ncost >= 0)
			queue[end++] = to;
		}
	    }
	    dglEdgeset_T_Release(&et);
	}
	if (found == -1)
	    break;		/*no augmenting path */
	/*find minimum residual capacity along the augmenting path */
	node = found;
	edge_id = dglEdgeGet_Id(graph, prev[node]);
	min_residue =
	    dglEdgeGet_Cost(graph,
			    prev[node]) - sign(edge_id) * flow[abs(edge_id)];
	while (!is_source[node]) {
	    dglInt32_t residue;

	    edge_id = dglEdgeGet_Id(graph, prev[node]);
	    residue =
		dglEdgeGet_Cost(graph,
				prev[node]) -
		sign(edge_id) * flow[abs(edge_id)];
	    if (residue < min_residue)
		min_residue = residue;
	    node = dglNodeGet_Id(graph, dglEdgeGet_Head(graph, prev[node]));
	}
	total_flow += min_residue;
	/*update flow along the augmenting path */
	node = found;
	while (!is_source[node]) {
	    edge_id = dglEdgeGet_Id(graph, prev[node]);
	    flow[abs(edge_id)] += sign(edge_id) * min_residue;
	    node = dglNodeGet_Id(graph, dglEdgeGet_Head(graph, prev[node]));
	}
    }

    G_free(queue);
    G_free(prev);
    G_free(is_source);
    G_free(is_sink);

    return total_flow;
}

/*!
   \brief Calculates minimum cut between source(s) and sink(s).

   Flow is the array produced by NetA_flow() method when called with
   source_list and sink_list as the input. The output of this and
   NetA_flow() method should be the same.

   \param graph input graph
   \param source_list list of sources
   \param sink_list list of sinks
   \param flow
   \param[out] cut list of edges (cut)

   \return number of edges
   \return -1 on failure
 */
int NetA_min_cut(dglGraph_s * graph, struct ilist *source_list,
		 struct ilist *sink_list, int *flow, struct ilist *cut)
{
    int nnodes, i;
    dglEdgesetTraverser_s et;
    dglInt32_t *queue;
    char *visited;
    int begin, end, total_flow;

    nnodes = dglGet_NodeCount(graph);
    queue = (dglInt32_t *) G_calloc(nnodes + 3, sizeof(dglInt32_t));
    visited = (char *)G_calloc(nnodes + 3, sizeof(char));
    if (!queue || !visited) {
	G_fatal_error(_("Out of memory"));
	return -1;
    }

    total_flow = begin = end = 0;

    for (i = 1; i <= nnodes; i++)
	visited[i] = 0;

    for (i = 0; i < source_list->n_values; i++) {
	queue[end++] = source_list->value[i];
	visited[source_list->value[i]] = 1;
    }

    /* find vertices reachable from source(s) using only non-saturated edges */
    while (begin != end) {
	dglInt32_t vertex = queue[begin++];
	dglInt32_t *edge, *node = dglGetNode(graph, vertex);

	dglEdgeset_T_Initialize(&et, graph,
				dglNodeGet_OutEdgeset(graph, node));
	for (edge = dglEdgeset_T_First(&et); edge;
	     edge = dglEdgeset_T_Next(&et)) {
	    dglInt32_t cap = dglEdgeGet_Cost(graph, edge);
	    dglInt32_t id = dglEdgeGet_Id(graph, edge);
	    dglInt32_t to =
		dglNodeGet_Id(graph, dglEdgeGet_Tail(graph, edge));
	    if (!visited[to] && cap > sign(id) * flow[abs(id)]) {
		visited[to] = 1;
		queue[end++] = to;
	    }
	}
	dglEdgeset_T_Release(&et);
    }
    /*saturated edges from reachable vertices to non-reachable ones form a minimum cost */
    Vect_reset_list(cut);
    for (i = 1; i <= nnodes; i++) {
	if (!visited[i])
	    continue;
	dglInt32_t *node, *edgeset, *edge;

	node = dglGetNode(graph, i);
	edgeset = dglNodeGet_OutEdgeset(graph, node);
	dglEdgeset_T_Initialize(&et, graph, edgeset);
	for (edge = dglEdgeset_T_First(&et); edge;
	     edge = dglEdgeset_T_Next(&et)) {
	    dglInt32_t to, edge_id;

	    to = dglNodeGet_Id(graph, dglEdgeGet_Tail(graph, edge));
	    edge_id = abs(dglEdgeGet_Id(graph, edge));
	    if (!visited[to] && flow[edge_id] != 0) {
		Vect_list_append(cut, edge_id);
		total_flow += abs(flow[abs(edge_id)]);
	    }
	}
	dglEdgeset_T_Release(&et);
    }

    G_free(visited);
    G_free(queue);
    return total_flow;
}

/*!
   \brief Splits each vertex of in graph into two vertices

   The method splits each vertex of in graph into two vertices: in
   vertex and out vertex. Also, it adds an edge from an in vertex to
   the corresponding out vertex (capacity=2) and it adds an edge from
   out vertex to in vertex for each edge present in the in graph
   (forward capacity=1, backward capacity=0). If the id of a vertex is
   v then id of in vertex is 2*v-1 and of out vertex 2*v.

   \param in from graph
   \param out to graph
   \param node_costs list of node costs

   \return number of undirected edges in the graph
   \return -1 on failure
 */
int NetA_split_vertices(dglGraph_s * in, dglGraph_s * out, int *node_costs)
{
    dglInt32_t opaqueset[16] =
	{ 360000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    dglNodeTraverser_s nt;
    dglInt32_t nnodes, edge_cnt;
    dglInt32_t *cur_node;

    nnodes = dglGet_NodeCount(in);
    dglInitialize(out, (dglByte_t) 1, (dglInt32_t) 0, (dglInt32_t) 0,
		  opaqueset);
    dglNode_T_Initialize(&nt, in);
    edge_cnt = 0;
    dglInt32_t max_node_cost = 0;

    for (cur_node = dglNode_T_First(&nt); cur_node;
	 cur_node = dglNode_T_Next(&nt)) {
	dglInt32_t v = dglNodeGet_Id(in, cur_node);

	edge_cnt++;
	dglInt32_t cost = 1;

	if (node_costs)
	    cost = node_costs[v];
	/* skip closed nodes */
	if (cost < 0)
	    continue;
	if (cost > max_node_cost)
	    max_node_cost = cost;
	dglAddEdge(out, 2 * v - 1, 2 * v, cost, edge_cnt);
	dglAddEdge(out, 2 * v, 2 * v - 1, (dglInt32_t) 0, -edge_cnt);
    }
    dglNode_T_Release(&nt);
    dglNode_T_Initialize(&nt, in);
    for (cur_node = dglNode_T_First(&nt); cur_node;
	 cur_node = dglNode_T_Next(&nt)) {
	dglEdgesetTraverser_s et;
	dglInt32_t *edge;
	dglInt32_t v = dglNodeGet_Id(in, cur_node);
	dglInt32_t cost = 1;

	if (node_costs)
	    cost = node_costs[v];
	/* skip closed nodes */
	if (cost < 0)
	    continue;

	dglEdgeset_T_Initialize(&et, in, dglNodeGet_OutEdgeset(in, cur_node));
	for (edge = dglEdgeset_T_First(&et); edge;
	     edge = dglEdgeset_T_Next(&et)) {
	    dglInt32_t to;

	    to = dglNodeGet_Id(in, dglEdgeGet_Tail(in, edge));
	    edge_cnt++;
	    dglAddEdge(out, 2 * v, 2 * to - 1, max_node_cost + 1, edge_cnt);
	    dglAddEdge(out, 2 * to - 1, 2 * v, (dglInt32_t) 0, -edge_cnt);
	}
	dglEdgeset_T_Release(&et);
    }
    dglNode_T_Release(&nt);
    if (dglFlatten(out) < 0)
	G_fatal_error(_("GngFlatten error"));
    return edge_cnt;
}

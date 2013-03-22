/*!
   \file lib/vector/neta/bridge.c

   \brief Network Analysis library - bridges

   Computes number of bridges in the graph.

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

/*!
   \brief Get number of bridges in the graph.

   Bridge is an array containing the indices of the bridges.

   \param graph input graph
   \param[out] bridge_list list of bridges

   \return number of bridges, -1 on error
 */
int NetA_compute_bridges(dglGraph_s * graph, struct ilist *bridge_list)
{
    int nnodes;
    int bridges = 0;

    dglEdgesetTraverser_s *current;	/*edge to be processed when the node is visited */
    int *tin, *min_tin;		/*time in, and smallest tin over all successors. 0 if not yet visited */
    dglInt32_t *parent;		/*edge from parent to the node */
    dglInt32_t **stack;		/*stack of nodes */
    dglInt32_t **current_edge;	/*current edge for each node */
    dglNodeTraverser_s nt;
    dglInt32_t *current_node;
    int stack_size;
    int i, time;

    nnodes = dglGet_NodeCount(graph);
    current =
	(dglEdgesetTraverser_s *) G_calloc(nnodes + 1,
					   sizeof(dglEdgesetTraverser_s));
    tin = (int *)G_calloc(nnodes + 1, sizeof(int));
    min_tin = (int *)G_calloc(nnodes + 1, sizeof(int));
    parent = (dglInt32_t *) G_calloc(nnodes + 1, sizeof(dglInt32_t));
    stack = (dglInt32_t **) G_calloc(nnodes + 1, sizeof(dglInt32_t *));
    current_edge = (dglInt32_t **) G_calloc(nnodes + 1, sizeof(dglInt32_t *));
    if (!tin || !min_tin || !parent || !stack || !current) {
	G_fatal_error(_("Out of memory"));
	return -1;
    }

    for (i = 1; i <= nnodes; i++) {
	dglEdgeset_T_Initialize(&current[i], graph,
				dglNodeGet_OutEdgeset(graph,
						      dglGetNode(graph, i)));
	current_edge[i] = dglEdgeset_T_First(&current[i]);
	tin[i] = 0;
    }

    dglNode_T_Initialize(&nt, graph);

    time = 0;
    for (current_node = dglNode_T_First(&nt); current_node;
	 current_node = dglNode_T_Next(&nt)) {
	dglInt32_t current_id = dglNodeGet_Id(graph, current_node);

	if (tin[current_id] == 0) {
	    stack[0] = current_node;
	    stack_size = 1;
	    parent[current_id] = 0;
	    while (stack_size) {
		dglInt32_t *node = stack[stack_size - 1];
		dglInt32_t node_id = dglNodeGet_Id(graph, node);

		if (tin[node_id] == 0)	/*vertex visited for the first time */
		    min_tin[node_id] = tin[node_id] = ++time;
		else {		/*return from the recursion */
		    dglInt32_t to = dglNodeGet_Id(graph,
						  dglEdgeGet_Tail(graph,
								  current_edge
								  [node_id]));
		    if (min_tin[to] > tin[node_id]) {	/*no path from the subtree above the current node */
			Vect_list_append(bridge_list, dglEdgeGet_Id(graph, current_edge[node_id]));	/*so it must be a bridge */
			bridges++;
		    }
		    if (min_tin[to] < min_tin[node_id])
			min_tin[node_id] = min_tin[to];
		    current_edge[node_id] = dglEdgeset_T_Next(&current[node_id]);	/*proceed to the next edge */
		}
		for (; current_edge[node_id]; current_edge[node_id] = dglEdgeset_T_Next(&current[node_id])) {	/*try next edges */
		    dglInt32_t *to =
			dglEdgeGet_Tail(graph, current_edge[node_id]);
		    dglInt32_t edge_id =
			dglEdgeGet_Id(graph, current_edge[node_id]);
		    if (abs(edge_id) == parent[node_id])
			continue;	/*skip edge we used to travel to this node */
		    int to_id = dglNodeGet_Id(graph, to);

		    if (tin[to_id]) {	/*back edge, cannot be a bridge/articualtion point */
			if (tin[to_id] < min_tin[node_id])
			    min_tin[node_id] = tin[to_id];
		    }
		    else {	/*forward edge */
			parent[to_id] = abs(edge_id);
			stack[stack_size++] = to;
			break;
		    }
		}
		if (!current_edge[node_id])
		    stack_size--;	/*current node completely processed */
	    }
	}
    }

    dglNode_T_Release(&nt);
    for (i = 1; i <= nnodes; i++)
	dglEdgeset_T_Release(&current[i]);

    G_free(current);
    G_free(tin);
    G_free(min_tin);
    G_free(parent);
    G_free(stack);
    G_free(current_edge);
    return bridges;
}

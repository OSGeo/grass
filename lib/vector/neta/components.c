/*!
   \file vector/neta/components.c

   \brief Network Analysis library - graph componets

   Computes strongly and weakly connected components.

   (C) 2009-2010 by Daniel Bundala, and the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Daniel Bundala (Google Summer of Code 2009)
   \author Markus Metz
 */

/* example:
 * 
 * X -->-- X ---- X --<-- X ---- X
 * N1      N2     N3      N4     N5
 * 
 * -->--, --<-- one-way
 * ---- both ways
 * 
 * weakly connected:
 * all 5 nodes, even though there is no direct path from N1 to N4, 5
 * but N1 connects to N2, 3, and N4, 5 also connect to N2, 3
 * 
 * strongly connected:
 * no path from N2 to N1, no path from N3 to N4
 * component 1: N1
 * component 2: N2, 3
 * Component3: N4, 5
 */

#include <stdio.h>
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>
#include <grass/dgl/graph.h>

/*!
   \brief Computes weakly connected components

   \param graph input graph
   \param[out] component array of component ids

   \return number of components
   \return -1 on failure
 */
int NetA_weakly_connected_components(dglGraph_s * graph, int *component)
{
    int nnodes, i;
    dglInt32_t *stack;
    int stack_size, components;
    dglInt32_t *cur_node;
    dglNodeTraverser_s nt;
    int have_node_costs;
    dglInt32_t ncost;

    if (graph->Version < 2) {
	G_warning("Directed graph must be version 2 or 3 for NetA_weakly_connected_components()");
	return -1;
    }

    components = 0;
    nnodes = dglGet_NodeCount(graph);
    stack = (dglInt32_t *) G_calloc(nnodes + 1, sizeof(dglInt32_t));
    if (!stack) {
	G_fatal_error(_("Out of memory"));
	return -1;
    }

    for (i = 1; i <= nnodes; i++)
	component[i] = 0;

    ncost = 0;
    have_node_costs = dglGet_NodeAttrSize(graph);

    dglNode_T_Initialize(&nt, graph);

    for (cur_node = dglNode_T_First(&nt); cur_node;
	 cur_node = dglNode_T_Next(&nt)) {
	dglInt32_t cur_node_id = dglNodeGet_Id(graph, cur_node);

	if (!component[cur_node_id]) {
	    stack[0] = cur_node_id;
	    stack_size = 1;
	    component[cur_node_id] = ++components;
	    while (stack_size) {
		dglInt32_t *node, *edgeset, *edge;
		dglEdgesetTraverser_s et;

		node = dglGetNode(graph, stack[--stack_size]);
		edgeset = dglNodeGet_OutEdgeset(graph, node);
		dglEdgeset_T_Initialize(&et, graph, edgeset);
		for (edge = dglEdgeset_T_First(&et); edge;
		     edge = dglEdgeset_T_Next(&et)) {
		    dglInt32_t to;

		    to = dglNodeGet_Id(graph, dglEdgeGet_Tail(graph, edge));
		    if (!component[to]) {
			component[to] = components;
			/* do not go through closed nodes */
			if (have_node_costs) {
			    memcpy(&ncost, dglNodeGet_Attr(graph, dglEdgeGet_Tail(graph, edge)),
				   sizeof(ncost));
			}
			if (ncost >= 0)
			    stack[stack_size++] = to;
		    }
		}
		dglEdgeset_T_Release(&et);

		edgeset = dglNodeGet_InEdgeset(graph, node);
		dglEdgeset_T_Initialize(&et, graph, edgeset);
		for (edge = dglEdgeset_T_First(&et); edge;
		     edge = dglEdgeset_T_Next(&et)) {
		    dglInt32_t to;

		    to = dglNodeGet_Id(graph, dglEdgeGet_Head(graph, edge));
		    if (!component[to]) {
			component[to] = components;
			/* do not go through closed nodes */
			if (have_node_costs) {
			    memcpy(&ncost, dglNodeGet_Attr(graph, dglEdgeGet_Tail(graph, edge)),
				   sizeof(ncost));
			}
			if (ncost >= 0)
			    stack[stack_size++] = to;
		    }
		}
		dglEdgeset_T_Release(&et);
	    }
	}
    }
    dglNode_T_Release(&nt);

    G_free(stack);
    return components;
}

/*!
   \brief Computes strongly connected components with Kosaraju's 
   two-pass algorithm

   \param graph input graph
   \param[out] component array of component ids

   \return number of components
   \return -1 on failure
 */
int NetA_strongly_connected_components(dglGraph_s * graph, int *component)
{
    int nnodes, i;
    dglInt32_t *stack, *order;
    int *processed;
    int stack_size, order_size, components;
    dglInt32_t *cur_node;
    dglNodeTraverser_s nt;
    int have_node_costs;
    dglInt32_t ncost;

    if (graph->Version < 2) {
	G_warning("Directed graph must be version 2 or 3 for NetA_strongly_connected_components()");
	return -1;
    }

    components = 0;
    nnodes = dglGet_NodeCount(graph);
    stack = (dglInt32_t *) G_calloc(nnodes + 1, sizeof(dglInt32_t));
    order = (dglInt32_t *) G_calloc(nnodes + 1, sizeof(dglInt32_t));
    processed = (int *)G_calloc(nnodes + 1, sizeof(int));
    if (!stack || !order || !processed) {
	G_fatal_error(_("Out of memory"));
	return -1;
    }

    for (i = 1; i <= nnodes; i++) {
	component[i] = 0;
    }

    ncost = 0;
    have_node_costs = dglGet_NodeAttrSize(graph);

    order_size = 0;
    dglNode_T_Initialize(&nt, graph);

    for (cur_node = dglNode_T_First(&nt); cur_node;
	 cur_node = dglNode_T_Next(&nt)) {
	dglInt32_t cur_node_id = dglNodeGet_Id(graph, cur_node);

	if (!component[cur_node_id]) {
	    component[cur_node_id] = --components;
	    stack[0] = cur_node_id;
	    stack_size = 1;
	    while (stack_size) {
		dglInt32_t *node, *edgeset, *edge;
		dglEdgesetTraverser_s et;
		dglInt32_t node_id = stack[stack_size - 1];

		if (processed[node_id]) {
		    stack_size--;
		    order[order_size++] = node_id;
		    continue;
		}
		processed[node_id] = 1;

		node = dglGetNode(graph, node_id);
		edgeset = dglNodeGet_OutEdgeset(graph, node);
		dglEdgeset_T_Initialize(&et, graph, edgeset);
		for (edge = dglEdgeset_T_First(&et); edge;
		     edge = dglEdgeset_T_Next(&et)) {
		    dglInt32_t to;

		    to = dglNodeGet_Id(graph, dglEdgeGet_Tail(graph, edge));
		    if (!component[to]) {
			component[to] = components;
			/* do not go through closed nodes */
			if (have_node_costs) {
			    memcpy(&ncost, dglNodeGet_Attr(graph, dglEdgeGet_Tail(graph, edge)),
				   sizeof(ncost));
			}
			if (ncost < 0)
			    processed[to] = 1;

			stack[stack_size++] = to;
		    }
		}
		dglEdgeset_T_Release(&et);
	    }
	}
    }

    dglNode_T_Release(&nt);

    components = 0;
    dglNode_T_Initialize(&nt, graph);

    while (order_size) {
	dglInt32_t cur_node_id = order[--order_size];
	int cur_comp = component[cur_node_id];

	if (cur_comp < 1) {
	    component[cur_node_id] = ++components;
	    stack[0] = cur_node_id;
	    stack_size = 1;
	    while (stack_size) {
		dglInt32_t *node, *edgeset, *edge;
		dglEdgesetTraverser_s et;
		dglInt32_t node_id = stack[--stack_size];

		node = dglGetNode(graph, node_id);
		edgeset = dglNodeGet_InEdgeset(graph, node);
		dglEdgeset_T_Initialize(&et, graph, edgeset);
		for (edge = dglEdgeset_T_First(&et); edge;
		     edge = dglEdgeset_T_Next(&et)) {
		    dglInt32_t to;

		    to = dglNodeGet_Id(graph, dglEdgeGet_Head(graph, edge));
		    if (component[to] == cur_comp) {
			component[to] = components;
			/* do not go through closed nodes */
			if (have_node_costs) {
			    memcpy(&ncost, dglNodeGet_Attr(graph, dglEdgeGet_Head(graph, edge)),
				   sizeof(ncost));
			}
			if (ncost >= 0)
			    stack[stack_size++] = to;
		    }
		}
		dglEdgeset_T_Release(&et);
	    }
	}
    }
    dglNode_T_Release(&nt);

    G_free(stack);
    G_free(order);
    G_free(processed);
    return components;
}

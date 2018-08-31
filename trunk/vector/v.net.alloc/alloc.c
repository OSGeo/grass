#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/dgl/graph.h>
#include <grass/glocale.h>
#include "alloc.h"

int alloc_from_centers_loop_tt(struct Map_info *Map, NODE *Nodes,
                               CENTER *Centers, int ncenters,
                               int tucfield)
{
    int center, line, nlines, i;
    int node1;
    int cat;
    struct line_cats *Cats;
    struct line_pnts *Points;
    double cost, n1cost, n2cost;
    int ret;

    Cats = Vect_new_cats_struct();
    Points = Vect_new_line_struct();
    
    nlines = Vect_get_num_lines(Map);

    for (i = 2; i <= (nlines * 2 + 2); i++) {
	Nodes[i].center = -1;/* NOTE: first two items of Nodes are not used */
	Nodes[i].cost = -1;
	Nodes[i].edge = 0;
    }

    for (center = 0; center < ncenters; center++) {
	G_percent(center, ncenters, 1);
	node1 = Centers[center].node;
	Vect_net_get_node_cost(Map, node1, &n1cost);
	G_debug(2, "center = %d node = %d cat = %d", center, node1,
		Centers[center].cat);

	for (line = 1; line <= nlines; line++) {
	    G_debug(5, "  node1 = %d line = %d", node1, line);
	    Vect_net_get_node_cost(Map, line, &n2cost);
	    /* closed, left it as not attached */

	    if (Vect_read_line(Map, Points, Cats, line) < 0)
		continue;
	    if (Vect_get_line_type(Map, line) != GV_LINE)
		continue;
	    if (!Vect_cat_get(Cats, tucfield, &cat))
		continue;

	    for (i = 0; i < 2; i++) {
		if (i == 1)
		    cat *= -1;

		ret =
		    Vect_net_ttb_shortest_path(Map, node1, 0, cat, 1,
					       tucfield, NULL,
					       &cost);
		if (ret == -1) {
		    continue;
		}		/* node unreachable */

		/* We must add center node costs (not calculated by Vect_net_shortest_path() ), but
		 *  only if center and node are not identical, because at the end node cost is add later */
		if (ret != 1)
		    cost += n1cost;

		G_debug(5,
			"Arc nodes: %d %d cost: %f (x old cent: %d old cost %f",
			node1, line, cost, Nodes[line * 2 + i].center,
			Nodes[line * 2 + i].cost);
		if (Nodes[line * 2 + i].center == -1 ||
		    Nodes[line * 2 + i].cost > cost) {
		    Nodes[line * 2 + i].cost = cost;
		    Nodes[line * 2 + i].center = center;
		}
	    }
	}
    }
    G_percent(1, 1, 1);

    Vect_destroy_cats_struct(Cats);
    Vect_destroy_line_struct(Points);

    return 0;
}

int alloc_to_centers_loop_tt(struct Map_info *Map, NODE *Nodes,
                               CENTER *Centers, int ncenters,
                               int tucfield)
{
    int center, line, nlines, i;
    int node1;
    int cat;
    struct line_cats *Cats;
    struct line_pnts *Points;
    double cost, n1cost, n2cost;
    int ret;

    Cats = Vect_new_cats_struct();
    Points = Vect_new_line_struct();
    
    nlines = Vect_get_num_lines(Map);

    for (i = 2; i <= (nlines * 2 + 2); i++) {
	Nodes[i].center = -1;/* NOTE: first two items of Nodes are not used */
	Nodes[i].cost = -1;
	Nodes[i].edge = 0;
    }

    for (line = 1; line <= nlines; line++) {
	G_debug(5, "  line = %d", line);
	Vect_net_get_node_cost(Map, line, &n2cost);
	/* closed, left it as not attached */

	if (Vect_read_line(Map, Points, Cats, line) < 0)
	    continue;
	if (Vect_get_line_type(Map, line) != GV_LINE)
	    continue;
	if (!Vect_cat_get(Cats, tucfield, &cat))
	    continue;

	for (center = 0; center < ncenters; center++) {
	    G_percent(center, ncenters, 1);
	    node1 = Centers[center].node;
	    Vect_net_get_node_cost(Map, node1, &n1cost);
	    G_debug(2, "center = %d node = %d cat = %d", center, node1,
		    Centers[center].cat);

	    for (i = 0; i < 2; i++) {
		if (i == 1)
		    cat *= -1;

		ret =
		    Vect_net_ttb_shortest_path(Map, cat, 1, node1, 0,
					       tucfield, NULL,
					       &cost);
		if (ret == -1) {
		    continue;
		}		/* node unreachable */

		/* We must add center node costs (not calculated by Vect_net_shortest_path() ), but
		 *  only if center and node are not identical, because at the end node cost is add later */
		if (ret != 1)
		    cost += n1cost;

		G_debug(5,
			"Arc nodes: %d %d cost: %f (x old cent: %d old cost %f",
			node1, line, cost, Nodes[line * 2 + i].center,
			Nodes[line * 2 + i].cost);
		if (Nodes[line * 2 + i].center == -1 ||
		    Nodes[line * 2 + i].cost > cost) {
		    Nodes[line * 2 + i].cost = cost;
		    Nodes[line * 2 + i].center = center;
		}
	    }
	}
    }
    G_percent(1, 1, 1);

    Vect_destroy_cats_struct(Cats);
    Vect_destroy_line_struct(Points);

    return 0;
}

int alloc_from_centers(dglGraph_s *graph, NODE *Nodes, CENTER *Centers, int ncenters)
{
    int i, nnodes;
    dglHeap_s heap;
    dglEdgesetTraverser_s et;
    int have_node_costs;
    dglInt32_t ncost;

    nnodes = dglGet_NodeCount(graph);

    /* initialize nodes */
    for (i = 1; i <= nnodes; i++) {
	Nodes[i].cost = -1;
	Nodes[i].center = -1;
	Nodes[i].edge = 0;
    }

    ncost = 0;
    have_node_costs = dglGet_NodeAttrSize(graph);

    dglHeapInit(&heap);

    for (i = 0; i < ncenters; i++) {
	int v = Centers[i].node;

	if (Nodes[v].cost == 0)
	    continue;		/* ignore duplicates */
	Nodes[v].cost = 0;		/* make sure all centers are processed first */
	Nodes[v].center = i;
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
	if (Nodes[v].cost < dist)
	    continue;

	node = dglGetNode(graph, v);

	if (have_node_costs && Nodes[v].edge) {
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

	    if (Nodes[to_id].cost < 0 || Nodes[to_id].cost > dist + d) {
		Nodes[to_id].cost = dist + d;
		Nodes[to_id].edge = dglEdgeGet_Id(graph, edge);
		Nodes[to_id].center = Nodes[v].center;
		heap_data.ul = to_id;
		dglHeapInsertMin(&heap, dist + d, ' ', heap_data);
	    }
	}

	dglEdgeset_T_Release(&et);
    }

    dglHeapFree(&heap, NULL);

    return 0;
}

int alloc_to_centers(dglGraph_s *graph, NODE *Nodes, CENTER *Centers, int ncenters)
{
    int i, nnodes;
    dglHeap_s heap;
    dglEdgesetTraverser_s et;
    int have_node_costs;
    dglInt32_t ncost;

    if (graph->Version < 2) {
	G_warning("Directed graph must be version 2 or 3 for distances to centers");
	return -1;
    }

    nnodes = dglGet_NodeCount(graph);

    /* initialize nodes */
    for (i = 1; i <= nnodes; i++) {
	Nodes[i].cost = -1;
	Nodes[i].center = -1;
	Nodes[i].edge = 0;
    }

    ncost = 0;
    have_node_costs = dglGet_NodeAttrSize(graph);

    dglHeapInit(&heap);

    for (i = 0; i < ncenters; i++) {
	int v = Centers[i].node;

	if (Nodes[v].cost == 0)
	    continue;		/* ignore duplicates */
	Nodes[v].cost = 0;		/* make sure all centers are processed first */
	Nodes[v].center = i;
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
	if (Nodes[v].cost < dist)
	    continue;

	node = dglGetNode(graph, v);

	if (have_node_costs && Nodes[v].edge) {
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

	    if (Nodes[from_id].cost < 0 || Nodes[from_id].cost > dist + d) {
		Nodes[from_id].cost = dist + d;
		Nodes[from_id].edge = dglEdgeGet_Id(graph, edge);
		Nodes[from_id].center = Nodes[v].center;
		heap_data.ul = from_id;
		dglHeapInsertMin(&heap, dist + d, ' ', heap_data);
	    }
	}

	dglEdgeset_T_Release(&et);
    }

    dglHeapFree(&heap, NULL);

    return 0;
}


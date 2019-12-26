/*!
   \file vector/neta/spanningtree.c

   \brief Network Analysis library - spanning tree

   Computes minimum spanning tree in the network.

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

struct union_find
{
    int *parent;
};

static int uf_initialize(struct union_find *uf, int size)
{
    int i;
    uf->parent = (int *)G_calloc(size, sizeof(int));
    if (!uf->parent)
	return 0;
    for (i = 0; i < size; i++)
	uf->parent[i] = i;
    return 1;
}

static void uf_release(struct union_find *uf)
{
    G_free(uf->parent);
}

static int uf_find(struct union_find *uf, int v)
{
    int cur = v, tmp;

    while (uf->parent[cur] != cur)
	cur = uf->parent[cur];
    while (uf->parent[v] != v) {
	tmp = uf->parent[v];
	uf->parent[v] = cur;
	v = tmp;
    }
    return cur;
}

/*TODO: union by rank */
static void uf_union(struct union_find *uf, int u, int v)
{
    int parent_u = uf_find(uf, u);
    int parent_v = uf_find(uf, v);

    if (parent_u != parent_v)
	uf->parent[parent_u] = parent_v;
}

typedef struct
{
    dglInt32_t cost;
    dglInt32_t *edge;
} edge_cost_pair;

static int cmp_edge(const void *pa, const void *pb)
{
    if (((edge_cost_pair *) pa)->cost < ((edge_cost_pair *) pb)->cost)
	return -1;

    return (((edge_cost_pair *) pa)->cost > ((edge_cost_pair *) pb)->cost);
}

/*!
   \brief Get number of edges in the spanning forest

   \param graph input graph
   \param[out] list of edges

   \return number of edges
   \return -1 on failure
 */
int NetA_spanning_tree(dglGraph_s * graph, struct ilist *tree_list)
{
    int nnodes, edges, nedges, i, index;
    edge_cost_pair *perm;	/*permutation of edges in ascending order */
    struct union_find uf;
    dglEdgesetTraverser_s et;

    /* TODO: consider closed nodes / node costs */

    nnodes = dglGet_NodeCount(graph);
    nedges = dglGet_EdgeCount(graph);
    perm = (edge_cost_pair *) G_calloc(nedges, sizeof(edge_cost_pair));
    if (!perm || !uf_initialize(&uf, nnodes + 1)) {
	G_fatal_error(_("Out of memory"));
	return -1;
    }
    /* dglGetEdge is only supported with graphs version > 1. Therefore this complicated enumeration of the edges... */
    index = 0;
    G_message(_("Computing minimum spanning tree..."));
    G_percent_reset();
    for (i = 1; i <= nnodes; i++) {
	G_percent(i, nnodes + nedges, 1);
	dglInt32_t *edge;

	dglEdgeset_T_Initialize(&et, graph,
				dglNodeGet_OutEdgeset(graph,
						      dglGetNode(graph,
								 (dglInt32_t)
								 i)));
	for (edge = dglEdgeset_T_First(&et); edge;
	     edge = dglEdgeset_T_Next(&et))
	    if (dglEdgeGet_Id(graph, edge) > 0) {
		perm[index].edge = edge;
		perm[index].cost = dglEdgeGet_Cost(graph, edge);
		index++;
	    }

	dglEdgeset_T_Release(&et);
    }
    edges = 0;
    qsort((void *)perm, index, sizeof(edge_cost_pair), cmp_edge);
    for (i = 0; i < index; i++) {
	G_percent(i + nnodes, nnodes + nedges, 1);
	dglInt32_t head =
	    dglNodeGet_Id(graph, dglEdgeGet_Head(graph, perm[i].edge));
	dglInt32_t tail =
	    dglNodeGet_Id(graph, dglEdgeGet_Tail(graph, perm[i].edge));
	if (uf_find(&uf, head) != uf_find(&uf, tail)) {
	    uf_union(&uf, head, tail);
	    edges++;
	    Vect_list_append(tree_list, dglEdgeGet_Id(graph, perm[i].edge));
	}
    }
    G_percent(index, index, 1);
    G_free(perm);
    uf_release(&uf);
    return edges;
}

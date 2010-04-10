/*!
   \file vector/neta/allpairs.c

   \brief Network Analysis library - shortest path between all pairs

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

/*!
   \brief Stores the directed distance between every pair

   \todo Use only O(W*W) memory where W is the number of nodes present in the graph

   Upon the completion, dist stores the directed distance between every pair (i,j)
   or -1 if the nodes are unreachable. It must be an array of dimension [nodes+1]*[nodes+1]

   \param graph input graph
   \param[out] dist list of directed distance

   \return -1 on error
   \return 0 on success
 */
int NetA_allpairs(dglGraph_s * graph, dglInt32_t ** dist)
{
    int nnodes, i, j, k, indices;
    dglEdgesetTraverser_s et;
    dglNodeTraverser_s nt;
    dglInt32_t *node;

    nnodes = dglGet_NodeCount(graph);
    dglInt32_t *node_indices;

    node_indices = (dglInt32_t *) G_calloc(nnodes, sizeof(dglInt32_t));
    if (!node_indices) {
	G_fatal_error(_("Out of memory"));
	return -1;
    }
    G_message(_("Computing all pairs shortest paths..."));
    G_percent_reset();
    for (i = 0; i <= nnodes; i++)
	for (j = 0; j <= nnodes; j++)
	    dist[i][j] = -1;
    dglNode_T_Initialize(&nt, graph);
    indices = 0;
    for (node = dglNode_T_First(&nt); node; node = dglNode_T_Next(&nt)) {
	dglInt32_t node_id = dglNodeGet_Id(graph, node);

	node_indices[indices++] = node_id;
	dglInt32_t *edge;

	dglEdgeset_T_Initialize(&et, graph,
				dglNodeGet_OutEdgeset(graph, node));
	for (edge = dglEdgeset_T_First(&et); edge;
	     edge = dglEdgeset_T_Next(&et))
	    if (dglEdgeGet_Id(graph, edge) < 0)	/*ignore backward edges */
		dist[node_id][dglNodeGet_Id
			      (graph, dglEdgeGet_Tail(graph, edge))] =
		    dglEdgeGet_Cost(graph, edge);
	dglEdgeset_T_Release(&et);
    }
    dglNode_T_Release(&nt);
    for (k = 0; k < indices; k++) {
	dglInt32_t k_index = node_indices[k];

	G_percent(k + 1, indices, 1);
	for (i = 0; i < indices; i++) {
	    dglInt32_t i_index = node_indices[i];

	    if (dist[i_index][k_index] == -1)
		continue;	/*no reason to proceed along infinite path */
	    for (j = 0; j < indices; j++) {
		dglInt32_t j_index = node_indices[j];

		if (dist[k_index][j_index] != -1 &&
		    (dist[i_index][k_index] + dist[k_index][j_index] <
		     dist[i_index][j_index] ||
		     dist[i_index][j_index] == -1)) {
		    dist[i_index][j_index] =
			dist[i_index][k_index] + dist[k_index][j_index];
		}
	    }
	}
    }

    G_free(node_indices);
    return 0;
}

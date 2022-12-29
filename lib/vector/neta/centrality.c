/*!
   \file lib/vector/neta/centrality.c

   \brief Network Analysis library - centrality

   Centrality measures

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
   \brief Computes degree centrality measure.

   Array degree has to be properly initialised to nnodes+1 elements

   \param graph input graph
   \param[out] degree array of degrees
 */
void NetA_degree_centrality(dglGraph_s * graph, double *degree)
{
    int i;
    double nnodes = dglGet_NodeCount(graph);

    for (i = 1; i <= nnodes; i++)
	degree[i] =
	    dglNodeGet_OutDegree(graph,
				 dglGetNode(graph, (dglInt32_t) i)) / nnodes;
}

/*!
   \brief Computes eigenvector centrality using edge costs as weights.

   \param graph input graph
   \param iterations number of iterations
   \param error ?
   \param[out] eigenvector eigen vector value

   \return 0 on success
   \return -1 on failure
 */
int NetA_eigenvector_centrality(dglGraph_s * graph, int iterations,
				double error, double *eigenvector)
{
    int i, iter, nnodes;
    double *tmp;

    nnodes = dglGet_NodeCount(graph);
    tmp = (double *)G_calloc(nnodes + 1, sizeof(double));
    if (!tmp) {
	G_fatal_error(_("Out of memory"));
	return -1;
    }

    error *= error;
    for (i = 1; i <= nnodes; i++)
	eigenvector[i] = 1;
    for (iter = 0; iter < iterations; iter++) {
	for (i = 1; i <= nnodes; i++)
	    tmp[i] = 0;
	dglInt32_t *node;
	dglNodeTraverser_s nt;
	dglEdgesetTraverser_s et;

	dglNode_T_Initialize(&nt, graph);
	for (node = dglNode_T_First(&nt); node; node = dglNode_T_Next(&nt)) {
	    dglInt32_t node_id = dglNodeGet_Id(graph, node);
	    double cur_value = eigenvector[node_id];
	    dglInt32_t *edge;

	    dglEdgeset_T_Initialize(&et, graph,
				    dglNodeGet_OutEdgeset(graph, node));
	    for (edge = dglEdgeset_T_First(&et); edge;
		 edge = dglEdgeset_T_Next(&et))
		tmp[dglNodeGet_Id(graph, dglEdgeGet_Tail(graph, edge))] +=
		    cur_value * dglEdgeGet_Cost(graph, edge);

	    dglEdgeset_T_Release(&et);
	}
	dglNode_T_Release(&nt);
	double cum_error = 0, max_value = tmp[1];

	for (i = 2; i <= nnodes; i++)
	    if (tmp[i] > max_value)
		max_value = tmp[i];
	for (i = 1; i <= nnodes; i++) {
	    tmp[i] /= max_value;
	    cum_error +=
		(tmp[i] - eigenvector[i]) * (tmp[i] - eigenvector[i]);
	    eigenvector[i] = tmp[i];
	}
	if (cum_error < error)
	    break;

    }

    G_free(tmp);
    return 0;
}

/*!
   \brief Computes betweenness and closeness centrality measure using Brandes algorithm. 

   Edge costs must be nonnegative. If some edge costs are negative then
   the behaviour of this method is undefined.

   \param graph input graph
   \param[out] betweenness betweeness values
   \param[out] closeness cloneness values

   \return 0 on success
   \return -1 on failure
 */
int NetA_betweenness_closeness(dglGraph_s * graph, double *betweenness,
			       double *closeness)
{
    int i, j, nnodes, stack_size, count;
    dglInt32_t *dst, *node, *stack, *cnt, *delta;
    dglNodeTraverser_s nt;
    dglEdgesetTraverser_s et;
    dglHeap_s heap;
    struct ilist **prev;

    nnodes = dglGet_NodeCount(graph);

    dst = (dglInt32_t *) G_calloc(nnodes + 1, sizeof(dglInt32_t));
    prev = (struct ilist **)G_calloc(nnodes + 1, sizeof(struct ilist *));
    stack = (dglInt32_t *) G_calloc(nnodes, sizeof(dglInt32_t));
    cnt = (dglInt32_t *) G_calloc(nnodes + 1, sizeof(dglInt32_t));
    delta = (dglInt32_t *) G_calloc(nnodes + 1, sizeof(dglInt32_t));

    if (!dst || !prev || !stack || !cnt || !delta) {
	G_fatal_error(_("Out of memory"));
	return -1;
    }


    for (i = 1; i <= nnodes; i++) {
	prev[i] = Vect_new_list();
	if (closeness)
	    closeness[i] = 0;
	if (betweenness)
	    betweenness[i] = 0;
    }

    count = 0;
    G_percent_reset();
    dglNode_T_Initialize(&nt, graph);
    for (node = dglNode_T_First(&nt); node; node = dglNode_T_Next(&nt)) {
	G_percent(count++, nnodes, 1);
	dglInt32_t s = dglNodeGet_Id(graph, node);
	dglHeapData_u heap_data;
	dglHeapNode_s heap_node;

	stack_size = 0;
	for (i = 1; i <= nnodes; i++)
	    Vect_reset_list(prev[i]);
	for (i = 1; i <= nnodes; i++) {
	    cnt[i] = 0;
	    dst[i] = -1;
	}
	dst[s] = 0;
	cnt[s] = 1;
	dglHeapInit(&heap);
	heap_data.ul = s;
	dglHeapInsertMin(&heap, 0, ' ', heap_data);
	while (1) {
	    dglInt32_t v, dist;

	    if (!dglHeapExtractMin(&heap, &heap_node))
		break;
	    v = heap_node.value.ul;
	    dist = heap_node.key;
	    if (dst[v] < dist)
		continue;
	    stack[stack_size++] = v;

	    dglInt32_t *edge;

	    dglEdgeset_T_Initialize(&et, graph,
				    dglNodeGet_OutEdgeset(graph,
							  dglGetNode(graph,
								     v)));
	    for (edge = dglEdgeset_T_First(&et); edge;
		 edge = dglEdgeset_T_Next(&et)) {
		dglInt32_t *to = dglEdgeGet_Tail(graph, edge);
		dglInt32_t to_id = dglNodeGet_Id(graph, to);
		dglInt32_t d = dglEdgeGet_Cost(graph, edge);

		if (dst[to_id] == -1 || dst[to_id] > dist + d) {
		    dst[to_id] = dist + d;
		    Vect_reset_list(prev[to_id]);
		    heap_data.ul = to_id;
		    dglHeapInsertMin(&heap, dist + d, ' ', heap_data);
		}
		if (dst[to_id] == dist + d) {
		    cnt[to_id] += cnt[v];
		    Vect_list_append(prev[to_id], v);
		}
	    }

	    dglEdgeset_T_Release(&et);
	}
	dglHeapFree(&heap, NULL);
	for (i = 1; i <= nnodes; i++)
	    delta[i] = 0;
	for (i = stack_size - 1; i >= 0; i--) {
	    dglInt32_t w = stack[i];

	    if (closeness)
		closeness[s] += dst[w];

	    for (j = 0; j < prev[w]->n_values; j++) {
		dglInt32_t v = prev[w]->value[j];

		delta[v] += (cnt[v] / (double)cnt[w]) * (1.0 + delta[w]);
	    }
	    if (w != s && betweenness)
		betweenness[w] += delta[w];

	}
	if (closeness)
	    closeness[s] /= (double)stack_size;
    }
    dglNode_T_Release(&nt);

    for (i = 1; i <= nnodes; i++)
	Vect_destroy_list(prev[i]);
    G_free(delta);
    G_free(cnt);
    G_free(stack);
    G_free(prev);
    G_free(dst);

    return 0;
};

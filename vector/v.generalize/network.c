
/****************************************************************
 *
 * MODULE:     v.generalize
 *
 * AUTHOR(S):  Daniel Bundala
 *
 * PURPOSE:    Network generalization
 *
 * COPYRIGHT:  (C) 2002-2005 by the GRASS Development Team
 *
 *             This program is free software under the
 *             GNU General Public License (>=v2).
 *             Read the file COPYING that comes with GRASS
 *             for details.
 *
 ****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>

typedef struct
{
    int **edge;			/* edge for each vertex */
    int *degree;		/* degree of vertices */
    int vertices;
} NdglGraph_s;

void graph_free(NdglGraph_s * g)
{
    int i;

    return;
    G_free(g->degree);
    if (g->edge) {
	for (i = 0; i < g->vertices; g++)
	    G_free(g->edge[i]);
    }
    G_free(g->edge);
    return;
}

int graph_init(NdglGraph_s * g, int vertices)
{
    g->edge = NULL;
    g->degree = NULL;

    g->vertices = vertices;
    g->degree = (int *)G_calloc(vertices, sizeof(int));
    if (!g->degree)
	return 0;
    g->edge = (int **)G_calloc(vertices, sizeof(int *));
    if (!g->edge) {
	graph_free(g);
	return 0;
    }

    return 1;
}

/* writes the most important part of the In network to Out network
 * according to the thresholds, output is bigger for smaller
 * thresholds. Function returns the number of points written 
 TODO: rewrite ilist by something more space and time efficient
 or at least, implement append which does not check whether
 the value is already in the list*/
int graph_generalization(struct Map_info *In, struct Map_info *Out,
			 int mask_type, double degree_thresh, 
			 double closeness_thresh, double betweeness_thresh)
{

    int i;
    int output = 0;
    dglGraph_s *gr;
    NdglGraph_s g;
    int nnodes;
    struct line_pnts *Points;
    struct line_cats *Cats;
    int type;
    int *closeness, *queue, *internal, *paths, *comp, *dist;
    double *betw, *betweeness;
    struct ilist **prev;

    if (0 != Vect_net_build_graph(In, mask_type, 0, 0, NULL, NULL, NULL, 0, 0))
        G_fatal_error(_("Unable to build graph for vector map <%s>"), Vect_get_full_name(In));
    
    gr = Vect_net_get_graph(In);
    
    /* build own graph by edge<->vertex */
    /* each vertex represents undirected edge */
    if (!graph_init(&g, dglGet_EdgeCount(gr) / 2 + 1)) {
	G_fatal_error(_("Out of memory"));
	return 0;
    }

    nnodes = dglGet_NodeCount(gr);

    for (i = 0; i < nnodes; i++) {
	dglInt32_t *node, *edgeset, *edge;
	dglEdgesetTraverser_s et;

	node = dglGetNode(gr, (dglInt32_t) i);
	edgeset = dglNodeGet_OutEdgeset(gr, node);
	dglEdgeset_T_Initialize(&et, gr, edgeset);
	for (edge = dglEdgeset_T_First(&et); edge;
	     edge = dglEdgeset_T_Next(&et)) {
	    int id, from_degree, to_degree;
	    dglInt32_t *to, *from, *to_edgeset, *to_edge;
	    dglEdgesetTraverser_s to_et;

	    from = dglEdgeGet_Head(gr, edge);
	    to = dglEdgeGet_Tail(gr, edge);
	    to_edgeset = dglNodeGet_OutEdgeset(gr, to);
	    dglEdgeset_T_Initialize(&to_et, gr, to_edgeset);


	    to_degree = dglNodeGet_OutDegree(gr, to);
	    from_degree = dglNodeGet_OutDegree(gr, from);
	    id = abs(dglEdgeGet_Id(gr, edge));

	    /* allocate memory, if it has not been not allocated already */
	    if (!g.edge[id]) {
		g.edge[id] =
		    G_malloc(sizeof(int) * (to_degree + from_degree));
		if (!g.edge[id]) {
		    graph_free(&g);
		    G_fatal_error(_("Out of memory"));
		    return 0;
		}
	    }

	    for (to_edge = dglEdgeset_T_First(&to_et); to_edge;
		 to_edge = dglEdgeset_T_Next(&to_et)) {
		int id2 = abs(dglEdgeGet_Id(gr, to_edge));

		g.edge[id][g.degree[id]++] = id2;
	    }


	    dglEdgeset_T_Release(&to_et);
	}
	dglEdgeset_T_Release(&et);

    }

    closeness = (int *)G_calloc(g.vertices, sizeof(int));
    queue = (int *)G_calloc(g.vertices, sizeof(int));
    dist = (int *)G_calloc(g.vertices, sizeof(int));
    internal = (int *)G_calloc(g.vertices, sizeof(int));
    betweeness = (double *)G_calloc(g.vertices, sizeof(double));
    paths = (int *)G_calloc(g.vertices, sizeof(int));
    comp = (int *)G_calloc(g.vertices, sizeof(int));
    betw = (double *)G_calloc(g.vertices, sizeof(double));
    prev = (struct ilist **)G_calloc(g.vertices, sizeof(struct ilist *));
    for (i = 0; i < g.vertices; i++)
	prev[i] = Vect_new_list();


    /* run BFS from each vertex and find the sum
     * of the shortest paths from each vertex */
    G_percent_reset();
    G_message(_("Calculating centrality measures..."));
    for (i = 1; i < g.vertices; i++) {
	int front, back, j;

	G_percent(i, g.vertices - 1, 1);
	front = 0;
	back = 1;
	queue[front] = i;
	/* Is this portable? */
	memset(dist, 127, sizeof(int) * g.vertices);
	dist[i] = 0;
	closeness[i] = 0;
	comp[i] = 0;

	memset(paths, 0, sizeof(int) * g.vertices);
	paths[i] = 1;
	memset(internal, 0, sizeof(int) * g.vertices);
	for (j = 0; j < g.vertices; j++)
	    Vect_reset_list(prev[j]);

	while (front != back) {
	    int v, j;

	    v = queue[front];
	    comp[i]++;
	    front = (front + 1) % g.vertices;
	    for (j = 0; j < g.degree[v]; j++) {
		int to = g.edge[v][j];

		if (dist[to] > dist[v] + 1) {
		    paths[to] = paths[v];
		    internal[v] = 1;
		    dist[to] = dist[v] + 1;
		    closeness[i] += dist[to];
		    queue[back] = to;
		    Vect_reset_list(prev[to]);
		    Vect_list_append(prev[to], v);
		    back = (back + 1) % g.vertices;
		}
		else if (dist[to] == dist[v] + 1) {
		    internal[v] = 1;
		    paths[to] += paths[v];
		    Vect_list_append(prev[to], v);
		}
	    }
	}
	/* finally run another BFS from the leaves in the BFS DAG
	 * and calculate betweeness centrality measure */
	front = 0;
	back = 0;
	for (j = 1; j < g.vertices; j++)
	    if (!internal[j] && dist[j] <= g.vertices) {
		queue[back] = j;
		back = (back + 1) % g.vertices;
	    }
	memset(betw, 0, sizeof(double) * g.vertices);
	while (front != back) {
	    int v, j;

	    v = queue[front];
	    front = (front + 1) % g.vertices;
	    betweeness[v] += betw[v];
	    for (j = 0; j < prev[v]->n_values; j++) {
		int to = prev[v]->value[j];

		if (betw[to] == 0) {
		    queue[back] = to;
		    back = (back + 1) % g.vertices;
		}
		betw[to] +=
		    (betw[v] +
		     (double)1.0) * ((double)paths[to] / (double)paths[v]);
	    }
	}
    }


    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();
    for (i = 1; i < g.vertices; i++) {
	if ((g.degree[i] >= degree_thresh &&
	     (comp[i] - 1.0) / closeness[i] >= closeness_thresh &&
	     betweeness[i] >= betweeness_thresh)) {
	    type = Vect_read_line(In, Points, Cats, i);
	    if (type & mask_type) {
		output += Points->n_points;
		Vect_write_line(Out, type, Points, Cats);
	    }
	}
    }

    G_free(dist);
    G_free(closeness);
    G_free(paths);
    G_free(betweeness);
    G_free(internal);
    G_free(queue);
    G_free(comp);
    G_free(betw);
    for (i = 0; i < g.vertices; i++)
	Vect_destroy_list(prev[i]);
    G_free(prev);
    graph_free(&g);
    return output;
}

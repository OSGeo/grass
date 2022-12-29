#ifndef GRASS_DdglGraph_s_H
#define GRASS_DdglGraph_s_H

/* pg comes from "planar graph" */
/* every edge is directed. Nevertheless, we can visit it on both sides */
struct pg_edge {
    int v1; /* first vertex */
    int v2; /* second vertex */
    char visited_left;
    char visited_right;
    char winding_left; /* winding numbers */
    char winding_right;
};

struct pg_vertex {
    double x; /* coordinates */
    double y;
    int ecount; /* number of neighbours */
    int eallocated; /* size of the array below */
    struct pg_edge **edges; /* array of pointers */
    double *angles; /* precalculated angles with Ox */
};

struct planar_graph {
    int vcount; /* number of vertices */
    struct pg_vertex *v;
    int ecount;
    int eallocated;
    struct pg_edge *e;
};

struct planar_graph* pg_create_struct(int n, int e);
void pg_destroy_struct(struct planar_graph *pg);
int pg_existsedge(struct planar_graph *pg, int v1, int v2);
void pg_addedge(struct planar_graph *pg, int v1, int v2);
struct planar_graph* pg_create(const struct line_pnts *Points);

#endif

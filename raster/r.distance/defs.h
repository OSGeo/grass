
/****************************************************************************
 *
 * MODULE:       r.distance
 *
 * AUTHOR(S):    Michael Shapiro - CERL
 *
 * PURPOSE:      Locates the closest points between objects in two
 *               raster maps.
 *
 * COPYRIGHT:    (C) 2003 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 ***************************************************************************/

#ifndef __R_DIST_DEFS_H__
#define __R_DIST_DEFS_H__

#include <grass/gis.h>
#include <grass/glocale.h>

struct EdgeList			/* keep track of edge cells */
{
    struct CatEdgeList
    {
	CELL cat;		/* category number */
	int *row, *col;		/* arrays of pixels indexes */
	int ncells;		/* count of edges cells with this cat */
	int nalloc;		/* lenght of allocation for row,col */
    } *catlist;			/* array of cat:edgelists */
    int ncats;			/* number of cats */
    int nalloc;			/* length of allocation for catlist */
    int count;			/* total number of edge cells */
};

struct Map
{
    char *name;			/* raster map name */
    char *mapset;		/* raster map mapset */
    char *fullname;		/* raster map fully qualified name */
    struct Categories labels;	/* category labels */
    struct EdgeList edges;	/* edge cells */
};

struct Parms
{
    struct Map map1, map2;	/* two raster maps to analyze */
    int labels;			/* boolean: report includes cat labels */
    char *fs;			/* report field separator     */
    int overlap;		/* checking for overlapping, than distance is 0 */
};

/* distance.c */
void find_minimum_distance(struct CatEdgeList *, struct CatEdgeList *,
			   double *, double *, double *, double *, double *,
			   struct Cell_head *, int, char *, char *);
int null_distance(char *name1, char *name2, int *, int *);

/* edges.c */
void print_edge_info(struct Map *);
void find_edge_cells(struct Map *);
void add_edge_cell(struct Map *, CELL, int, int);
void init_edge_list(struct Map *);
void sort_edge_list(struct Map *);

/* labels.c */
void read_labels(struct Map *);
char *get_label(struct Map *, CELL);

/* parse.c */
void parse(int, char *[], struct Parms *);

/* report.c */
void report(struct Parms *);

#endif /* __R_DIST_DEFS_H__ */

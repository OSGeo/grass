
/****************************************************************************
 *
 * MODULE:       r.distance
 *
 * AUTHOR(S):    Michael Shapiro - CERL
 *               Sort/reverse sort by distance by Huidae Cho
 *
 * PURPOSE:      Locates the closest points between objects in two 
 *               raster maps.
 *
 * COPYRIGHT:    (C) 2003-2014 by the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2). Read the file COPYING that
 *               comes with GRASS for details.
 *
 ***************************************************************************/

#include <stdlib.h>

#include <grass/raster.h>
#include <grass/glocale.h>

#include "defs.h"

void print_edge_info(struct Map *map)
{
    int i;

    fprintf(stdout, "%s: %d edge cells\n", map->fullname, map->edges.count);
    for (i = 0; i < map->edges.ncats; i++)
	fprintf(stdout, " %d", map->edges.catlist[i].cat);
    fprintf(stdout, "\n");
}

void find_edge_cells(struct Map *map, int null)
{
    void init_edge_list();
    void add_edge_cell();
    void sort_edge_list();

    int nrows, ncols, row, col;
    int fd;
    CELL *buf0, *buf1, *buf2, *tmp;

    G_message(_("Reading map %s ..."), map->fullname);

    ncols = Rast_window_cols();
    nrows = Rast_window_rows();

    buf0 = (CELL *) G_calloc(ncols + 2, sizeof(CELL));
    buf1 = (CELL *) G_calloc(ncols + 2, sizeof(CELL));
    buf2 = (CELL *) G_calloc(ncols + 2, sizeof(CELL));

    for (col = 0; col < (ncols + 2); col++) {
	buf0[col] = 0;
	buf1[col] = 0;
	buf2[col] = 0;
    }

    fd = Rast_open_old(map->name, map->mapset);

    init_edge_list(map);

    for (row = 0; row < nrows; row++) {
	G_percent(row, nrows, 2);
	/* rotate the input buffers */
	tmp = buf0;
	buf0 = buf1;
	buf1 = buf2;
	buf2 = tmp;

	/* read a row */
	Rast_get_c_row(fd, &buf1[1], row);

	for (col = 1; col <= ncols; col++) {
	    if ((buf1[col]	/* is a valid category */
		&&(buf1[col - 1] != buf1[col]	/* 4 neighbors not the same? */
		   ||buf1[col + 1] != buf1[col]
		   || buf0[col] != buf1[col]
		   || buf2[col] != buf1[col])) &&
		(null || !Rast_is_c_null_value(&buf1[col])))
		add_edge_cell(map, buf1[col], row, col - 1);
	}
    }
    G_percent(row, nrows, 2);

    Rast_close(fd);

    G_free(buf0);
    G_free(buf1);
    G_free(buf2);

    sort_edge_list(map);
}

void add_edge_cell(struct Map *map, CELL cat, int row, int col)
{
    int i, k;

    /* search for this cat (note: sequential search for now) */
    for (i = 0; i < map->edges.ncats; i++)
	if (cat == map->edges.catlist[i].cat)
	    break;
    if (i == map->edges.ncats) {	/* new category */
	map->edges.ncats += 1;
	if (map->edges.nalloc < map->edges.ncats) {
	    map->edges.nalloc += 32;
	    map->edges.catlist =
		(struct CatEdgeList *)G_realloc(map->edges.catlist,
						map->edges.nalloc *
						sizeof(struct CatEdgeList));
	}
	map->edges.catlist[i].ncells = 0;
	map->edges.catlist[i].nalloc = 0;
	map->edges.catlist[i].row = NULL;
	map->edges.catlist[i].col = NULL;
	map->edges.catlist[i].cat = cat;
    }

    /* new edge cell insert */
    k = map->edges.catlist[i].ncells;
    map->edges.catlist[i].ncells += 1;
    if (map->edges.catlist[i].nalloc < map->edges.catlist[i].ncells) {
	map->edges.catlist[i].nalloc += 256;
	map->edges.catlist[i].row =
	    (int *)G_realloc(map->edges.catlist[i].row,
			     map->edges.catlist[i].nalloc * sizeof(int));
	map->edges.catlist[i].col =
	    (int *)G_realloc(map->edges.catlist[i].col,
			     map->edges.catlist[i].nalloc * sizeof(int));
    }
    map->edges.catlist[i].row[k] = row;
    map->edges.catlist[i].col[k] = col;

    /* for good measure, count the total edge cells */
    map->edges.count++;
}

void init_edge_list(struct Map *map)
{
    map->edges.count = 0;
    map->edges.ncats = 0;
    map->edges.nalloc = 0;
    map->edges.catlist = NULL;
}

static int cmp(const void *aa, const void *bb)
{
    const struct CatEdgeList *a = aa, *b = bb;

    if (a->cat < b->cat)
	return -1;

    return (a->cat > b->cat);
}

void sort_edge_list(struct Map *map)
{
    if (map->edges.ncats > 0)
	qsort(map->edges.catlist, map->edges.ncats,
	      sizeof(struct CatEdgeList), cmp);
}

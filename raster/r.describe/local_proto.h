
/****************************************************************************
 *
 * MODULE:       r.describe
 *
 * AUTHOR(S):    Michael Shapiro - CERL
 *
 * PURPOSE:      Prints terse list of category values found in a raster
 *               map layer.
 *
 * COPYRIGHT:    (C) 2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 ***************************************************************************/

#ifndef __R_DESC_LOCAL_PROTO_H__
#define __R_DESC_LOCAL_PROTO_H__

#include <grass/raster.h>

/* describe.c */
int describe(const char *, int, char *, int, int, int, int, int);

/* dumplist.c */
int long_list(struct Cell_stats *, DCELL, DCELL, char *, int, RASTER_MAP_TYPE,
	      int);
int compact_list(struct Cell_stats *, DCELL, DCELL, char *, int,
		 RASTER_MAP_TYPE, int);
int compact_range_list(CELL, CELL, CELL, CELL, CELL, CELL, char *, int);
int range_list(CELL, CELL, CELL, CELL, CELL, CELL, char *, int);

/* main.c */
int main(int, char *[]);

/* tree.c */
int plant_tree(void);
int add_node_to_tree(register CELL);
int first_node(void);
int next_node(void);
int first_cat(CELL *);
int next_cat(CELL *);

#endif /* __R_DESC_LOCAL_PROTO_H__ */

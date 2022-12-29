
/****************************************************************
 * MODULE:     v.path.obstacles
 *
 * AUTHOR(S):  Maximilian Maldacker
 *  
 *
 * COPYRIGHT:  (C) 2002-2005 by the GRASS Development Team
 *
 *             This program is free software under the
 *             GNU General Public License (>=v2).
 *             Read the file COPYING that comes with GRASS
 *             for details.
 *
 ****************************************************************/
#ifndef VISIBILITY_H
#define VISIBILITY_H


#include <stdlib.h>
#include <assert.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/vector.h>
#include "data_structures.h"
#include "geometry.h"
#include "rotation_tree.h"


void construct_visibility(struct Point *points, int num_points,
			  struct Line *lines, int num_lines,
			  struct Map_info *out);

void handle(struct Point *p, struct Point *q, struct Map_info *out);
void report(struct Point *p, struct Point *q, struct Map_info *out);

void init_vis(struct Point *points, int num_points, struct Line *lines,
	      int num_lines);

void visibility_points(struct Point *points, int num_points,
		       struct Line *lines, int num_lines,
		       struct Map_info *out, int n);


#endif


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

#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <stdlib.h>
#include <assert.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/vector.h>
#include "rotation_tree.h"

int point_inside(struct Point *p, double x, double y);
int in_between(struct Point *p, struct Line *e);
int left_turn(struct Point *p1, struct Point *p2, struct Point *p3);
int before(struct Point *p, struct Point *q, struct Line *e);

int segment_intersect(struct Line *line, struct Point *p, double *y);


#endif

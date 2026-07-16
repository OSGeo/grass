/****************************************************************
 * MODULE:     v.path.obstacles
 *
 * AUTHOR(S):  Maximilian Maldacker
 *
 *
 * SPDX-FileCopyrightText: 2002-2005 Other GRASS authors
 * SPDX-License-Identifier: GPL-2.0-or-later
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

void visibility_points(struct Point *points, int num_points, struct Line *lines,
                       int num_lines, struct Map_info *out, int n);

#endif


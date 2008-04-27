
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
#ifndef DATA_STRUCTURES_H
#define DATA_STRUCTURES_H

#include <grass/gis.h>
#include <grass/glocale.h>
#include "rotation_tree.h"

struct Point *pop();
struct Point *top();
void push(struct Point *p);
int empty_stack();
void init_stack();

int cmp_points(const void *v1, const void *v2, void *param);

void quickSort(struct Point a[], int l, int r);
int partition(struct Point a[], int l, int r);


#endif

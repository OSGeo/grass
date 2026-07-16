/****************************************************************
 * MODULE:     v.path.obstacles
 *
 * AUTHOR(S):  Maximilian Maldacker
 *
 *
 * SPDX-FileCopyrightText: 2002-2005 Other GRASS authors
 * SPDX-License-Identifier: GPL-2.0-or-later
 ****************************************************************/
#ifndef DATA_STRUCTURES_H
#define DATA_STRUCTURES_H

#include <grass/gis.h>
#include <grass/glocale.h>
#include "rotation_tree.h"

struct Point *pop(void);
struct Point *top(void);
void push(struct Point *p);
int empty_stack(void);
void init_stack(int);

int cmp_points(const void *v1, const void *v2, void *param);

void quickSort(struct Point a[], int l, int r);
int partition(struct Point a[], int l, int r);

#endif


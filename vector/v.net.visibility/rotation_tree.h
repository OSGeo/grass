
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

#ifndef ROTATION_TREE_H
#define ROTATION_TREE_H

#include <stdlib.h>
#include <assert.h>

struct Point
{
    double x;
    double y;

    struct Line *line1;
    struct Line *line2;

    struct Line *vis;

    struct Point *left_brother;
    struct Point *right_brother;
    struct Point *father;
    struct Point *rightmost_son;

    int cat;
};

struct Line
{
    struct Point *p1;
    struct Point *p2;
};

void add_rightmost(struct Point *p, struct Point *q);
void add_leftof(struct Point *p, struct Point *q);

void remove_point(struct Point *p);

struct Point *right_brother(struct Point *p);
struct Point *left_brother(struct Point *p);
struct Point *father(struct Point *p);
struct Point *rightmost_son(struct Point *p);

struct Line *segment1(struct Point *p);
struct Line *segment2(struct Point *p);
struct Point *other1(struct Point *p);
struct Point *other2(struct Point *p);





#endif

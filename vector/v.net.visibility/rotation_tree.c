
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

#include "rotation_tree.h"


void add_rightmost(struct Point *p, struct Point *q)
{
    struct Point *right;

    p->left_brother = NULL;
    p->right_brother = NULL;

    if (q->rightmost_son == NULL) {
	q->rightmost_son = p;
    }
    else {
	right = q->rightmost_son;

	right->right_brother = p;
	p->left_brother = right;

	q->rightmost_son = p;
    }

    p->father = q;


}

void add_leftof(struct Point *p, struct Point *q)
{
    struct Point *left;

    if (q->left_brother == NULL) {
	p->left_brother = NULL;
	q->left_brother = p;
	p->right_brother = q;
    }
    else {
	left = q->left_brother;

	p->left_brother = left;
	left->right_brother = p;

	p->right_brother = q;
	q->left_brother = p;
    }


    p->father = q->father;
}

void remove_point(struct Point *p)
{
    struct Point *f = p->father;
    struct Point *l = p->left_brother;
    struct Point *r = p->right_brother;

    if (l != NULL)
	l->right_brother = r;
    if (r != NULL)
	r->left_brother = l;

    if (f->rightmost_son == p)
	f->rightmost_son = NULL;

    p->father = NULL;
    p->left_brother = NULL;
    p->right_brother = NULL;

}

struct Point *right_brother(struct Point *p)
{
    return p->right_brother;
}

struct Point *left_brother(struct Point *p)
{
    return p->left_brother;
}

struct Point *father(struct Point *p)
{
    return p->father;
}

struct Point *rightmost_son(struct Point *p)
{
    return p->rightmost_son;
}

struct Line *segment1(struct Point *p)
{
    return p->line1;
}


struct Line *segment2(struct Point *p)
{
    return p->line2;
}

struct Point *other1(struct Point *p)
{
    if (p->line1 == NULL)
	return NULL;

    if (p->line1->p1 == p)
	return p->line1->p2;
    else
	return p->line1->p1;
}

struct Point *other2(struct Point *p)
{
    if (p->line2 == NULL)
	return NULL;

    if (p->line2->p1 == p)
	return p->line2->p2;
    else
	return p->line2->p1;
}

/*
 ****************************************************************************
 *
 * MODULE:       v.vol.rst: program for 3D (volume) interpolation and geometry
 *               analysis from scattered point data using regularized spline
 *               with tension
 *
 * AUTHOR(S):    Original program (1989) and various modifications:
 *               Lubos Mitas
 *
 *               GRASS 4.2, GRASS 5.0 version and modifications:
 *               H. Mitasova,  I. Kosinovsky, D. Gerdes, J. Hofierka
 *
 * PURPOSE:      v.vol.rst interpolates the values to 3-dimensional grid from
 *               point data (climatic stations, drill holes etc.) given in a
 *               3D vector point input. Output grid3 file is elev. 
 *               Regularized spline with tension is used for the
 *               interpolation.
 *
 * COPYRIGHT:    (C) 1989, 1993, 2000 L. Mitas,  H. Mitasova,
 *               I. Kosinovsky, D. Gerdes, J. Hofierka
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <grass/gis.h>

#include "oct.h"
#include "externs.h"
#include "dataoct.h"




struct octfunc *OT_functions_new(int (*compare) (),
				 VOID_T ** (*divide_data) (),
				 int (*add_data) (), int (*intersect) (),
				 int (*division_check) (),
				 int (*get_points) ())







/* Initializes FUNCTIONS structure with given arguments */
{
    struct octfunc *functions;
    if (!(functions = (struct octfunc *)G_malloc(sizeof(struct octfunc)))) {
	return NULL;
    }
    functions->compare = compare;
    functions->divide_data = divide_data;
    functions->add_data = add_data;
    functions->intersect = intersect;
    functions->division_check = division_check;
    functions->get_points = get_points;
    return functions;
}




struct octtree *OT_tree_new(VOID_T * data, struct octtree **leafs,
			    struct octtree *parent, struct octfunc *functions,
			    int octant)






/*Initializes TREE using given arguments */
{
    struct octtree *tree;
    if (!(tree = (struct octtree *)G_malloc(sizeof(struct octtree)))) {
	return NULL;
    }
    tree->data = data;
    tree->leafs = leafs;
    tree->parent = parent;
    tree->functions = functions;
    tree->octant = octant;
    return tree;
}






int OT_insert_oct(struct quadruple *point, struct octtree *tree)
{
    int j = 0, i, k, comp;

    if (tree == NULL) {
	fprintf(stderr, "insert: tree is NULL\n");
	return -5;
    }
    if (tree->data == NULL) {
	fprintf(stderr, "insert: tree->data is NULL\n");
	return -5;
    }
    i = tree->functions->division_check(tree->data);
    if (i <= 0) {
	if (i == -1) {
	    comp = tree->functions->compare(point, tree->data);
	    if ((comp < 1) || (comp > NUMLEAFS))
		return -3;
	    j = OT_insert_oct(point, tree->leafs[comp - 1]);
	}
	else {
	    if (i == 0) {
		j = tree->functions->add_data(point, tree->data);
	    }
	}
    }
    else {
	k = OT_divide_oct(tree);
	if (k == 1)
	    j = OT_insert_oct(point, tree);
	/* DPG hack */
	if (k == -3) {
	    static int once = 0;

	    if (!once) {
		fprintf(stderr, "Point out of range!\n");
		once = 1;
	    }
	}
	if (k < 0)
	    return k;

    }
    return j;
}







int OT_divide_oct(struct octtree *tree)
{
    int i;
    VOID_T **datas;
    struct octtree *par;
    struct octtree **leafs;

    datas = tree->functions->divide_data(tree->data);
    if (datas == NULL) {
	fprintf(stderr, "datas is NULL\n");
	return -7;
    }
    par = tree;
    leafs = (struct octtree **)G_malloc(sizeof(struct octtree *) * NUMLEAFS);
    for (i = 1; i <= NUMLEAFS; i++) {
	leafs[i - 1] = OT_tree_new(datas[i], NULL, par, tree->functions, i);
    }
    tree->leafs = leafs;
    return 1;
}






int
OT_region_data(struct octtree *tree, double xmin, double xmax, double ymin,
	       double ymax, double zmin, double zmax,
	       struct quadruple *points, int MAX)









	/* max number of points we can add (KMAX2) */
 /* note: this KMAX2 can be larger then KMAX */
{
    int n = 0, j;

    if (tree == NULL) {
	fprintf(stderr, "OT_region_data: tree is NULL\n");
	return n;
    }
    if (tree->data == NULL) {
	fprintf(stderr, "OT_region_data: tree is NULL\n");
	return n;
    }
    if (tree->functions->
	intersect(xmin, xmax, ymin, ymax, zmin, zmax, tree->data)) {
	if (tree->leafs != NULL) {
	    for (j = 0; j < NUMLEAFS; j++) {
		if ((n = n + OT_region_data(tree->leafs[j], xmin, xmax,
					    ymin, ymax, zmin, zmax,
					    points + n, MAX - n)) > MAX)
		    return n;
	    }
	}
	else {
	    n = tree->functions->get_points(points, tree->data,
					    xmin, xmax, ymin, ymax, zmin,
					    zmax, MAX);
	}
	return n;
    }
    return 0;
}

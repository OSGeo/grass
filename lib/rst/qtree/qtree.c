/*!
 * \file qtree.c
 *
 * \author
 * H. Mitasova, I. Kosinovsky, D. Gerdes, Fall 1993,
 * University of Illinois and
 * US Army Construction Engineering Research Lab
 *
 * \author H. Mitasova (University of Illinois),
 * \author I. Kosinovsky, (USA-CERL)
 * \author D.Gerdes (USA-CERL)
 *
 * \author updated/checked by Mitasova Nov. 96 (no changes necessary)
 *
 * \copyright
 * (C) 1993-1996 by Helena Mitasova and the GRASS Development Team
 *
 * \copyright
 * This program is free software under the
 * GNU General Public License (>=v2).
 * Read the file COPYING that comes with GRASS for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <grass/dataquad.h>
#include <grass/qtree.h>

/*! Initializes multfunc structure with given arguments */
struct multfunc
    *MT_functions_new(int (*compare) (struct triple *, struct quaddata *),
		      struct quaddata **(*divide_data) (struct quaddata *,
							int, double),
		      int (*add_data) (struct triple *, struct quaddata *,
				       double),
		      int (*intersect) (struct quaddata *, struct quaddata *),
		      int (*division_check) (struct quaddata *, int),
		      int (*get_points) (struct quaddata *, struct quaddata *,
					 int))
{
    struct multfunc *functions;
    if (!(functions = (struct multfunc *)malloc(sizeof(struct multfunc)))) {
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

/*! Initializes tree_info using given arguments */
struct tree_info *MT_tree_info_new(struct multtree *root,
				   struct multfunc *functions, double dmin,
				   int kmax)
{
    struct tree_info *info;
    if (!(info = (struct tree_info *)malloc(sizeof(struct tree_info)))) {
	return NULL;
    }
    info->root = root;
    info->functions = functions;
    info->dmin = dmin;
    info->kmax = kmax;
    return info;
}

/** Initializes multtree using given arguments */
struct multtree *MT_tree_new(struct quaddata *data,
			     struct multtree **leafs, struct multtree *parent,
			     int multant)
{
    struct multtree *tree;
    if (!(tree = (struct multtree *)malloc(sizeof(struct multtree)))) {
	return NULL;
    }
    tree->data = data;
    tree->leafs = leafs;
    tree->parent = parent;
    tree->multant = multant;
    return tree;
}


/*!
 * First checks for dividing cond. (if n_points>=KMAX) and tree
 * is a leaf by calling one of tree's functions (`division_check()`).
 * If tree is not a leaf (is a node) uses function compare to determine
 * into which "son" we need to insert the point and calls MT_insert()
 * with this son as a n argument.
 *
 * If TREE is a leaf but we don't need to divide it (n_points<KMAX) then
 * calls function `add_data(point, ...)` to add point to the data of tree
 * and returns the result of `add_data()` (which returns 1 if the point is
 * inserted and 0 if its ignored (when its too dense)).
 *
 * If `division_check()` returns true, calls MT_divide() and then calls
 * MT_insert() to insert the point into divided tree and returns the
 * result of MT_divide().
 */
int MT_insert(struct triple *point,
	      struct tree_info *info, struct multtree *tree, int n_leafs)
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
    i = info->functions->division_check(tree->data, info->kmax);
    if (i <= 0) {
	if (i == -1) {
	    comp = info->functions->compare(point, tree->data);
	    if ((comp < 1) || (comp > n_leafs))
		return -3;
	    j = MT_insert(point, info, tree->leafs[comp - 1], n_leafs);
	}
	else {
	    if (i == 0) {
		j = info->functions->add_data(point, tree->data, info->dmin);
	    }
	}
    }
    else {
	k = MT_divide(info, tree, n_leafs);
	if (k == 1)
	    j = MT_insert(point, info, tree, n_leafs);
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


/*!
 * Divide a tree
 * 
 * Divides the tree by calling one of tree's functions (divide_data())
 * and returns the result of divide_data()
 */
int MT_divide(struct tree_info *info, struct multtree *tree, int n_leafs)
{
    int i;
    struct quaddata **datas;
    struct multtree *par;
    struct multtree **leafs;

    datas = info->functions->divide_data(tree->data, info->kmax, info->dmin);
    if (datas == NULL) {
	fprintf(stderr, "datas is NULL\n");
	return -7;
    }
    par = tree;
    leafs = (struct multtree **)malloc(sizeof(struct multtree *) * n_leafs);
    for (i = 1; i <= n_leafs; i++) {
	leafs[i - 1] = MT_tree_new(datas[i], NULL, par, i);
    }
    tree->leafs = leafs;
    return 1;
}





/*!
 * Get points inside a region from a tree
 *
 * Gets points inside the region defined by DATA from TREE and
 * adds them to DATA. If the number of eligible
 * point is more than MAX returns MAX+1 otherwise returns number of points added
 * to DATA.
 *
 * Uses tree's functions intersect() to find leafs that intersect given region
 * and get_points() to get points from such leafs.
 */
int MT_region_data(struct tree_info *info, struct multtree *tree,
                   struct quaddata *data,
                   int MAX,  /*!< max number of points we can add (KMAX2) */
                   int n_leafs
                   )
{
    int n = 0, j;

    if (tree == NULL) {
	fprintf(stderr, "MT_region_data: tree is NULL\n");
	return n;
    }
    if (tree->data == NULL) {
	fprintf(stderr, "MT_region_data: data is NULL\n");
	return n;
    }
    if (info->functions->intersect(data, tree->data)) {
	if (tree->leafs != NULL) {
	    for (j = 0; j < n_leafs; j++) {
		if ((n =
		     n + MT_region_data(info, tree->leafs[j], data, MAX - n,
					n_leafs)) > MAX)
		    return n;
	    }
	}
	else {
	    n = info->functions->get_points(data, tree->data, MAX);
	}
	return n;
    }
    return 0;
}

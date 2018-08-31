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


#ifndef TREE_H

#define TREE_H

#define VOID_T char


/*!
 * Function table for a tree
 *
 * From object oriented point of view, this structure represents
 * a class or a virtual table of functions/methods for a class.
 */
struct multfunc
{
    int (*compare) ();
    struct quaddata **(*divide_data) ();
    int (*add_data) ();
    int (*intersect) ();
    int (*division_check) ();
    int (*get_points) ();
};

struct tree_info
{
    struct multfunc *functions;
    double dmin;
    int kmax;
    struct multtree *root;
};

struct multtree
{
    struct quaddata *data;
    struct multtree **leafs;
    struct multtree *parent;
    int multant;
};

struct multfunc *MT_functions_new(int (*)(struct triple *, struct quaddata *),
				  struct quaddata **(*)(struct quaddata *,
							int, double),
				  int (*)(struct triple *, struct quaddata *,
					  double), int (*)(struct quaddata *,
							   struct quaddata *),
				  int (*)(struct quaddata *, int),
				  int (*)(struct quaddata *,
					  struct quaddata *, int));
struct tree_info *MT_tree_info_new(struct multtree *, struct multfunc *,
				   double, int);
struct multtree *MT_tree_new(struct quaddata *, struct multtree **,
			     struct multtree *, int);
int MT_insert(struct triple *, struct tree_info *, struct multtree *, int);
int MT_divide(struct tree_info *, struct multtree *, int);
int MT_region_data(struct tree_info *, struct multtree *, struct quaddata *,
		   int, int);

#endif

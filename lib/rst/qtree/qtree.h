
/*-
 * Written by H. Mitasova, I. Kosinovsky, D. Gerdes Fall 1993
 * University of Illinois
 * US Army Construction Engineering Research Lab  
 * Copyright 1993, H. Mitasova (University of Illinois),
 * I. Kosinovsky, (USA-CERL), and D.Gerdes (USA-CERL)   
 *
 * updated by Mitasova Nov. 96, no changes necessary 
 */


#ifndef TREE_H

#define TREE_H

#define VOID_T char


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

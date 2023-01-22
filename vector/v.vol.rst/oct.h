#ifndef __OCT_H__
#define __OCT_H__

#include "dataoct.h"

struct octfunc {
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
    int (*compare)(struct quadruple *, struct octdata *);
    struct octdata **(*divide_data)(struct octdata *);
    int (*add_data)(struct quadruple *, struct octdata *);
    int (*intersect)(double, double, double, double, double, double,
                     struct octdata *);
    int (*division_check)(struct octdata *);
    int (*get_points)(struct quadruple *, struct octdata *, double, double,
                      double, double, double, double, int);
<<<<<<< HEAD
};

struct octtree {
    struct octdata *data;
=======
    int (*compare)();
    VOID_T **(*divide_data)();
    int (*add_data)();
    int (*intersect)();
    int (*division_check)();
    int (*get_points)();
};

struct octtree {
=======
    int (*compare)();
    VOID_T **(*divide_data)();
    int (*add_data)();
    int (*intersect)();
    int (*division_check)();
    int (*get_points)();
};

struct octtree {
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
    VOID_T *data;
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
};

struct octtree {
    struct octdata *data;
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
    struct octtree **leafs;
    struct octtree *parent;
    struct octfunc *functions;
    int octant;
};

struct octfunc *OT_functions_new(
    int (*compare)(struct quadruple *, struct octdata *),
    struct octdata **(*divide_data)(struct octdata *),
    int (*add_data)(struct quadruple *, struct octdata *),
    int (*intersect)(double, double, double, double, double, double,
                     struct octdata *),
    int (*division_check)(struct octdata *),
    int (*get_points)(struct quadruple *, struct octdata *, double, double,
                      double, double, double, double, int));
struct octtree *OT_tree_new(struct octdata *, struct octtree **,
                            struct octtree *, struct octfunc *, int octant);
int OT_insert_oct(struct quadruple *, struct octtree *);
int OT_region_data(struct octtree *, double, double, double, double, double,
                   double, struct quadruple *, int);

#endif

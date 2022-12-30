#include <grass/gis.h>

struct order {
    int cat;   /* point category */
<<<<<<< HEAD
    int count; /* number of points with category 'cat' */
=======
    int count; /* nuber of points with category 'cat' */
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
    double x;
    double y;
    double z;
    FCELL fvalue;
    DCELL dvalue;
};

/* search.c */
int by_cat(const void *, const void *);
int srch_cat(const void *, const void *);

#include <grass/gis.h>

struct order {
    int cat;   /* point category */
<<<<<<< HEAD
<<<<<<< HEAD
    int count; /* nuber of points with category 'cat' */
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
    int count; /* number of points with category 'cat' */
=======
    int count; /* nuber of points with category 'cat' */
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    int count; /* nuber of points with category 'cat' */
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
    double x;
    double y;
    double z;
    FCELL fvalue;
    DCELL dvalue;
};

/* search.c */
int by_cat(const void *, const void *);
int srch_cat(const void *, const void *);

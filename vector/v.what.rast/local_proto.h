#include <grass/gis.h>

struct order {
    int cat;   /* point category */
<<<<<<< HEAD
<<<<<<< HEAD
    int count; /* number of points with category 'cat' */
=======
    int count; /* nuber of points with category 'cat' */
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    int count; /* nuber of points with category 'cat' */
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
    int row;
    int col;
    double x, y; /* used with interp flag */
    CELL value;
    DCELL dvalue; /* used for FCELL and DCELL */
};

/* search.c */
int by_row(const void *, const void *);
int by_cat(const void *, const void *);
int srch_cat(const void *, const void *);

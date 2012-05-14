#include <grass/gis.h>

struct order
{
    int cat;			/* point category */
    int count;			/* nuber of points with category 'cat' */
    double x;
    double y;
    double z;
    FCELL fvalue;
    DCELL dvalue;
};

/* search.c */
int by_cat(const void *, const void *);
int srch_cat(const void *, const void *);

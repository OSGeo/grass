#include <grass/gis.h>

struct order {
    int cat;   /* point category */
    int count; /* number of points with category 'cat' */
    int row;
    int col;
    double x, y; /* used with interp flag */
    CELL value;
    DCELL dvalue; /* used for FCELL and DCELL */
};

/* search.c */
int by_row_col(const void *, const void *);
int by_cat(const void *, const void *);
int srch_cat(const void *, const void *);

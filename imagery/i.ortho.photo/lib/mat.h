/* mat.h */

#define MAXROWS 25
#define MAXCOLS MAXROWS

typedef struct matrix
{
    int nrows;			/* row index */
    int ncols;			/* col index */
    double x[MAXROWS][MAXCOLS];
} MATRIX;

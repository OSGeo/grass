#include <stdio.h>
#include <math.h>
#include <grass/raster.h>
#include "flag.h"

#define NODE		struct _n_o_d_e_
#define INIT_AR		64
#define AR_INCR		64
#define ABS(x)		(((x) < 0) ? -(x) : (x))
#define MIN(x,y)	(((x) < (y)) ? (x) : (y))
#define TST(x)			/* fprintf(stderr,"(x):%d\n",(x)) */
#define TSTSTR(x)		/* fprintf(stderr,"%s\n",(x)) */

NODE {
    int r, c;
    double d;
};

extern int nrows;
extern int ncols;
extern int minc;
extern int minr;
extern int maxc;
extern int maxr;
extern int array_size;
extern double i_val_l_f;
extern DCELL **con;
extern FLAG *seen, *mask;
extern NODE *zero;

/* add_in.c */
NODE *add_in(int, int, int, int, NODE *, int *);

/* addpts.c */
NODE *addpts(NODE *, int, int, int, int, int *);

/* find_con.c */
int find_con(int, int, double *, double *, DCELL *, DCELL *);

/* read_cell.c */
DCELL **read_cell(const char *);
void free_cell(DCELL **);

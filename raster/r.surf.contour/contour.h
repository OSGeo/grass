#include <stdio.h>
#include <math.h>
#include <grass/gis.h>
#include "flag.h"
#include "cseg.h"

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

#ifdef MAIN
#define GLOBAL
#else
#define GLOBAL extern
#endif

GLOBAL int nrows;
GLOBAL int ncols;
GLOBAL int minc;
GLOBAL int minr;
GLOBAL int maxc;
GLOBAL int maxr;
GLOBAL int array_size;
GLOBAL double i_val_l_f;
GLOBAL CSEG con;
GLOBAL FLAG *seen, *mask;
GLOBAL BSEG bseen, bmask;
GLOBAL NODE *zero;
GLOBAL CELL on, off;

/* add_in.c */
NODE *add_in_slow(int, int, int, int, NODE *, int *);
NODE *add_in(int, int, int, int, NODE *, int *);

/* addpts.c */
NODE *addpts_slow(NODE *, int, int, int, int, int *);
NODE *addpts(NODE *, int, int, int, int, int *);

/* find_con.c */
int find_con_slow(int, int, double *, double *, CELL *, CELL *);
int find_con(int, int, double *, double *, CELL *, CELL *);

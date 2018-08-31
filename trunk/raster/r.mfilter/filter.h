#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/rowio.h>
typedef struct
{
    int size;			/* size of filter matrix */
    double **matrix;		/* filter coefficient matrix */
    double **dmatrix;		/* divisor coefficient matrix */
    double divisor;		/* filter scale factor */
    int type;			/* sequential or parallel */
    int start;			/* starting corner */
} FILTER;

#define PARALLEL 1
#define SEQUENTIAL 2
#define UL 1
#define UR 2
#define LL 3
#define LR 4

/* apply.c */
DCELL apply_filter(FILTER *, DCELL **);

/* getfilt.c */
FILTER *get_filter(char *, int *, char *);

/* perform.c */
int perform_filter(const char *, const char *, FILTER *, int, int);

/* execute.c */
int execute_filter(ROWIO *, int, FILTER *, DCELL *);

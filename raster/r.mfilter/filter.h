#include <grass/gis.h>
typedef struct
{
    int size;			/* size of filter matrix */
    int **matrix;		/* filter coefficient matrix */
    int **dmatrix;		/* divisor coefficient matrix */
    int divisor;		/* filter scale factor */
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
CELL apply_filter(FILTER *, CELL **);

/* getfilt.c */
FILTER *get_filter(char *, int *, char *);

/* perform.c */
int perform_filter(char *, char *, char *, FILTER *, int, int);

#ifdef GRASS_ROWIO_H
/* execute.c */
int execute_filter(ROWIO *, int, FILTER *, CELL *);
#endif

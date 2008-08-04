/* ransurf.h                                                            */

#include <stdio.h>
#include <math.h>
#include <grass/gis.h>
#include "flag.h"

#define ODD(a)	((a) & 1)

#define SEED_MAX		54772
#define SEED_MIN		0
#define PI       		M_PI

#ifdef MAIN
#define GLOBAL
#else
#define GLOBAL extern
#endif

#define CELLSORTER struct cell_sorter_
CELLSORTER {
    int R, C;
    double Value;
};

GLOBAL double NS, EW;
GLOBAL int CellCount, Rs, Cs;
GLOBAL double MaxDist, MaxDistSq;
GLOBAL FLAG *Cells;
GLOBAL CELLSORTER *DoNext;
GLOBAL CELL **Out, *CellBuffer;
GLOBAL int Seed, OutFD;
GLOBAL struct Flag *Verbose;
GLOBAL struct Option *Distance;
GLOBAL struct Option *Output;

#ifdef DEBUG
#define INDX(a,b) (printf("(a)[%d]:%lf ",(b),(a)[(b)]))
#define CHARS(a) (printf("(a):%s ",(a)))
#define DOUBLE(a) (printf("(a):%.12lf ",(a)))
#define INT(a) (printf("(a):%d ",(a)))
#define RETURN (printf("\n"))
#else
#define INDX(a,b)
#define CHARS(a)
#define DOUBLE(a)
#define INT(a)
#define RETURN
#endif

#ifdef TRACE
#define FUNCTION(a) (printf("Function:(a)\n"))
#else
#define FUNCTION(a)
#endif

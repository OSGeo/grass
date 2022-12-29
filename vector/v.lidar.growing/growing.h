#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include <grass/lidar.h>

/*--------------------------------------------------------------------------*/
/* Definitions for Convex-Hull algorithm */
#define NR_END 1
#define FREE_ARG char*

#define CMPM(c,A,B) \
v = (*(double**)A)[c] - (*(double**)B)[c];\
if (v>0) return 1;\
if (v<0) return -1;

/*--------------------------------------------------------------------------*/
/*STRUCTS DECLARATION */
struct element_grow
{
    double interp;		/* Interpolated value */
    int fi;			/* Interpolated value counter */
    int bordo;			/* Border point */
    int dueImp;			/* Double impulse point */
    double orig;		/* Original value */
    int fo;			/* Original value counter */
    double clas;		/* Classification */
    int fc;			/* Classification counter */
    int obj;			/* Object counter */
};


/*--------------------------------------------------------------------------*/
/*FUNCTIONS DECLARATION */
void P_Aux_to_Coor(struct Map_info *, /**/
		   struct Map_info *, /**/ dbDriver *, /**/ FILE * /**/);

/* Convex-Hull */
struct element_grow **P_alloc_element(int, int);
double **Pvector(long, long);
double pianOriz(double **, int, double *, double *, double *, double *,
		struct element_grow **, int);
void nrerror(char error_text[]);
void regGrow8(struct Cell_head, struct element_grow **, double **, int *, int,
	      int, int, double, int);
int checkHull(int, int, double **, int);
int ch2d(double **, int);
int ccw(double **, int, int, int);
int cmpl(const void *, const void *);
int cmph(const void *, const void *);
struct element_grow **structMatrix(long, long, long, long);
void free_Pvector(double **, long, long);
void free_structmatrix(struct element_grow **, long, long, long, long);

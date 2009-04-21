#include <grass/gis.h>

#define	cv(i,j)		cell[i][j]
#define	av(i,j)		a[i][j]
#define	atbv(i,j)	atb[i][j]
#define	IScvNULL(i,j)	G_is_d_null_value(&cv(i,j))
#define	ISatbvNULL(i,j)	G_is_d_null_value(&atbv(i,j))

#define	ZERO		0.0000001

#ifdef _MAIN_C_
#define GLOBAL
#else
#define GLOBAL extern
#endif

GLOBAL char *iname, *oname;
GLOBAL struct Cell_head window;
GLOBAL DCELL **cell;
GLOBAL DCELL **atb, **a;

void getcells(void);
void putcells(void);
void initialize(void);
void atanb(void);

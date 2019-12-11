#include <float.h>

#include <grass/gis.h>
#include <grass/raster.h>

#define	cv(i,j)			cell[i][j]
#define	av(i,j)			a[i][j]
#define	atbv(i,j)		atb[i][j]
#define	is_cv_null(i,j)		Rast_is_d_null_value(&cv(i,j))
#define	is_atbv_null(i,j)	Rast_is_d_null_value(&atbv(i,j))
#define is_atbv_unprocessed(i,j) (atbv(i,j) == UNPROCESSED)

#ifndef DBL_MAX
#define DBL_MAX 1.797693E308  /* DBL_MAX approximation */
#endif
#define	ZERO			0.0000001
#define	UNPROCESSED		-DBL_MAX

#ifdef _MAIN_C_
#define GLOBAL
#else
#define GLOBAL extern
#endif

GLOBAL char *input, *output;
GLOBAL struct Cell_head window;
GLOBAL DCELL **cell;
GLOBAL DCELL **atb, **a;

void read_cells(void);
void write_cells(void);
void calculate_statistics(void);
void initialize(void);
void calculate_atanb(void);

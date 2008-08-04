
/***********************************************************************
 *
 * MODULE:       v.surf.bspline
 *
 * AUTHOR(S):    Roberto Antolin
 *
 * PURPOSE:      Spline Interpolation and cross correlation
 *
 * COPYRIGHT:    (C) 2006 by Politecnico di Milano -
 *			     Polo Regionale di Como
 *
 *               This program is free software under the
 *               GNU General Public License (>=v2).
 *               Read the file COPYING that comes with GRASS
 *               for details.
 *
 **************************************************************************/

 /*INCLUDES*/
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/Vect.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>
#include <grass/config.h>
#include <grass/PolimiFunct.h>
     /*STRUCTURES*/ struct Stats
{
    double *estima;
    double *error;
    int n_points;
};

struct Param
{
    struct Option *in, *in_ext, *out, *out_map, *dbdriver,
	*dbdatabase, *passoE, *passoN, *lambda_f, *type;
    struct Flag *cross_corr;
};

struct SubZone
{
    int row;
    int col;
};


/*-------------------------------------------------------------------------------------------*/
 /*FUNCTIONS*/
    /* CrossCorrelation.c */
int cross_correlation(struct Map_info *, double, double);
double calc_mean(double *, int);
double calc_root_mean_square(double *, int);
double calc_standard_deviation(double *, int);
struct Stats alloc_Stats(int);
double find_minimum(double *, int *);

/*readwritedb.c */
int Create_tmp_table(dbDriver *, char *);
int Point_to_db(struct Map_info *, dbDriver *);
struct SubZone where_am_i(double, double);
struct Point *swap(struct Point *, int, int);

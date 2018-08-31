
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
#include <grass/lidar.h>
     /*STRUCTURES*/ struct Stats
{
    double *estima;
    double *error;
    int n_points;
};

struct Param
{
    struct Option *in, *in_ext, *out, *out_map, *dbdriver,
	*dbdatabase, *stepE, *stepN, *lambda_f, *type;
    struct Flag *cross_corr;
};

struct RasterPoint
{
    int row;
    int col;
};

/* resamp.c */
struct Point *P_Read_Raster_Region_masked(SEGMENT *, /**/
				          struct Cell_head *, /**/
				          struct bound_box, /**/
				          struct bound_box, /**/
				          int *, /**/ int, /**/ double);
int P_Sparse_Raster_Points(SEGMENT *, /**/
			struct Cell_head *, /**/
			struct Cell_head *, /**/
			struct bound_box, /**/
			struct bound_box, /**/
			struct Point *, /**/
			double *, /**/
			double, /**/
			double, /**/
			double, /**/
			int, /**/
			int, /**/
			int, /**/
			int, /**/
			double /**/);
int align_elaboration_box(struct Cell_head *, struct Cell_head *, int);
int align_interp_boxes(struct bound_box *, struct bound_box *,
                       struct Cell_head *, struct bound_box, struct bound_box, int);

				       
/* crosscor.c */
int cross_correlation(SEGMENT *, struct Cell_head *, double, double);

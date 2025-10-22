/*!
 * \file point2d_parallel.c
 *
 * \author
 * Lubos Mitas (original program and various modifications)
 *
 * \author
 * C.Y. Liang,
 * H. Mitasova,
 * I. Kosinovsky,
 * D. Gerdes,
 * D. McCauley
 * (GRASS4.1 version of the program and GRASS4.2 modifications)
 *
 * \author modified by Liang in May 2024
 * \author modified by McCauley in August 1995
 * \author modified by Mitasova in August 1995, Nov. 1996
 *
 * \copyright
 * (C) 1993-2006 by Helena Mitasova and the GRASS Development Team
 *
 * \copyright
 * This program is free software under the
 * GNU General Public License (>=v2).
 * Read the file COPYING that comes with GRASS for details.
 */

#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/interpf.h>

/* needed for AIX */
#ifdef hz
#undef hz
#endif

/*!
 * \brief A parallel version of IL_check_at_points_2d.
 * Sperate the cross-validation/deviation computing part
 * and the database writing part.
 *
 * \param params interpolation parameters
 * \param data data in the segment for computing, NULL for writing out
 * \param b solution of linear equations for computing, NULL for writing out
 * \param ertot total error for computing, point at single point for writing out
 * \param zmin min z-value for computing
 * \param dnorm normalization factor for computing
 * \param target_point point for computing or writing out
 *
 * \return 1
 *
 * \todo
 * Alternative description:
 * ...calculate the maximum and RMS deviation caused by smoothing.
 */
int IL_check_at_points_2d_cvdev(struct interp_params *params,
                                struct quaddata *data, /*!< current region */
                                double *b, /*!< solution of linear equations */
                                double *ertot, /*!< total error */
                                double zmin,   /*!< min z-value */
                                double dnorm, struct triple *target_point)
{
    if ((data != NULL) && (b != NULL)) {
        int n_points = data->n_points;        /* number of points */
        struct triple *points = data->points; /* points for interpolation */
        double west = data->x_orig;
        double south = data->y_orig;
        double /* rfsta2, errmax, */ h, xx, yy, r2, hz, zz, err, xmm, ymm, r;
        int /* n1, */ m;

        if (params->cv) { /* one point is skipped for cross-validation*/
            n_points -= 1;
        }

        h = b[0];
        for (m = 1; m <= n_points; m++) {
            xx = target_point->x - points[m - 1].x;
            yy = target_point->y - points[m - 1].y;
            r2 = yy * yy + xx * xx;
            if (r2 != 0.) {
                /* rfsta2 = fstar2 * r2; */
                r = r2;
                h = h + b[m] * params->interp(r, params->fi);
            }
        }
        /* modified by helena january 1997 - normalization of z was
            removed from segm2d.c and interp2d.c
            hz = (h * dnorm) + zmin;
            zz = (points[mm - 1].z * dnorm) + zmin;
        */
        hz = h + zmin;
        zz = target_point->z + zmin;
        err = hz - zz;
        xmm = target_point->x * dnorm + params->x_orig + west;
        ymm = target_point->y * dnorm + params->y_orig + south;
        (*ertot) += err * err;

        /* take the values out*/
        target_point->x = xmm;
        target_point->y = ymm;
        target_point->z = err;
    }
    else {
        IL_write_point_2d(*target_point, *ertot);
    }
    return 1;
}

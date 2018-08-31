/*
 ****************************************************************************
 *
 * MODULE:       v.vol.rst: program for 3D (volume) interpolation and geometry
 *               analysis from scattered point data using regularized spline
 *               with tension
 *
 * AUTHOR(S):    Original program (1989) and various modifications:
 *               Lubos Mitas
 *
 *               GRASS 4.2, GRASS 5.0 version and modifications:
 *               H. Mitasova,  I. Kosinovsky, D. Gerdes, J. Hofierka
 *
 * PURPOSE:      v.vol.rst interpolates the values to 3-dimensional grid from
 *               point data (climatic stations, drill holes etc.) given in a
 *               3D vector point input. Output grid3 file is elev. 
 *               Regularized spline with tension is used for the
 *               interpolation.
 *
 * COPYRIGHT:    (C) 1989, 1993, 2000 L. Mitas,  H. Mitasova,
 *               I. Kosinovsky, D. Gerdes, J. Hofierka
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

#include "oct.h"
#include "surf.h"
#include "dataoct.h"
#include "userextern.h"
#include "userglobs.h"
#include "user.h"
#include <grass/raster3d.h>
#include "points.h"

int point_save(double xmm, double ymm, double zmm, double err)




/*
   c  saves point deviations
   c
 */
{
    int cat;

    Vect_reset_line(Pnts);
    Vect_reset_cats(Cats);

    Vect_append_point(Pnts, xmm, ymm, zmm);
    cat = count;
    Vect_cat_set(Cats, 1, cat);
    Vect_write_line(&Map, GV_POINT, Pnts, Cats);

    db_zero_string(&sql);
    sprintf(buf, "insert into %s values ( %d ", f->table, cat);
    db_append_string(&sql, buf);

    sprintf(buf, ", %f", err);
    db_append_string(&sql, buf);
    db_append_string(&sql, ")");
    G_debug(3, "%s", db_get_string(&sql));

    if (db_execute_immediate(driver, &sql) != DB_OK) {
	db_close_database(driver);
	db_shutdown_driver(driver);
	G_fatal_error(_("Cannot insert new row: %s"), db_get_string(&sql));
    }
    count++;

    return 1;
}

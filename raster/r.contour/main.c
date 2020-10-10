
/****************************************************************************
 *
 * MODULE:       r.contour
 *
 * AUTHOR(S):    Terry Baker - CERL
 *               Andrea Aime <aaime liberto it>
 *
 * PURPOSE:      Produces a vector map of specified contours from a 
 *               raster map layer.
 *
 * COPYRIGHT:    (C) 2001-2008 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 ***************************************************************************/

/*
 * Fixes 3/2001: Andrea Aime  <aaime libero.it>
 *
 * o category support bug has been fixed: r.contour now produces also dig_cats
 *   file, so d.what.vect no more complain about missing category support.
 *   Since a contour level could be floatig point I've inserted in the label
 *   the true value, whilst category number remains its integer equivalent;
 * o a little trick (derived from GMT contour program) avoids any kind of
 *   backtrack, so running v.spag -i is no more needed (I hope): data matrix
 *   is parsed, and if a cell whose data matches exactly a contour value its
 *   value is added a small quantity (the smaller one that makes cell[i] +
 *   small != cell[i], I've taken into consideration double encoding)
 * o one can specify a minimum number of point for a contour line to be put
 *   into outout, so that small spurs, single points and so on won't be
 *   present and the output will be more clear (that's optional anyway);
 * o in my opinion there were minor memory handling problems in r.contour,
 *   I've corrected them (Head.map_name was not guaranteed to be properly          
 *   terminated, Points structures in contour function were not                    
 *   deallocated).    
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <float.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/dbmi.h>
#include <grass/vector.h>
#include <grass/glocale.h>
#include "local_proto.h"

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct Option *map;
    struct Option *levels;
    struct Option *vect;
    struct Option *min;
    struct Option *max;
    struct Option *step;
    struct Option *cut;
    struct Flag *notable;

    int i;

    struct Cell_head Wind;
    char *name;
    struct Map_info Map;
    DCELL **z_array;
    struct FPRange range;
    int fd;
    double *lev;
    double snap;
    int nlevels;
    int n_cut;

    /* Attributes */
    struct field_info *Fi;
    dbDriver *Driver;
    char buf[2000];
    dbString sql;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("surface"));
    G_add_keyword(_("contours"));
    G_add_keyword(_("vector"));
    module->description = _("Produces a vector map of specified "
			    "contours from a raster map.");

    map = G_define_standard_option(G_OPT_R_INPUT);
    vect = G_define_standard_option(G_OPT_V_OUTPUT);

    step = G_define_option();
    step->key = "step";
    step->type = TYPE_DOUBLE;
    step->required = NO;
    step->description = _("Increment between contour levels");
    step->guisection = _("Contour levels");

    levels = G_define_option();
    levels->key = "levels";
    levels->type = TYPE_DOUBLE;
    levels->required = NO;
    levels->multiple = YES;
    levels->description = _("List of contour levels");
    levels->guisection = _("Contour levels");

    min = G_define_option();
    min->key = "minlevel";
    min->type = TYPE_DOUBLE;
    min->required = NO;
    min->description = _("Minimum contour level");
    min->guisection = _("Contour levels");

    max = G_define_option();
    max->key = "maxlevel";
    max->type = TYPE_DOUBLE;
    max->required = NO;
    max->description = _("Maximum contour level");
    max->guisection = _("Contour levels");

    cut = G_define_option();
    cut->key = "cut";
    cut->type = TYPE_INTEGER;
    cut->required = NO;
    cut->answer = "2";
    cut->description =
	_("Minimum number of points for a contour line (0 -> no limit)");

    notable = G_define_standard_flag(G_FLG_V_TABLE);

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    if (!levels->answers && !step->answer) {
        G_fatal_error(_("Either <%s> or <%s> option must be specified"),
                      levels->key, step->key);
    }

    name = map->answer;

    fd = Rast_open_old(name, "");

    if (Rast_read_fp_range(name, "", &range) < 0)
	G_fatal_error(_("Unable to read fp range of raster map <%s>"),
		      name);

    /* get window info */
    G_get_window(&Wind);

    if (Vect_open_new(&Map, vect->answer, 1) < 0)
	G_fatal_error(_("Unable to create vector map <%s>"), vect->answer);

    Vect_hist_command(&Map);

    if (!notable->answer) {
        db_init_string(&sql);
    
        /* Open database, create table */
        Fi = Vect_default_field_info(&Map, 1, NULL, GV_1TABLE);
        Vect_map_add_dblink(&Map, Fi->number, Fi->name, Fi->table, Fi->key,
                            Fi->database, Fi->driver);
    
        Driver =
            db_start_driver_open_database(Fi->driver,
                                          Vect_subst_var(Fi->database, &Map));
        if (Driver == NULL)
            G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
                          Fi->database, Fi->driver);
    
        sprintf(buf, "create table %s ( cat integer, level double precision )",
                Fi->table);
    
        db_set_string(&sql, buf);
    
        G_debug(1, "SQL: %s", db_get_string(&sql));
    
        if (db_execute_immediate(Driver, &sql) != DB_OK) {
            G_fatal_error(_("Unable to create table: '%s'"),
                          db_get_string(&sql));
        }
    
        if (db_create_index2(Driver, Fi->table, Fi->key) != DB_OK)
            G_warning(_("Unable to create index for table <%s>, key <%s>"),
                      Fi->table, Fi->key);
    
        if (db_grant_on_table
            (Driver, Fi->table, DB_PRIV_SELECT, DB_GROUP | DB_PUBLIC) != DB_OK)
            G_fatal_error(_("Unable to grant privileges on table <%s>"),
                          Fi->table);
    }

    z_array = get_z_array(fd, Wind.rows, Wind.cols);
    lev = getlevels(levels, max, min, step, &range, &nlevels);
    displaceMatrix(z_array, Wind.rows, Wind.cols, lev, nlevels);
    n_cut = atoi(cut->answer);
    contour(lev, nlevels, Map, z_array, Wind, n_cut);

    G_message(_("Writing attributes..."));
    /* Write levels */

    if (!notable->answer) {
        db_begin_transaction(Driver);
        for (i = 0; i < nlevels; i++) {
            sprintf(buf, "insert into %s values ( %d, %e )", Fi->table, i + 1,
                    lev[i]);
            db_set_string(&sql, buf);
            
            G_debug(3, "SQL: %s", db_get_string(&sql));
            
            if (db_execute_immediate(Driver, &sql) != DB_OK) {
                G_fatal_error(_("Unable to insert new record: '%s'"), db_get_string(&sql));
            }
        }
        db_commit_transaction(Driver);
        
        db_close_database_shutdown_driver(Driver);
    }
    Vect_build(&Map);

    /* if a contour line hits a border of NULL cells, it traces 
     * itself back until it hits a border of NULL cells again,
     * then goes back to the starting point
     * -> cleaning is needed */
    snap = (Wind.ns_res + Wind.ew_res) / 2000.0;
    G_message(_("Snap lines"));
    Vect_snap_lines(&Map, GV_LINE, snap, NULL);
    G_message(_("Break lines at intersections"));
    Vect_break_lines(&Map, GV_LINE, NULL);
    G_message(_("Remove duplicates"));
    Vect_remove_duplicates(&Map, GV_LINE, NULL);
    G_message(_("Merge lines"));
    Vect_merge_lines(&Map, GV_LINE, NULL, NULL);
    Vect_build_partial(&Map, GV_BUILD_NONE);
    Vect_build(&Map);
    
    Vect_close(&Map);

    exit(EXIT_SUCCESS);
}

/*********************************************************************/
DCELL **get_z_array(int fd, int nrow, int ncol)
{
    DCELL **z_array;
    int i;

    z_array = (DCELL **) G_malloc(nrow * sizeof(DCELL *));

    G_message(_("Reading data..."));

    for (i = 0; i < nrow; i++) {
	z_array[i] = (DCELL *) G_malloc(ncol * sizeof(DCELL));
	Rast_get_d_row(fd, z_array[i], i);
	G_percent(i + 1, nrow, 2);
    }
    return z_array;
}


/********************************************************************/
double *getlevels(struct Option *levels,
		  struct Option *max, struct Option *min,
		  struct Option *step, struct FPRange *range, int *num)
{
    double dmax, dmin, dstep;
    int nlevels, i, k, n;
    double j;
    DCELL zmin, zmax;		/* min and max data values */
    double *lev;
    double tmp;

    Rast_get_fp_range_min_max(range, &zmin, &zmax);

    if (!Rast_is_d_null_value(&zmin) && !Rast_is_d_null_value(&zmax))
	G_verbose_message(_("Range of data: min=%f, max=%f"), zmin, zmax);
    else
	G_verbose_message(_("Range of data: empty"));

    nlevels = 0;
    if (levels->answers) {
	for (n = 0; levels->answers[n] != NULL; n++)
	    nlevels++;

	lev = (double *)G_malloc((nlevels) * sizeof(double));

	n = nlevels;
	k = 0;
	for (i = 0; i < n; i++) {
	    j = atof(levels->answers[i]);
	    if ((j < zmin) || (j > zmax)) {
		nlevels--;
	    }
	    else {
		lev[k] = j;
		k++;
	    }
	}
    }
    else {			/* step */

	dstep = atof(step->answer);
	/* fix if step < 1, Roger Bivand 1/2001: */

	dmax = (max->answer) ? atof(max->answer) :
	    dstep == 0 ? (G_fatal_error(_("This step value is not allowed")), 0) :
	    zmax - fmod(zmax, dstep);
	dmin = (min->answer) ? atof(min->answer) :
	    dstep == 0 ? (G_fatal_error(_("This step value is not allowed")), 0) :
	    fmod(zmin, dstep) ? zmin - fmod(zmin, dstep) + dstep :
	    zmin;

	while (dmin < zmin) {
	    dmin += dstep;
	}
	while (dmin > zmax) {
	    dmin -= dstep;
	}

	while (dmax > zmax) {
	    dmax -= dstep;
	}
	while (dmax < zmin) {
	    dmax += dstep;
	}
	if (dmin > dmax) {
	    tmp = dmin;
	    dmin = dmax;
	    dmax = tmp;
	}
	dmin = dmin < zmin ? zmin : dmin;
	dmax = dmax > zmax ? zmax : dmax;

	G_verbose_message(_("Range of levels: min = %f, max = %f"), dmin, dmax);

	nlevels = (dmax - dmin) / dstep + 2;
	lev = (double *)G_malloc(nlevels * sizeof(double));
	for (nlevels = 0; dmin < dmax; dmin += dstep) {
	    lev[nlevels] = dmin;
	    nlevels++;
	}
	lev[nlevels] = dmax;
	nlevels++;
    }

    *num = nlevels;
    return lev;
}


/********************************************************************/
/*      parse the matrix and offset values that exactly match a                 */
/*      contour level. Contours values are added DBL_EPSILON*val, which */
/*      is defined in K&R as the minimum double x such as 1.0+x != 1.0  */

/********************************************************************/
void displaceMatrix(DCELL ** z, int nrow, int ncol, double *lev, int nlevels)
{
    int i, j, k;
    double *currRow;
    double currVal;

    G_message(_("Displacing data..."));

    for (i = 0; i < nrow; i++) {
	currRow = z[i];
	for (j = 0; j < ncol; j++) {
	    currVal = currRow[j];
	    for (k = 0; k < nlevels; k++)
		if (currVal == lev[k]) {
		    currRow[j] = currVal + currVal * DBL_EPSILON;
		    break;
		}
	}
	G_percent(i + 1, nrow, 2);
    }
}

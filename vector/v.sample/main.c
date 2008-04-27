
/*-
 * s.sample - GRASS program to sample a raster map at site locations.
 * Copyright (C) 1994. James Darrell McCauley.
 *
 * Author: James Darrell McCauley darrell@mccauley-usa.com
 * 	                          http://mccauley-usa.com/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * Modification History:
 * <04 Jan 1994> - began coding (jdm)
 * <06 Jan 1994> - announced version 0.1B on pasture.ecn.purdue.edu (jdm)
 * <21 Jan 1994> - changed wording on help screen. Revised to 0.2B (jdm)
 * <24 Jan 1994> - got rid of diagnostic messages. Revised to 0.3B (jdm)
 * ?? Revised to 0.4B (jdm)
 * <19 Dec 1994> - fixed bug in readsites, added html. Revised to 0.5B (jdm)
 * <02 Jan 1995> - cleaned Gmakefile, man page, html. 
 *                 fixed memory error in bilinear and cubic 0.6B (jdm)
 * <25 Feb 1995> - cleaned 'gcc -Wall' warnings 0.7B (jdm)
 * <15 Jun 1995> - fixed pointer error for G_{col,row}_to_{easting,northing}.
 *                 0.8B (jdm)
 * <13 Sep 2000> - released under GPL
 *
 */

/* s.sample v 0.8B <15 Jun 1995>; Copyright (c) 1994-1995. James Darrell McCauley" */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/dbmi.h>
#include <grass/Vect.h>


int main(int argc, char **argv)
{
    char *mapset;
    double scale, predicted, actual;
    INTERP_TYPE method = UNKNOWN;
    int fdrast;			/* file descriptor for raster map is int */
    struct Cell_head window;
    struct GModule *module;
    struct Map_info In, Out;
    struct
    {
        struct Option *input, *output, *rast, *z, *column;
    } parm;
    struct
    {
        struct Flag *B, *C, *q;
    } flag;

    int line, nlines;
    struct line_pnts *Points;
    struct line_cats *Cats;

    /* Attributes */
    int field, nrecords;
    int ctype;
    struct field_info *Fi;
    dbDriver *Driver;
    dbCatValArray cvarr;

    char buf[2000];
    dbString sql;

    G_gisinit(argv[0]);

    module = G_define_module();
    module->keywords = _("vector");
    module->description = _("Samples a raster map at vector point locations.");

    parm.input = G_define_standard_option(G_OPT_V_INPUT);
    parm.input->description = _("Vector map defining sample points");

    parm.column = G_define_option();
    parm.column->key = "column";
    parm.column->type = TYPE_STRING;
    parm.column->required = YES;
    parm.column->description =
        _("Vector map attribute column to use for comparison");

    parm.output = G_define_standard_option(G_OPT_V_OUTPUT);
    parm.output->description = _("Vector map to store differences");

    parm.rast = G_define_standard_option(G_OPT_R_INPUT);
    parm.rast->key = "raster";
    parm.rast->description = _("Raster map to be sampled");

    parm.z = G_define_option();
    parm.z->key = "z";
    parm.z->type = TYPE_DOUBLE;
    parm.z->required = NO;
    parm.z->answer = "1.0";
    parm.z->description =
        _("Option scaling factor for values read from raster map. "
          "Sampled values will be multiplied by this factor");

    flag.B = G_define_flag();
    flag.B->key = 'b';
    flag.B->description =
        _("Bilinear interpolation (default is nearest neighbor)");

    flag.C = G_define_flag();
    flag.C->key = 'c';
    flag.C->description =
        _("Cubic convolution interpolation (default is nearest neighbor)");

    /* please, remove before GRASS 7 released */
    flag.q = G_define_flag();
    flag.q->key = 'q';
    flag.q->description = _("Quiet");

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    sscanf(parm.z->answer, "%lf", &scale);

    if (flag.B->answer || flag.C->answer)
    {
        if (flag.C->answer)
            method = CUBIC;
        if (flag.B->answer)
            method = BILINEAR;
        if (flag.B->answer && flag.C->answer)
            G_fatal_error(_("Flags -b & -c are mutually exclusive. Choose only one."));
    } else {
        method = NEAREST;
    }

    G_get_window(&window);

    /* Open input */
    if ((mapset = G_find_vector2(parm.input->answer, "")) == NULL)
        G_fatal_error(_("Vector map <%s> not found"), parm.input->answer);

    Vect_set_open_level(2);
    Vect_open_old(&In, parm.input->answer, mapset);

    if ((mapset = G_find_cell2(parm.rast->answer, "")) == NULL)
        G_fatal_error(_("Raster map <%s> not found"), parm.rast->answer);

    if ((fdrast = G_open_cell_old(parm.rast->answer, mapset)) < 0)
        G_fatal_error(_("Unable to open raster map <%s>"), parm.rast->answer);

    /* Read attributes */
    field=1;
    Fi = Vect_get_field(&In, field);
    if (Fi == NULL)
        G_fatal_error(_("Database connection not defined for layer %d"), field);

    Driver = db_start_driver_open_database(Fi->driver, Fi->database);
    if (Driver == NULL)
        G_fatal_error("Unable to open database <%s> by driver <%s>", Fi->database, Fi->driver);

    nrecords = db_select_CatValArray(Driver, Fi->table, Fi->key, 
                      parm.column->answer, NULL, &cvarr);
    G_debug(3, "nrecords = %d", nrecords);

    ctype = cvarr.ctype;
    if (ctype != DB_C_TYPE_INT && ctype != DB_C_TYPE_DOUBLE)
        G_fatal_error(_("Column type <%s> not supported (must be integer or double precision)"), db_sqltype_name(ctype));

    if (nrecords < 0)
        G_fatal_error(_("Unable to select data from table"));

    G_message(_("%d records selected from table"), nrecords);

    db_close_database_shutdown_driver(Driver);

    /* Open output */
    Vect_open_new(&Out, parm.output->answer, 0);
    Vect_hist_copy (&In, &Out);
    Vect_hist_command (&Out);

    /* Create table */
    db_init_string(&sql);

    Fi = Vect_default_field_info(&Out, 1, NULL, GV_1TABLE);
    Vect_map_add_dblink(&Out, Fi->number, Fi->name, Fi->table, Fi->key,
                        Fi->database, Fi->driver);

    Driver = db_start_driver_open_database(Fi->driver, Vect_subst_var(Fi->database, &Out));
    if (Driver == NULL)
        G_fatal_error(_("Unable to open database <%s> by driver <%s>"), 
                      Fi->database, Fi->driver);

    sprintf(buf,
        "create table %s ( cat integer, pnt_val double precision, rast_val double precision, "
        "diff double precision)", Fi->table);
    db_set_string(&sql, buf);

    if (db_execute_immediate(Driver, &sql) != DB_OK)
        G_fatal_error(_("Unable to create table <%s>"), db_get_string(&sql));

    if (db_create_index2(Driver, Fi->table, Fi->key) != DB_OK)
        G_warning(_("Cannot create index"));

    if (db_grant_on_table(Driver, Fi->table, DB_PRIV_SELECT, DB_GROUP | DB_PUBLIC) != DB_OK)
        G_fatal_error(_("Unable to grant privileges on table <%s>"), Fi->table);

    if (flag.q->answer)
        G_message(_("Checking vector points..."));

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    nlines = Vect_get_num_lines(&In);

    for (line = 1; line <= nlines; line++) {
        int type, cat, ret, cval;
        double dval;

        G_debug(3, "line = %d", line);

        type = Vect_read_line(&In, Points, Cats, line);
        if (!(type & GV_POINT))
            continue;

        Vect_cat_get(Cats, 1, &cat);

        G_debug(4, "cat = %d", cat);

        /* find actual value */
        if (ctype == DB_C_TYPE_INT) {
            ret = db_CatValArray_get_value_int(&cvarr, cat, &cval);
            if (ret != DB_OK)
                G_warning(_("No record for category %d in table <%s>"), cat, Fi->table);

            actual = cval;
        }
        else if (ctype == DB_C_TYPE_DOUBLE)
        {
            ret = db_CatValArray_get_value_double(&cvarr, cat, &dval);
            if (ret != DB_OK)
                G_warning(_("No record for category %d in table <%s>"), cat, Fi->table);

            actual = dval;
        }
	else
        {
            G_fatal_error(_("Column type not supported"));
        }

        G_debug(4, "actual = %e", actual);

        /* find predicted value */
        predicted =
            scale * G_get_raster_sample(fdrast, &window, NULL, Points->y[0],
                                        Points->x[0], 0, method);

        G_debug(4, "predicted = %e", predicted);

        Vect_reset_cats(Cats);
        Vect_cat_set(Cats, 1, cat);

        sprintf(buf, "insert into %s values ( %d, %e, %e, %e )", 
                Fi->table, cat,	actual, predicted, predicted - actual);
        db_set_string(&sql, buf);

        if (db_execute_immediate(Driver, &sql) != DB_OK)
            G_fatal_error(_("Unable to insert row: %s"), db_get_string(&sql));

        Vect_write_line(&Out, GV_POINT, Points, Cats);
    }

    db_close_database_shutdown_driver(Driver);

    G_close_cell(fdrast);

    Vect_close(&In);

    Vect_build(&Out, stderr);
    Vect_close(&Out);

    exit(EXIT_SUCCESS);
}

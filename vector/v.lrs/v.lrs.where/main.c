/*
 *  v.lrs.where - Find line id and real km+offset for given points in vector map
 *                using linear reference system
 */

 /******************************************************************************
 * Copyright (c) 2004, Radim Blazek (blazek@itc.it)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *****************************************************************************/
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>
#include "../lib/lrs.h"

int main(int argc, char **argv)
{
    int lfield, pfield, n_points, n_outside, n_found, n_no_record,
	n_many_records;
    int line, type, nlines;
    double thresh, multip;
    struct Option *lines_opt, *points_opt;
    struct Option *lfield_opt, *pfield_opt;
    struct Option *driver_opt, *database_opt, *table_opt, *thresh_opt;
    struct GModule *module;
    const char *mapset;
    struct Map_info LMap, PMap;
    struct line_cats *LCats, *PCats;
    struct line_pnts *LPoints, *PPoints;
    dbDriver *rsdriver;
    dbHandle rshandle;
    dbString rsstmt;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("linear reference system"));
    G_add_keyword(_("network"));
    module->description =
	_("Finds line id and real km+offset for given points in vector map "
	  "using linear reference system.");

    lines_opt = G_define_standard_option(G_OPT_V_INPUT);
    lines_opt->key = "lines";
    lines_opt->description = _("Input vector map containing lines");

    points_opt = G_define_standard_option(G_OPT_V_INPUT);
    points_opt->key = "points";
    points_opt->description = _("Input vector map containing points");

    lfield_opt = G_define_standard_option(G_OPT_V_FIELD);
    lfield_opt->key = "llayer";
    lfield_opt->answer = "1";
    lfield_opt->description = _("Line layer");

    pfield_opt = G_define_standard_option(G_OPT_V_FIELD);
    pfield_opt->key = "player";
    pfield_opt->answer = "1";
    pfield_opt->description = _("Point layer");

    driver_opt = G_define_option();
    driver_opt->key = "rsdriver";
    driver_opt->type = TYPE_STRING;
    driver_opt->required = NO;
    driver_opt->description = _("Driver name for reference system table");
    driver_opt->options = db_list_drivers();
    driver_opt->answer = db_get_default_driver_name();

    database_opt = G_define_option();
    database_opt->key = "rsdatabase";
    database_opt->type = TYPE_STRING;
    database_opt->required = NO;
    database_opt->description = _("Database name for reference system table");
    database_opt->answer = db_get_default_database_name();

    table_opt = G_define_option();
    table_opt->key = "rstable";
    table_opt->type = TYPE_STRING;
    table_opt->required = YES;
    table_opt->description = _("Name of the reference system table");

    thresh_opt = G_define_option();
    thresh_opt->key = "threshold";
    thresh_opt->type = TYPE_DOUBLE;
    thresh_opt->required = NO;
    thresh_opt->answer = "1000";
    thresh_opt->description = _("Maximum distance to nearest line");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    LCats = Vect_new_cats_struct();
    PCats = Vect_new_cats_struct();
    LPoints = Vect_new_line_struct();
    PPoints = Vect_new_line_struct();


    lfield = atoi(lfield_opt->answer);
    pfield = atoi(pfield_opt->answer);
    multip = 1000;		/* Number of map units per MP unit */
    thresh = atof(thresh_opt->answer);

    /* Open input lines */
    mapset = G_find_vector2(lines_opt->answer, NULL);
    if (mapset == NULL)
	G_fatal_error(_("Vector map <%s> not found"), lines_opt->answer);

    Vect_set_open_level(2);
    if (Vect_open_old(&LMap, lines_opt->answer, mapset) < 0)
	G_fatal_error(_("Unable to open vector map <%s>"), lines_opt->answer);

    /* Open input points */
    mapset = G_find_vector2(points_opt->answer, NULL);
    if (mapset == NULL)
	G_fatal_error(_("Vector map <%s> not found"), points_opt->answer);

    Vect_set_open_level(2);
    if (Vect_open_old(&PMap, points_opt->answer, mapset) < 0)
	G_fatal_error(_("Unable to open vector map <%s>"), points_opt->answer);

    db_init_handle(&rshandle);
    db_init_string(&rsstmt);
    rsdriver = db_start_driver(driver_opt->answer);
    db_set_handle(&rshandle, database_opt->answer, NULL);
    if (db_open_database(rsdriver, &rshandle) != DB_OK)
	G_fatal_error(_("Unable to open database for reference table"));

    n_points = n_outside = n_found = n_no_record = n_many_records = 0;

    nlines = Vect_get_num_lines(&PMap);
    G_debug(2, "nlines = %d", nlines);
    G_message("pcat|lid|mpost|offset");
    for (line = 1; line <= nlines; line++) {
	int nearest, pcat, lcat, lid, ret;
	double along, mpost, offset;

	G_debug(3, "point = %d", line);
	type = Vect_read_line(&PMap, PPoints, PCats, line);
	if (type != GV_POINT)
	    continue;
	Vect_cat_get(PCats, pfield, &pcat);
	if (pcat < 0)
	    continue;
	n_points++;

	nearest =
	    Vect_find_line(&LMap, PPoints->x[0], PPoints->y[0], 0.0, GV_LINE,
			   thresh, 0, 0);

	fprintf(stdout, "%d", pcat);

	if (nearest <= 0) {
	    fprintf(stdout, "|-|-  # outside threshold\n");
	    n_outside++;
	    continue;
	}

	/* Read nearest line */
	Vect_read_line(&LMap, LPoints, LCats, nearest);
	Vect_cat_get(LCats, lfield, &lcat);

	Vect_line_distance(LPoints, PPoints->x[0], PPoints->y[0], 0.0, 0,
			   NULL, NULL, NULL, NULL, NULL, &along);

	G_debug(3, "  nearest = %d lcat = %d along = %f", nearest, lcat,
		along);

	if (lcat >= 0) {
	    ret = LR_get_milepost(rsdriver, table_opt->answer, "lcat", "lid",
				  "start_map", "end_map", "start_mp",
				  "start_off", "end_mp", "end_off", lcat,
				  along, multip, &lid, &mpost, &offset);
	}
	else {
	    ret = 0;
	}

	if (ret == 0) {
	    n_no_record++;
	    fprintf(stdout, "|-|-  # no record\n");
	    continue;
	}
	if (ret == 2) {
	    n_many_records++;
	    fprintf(stdout, "|-|-  # too many records\n");
	    continue;
	}

	G_debug(3, "  lid = %d mpost = %f offset = %f", lid, mpost, offset);

	fprintf(stdout, "|%d|%f+%f\n", lid, mpost, offset);
	n_found++;
    }

    db_close_database(rsdriver);

    /* Free, close ... */
    Vect_close(&LMap);
    Vect_close(&PMap);

    G_message(n_("[%d] point read from input",
                 "[%d] points read from input",
                 n_points), n_points);
    G_message(n_("[%d] position found",
                 "[%d] positions found",
                 n_found), n_found);
    if (n_outside)
	G_message(n_("[%d] point outside threshold",
                     "[%d] points outside threshold",
                     n_outside), n_outside);
    if (n_no_record)
	G_message(n_("[%d] point - no record found",
                     "[%d] points - no record found",
                     n_no_record), n_no_record);
    if (n_many_records)
	G_message(n_("[%d] point - too many records found",
                     "[%d] points - too many records found",
                     n_many_records), n_many_records);

    exit(EXIT_SUCCESS);
}

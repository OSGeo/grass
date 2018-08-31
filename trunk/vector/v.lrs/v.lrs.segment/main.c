/*
 *  v.lrs.segment - Generate segments or points from input map for existing 
 *                 linear reference system
 */

 /******************************************************************************
 * Copyright (c) 2004-2007, Radim Blazek (blazek@itc.it),
 *			    Hamish Bowman (side_offset bits)
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
#include <math.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>
#include "../lib/lrs.h"

int find_line(struct Map_info *Map, int lfield, int cat);
void offset_pt_90(double *, double *, double, double);


int main(int argc, char **argv)
{
    FILE *in_file;
    int ret, points_written, lines_written, points_read, lines_read;
    int lfield;
    int line;
    int id, lid, lcat1, lcat2;
    double mpost, offset, mpost2, offset2, map_offset1, map_offset2, multip,
	side_offset;
    double x, y, z, angle, len;
    char stype;
    struct Option *in_opt, *out_opt, *driver_opt, *database_opt;
    struct Option *lfield_opt, *file_opt;
    struct Option *table_opt;
    struct GModule *module;
    const char *mapset;
    char buf[2000];
    struct Map_info In, Out;
    struct line_cats *LCats, *SCats;
    struct line_pnts *LPoints, *SPoints, *PlPoints;
    dbDriver *rsdriver;
    dbHandle rshandle;
    dbString rsstmt;
    char *tmpstr1;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("linear reference system"));
    G_add_keyword(_("network"));
    module->description =
	_("Creates points/segments from input lines, linear reference "
	  "system and positions read from stdin or a file.");

    in_opt = G_define_standard_option(G_OPT_V_INPUT);
    in_opt->description = _("Input vector map containing lines");

    out_opt = G_define_standard_option(G_OPT_V_OUTPUT);
    out_opt->description =
	_("Output vector map where segments will be written");

    lfield_opt = G_define_standard_option(G_OPT_V_FIELD);
    lfield_opt->key = "llayer";
    lfield_opt->answer = "1";
    lfield_opt->description = _("Line layer");

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

    file_opt = G_define_standard_option(G_OPT_F_INPUT);
    file_opt->key = "file";
    file_opt->required = NO;
    file_opt->description = _("Name of file containing segment rules. "
			      "If not given, read from stdin.");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);


    LCats = Vect_new_cats_struct();
    SCats = Vect_new_cats_struct();
    LPoints = Vect_new_line_struct();
    SPoints = Vect_new_line_struct();
    PlPoints = Vect_new_line_struct();

    lfield = atoi(lfield_opt->answer);
    multip = 1000;		/* Number of map units per MP unit */

    if (file_opt->answer) {
	/* open input file */
	if ((in_file = fopen(file_opt->answer, "r")) == NULL)
	    G_fatal_error(_("Unable to open input file <%s>"),
			  file_opt->answer);
    }

    /* Open input lines */
    mapset = G_find_vector2(in_opt->answer, NULL);
    if (mapset == NULL)
	G_fatal_error(_("Vector map <%s> not found"), in_opt->answer);

    Vect_set_open_level(2);
    if (Vect_open_old(&In, in_opt->answer, mapset) < 0)
	G_fatal_error(_("Unable to open vector map <%s>"), in_opt->answer);

    /* Open output segments */
    if (Vect_open_new(&Out, out_opt->answer, Vect_is_3d(&In)) < 0)
	G_fatal_error(_("Unable to create vector map <%s>"), out_opt->answer);

    db_init_handle(&rshandle);
    db_init_string(&rsstmt);
    rsdriver = db_start_driver(driver_opt->answer);
    db_set_handle(&rshandle, database_opt->answer, NULL);
    if (db_open_database(rsdriver, &rshandle) != DB_OK)
	G_fatal_error(_("Unable to open database for reference table"));
    db_set_error_handler_driver(rsdriver);

    points_read = 0;
    lines_read = 0;
    points_written = 0;
    lines_written = 0;

    while (1) {

	if (!file_opt->answer) {
	    if (fgets(buf, sizeof(buf), stdin) == NULL)
		break;
	}
	else {
	    if (G_getl2(buf, sizeof(buf) - 1, in_file) == 0)
		break;
	}

	G_debug(2, "SEGMENT: %s", G_chop(buf));
	side_offset = 0;
	Vect_reset_line(SPoints);
	Vect_reset_cats(SCats);
	switch (buf[0]) {
	case 'P':
	    side_offset = 0;
	    ret = sscanf(buf, "%c %d %d %lf+%lf %lf", &stype, &id,
			 &lid, &mpost, &offset, &side_offset);
	    if (ret < 5) {
		G_warning(_("Cannot read input: %s"), buf);
		break;
	    }
	    points_read++;
	    G_debug(2, "point: %d %d %f+%f %f", id, lid, mpost, offset,
		    side_offset);

	    ret = LR_get_offset(rsdriver, table_opt->answer, "lcat", "lid",
				"start_map", "end_map", "start_mp",
				"start_off", "end_mp", "end_off", lid, mpost,
				offset, multip, &lcat1, &map_offset1);
	    if (ret == 0) {
		G_warning(_("No record in LR table for: %s"), buf);
		break;
	    }
	    if (ret == 3) {
		G_warning(_("More than one record in LR table for: %s"), buf);
		break;
	    }

	    /* OK, write point */
	    line = find_line(&In, lfield, lcat1);
	    if (line == 0) {
		G_warning(_("Unable to find line of cat [%d]"), lcat1);
		break;
	    }

	    Vect_read_line(&In, LPoints, LCats, line);
	    ret =
		Vect_point_on_line(LPoints, map_offset1, &x, &y, &z, &angle,
				   NULL);
	    if (ret == 0) {
		len = Vect_line_length(LPoints);
		G_warning(_("Cannot get point on line: cat = [%d] "
			    "distance = [%f] (line length = %f)\n%s"),
			  lcat1, map_offset1, len, buf);
		break;
	    }

	    if (fabs(side_offset) > 0.0)
		offset_pt_90(&x, &y, angle, side_offset);

	    Vect_append_point(SPoints, x, y, z);
	    Vect_cat_set(SCats, 1, id);

	    Vect_write_line(&Out, GV_POINT, SPoints, SCats);
	    points_written++;
	    break;
	case 'L':
	    side_offset = 0;
	    ret = sscanf(buf, "%c %d %d %lf+%lf %lf+%lf %lf", &stype, &id,
			 &lid, &mpost, &offset, &mpost2, &offset2,
			 &side_offset);
	    if (ret < 7) {
		G_warning(_("Cannot read input: %s"), buf);
		break;
	    }
	    lines_read++;
	    G_debug(2, "line: %d %d %f+%f %f+%f %f", id, lid, mpost, offset,
		    mpost2, offset2, side_offset);
	    /* Find both points */
	    /* Nearest up */
	    ret =
		LR_get_nearest_offset(rsdriver, table_opt->answer, "lcat",
				      "lid", "start_map", "end_map",
				      "start_mp", "start_off", "end_mp",
				      "end_off", lid, mpost, offset, multip,
				      0, &lcat1, &map_offset1);
	    if (ret == 0) {
		G_warning(_("No record in LRS table for 1. point of:\n  %s"),
			  buf);
		break;
	    }
	    if (ret == 3) {
		G_warning(_("Using last from more offsets found for 1. "
			    "point of:\n  %s"), buf);
	    }
	    if (ret == 2) {
		G_warning(_("Requested offset for the 1. point not found, "
			    "using nearest found:\n  %s"), buf);
	    }
	    /* Nearest down */
	    ret =
		LR_get_nearest_offset(rsdriver, table_opt->answer, "lcat",
				      "lid", "start_map", "end_map",
				      "start_mp", "start_off", "end_mp",
				      "end_off", lid, mpost2, offset2, multip,
				      1, &lcat2, &map_offset2);
	    if (ret == 0) {
		G_warning(_("No record in LRS table for 2. point of:\n  %s"),
			  buf);
		break;
	    }
	    if (ret == 2) {
		G_warning(_("Requested offset for the 2. point not found, "
			    "using nearest found:\n  %s"), buf);
	    }
	    if (ret == 3) {
		G_warning(_("Using first from more offsets found for 2. "
			    "point of:\n  %s"), buf);
	    }
	    /* Check if both points are at the same line */
	    if (lcat1 != lcat2) {
		G_warning(_("Segment over 2 (or more) segments, not yet supported"));
		break;
	    }
	    G_debug(2, "segment: lcat = %d : %f -  %f", lcat1, map_offset1,
		    map_offset2);
	    line = find_line(&In, lfield, lcat1);
	    if (line == 0) {
		G_warning(_("Unable to find line of cat [%d]"), lcat1);
		break;
	    }

	    Vect_read_line(&In, LPoints, LCats, line);

	    len = Vect_line_length(LPoints);
	    if (map_offset2 > len) {
		/* This is mostly caused by calculation only -> use a threshold for warning */
		if (fabs(map_offset2 - len) > 1e-6) {	/* usually around 1e-7 */
		    G_warning(_("End of segment > line length (%e) -> cut"),
			      fabs(map_offset2 - len));
		}
		map_offset2 = len;
	    }

	    ret =
		Vect_line_segment(LPoints, map_offset1, map_offset2, SPoints);
	    if (ret == 0) {
		G_warning(_("Cannot make line segment: cat = %d : "
			    "%f - %f (line length = %f)\n%s"),
			  lcat1, map_offset1, map_offset2, len, buf);
		break;
	    }

	    Vect_cat_set(SCats, 1, id);

	    if (fabs(side_offset) > 0.0) {
	        Vect_line_parallel2(SPoints, side_offset, side_offset,
		   		   0.0, 1, FALSE, side_offset / 10.,
		   	           PlPoints);
		Vect_write_line(&Out, GV_LINE, PlPoints, SCats);
		G_debug(3, "  segment n_points = %d", PlPoints->n_points);
	    }
	    else {
		Vect_write_line(&Out, GV_LINE, SPoints, SCats);
		G_debug(3, "  segment n_points = %d", SPoints->n_points);
	    }

	    lines_written++;
	    G_debug(3, "  -> written.");
	    break;
	default:
	    G_warning(_("Incorrect segment type: %s"), buf);
	}

    }

    db_close_database(rsdriver);

    Vect_copy_head_data(&In, &Out);
    Vect_hist_copy(&In, &Out);
    Vect_hist_command(&Out);

    Vect_build(&Out);

    /* Free, close ... */
    Vect_close(&In);
    Vect_close(&Out);

    if (file_opt->answer)
	fclose(in_file);

    G_message(n_("[%d] point read from input",
                 "[%d] points read from input",
                 points_read), points_read);
    G_asprintf(&tmpstr1, n_("%d lost", "%d lost", 
                            points_read - points_written), 
               points_read - points_written);
    /* GTC %s will be replaced with a message about lost points. */
    G_message(n_("[%d] point written to output map (%s)",
                 "[%d] points written to output map (%s)",
                 points_written),
	      points_written, tmpstr1);
    G_free(tmpstr1);
    G_message(n_("[%d] line read from input",
                 "[%d] lines read from input",
                 lines_read), lines_read);
    G_asprintf(&tmpstr1, n_("%d lost", "%d lost",
                            lines_read - lines_written),
               lines_read - lines_written);
    /* GTC %s will be replaced with a message about lost lines. */
    G_message(n_("[%d] line written to output map (%s)",
                 "[%d] lines written to output map (%s)",
                 lines_written),
	      lines_written, tmpstr1);
    G_free(tmpstr1);

    exit(EXIT_SUCCESS);
}


/* Find line by cat, returns 0 if not found */
/* TODO: use category index */
int find_line(struct Map_info *Map, int lfield, int lcat)
{
    int i, nlines, type, cat;
    struct line_cats *Cats;

    G_debug(2, "find_line(): lfield = %d lcat = %d", lfield, lcat);
    Cats = Vect_new_cats_struct();

    nlines = Vect_get_num_lines(Map);
    for (i = 1; i <= nlines; i++) {
	type = Vect_read_line(Map, NULL, Cats, i);
	if (!(type & GV_LINE))
	    continue;
	Vect_cat_get(Cats, lfield, &cat);
	if (cat == lcat)
	    return i;
    }

    return 0;
}


/* calculate a point perpendicular to the current line angle, offset by a distance
 * works in the x,y plane.
 */
void offset_pt_90(double *x, double *y, double angle, double distance)
{
    *x -= distance * cos(M_PI_2 + angle);
    *y -= distance * sin(M_PI_2 + angle);
}

/*
 *  v.lrs.stationing - Generate stationing as new vector map and labels from
 *                     input map for existing linear reference system
 *
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
#include <math.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>
#include "../lib/lrs.h"

# define PI    3.1415926535897932384626433832795029L

typedef struct
{
    int lid;
    double start_map, end_map;
    double start_mp, start_off;	/* milepost, offset for the beginning of ref. segment */
    double end_mp, end_off;	/* milepost, offset for the end of ref. segment */
} RSEGMENT;

int cmp_along(const void *pa, const void *pb);

int main(int argc, char **argv)
{
    int i, ret, nlines, more;
    int cat, lfield;
    int line, type;
    int lcat, mp, sta;
    double start, end, station;
    double mp_multip, sta_multip, map_offset;
    double x, y, z, angle, xs, ys, rotate;
    double sta_l_off, sta_r_off, mp_l_off, mp_r_off;	/* Left and right offset of stationing */
    double l_off, r_off;
    double lab_x_off, lab_y_off, lab_x, lab_y;
    int arseg, nrseg, seg;
    RSEGMENT *rseg;
    struct Option *in_opt, *out_opt, *labels_opt;
    struct Option *lfield_opt;
    struct Option *driver_opt, *database_opt, *table_opt, *offset_opt;
    struct Option *Xoffset;
    struct Option *Yoffset;
    struct Option *Reference;
    struct Option *Font;
    struct Option *Color;
    struct Option *Size;
    struct Option *Width;
    struct Option *Hcolor;
    struct Option *Hwidth;
    struct Option *Bcolor;
    struct Option *Border;
    struct Option *Opaque;

    struct GModule *module;
    const char *mapset;
    char buf[2000];
    struct Map_info In, Out;
    struct line_cats *LCats, *SCats;
    struct line_pnts *LPoints, *SPoints;
    dbDriver *rsdriver;
    dbHandle rshandle;
    dbString stmt;
    dbCursor cursor;
    dbTable *table;
    dbColumn *column;
    dbValue *value;
    FILE *labels;

    double t1, t2;
    double dt;
    int nstat;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("Linear Reference System"));
    G_add_keyword(_("networking"));
    module->description = _("Creates stationing from input lines, "
			    "and linear reference system.");

    in_opt = G_define_standard_option(G_OPT_V_INPUT);
    in_opt->description = _("Input vector map containing lines");

    out_opt = G_define_standard_option(G_OPT_V_OUTPUT);
    out_opt->description =
	_("Output vector map where stationing will be written");

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

    labels_opt = G_define_option();
    labels_opt->key = "labels";
    labels_opt->type = TYPE_STRING;
    labels_opt->required = NO;
    labels_opt->multiple = NO;
    labels_opt->gisprompt = "new,paint/labels,Labels";
    labels_opt->description = _("Label file");

    offset_opt = G_define_option();
    offset_opt->key = "offset";
    offset_opt->type = TYPE_STRING;
    offset_opt->required = NO;
    offset_opt->multiple = YES;
    offset_opt->answer = "50,100,25,25";
    offset_opt->description =
	_("PM left, MP right, stationing left, stationing right offset");

    Xoffset = G_define_option();
    Xoffset->key = "xoffset";
    Xoffset->description =
	_("Offset label in label x-direction in map units");
    Xoffset->type = TYPE_DOUBLE;
    Xoffset->answer = "25";

    Yoffset = G_define_option();
    Yoffset->key = "yoffset";
    Yoffset->description =
	_("Offset label in label y-direction in map units");
    Yoffset->type = TYPE_DOUBLE;
    Yoffset->answer = "5";

    Reference = G_define_option();
    Reference->key = "reference";
    Reference->description = _("Reference position");
    Reference->type = TYPE_STRING;
    Reference->answer = "center";
    Reference->options = "center,left,right,upper,lower";

    Font = G_define_option();
    Font->key = "font";
    Font->description = _("Font");
    Font->type = TYPE_STRING;
    Font->answer = "standard";

    Size = G_define_option();
    Size->key = "size";
    Size->description = _("Label size (in map-units)");
    Size->type = TYPE_INTEGER;
    Size->answer = "100";
    Size->options = "1-1000";

    Color = G_define_option();
    Color->key = "color";
    Color->description = _("Text color");
    Color->type = TYPE_STRING;
    Color->answer = "black";
    Color->options =
	"aqua,black,blue,brown,cyan,gray,green,grey,indigo,magenta,"
	"orange,purple,red,violet,white,yellow";

    Width = G_define_option();
    Width->key = "width";
    Width->label = _("Line width of text");
    Width->description = _("Only for d.label output");
    Width->type = TYPE_INTEGER;
    Width->answer = "1";
    Width->options = "1-100";

    Hcolor = G_define_option();
    Hcolor->key = "hcolor";
    Hcolor->label = _("Highlight color for text");
    Hcolor->description = _("Only for d.label output");
    Hcolor->type = TYPE_STRING;
    Hcolor->answer = "none";
    Hcolor->options =
	"none,aqua,black,blue,brown,cyan,gray,green,grey,indigo,magenta,"
	"orange,purple,red,violet,white,yellow";

    Hwidth = G_define_option();
    Hwidth->key = "hwidth";
    Hwidth->label = _("Line width of highlight color");
    Hwidth->description = _("Only for d.label output");
    Hwidth->type = TYPE_INTEGER;
    Hwidth->answer = "0";
    Hwidth->options = "0-100";

    Bcolor = G_define_option();
    Bcolor->key = "background";
    Bcolor->description = _("Background color");
    Bcolor->type = TYPE_STRING;
    Bcolor->answer = "none";
    Bcolor->options =
	"none,aqua,black,blue,brown,cyan,gray,green,grey,indigo,magenta,"
	"orange,purple,red,violet,white,yellow";

    Border = G_define_option();
    Border->key = "border";
    Border->description = _("Border color");
    Border->type = TYPE_STRING;
    Border->answer = "none";
    Border->options =
	"none,aqua,black,blue,brown,cyan,gray,green,grey,indigo,magenta,"
	"orange,purple,red,violet,white,yellow";

    Opaque = G_define_option();
    Opaque->key = "opaque";
    Opaque->label = _("Opaque to vector");
    Opaque->description = _("Only relevant if background color is selected");
    Opaque->type = TYPE_STRING;
    Opaque->answer = "yes";
    Opaque->options = "yes,no";

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    LCats = Vect_new_cats_struct();
    SCats = Vect_new_cats_struct();
    LPoints = Vect_new_line_struct();
    SPoints = Vect_new_line_struct();

    lfield = atoi(lfield_opt->answer);
    lab_x_off = atof(Xoffset->answer);
    lab_y_off = atof(Yoffset->answer);
    mp_multip = 1000;		/* Number of map units per MP unit */
    sta_multip = 100;		/* Number of map units per stationing unit */

    i = 0;
    mp_l_off = 50;
    mp_r_off = 100;
    sta_l_off = 25;
    sta_r_off = 25;
    while (offset_opt->answers[i]) {
	if (i == 0)
	    mp_l_off = atoi(offset_opt->answers[i]);
	else if (i == 1)
	    mp_r_off = atoi(offset_opt->answers[i]);
	else if (i == 2)
	    sta_l_off = atoi(offset_opt->answers[i]);
	else if (i == 3)
	    sta_r_off = atoi(offset_opt->answers[i]);
	i++;
    }

    /* Open input lines */
    mapset = G_find_vector2(in_opt->answer, NULL);
    if (mapset == NULL)
	G_fatal_error(_("Vector map <%s> not found"), in_opt->answer);

    Vect_set_open_level(2);
    Vect_open_old(&In, in_opt->answer, mapset);

    /* Open output segments */
    Vect_open_new(&Out, out_opt->answer, Vect_is_3d(&In));

    /* open labels */
    labels = NULL;
    if (labels_opt->answer) {
	labels = G_fopen_new("paint/labels", labels_opt->answer);
	if (labels == NULL)
	    G_fatal_error(_("Unable to open label file <%s>"),
			  labels_opt->answer);
    }

    db_init_handle(&rshandle);
    db_init_string(&stmt);
    rsdriver = db_start_driver(driver_opt->answer);
    db_set_handle(&rshandle, database_opt->answer, NULL);
    if (db_open_database(rsdriver, &rshandle) != DB_OK)
	G_fatal_error(_("Unable to open database for reference table"));

    /* For each line select all existeng reference segments, sort them along the line
     *  and fcreate stationing. */

    G_debug(2, "find_line(): lfield = %d lcat = %d", lfield, lcat);

    arseg = 1000;
    rseg = (RSEGMENT *) G_malloc(arseg * sizeof(RSEGMENT));

    nlines = Vect_get_num_lines(&In);
    /* for ( line = 19; line <= 19; line++ ) { */
    for (line = 1; line <= nlines; line++) {
	G_debug(3, "  line = %d / %d", line, nlines);
	type = Vect_read_line(&In, LPoints, LCats, line);
	if (!(type & GV_LINE))
	    continue;
	Vect_cat_get(LCats, lfield, &cat);
	if (cat < 0)
	    continue;

	sprintf(buf,
		"select start_map, end_map, start_mp, start_off, end_mp, end_off, lid "
		"from %s where lcat = %d;", table_opt->answer, cat);
	G_debug(2, "  SQL: %s", buf);
	db_append_string(&stmt, buf);

	G_debug(1, "    select");
	if (db_open_select_cursor(rsdriver, &stmt, &cursor, DB_SEQUENTIAL) !=
	    DB_OK)
	    G_fatal_error(_("Unable to select records from LRS table: %s"),
			  buf);

	table = db_get_cursor_table(&cursor);

	G_debug(1, "    fetch");
	nrseg = 0;
	while (1) {
	    if (db_fetch(&cursor, DB_NEXT, &more) != DB_OK)
		G_fatal_error(_("Unable to fetch data from table"));

	    if (!more)
		break;

	    if (nrseg == arseg) {
		arseg += 1000;
		rseg = (RSEGMENT *) G_realloc(rseg, arseg * sizeof(RSEGMENT));
	    }

	    column = db_get_table_column(table, 0);
	    value = db_get_column_value(column);
	    rseg[nrseg].start_map = db_get_value_double(value);

	    column = db_get_table_column(table, 1);
	    value = db_get_column_value(column);
	    rseg[nrseg].end_map = db_get_value_double(value);

	    column = db_get_table_column(table, 2);
	    value = db_get_column_value(column);
	    rseg[nrseg].start_mp = db_get_value_double(value);

	    column = db_get_table_column(table, 3);
	    value = db_get_column_value(column);
	    rseg[nrseg].start_off = db_get_value_double(value);

	    column = db_get_table_column(table, 4);
	    value = db_get_column_value(column);
	    rseg[nrseg].end_mp = db_get_value_double(value);

	    column = db_get_table_column(table, 5);
	    value = db_get_column_value(column);
	    rseg[nrseg].end_off = db_get_value_double(value);

	    column = db_get_table_column(table, 6);
	    value = db_get_column_value(column);
	    rseg[nrseg].lid = db_get_value_int(value);

	    G_debug(2, "RS: %f - %f => %f+%f - %f+%f", rseg[nrseg].start_map,
		    rseg[nrseg].end_map, rseg[nrseg].start_mp,
		    rseg[nrseg].start_off, rseg[nrseg].end_mp,
		    rseg[nrseg].end_off);

	    nrseg++;
	}

	G_debug(3, "    %d reference segments selected", nrseg);
	if (nrseg == 0)
	    continue;

	/* Sort along the line */
	qsort((void *)rseg, nrseg, sizeof(RSEGMENT), cmp_along);

	/* Go through all segments of current line */
	G_debug(1, "    write");
	/* t1 = clock(); */
	t1 = (double)time(NULL);
	nstat = 0;
	for (seg = 0; seg < nrseg; seg++) {
	    nstat++;
	    start = mp_multip * rseg[seg].start_mp + rseg[seg].start_off;
	    end = mp_multip * rseg[seg].end_mp + rseg[seg].end_off;
	    mp = start / mp_multip;
	    sta = (start - mp * mp_multip) / sta_multip;
	    station = mp * mp_multip + sta * sta_multip;
	    G_debug(1, "      seg = %d length = %f", seg, end - start);

	    while (station < end) {
		G_debug(2, "mp = %d sta = %d station = %f", mp, sta, station);

		G_debug(1, "      get offset");
		ret =
		    LR_get_offset(rsdriver, table_opt->answer, "lcat", "lid",
				  "start_map", "end_map", "start_mp",
				  "start_off", "end_mp", "end_off",
				  rseg[seg].lid, mp, sta * sta_multip,
				  mp_multip, &lcat, &map_offset);
		/* G_debug(1, "      get offset time = %d", t2 - t1); */
		G_debug(1, "      get offset");

		if (ret == 0) {
		    G_warning(_("No record in LR table"));
		    break;
		}
		if (ret == -1) {
		    G_warning(_("More than one record in LR table"));
		    break;
		}
		G_debug(2, "map_offset = %f", map_offset);

		Vect_reset_cats(SCats);
		Vect_cat_set(SCats, 2, cat);

		/* Decide if MP or common stationing */
		if (sta == 0) {	/* MP */
		    l_off = mp_l_off;
		    r_off = mp_r_off;
		    Vect_cat_set(SCats, 1, 1);
		}
		else {
		    l_off = sta_l_off;
		    r_off = sta_r_off;
		    Vect_cat_set(SCats, 1, 2);
		}

		Vect_point_on_line(LPoints, map_offset, &x, &y, &z, &angle,
				   NULL);

		Vect_reset_line(SPoints);
		Vect_append_point(SPoints, x, y, 0);
		Vect_write_line(&Out, GV_POINT, SPoints, SCats);

		Vect_reset_line(SPoints);
		xs = x + l_off * cos(angle + PI / 2);
		ys = y + l_off * sin(angle + PI / 2);
		Vect_append_point(SPoints, xs, ys, 0);
		xs = x + r_off * cos(angle - PI / 2);
		ys = y + r_off * sin(angle - PI / 2);
		Vect_append_point(SPoints, xs, ys, 0);

		Vect_write_line(&Out, GV_LINE, SPoints, SCats);

		/* Create label */
		lab_x = x + lab_x_off * cos(angle - PI / 2);
		lab_y = y + lab_x_off * sin(angle - PI / 2);
		lab_x = lab_x + lab_y_off * cos(angle);
		lab_y = lab_y + lab_y_off * sin(angle);
		if (sta == 0 && labels != NULL) {
		    rotate = 360 * angle / 2 / PI - 90;

		    fprintf(labels, "east: %f\n", lab_x);
		    fprintf(labels, "north: %f\n", lab_y);
		    fprintf(labels, "xoffset: 0\n");
		    fprintf(labels, "yoffset: 0\n");
		    fprintf(labels, "ref: %s\n", Reference->answer);
		    fprintf(labels, "font: %s\n", Font->answer);
		    fprintf(labels, "color: %s\n", Color->answer);
		    fprintf(labels, "size: %s\n", Size->answer);
		    fprintf(labels, "width: %s\n", Width->answer);
		    fprintf(labels, "hcolor: %s\n", Hcolor->answer);
		    fprintf(labels, "hwidth: %s\n", Hwidth->answer);
		    fprintf(labels, "background: %s\n", Bcolor->answer);
		    fprintf(labels, "border: %s\n", Border->answer);
		    fprintf(labels, "opaque: %s\n", Opaque->answer);
		    fprintf(labels, "rotate: %f\n\n", rotate);
		    fprintf(labels, "text: %d+%.0f\n\n", mp, sta * mp_multip);
		}

		sta++;
		if (sta == (mp_multip / sta_multip)) {
		    mp++;
		    sta = 0;
		}
		station = mp * mp_multip + sta * sta_multip;

	    }

	}
	/* t2 = clock(); */
	t2 = (double)time(NULL);
	dt = (t2 - t1) / nstat;
	G_debug(1, "    time / seg = %f, time = %f, nstat = %d", dt, t2 - t1,
		nstat);
	/* break; */
	/* debug */

    }

    db_close_database(rsdriver);
    Vect_build(&Out);

    /* Free, close ... */
    Vect_close(&In);
    Vect_close(&Out);

    exit(EXIT_SUCCESS);
}

int cmp_along(const void *pa, const void *pb)
{
    RSEGMENT *p1 = (RSEGMENT *) pa;
    RSEGMENT *p2 = (RSEGMENT *) pb;

    if (p1->start_map < p2->start_map)
	return -1;
    if (p1->start_map > p2->start_map)
	return 1;
    return 0;
}

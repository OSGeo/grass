/*
 *  v.lrs.create - Create Linear reference system
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

/* MP is milepost */

#define TO_TYPE_FROM 1		/* the same as specified fo from_ */
#define TO_TYPE_MAP  2		/* calculated from map along the line from previous MP */
#define TO_TYPE_USER 3		/* defined by user */

#define DIR_FORWARD   1
#define DIR_BACKWARD  2
#define DIR_UNKNOWN   3

#define ERR_OK           0	/* No error */
#define ERR_END_GT_START 1	/* MP: end > start */
#define ERR_THRESHOLD    2	/* MP is outside threshold */
#define ERR_IDENT        3	/* more MPs with identical distance along the line */
#define ERR_ORDER        4	/* MP in wrong order (used for points) */
#define ERR_NO_POINT     5	/* No MP point found for MP DB record */
#define ERR_NO_MP        6	/* Line without MP */
#define ERR_ONE_MP       7	/* Line with 1 MP only */
#define ERR_NO_DIR       8	/* Unknown direction of line */
#define ERR_LINE_ORDER   9	/* Wrong order of MP along line (used for lines) */

typedef struct
{
    double x, y;
    int cat;
    int line_idx;		/* index to line in 'lines' array */
    double dist_along;		/* distance from the beginning of the line */
    double start_mp, start_off;	/* milepost, offset for the beginning of ref. segment */
    double end_mp, end_off;	/* milepost, offset for the end of ref. segment */
    int to_type;		/* type of the end_mp, end_off */
    int err;			/* error number */
} MILEPOST;

typedef struct
{
    int line, cat;		/* line number in 'In' vector and category of nearest line */
    int nmposts;		/* number of attached MPs */
    int first_mpost_idx;	/* index of first/last MP in 'mposts' */
    int direction;		/* direction DIR_FORWARD/DIR_BACKWARD if MPs have increasing/decreasing */
    /* values along the line */
    double length;		/* line length */
    int err;			/* error number */
} RLINE;

int cmp_along(const void *pa, const void *pb);

int main(int argc, char **argv)
{
    int i, j, k, lid, more, found, ret, totype, rsid, dir, forward, backward,
	order;
    int *Lid_int, nLid, aLid;
    int cat, *cats, ncat, lfield, pfield;
    int line, first, last, point, type;
    int nrlines, nmposts, nallmposts, mpost;
    MILEPOST *mposts /* table of currently referenced MPs */ ;
    RLINE *rlines;		/* table of currently referenced lines */
    double thresh, dist_to, dist_along, distance_to;
    double end_mp, end_off;
    double multip = 1000;	/* Number of map units per MP unit */
    struct Option *in_lines_opt, *out_lines_opt, *points_opt, *err_opt;
    struct Option *lfield_opt, *pfield_opt;
    struct Option *lidcol_opt, *pidcol_opt;
    struct Option *start_mp_opt, *start_off_opt, *end_mp_opt, *end_off_opt;
    struct Option *driver_opt, *database_opt, *table_opt, *thresh_opt;
    struct GModule *module;
    const char *mapset;
    char buf[2000];
    struct Map_info In, Out, PMap, EMap;
    struct line_cats *LCats, *PCats;
    struct line_pnts *LPoints, *L2Points, *PPoints;
    struct field_info *Lfi, *Pfi;
    dbDriver *ldriver, *pdriver, *rsdriver;
    dbHandle lhandle, phandle, rshandle;
    dbString lstmt, pstmt, rsstmt;
    dbCursor lcursor, pcursor;
    dbTable *ltable, *ptable;
    dbColumn *lcolumn, *pcolumn;
    dbValue *lvalue, *pvalue;
    int lcoltype, lccoltype;
    int debug = 2;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("linear reference system"));
    G_add_keyword(_("network"));
    module->description = _("Creates a linear reference system.");

    in_lines_opt = G_define_standard_option(G_OPT_V_INPUT);
    in_lines_opt->key = "in_lines";
    in_lines_opt->description = _("Input vector map containing lines");

    out_lines_opt = G_define_standard_option(G_OPT_V_OUTPUT);
    out_lines_opt->key = "out_lines";
    out_lines_opt->description =
	_("Output vector map where oriented lines are written");

    err_opt = G_define_standard_option(G_OPT_V_OUTPUT);
    err_opt->key = "err";
    err_opt->required = NO;
    err_opt->description = _("Output vector map of errors");

    points_opt = G_define_standard_option(G_OPT_V_INPUT);
    points_opt->key = "points";
    points_opt->description =
	_("Input vector map containing reference points");

    lfield_opt = G_define_standard_option(G_OPT_V_FIELD);
    lfield_opt->key = "llayer";
    lfield_opt->description = _("Line layer");

    pfield_opt = G_define_standard_option(G_OPT_V_FIELD);
    pfield_opt->key = "player";
    pfield_opt->description = _("Point layer");

    lidcol_opt = G_define_option();
    lidcol_opt->key = "lidcol";
    lidcol_opt->type = TYPE_STRING;
    lidcol_opt->required = YES;
    lidcol_opt->description =
	_("Column containing line identifiers for lines");

    pidcol_opt = G_define_option();
    pidcol_opt->key = "pidcol";
    pidcol_opt->type = TYPE_STRING;
    pidcol_opt->required = YES;
    pidcol_opt->description =
	_("Column containing line identifiers for points");

    start_mp_opt = G_define_option();
    start_mp_opt->key = "start_mp";
    start_mp_opt->type = TYPE_STRING;
    start_mp_opt->required = NO;
    start_mp_opt->answer = "start_mp";
    start_mp_opt->description =
	_("Column containing milepost position for the beginning "
	  "of next segment");

    start_off_opt = G_define_option();
    start_off_opt->key = "start_off";
    start_off_opt->type = TYPE_STRING;
    start_off_opt->required = NO;
    start_off_opt->answer = "start_off";
    start_off_opt->description =
	_("Column containing offset from milepost for the beginning "
	  "of next segment");

    end_mp_opt = G_define_option();
    end_mp_opt->key = "end_mp";
    end_mp_opt->type = TYPE_STRING;
    end_mp_opt->required = NO;
    end_mp_opt->answer = "end_mp";
    end_mp_opt->description =
	_("Column containing milepost position for the end "
	  "of previous segment");

    end_off_opt = G_define_option();
    end_off_opt->key = "end_off";
    end_off_opt->type = TYPE_STRING;
    end_off_opt->required = NO;
    end_off_opt->answer = "end_off";
    end_off_opt->description =
	_("Column containing offset from milepost for the end "
	  "of previous segment");

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
    table_opt->label =
	_("Name of table where the reference system will be written");
    table_opt->description = _("New table is created by this module");

    thresh_opt = G_define_option();
    thresh_opt->key = "threshold";
    thresh_opt->type = TYPE_DOUBLE;
    thresh_opt->required = NO;
    thresh_opt->answer = "1";
    thresh_opt->description = _("Maximum distance of point to line allowed");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    LCats = Vect_new_cats_struct();
    PCats = Vect_new_cats_struct();
    LPoints = Vect_new_line_struct();
    L2Points = Vect_new_line_struct();
    PPoints = Vect_new_line_struct();

    lfield = atoi(lfield_opt->answer);
    pfield = atoi(pfield_opt->answer);
    thresh = atof(thresh_opt->answer);

    G_debug(debug, "Creating LRS ...");

    /* Open input lines */
    mapset = G_find_vector2(in_lines_opt->answer, NULL);
    if (mapset == NULL)
	G_fatal_error(_("Vector map <%s> not found"), in_lines_opt->answer);

    if (Vect_open_old(&In, in_lines_opt->answer, mapset) < 0)
	G_fatal_error(_("Unable to open vector map <%s>"),
			in_lines_opt->answer);

    /* Open input ipoints */
    mapset = G_find_vector2(points_opt->answer, NULL);
    if (mapset == NULL)
	G_fatal_error(_("Vector map <%s> not found"), points_opt->answer);

    if (Vect_open_old(&PMap, points_opt->answer, mapset) < 0)
	G_fatal_error(_("Unable to open vector map <%s>"), points_opt->answer);

    /* Open output lines */
    if (Vect_open_new(&Out, out_lines_opt->answer, Vect_is_3d(&In)) < 0)
	G_fatal_error(_("Unable to create vector map <%s>"),
			out_lines_opt->answer);

    /* Open output error map */
    if (err_opt->answer) {
	if (Vect_open_new(&EMap, err_opt->answer, Vect_is_3d(&In)) < 0)
	    G_fatal_error(_("Unable to create vector map <%s>"),
			    err_opt->answer);
    }

    /* Because the line feature identified by one id (lidcol) may be split
     *  to more line parts, and milepost may be in threshold for more such parts,
     *  so that if each line part would be processed separetely, it could be attached
     *  to more parts, it is better to process always whole line feature (all parts)
     *  of one id at the same time, and attache mileposts always to nearest one */

    /* Open the database for lines and points */
    Lfi = Vect_get_field(&In, lfield);
    if (Lfi == NULL)
	G_fatal_error(_("Cannot get layer info for lines"));
    Pfi = Vect_get_field(&PMap, pfield);
    if (Pfi == NULL)
	G_fatal_error(_("Cannot get layer info for points"));

    db_init_handle(&lhandle);
    db_init_string(&lstmt);
    ldriver = db_start_driver(Lfi->driver);
    db_set_handle(&lhandle, Lfi->database, NULL);
    if (db_open_database(ldriver, &lhandle) != DB_OK)
	G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
		      Lfi->database, Lfi->driver);
    db_set_error_handler_driver(ldriver);

    db_init_handle(&phandle);
    db_init_string(&pstmt);
    pdriver = db_start_driver(Pfi->driver);
    db_set_handle(&phandle, Pfi->database, NULL);
    if (db_open_database(pdriver, &phandle) != DB_OK)
	G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
		      Pfi->database, Pfi->driver);
    db_set_error_handler_driver(pdriver);

    /* Open database for RS table */
    db_init_handle(&rshandle);
    db_init_string(&rsstmt);
    rsdriver = db_start_driver(driver_opt->answer);
    db_set_handle(&rshandle, database_opt->answer, NULL);
    if (db_open_database(rsdriver, &rshandle) != DB_OK)
	G_fatal_error(_("Unable to open database for reference table"));
    db_set_error_handler_driver(rsdriver);

    /* Create new reference table */
    /* perhaps drop table to be conditionalized upon --o ? */
    if (db_table_exists(db_get_default_driver_name(),
			db_get_default_database_name(),
			table_opt->answer) == 1) {
	db_init_string(&rsstmt);
	sprintf(buf, "drop table %s", table_opt->answer);
	db_append_string(&rsstmt, buf);
	if (db_execute_immediate(rsdriver, &rsstmt) != DB_OK)
	    G_warning(_("Unable to drop table: %s"), buf);
    }
    db_init_string(&rsstmt);
    sprintf(buf,
	    "create table %s (rsid int, lcat int, lid int, start_map double precision, "
	    "end_map double precision, start_mp double precision, start_off double precision, "
	    "end_mp double precision, end_off double precision, end_type int)",
	    table_opt->answer);
    G_debug(debug, "ref tab SQL: %s", buf);
    db_append_string(&rsstmt, buf);
    if (db_execute_immediate(rsdriver, &rsstmt) != DB_OK)
	G_fatal_error(_("Unable to create table: %s"), buf);

    /* Select all lid from line table */
    sprintf(buf, "select %s from %s", lidcol_opt->answer, Lfi->table);
    G_debug(debug, "line tab lid SQL: %s", buf);
    db_append_string(&lstmt, buf);
    if (db_open_select_cursor(ldriver, &lstmt, &lcursor, DB_SEQUENTIAL) !=
	DB_OK)
	G_fatal_error(_("Unable to select line id values from %s.%s."),
		      Lfi->table, lidcol_opt->answer);

    /* TODO: we expect line id to be integer, extend to string later */
    ltable = db_get_cursor_table(&lcursor);
    lcolumn = db_get_table_column(ltable, 0);	/* first column */
    lcoltype = db_get_column_sqltype(lcolumn);
    lccoltype = db_sqltype_to_Ctype(lcoltype);

    if (lccoltype != DB_C_TYPE_INT)
	G_fatal_error(_("Line id column must be integer"));

    lvalue = db_get_column_value(lcolumn);

    /* Fetch all line id and store it as unique value */
    G_debug(debug, "Fetch all line id");
    nLid = 0;
    aLid = 1;
    Lid_int = (int *)G_malloc(aLid * sizeof(int));
    while (1) {
	if (db_fetch(&lcursor, DB_NEXT, &more) != DB_OK)
	    G_fatal_error(_("Unable to fetch line id from line table"));

	if (!more)
	    break;

	lid = db_get_value_int(lvalue);
	found = 0;
	for (i = 0; i < nLid; i++) {
	    if (Lid_int[i] == lid) {
		G_debug(debug, "lid = %d (duplicate)", lid);
		found = 1;
		break;
	    }
	}
	if (!found) {		/* add new lid */
	    G_debug(debug, "lid = %d (new)", lid);
	    if (nLid == aLid) {
		aLid += 1;
		Lid_int = (int *)G_realloc(Lid_int, aLid * sizeof(int));
	    }
	    Lid_int[nLid] = lid;
	    nLid++;
	}
    }
    db_close_cursor(&lcursor);


    /* Allocate space for lines and points (maybe more than used, but not less ) */
    rlines = (RLINE *) G_malloc(Vect_get_num_lines(&In) * sizeof(RLINE));
    mposts =
	(MILEPOST *) G_malloc(Vect_get_num_lines(&PMap) * sizeof(MILEPOST));

    /* Go throuhg each line id */
    G_debug(debug, "Process each line id");
    rsid = 1;
    for (i = 0; i < nLid; i++) {
	lid = Lid_int[i];
	G_debug(debug, "lid = %d", lid);

	/* Select all LINES for current lid */

	sprintf(buf, "%s = %d", lidcol_opt->answer, lid);
	ncat = db_select_int(ldriver, Lfi->table, Lfi->key, buf, &cats);
	G_debug(debug, "  %d cats selected:", ncat);

	/* Go through all lines and store numbers of those matching category */
	nrlines = 0;
	for (line = 1; line <= Vect_get_num_lines(&In); line++) {
	    type = Vect_read_line(&In, LPoints, LCats, line);
	    if (!(type & GV_LINES))
		continue;
	    if (!(Vect_cat_get(LCats, lfield, &cat))) {
		G_warning(_("Line [%d] without category (layer [%d])"),
			  line, lfield);
		continue;
	    }
	    for (j = 0; j < ncat; j++) {
		if (cat == cats[j]) {
		    rlines[nrlines].line = line;
		    rlines[nrlines].cat = cat;
		    rlines[nrlines].length = Vect_line_length(LPoints);
		    rlines[nrlines].nmposts = 0;
		    rlines[nrlines].err = ERR_OK;
		    nrlines++;
		    break;
		}
	    }
	}
	G_debug(debug, "  %d lines selected for line id %d", nrlines, lid);
	free(cats);

	if (nrlines == 0) {
	    G_warning(_("No lines selected for line id [%d]"), lid);
	    continue;
	}

	/* Select all POINTS for current lid */
	/* Note: all attributes of MPs are saved in mposts, but if point of that
	 *  cat does not exist, or line is not in threshold line_idx is set
	 *  to PORT_DOUBLE_MAX, and such records are not used after qsort */

	/* Select all attributes for points */
	sprintf(buf, "select %s, %s, %s, %s, %s from %s where %s = %d",
		Pfi->key, start_mp_opt->answer, start_off_opt->answer,
		end_mp_opt->answer, end_off_opt->answer, Pfi->table,
		pidcol_opt->answer, lid);
	G_debug(debug, "  SQL: %s", buf);
	db_init_string(&pstmt);
	db_append_string(&pstmt, buf);

	if (db_open_select_cursor(pdriver, &pstmt, &pcursor, DB_SEQUENTIAL) !=
	    DB_OK)
	    G_fatal_error(_("Unable to select point attributes from <%s>"),
			  Pfi->table);

	ptable = db_get_cursor_table(&pcursor);

	nmposts = 0;
	while (1) {
	    double mp_tmp, mp_tmp2, off_tmp, off_tmp2;

	    if (db_fetch(&pcursor, DB_NEXT, &more) != DB_OK)
		G_fatal_error(_("Unable to fetch line id from line table"));

	    if (!more)
		break;

	    pcolumn = db_get_table_column(ptable, 0);	/* first column */
	    pvalue = db_get_column_value(pcolumn);
	    mposts[nmposts].cat = db_get_value_int(pvalue);

	    pcolumn = db_get_table_column(ptable, 1);
	    pvalue = db_get_column_value(pcolumn);
	    mp_tmp = db_get_value_double(pvalue);

	    pcolumn = db_get_table_column(ptable, 2);
	    pvalue = db_get_column_value(pcolumn);
	    off_tmp = db_get_value_double(pvalue);

	    /* mp must be integer */
	    if (floor(mp_tmp) != mp_tmp) {
		mp_tmp2 = floor(mp_tmp);
		off_tmp2 = off_tmp + multip * (mp_tmp - mp_tmp2);
		G_warning(_("Milepost (start) %f+%f used as %f+%f (change MP to integer)"),
			  mp_tmp, off_tmp, mp_tmp2, off_tmp2);
	    }
	    else {
		mp_tmp2 = mp_tmp;
		off_tmp2 = off_tmp;
	    }
	    mposts[nmposts].start_mp = mp_tmp2;
	    mposts[nmposts].start_off = off_tmp2;

	    pcolumn = db_get_table_column(ptable, 3);
	    pvalue = db_get_column_value(pcolumn);
	    mp_tmp = db_get_value_double(pvalue);

	    pcolumn = db_get_table_column(ptable, 4);
	    pvalue = db_get_column_value(pcolumn);
	    off_tmp = db_get_value_double(pvalue);

	    /* mp must be integer */
	    if (floor(mp_tmp) != mp_tmp) {
		mp_tmp2 = floor(mp_tmp);
		off_tmp2 = off_tmp + multip * (mp_tmp - mp_tmp2);
		G_warning(_("Milepost (end) %f+%f used as %f+%f (change MP to integer)"),
			  mp_tmp, off_tmp, mp_tmp2, off_tmp2);
	    }
	    else {
		mp_tmp2 = mp_tmp;
		off_tmp2 = off_tmp;
	    }
	    mposts[nmposts].end_mp = mp_tmp2;
	    mposts[nmposts].end_off = off_tmp2;

	    mposts[nmposts].line_idx = PORT_INT_MAX;
	    mposts[nmposts].err = ERR_NO_POINT;

	    nmposts++;
	}
	db_close_cursor(&pcursor);

	G_debug(debug, "  %d mileposts selected from db", nmposts);

	/* Go through all points and store numbers of those matching category and within
	 *  threshold of any line*/
	for (point = 1; point <= Vect_get_num_lines(&PMap); point++) {
	    type = Vect_read_line(&PMap, PPoints, PCats, point);
	    if (!(type & GV_POINT))
		continue;
	    if (!(Vect_cat_get(PCats, pfield, &cat))) {
		G_warning(_("Point [%d] without category (layer [%d])"),
			  point, pfield);
		continue;
	    }
	    mpost = -1;
	    for (j = 0; j < nmposts; j++) {
		if (cat == mposts[j].cat) {
		    mpost = j;
		    break;
		}
	    }
	    if (mpost >= 0) {
		G_debug(debug, "  Point %d:", point);
		mposts[mpost].line_idx = ERR_OK;
		mposts[mpost].x = PPoints->x[0];
		mposts[mpost].y = PPoints->y[0];
		/* Find the nearest line from selection set and fill in the
		 *  structure the distance from the beginning */
		distance_to = PORT_DOUBLE_MAX;
		for (j = 0; j < nrlines; j++) {
		    line = rlines[j].line;
		    Vect_read_line(&In, LPoints, NULL, line);
		    Vect_line_distance(LPoints, PPoints->x[0], PPoints->y[0],
				       0, 0, NULL, NULL, NULL, &dist_to, NULL,
				       &dist_along);
		    G_debug(debug,
			    " line %d dist to line = %f, dist along line = %f",
			    line, dist_to, dist_along);
		    if (dist_to < distance_to) {
			distance_to = dist_to;
			mposts[mpost].line_idx = j;
			mposts[mpost].dist_along = dist_along;
		    }
		}
		/* Check if in threshold */
		if (distance_to <= thresh) {
		    G_debug(debug,
			    "Point = %d cat = %d line = %d, distance = %f",
			    point, cat, mposts[mpost].line_idx, distance_to);
		    G_debug(debug,
			    "  start_mp = %f start_off = %f end_mp = %f end_off = %f",
			    mposts[mpost].start_mp, mposts[mpost].start_off,
			    mposts[mpost].end_mp, mposts[mpost].end_off);
		}
		else {
		    mposts[mpost].line_idx = PORT_INT_MAX;
		    mposts[mpost].err = ERR_THRESHOLD;
		    G_warning(_("Point [%d] cat [%d] is out of threshold (distance = %f)"),
			      point, cat, distance_to);
		}
	    }
	}
	G_debug(debug, "  %d points attached to line(s) of line id %d",
		nmposts, lid);

	/* Sort MPs according to line_idx, and dist_along */
	qsort((void *)mposts, nmposts, sizeof(MILEPOST), cmp_along);

	/* Reduce nmposts to exclude not attached db records of MPs from processing */
	nallmposts = nmposts;
	for (j = 0; j < nmposts; j++) {
	    if (mposts[j].line_idx == PORT_INT_MAX) {
		nmposts = j;
		break;
	    }
	}
	G_debug(debug, "  %d mileposts attached to line(s)", nmposts);

	/* Go thourough all attached MPs and fill in info about MPs to 'lines' table */
	last = -1;
	for (j = 0; j < nmposts; j++) {
	    G_debug(debug, " line_idx = %d, point cat = %d dist_along = %f",
		    mposts[j].line_idx, mposts[j].cat, mposts[j].dist_along);

	    if (mposts[j].line_idx != last) {	/* line_index changed */
		rlines[mposts[j].line_idx].first_mpost_idx = j;
		rlines[mposts[j].line_idx].nmposts = 1;
	    }
	    else {
		rlines[mposts[j].line_idx].nmposts++;
	    }
	    last = mposts[j].line_idx;
	}

	/* 1) Check number of MP 
	 * 2) Guess direction: find direction for each segment between 2 MPs and at the end
	 *  compare number of segmnets in both directions, if equal assign DIR_UNKNOWN. */
	for (j = 0; j < nrlines; j++) {
	    G_debug(debug,
		    " Guess direction line_idx = %d, cat = %d, nmposts = %d first_mpost_idx = %d",
		    j, rlines[j].cat, rlines[j].nmposts,
		    rlines[j].first_mpost_idx);

	    forward = 0;
	    backward = 0;
	    rlines[j].direction = DIR_UNKNOWN;

	    if (rlines[j].nmposts == 0)
		rlines[j].err = ERR_NO_MP;
	    if (rlines[j].nmposts == 1)
		rlines[j].err = ERR_ONE_MP;

	    if (rlines[j].nmposts < 2) {
		/* TODO?: in some cases could be possible to guess from one point?
		 *        E.g. start_mp = 0 for  -+-------------  */

		continue;
	    }

	    first = rlines[j].first_mpost_idx;
	    last = first + rlines[j].nmposts - 1;
	    for (k = first; k < last; k++) {
		if ((mposts[k].start_mp < mposts[k + 1].start_mp) ||
		    ((mposts[k].start_mp == mposts[k + 1].start_mp) &&
		     (mposts[k].start_off < mposts[k + 1].start_off))) {
		    forward++;
		    G_debug(debug, "    segment direction forward %d",
			    mposts[k].cat);
		}
		else {
		    backward++;
		    G_debug(debug, "    segment direction backward %d",
			    mposts[k].cat);
		}
	    }
	    G_debug(debug, "  forward = %d backward = %d", forward, backward);
	    if (forward > backward) {
		rlines[j].direction = DIR_FORWARD;
		G_debug(debug, "  line direction forward");
	    }
	    else if (forward < backward) {
		rlines[j].direction = DIR_BACKWARD;
		/* Recalculate distances from the other end */
		for (k = first; k <= last; k++)
		    mposts[k].dist_along =
			rlines[j].length - mposts[k].dist_along;

		G_debug(debug, "  line direction backward");
	    }
	    else {
		rlines[j].err = ERR_NO_DIR;
		G_debug(debug, "  line direction unknown");
	    }
	}

	/* Sort MPs again according to line_idx, and dist_along but with correct direction */
	qsort((void *)mposts, nmposts, sizeof(MILEPOST), cmp_along);

	/* Check order of MPs along the line and write LRS for line */
	for (j = 0; j < nrlines; j++) {
	    G_debug(debug,
		    "MAKE LR: line_idx = %d, nmposts = %d first_mpost_idx = %d",
		    j, rlines[j].nmposts, rlines[j].first_mpost_idx);


	    first = rlines[j].first_mpost_idx;
	    last = first + rlines[j].nmposts - 1;

	    /* Check order of MPs along the line if possible */
	    order = 0;		/* wrong order in case nmposts < 2 or DIR_UNKNOWN */
	    if (rlines[j].nmposts >= 2 && rlines[j].direction != DIR_UNKNOWN) {
		/* Note: some MP could have more errors at the time, only first is recorded */
		G_debug(debug, "Check order of MPs along the line");
		order = 1;	/* first we expect that MP order is OK */
		for (k = first; k <= last; k++) {
		    G_debug(debug, "  point cat = %d dist_along = %f",
			    mposts[k].cat, mposts[k].dist_along);
		    G_debug(debug,
			    "    start_mp = %f start_off = %f end_mp = %f end_off = %f",
			    mposts[k].start_mp, mposts[k].start_off,
			    mposts[k].end_mp, mposts[k].end_off);

		    /* Do not break after first error, to get printed all errors */
		    /* 1) For each MP must be end <= start */
		    ret =
			LR_cmp_mileposts(mposts[k].end_mp, mposts[k].end_off,
					 mposts[k].start_mp,
					 mposts[k].start_off);
		    if (ret == 1) {	/* end > start */
			G_warning(_("End > start for point cat [%d]"),
				  mposts[k].cat);
			mposts[k].err = ERR_END_GT_START;
			order = 0;
			continue;
		    }
		    if (k < last) {	/* each segment ( MP <-> nextMP ) */
			if (mposts[k + 1].end_mp > 0 ||
			    mposts[k + 1].end_off > 0) {
			    /* 2) For 2 MPs must be first.start < second.end 
			     *     if end > 0 ( otherwise it is considered to be NULL ) */
			    ret =
				LR_cmp_mileposts(mposts[k].start_mp,
						 mposts[k].start_off,
						 mposts[k + 1].end_mp,
						 mposts[k + 1].end_off);
			    if (ret > -1) {	/* start >= end */
				G_warning(_("Start of 1. MP >= end of 2. MP for points' "
					   "cats %[d], [%d]"), mposts[k].cat,
					  mposts[k + 1].cat);
				mposts[k].err = ERR_END_GT_START;
				order = 0;
				continue;
			    }
			}
			else {
			    /* 3) For 2 MPs must be first.start < second.start 
			     *     if end = 0 ( NULL, not used ) */
			    ret =
				LR_cmp_mileposts(mposts[k].start_mp,
						 mposts[k].start_off,
						 mposts[k + 1].start_mp,
						 mposts[k + 1].start_off);
			    if (ret > -1) {	/* start > end */
				G_warning(_("Start of 1. MP >= start of 2. MP for points' "
					   "cats [%d], [%d]"), mposts[k].cat,
					  mposts[k + 1].cat);
				mposts[k].err = ERR_END_GT_START;
				order = 0;
				continue;
			    }
			}
			/* 4) For 2 MPs must be distance along line different (duplicate points) */
			if (mposts[k].dist_along == mposts[k + 1].dist_along) {
			    G_warning(_("Distance along line identical for points' "
				       "cats [%d], [%d]"), mposts[k].cat,
				      mposts[k + 1].cat);
			    mposts[k].err = ERR_IDENT;
			    mposts[k + 1].err = ERR_IDENT;
			    order = 0;
			}
		    }
		}
	    }

	    /* Write errors if any ( and continue ) */
	    if (!order) {	/* something is wrong */
		if (rlines[j].nmposts < 2) {	/* Impossible to get reference/direction */
		    G_warning(_("Not enough points (%d) attached to the line (cat %d), "
			       "line skip."), rlines[j].nmposts,
			      rlines[j].cat);
		}
		else if (rlines[j].direction == DIR_UNKNOWN) {	/* Unknown direction */
		    G_warning(_("Unable to guess direction for the line (cat %d), "
			       "line skip."), rlines[j].cat);
		}
		else {
		    G_warning(_("Incorrect order of points along line cat [%d]"),
			      rlines[j].cat);
		    rlines[j].err = ERR_LINE_ORDER;
		}

		/* Write line errors */
		if (err_opt->answer) {
		    Vect_reset_cats(LCats);

		    Vect_read_line(&In, LPoints, NULL, rlines[j].line);
		    Vect_cat_set(LCats, 1, rlines[j].err);

		    Vect_write_line(&EMap, GV_LINE, LPoints, LCats);
		}
		continue;
	    }

	    /* Order is correct and we can store reference records for this line */
	    G_debug(debug,
		    "  lcat |   lid | start_map |   end_map |  start_mp | start_off |    end_mp |   end_off | end type");
	    for (k = first; k < last; k++) {
		/* Decide which value to use for end */
		if (mposts[k + 1].end_mp > 0 || mposts[k + 1].end_off > 0) {	/* TODO: use NULL ? */
		    totype = TO_TYPE_USER;
		    end_mp = mposts[k + 1].end_mp;
		    end_off = mposts[k + 1].end_off;
		}
		else {		/* values not specified -> use start values from next MP */
		    totype = TO_TYPE_MAP;
		    end_mp = mposts[k + 1].start_mp;
		    end_off = mposts[k + 1].start_off;
		}
		G_debug(debug,
			" %5d | %5d | %9.3f | %9.3f | %9.3f | %9.3f | %9.3f | %9.3f | %1d",
			rlines[j].cat, lid, mposts[k].dist_along,
			mposts[k + 1].dist_along, mposts[k].start_mp,
			mposts[k].start_off, end_mp, end_off, totype);

		sprintf(buf,
			"insert into %s (rsid, lcat, lid, start_map, end_map, "
			"start_mp, start_off, end_mp, end_off, end_type) "
			"values ( %d, %d, %d, %f, %f, %f, %f, %f, %f, %d )",
			table_opt->answer, rsid, rlines[j].cat, lid,
			mposts[k].dist_along, mposts[k + 1].dist_along,
			mposts[k].start_mp, mposts[k].start_off, end_mp,
			end_off, totype);
		G_debug(debug, "  SQL: %s", buf);
		db_init_string(&rsstmt);
		db_append_string(&rsstmt, buf);
		if (db_execute_immediate(rsdriver, &rsstmt) != DB_OK)
		    G_fatal_error(_("Unable to insert reference records: %s"),
				  buf);
		rsid++;

	    }
	    /* Write the line to output */
	    type = Vect_read_line(&In, LPoints, LCats, rlines[j].line);
	    Vect_reset_line(L2Points);

	    if (rlines[j].direction == DIR_FORWARD)
		dir = GV_FORWARD;
	    else
		dir = GV_BACKWARD;

	    Vect_append_points(L2Points, LPoints, dir);
	    Vect_write_line(&Out, type, L2Points, LCats);
	}

	/* Write MP errors for all points, also those out of threshold */
	if (err_opt->answer) {
	    for (k = 0; k < nallmposts; k++) {
		if (mposts[k].err != ERR_OK && mposts[k].err != ERR_NO_POINT) {
		    Vect_reset_line(PPoints);
		    Vect_reset_cats(PCats);

		    Vect_append_point(PPoints, mposts[k].x, mposts[k].y, 0);
		    Vect_cat_set(PCats, 1, mposts[k].err);

		    Vect_write_line(&EMap, GV_POINT, PPoints, PCats);
		}
	    }
	}
    }

    db_close_database_shutdown_driver(rsdriver);
    db_close_database_shutdown_driver(pdriver);
    db_close_database_shutdown_driver(ldriver);

    Vect_close(&In);
    Vect_close(&PMap);

    G_message(_("Building topology for output (out_lines) map..."));

    Vect_copy_head_data(&In, &Out);
    Vect_hist_copy(&In, &Out);
    Vect_hist_command(&Out);

    Vect_build(&Out);
    Vect_close(&Out);

    /* Write errors */
    if (err_opt->answer) {
	G_message(_("Building topology for error (err) map..."));
	Vect_build(&EMap);
	Vect_close(&EMap);
    }

    exit(EXIT_SUCCESS);
}

int cmp_along(const void *pa, const void *pb)
{
    MILEPOST *p1 = (MILEPOST *) pa;
    MILEPOST *p2 = (MILEPOST *) pb;

    /* compare line_idx */
    if (p1->line_idx < p2->line_idx)
	return -1;
    if (p1->line_idx > p2->line_idx)
	return 1;
    /* the same line_idx -> compare dist_along */
    if (p1->dist_along < p2->dist_along)
	return -1;
    if (p1->dist_along > p2->dist_along)
	return 1;
    return 0;
}


/****************************************************************************
 *
 * MODULE:       v.out.svg
 * AUTHOR(S):    Original author Klaus Foerster
 *               Klaus Foerster - klaus.foerster@uibk.ac.at
 *               OGR support by Martin Landa <landa.martin gmail.com> (2009)
 * PURPOSE:      Export GRASS vector map to SVG with custom
 *               coordinate-precision and optional attributes
 * COPYRIGHT:    (C) 2006-2009 by the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2). Read the file COPYING that
 *               comes with GRASS for details.
 *
 *****************************************************************************/

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <grass/glocale.h>
#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/vector.h>

#define SVG_NS   "http://www.w3.org/2000/svg"
#define XLINK_NS "http://www.w3.org/1999/xlink"
#define GRASS_NS "http:/grass.itc.it/2006/gg"
#define RADIUS_SCALE	.003
#define WIDTH_SCALE	.001
#define G_Areas  "G_Areas"
#define G_Lines  "G_Lines"
#define G_Points "G_Points"

#define TYPE_POINT    1
#define TYPE_LINE     2
#define TYPE_POLY     3

FILE *fpsvg;

static int mk_path(struct line_pnts *Points, int precision);
static int mk_attribs(int cat, struct field_info *Fi, dbDriver * Driver,
		      dbTable * Table, int attr_cols[], int attr_size,
		      int do_attr);
static int print_escaped_for_xml(char *str);

int main(int argc, char *argv[])
{
    int i, j, precision, field, type, nlines;
    int do_attr = 0, attr_cols[8], attr_size = 0, db_open = 0, cnt = 0;

    double width, radius;
    struct Option *in_opt, *out_opt, *prec_opt, *type_opt, *attr_opt,
	*field_opt;
    struct GModule *module;
    struct Map_info In;
    struct bound_box box;

    /* vector */
    struct line_pnts *Points;
    struct line_cats *Cats;

    /* attribs */
    dbDriver *Driver = NULL;
    dbHandle handle;
    dbTable *Table;
    dbString dbstring;
    struct field_info *Fi;

    /* init */
    G_gisinit(argv[0]);

    /* parse command-line */
    module = G_define_module();
    module->description = _("Exports a vector map to SVG file.");
    G_add_keyword(_("vector"));
    G_add_keyword(_("export"));

    in_opt = G_define_standard_option(G_OPT_V_INPUT);

    field_opt = G_define_standard_option(G_OPT_V_FIELD_ALL);

    out_opt = G_define_standard_option(G_OPT_F_OUTPUT);
    out_opt->description = _("Name for SVG output file");

    type_opt = G_define_option();
    type_opt->key = "type";
    type_opt->type = TYPE_STRING;
    type_opt->required = YES;
    type_opt->multiple = NO;
    type_opt->answer = "poly";
    type_opt->options = "poly,line,point";
    type_opt->label = _("Output type");
    type_opt->description = _("Defines which feature-type will be extracted");

    prec_opt = G_define_option();
    prec_opt->key = "precision";
    prec_opt->type = TYPE_INTEGER;
    prec_opt->required = NO;
    prec_opt->answer = "6";
    prec_opt->multiple = NO;
    prec_opt->description = _("Coordinate precision");

    attr_opt = G_define_standard_option(G_OPT_DB_COLUMNS);
    attr_opt->key = "attribute";
    attr_opt->required = NO;
    attr_opt->multiple = YES;
    attr_opt->description = _("Attribute(s) to include in output SVG");
    
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    if (type_opt->answer[0] == 'l') {
        type = TYPE_LINE;
    }
    else {
        if (type_opt->answer[2] == 'l')
            type = TYPE_POLY;
        else
            type = TYPE_POINT;
    }
            
    /* override coordinate precision if any */
    precision = atof(prec_opt->answer);
    if (precision < 0) {
	G_fatal_error(_("Precision must not be negative"));
    }
    if (precision > 15) {
	G_fatal_error(_("Precision must not be higher than 15"));
    }

    /* open input vector */
    Vect_set_open_level(2);
    Vect_open_old2(&In, in_opt->answer, "", field_opt->answer);

    /* parse field number */
    field = Vect_get_field_number(&In, field_opt->answer);

    /* open db-driver to attribs */
    db_init_string(&dbstring);

    /* check for requested field */
    Fi = Vect_get_field(&In, field);
    if (Fi != NULL) {
	Driver = db_start_driver(Fi->driver);
	if (Driver == NULL) {
	    G_fatal_error(_("Unable to start driver <%s>"), Fi->driver);
	}

	/* open db */
	db_init_handle(&handle);
	db_set_handle(&handle, Fi->database, NULL);
	if (db_open_database(Driver, &handle) != DB_OK) {
	    G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
			  Fi->database, Fi->driver);
	}

	db_set_string(&dbstring, Fi->table);
	if (db_describe_table(Driver, &dbstring, &Table) != DB_OK) {
	    G_fatal_error(_("Unable to describe table <%s>"), Fi->table);
	}

	/* define column-indices for columns to extract */
	dbColumn *Column;

	for (i = 0; i < db_get_table_number_of_columns(Table); i++) {
	    Column = db_get_table_column(Table, i);
	    if (attr_opt->answer != NULL) {
		for (j = 0; attr_opt->answers[j] != NULL; j++) {
		    if (G_strcasecmp(attr_opt->answers[j],
				     db_get_column_name(Column)) == 0) {
			attr_cols[attr_size] = i;
			attr_size += 1;
			break;
		    }
		}
	    }
	}
	do_attr = 1;
	db_open = 1;
    }

    /* parse bounding box and define default stroke-width, radius */
    Vect_get_map_box(&In, &box);
    if ((box.E - box.W) >= (box.N - box.S)) {
	radius = (box.E - box.W) * RADIUS_SCALE;
	width = (box.E - box.W) * WIDTH_SCALE;
    }
    else {
	radius = (box.N - box.S) * RADIUS_SCALE;
	width = (box.N - box.S) * WIDTH_SCALE;
    }

    /* open output SVG-file and print SVG-header with viewBox and Namenspaces */
    if ((fpsvg = fopen(out_opt->answer, "w")) == NULL) {
	G_fatal_error(_("Unable to create SVG file <%s>"), out_opt->answer);
    }

    fprintf(fpsvg, "<svg xmlns=\"%s\" xmlns:xlink=\"%s\" xmlns:gg=\"%s\" ",
	    SVG_NS, XLINK_NS, GRASS_NS);
    fprintf(fpsvg, "viewBox=\"%.*f %.*f %.*f %.*f\">\n",
	    precision, box.W,
	    precision, box.N * -1,
	    precision, box.E - box.W, precision, box.N - box.S);
    fprintf(fpsvg, "<title>v.out.svg %s %s</title>\n", in_opt->answer,
	    out_opt->answer);

    nlines = Vect_get_num_lines(&In);
    
    /* extract areas if any or requested */
    if (type == TYPE_POLY) {
	if (Vect_get_num_areas(&In) == 0) {
	    G_warning(_("No areas found, skipping %"), "type=poly");
	}
	else {
            int nareas;
            
            nareas = Vect_get_num_areas(&In);
	    /* extract area as paths */
	    fprintf(fpsvg,
		    " <g id=\"%s\" fill=\"#CCC\" stroke=\"#000\" stroke-width=\"%.*f\" >\n",
		    G_Areas, precision, width);
	    for (i = 1; i <= nareas; i++) {
		G_percent(i, nareas, 5);

		/* skip areas without centroid */
		if (Vect_get_area_centroid(&In, i) == 0) {
		    G_warning(_("Skipping area %d without centroid"), i);
		    continue;
		}

		/* extract attribs, parse area */
		Vect_get_area_cats(&In, i, Cats);
		fprintf(fpsvg, "  <path ");
		if (Cats->n_cats > 0) {
		    mk_attribs(Cats->cat[0], Fi, Driver, Table, attr_cols,
			       attr_size, do_attr);
		}
		fprintf(fpsvg, "d=\"");

		Vect_get_area_points(&In, i, Points);
		mk_path(Points, precision);

		/* append islands if any within current path */
		for (j = 0; j < Vect_get_area_num_isles(&In, i); j++) {
		    Vect_get_isle_points(&In, Vect_get_area_isle(&In, i, j),
					 Points);
		    mk_path(Points, precision);
		}
		fprintf(fpsvg, "\" />\n");
		cnt += 1;
	    }
	    fprintf(fpsvg, " </g>\n");
	    G_message(_("%d areas extracted"), cnt);
	}
    }
    
    /* extract points if requested */
    if (type == TYPE_POINT) {
	if (Vect_get_num_primitives(&In, GV_POINTS) == 0) {
	    G_warning(_("No points found, skipping %s"), "type=point");
	}
	else {
	    /* extract points as circles */
	    fprintf(fpsvg, " <g id=\"%s\" fill=\"#FC0\" stroke=\"#000\" "
		    "stroke-width=\"%.*f\" >\n", G_Points, precision, width);
	    for (i = 1; i <= nlines; i++) {
		G_percent(i, nlines, 5);
                
		if (!(Vect_read_line(&In, Points, Cats, i) & GV_POINTS))
                    continue;
                
		if (field != -1 && !Vect_cat_get(Cats, field, NULL))
		    continue;
                
		for (j = 0; j < Points->n_points; j++) {
		    fprintf(fpsvg, "  <circle ");
		    if (Cats->n_cats > 0) {
			mk_attribs(Cats->cat[j], Fi, Driver, Table, attr_cols,
				   attr_size, do_attr);
		    }
		    fprintf(fpsvg, "cx=\"%.*f\" cy=\"%.*f\" r=\"%.*f\" />\n",
			    precision, Points->x[j],
			    precision, Points->y[j] * -1, precision, radius);
		    cnt += 1;
		}

	    }
	    fprintf(fpsvg, " </g>\n");
	    G_message(_("%d points extracted"), cnt);
	}
    }
    
    /* extract lines if requested */
    if (type == TYPE_LINE) {
	if (Vect_get_num_primitives(&In, GV_LINES) == 0) {
	    G_warning(_("No lines found, skipping %s"), "type=line");
	}
	else {
	    /* extract lines as paths */
	    fprintf(fpsvg, " <g id=\"%s\" fill=\"none\" stroke=\"#000\" "
		    "stroke-width=\"%.*f\" >\n", G_Lines, precision, width);
	    for (i = 1; i <= nlines; i++) {
		G_percent(i, nlines, 5);
                
		if (!(Vect_read_line(&In, Points, Cats, i) & GV_LINES))
                    continue;
                
                if (field != -1 && !Vect_cat_get(Cats, field, NULL))
		    continue;
                
		fprintf(fpsvg, "  <path ");
		if (Cats->n_cats > 0) {
		    mk_attribs(Cats->cat[0], Fi, Driver, Table,
			       attr_cols, attr_size, do_attr);
		}

		fprintf(fpsvg, "d=\"");
		mk_path(Points, precision);
		fprintf(fpsvg, "\" />\n");
		cnt += 1;
	    }
	    fprintf(fpsvg, " </g>\n");
	    G_message(_("%d lines extracted"), cnt);
	}
    }
    /* finish code */
    fprintf(fpsvg, "</svg>\n");

    if (db_open == 1) {
	/* close database handle */
	db_close_database(Driver);
	db_shutdown_driver(Driver);
    }

    /* close SVG-file */
    fclose(fpsvg);
    
    exit(EXIT_SUCCESS);
}


static int mk_path(struct line_pnts *Points, int precision)
{
    int i;

    /* loop through points and create relative moves to save bandwidth */
    for (i = 0; i < Points->n_points; i++) {
	if (i == 0) {
	    fprintf(fpsvg, "M %.*f %.*f l",
		    precision, Points->x[i], precision, Points->y[i] * -1);
	}
	else {
	    fprintf(fpsvg, " %.*f %.*f",
		    precision, Points->x[i] - Points->x[i - 1],
		    precision, Points->y[i] * -1 - Points->y[i - 1] * -1);
	}
    }

    return 1;
}

/* extract custom-namespaced attributes if any */
static int mk_attribs(int cat, struct field_info *Fi, dbDriver * Driver,
		      dbTable * Table, int attr_cols[], int attr_size,
		      int do_attr)
{
    int i, more;
    char buf[2000];
    dbString dbstring;
    dbColumn *Column;
    dbCursor cursor;

    /* include cat in any case */
    fprintf(fpsvg, "gg:cat=\"%d\" ", cat);

    /* skip attribs if none */
    if (do_attr == 0) {
	return 1;
    }

    /* create SQL-string and query attribs */
    db_init_string(&dbstring);

    sprintf(buf, "SELECT * FROM %s WHERE %s = %d", Fi->table, Fi->key, cat);

    db_set_string(&dbstring, buf);

    if (db_open_select_cursor(Driver, &dbstring, &cursor, DB_SEQUENTIAL) !=
	DB_OK) {
	G_fatal_error(_("Cannot select attributes for cat=%d"), cat);
    }
    else {
	if (db_fetch(&cursor, DB_NEXT, &more) != DB_OK) {
	    G_fatal_error(_("Unable to fetch data from table"));
	}

	/* extract attribs and data if wanted */
	Table = db_get_cursor_table(&cursor);

	for (i = 0; i < attr_size; i++) {
	    Column = db_get_table_column(Table, attr_cols[i]);
	    db_convert_column_value_to_string(Column, &dbstring);
	    strcpy(buf, db_get_column_name(Column));
	    fprintf(fpsvg, "gg:%s=\"", G_tolcase(buf));
	    print_escaped_for_xml(db_get_string(&dbstring));
	    fprintf(fpsvg, "\" ");
	}
    }
    return 1;
}

/* escape for XML and replace double-quotes */
static int print_escaped_for_xml(char *str)
{
    for (; *str; str++) {
	switch (*str) {
	case '&':
	    fputs("&amp;", fpsvg);
	    break;
	case '<':
	    fputs("&lt;", fpsvg);
	    break;
	case '>':
	    fputs("&gt;", fpsvg);
	    break;
	case '"':
	    fputs("&quot;", fpsvg);
	    break;
	default:
	    fputc(*str, fpsvg);
	}
    }
    return 1;
}

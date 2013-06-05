
/***************************************************************
 *
 * MODULE:       v.db.select
 * 
 * AUTHOR(S):    Radim Blazek
 *               OGR support by Martin Landa <landa.martin gmail.com>
 *               
 * PURPOSE:      Print vector attributes
 *               
 * COPYRIGHT:    (C) 2005-2009, 2011-2012 by the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2). Read the file COPYING that
 *               comes with GRASS for details.
 *
 **************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include <grass/glocale.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/dbmi.h>

int main(int argc, char **argv)
{
    struct GModule *module;
    struct Option *map_opt, *field_opt, *fs_opt, *vs_opt, *nv_opt, *col_opt,
	*where_opt, *file_opt;
    struct Flag *c_flag, *v_flag, *r_flag;
    dbDriver *driver;
    dbString sql, value_string;
    dbCursor cursor;
    dbTable *table;
    dbColumn *column;
    dbValue *value;
    struct field_info *Fi;
    int ncols, col, more;
    struct Map_info Map;
    char query[1024];
    struct ilist *list_lines;
    char *fs, *vs;
    struct bound_box *min_box, *line_box;
    int i, line, area, init_box, cat;

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("attribute table"));
    G_add_keyword(_("database"));
    module->description = _("Prints vector map attributes.");

    map_opt = G_define_standard_option(G_OPT_V_MAP);

    field_opt = G_define_standard_option(G_OPT_V_FIELD);
    field_opt->guisection = _("Selection");

    col_opt = G_define_standard_option(G_OPT_DB_COLUMNS);
    col_opt->guisection = _("Selection");
    
    where_opt = G_define_standard_option(G_OPT_DB_WHERE);
    where_opt->guisection = _("Selection");
    
    fs_opt = G_define_standard_option(G_OPT_F_SEP);
    fs_opt->guisection = _("Format");

    vs_opt = G_define_standard_option(G_OPT_F_SEP);
    vs_opt->key = "vs";
    vs_opt->label = _("Output vertical record separator");
    vs_opt->answer = NULL;
    vs_opt->guisection = _("Format");

    nv_opt = G_define_option();
    nv_opt->key = "nv";
    nv_opt->type = TYPE_STRING;
    nv_opt->required = NO;
    nv_opt->description = _("Null value indicator");
    nv_opt->guisection = _("Format");

    file_opt = G_define_standard_option(G_OPT_F_OUTPUT); 
    file_opt->key = "file";
    file_opt->required = NO; 
    file_opt->description = 
	_("Name for output file (if omitted or \"-\" output to stdout)"); 
    
    r_flag = G_define_flag();
    r_flag->key = 'r';
    r_flag->description =
	_("Print minimal region extent of selected vector features instead of attributes");
    r_flag->guisection = _("Region");

    c_flag = G_define_flag();
    c_flag->key = 'c';
    c_flag->description = _("Do not include column names in output");
    c_flag->guisection = _("Format");

    v_flag = G_define_flag();
    v_flag->key = 'v';
    v_flag->description = _("Vertical output (instead of horizontal)");
    v_flag->guisection = _("Format");

    G_gisinit(argv[0]);

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    /* set input vector map name and mapset */
    if (file_opt->answer && strcmp(file_opt->answer, "-") != 0) { 
	if (NULL == freopen(file_opt->answer, "w", stdout)) { 
	    G_fatal_error(_("Unable to open file <%s> for writing"), file_opt->answer); 
	} 
    } 
    
    if (r_flag->answer) {
	min_box = (struct bound_box *) G_malloc(sizeof(struct bound_box));
	G_zero((void *)min_box, sizeof(struct bound_box));

	line_box = (struct bound_box *) G_malloc(sizeof(struct bound_box));
	list_lines = Vect_new_list();
    }
    else {
      min_box = line_box = NULL;
      list_lines = NULL;
    }

    /* the field separator */
    fs = G_option_to_separator(fs_opt);
    if (vs_opt->answer)
        vs = G_option_to_separator(vs_opt);
    else
        vs = NULL;
    
    db_init_string(&sql);
    db_init_string(&value_string);

    /* open input vector */
    if (!r_flag->answer)
	Vect_open_old_head2(&Map, map_opt->answer, "", field_opt->answer);
    else {
	if (2 > Vect_open_old2(&Map, map_opt->answer, "", field_opt->answer)) {
	    Vect_close(&Map);
	    G_fatal_error(_("Unable to open vector map <%s> at topology level. "
			   "Flag '%c' requires topology level."),
			  map_opt->answer, r_flag->key);
	}
    }

    if ((Fi = Vect_get_field2(&Map, field_opt->answer)) == NULL)
	G_fatal_error(_("Database connection not defined for layer <%s>"),
		      field_opt->answer);

    driver = db_start_driver_open_database(Fi->driver, Fi->database);

    if (!driver)
	G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
		      Fi->database, Fi->driver);

    if (col_opt->answer)
	sprintf(query, "SELECT %s FROM ", col_opt->answer);
    else
	sprintf(query, "SELECT * FROM ");

    db_set_string(&sql, query);
    db_append_string(&sql, Fi->table);

    if (where_opt->answer) {
	char *buf = NULL;

	buf = G_malloc((strlen(where_opt->answer) + 8));
	sprintf(buf, " WHERE %s", where_opt->answer);
	db_append_string(&sql, buf);
	G_free(buf);
    }

    if (db_open_select_cursor(driver, &sql, &cursor, DB_SEQUENTIAL) != DB_OK)
	G_fatal_error(_("Unable to open select cursor"));

    table = db_get_cursor_table(&cursor);
    ncols = db_get_table_number_of_columns(table);

    /* column names if horizontal output (ignore for -r) */
    if (!v_flag->answer && !c_flag->answer && !r_flag->answer) {
	for (col = 0; col < ncols; col++) {
	    column = db_get_table_column(table, col);
	    if (col)
		fprintf(stdout, "%s", fs);
	    fprintf(stdout, "%s", db_get_column_name(column));
	}
	fprintf(stdout, "\n");
    }

    init_box = 1;

    /* fetch the data */
    while (1) {
	if (db_fetch(&cursor, DB_NEXT, &more) != DB_OK)
	    G_fatal_error(_("Unable to fetch data from table <%s>"),
			  Fi->table);

	if (!more)
	    break;

	cat = -1;
	for (col = 0; col < ncols; col++) {
	    column = db_get_table_column(table, col);
	    value = db_get_column_value(column);

	    if (cat < 0 && strcmp(Fi->key, db_get_column_name(column)) == 0) {
		cat = db_get_value_int(value);
		if (r_flag->answer)
		    break;
	    }

	    if (r_flag->answer)
		continue;

	    db_convert_column_value_to_string(column, &value_string);

	    if (!c_flag->answer && v_flag->answer)
		fprintf(stdout, "%s%s", db_get_column_name(column), fs);

	    if (col && !v_flag->answer)
		fprintf(stdout, "%s", fs);

	    if (nv_opt->answer && db_test_value_isnull(value))
		fprintf(stdout, "%s", nv_opt->answer);
	    else
		fprintf(stdout, "%s", db_get_string(&value_string));

	    if (v_flag->answer)
		fprintf(stdout, "\n");
	}

	if (r_flag->answer) {
	    /* get minimal region extent */
	    Vect_cidx_find_all(&Map, Vect_get_field_number(&Map, field_opt->answer), -1, cat, list_lines);
	    for (i = 0; i < list_lines->n_values; i++) {
		line = list_lines->value[i];
		if (Vect_get_line_type(&Map, line) == GV_CENTROID) {
		    area = Vect_get_centroid_area(&Map, line);
		    if (area > 0) {
			if (!Vect_get_area_box(&Map, area, line_box))
			    G_fatal_error(_("Unable to get bounding box of area %d"),
					  area);
		    }
		}
		else {
		    if (!Vect_get_line_box(&Map, line, line_box))
			G_fatal_error(_("Unable to get bounding box of line %d"),
				      line);
		}
		if (init_box) {
		    Vect_box_copy(min_box, line_box);
		    init_box = 0;
		}
		else {
		    Vect_box_extend(min_box, line_box);
		}
	    }
	}
	else {
	    if (!v_flag->answer)
		fprintf(stdout, "\n");
	    else if (vs)
		fprintf(stdout, "%s\n", vs);
	}
    }

    if (r_flag->answer) {
	fprintf(stdout, "n=%f\n", min_box->N);
	fprintf(stdout, "s=%f\n", min_box->S);
	fprintf(stdout, "w=%f\n", min_box->W);
	fprintf(stdout, "e=%f\n", min_box->E);
	if (Vect_is_3d(&Map)) {
	    fprintf(stdout, "t=%f\n", min_box->T);
	    fprintf(stdout, "b=%f\n", min_box->B);
	}
	fflush(stdout);

	G_free((void *)min_box);
	G_free((void *)line_box);

	Vect_destroy_list(list_lines);
    }

    db_close_cursor(&cursor);
    db_close_database_shutdown_driver(driver);
    Vect_close(&Map);

    exit(EXIT_SUCCESS);
}


/****************************************************************************
 *
 * MODULE:       v.to.db
 * AUTHOR(S):    Radim Blazek <radim.blazek gmail.com> (original contributor)
 *               Wolf Bergenheim <wolf+grass bergenheim net>,
 *               Glynn Clements <glynn gclements.plus.com>,
 *               Markus Neteler <neteler itc.it>
 * PURPOSE:      load values from vector to database
 * COPYRIGHT:    (C) 2000-2020 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>
#include "global.h"

struct value *Values;
struct options options;
struct vstat vstat;

int main(int argc, char *argv[])
{
    int n, i, j, cat, lastcat, type, id, findex;
    struct Map_info Map;
    struct GModule *module;
    struct field_info *Fi, *qFi;
    int ncols;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("attribute table"));
    G_add_keyword(_("database"));
    G_add_keyword(_("area"));
    G_add_keyword(_("azimuth"));
    G_add_keyword(_("bounding box"));
    G_add_keyword(_("category"));
    G_add_keyword(_("compactness"));
    G_add_keyword(_("coordinates"));
    G_add_keyword(_("fractal"));
    G_add_keyword(_("geometry"));
    G_add_keyword(_("length"));
    G_add_keyword(_("perimeter"));
    G_add_keyword(_("sides"));
    G_add_keyword(_("sinuous"));
    G_add_keyword(_("slope"));
    module->description = _("Populates attribute values from vector features.");
    module->overwrite = 1;

    parse_command_line(argc, argv);

    if (!options.print && !options.total) {
        const char *mapset;

        mapset = G_find_vector2(options.name, "");
        if (!mapset || (strcmp(mapset, G_mapset()) != 0))
            G_fatal_error(_("Vector map <%s> not found in the current mapset. "
                            "Unable to modify vector maps from different mapsets."),
                          options.name);
    }

    G_begin_distance_calculations();
    G_begin_polygon_area_calculations();

    /* open map */
    Vect_set_open_level(2);
    if (Vect_open_old(&Map, options.name, "") < 0)
	G_fatal_error(_("Unable to open vector map <%s>"), options.name);
    Vect_set_error_handler_io(&Map, NULL);
   
    Fi = Vect_get_field(&Map, options.field);

    if (!options.print && Fi == NULL) {
	G_fatal_error(_("Database connection not defined for layer %d. "
			"Use v.db.connect first."),
		      options.field);
    }

    qFi = Vect_get_field(&Map, options.qfield);
    if (options.option == O_QUERY && qFi == NULL)
        G_fatal_error(_("Database connection not defined for layer %d. Use v.db.connect first."),
                      options.qfield);

    if (!options.print) {
	dbDriver *driver = NULL;
	dbString table_name;
	dbTable *table;
	dbColumn *column;
	const char *colname;
	int col, fncols, icol;
	int col_sqltype[4];
	int create_col[4], create_cols;
	int qlength;

	/* get required column types */
	col_sqltype[0] = col_sqltype[1] = col_sqltype[2] = col_sqltype[3] = -1;
	ncols = 1;
	qlength = 0;

	switch (options.option) {
	case O_CAT:
	case O_COUNT:
	    col_sqltype[0] = DB_SQL_TYPE_INTEGER;
	    break;

	case O_LENGTH:
	case O_AREA:
	case O_PERIMETER:
	case O_SLOPE:
	case O_SINUOUS:
	case O_AZIMUTH:
	case O_COMPACT:
	case O_FD:
	    col_sqltype[0] = DB_SQL_TYPE_DOUBLE_PRECISION;
	    break;
	
	case O_BBOX:
	    col_sqltype[0] = col_sqltype[1] = col_sqltype[2] = col_sqltype[3] = DB_SQL_TYPE_DOUBLE_PRECISION;
	    ncols = 4;
	    break;

	case O_COOR:
	case O_START:
	case O_END:
	    col_sqltype[0] = col_sqltype[1] = col_sqltype[2] = DB_SQL_TYPE_DOUBLE_PRECISION;
	    ncols = 2;
	    if (options.col[2])
		ncols = 3;
	    break;

	case O_SIDES:
	    col_sqltype[0] = col_sqltype[1] = DB_SQL_TYPE_INTEGER;
	    ncols = 2;
	    break;

	case O_QUERY:
	    driver = db_start_driver_open_database(qFi->driver, qFi->database);
	    db_init_string(&table_name);
	    db_set_string(&table_name, qFi->table);
	    if (db_describe_table(driver, &table_name, &table) != DB_OK)
		G_fatal_error(_("Unable to describe table <%s>"),
			      qFi->table);

	    fncols = db_get_table_number_of_columns(table);
	    for (col = 0; col < fncols; col++) {
		column = db_get_table_column(table, col);
		colname = db_get_column_name(column);
		if (strcmp(options.qcol, colname) == 0) {
		    col_sqltype[0] = db_get_column_sqltype(column);
		    qlength = db_get_column_length(column);
		    break;
		}
	    }
	    db_close_database_shutdown_driver(driver);
	    driver = NULL;
	    db_free_string(&table_name);
	    break;
	}

	/* check if columns exist */
	create_col[0] = create_col[1] = create_col[2] = create_col[3] = 0;
	create_cols = 0;
	driver = db_start_driver_open_database(Fi->driver, Fi->database);
	db_init_string(&table_name);
	db_set_string(&table_name, Fi->table);
	if (db_describe_table(driver, &table_name, &table) != DB_OK)
	    G_fatal_error(_("Unable to describe table <%s>"),
			  qFi->table);

	fncols = db_get_table_number_of_columns(table);
	for (col = 0; col < ncols; col++) {
	    int col_exists = 0;

	    if (options.col[col] == NULL)
		G_fatal_error(_("Missing column name for input column number %d"), col + 1);

	    for (icol = 0; icol < fncols; icol++) {
		column = db_get_table_column(table, icol);
		colname = db_get_column_name(column);
		if (colname == NULL)
		    G_fatal_error(_("Missing column name for table column number %d"), col + 1);
		if (strcmp(options.col[col], colname) == 0) {
		    int isqltype;

		    col_exists = 1;
		    isqltype = db_get_column_sqltype(column);

		    if (isqltype != col_sqltype[col]) {
			int ctype1, ctype2;

			ctype1 = db_sqltype_to_Ctype(isqltype);
			ctype2 = db_sqltype_to_Ctype(col_sqltype[col]);
			
			if (ctype1 == ctype2) {
			    G_warning(_("Existing column <%s> has a different but maybe compatible type"),
					  options.col[col]);
			}
			else {
			    G_fatal_error(_("Existing column <%s> has the wrong type"),
					  options.col[col]);
			}
		    }

		    if (G_get_overwrite())
			G_warning(_("Values in column <%s> will be overwritten"),
				  options.col[col]);
		    else
			G_fatal_error(_("Column <%s> exists. To overwrite, use the --overwrite flag"),
				      options.col[col]);

		    break;
		}
	    }
	    if (!col_exists) {
		create_col[col] = 1;
		create_cols = 1;
	    }
	}
	db_close_database_shutdown_driver(driver);
	driver = NULL;
	db_free_string(&table_name);

	/* create columns if not existing */
	if (create_cols) {
	    char sqlbuf[4096];
	    dbString stmt;

	    db_init_string(&stmt);
	    driver = db_start_driver_open_database(Fi->driver, Fi->database);
	    db_begin_transaction(driver);
	    for (col = 0; col < ncols; col++) {
		if (!create_col[col])
		    continue;

		if (col_sqltype[col] == DB_SQL_TYPE_INTEGER) {
		    sprintf(sqlbuf, "ALTER TABLE %s ADD COLUMN %s integer",
			    Fi->table, options.col[col]);
		}
		else if (col_sqltype[col] == DB_SQL_TYPE_DOUBLE_PRECISION ||
		         col_sqltype[col] == DB_SQL_TYPE_REAL) {
		    sprintf(sqlbuf, "ALTER TABLE %s ADD COLUMN %s double precision",
			    Fi->table, options.col[col]);
		}
		else if (col_sqltype[col] == DB_SQL_TYPE_CHARACTER) {
		    if (qlength > 0) {
			sprintf(sqlbuf, "ALTER TABLE %s ADD COLUMN %s varchar(%d)",
				Fi->table, options.col[col], qlength);
		    }
		    else {
			sprintf(sqlbuf, "ALTER TABLE %s ADD COLUMN %s text",
				Fi->table, options.col[col]);
		    }
		}
		else if (col_sqltype[col] == DB_SQL_TYPE_TEXT) {
		    sprintf(sqlbuf, "ALTER TABLE %s ADD COLUMN %s text",
			    Fi->table, options.col[col]);
		}
		else if (col_sqltype[col] == DB_SQL_TYPE_DATE) {
		    sprintf(sqlbuf, "ALTER TABLE %s ADD COLUMN %s date",
			    Fi->table, options.col[col]);
		}
		else if (col_sqltype[col] == DB_SQL_TYPE_TIME) {
		    sprintf(sqlbuf, "ALTER TABLE %s ADD COLUMN %s time",
			    Fi->table, options.col[col]);
		}
		else {
		    sprintf(sqlbuf, "ALTER TABLE %s ADD COLUMN %s %s",
			    Fi->table, options.col[col],
			    db_sqltype_name(col_sqltype[col]));
		}
		db_set_string(&stmt, sqlbuf);
		if (db_execute_immediate(driver, &stmt) != DB_OK) {
		    G_fatal_error(_("Unable to create column <%s>"),
				  options.col[col]);
		}
	    }
	    db_commit_transaction(driver);
	    db_close_database_shutdown_driver(driver);
	    db_free_string(&stmt);
	}
    }

    /* allocate array for values */
    /* (+1 is for cat -1 (no category) reported at the end ) */
    findex = Vect_cidx_get_field_index(&Map, options.field);
    if (findex > -1) {
	n = Vect_cidx_get_num_unique_cats_by_index(&Map, findex);
    }
    else {
	n = 0;
    }
    G_debug(2, "%d unique cats", n);
    Values = (struct value *) G_calloc(n + 1, sizeof(struct value));

    /* prepopulate Values */
    if (findex > -1)
	n = Vect_cidx_get_num_cats_by_index(&Map, findex);
    i = 0;
    Values[i].cat = -1;		/* features without category */
    Values[i].used = 0;
    Values[i].count1 = 0;
    Values[i].count2 = 0;
    Values[i].i1 = -1;
    Values[i].i2 = -1;
    Values[i].d1 = 0.0;
    Values[i].d2 = 0.0;
    Values[i].d3 = 0.0;
    Values[i].d4 = 0.0;
    if (options.option == O_BBOX) {
	Values[i].d1 = -PORT_DOUBLE_MAX;
	Values[i].d2 = PORT_DOUBLE_MAX;
	Values[i].d3 = -PORT_DOUBLE_MAX;
	Values[i].d4 = PORT_DOUBLE_MAX;
    }
    Values[i].qcat = NULL;
    Values[i].nqcats = 0;
    Values[i].aqcats = 0;

    i = 1;
    lastcat = -1;
    /* category index must be sorted,
     * i.e. topology must have been built with GV_BUILD_ALL */
    for (j = 0; j < n; j++) {
	Vect_cidx_get_cat_by_index(&Map, findex, j, &cat, &type, &id);
	if (lastcat > cat) {
	    Vect_close(&Map);
	    G_fatal_error(_("Category index for vector map <%s> is not sorted"),
	                  options.name);
	}

	if (lastcat != cat) {
	    Values[i].cat = cat;
	    Values[i].used = 0;
	    Values[i].count1 = 0;
	    Values[i].count2 = 0;
	    Values[i].i1 = -1;
	    Values[i].i2 = -1;
	    Values[i].d1 = 0.0;
	    Values[i].d2 = 0.0;
	    Values[i].d3 = 0.0;
	    Values[i].d4 = 0.0;
	    if (options.option == O_BBOX) {
		Values[i].d1 = -PORT_DOUBLE_MAX;
		Values[i].d2 = PORT_DOUBLE_MAX;
		Values[i].d3 = -PORT_DOUBLE_MAX;
		Values[i].d4 = PORT_DOUBLE_MAX;
	    }
	    Values[i].qcat = NULL;
	    Values[i].nqcats = 0;
	    Values[i].aqcats = 0;

	    lastcat = cat;
	    i++;
	}
    }

    vstat.rcat = i;

    /* Read values from map */
    if (options.option == O_QUERY) {
	query(&Map);
    }
    else if ((options.option == O_AREA) || (options.option == O_COMPACT) ||
	     (options.option == O_PERIMETER) || (options.option == O_FD) || 
	     (options.option == O_BBOX)) {
	read_areas(&Map);
    }
    else {
	read_lines(&Map);
    }

    /* prune unused values */
    n = vstat.rcat;
    j = 0;
    for (i = 0; i < n; i++) {
	if (Values[i].used) {
	    Values[j] = Values[i];
	    j++;
	}
    }
    vstat.rcat = j;

    conv_units();

    if (options.print || options.total) {
	report();
    }
    else {
	update(&Map);
	Vect_set_db_updated(&Map);
    }

    Vect_close(&Map);

    if (!(options.print || options.total)) {
	print_stat();

	if (Vect_open_update_head(&Map, options.name, "") < 0)
	    G_warning(_("Unable to write history for vector map <%s>"),
		      options.name);
	else {
	    Vect_hist_command(&Map);
	    Vect_close(&Map);
	}
    }

    /* free list */
    G_free(Values);

    exit(EXIT_SUCCESS);
}

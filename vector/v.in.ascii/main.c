
/****************************************************************************
 *
 * MODULE:       v.in.ascii
 *               
 * AUTHOR(S):    Original authors Michael Higgins, James Westervelt (CERL)
 *               Updated to GRASS 5.7 Radim Blazek, ITC-Irst, Trento, Italy
 * PURPOSE:      Converts a vector map in ASCII format to a vector map
 *               in binary format
 *
 * COPYRIGHT:    (C) 2000-2009 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/vector.h>
#include <grass/glocale.h>
#include "local_proto.h"

#define	A_DIR	"dig_ascii"

int main(int argc, char *argv[])
{
    FILE *ascii;
    struct GModule *module;
    struct Option *old, *new, *delim_opt, *columns_opt, *xcol_opt,
	*ycol_opt, *zcol_opt, *catcol_opt, *format_opt, *skip_opt;
    int xcol, ycol, zcol, catcol, format, skip_lines;
    struct Flag *zcoorf, *t_flag, *e_flag, *noheader_flag, *notopol_flag,
	*region_flag;
    char *table;
    char *fs;
    char *desc;
    int zcoor = WITHOUT_Z, make_table;

    struct Map_info Map;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("import"));
    G_add_keyword("ASCII");
    module->description =
	_("Creates a vector map from an ASCII points file or ASCII vector file.");

    /************************** Command Parser ************************************/
    old = G_define_standard_option(G_OPT_F_INPUT);
    old->label = _("Name of input file to be imported");
    old->description = ("'-' for standard input");
    
    new = G_define_standard_option(G_OPT_V_OUTPUT);

    format_opt = G_define_option();
    format_opt->key = "format";
    format_opt->type = TYPE_STRING;
    format_opt->required = NO;
    format_opt->multiple = NO;
    format_opt->options = "point,standard";
    desc = NULL;
    G_asprintf(&desc,
	       "point;%s;standard;%s",
	       _("simple x,y[,z] list"),
	       _("GRASS vector ASCII format"));
    format_opt->descriptions = desc;
    format_opt->answer = "point";
    format_opt->description = _("Input file format");
    format_opt->guisection = _("Input format");

    delim_opt = G_define_standard_option(G_OPT_F_SEP);
    delim_opt->guisection = _("Input format");
    
    skip_opt = G_define_option();
    skip_opt->key = "skip";
    skip_opt->type = TYPE_INTEGER;
    skip_opt->required = NO;
    skip_opt->multiple = NO;
    skip_opt->answer = "0";
    skip_opt->description =
	_("Number of header lines to skip at top of input file (points mode)");
    skip_opt->guisection = _("Points");

    columns_opt = G_define_option();
    columns_opt->key = "columns";
    columns_opt->type = TYPE_STRING;
    columns_opt->required = NO;
    columns_opt->multiple = NO;
    columns_opt->guisection = _("Points");
    columns_opt->label = _("Column definition in SQL style (points mode)");
    columns_opt->description = _("For example: "
				 "'x double precision, y double precision, cat int, "
				 "name varchar(10)'");
    
    xcol_opt = G_define_option();
    xcol_opt->key = "x";
    xcol_opt->type = TYPE_INTEGER;
    xcol_opt->required = NO;
    xcol_opt->multiple = NO;
    xcol_opt->answer = "1";
    xcol_opt->guisection = _("Points");
    xcol_opt->label = ("Number of column used as x coordinate (points mode)");
    xcol_opt->description = _("First column is 1");

    ycol_opt = G_define_option();
    ycol_opt->key = "y";
    ycol_opt->type = TYPE_INTEGER;
    ycol_opt->required = NO;
    ycol_opt->multiple = NO;
    ycol_opt->answer = "2";
    ycol_opt->guisection = _("Points");
    ycol_opt->label = _("Number of column used as y coordinate (points mode)");
    ycol_opt->description = _("First column is 1");

    zcol_opt = G_define_option();
    zcol_opt->key = "z";
    zcol_opt->type = TYPE_INTEGER;
    zcol_opt->required = NO;
    zcol_opt->multiple = NO;
    zcol_opt->answer = "0";
    zcol_opt->guisection = _("Points");
    zcol_opt->label = _("Number of column used as z coordinate (points mode)");
    zcol_opt->description = _("First column is 1. If 0, z coordinate is not used");

    catcol_opt = G_define_option();
    catcol_opt->key = "cat";
    catcol_opt->type = TYPE_INTEGER;
    catcol_opt->required = NO;
    catcol_opt->multiple = NO;
    catcol_opt->answer = "0";
    catcol_opt->guisection = _("Points");
    catcol_opt->label =
	_("Number of column used as category (points mode)");
    catcol_opt->description =
	_("First column is 1. If 0, unique category is assigned to each row and written to new column 'cat'");
    
    zcoorf = G_define_flag();
    zcoorf->key = 'z';
    zcoorf->description = _("Create 3D vector map");

    e_flag = G_define_flag();
    e_flag->key = 'e';
    e_flag->description =
	_("Create a new empty vector map and exit. Nothing is read from input.");

    noheader_flag = G_define_flag();
    noheader_flag->key = 'n';
    noheader_flag->description =
	_("Don't expect a header when reading in standard format");
    noheader_flag->guisection = _("Input format");

    t_flag = G_define_flag();
    t_flag->key = 't';
    t_flag->description = _("Do not create table in points mode");
    t_flag->guisection = _("Points");

    notopol_flag = G_define_standard_flag(G_FLG_V_TOPO);
    notopol_flag->description = _("Do not build topology in points mode");
    notopol_flag->guisection = _("Points");

    region_flag = G_define_flag();
    region_flag->key = 'r';
    region_flag->description =
	_("Only import points falling within current region (points mode)");
    region_flag->guisection = _("Points");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);


    if (format_opt->answer[0] == 'p')
	format = GV_ASCII_FORMAT_POINT;
    else
	format = GV_ASCII_FORMAT_STD;

    skip_lines = atoi(skip_opt->answer);
    if (skip_lines < 0)
	G_fatal_error(_("Please specify reasonable number of lines to skip"));

    if (zcoorf->answer && format == GV_ASCII_FORMAT_POINT && !zcol_opt->answer)
	G_fatal_error(_("Please specify z column"));

    xcol = atoi(xcol_opt->answer) - 1;
    ycol = atoi(ycol_opt->answer) - 1;
    zcol = atoi(zcol_opt->answer) - 1;

    /* specifying zcol= implies that a 3D map is needed */
    if (zcol >= 0 && !zcoorf->answer)
	zcoorf->answer = TRUE;

    if (zcoorf->answer && format == GV_ASCII_FORMAT_POINT && zcol < 0)
	G_fatal_error(_("Please specify reasonable z column"));

    catcol = atoi(catcol_opt->answer) - 1;

    if (xcol+1 < 1 || ycol+1 < 1 || zcol+1 < 0 || catcol+1 < 0)
	G_fatal_error(_("Column numbers must not be negative"));

    if (strcmp(old->answer, "-")) {
	if ((ascii = fopen(old->answer, "r")) == NULL) {
	    G_fatal_error(_("Unable to open ASCII file <%s>"), old->answer);
	}
    }
    else {
	ascii = stdin;
    }
    
    fs = delim_opt->answer;
    if (strcmp(fs, "\\t") == 0)
	fs = "\t";
    if (strcmp(fs, "tab") == 0)
	fs = "\t";
    if (strcmp(fs, "space") == 0)
	fs = " ";
    if (strcmp(fs, "comma") == 0)
	fs = ",";

    /* check dimension */
    if (zcoorf->answer) {
	zcoor = 1;
    }

    Vect_open_new(&Map, new->answer, zcoor);
    Vect_set_error_handler_io(NULL, &Map);
    Vect_hist_command(&Map);

    if (e_flag->answer) {
	Vect_build(&Map);
	Vect_close(&Map);
	exit(EXIT_SUCCESS);
    }

    if (format == GV_ASCII_FORMAT_POINT) {
	int i, rowlen, ncols, minncols, *coltype, *coltype2, *collen, nrows;
	int n_int = 0, n_double = 0, n_string = 0;
	char buf[1000];
	struct field_info *Fi;
	char *tmp, *key;
	dbDriver *driver;
	dbString sql;
	FILE *tmpascii;

	/* Open temporary file */
	tmp = G_tempfile();
	if (NULL == (tmpascii = fopen(tmp, "w+"))) {
	    G_fatal_error(_("Unable to open temporary file <%s>"), tmp);
	}
	unlink(tmp);

	points_analyse(ascii, tmpascii, fs, &rowlen, &ncols, &minncols,
		       &nrows, &coltype, &collen, skip_lines, xcol, ycol,
		       region_flag->answer);

	G_verbose_message(_("Maximum input row length: %d"), rowlen);
        if (ncols != minncols) {
            G_message(_("Maximum number of columns: %d"), ncols);
            G_message(_("Minimum number of columns: %d"), minncols);
        }
        else {
               G_message(_("Number of columns: %d"), ncols);
        }
        
	/* check column numbers */
	if (xcol >= minncols) {
	    G_fatal_error(_("'%s' column number > minimum last column number "
                            "(incorrect field separator or format?)"), "x");
	}
	if (ycol >= minncols) {
	    G_fatal_error(_("'%s' column number > minimum last column number "
                            "(incorrect field separator or format?)"), "y");

	}
	if (zcol >= minncols) {
	    G_fatal_error(_("'%s' column number > minimum last column number "
                            "(incorrect field separator or format?)"), "z");

	}
	if (catcol >= minncols) {
	    G_fatal_error(_("'%s' column number > minimum last column number "
                            "(incorrect field separator or format?)"), "cat");
	}

	if (coltype[xcol] == DB_C_TYPE_STRING) {
	    G_fatal_error(_("'%s' column is not of number type"), "x");
	}
	if (coltype[ycol] == DB_C_TYPE_STRING) {
	    G_fatal_error(_("'%s' column is not of number type"), "y");
	}
	if (zcol >= 0 && coltype[zcol] == DB_C_TYPE_STRING) {
	    G_fatal_error(_("'%s' column is not of number type"), "z");
	}
	if (catcol >= 0 && coltype[catcol] == DB_C_TYPE_STRING) {
	    G_fatal_error(_("'%s' column is not of number type"), "cat");
	}

	/* Create table */
	make_table = FALSE;
	for (i = 0; i < ncols; i++) {
	    if (xcol != i && ycol != i && zcol != i && catcol != i) {
		make_table = TRUE;
		break;
	    }
	}
	if (t_flag->answer) {
	    make_table = FALSE;
	}

	if (make_table) {
	    Fi = Vect_default_field_info(&Map, 1, NULL, GV_1TABLE);
	    driver =
		db_start_driver_open_database(Fi->driver,
					      Vect_subst_var(Fi->database,
							     &Map));
	    if (driver == NULL) {
		G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
			      Vect_subst_var(Fi->database, &Map), Fi->driver);
	    }
	    db_begin_transaction(driver);

	    db_init_string(&sql);
	    sprintf(buf, "create table %s ( ", Fi->table);
	    db_append_string(&sql, buf);

	    if (catcol < 0) {
		db_append_string(&sql, "cat integer, ");
	    }

	    for (i = 0; i < ncols; i++) {
		if (i > 0 && !columns_opt->answer) {
		    db_append_string(&sql, ", ");
		}
		if (catcol == i && coltype[i] != DB_C_TYPE_INT) {
		    G_fatal_error(_("Category column is not of integer type"));
		}

		switch (coltype[i]) {
		case DB_C_TYPE_INT:
		    G_verbose_message("Column: %d  type: integer", i + 1);
		    if (!columns_opt->answer) {
			sprintf(buf, "int_%d integer", n_int + 1);
			db_append_string(&sql, buf);
			if (catcol == i) {
			    sprintf(buf, "int_%d", n_int + 1);
			    key = G_store(buf);
			}
		    }
		    n_int++;
		    break;
		case DB_C_TYPE_DOUBLE:
		    G_verbose_message("Column: %d  type: double", i + 1);
		    if (!columns_opt->answer) {
			sprintf(buf, "dbl_%d double precision", n_double + 1);
			db_append_string(&sql, buf);
		    }
		    n_double++;
		    break;
		case DB_C_TYPE_STRING:
		    G_verbose_message("Column: %d  type: string length: %d",
				      i + 1, collen[i]);
		    if (!columns_opt->answer) {
			sprintf(buf, "str_%d varchar(%d)", n_string + 1,
				collen[i]);
			db_append_string(&sql, buf);
		    }
		    n_string++;
		    break;
		}
	    }
	    if (columns_opt->answer) {
		db_append_string(&sql, columns_opt->answer);
	    }
	    db_append_string(&sql, " )");

	    /* this link is added with default 'cat' key, later deleted and replaced by true key name,
	     * otherwise if module crashes when the table exists but link is not written it makes troubles */
	    Vect_map_add_dblink(&Map, 1, NULL, Fi->table, GV_KEY_COLUMN, Fi->database,
				Fi->driver);

	    /* Create table */
	    G_debug(3, db_get_string(&sql));
	    if (db_execute_immediate(driver, &sql) != DB_OK) {
		G_fatal_error(_("Unable to create table: %s"),
			      db_get_string(&sql));
	    }

	    /* Grant */
	    if (db_grant_on_table
		(driver, Fi->table, DB_PRIV_SELECT,
		 DB_GROUP | DB_PUBLIC) != DB_OK) {
		G_fatal_error(_("Unable to grant privileges on table <%s>"),
			      Fi->table);
	    }

	    /* Check column types */
	    if (columns_opt->answer) {
		int nc;
		dbTable *table;
		dbColumn *column;

		db_set_string(&sql, Fi->table);
		if (db_describe_table(driver, &sql, &table) != DB_OK) {
		    G_fatal_error(_("Unable to describe table <%s>"),
				  Fi->table);
		}

		nc = db_get_table_number_of_columns(table);

		if ((catcol >= 0 && nc != ncols) ||
		    (catcol < 0 && (nc - 1) != ncols)) {
		    G_fatal_error(_("Number of columns defined (%d) does not match number "
				   "of columns (%d) in input"),
				  catcol < 0 ? nc - 1 : nc, ncols);
		}

		coltype2 = (int *)G_malloc(ncols * sizeof(int));

		for (i = 0; i < ncols; i++) {
		    int dbcol, ctype, length;

		    if (catcol < 0)
			dbcol = i + 1;	/* first is category */
		    else
			dbcol = i;

		    column = db_get_table_column(table, dbcol);
		    ctype =
			db_sqltype_to_Ctype(db_get_column_sqltype(column));
		    length = db_get_column_length(column);
		    coltype2[i] = ctype;

		    if (catcol == i) {	/* if catcol == -1 it cannot be tru */
			key = G_store(db_get_column_name(column));
		    }

		    switch (coltype[i]) {
		    case DB_C_TYPE_INT:
			if (ctype == DB_C_TYPE_DOUBLE) {
			    G_warning(_("Column number %d <%s> defined as double "
				       "has only integer values"), i + 1,
				      db_get_column_name(column));
			}
			else if (ctype == DB_C_TYPE_STRING) {
			    G_warning(_("Column number %d <%s> defined as string "
				       "has only integer values"), i + 1,
				      db_get_column_name(column));
			}
			break;
		    case DB_C_TYPE_DOUBLE:
			if (ctype == DB_C_TYPE_INT) {
			    G_fatal_error(_("Column number %d <%s> defined as integer "
					   "has double values"), i + 1,
					  db_get_column_name(column));
			}
			else if (ctype == DB_C_TYPE_STRING) {
			    G_warning(_("Column number %d <%s> defined as string "
				       "has double values"), i + 1,
				      db_get_column_name(column));
			}
			break;
		    case DB_C_TYPE_STRING:
			if (ctype == DB_C_TYPE_INT) {
			    G_fatal_error(_("Column number %d <%s> defined as integer "
					   "has string values"), i + 1,
					  db_get_column_name(column));
			}
			else if (ctype == DB_C_TYPE_DOUBLE) {
			    G_fatal_error(_("Column number %d <%s> defined as double "
					   "has string values"), i + 1,
					  db_get_column_name(column));
			}
			if (length < collen[i]) {
			    G_fatal_error(_("Length of column %d <%s> (%d) is less than "
					   "maximum value " "length (%d)"),
					  i + 1, db_get_column_name(column),
					  length, collen[i]);
			}
			break;
		    }
		}
	    }
	    else {
		coltype2 = coltype;
	    }

	    if (catcol < 0) {
		key = GV_KEY_COLUMN;
	    }
	    else if (!columns_opt->answer) {


	    }

	    if (db_create_index2(driver, Fi->table, key) != DB_OK)
		G_warning(_("Unable to create index for table <%s>, key <%s>"),
			  Fi->table, key);

	    Vect_map_del_dblink(&Map, 1);
	    Vect_map_add_dblink(&Map, 1, NULL, Fi->table, key, Fi->database,
				Fi->driver);

	    table = Fi->table;
	}
	else {
	    driver = NULL;
	    table = NULL;
	}

	points_to_bin(tmpascii, rowlen, &Map, driver, table, fs, nrows, ncols,
		      coltype2, xcol, ycol, zcol, catcol, skip_lines);

	if (driver) {
	    G_message(_("Populating table..."));
	    db_commit_transaction(driver);
	    if(db_close_database_shutdown_driver(driver) == DB_FAILED)
#ifdef __MINGW32__
		G_warning("FIXME: db_close_database_shutdown_driver() fails on WinGrass. Ignoring...");
#else
		G_fatal_error(_("Could not close attribute table. The DBMI driver did not accept all attributes"));
#endif
	}
	fclose(tmpascii);
    }

    else {			/* FORMAT_ALL (standard mode) */
	if (!noheader_flag->answer)
            if (Vect_read_ascii_head(ascii, &Map) == -1)
                G_fatal_error(_("Import failed"));

	if (Vect_read_ascii(ascii, &Map) == -1)
            G_fatal_error(_("Import failed"));
    }

    if (ascii != stdin)
	fclose(ascii);

    if (notopol_flag->answer) {
	Vect_close(&Map);
    }
    else {
	Vect_build(&Map);
	Vect_close(&Map);
    }

    exit(EXIT_SUCCESS);
}

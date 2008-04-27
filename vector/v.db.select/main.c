/***************************************************************
 *
 * MODULE:       v.db.select
 * 
 * AUTHOR(S):    Radim Blazek
 *               
 * PURPOSE:      Print vector attributes
 *               
 * COPYRIGHT:    (C) 2005-2007 by the GRASS Development Team
 *
 *               This program is free software under the 
 *               GNU General Public License (>=v2). 
 *               Read the file COPYING that comes with GRASS
 *               for details.
 *
 **************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <grass/glocale.h>
#include <grass/gis.h>
#include <grass/Vect.h>
#include <grass/dbmi.h>


int main (int argc, char **argv)
{
    struct GModule *module;
    struct Option *map_opt, *field_opt, *fs_opt, *vs_opt, *nv_opt, *col_opt, *where_opt;
    struct Flag *c_flag, *v_flag;
    dbDriver *driver;
    dbString sql, value_string;
    dbCursor cursor;
    dbTable *table;
    dbColumn *column;
    dbValue *value;
    struct field_info *Fi;
    int field, ncols, col, more;
    struct Map_info Map;
    char   *mapset;
    char query[1024];

    module = G_define_module();
    module->keywords = _("vector, database, attribute table");
    module->description = _("Prints vector map attributes.");

    map_opt = G_define_standard_option(G_OPT_V_MAP);
    field_opt = G_define_standard_option(G_OPT_V_FIELD) ;

    col_opt 		= G_define_standard_option(G_OPT_COLUMN);

    where_opt = G_define_standard_option(G_OPT_WHERE);

    fs_opt 		= G_define_standard_option(G_OPT_F_SEP);
    fs_opt->description = _("Output field separator");

    vs_opt 		= G_define_standard_option(G_OPT_F_SEP);
    vs_opt->key 	= "vs";
    vs_opt->description = _("Output vertical record separator");
    vs_opt->answer      = NULL;

    nv_opt 		= G_define_option();
    nv_opt->key 	= "nv";
    nv_opt->type 	= TYPE_STRING;
    nv_opt->required 	= NO;
    nv_opt->description = _("Null value indicator");

    c_flag		= G_define_flag();
    c_flag->key		= 'c';
    c_flag->description	= _("Do not include column names in output");

    v_flag		= G_define_flag();
    v_flag->key		= 'v';
    v_flag->description	= _("Vertical output (instead of horizontal)");

    G_gisinit (argv[0]);

    if (G_parser (argc, argv))
        exit (EXIT_FAILURE);

    /* set input vector map name and mapset */
    field = atoi (field_opt->answer);

    db_init_string (&sql);
    db_init_string (&value_string);

    /* open input vector */
    if ((mapset = G_find_vector2 (map_opt->answer, "")) == NULL) {
         G_fatal_error (_("Vector map <%s> not found"), map_opt->answer);
    }

    Vect_open_old_head ( &Map, map_opt->answer, mapset);

    if ( (Fi = Vect_get_field ( &Map, field)) == NULL ) 
	G_fatal_error(_("Database connection not defined for layer %d"),
		      field);

    driver = db_start_driver_open_database ( Fi->driver, Fi->database );
    
    if (!driver)
	G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
		      Fi->database, Fi->driver);

    if ( col_opt->answer )
      sprintf(query, "SELECT %s FROM ", col_opt->answer);
    else
      sprintf(query, "SELECT * FROM ");

    db_set_string ( &sql, query );
    db_append_string ( &sql, Fi->table );

    if (where_opt->answer) {
       char *buf = NULL;

       buf = G_malloc ((strlen(where_opt->answer) + 8));
       sprintf (buf, " WHERE %s", where_opt->answer);
       db_append_string ( &sql, buf );
       G_free (buf);
    }

    if (db_open_select_cursor(driver, &sql, &cursor, DB_SEQUENTIAL) != DB_OK)
	G_fatal_error(_("Unable to open select cursor"));

    table = db_get_cursor_table (&cursor);
    ncols = db_get_table_number_of_columns (table);

    /* column names if horizontal output */
    if ( !v_flag->answer && !c_flag->answer ) {
	for (col = 0; col < ncols; col++) {
	    column = db_get_table_column(table, col);
	    if (col) fprintf (stdout, "%s", fs_opt->answer);
	    fprintf (stdout, "%s", db_get_column_name (column));
	}
	fprintf (stdout, "\n");
    }

    /* fetch the data */
    while(1) {
	if(db_fetch (&cursor, DB_NEXT, &more) != DB_OK)
	    G_fatal_error(_("Unable to fetch data from table <%s>"),
			  Fi->table);

	if (!more) break;

	for (col = 0; col < ncols; col++) {
	    column = db_get_table_column(table, col);
	    value  = db_get_column_value(column);
	    db_convert_column_value_to_string (column, &value_string);

	    if ( !c_flag->answer && v_flag->answer ) 
		fprintf (stdout, "%s%s", db_get_column_name (column), fs_opt->answer );

	    if (col && !v_flag->answer )
		fprintf (stdout, "%s", fs_opt->answer );

	    if ( nv_opt->answer && db_test_value_isnull(value) )
	        fprintf (stdout, "%s", nv_opt->answer);	
	    else
		fprintf (stdout, "%s", db_get_string (&value_string));

	    if ( v_flag->answer )
		fprintf (stdout, "\n");
	}
	if ( !v_flag->answer )
	    fprintf (stdout, "\n");
    	else if ( vs_opt->answer )
	    fprintf (stdout, "%s\n", vs_opt->answer);
    }

    if ( !driver )
        G_fatal_error(_("Unable to open database <%s> by driver <%s>"), Fi->database, Fi->driver);

    db_close_database_shutdown_driver(driver);
    Vect_close ( &Map);

    exit (EXIT_SUCCESS);
}

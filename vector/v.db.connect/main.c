
/***************************************************************
 *
 * MODULE:       v.db.connect
 * 
 * AUTHOR(S):    Markus Neteler
 *               
 * PURPOSE:      sets/prints DB connection for a given vector map
 *               
 * COPYRIGHT:    (C) 2002-2010 by the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2).  Read the file COPYING that
 *               comes with GRASS for details.
 *
 * TODO: - fix -o flag (needs fix in Vect lib)
 *
 **************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

int main(int argc, char **argv)
{
    char *input;
    struct GModule *module;
    struct Option *inopt, *dbdriver, *dbdatabase, *dbtable, *field_opt,
	*dbkey, *sep_opt;
    struct Flag *overwrite, *print, *columns, *delete, *shell_print;
    dbDriver *driver;
    dbString table_name;
    dbTable *table;
    dbHandle handle;
    struct field_info *fi;
    int field, ret, num_dblinks, i, ncols, col;
    char *fieldname;
    struct Map_info Map;

    /* set up the options and flags for the command line parser */

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("database"));
    G_add_keyword(_("attribute table"));
    module->description =
	_("Prints/sets DB connection for a vector map to attribute table.");

    inopt = G_define_standard_option(G_OPT_V_MAP);

    dbdriver = G_define_standard_option(G_OPT_DB_DRIVER);
    dbdriver->options = db_list_drivers();
    dbdriver->answer = G_store(db_get_default_driver_name());
    dbdriver->guisection = _("Settings");

    dbdatabase = G_define_standard_option(G_OPT_DB_DATABASE);
    dbdatabase->answer = G_store(db_get_default_database_name());
    dbdatabase->guisection = _("Settings");

    dbtable = G_define_standard_option(G_OPT_DB_TABLE);

    dbkey = G_define_standard_option(G_OPT_DB_COLUMN);
    dbkey->key = "key";
    dbkey->answer = "cat";
    dbkey->label = _("Key column name");
    dbkey->description = _("Must refer to an integer column");

    field_opt = G_define_standard_option(G_OPT_V_FIELD);
    field_opt->description = _("Format: layer number[/layer name]");
    field_opt->gisprompt = "new_layer,layer,layer";

    sep_opt = G_define_standard_option(G_OPT_F_SEP);
    sep_opt->description = _("Field separator for shell script style output");
    sep_opt->guisection = _("Print");
    sep_opt->answer = "=";

    print = G_define_flag();
    print->key = 'p';
    print->description = _("Print all map connection parameters and exit");
    print->guisection = _("Print");

    shell_print = G_define_flag();
    shell_print->key = 'g';
    shell_print->label = _("Print all map connection parameters in shell script style and exit");
    shell_print->description =
	_("Format: layer[/layer name] table key database driver");
    shell_print->guisection = _("Print");

    columns = G_define_flag();
    columns->key = 'c';
    columns->description =
	_("Print types/names of table columns for specified "
	  "layer and exit");
    columns->guisection = _("Print");

    overwrite = G_define_flag();
    overwrite->key = 'o';
    overwrite->description =
	_("Overwrite connection parameter for certain layer");

    delete = G_define_flag();
    delete->key = 'd';
    delete->description =
	_("Delete connection for certain layer (not the table)");

    G_gisinit(argv[0]);

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    /* The check must allow '.' in the name (schema.table) */
    /*
       if (dbtable->answer) {
       if ( db_legal_tablename(dbtable->answer) == DB_FAILED )
       G_fatal_error ( _("Table name '%s' is not valid."), dbtable->answer );
       }
     */

    /* set input vector map name */
    input = inopt->answer;

    if (field_opt->answer) {
	fieldname = strchr(field_opt->answer, '/');
	if (fieldname != NULL) {	/* field has name */
	    fieldname[0] = 0;
	    fieldname++;
	}
	
	field = atoi(field_opt->answer);
    }
    else {
	field = 1;
	fieldname = NULL;
    }
    G_debug(1, "layer number %d, layer name %s", field, fieldname);

    if (print->answer && shell_print->answer)
	G_fatal_error(_("Please choose only one print style"));

    if (print->answer || shell_print->answer || columns->answer) {
	Vect_set_open_level(1); /* no topology needed */
	Vect_open_old2(&Map, inopt->answer, "", field_opt->answer);
    }
    else {
	if (Vect_open_update_head(&Map, inopt->answer, G_mapset()) < 1)
	    G_fatal_error(_("Unable to modify vector map stored in other mapset"));
	Vect_hist_command(&Map);
    }

    if (print->answer || shell_print->answer || columns->answer) {
	num_dblinks = Vect_get_num_dblinks(&Map);
	if (num_dblinks <= 0)
	    G_fatal_error(_("Database connection for map <%s> is not defined in DB file"),
			  input);
	else {			/* num_dblinks > 0 */

	    if (print->answer || shell_print->answer) {
		if (!(shell_print->answer)) {
		    G_message(_("Vector map <%s> is connected by:"),
			      input);
		}
		for (i = 0; i < num_dblinks; i++) {
		    if ((fi = Vect_get_dblink(&Map, i)) == NULL)
			G_fatal_error(_("Database connection not defined"));

		    if (shell_print->answer) {
			const char *sep = sep_opt->answer;
			if (!sep)
			    sep = "=";
			fprintf(stdout, "layer%s%d\n", sep, fi->number);
			if (fi->name)
			    fprintf(stdout, "name%s%s\n", sep, fi->name);
			fprintf(stdout, "table%s%s\n", sep, fi->table);
			fprintf(stdout, "key%s%s\n", sep, fi->key);
			fprintf(stdout, "database%s%s\n", sep, fi->database);
			fprintf(stdout, "driver%s%s\n", sep, fi->driver);
		    }
		    else {
			if (fi->name) {
			    G_message(_("layer <%d/%s>"), fi->number, fi->name);
			}
			else {
			    G_message(_("layer <%d>"), fi->number);
			}
			G_message(_("table <%s>"), fi->table);
			G_message(_("in database <%s>"), fi->database);;
			G_message(_("through driver <%s>"), fi->driver);
			G_message(_("with key <%s>"), fi->key);
		    }
		}
	    }			/* end print */
	    else {		/* columns */

		if ((fi = Vect_get_field2(&Map, field_opt->answer)) == NULL)
		    G_fatal_error(_("Database connection not defined for layer <%s>"),
				  field_opt->answer);
		driver = db_start_driver(fi->driver);
		if (driver == NULL)
		    G_fatal_error(_("Unable to start driver <%s>"),
				  fi->driver);

		db_init_handle(&handle);
		db_set_handle(&handle, fi->database, NULL);
		if (db_open_database(driver, &handle) != DB_OK)
		    G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
				  fi->database, fi->driver);
		db_init_string(&table_name);
		db_set_string(&table_name, fi->table);
		if (db_describe_table(driver, &table_name, &table) != DB_OK)
		    G_fatal_error(_("Unable to describe table <%s>"),
				  fi->table);

		ncols = db_get_table_number_of_columns(table);
		for (col = 0; col < ncols; col++) {
		    fprintf(stdout, "%s|%s\n",
			    db_sqltype_name(db_get_column_sqltype
					    (db_get_table_column
					     (table, col))),
			    db_get_column_name(db_get_table_column
					       (table, col)));
		}

		db_close_database(driver);
		db_shutdown_driver(driver);
	    }
	}			/* end else num_dblinks */
    }				/* end print/columns */
    else {			/* define new dbln settings or delete */

	if (delete->answer) {
	    Vect_map_del_dblink(&Map, field);
	}
	else {
	    if (field_opt->answer && dbtable->answer && dbkey->answer
		&& dbdatabase->answer && dbdriver->answer) {
		fi = (struct field_info *)G_malloc(sizeof(struct field_info));
		fi->name = fieldname;
		fi->table = dbtable->answer;
		fi->key = dbkey->answer;
		fi->database = dbdatabase->answer;
		fi->driver = dbdriver->answer;

		ret = Vect_map_check_dblink(&Map, field, fieldname);
		G_debug(3, "Vect_map_check_dblink = %d", ret);
		if (ret == 1) {
		    /* field already defined */
		    if (!overwrite->answer)
			G_fatal_error(_("Use -o to overwrite existing link "
					"for layer <%d>"), field);
		    else {
			dbColumn *column;

			if (db_table_exists
			    (dbdriver->answer, dbdatabase->answer,
			     dbtable->answer) < 1)
			    G_fatal_error(_("Table <%s> does not exist in database <%s>"),
					  dbtable->answer,
					  dbdatabase->answer);

			driver = db_start_driver_open_database(fi->driver,
							       Vect_subst_var
							       (fi->database,
								&Map));
			if (!driver)
			    G_fatal_error(_("Unable to start driver <%s>"),
					  dbdriver->answer);

			db_get_column(driver, dbtable->answer, dbkey->answer,
				      &column);
			if (!column)
			    G_fatal_error(_("Missing column <%s> in table <%s>"),
					  dbkey->answer, dbtable->answer);

			if (db_column_Ctype
			    (driver, dbtable->answer,
			     dbkey->answer) != DB_C_TYPE_INT)
			    G_fatal_error(_("Data type of key column must be integer"));

			ret = Vect_map_del_dblink(&Map, field);
			if (Vect_map_add_dblink(&Map, field,
						fi->name, fi->table, fi->key,
						fi->database,
						fi->driver) == 0) {
			    G_important_message(_("The table <%s> is now part of vector map <%s> "
						 "and may be deleted "
						 "or overwritten by GRASS modules"),
						dbtable->answer, input);
			}
			db_close_database_shutdown_driver(driver);
		    }
		}
		else {		/* field not yet defined, add new field */
		    if (db_table_exists
			(dbdriver->answer, dbdatabase->answer,
			 dbtable->answer) < 1)
			G_warning(_("Table <%s> does not exist in database <%s>"),
				  dbtable->answer, dbdatabase->answer);

		    if (Vect_map_add_dblink(&Map, field,
					    fi->name, fi->table, fi->key,
					    fi->database, fi->driver) == 0) {
			G_important_message(_("The table <%s> is now part of vector map <%s> "
					     "and may be deleted "
					     "or overwritten by GRASS modules"),
					    dbtable->answer, input);

			driver = db_start_driver_open_database(fi->driver,
							       Vect_subst_var
							       (fi->database,
								&Map));

			if (!driver)
			    G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
					  fi->database, fi->driver);

			if (db_create_index2(driver, fi->table, fi->key) !=
			    DB_OK)
			    G_warning(_("Cannot create index"));

			if (db_grant_on_table
			    (driver, fi->table, DB_PRIV_SELECT,
			     DB_GROUP | DB_PUBLIC) != DB_OK)
			    G_warning(_("Cannot grant privileges on table %s"),
				      fi->table);

			G_important_message(_("Select privileges were granted on the table"));

			db_close_database_shutdown_driver(driver);
		    }
		}
	    }
	    else		/* incomplete parameters given */
		G_fatal_error(_("For defining a new connection you have "
				"to specify these parameters: "
				"driver, database, table [, key [, layer]]"));
	}
    }				/* end define new dbln settings */

    Vect_close(&Map);

    exit(EXIT_SUCCESS);
}

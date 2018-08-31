/****************************************************************************
 *
 * MODULE:       test.dbmi_base.lib
 *
 * AUTHOR(S):    Original author
 *               Soeren Gebbert soerengebbert <at> googlemail <dot> com
 * 		 2010 Braunschweig, Germany
 *
 * PURPOSE:      Unit and integration tests for the dbmi_base library
 *
 * COPYRIGHT:    (C) 2007 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *   	    	License (>=v2). Read the file COPYING that comes with GRASS
 *   	    	for details.
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/dbmi.h>
#include "test_dbmi_base_lib.h"

/* prototypes */
static int test_table(void);
static dbColumn * create_column(const char *name, const char *desctiption, int sqltype);

/* ************************************************************************* */
/* Performe the table unit tests  ****************************************** */
/* ************************************************************************* */
int unit_test_table(void)
{
	int sum = 0;

	G_message(_("\n++ Running table unit tests ++"));

	sum += test_table();

	if (sum > 0)
		G_warning(_("\n-- Table unit tests failure --"));
	else
		G_message(_("\n-- Table unit tests finished successfully --"));

	return sum;
}

/* *************************************************************** */
/* Test some functions of the dbTable functionality ************** */
/* *************************************************************** */
int test_table(void)
{
	int sum = 0;
        dbTable *t1 = db_alloc_table(0);
        dbTable *t2;

        db_init_table(t1);

        /*Append three columns of the different type*/
        db_append_table_column(t1, create_column("first", "first column", DB_SQL_TYPE_DOUBLE_PRECISION));
        db_append_table_column(t1, create_column("second", "second column", DB_SQL_TYPE_REAL));
        db_append_table_column(t1, create_column("third", "third column", DB_SQL_TYPE_DECIMAL));

        if(t1->numColumns != 3) {
            G_warning("Error appending columns");
            sum++;
        }

        db_set_table_delete_priv_granted(t1);
        db_set_table_description(t1, "test table");
        db_set_table_insert_priv_granted(t1);
        db_set_table_name(t1, "test");
        db_set_table_select_priv_granted(t1);
        db_set_table_update_priv_granted(t1);

        G_message("##### First table:\n");
        db_print_table_definition(stdout, t1);

        /*Clone the table*/
        t2 = db_clone_table(t1);

        G_message("##### Second table:\n");
        db_print_table_definition(stdout, t2);

        /*Compare the tables*/
        if(strcmp(t1->description.string, t2->description.string) != 0) {
            G_warning("Error copying description");
            sum++;
        }
        if(strcmp(t1->tableName.string, t2->tableName.string) != 0) {
            G_warning("Error copying tableName");
            sum++;
        }
        if(t1->numColumns != t2->numColumns) {
            G_warning("Error copying table numColumns");
            sum++;
        }
        if(t1->priv_delete != t2->priv_delete) {
            G_warning("Error copying privileg delete");
            sum++;
        }
        if(t1->priv_insert != t2->priv_insert) {
            G_warning("Error copying privileg insert");
            sum++;
        }
        /*We check only the column names*/
        if(strcmp(t1->columns[0].columnName.string, t2->columns[0].columnName.string) != 0) {
            G_warning("Error copying first column");
            sum++;
        }
        if(strcmp(t1->columns[1].columnName.string, t2->columns[1].columnName.string) != 0) {
            G_warning("Error copying second column");
            sum++;
        }
        if(strcmp(t1->columns[2].columnName.string, t2->columns[2].columnName.string) != 0) {
            G_warning("Error copying third column");
            sum++;
        }

	/*Now test the set column and get column by name functions*/
	db_set_table_column(t2, 0, create_column("new_first", "new first column", DB_SQL_TYPE_DOUBLE_PRECISION));
	db_set_table_column(t2, 1, create_column("new_second", "new second column", DB_SQL_TYPE_REAL));
	db_set_table_column(t2, 2, create_column("new_third", "new third column", DB_SQL_TYPE_INTEGER));

        G_message("##### Second table new columns:\n");
        db_print_table_definition(stdout, t2);

	dbColumn *c1 = db_get_table_column_by_name(t2, "new_first");
	dbColumn *c2 = db_get_table_column_by_name(t2, "new_second");
	dbColumn *c3 = db_get_table_column_by_name(t2, "new_third");


        /*We check the column names*/
        if(strcmp(c1->columnName.string, "new_first") != 0) {
            G_warning("Error set table or get table by name first column");
            sum++;
        }
        if(strcmp(c2->columnName.string, "new_second") != 0) {
            G_warning("Error set table or get table by name second column");
            sum++;
        }
        if(strcmp(c3->columnName.string, "new_third") != 0) {
            G_warning("Error set table or get table by name third column");
            sum++;
        }

	return sum;
}

/* *************************************************************** */
/* Simple table creation ***************************************** */
/* *************************************************************** */
dbColumn *create_column(const char *name, const char *desctiption, int sqltype)
{
        dbColumn *column = (dbColumn*)db_calloc(sizeof(dbColumn), 1);
        db_init_column(column);

        /*Write some initial values*/
        db_set_value_double(&column->defaultValue, 0.5);
        db_set_value_double(&column->value, 10.5);

        db_set_column_description(column, desctiption);
        db_set_column_host_type(column, 1);
        db_set_column_length(column, 8);
        db_set_column_name(column, name);
        db_set_column_null_allowed(column);
        db_set_column_precision(column, 20);
        db_set_column_scale(column, 1);
        db_set_column_select_priv_granted(column);
        db_set_column_sqltype(column, sqltype);
        db_set_column_select_priv_granted(column);
        db_set_column_update_priv_granted(column);
        db_set_column_use_default_value(column);

        return column;
}


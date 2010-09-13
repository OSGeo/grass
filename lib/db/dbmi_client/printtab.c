/*!
 * \file db/dbmi_client/printtab.c
 * 
 * \brief DBMI Library (client) - print table description info
 *
 * (C) 1999-2008 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public
 * License (>=v2). Read the file COPYING that comes with GRASS
 * for details.
 *
 * \author Joel Jones (CERL/UIUC), Radim Blazek
 */

#include <string.h>
#include <grass/dbmi.h>

static void print_priv(FILE * fd, char *label, int priv);

/*!
  \brief Print table definition info

  \param fd file descriptor
  \param table table info
*/
void db_print_table_definition(FILE * fd, dbTable * table)
{
    int ncols, col;
    dbColumn *column;

    fprintf(fd, "table:%s\n", db_get_table_name(table));
    fprintf(fd, "description:%s\n", db_get_table_description(table));
    print_priv(fd, "insert", db_get_table_insert_priv(table));
    print_priv(fd, "delete", db_get_table_delete_priv(table));

    ncols = db_get_table_number_of_columns(table);
    fprintf(fd, "ncols:%d\n", ncols);
    for (col = 0; col < ncols; col++) {
	column = db_get_table_column(table, col);
	fprintf(fd, "\n");
	db_print_column_definition(fd, column);
    }
}

/*!
  \brief Print column definition info

  \param fd file descriptor
  \param column column info
*/
void db_print_column_definition(FILE * fd, dbColumn * column)
{
    dbString value_string;

    fprintf(fd, "column:%s\n", db_get_column_name(column));
    fprintf(fd, "description:%s\n", db_get_column_description(column));
    fprintf(fd, "type:%s\n", db_sqltype_name(db_get_column_sqltype(column)));
    fprintf(fd, "len:%d\n", db_get_column_length(column));
    fprintf(fd, "scale:%d\n", db_get_column_scale(column));
    fprintf(fd, "precision:%d\n", db_get_column_precision(column));
    fprintf(fd, "default:");
    if (db_test_column_has_default_value(column)) {
	db_init_string(&value_string);
	db_convert_column_default_value_to_string(column, &value_string);
	fprintf(fd, "%s", db_get_string(&value_string));
    }
    fprintf(fd, "\n");
    fprintf(fd, "nullok:%s\n",
	    db_test_column_null_allowed(column) ? "yes" : "no");
    print_priv(fd, "select", db_get_column_select_priv(column));
    print_priv(fd, "update", db_get_column_update_priv(column));
}

static void print_priv(FILE * fd, char *label, int priv)
{
    fprintf(fd, "%s:", label);
    switch (priv) {
    case DB_GRANTED:
	fprintf(fd, "yes");
	break;
    case DB_NOT_GRANTED:
	fprintf(fd, "no");
	break;
    default:
	fprintf(fd, "?");
	break;
    }
    fprintf(fd, "\n");
}

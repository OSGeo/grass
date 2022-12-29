#include <grass/gis.h>
#include <grass/dbmi.h>
#include "local_proto.h"
#include <grass/glocale.h>

int print_table_definition(dbDriver * driver, dbTable * table)
{
    int ncols, col, nrows;
    dbColumn *column;
    char buf[1024];
    dbString stmt;

    fprintf(stdout, "table:%s\n", db_get_table_name(table));
    fprintf(stdout, "description:%s\n", db_get_table_description(table));
    print_priv("insert", db_get_table_insert_priv(table));
    print_priv("delete", db_get_table_delete_priv(table));

    ncols = db_get_table_number_of_columns(table);

    db_init_string(&stmt);
    sprintf(buf, "select * from %s", db_get_table_name(table));
    db_set_string(&stmt, buf);
    nrows = db_get_table_number_of_rows(driver, &stmt);
    fprintf(stdout, "ncols:%d\n", ncols);
    fprintf(stdout, "nrows:%d\n", nrows);
    for (col = 0; col < ncols; col++) {
	column = db_get_table_column(table, col);
	fprintf(stdout, "\n");
	print_column_definition(column);
    }

    return 0;
}

int print_column_definition(dbColumn * column)
{
    dbString value_string;

    fprintf(stdout, "column:%s\n", db_get_column_name(column));
    fprintf(stdout, "description:%s\n", db_get_column_description(column));
    fprintf(stdout, "type:%s\n",
	    db_sqltype_name(db_get_column_sqltype(column)));
    fprintf(stdout, "len:%d\n", db_get_column_length(column));
    fprintf(stdout, "scale:%d\n", db_get_column_scale(column));
    fprintf(stdout, "precision:%d\n", db_get_column_precision(column));
    fprintf(stdout, "default:");
    if (db_test_column_has_default_value(column)) {
	db_init_string(&value_string);
	db_convert_column_default_value_to_string(column, &value_string);
	fprintf(stdout, "%s", db_get_string(&value_string));
    }
    fprintf(stdout, "\n");
    fprintf(stdout, "nullok:%s\n",
	    db_test_column_null_allowed(column) ? "yes" : "no");
    print_priv("select", db_get_column_select_priv(column));
    print_priv("update", db_get_column_update_priv(column));

    return 0;
}

int print_priv(char *label, int priv)
{
    fprintf(stdout, "%s:", label);
    switch (priv) {
    case DB_GRANTED:
	fprintf(stdout, "yes");
	break;
    case DB_NOT_GRANTED:
	fprintf(stdout, "no");
	break;
    default:
	fprintf(stdout, "?");
	break;
    }
    fprintf(stdout, "\n");

    return 0;
}

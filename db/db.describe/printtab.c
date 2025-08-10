#include <grass/gis.h>
#include <grass/dbmi.h>
#include "local_proto.h"
#include <grass/parson.h>

int print_table_definition(dbDriver *driver, dbTable *table,
                           enum OutputFormat format, JSON_Object *root_object,
                           JSON_Array *cols_array)
{
    int ncols, col, nrows;
    dbColumn *column;
    char buf[1024];
    dbString stmt;

    switch (format) {
    case PLAIN:
        fprintf(stdout, "table:%s\n", db_get_table_name(table));
        fprintf(stdout, "description:%s\n", db_get_table_description(table));
        break;
    case JSON:
        json_object_set_string(root_object, "table", db_get_table_name(table));
        json_object_set_string(root_object, "description",
                               db_get_table_description(table));
        break;
    }

    print_priv("insert", db_get_table_insert_priv(table), format, root_object);
    print_priv("delete", db_get_table_delete_priv(table), format, root_object);

    ncols = db_get_table_number_of_columns(table);

    db_init_string(&stmt);
    snprintf(buf, sizeof(buf), "select * from %s", db_get_table_name(table));
    db_set_string(&stmt, buf);
    nrows = db_get_table_number_of_rows(driver, &stmt);

    switch (format) {
    case PLAIN:
        fprintf(stdout, "ncols:%d\n", ncols);
        fprintf(stdout, "nrows:%d\n", nrows);
        break;
    case JSON:
        json_object_set_number(root_object, "ncols", ncols);
        json_object_set_number(root_object, "nrows", nrows);
        break;
    }

    for (col = 0; col < ncols; col++) {
        column = db_get_table_column(table, col);
        print_column_definition(column, col + 1, format, cols_array);
    }

    return 0;
}

int print_column_definition(dbColumn *column, int position,
                            enum OutputFormat format, JSON_Array *cols_array)
{
    JSON_Object *col_object = NULL;
    JSON_Value *col_value = NULL;

    dbString value_string;

    switch (format) {
    case PLAIN:
        fprintf(stdout, "\n");
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
        break;
    case JSON:
        col_value = json_value_init_object();
        col_object = json_object(col_value);
        json_object_set_number(col_object, "position", position);
        json_object_set_string(col_object, "column",
                               db_get_column_name(column));
        json_object_set_string(col_object, "description",
                               db_get_column_description(column));
        json_object_set_string(col_object, "type",
                               db_sqltype_name(db_get_column_sqltype(column)));
        json_object_set_number(col_object, "length",
                               db_get_column_length(column));
        json_object_set_number(col_object, "scale",
                               db_get_column_scale(column));
        json_object_set_number(col_object, "precision",
                               db_get_column_precision(column));
        if (db_test_column_has_default_value(column)) {
            db_init_string(&value_string);
            db_convert_column_default_value_to_string(column, &value_string);
            json_object_set_string(col_object, "default",
                                   db_get_string(&value_string));
        }
        else {
            json_object_set_null(col_object, "default");
        }
        json_object_set_boolean(col_object, "nullok",
                                db_test_column_null_allowed(column));
        break;
    }
    print_priv("select", db_get_column_select_priv(column), format, col_object);
    print_priv("update", db_get_column_update_priv(column), format, col_object);
    if (format == JSON) {
        json_array_append_value(cols_array, col_value);
    }

    return 0;
}

int print_priv(char *label, int priv, enum OutputFormat format,
               JSON_Object *root_object)
{
    switch (format) {
    case PLAIN:
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
        break;
    case JSON:
        switch (priv) {
        case DB_GRANTED:
            json_object_set_boolean(root_object, label, 1);
            break;
        case DB_NOT_GRANTED:
            json_object_set_boolean(root_object, label, 0);
            break;
        default:
            json_object_set_null(root_object, label);
            break;
        }
        break;
    }

    return 0;
}

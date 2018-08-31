#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

/* get height for DB
   returns 0 on success -1 on failure
*/
int get_height(const struct field_info *Fi, const char *hcolumn,
               dbDriver *driver, int cat, double *height)
{
    int more;
    double objheight;
    char query[DB_SQL_MAX];
    
    dbString sql;
    dbCursor cursor;
    dbTable *table;
    dbColumn *column;
    dbValue *value;

    db_init_string(&sql);
    sprintf(query, "SELECT %s FROM %s WHERE %s = %d",
            hcolumn, Fi->table, Fi->key, cat);
    G_debug(3, "SQL: %s", query);
    db_set_string(&sql, query);
    if (db_open_select_cursor(driver, &sql, &cursor, DB_SEQUENTIAL) != DB_OK)
        G_fatal_error(_("Unable to select attributes category %d"),
                      cat);
    table = db_get_cursor_table(&cursor);
    column = db_get_table_column(table, 0);	/* first column */
    
    if (db_fetch(&cursor, DB_NEXT, &more) != DB_OK)
        return -1;
    
    value = db_get_column_value(column);
    G_debug(3, "column_host_type: %d", db_get_column_host_type(column));
    objheight = db_get_value_as_double(value, DB_C_TYPE_DOUBLE);
    G_debug(3, "height from DB: %f", objheight);
    
    *height = objheight;
    
    return 0;
}

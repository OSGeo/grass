#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

/* Check columns 

   1) FID column - must be integer

   @todo check for unique values

   2) Geometry column

   calls G_fatal_error() on error
*/
void check_columns(const struct Map_info *Map, const char *layer,
		   const char *fid_column, const char *geom_column)
{
    struct field_info *fi;
    
    dbDriver *driver;
    dbTable *table;
    dbColumn *column;
    dbString stmt;
    
    fi = Vect_get_field2(Map, layer);
    if (!fi) {
	G_verbose_message(_("No database connection for layer <%s>"), layer);
	return;
    }
    
    /* open connection */
    driver = db_start_driver_open_database(fi->driver, fi->database);
    if (!driver)
	G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
		      fi->database, fi->driver);
    
    db_init_string(&stmt);
    db_set_string(&stmt, fi->table);
    if (db_describe_table(driver, &stmt, &table) != DB_OK) {
	db_close_database_shutdown_driver(driver);
	G_fatal_error(_("Unable to describe table <%s>"), fi->table);
    }
    
    /* check for fid column in attribute table */
    column = db_get_table_column_by_name(table, fid_column);
    if (column) {
	int ctype;
	ctype = db_sqltype_to_Ctype(db_get_column_sqltype(column));
	if (ctype != DB_C_TYPE_INT)
	    G_fatal_error(_("Invalid FID column (%s). FID column must be integer. "
			    "Please specify different FID column by 'options=\"FID=<name>\"'."),
			  fid_column);
    }
    
    /* check if geometry column already exists in the attribute table */
    if (db_get_table_column_by_name(table, geom_column))
	G_fatal_error(_("Column (%s) already exists in the table. "
			"Please specify different geometry column by 'options=\"GEOMETRY_NAME=<name>\"'."),
		      geom_column);
    
    db_close_database_shutdown_driver(driver);
}

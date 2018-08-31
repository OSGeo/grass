#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/raster.h>
#include <grass/glocale.h>

void write_rgb_values(const struct Map_info *Map, int layer, const char *column_name,
		      struct Colors *colors)
{
    int ctype, nrec, i;
    int red, grn, blu;
    int *pval;
    char buf[1024];
    struct field_info *fi;
    dbDriver *driver;
    dbString stmt;

    fi = Vect_get_field(Map, layer);
    if (!fi)
	G_fatal_error(_("Database connection not defined for layer %d"),
		      layer);

    driver = db_start_driver_open_database(fi->driver, fi->database);
    if (!driver)
	G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
		      fi->database, fi->driver);
    db_set_error_handler_driver(driver);

    db_init_string(&stmt);
    
    ctype = db_column_Ctype(driver, fi->table, column_name);
    if (ctype == -1) {
	sprintf(buf, "ALTER TABLE \"%s\" ADD COLUMN \"%s\" VARCHAR(11)",
		fi->table, column_name);
	db_set_string(&stmt, buf);
	if (db_execute_immediate(driver, &stmt) != DB_OK)
	    G_fatal_error(_("Unable to add column <%s> to table <%s>"),
			  column_name, fi->table);

	/*
	  db_add_column needs to be implemented for DB drivers first...
	  
	  dbString table;
	  dbColumn column;
	  
	  db_init_column(&column);
	  db_set_column_name(&column, column_name);
	  db_set_column_sqltype(&column, DB_SQL_TYPE_CHARACTER);
	  db_set_column_null_allowed(&column);
	  db_set_column_length(&column, 11);
	  
	  db_init_string(&table);
	  db_set_string(&table, fi->table);
	  if (db_add_column(driver, &table, &column) != DB_OK)
	  G_fatal_error(_("Unable to add column <%s> to table <%s>"),
	  column_name, fi->table);
	  db_free_column(&column);
	*/

	G_important_message(_("Column <%s> added to table <%s>"),
			     column_name, fi->table);
    }
    else if (ctype != DB_C_TYPE_STRING)
	G_fatal_error(_("Data type of column <%s> must be char"), column_name);
	
    nrec = db_select_int(driver, fi->table, fi->key, NULL, &pval);
    if (nrec < 1) {
	G_warning(_("No categories found"));
	return;
    }
    
    db_begin_transaction(driver);
    
    for (i = 0; i < nrec; i++) {
	G_percent(i, nrec, 2);
	if (Rast_get_c_color((const CELL *) &(pval[i]), &red, &grn, &blu,
			     colors) == 0)
	    G_warning(_("No color value defined for category %d"), pval[i]);

	sprintf(buf, "UPDATE %s SET \"%s\"='%d:%d:%d' WHERE %s=%d", fi->table,
                column_name, red, grn, blu, fi->key, pval[i]);
	G_debug(3, "\tSQL: %s", buf);

	db_set_string(&stmt, buf);
	if (db_execute_immediate(driver, &stmt) != DB_OK)
	    G_fatal_error(_("Unable to update RGB values"));
    }
    G_percent(1, 1, 1);
    
    db_commit_transaction(driver);
    
    db_close_database_shutdown_driver(driver);
}

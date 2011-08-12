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

    ctype = db_column_Ctype(driver, fi->table, column_name);
    if (ctype == -1)
	G_fatal_error(_("Column <%s> not found in table <%s>"),
		      column_name, fi->table);
    if (ctype != DB_C_TYPE_STRING)
	G_fatal_error(_("Data type of column <%s> must be char"), column_name);
	
    nrec = db_select_int(driver, fi->table, fi->key, NULL, &pval);
    if (nrec < 1) {
	G_warning(_("No categories found"));
	return;
    }
    
    db_init_string(&stmt);
    db_begin_transaction(driver);
    
    for (i = 0; i < nrec; i++) {
	if (Rast_get_c_color((const CELL *) &(pval[i]), &red, &grn, &blu,
			     colors) == 0)
	    G_warning(_("No color value defined for category %d"), pval[i]);

	sprintf(buf, "UPDATE %s SET %s='%d:%d:%d' WHERE %s=%d", fi->table,
		   column_name, red, grn, blu, fi->key, pval[i]);
	G_debug(3, "\tSQL: %s", buf);

	db_set_string(&stmt, buf);
	if (db_execute_immediate(driver, &stmt) != DB_OK)
	    G_fatal_error(_("Unable to update RGB values"));
    }
    
    db_commit_transaction(driver);
    
    db_close_database(driver);
}

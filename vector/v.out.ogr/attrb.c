#include <grass/glocale.h>

#include "local_proto.h"

int mk_att(int cat, struct field_info *Fi, dbDriver *driver, int ncol,
	   int *colctype, const char **colname, int doatt, int nocat,
	   OGRFeatureH Ogr_feature, int *noatt)
{
    int j, ogrfieldnum;
    int more;
    dbTable *Table;
    static int first = 1;
    static dbString dbstring;
    dbColumn *Column;
    dbValue *Value;
    char buf[SQL_BUFFER_SIZE];
    dbCursor cursor;


    G_debug(2, "mk_att() cat = %d, doatt = %d", cat, doatt);

    /* init constants */
    if (first) {
	db_init_string(&dbstring);
	first = 0;
    }

    /* Attributes */
    /* Reset */
    if (!doatt) {
	ogrfieldnum = OGR_F_GetFieldIndex(Ogr_feature, GV_KEY_COLUMN);
        if (ogrfieldnum > -1)
            OGR_F_UnsetField(Ogr_feature, ogrfieldnum);
	/* doatt reset moved into have cat loop as the table needs to be
	   open to know the OGR field ID. Hopefully this has no ill consequences */
    }

    /* Read & set attributes */
    if (cat >= 0) {		/* Line with category */
	if (doatt) {
	    /* Fetch all attribute records for cat <cat> */ 
	    /* opening and closing the cursor is slow, 
	     * but the cursor really needs to be opened for each cat separately */
	    sprintf(buf, "SELECT * FROM %s WHERE %s = %d", Fi->table, Fi->key, cat);
	    G_debug(2, "SQL: %s", buf);
	    db_set_string(&dbstring, buf);
	    if (db_open_select_cursor (driver, &dbstring, &cursor, DB_SEQUENTIAL) != DB_OK) {
		    G_fatal_error(_("Cannot select attributes for cat = %d"),
		  cat);
	    }

	    if (db_fetch(&cursor, DB_NEXT, &more) != DB_OK)
		G_fatal_error(_("Unable to fetch data from table"));

	    if (!more) {
		/* G_warning ("No database record for cat = %d", cat); */
		/* Set at least key column to category */
		if (!nocat) {
		    ogrfieldnum =
			OGR_F_GetFieldIndex(Ogr_feature, Fi->key);
		    OGR_F_SetFieldInteger(Ogr_feature, ogrfieldnum, cat);
		    (*noatt)++;
		}
		else {
		    G_fatal_error(_("No database record for cat = %d and export of 'cat' disabled"),
				  cat);
		}
	    }
	    else {
		Table = db_get_cursor_table(&cursor);
		for (j = 0; j < ncol; j++) {
		    Column = db_get_table_column(Table, j);
		    Value = db_get_column_value(Column);
		    db_convert_column_value_to_string(Column, &dbstring);	/* for debug only */
		    G_debug(2, "col %d : val = %s", j,
			    db_get_string(&dbstring));

		    G_debug(2, "  colctype = %d", colctype[j]);

		    if (nocat && strcmp(Fi->key, colname[j]) == 0)
			continue;

		    ogrfieldnum = OGR_F_GetFieldIndex(Ogr_feature,
						      colname[j]);
		    G_debug(2, "  column = %s -> fieldnum = %d",
			    colname[j], ogrfieldnum);

		    if (ogrfieldnum < 0) {
			G_debug(4, "Could not get OGR field number for column %s",
				                         colname[j]);
			continue;
		    }

		    /* Reset */
		    if ((nocat && strcmp(Fi->key, colname[j]) == 0) == 0) {
			/* if this is 'cat', then execute the following only if the '-s' flag was NOT given*/
#if GDAL_VERSION_NUM >= 2020000
			OGR_F_SetFieldNull(Ogr_feature, ogrfieldnum);
#else
			OGR_F_UnsetField(Ogr_feature, ogrfieldnum);
#endif
		    }

		    /* prevent writing NULL values */
		    if (!db_test_value_isnull(Value)) {
			if ((nocat && strcmp(Fi->key, colname[j]) == 0) == 0) {
			/* if this is 'cat', then execute the following only if the '-s' flag was NOT given*/

			    switch (colctype[j]) {
			    case DB_C_TYPE_INT:
				OGR_F_SetFieldInteger(Ogr_feature,
						      ogrfieldnum,
						      db_get_value_int
						      (Value));
				break;
			    case DB_C_TYPE_DOUBLE:
				OGR_F_SetFieldDouble(Ogr_feature, ogrfieldnum,
						     db_get_value_double
						     (Value));
				break;
			    case DB_C_TYPE_STRING:
				OGR_F_SetFieldString(Ogr_feature, ogrfieldnum,
						     db_get_value_string
						     (Value));
				break;
			    case DB_C_TYPE_DATETIME:
				db_convert_column_value_to_string(Column,
								  &dbstring);
				OGR_F_SetFieldString(Ogr_feature, ogrfieldnum,
						     db_get_string
						     (&dbstring));
				break;
			    }
			}
		    }
#if GDAL_VERSION_NUM >= 2020000
		    else
			OGR_F_SetFieldNull(Ogr_feature, ogrfieldnum);
#endif
		}
	    }
	    db_close_cursor(&cursor);
	}
	else {			/* Use cat only */
	    ogrfieldnum = OGR_F_GetFieldIndex(Ogr_feature, GV_KEY_COLUMN);
	    OGR_F_SetFieldInteger(Ogr_feature, ogrfieldnum, cat);
	}
    }
    /* else G_warning ("Line without cat of layer %d", field); */

    /*
    db_free_string(&dbstring);
    */

    return 1;
}

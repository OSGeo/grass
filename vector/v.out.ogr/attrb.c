#include <grass/glocale.h>

#include "local_proto.h"

int mk_att(int cat, struct field_info *Fi, dbDriver *Driver, int ncol,
	   int doatt, int nocat, OGRFeatureH Ogr_feature, int *noatt,
	   int *fout, dbCursor cursor)
{
    int j, ogrfieldnum;
    int colsqltype, colctype, more;
    dbTable *Table;
    dbString dbstring;
    dbColumn *Column;
    dbValue *Value;

    G_debug(2, "mk_att() cat = %d, doatt = %d", cat, doatt);
    db_init_string(&dbstring);

    /* Attributes */
    /* Reset */
    if (!doatt) {
	ogrfieldnum = OGR_F_GetFieldIndex(Ogr_feature, "cat");
	OGR_F_UnsetField(Ogr_feature, ogrfieldnum);
	/* doatt reset moved into have cat loop as the table needs to be
	   open to know the OGR field ID. Hopefully this has no ill consequences */
    }

    /* Read & set attributes */
    if (cat >= 0) {		/* Line with category */
	if (doatt) {
		if (db_fetch(&cursor, DB_NEXT, &more) != DB_OK)
		    G_fatal_error(_("Unable to fetch data from table"));
		if (!more) {
		    /* start from the beginning in case multiple grass vector features
		     * share the same category */
		    if (db_fetch(&cursor, DB_NEXT, &more) != DB_OK)
			G_fatal_error(_("Unable to fetch data from table"));
		}

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

			colsqltype = db_get_column_sqltype(Column);
			colctype = db_sqltype_to_Ctype(colsqltype);
			G_debug(2, "  colctype = %d", colctype);

			ogrfieldnum = OGR_F_GetFieldIndex(Ogr_feature,
							  db_get_column_name
							  (Column));
			G_debug(2, "  column = %s -> fieldnum = %d",
				db_get_column_name(Column), ogrfieldnum);

			/* Reset */
			if ( ( ( nocat ) && (strcmp(Fi->key, db_get_column_name(Column)) == 0) ) == 0 ) {
				/* if this is 'cat', then execute the following only if the '-s' flag was NOT given*/
				OGR_F_UnsetField(Ogr_feature, ogrfieldnum);
			}

			/* prevent writing NULL values */
			if (!db_test_value_isnull(Value)) {
				if ( ( (nocat) && (strcmp(Fi->key, db_get_column_name(Column)) == 0) ) == 0 ) {
				/* if this is 'cat', then execute the following only if the '-s' flag was NOT given*/
			    switch (colctype) {
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
		    }
		}
	}
	else {			/* Use cat only */
	    ogrfieldnum = OGR_F_GetFieldIndex(Ogr_feature, "cat");
	    OGR_F_SetFieldInteger(Ogr_feature, ogrfieldnum, cat);
	}
    }
    else {
	/* G_warning ("Line without cat of layer %d", field); */
	nocat++;
    }
    (*fout)++;

    db_free_string(&dbstring);

    return 1;
}

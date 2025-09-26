#include <grass/glocale.h>

#include "local_proto.h"

int mk_att_fast(int cat, struct field_info *Fi, int ncol, int *colctype,
                const char **colname, int doatt, int nocat,
                OGRFeatureH Ogr_feature, int *noatt, dbCursor *cursor,
                int *more, int *db_cat, int key_col_index)
{
    int j, ogrfieldnum;
    dbTable *Table;
    static int first = 1;
    static dbString dbstring;
    dbColumn *Column;
    dbValue *Value;

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
           open to know the OGR field ID. Hopefully this has no ill consequences
         */
    }

    /* Read & set attributes */
    if (cat >= 0) { /* Line with category */
        if (doatt) {
            /* get current entries from cursor,
             * check cat value in attributes */

            Table = db_get_cursor_table(cursor);
            while (*more && cat > *db_cat) {
                Column = db_get_table_column(Table, key_col_index);
                Value = db_get_column_value(Column);

                /* yes, the key column is sometimes of type double */
                switch (colctype[key_col_index]) {
                case DB_C_TYPE_INT:
                    *db_cat = db_get_value_int(Value);
                    break;
                case DB_C_TYPE_DOUBLE:
                    *db_cat = (int)db_get_value_double(Value);
                    break;
                }

                G_debug(2, "found db_cat %d for cat %d in column %s", *db_cat,
                        cat, db_get_column_name(Column));

                if (cat > *db_cat) {
                    if (db_fetch(cursor, DB_NEXT, more) != DB_OK) {
                        G_fatal_error(_("Unable to fetch data from table"));
                    }
                }
            }

            if (!(*more) || cat != *db_cat) {
                G_debug(1, "No database record for cat = %d", cat);
                /* Set at least key column to category */
                if (!nocat) {
                    ogrfieldnum = OGR_F_GetFieldIndex(Ogr_feature, Fi->key);
                    OGR_F_SetFieldInteger(Ogr_feature, ogrfieldnum, cat);
                    (*noatt)++;
                }
                else {
                    G_fatal_error(_("No database record for cat = %d and "
                                    "export of 'cat' disabled"),
                                  cat);
                }
            }
            else {
                for (j = 0; j < ncol; j++) {
                    Column = db_get_table_column(Table, j);
                    Value = db_get_column_value(Column);
                    db_convert_column_value_to_string(
                        Column, &dbstring); /* for debug only */
                    G_debug(2, "col %d : val = %s", j,
                            db_get_string(&dbstring));

                    G_debug(2, "  colctype = %d", colctype[j]);

                    if (nocat && strcmp(Fi->key, colname[j]) == 0)
                        continue;

                    ogrfieldnum = OGR_F_GetFieldIndex(Ogr_feature, colname[j]);
                    G_debug(2, "  column = %s -> fieldnum = %d", colname[j],
                            ogrfieldnum);

                    if (ogrfieldnum < 0) {
                        G_debug(4,
                                "Could not get OGR field number for column %s",
                                colname[j]);
                        continue;
                    }

                    /* Reset */
                    if ((nocat && strcmp(Fi->key, colname[j]) == 0) == 0) {
                        /* if this is 'cat', then execute the following only if
                         * the '-s' flag was NOT given */
                        OGR_F_SetFieldNull(Ogr_feature, ogrfieldnum);
                    }

                    /* prevent writing NULL values */
                    if (!db_test_value_isnull(Value)) {
                        if ((nocat && strcmp(Fi->key, colname[j]) == 0) == 0) {
                            /* if this is 'cat', then execute the following only
                             * if the '-s' flag was NOT given */

                            switch (colctype[j]) {
                            case DB_C_TYPE_INT:
                                OGR_F_SetFieldInteger(Ogr_feature, ogrfieldnum,
                                                      db_get_value_int(Value));
                                break;
                            case DB_C_TYPE_DOUBLE:
                                OGR_F_SetFieldDouble(
                                    Ogr_feature, ogrfieldnum,
                                    db_get_value_double(Value));
                                break;
                            case DB_C_TYPE_STRING:
                                OGR_F_SetFieldString(
                                    Ogr_feature, ogrfieldnum,
                                    db_get_value_string(Value));
                                break;
                            case DB_C_TYPE_DATETIME:
                                db_convert_column_value_to_string(Column,
                                                                  &dbstring);
                                OGR_F_SetFieldString(Ogr_feature, ogrfieldnum,
                                                     db_get_string(&dbstring));
                                break;
                            }
                        }
                    }
                    else
                        OGR_F_SetFieldNull(Ogr_feature, ogrfieldnum);
                }
            }
        }
        else { /* Use cat only */
            ogrfieldnum = OGR_F_GetFieldIndex(Ogr_feature, GV_KEY_COLUMN);
            OGR_F_SetFieldInteger(Ogr_feature, ogrfieldnum, cat);
        }
    }

    return 1;
}

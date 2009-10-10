/*!
   \file vector/Vlib/write_ogr.c

   \brief Vector library - write vector feature (OGR format)

   Higher level functions for reading/writing/manipulating vectors.

   Inspired by v.out.ogr's code.

   (C) 2009 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Martin Landa <landa.martin gmail.com> (2009)
 */

#include <grass/config.h>
#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

#ifdef HAVE_OGR
#include <ogr_api.h>

static int write_attributes(int, const struct field_info *,
			    OGRFeatureH);

/*!
  \brief Writes feature on level 1 (OGR format)

  \param Map pointer to Map_info structure
  \param type feature type
  \param points pointer to line_pnts structure (feature geometry) 
  \param cats pointer to line_cats structure (feature categories)
  
  \return 0 on success
  \return -1 on error
*/
int V1_write_line_ogr(struct Map_info *Map,
		      int type, const struct line_pnts *points, const struct line_cats *cats)
{
    int i, cat;

    struct field_info *Fi;
    
    OGRGeometryH Ogr_geometry;
    OGRFeatureH Ogr_feature;
    OGRFeatureDefnH Ogr_featuredefn;
    
    if (!Map->fInfo.ogr.layer) {
	G_warning(_("Unable to write feature, OGR layer not defined"));
	return -1;
    }

    /* determine feature's geometry */
    if (type & (GV_POINTS | GV_KERNEL)) {
	Ogr_geometry = OGR_G_CreateGeometry(wkbPoint);
    }
    else if (type & GV_LINES) {
	Ogr_geometry = OGR_G_CreateGeometry(wkbLineString);
    }
    else if (type & GV_FACE) {
	Ogr_geometry = OGR_G_CreateGeometry(wkbPolygon25D);
    }
    else {
	G_warning("V1_write_line_ogr(): %s (%d)", _("Unsupported feature type"), type);
	return -1;
    }

    G_debug(3, "V1_write_line_ogr(): type = %d", type);

    for (i = 0; i < points->n_points; i++) {
	OGR_G_AddPoint(Ogr_geometry, points->x[i], points->y[i],
		       points->z[i]);
    }
    
    G_debug(4, "   n_points = %d", points->n_points);

    Ogr_featuredefn = OGR_L_GetLayerDefn(Map->fInfo.ogr.layer);
    
    /* create feature & set geometry */
    Ogr_feature = OGR_F_Create(Ogr_featuredefn);
    OGR_F_SetGeometry(Ogr_feature, Ogr_geometry);

    /* write attributes */
    if (cats->n_cats > 0) {
	cat = cats->cat[0];
	if (cats->n_cats > 1) {
	    G_warning(_("Feature has more categories, using "
			"category %d (from layer %d)"),
		      cat, cats->field[0]);
	}
	
	Fi = Vect_get_field(Map, cats->field[0]);
	if (!Fi) {
	    G_fatal_error(_("Database connection not defined for layer %d"),
			  cats->field[0]);
	}
	write_attributes(cat, Fi, Ogr_feature);
    }
    else { /* no attributes */
	G_warning(_("Feature has no categories"));
    }
    
    /* write feature into layer */
    OGR_L_CreateFeature(Map->fInfo.ogr.layer, Ogr_feature);

    /* destroy */
    OGR_G_DestroyGeometry(Ogr_geometry);
    OGR_F_Destroy(Ogr_feature);
    
    return 0;
}

int write_attributes(int cat, const struct field_info *Fi,
		     OGRFeatureH Ogr_feature)
{
    int j, ogrfieldnum;
    char buf[2000];
    int ncol, colsqltype, colctype, more;
    dbDriver *Driver;
    dbTable *Table;
    dbString dbstring;
    dbColumn *Column;
    dbCursor cursor;
    dbValue *Value;

    G_debug(3, "write_attributes(): cat = %d", cat);

    if (cat < 0) {
	G_warning ("Feature without category of layer %d", Fi->number);
	return 0;
    }

    db_init_string(&dbstring);
    
    /* read & set attributes */
    sprintf(buf, "SELECT * FROM %s WHERE %s = %d", Fi->table, Fi->key,
	    cat);
    G_debug(4, "SQL: %s", buf);
    db_set_string(&dbstring, buf);

    /* open driver & select data */
    Driver = db_start_driver(Fi->driver);
    if (db_open_select_cursor(Driver, &dbstring, &cursor, DB_SEQUENTIAL) != DB_OK) {
	G_fatal_error(_("Unable to select attributes for category %d"),
		      cat);
    }
    
    if (db_fetch(&cursor, DB_NEXT, &more) != DB_OK) {
	G_fatal_error(_("Unable to fetch data from table <%s>"),
		      Fi->table);
    }
    
    if (!more) {
	G_warning (_("No database record for category %d - export of 'cat' disabled"),
		   cat);
	return -1;
    }

    Table = db_get_cursor_table(&cursor);
    ncol = db_get_table_number_of_columns(Table);
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
					  db_get_column_name(Column));
	
	/* Reset */
	OGR_F_UnsetField(Ogr_feature, ogrfieldnum);
	
	/* prevent writing NULL values */
	if (!db_test_value_isnull(Value)) {
	    switch (colctype) {
	    case DB_C_TYPE_INT:
		OGR_F_SetFieldInteger(Ogr_feature, ogrfieldnum,
				      db_get_value_int(Value));
		break;
	    case DB_C_TYPE_DOUBLE:
		OGR_F_SetFieldDouble(Ogr_feature, ogrfieldnum,
				     db_get_value_double(Value));
		break;
	    case DB_C_TYPE_STRING:
		OGR_F_SetFieldString(Ogr_feature, ogrfieldnum,
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

    db_close_cursor (&cursor);
    db_close_database(Driver);
    db_shutdown_driver(Driver);
    
    db_free_string(&dbstring);
    
    return 1;
}

#endif /* HAVE_OGR */

/*!
   \file lib/vector/Vlib/write_ogr.c

   \brief Vector library - write vector feature (OGR format)

   Higher level functions for reading/writing/manipulating vectors.

   Partly inspired by v.out.ogr's code.

   \todo How to deal with OGRNullFID
   
   (C) 2009-2013 by Martin Landa, and the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Martin Landa <landa.martin gmail.com>
 */

#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

#ifdef HAVE_OGR
#include <ogr_api.h>

static off_t write_feature(struct Map_info *, int, const struct line_pnts **, int,
                           const struct line_cats *);
static int write_attributes(dbDriver *, int, const struct field_info *,
			    OGRLayerH, OGRFeatureH);
static int sqltype_to_ogrtype(int);
#endif

/*!
  \brief Writes feature on level 1 (OGR interface)

  Note:
   - centroids are not supported in OGR, pseudotopo holds virtual
     centroids (it's coordinates determined from spatial index)
   - unclosed boundaries are not supported in OGR, pseudotopo treats
     polygons as boundaries
     
  Supported feature types:
   - GV_POINT (written as wkbPoint)
   - GV_LINE (wkbLineString)
   - GV_BOUNDARY (wkbPolygon)
   - GV_FACE (wkbPolygon25D)
   - GV_KERNEL (wkbPoint25D)

  \param Map pointer to Map_info structure
  \param type feature type
  \param points pointer to line_pnts structure (feature geometry) 
  \param cats pointer to line_cats structure (feature categories)
  
  \return feature index in offset array (related to pseudo-topology)
  \return -1 on error
*/
off_t V1_write_line_ogr(struct Map_info *Map, int type,
			const struct line_pnts *points,
			const struct line_cats *cats)
{
#ifdef HAVE_OGR
    return write_feature(Map, type, &points, 1, cats);
#else
    G_fatal_error(_("GRASS is not compiled with OGR support"));
    return -1;
#endif
}

/*!
  \brief Rewrites feature at the given offset on level 1 (OGR interface)
  
  This function simply calls V1_delete_line_ogr() and V1_write_line_ogr().
  
  \param Map pointer to Map_info structure
  \param offset feature offset
  \param type feature type (see V1_write_line_ogr() for supported types)
  \param points pointer to line_pnts structure (feature geometry)
  \param cats pointer to line_cats structure (feature categories)
  
  \return feature offset (rewriten feature)
  \return -1 on error
*/
off_t V1_rewrite_line_ogr(struct Map_info *Map,
			  int line, int type, off_t offset,
			  const struct line_pnts *points, const struct line_cats *cats)
{
    G_debug(3, "V1_rewrite_line_ogr(): line=%d type=%d offset=%"PRI_OFF_T,
	    line, type, offset);
#ifdef HAVE_OGR
    if (type != V1_read_line_ogr(Map, NULL, NULL, offset)) {
	G_warning(_("Unable to rewrite feature (incompatible feature types)"));
	return -1;
    }

    /* delete old */
    V1_delete_line_ogr(Map, offset);

    return V1_write_line_ogr(Map, type, points, cats);
#else
    G_fatal_error(_("GRASS is not compiled with OGR support"));
    return -1;
#endif
}

/*!
  \brief Deletes feature at the given offset on level 1 (OGR interface)
  
  \param Map pointer Map_info structure
  \param offset offset of feature to be deleted
  
  \return  0 on success
  \return -1 on error
*/
int V1_delete_line_ogr(struct Map_info *Map, off_t offset)
{
#ifdef HAVE_OGR
    struct Format_info_ogr *ogr_info;
    
    G_debug(3, "V1_delete_line_ogr(), offset = %lu", (unsigned long) offset);

    ogr_info = &(Map->fInfo.ogr);
    
    if (!ogr_info->layer) {
	G_warning(_("OGR layer not defined"));
	return -1;
    }
    
    if (offset >= ogr_info->offset.array_num) {
	G_warning(_("Invalid offset (%d)"), offset);
	return -1;
    }
    
    if (OGR_L_DeleteFeature(ogr_info->layer,
			    ogr_info->offset.array[offset]) != OGRERR_NONE)
	G_warning(_("Unable to delete feature"));
	return -1;
    
    return 0;
#else
    G_fatal_error(_("GRASS is not compiled with OGR support"));
    return -1;
#endif
}

#ifdef HAVE_OGR
/*!
   \brief Writes area on topological level (OGR Simple Features
   interface, internal use only)

   \param Map pointer to Map_info structure
   \param points feature geometry (exterior + interior rings)
   \param nparts number of parts including exterior ring
   \param cats feature categories
   
   \return feature offset
   \return -1 on error
*/
off_t V2__write_area_ogr(struct Map_info *Map,
                         const struct line_pnts **points, int nparts,
                         const struct line_cats *cats)
{
    return write_feature(Map, GV_BOUNDARY, points, nparts, cats);
}

/*!
  \brief Write OGR feature

   \param Map pointer to Map_info structure
   \param type feature type (GV_POINT, GV_LINE, ...)
   \param bpoints feature geometry
   \param cats feature categories
   \param ipoints isle geometry for polygons on NULL
   \param nisles number of isles
   
   \return feature offset into file
   \return -1 on error
*/
off_t write_feature(struct Map_info *Map, int type,
                    const struct line_pnts **p_points, int nparts,
                    const struct line_cats *cats)
{
    int i, cat, ret;

    struct field_info *Fi;
    const struct line_pnts *points;
    struct Format_info_ogr *ogr_info;
    struct Format_info_offset *offset_info;
    
    off_t offset;
    
    OGRGeometryH       Ogr_geometry;
    OGRFeatureH        Ogr_feature;
    OGRFeatureDefnH    Ogr_featuredefn;
    OGRwkbGeometryType Ogr_geom_type;

    ogr_info = &(Map->fInfo.ogr);
    offset_info = &(ogr_info->offset);
    
    if (nparts < 1)
        return -1;
    
    points = p_points[0]; /* feature geometry */
    
    if (!ogr_info->layer) {
	/* create OGR layer if doesn't exist */
	if (V2_open_new_ogr(Map, type) < 0)
	    return -1;
    }

    cat = -1; /* no attributes to be written */
    if (cats->n_cats > 0 && Vect_get_num_dblinks(Map) > 0) {
	/* check for attributes */
	Fi = Vect_get_dblink(Map, 0);
	if (Fi) {
	    if (!Vect_cat_get(cats, Fi->number, &cat))
		G_warning(_("No category defined for layer %d"), Fi->number);
	    if (cats->n_cats > 1) {
		G_warning(_("Feature has more categories, using "
			    "category %d (from layer %d)"),
			  cat, cats->field[0]);
	    }
	}
    }
    
    Ogr_featuredefn = OGR_L_GetLayerDefn(ogr_info->layer);
    Ogr_geom_type = OGR_FD_GetGeomType(Ogr_featuredefn);
    
    /* determine matching OGR feature geometry type */
    if (type & (GV_POINT | GV_KERNEL)) {
	if (Ogr_geom_type != wkbPoint &&
	    Ogr_geom_type != wkbPoint25D) {
	    G_warning(_("Feature is not a point. Skipping."));
	    return -1;
	}
	Ogr_geometry = OGR_G_CreateGeometry(wkbPoint);
    }
    else if (type & GV_LINE) {
	if (Ogr_geom_type != wkbLineString &&
	    Ogr_geom_type != wkbLineString25D) {
	    G_warning(_("Feature is not a line. Skipping."));
	    return -1;
	}
	Ogr_geometry = OGR_G_CreateGeometry(wkbLineString);
    }
    else if (type & GV_BOUNDARY) {
	if (Ogr_geom_type != wkbPolygon) {
	    G_warning(_("Feature is not a polygon. Skipping."));
	    return -1;
	}
	Ogr_geometry = OGR_G_CreateGeometry(wkbPolygon);
    }
    else if (type & GV_FACE) {
	if (Ogr_geom_type != wkbPolygon25D) {
	    G_warning(_("Feature is not a face. Skipping."));
	    return -1;
	}
	Ogr_geometry = OGR_G_CreateGeometry(wkbPolygon25D);
    }
    else {
	G_warning(_("Unsupported feature type (%d)"), type);
	return -1;
    }

    G_debug(3, "V1_write_line_ogr(): type = %d", type);

    if (Ogr_geom_type == wkbPolygon || Ogr_geom_type == wkbPolygon25D) {
	int iring, npoints;
	
        /* add rings (first is exterior ring) */
        for (iring = 0; iring < nparts; iring++) {
            OGRGeometryH Ogr_ring;
            
            points = p_points[iring];
            npoints = points->n_points - 1;
            Ogr_ring = OGR_G_CreateGeometry(wkbLinearRing);
            if (points->x[0] != points->x[npoints] ||
                points->y[0] != points->y[npoints] ||
                points->z[0] != points->z[npoints]) {
                G_warning(_("Boundary is not closed. Feature skipped."));
                return -1;
            }
	
            /* add points */
            for (i = 0; i < npoints; i++) {
                OGR_G_AddPoint(Ogr_ring, points->x[i], points->y[i],
                               points->z[i]);
            }
            G_debug(4, "   ring(%d): n_points = %d", iring, npoints);
            OGR_G_AddGeometry(Ogr_geometry, Ogr_ring);
        }
    }
    else {
	for (i = 0; i < points->n_points; i++) {
	    OGR_G_AddPoint(Ogr_geometry, points->x[i], points->y[i],
			   points->z[i]);
	}
        G_debug(4, "   n_points = %d", points->n_points);
    }
    
    /* create feature & set geometry */
    Ogr_feature = OGR_F_Create(Ogr_featuredefn);
    OGR_F_SetGeometry(Ogr_feature, Ogr_geometry);

    /* write attributes */
    if (cat > -1 && ogr_info->dbdriver) {
	if (0 > write_attributes(ogr_info->dbdriver,
                                 cat, Fi, ogr_info->layer, Ogr_feature))
            G_warning(_("Unable to writes feature attributes"));
	G_free(Fi);
    }
    /* write feature into layer */
    ret = OGR_L_CreateFeature(ogr_info->layer, Ogr_feature);

    /* update offset array */
    if (offset_info->array_num >= offset_info->array_alloc) {
	offset_info->array_alloc += 1000;
	offset_info->array = (int *) G_realloc(offset_info->array,
						offset_info->array_alloc *
						sizeof(int));
    }

    offset = offset_info->array_num;
    
    offset_info->array[offset_info->array_num++] = (int) OGR_F_GetFID(Ogr_feature);
    if (Ogr_geom_type == wkbPolygon || Ogr_geom_type == wkbPolygon25D) {
	/* register exterior ring in offset array */
	offset_info->array[offset_info->array_num++] = 0; 
    }
      
    /* destroy */
    OGR_G_DestroyGeometry(Ogr_geometry);
    OGR_F_Destroy(Ogr_feature);
    
    if (ret != OGRERR_NONE)
	return -1;
    
    G_debug(3, "write_feature(): -> offset = %lu offset_num = %d cat = %d",
	    (unsigned long) offset, offset_info->array_num, cat);

    return offset;
}

/*!
  \brief Writes attributes

  \param driver pointer to dbDriver
  \param Fi pointer to field_info struct
  \param[in,out] Ogr_layer OGR layer
  \param[in,out] Ogr_feature OGR feature to modify

  \return 1 on success
  \return 0 no attributes
  \return -1 on error
*/
int write_attributes(dbDriver *driver, int cat, const struct field_info *Fi,
		     OGRLayerH Ogr_layer, OGRFeatureH Ogr_feature)
{
    int j, ogrfieldnum;
    char buf[2000];
    int ncol, sqltype, ctype, ogrtype, more;
    const char *fidcol, *colname;
    dbTable *table;
    dbString dbstring;
    dbColumn *column;
    dbCursor cursor;
    dbValue *value;

    OGRFieldDefnH hFieldDefn;
    
    G_debug(3, "write_attributes(): cat = %d", cat);

    if (cat < 0) {
	G_warning(_("Feature without category of layer %d"), Fi->number);
	return 0;
    }

    db_init_string(&dbstring);
    
    /* read & set attributes */
    sprintf(buf, "SELECT * FROM %s WHERE %s = %d", Fi->table, Fi->key,
	    cat);
    G_debug(4, "SQL: %s", buf);
    db_set_string(&dbstring, buf);

    /* select data */
    if (db_open_select_cursor(driver, &dbstring, &cursor, DB_SEQUENTIAL) != DB_OK) {
        G_warning(_("Unable to select attributes for category %d"),
                  cat);
        return -1;
    }
    
    if (db_fetch(&cursor, DB_NEXT, &more) != DB_OK) {
	G_warning(_("Unable to fetch data from table <%s>"),
                  Fi->table);
        return -1;
    }
    
    if (!more) {
	G_warning(_("No database record for category %d, "
		    "no attributes will be written"),
		  cat);
	return -1;
    }

    fidcol = OGR_L_GetFIDColumn(Ogr_layer); 

    table = db_get_cursor_table(&cursor);
    ncol = db_get_table_number_of_columns(table);
    for (j = 0; j < ncol; j++) {
	column = db_get_table_column(table, j);
	colname = db_get_column_name(column);
	if (fidcol && *fidcol && strcmp(colname, fidcol) == 0) {
	    /* skip fid column */
	    continue;
	}
	value = db_get_column_value(column);
	/* for debug only */
	db_convert_column_value_to_string(column, &dbstring);	
	G_debug(2, "col %d : val = %s", j,
		db_get_string(&dbstring));
	
	sqltype = db_get_column_sqltype(column);
	ctype = db_sqltype_to_Ctype(sqltype);
	ogrtype = sqltype_to_ogrtype(sqltype);
	G_debug(2, "  colctype = %d", ctype);
	
	ogrfieldnum = OGR_F_GetFieldIndex(Ogr_feature, colname);
	if (ogrfieldnum < 0) {
	    /* create field if not exists */
	    hFieldDefn = OGR_Fld_Create(colname, ogrtype);
	    if (OGR_L_CreateField(Ogr_layer, hFieldDefn, TRUE) != OGRERR_NONE)
		G_warning(_("Unable to create field <%s>"), colname);
	    ogrfieldnum = OGR_F_GetFieldIndex(Ogr_feature, colname);
	}
	    
	/* Reset */
	OGR_F_UnsetField(Ogr_feature, ogrfieldnum);
	
	/* prevent writing NULL values */
	if (!db_test_value_isnull(value)) {
	    switch (ctype) {
	    case DB_C_TYPE_INT:
		OGR_F_SetFieldInteger(Ogr_feature, ogrfieldnum,
				      db_get_value_int(value));
		break;
	    case DB_C_TYPE_DOUBLE:
		OGR_F_SetFieldDouble(Ogr_feature, ogrfieldnum,
				     db_get_value_double(value));
		break;
	    case DB_C_TYPE_STRING:
		OGR_F_SetFieldString(Ogr_feature, ogrfieldnum,
				     db_get_value_string(value));
		break;
	    case DB_C_TYPE_DATETIME:
		db_convert_column_value_to_string(column,
						  &dbstring);
		OGR_F_SetFieldString(Ogr_feature, ogrfieldnum,
				     db_get_string(&dbstring));
		break;
	    default:
		G_warning(_("Unsupported column type %d"), ctype);
		break;
	    }
	}
    }

    db_close_cursor (&cursor);
    
    db_free_string(&dbstring);
    
    return 1;
}

int sqltype_to_ogrtype(int sqltype)
{
    int ctype, ogrtype;

    ctype = db_sqltype_to_Ctype(sqltype);
    
    switch(ctype) {
    case DB_C_TYPE_INT:
	ogrtype = OFTInteger;
	break;
    case DB_C_TYPE_DOUBLE:
	ogrtype = OFTReal;
	break;
    case DB_C_TYPE_STRING:
	ogrtype = OFTString;
	break;
    case DB_C_TYPE_DATETIME:
	ogrtype = OFTString;
	break;
    default:
	ogrtype = OFTString;
	break;
    }
    
    return ogrtype;
}

#endif /* HAVE_OGR */

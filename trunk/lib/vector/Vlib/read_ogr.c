/*!
   \file lib/vector/Vlib/read_ogr.c

   \brief Vector library - reading data (OGR format)

   Higher level functions for reading/writing/manipulating vectors.

   (C) 2001-2011 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Radim Blazek, Piero Cavalieri
   \author Martin Landa <landa.martin gmail.com>
 */

#include <grass/vector.h>
#include <grass/glocale.h>

#ifdef HAVE_OGR
#include <ogr_api.h>

static int cache_feature(struct Map_info *, OGRGeometryH, int);
static int read_line(const struct Map_info *, OGRGeometryH, long,
		     struct line_pnts *);
static int get_line_type(const struct Map_info *, long);
static int read_next_line_ogr(struct Map_info *, struct line_pnts *,
			      struct line_cats *, int);
#endif

/*!
  \brief Read next feature from OGR layer.
  Skip empty features (level 1 without topology).
  
  This function implements sequential access.
  
  The action of this routine can be modified by:
   - Vect_read_constraint_region()
   - Vect_read_constraint_type()
   - Vect_remove_constraints()
  
  \param Map pointer to Map_info structure
  \param[out] line_p container used to store line points within
  \param[out] line_c container used to store line categories within
  
  \return feature type
  \return -2 no more features (EOF)
  \return -1 out of memory
*/
int V1_read_next_line_ogr(struct Map_info *Map, struct line_pnts *line_p,
			  struct line_cats *line_c)
{
#ifdef HAVE_OGR
    return read_next_line_ogr(Map, line_p, line_c, FALSE);
#else
    G_fatal_error(_("GRASS is not compiled with OGR support"));
    return -1;
#endif
}

/*!
  \brief Read next feature from OGR layer on topological level.

  This function implements sequential access.

  \param Map pointer to Map_info structure
  \param[out] line_p container used to store line points within
  (pointer to line_pnts struct)
  \param[out] line_c container used to store line categories within
  (pointer to line_cats struct)
  
  \return feature type
  \return -2 no more features (EOF)
  \return -1 on failure
*/
int V2_read_next_line_ogr(struct Map_info *Map, struct line_pnts *line_p,
			  struct line_cats *line_c)
{
#ifdef HAVE_OGR
    int line, ret;
    struct P_line *Line;
    struct bound_box lbox, mbox;

    G_debug(3, "V2_read_next_line_ogr()");
    
    if (Map->constraint.region_flag)
	Vect_get_constraint_box(Map, &mbox);
    
    while(TRUE) {
	line = Map->next_line;
	
	if (Map->next_line > Map->plus.n_lines)
	    return -2; /* nothing to read */
	
	Map->next_line++;
	Line = Map->plus.Line[line];
	if (Line == NULL) {	/* skip dead features */
	    continue;
	}

	if (Map->constraint.type_flag) {
	    /* skip feature by type */
	    if (!(Line->type & Map->constraint.type))
		continue;
	}

	if (Line->type == GV_CENTROID) {
	    G_debug(4, "Centroid");
	    
	    if (line_p != NULL) {
		int i, found;
		struct bound_box box;
		struct boxlist list;
		struct P_topo_c *topo = (struct P_topo_c *)Line->topo;
		
		/* get area bbox */
		Vect_get_area_box(Map, topo->area, &box);
		/* search in spatial index for centroid with area bbox */
		dig_init_boxlist(&list, TRUE);
		Vect_select_lines_by_box(Map, &box, Line->type, &list);
		
		found = 0;
		for (i = 0; i < list.n_values; i++) {
		    if (list.id[i] == line) {
			found = i;
			break;
		    }
		}
		
		Vect_reset_line(line_p);
		Vect_append_point(line_p, list.box[found].E, list.box[found].N, 0.0);
	    }
	    if (line_c != NULL) {
		/* cat = FID and offset = FID for centroid */
		Vect_reset_cats(line_c);
		Vect_cat_set(line_c, 1, (int) Line->offset);
	    }

	    ret = GV_CENTROID;
	}
	else {
	    ret = read_next_line_ogr(Map, line_p, line_c, TRUE);
	}

	if (line_p && Map->constraint.region_flag) {
	    /* skip feature by region */
	    Vect_line_box(line_p, &lbox);
	    if (!Vect_box_overlap(&lbox, &mbox))
		continue;
	}
	
	/* skip feature by field ignored */
		
	return ret;
    }
#else
    G_fatal_error(_("GRASS is not compiled with OGR support"));
    return -1;
#endif
}

/*!
  \brief Read feature from OGR layer at given offset (level 1 without topology)
  
  This function implements random access on level 1.

  \param Map pointer to Map_info structure 
  \param[out] line_p container used to store line points within
  (pointer line_pnts struct)
  \param[out] line_c container used to store line categories within
  (pointer line_cats struct)
  \param offset given offset 
  
  \return line type
  \return 0 dead line
  \return -2 no more features
  \return -1 on failure
*/
int V1_read_line_ogr(struct Map_info *Map,
		     struct line_pnts *line_p, struct line_cats *line_c, off_t offset)
{
#ifdef HAVE_OGR
    long fid;
    int type;
    OGRGeometryH hGeom;

    struct Format_info_ogr *ogr_info;
    
    ogr_info = &(Map->fInfo.ogr);
    G_debug(3, "V1_read_line_ogr(): offset = %lu offset_num = %lu",
	    (long) offset, (long) ogr_info->offset.array_num);

    if (offset >= ogr_info->offset.array_num)
	return -2; /* nothing to read */
    
    if (line_p != NULL)
	Vect_reset_line(line_p);
    if (line_c != NULL)
	Vect_reset_cats(line_c);

    fid = ogr_info->offset.array[offset];
    G_debug(4, "  fid = %ld", fid);
    
    /* coordinates */
    if (line_p != NULL) {
	/* read feature to cache if necessary */
	if (ogr_info->cache.fid != fid) {
	    G_debug(4, "Read feature (fid = %ld) to cache", fid);
	    if (ogr_info->feature_cache) {
		OGR_F_Destroy(ogr_info->feature_cache);
	    }
	    ogr_info->feature_cache =
		OGR_L_GetFeature(ogr_info->layer, fid);
	    if (ogr_info->feature_cache == NULL) {
		G_warning(_("Unable to get feature geometry, fid %ld"),
			  fid);
		return -1;
	    }
	    ogr_info->cache.fid = fid;
	}
	
	hGeom = OGR_F_GetGeometryRef(ogr_info->feature_cache);
	if (hGeom == NULL) {
	    G_warning(_("Unable to get feature geometry, fid %ld"),
		      fid);
	    return -1;
	}
	
	type = read_line(Map, hGeom, offset + 1, line_p);
    }
    else {
	type = get_line_type(Map, fid);
    }

    /* category */
    if (line_c != NULL) {
	Vect_cat_set(line_c, 1, (int) fid);
    }

    return type;
#else
    G_fatal_error(_("GRASS is not compiled with OGR support"));
    return -1;
#endif
}

#ifdef HAVE_OGR
/*!
  \brief Recursively read feature and add all elements to points_cache and types_cache.
  
  ftype: if > 0 use this type (because parts of Polygon are read as wkbLineString)
  
  \param Map pointer to Map_info structure
  \param[out] hGeom OGR geometry
  \param ftype feature type
  
  \return 0 on success
  \return 1 on error
*/
int cache_feature(struct Map_info *Map, OGRGeometryH hGeom, int ftype)
{
    int line, i, np, ng, tp;

    struct Format_info_ogr *ogr_info;

    OGRwkbGeometryType type;
    OGRGeometryH hGeom2;

    G_debug(4, "cache_feature() ftype = %d", ftype);

    ogr_info = &(Map->fInfo.ogr);
    
    /* alloc space in lines cache */
    line = ogr_info->cache.lines_num;
    if (line == ogr_info->cache.lines_alloc) {
	ogr_info->cache.lines_alloc += 1;
	ogr_info->cache.lines =
	    (struct line_pnts **)G_realloc((void *)ogr_info->cache.lines,
					   ogr_info->cache.lines_alloc *
					   sizeof(struct line_pnts *));

	ogr_info->cache.lines_types =
	    (int *)G_realloc(ogr_info->cache.lines_types,
			     ogr_info->cache.lines_alloc * sizeof(int));

	for (i = ogr_info->cache.lines_num; i < ogr_info->cache.lines_alloc; i++)
	    ogr_info->cache.lines[i] = Vect_new_line_struct();
    }
    Vect_reset_line(ogr_info->cache.lines[line]);

    type = wkbFlatten(OGR_G_GetGeometryType(hGeom));

    switch (type) {
    case wkbPoint:
	G_debug(4, "Point");
	Vect_append_point(ogr_info->cache.lines[line],
			  OGR_G_GetX(hGeom, 0), OGR_G_GetY(hGeom, 0),
			  OGR_G_GetZ(hGeom, 0));
	ogr_info->cache.lines_types[line] = GV_POINT;
	ogr_info->cache.lines_num++;
	return 0;
	break;

    case wkbLineString:
	G_debug(4, "LineString");
	np = OGR_G_GetPointCount(hGeom);
	for (i = 0; i < np; i++) {
	    Vect_append_point(ogr_info->cache.lines[line],
			      OGR_G_GetX(hGeom, i), OGR_G_GetY(hGeom, i),
			      OGR_G_GetZ(hGeom, i));
	}

	if (ftype > 0) {	/* Polygon rings */
	    ogr_info->cache.lines_types[line] = ftype;
	}
	else {
	    ogr_info->cache.lines_types[line] = GV_LINE;
	}
	ogr_info->cache.lines_num++;
	return 0;
	break;

    case wkbMultiPoint:
    case wkbMultiLineString:
    case wkbPolygon:
    case wkbMultiPolygon:
    case wkbGeometryCollection:
	ng = OGR_G_GetGeometryCount(hGeom);
	G_debug(4, "%d geoms -> next level", ng);
	if (type == wkbPolygon) {
	    tp = GV_BOUNDARY;
	}
	else {
	    tp = -1;
	}
	for (i = 0; i < ng; i++) {
	    hGeom2 = OGR_G_GetGeometryRef(hGeom, i);
	    cache_feature(Map, hGeom2, tp);
	}
	return 0;
	break;

    default:
	G_warning(_("OGR feature type %d not supported"), type);
	return 1;
	break;
    }
}

int read_next_line_ogr(struct Map_info *Map, struct line_pnts *line_p,
		       struct line_cats *line_c, int ignore_constraint)
{
    int itype;
    struct bound_box lbox, mbox;
    OGRFeatureH hFeature;
    OGRGeometryH hGeom;

    struct Format_info_ogr *ogr_info;

    G_debug(3, "V1_read_next_line_ogr()");

    if (Map->constraint.region_flag && !ignore_constraint)
	Vect_get_constraint_box(Map, &mbox);

    ogr_info = &(Map->fInfo.ogr);
    while (TRUE) {
	/* reset data structures */
	if (line_p != NULL)
	    Vect_reset_line(line_p);
	if (line_c != NULL)
	    Vect_reset_cats(line_c);
    
	/* read feature to cache if necessary */
	while (ogr_info->cache.lines_next == ogr_info->cache.lines_num) {
	    hFeature = OGR_L_GetNextFeature(ogr_info->layer);
	    if (hFeature == NULL) {
		return -2;              /* nothing to read */
	    }			

	    hGeom = OGR_F_GetGeometryRef(hFeature);
	    if (hGeom == NULL) {	/* skip feature without geometry */
		G_warning(_("Feature without geometry. Skipped."));
		OGR_F_Destroy(hFeature);
		continue;
	    }

	    /* cache OGR feature */
	    ogr_info->cache.fid = (int)OGR_F_GetFID(hFeature);
	    if (ogr_info->cache.fid == OGRNullFID) {
		G_warning(_("OGR feature without ID"));
	    }

	    /* cache feature */
	    ogr_info->cache.lines_num = 0;
	    cache_feature(Map, hGeom, -1);
	    G_debug(4, "%d lines read to cache", ogr_info->cache.lines_num);
	    OGR_F_Destroy(hFeature);

	    /* next to be read from cache */
	    ogr_info->cache.lines_next = 0;	
	}

	/* read next part of the feature */
	G_debug(4, "read next cached line %d", ogr_info->cache.lines_next);
	itype = ogr_info->cache.lines_types[ogr_info->cache.lines_next];

	if (Map->constraint.type_flag && !ignore_constraint) {
	    /* skip feature by type */
	    if (!(itype & Map->constraint.type)) {
		ogr_info->cache.lines_next++;
		continue;
	    }
	}

	if (Map->constraint.region_flag && !ignore_constraint) {
	    /* skip feature by region */
	    Vect_line_box(ogr_info->cache.lines[ogr_info->cache.lines_next],
			  &lbox);
	    
	    if (!Vect_box_overlap(&lbox, &mbox)) {
		ogr_info->cache.lines_next++;
		continue;
	    }
	}
	
	/* skip feature by field - ignored */
	
	if (line_p != NULL)
	    Vect_append_points(line_p,
			       ogr_info->cache.lines[ogr_info->cache.lines_next], GV_FORWARD);

	if (line_c != NULL && ogr_info->cache.fid != OGRNullFID)
	    Vect_cat_set(line_c, 1, ogr_info->cache.fid);

	ogr_info->cache.lines_next++;
	G_debug(4, "next line read, type = %d", itype);
	
	return itype;
    }

    return -1;			/* not reached */
}

/*!
  \brief Recursively descend to feature and read the part
  
  \param Map pointer to Map_info structure
  \param hGeom OGR geometry
  \param offset given offset
  \param[out] Points container used to store line pointes within
  
  \return feature type
  \return -1 on error
*/
int read_line(const struct Map_info *Map, OGRGeometryH hGeom, long offset,
	      struct line_pnts *Points)
{
    int i, nPoints;
    int eType, line;
    
    const struct Format_info_ogr *ogr_info;
    
    OGRGeometryH hGeom2;

    /* Read coors if hGeom is a simple element (wkbPoint,
     * wkbLineString) otherwise descend to geometry specified by
     * offset[offset] */

    ogr_info = &(Map->fInfo.ogr);
    
    eType = wkbFlatten(OGR_G_GetGeometryType(hGeom));
    G_debug(4, "OGR geometry type: %d", eType);
    
    switch (eType) {
    case wkbPoint:
	G_debug(4, "\t->Point");
	if (Points) {
	    Vect_append_point(Points, OGR_G_GetX(hGeom, 0), OGR_G_GetY(hGeom, 0),
			      OGR_G_GetZ(hGeom, 0));
	}
	return GV_POINT;
	break;

    case wkbLineString:
	G_debug(4, "\t->LineString");	
	if (Points) {
	    nPoints = OGR_G_GetPointCount(hGeom);
	    for (i = 0; i < nPoints; i++) {
		Vect_append_point(Points, OGR_G_GetX(hGeom, i),
				  OGR_G_GetY(hGeom, i), OGR_G_GetZ(hGeom, i));
	    }
	}
	return GV_LINE;
	break;

    case wkbPolygon:
    case wkbMultiPoint:
    case wkbMultiLineString:
    case wkbMultiPolygon:
    case wkbGeometryCollection:
	G_debug(4, "\t->more geoms -> part %d", ogr_info->offset.array[offset]);
	hGeom2 = OGR_G_GetGeometryRef(hGeom, ogr_info->offset.array[offset]);
	line = read_line(Map, hGeom2, offset + 1, Points);
	if (eType == wkbPolygon || eType == wkbMultiPolygon)
	    return GV_BOUNDARY;
	if (eType == wkbMultiPoint)
	    return GV_POINT;
	if (eType == wkbMultiLineString)
	    return GV_LINE;
	return line;
	break;

    default:
	G_warning(_("OGR feature type '%s' not supported"),
		  OGRGeometryTypeToName(eType));
	break;
    }

    return -1;
}

/*!
  \brief Recursively descend to feature and read the part
  
  \param Map pointer to Map_info structure
  \param hGeom OGR geometry
  \param offset given offset
  \param[out] Points container used to store line pointes within
  
  \return feature type
  \return -1 on error
*/
int get_line_type(const struct Map_info *Map, long fid)
{
    int eType;

    const struct Format_info_ogr *ogr_info;
    
    OGRFeatureH hFeat;
    OGRGeometryH hGeom;

    G_debug(4, "get_line_type() fid = %ld", fid);

    ogr_info = &(Map->fInfo.ogr);
    
    hFeat = OGR_L_GetFeature(ogr_info->layer, fid);
    if (hFeat == NULL)
	return -1;

    hGeom = OGR_F_GetGeometryRef(hFeat);
    if (hGeom == NULL)
	return -1;
    
    eType = wkbFlatten(OGR_G_GetGeometryType(hGeom));

    OGR_F_Destroy(hFeat);

    G_debug(4, "OGR Geometry of type: %d", eType);

    switch (eType) {
    case wkbPoint:
    case wkbMultiPoint:
	return GV_POINT;
	break;
	
    case wkbLineString:
    case wkbMultiLineString:
	return GV_LINE;
	break;

    case wkbPolygon:
    case wkbMultiPolygon:
    case wkbGeometryCollection:
	return GV_BOUNDARY;
	break;

    default:
	G_warning(_("OGR feature type %d not supported"), eType);
	break;
    }

    return -1;
}
#endif

/*!
   \file lib/vector/Vlib/build_sfa.c

   \brief Vector library - Building pseudo-topology for simple feature access

   Higher level functions for reading/writing/manipulating vectors.

   Line offset is
    - centroids   : FID
    - other types : index of the first record (which is FID) in offset array.
 
   (C) 2001-2012 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Radim Blazek
   \author Piero Cavalieri
   \author Various updates for GRASS 7 by Martin Landa <landa.martin gmail.com>
*/

#include <stdlib.h>

#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>

/*!  
  \brief This structure keeps info about geometry parts above current
  geometry, path to curent geometry in the feature. First 'part' number
  however is feature id */
struct geom_parts
{
    int *part;
    int a_parts;
    int n_parts;
};

static void init_parts(struct geom_parts *);
static void reset_parts(struct geom_parts *);
static void free_parts(struct geom_parts *);
static void add_part(struct geom_parts *, int);
static void del_part(struct geom_parts *);
static void add_parts_to_offset(struct Format_info_offset *,
				struct geom_parts *);
static int add_line(struct Plus_head *, struct Format_info_offset *,
		    int, struct line_pnts *,
		    int, struct geom_parts *);

#ifdef HAVE_POSTGRES
#include "pg_local_proto.h"

static int add_geometry_pg(struct Plus_head *,
			   struct Format_info_pg *,
			   struct feat_parts *, int, 
			   int, int,
 struct geom_parts *);
static void build_pg(struct Map_info *, int);
#endif

#ifdef HAVE_OGR
#include <ogr_api.h>

static int add_geometry_ogr(struct Plus_head *,
			    struct Format_info_ogr *,
			    OGRGeometryH, int, int,
			    struct geom_parts *);

static void build_ogr(struct Map_info *, int);
#endif

/*!
  \brief Init parts
*/
void init_parts(struct geom_parts * parts)
{
    G_zero(parts, sizeof(struct geom_parts));
}

/*!
  \brief Reset parts
*/
void reset_parts(struct geom_parts * parts)
{
    parts->n_parts = 0;
}

/*!
  \brief Free parts
*/
void free_parts(struct geom_parts * parts)
{
    G_free(parts->part);
    G_zero(parts, sizeof(struct geom_parts));
}

/*!
  \brief Add new part number to parts
*/
void add_part(struct geom_parts *parts, int part)
{
    if (parts->a_parts == parts->n_parts) {
	parts->a_parts += 10;
	parts->part = (int *) G_realloc((void *)parts->part,
					parts->a_parts * sizeof(int));
    }
    parts->part[parts->n_parts] = part;
    parts->n_parts++;
}

/*!
  \brief Remove last part
*/
void del_part(struct geom_parts *parts)
{
    parts->n_parts--;
}

/*!
  \brief Add parts to offset
*/
void add_parts_to_offset(struct Format_info_offset *offset,
			 struct geom_parts *parts)
{
    int i, j;

    if (offset->array_num + parts->n_parts >= offset->array_alloc) {
	offset->array_alloc += parts->n_parts + 1000;
	offset->array = (int *)G_realloc(offset->array,
					 offset->array_alloc * sizeof(int));
    }
    j = offset->array_num;
    for (i = 0; i < parts->n_parts; i++) {
	G_debug(4, "add offset %d", parts->part[i]);
	offset->array[j] = parts->part[i];
	j++;
    }
    offset->array_num += parts->n_parts;
}

/*!
  \brief Add line to support structures
*/
int add_line(struct Plus_head *plus, struct Format_info_offset *offset,
	     int type, struct line_pnts *Points,
	     int FID, struct geom_parts *parts)
{
    int line;
    long offset_value;
    struct bound_box box;

    if (type != GV_CENTROID) {
	/* beginning in the offset array */
	offset_value = offset->array_num;
    }
    else {
	/* TODO : could be used to statore category ? */
	/* because centroids are read from topology, not from layer */
	offset_value = FID; 
    }

    G_debug(4, "Register line: FID = %d offset = %ld", FID, offset_value);
    dig_line_box(Points, &box);
    line = dig_add_line(plus, type, Points, &box, offset_value);
    G_debug(4, "Line registered with line = %d", line);

    /* Set box */
    if (line == 1)
	Vect_box_copy(&(plus->box), &box);
    else
	Vect_box_extend(&(plus->box), &box);

    if (type != GV_BOUNDARY) {
	dig_cidx_add_cat(plus, 1, (int)FID, line, type);
    }
    else {
	dig_cidx_add_cat(plus, 0, 0, line, type);
    }

    /* because centroids are read from topology, not from layer */
    if (type != GV_CENTROID)
	add_parts_to_offset(offset, parts);
    
    return line;
}

#ifdef HAVE_POSTGRES
/*!
  \brief Recursively add geometry (PostGIS) to topology
*/
int add_geometry_pg(struct Plus_head *plus,
		    struct Format_info_pg *pg_info,
		    struct feat_parts *fparts, int ipart,
		    int FID, int build, struct geom_parts *parts)
{
    int line, i, idx, area, isle, outer_area, ret;
    int lines[1];
    double area_size, x, y;
    SF_FeatureType ftype;
    struct bound_box box;
    struct Format_info_offset *offset;
    struct line_pnts *line_i;
    
    ftype = fparts->ftype[ipart];
    
    G_debug(4, "add_geometry_pg() FID = %d ftype = %d", FID, ftype);
    
    offset = &(pg_info->offset);
    
    outer_area = 0;
    
    switch(ftype) {	
    case SF_POINT:
	G_debug(4, "Point");
	line_i = pg_info->cache.lines[fparts->idx[ipart]];
	add_line(plus, offset, GV_POINT, line_i,
		 FID, parts);
	break;
    case SF_LINESTRING:
	G_debug(4, "LineString");
	line_i = pg_info->cache.lines[fparts->idx[ipart]];
	add_line(plus, offset, GV_LINE, line_i,
		 FID, parts);
	break;
    case SF_POLYGON:
	G_debug(4, "Polygon");

	/* register boundaries */
	idx = fparts->idx[ipart];
	for (i = 0; i < fparts->nlines[ipart]; i++) {
	    line_i = pg_info->cache.lines[idx++];
	    G_debug(4, "part %d", i);
	    add_part(parts, i);
	    line = add_line(plus, offset, GV_BOUNDARY,
			    line_i, FID, parts);
	    del_part(parts);

	    if (build < GV_BUILD_AREAS)
		continue;
	    
	    /* add area (each inner ring is also area) */
	    dig_line_box(line_i, &box);
	    dig_find_area_poly(line_i, &area_size);

	    if (area_size > 0)	        /* area clockwise */
		lines[0] = line;
	    else
		lines[0] = -line;

	    area = dig_add_area(plus, 1, lines, &box);

	    /* each area is also isle */
	    lines[0] = -lines[0];	/* island is counter clockwise */

	    isle = dig_add_isle(plus, 1, lines, &box);

	    if (build < GV_BUILD_ATTACH_ISLES)
		continue;
	    
	    if (i == 0) {	/* outer ring */
		outer_area = area;
	    }
	    else {		/* inner ring */
		struct P_isle *Isle;

		Isle = plus->Isle[isle];
		Isle->area = outer_area;

		dig_area_add_isle(plus, outer_area, isle);
	    }
	}

	if (build >= GV_BUILD_CENTROIDS) {
	    /* create virtual centroid */
	    ret = Vect_get_point_in_poly_isl((const struct line_pnts *) pg_info->cache.lines[fparts->idx[ipart]],
					     (const struct line_pnts **) &pg_info->cache.lines[fparts->idx[ipart]] + 1,
					     fparts->nlines[ipart] - 1, &x, &y);
	    if (ret < -1) {
		G_warning(_("Unable to calculate centroid for area %d"),
			  outer_area);
	    }
	    else {
		struct P_area *Area;
		struct P_topo_c *topo;
		struct P_line *Line;
		struct line_pnts *line_c;

		G_debug(4, "  Centroid: %f, %f", x, y);
		line_c = Vect_new_line_struct();
		Vect_append_point(line_c, x, y, 0.0);
		line = add_line(plus, offset, GV_CENTROID, line_c, FID, parts);
		
		Line = plus->Line[line];
		topo = (struct P_topo_c *)Line->topo;
		topo->area = outer_area;
		
		/* register centroid to area */
		Area = plus->Area[outer_area];
		Area->centroid = line;
		Vect_destroy_line_struct(line_c);
	    }
	}
	break;
    default:
	G_warning(_("Feature type %d not supported"), ftype);
	break;
    }
    
    return 0;
}

/*!
  \brief Build pseudo-topology for PostGIS layers
*/
void build_pg(struct Map_info *Map, int build)
{
    int iFeature, ipart, fid, nrecords, npoints;
    char *wkb_data;
    
    struct Format_info_pg *pg_info;
    
    struct feat_parts fparts;
    struct geom_parts parts;

    pg_info = &(Map->fInfo.pg);

    /* initialize data structures */
    init_parts(&parts);
    G_zero(&fparts, sizeof(struct feat_parts));
    
    /* get all features */
    if (Vect__open_cursor_next_line_pg(pg_info, TRUE) != 0)
        return;
    
    /* scan records */
    npoints = 0;
    nrecords = PQntuples(pg_info->res);
    G_debug(4, "build_pg(): nrecords = %d", nrecords);
    G_message(_("Registering primitives..."));
    for (iFeature = 0; iFeature < nrecords; iFeature++) {
	/* get feature id */
	fid  = atoi(PQgetvalue(pg_info->res, iFeature, 1));
        if (fid < 1)
            continue; /* PostGIS Topology: skip features with negative
                       * fid (isles, universal face, ...) */

	wkb_data = PQgetvalue(pg_info->res, iFeature, 0);
	
	G_progress(iFeature + 1, 1e4);

	/* cache feature (lines) */
	if (SF_NONE == Vect__cache_feature_pg(wkb_data, FALSE, FALSE,
                                              &(pg_info->cache), &fparts)) {
	    G_warning(_("Feature %d without geometry skipped"),
		      iFeature + 1);
	    continue;
	}
	
	/* register all parts */
	reset_parts(&parts);
	add_part(&parts, fid);
	for (ipart = 0; ipart < fparts.n_parts; ipart++) {
	    if (fparts.nlines[ipart] < 1) {
		G_warning(_("Feature %d without geometry skipped"), fid);
		continue;
	    }
	    
	    npoints += pg_info->cache.lines[ipart]->n_points;

	    G_debug(4, "Feature: fid = %d part = %d", fid, ipart);
	    
	    if (fparts.n_parts > 1)
		add_part(&parts, ipart);
	    add_geometry_pg(&(Map->plus), pg_info, &fparts, ipart,
			    fid, build, &parts);
	    if (fparts.n_parts > 1)
		del_part(&parts);
	}

	/* read next feature from cache */
	pg_info->cache.lines_next = 0;
    }
    G_progress(1, 1);

    G_message(_("%d primitives registered"), Map->plus.n_lines);
    G_message(_("%d vertices registered"), npoints);

    Map->plus.built = GV_BUILD_BASE;

    PQclear(pg_info->res);
    pg_info->res = NULL;
    
    /* free allocated space */
    free_parts(&parts);
}
#endif /* HAVE_POSTGRES */

#ifdef HAVE_OGR
/*!
  \brief Recursively add geometry (OGR) to topology
*/
int add_geometry_ogr(struct Plus_head *plus,
		     struct Format_info_ogr *ogr_info,
		     OGRGeometryH hGeom, int FID, int build,
		     struct geom_parts *parts)
{
    int i, ret, npoints, line;
    int area, isle, outer_area;
    int lines[1];
    double area_size, x, y;
    int eType, nRings, iPart, nParts, nPoints;

    struct bound_box box;
    struct P_line *Line;
    struct Format_info_offset *offset;
    
    OGRGeometryH hGeom2, hRing;

    G_debug(4, "add_geometry_ogr() FID = %d", FID);

    offset = &(ogr_info->offset);
    
    /* allocate space in cache */
    if (!ogr_info->cache.lines) {
	ogr_info->cache.lines_alloc = 1;
	ogr_info->cache.lines = (struct line_pnts **) G_malloc(sizeof(struct line_pnts *));
	
	ogr_info->cache.lines_types = (int *) G_malloc(sizeof(int));
	ogr_info->cache.lines[0] = Vect_new_line_struct();
	ogr_info->cache.lines_types[0] = -1;
    }

    npoints = outer_area = 0;
    eType = wkbFlatten(OGR_G_GetGeometryType(hGeom));
    G_debug(4, "OGR type = %d", eType);

    switch (eType) {
    case wkbPoint:
	G_debug(4, "Point");
	
	ogr_info->cache.lines_types[0] = GV_POINT;
	Vect_reset_line(ogr_info->cache.lines[0]);
	Vect_append_point(ogr_info->cache.lines[0], OGR_G_GetX(hGeom, 0),
			  OGR_G_GetY(hGeom, 0), OGR_G_GetZ(hGeom, 0));
	add_line(plus, offset, GV_POINT, ogr_info->cache.lines[0], FID, parts);
	npoints += ogr_info->cache.lines[0]->n_points;
	break;

    case wkbLineString:
	G_debug(4, "LineString");
	
	ogr_info->cache.lines_types[0] = GV_LINE;
	nPoints = OGR_G_GetPointCount(hGeom);
	Vect_reset_line(ogr_info->cache.lines[0]);
	for (i = 0; i < nPoints; i++) {
	    Vect_append_point(ogr_info->cache.lines[0],
			      OGR_G_GetX(hGeom, i), OGR_G_GetY(hGeom, i),
			      OGR_G_GetZ(hGeom, i));
	}
	add_line(plus, offset, GV_LINE, ogr_info->cache.lines[0], FID, parts);	
	npoints += ogr_info->cache.lines[0]->n_points;
	break;

    case wkbPolygon:
	G_debug(4, "Polygon");

	nRings = OGR_G_GetGeometryCount(hGeom);
	G_debug(4, "Number of rings: %d", nRings);

	/* alloc space for islands if needed */
	if (nRings > ogr_info->cache.lines_alloc) {
	    ogr_info->cache.lines_alloc += nRings;
	    ogr_info->cache.lines = (struct line_pnts **) G_realloc(ogr_info->cache.lines,
								    ogr_info->cache.lines_alloc *
								    sizeof(struct line_pnts *));
	    ogr_info->cache.lines_types = (int *) G_realloc(ogr_info->cache.lines_types,
							    ogr_info->cache.lines_alloc * sizeof(int));
		
	    for (i = ogr_info->cache.lines_alloc - nRings; i < ogr_info->cache.lines_alloc; i++) {
		ogr_info->cache.lines[i] = Vect_new_line_struct();
		ogr_info->cache.lines_types[i] = -1;
	    }
	}

	/* go thru rings */
	for (iPart = 0; iPart < nRings; iPart++) {
	    ogr_info->cache.lines_types[iPart] = GV_BOUNDARY;
	    hRing = OGR_G_GetGeometryRef(hGeom, iPart);
	    nPoints = OGR_G_GetPointCount(hRing);
	    G_debug(4, "  ring %d : nPoints = %d", iPart, nPoints);

	    Vect_reset_line(ogr_info->cache.lines[iPart]);
	    for (i = 0; i < nPoints; i++) {
		Vect_append_point(ogr_info->cache.lines[iPart],
				  OGR_G_GetX(hRing, i), OGR_G_GetY(hRing, i),
				  OGR_G_GetZ(hRing, i));
	    }
	    npoints += ogr_info->cache.lines[iPart]->n_points;
	    
	    /* register boundary */
	    add_part(parts, iPart);
	    line = add_line(plus, offset, GV_BOUNDARY, ogr_info->cache.lines[iPart], FID, parts);
	    del_part(parts);

	    if (build < GV_BUILD_AREAS)
		continue;
	    
	    /* add area (each inner ring is also area) */
	    dig_line_box(ogr_info->cache.lines[iPart], &box);
	    dig_find_area_poly(ogr_info->cache.lines[iPart], &area_size);

	    if (area_size > 0)	        /* area clockwise */
		lines[0] = line;
	    else
		lines[0] = -line;

	    area = dig_add_area(plus, 1, lines, &box);

	    /* each area is also isle */
	    lines[0] = -lines[0];	/* island is counter clockwise */

	    isle = dig_add_isle(plus, 1, lines, &box);

	    if (build < GV_BUILD_ATTACH_ISLES)
		continue;
	    
	    if (iPart == 0) {	/* outer ring */
		outer_area = area;
	    }
	    else {		/* inner ring */
		struct P_isle *Isle;

		Isle = plus->Isle[isle];
		Isle->area = outer_area;

		dig_area_add_isle(plus, outer_area, isle);
	    }
	}
	
	if (build >= GV_BUILD_CENTROIDS) {
	    /* create virtual centroid */
	    ret = Vect_get_point_in_poly_isl((const struct line_pnts *) ogr_info->cache.lines[0],
					     (const struct line_pnts **) ogr_info->cache.lines + 1,
					     nRings - 1, &x, &y);
	    if (ret < -1) {
		G_warning(_("Unable to calculate centroid for area %d"),
			  outer_area);
	    }
	    else {
		struct P_area *Area;
		struct P_topo_c *topo;
		
		G_debug(4, "  Centroid: %f, %f", x, y);
		Vect_reset_line(ogr_info->cache.lines[0]);
		Vect_append_point(ogr_info->cache.lines[0], x, y, 0.0);
		line = add_line(plus, offset, GV_CENTROID, ogr_info->cache.lines[0],
				FID, parts);
		
		Line = plus->Line[line];
		topo = (struct P_topo_c *)Line->topo;
		topo->area = outer_area;
		
		/* register centroid to area */
		Area = plus->Area[outer_area];
		Area->centroid = line;
	    }
	}
	break;

    case wkbMultiPoint:
    case wkbMultiLineString:
    case wkbMultiPolygon:
    case wkbGeometryCollection:
	nParts = OGR_G_GetGeometryCount(hGeom);
	G_debug(4, "%d geoms -> next level", nParts);

	/* alloc space for parts if needed */
	if (nParts > ogr_info->cache.lines_alloc) {
	    ogr_info->cache.lines_alloc += nParts;
	    ogr_info->cache.lines = (struct line_pnts **) G_realloc(ogr_info->cache.lines,
								    ogr_info->cache.lines_alloc *
								    sizeof(struct line_pnts *));
	    ogr_info->cache.lines_types = (int *) G_realloc(ogr_info->cache.lines_types,
							    ogr_info->cache.lines_alloc * sizeof(int));
		
	    for (i = ogr_info->cache.lines_alloc - nParts; i < ogr_info->cache.lines_alloc; i++) {
		ogr_info->cache.lines[i] = Vect_new_line_struct();
		ogr_info->cache.lines_types[i] = -1;
	    }
	}
	
	/* go thru all parts */
	for (i = 0; i < nParts; i++) {
	    add_part(parts, i);
	    hGeom2 = OGR_G_GetGeometryRef(hGeom, i);
	    npoints += add_geometry_ogr(plus, ogr_info, hGeom2,
					FID, build, parts);
	    del_part(parts);
	}
	break;

    default:
	G_warning(_("OGR feature type %d not supported"), eType);
	break;
    }

    return npoints;
}

void build_ogr(struct Map_info *Map, int build)
{
    int iFeature, FID, npoints, nskipped;
    
    struct Format_info_ogr *ogr_info;
    
    OGRFeatureH hFeature;
    OGRGeometryH hGeom;
		
    struct geom_parts parts;
    
    ogr_info = &(Map->fInfo.ogr);
    
    /* initialize data structures */
    init_parts(&parts);

    /* Note: Do not use OGR_L_GetFeatureCount (it may scan all features) */
    OGR_L_ResetReading(ogr_info->layer);
    npoints = iFeature = nskipped = 0;
    G_message(_("Registering primitives..."));
    while ((hFeature = OGR_L_GetNextFeature(ogr_info->layer)) != NULL) {
	G_debug(3, "   Feature %d", iFeature);
	
	G_progress(++iFeature, 1e4);
	
	hGeom = OGR_F_GetGeometryRef(hFeature);
	if (hGeom == NULL) {
	    G_debug(3, "Feature %d without geometry skipped", iFeature);
	    OGR_F_Destroy(hFeature);
            nskipped++;
	    continue;
	}
	
	FID = (int) OGR_F_GetFID(hFeature);
	if (FID == OGRNullFID) {
	    G_debug(3, "OGR feature %d without ID skipped", iFeature);
	    OGR_F_Destroy(hFeature);
            nskipped++;
	    continue;
	}
	G_debug(4, "    FID = %d", FID);
	
	reset_parts(&parts);
	add_part(&parts, FID);
	npoints += add_geometry_ogr(&(Map->plus), ogr_info, hGeom,
				    FID, build, &parts);
	
	OGR_F_Destroy(hFeature);
    } /* while */
    G_progress(1, 1);
    
    G_message(_("%d primitives registered"), Map->plus.n_lines);
    G_message(_("%d vertices registered"), npoints);
    
    if (nskipped > 0)
        G_warning(_("%d %s without geometry skipped"), nskipped,
                  nskipped == 1 ? "feature" : "features");
        
    Map->plus.built = GV_BUILD_BASE;
    
    free_parts(&parts);
}
#endif /* HAVE_OGR */

/*!
   \brief Build pseudo-topology (for simple features) - internal use only

   See Vect_build_ogr() and Vect_build_pg() for implemetation issues.
   
   Build levels:
    - GV_BUILD_NONE
    - GV_BUILD_BASE
    - GV_BUILD_ATTACH_ISLES
    - GV_BUILD_CENTROIDS
    - GV_BUILD_ALL
   
   \param Map pointer to Map_info structure
   \param build build level

   \return 1 on success
   \return 0 on error
*/
int Vect__build_sfa(struct Map_info *Map, int build)
{
    struct Plus_head *plus;
    
    plus = &(Map->plus);
    
    /* check if upgrade or downgrade */
    if (build < plus->built) {
        /* -> downgrade */
        Vect__build_downgrade(Map, build);
        return 1;
    }
    
    /* -> upgrade */
    if (plus->built < GV_BUILD_BASE) {
        if (Map->format == GV_FORMAT_OGR ||
            Map->format == GV_FORMAT_OGR_DIRECT) {
#ifdef HAVE_OGR
            build_ogr(Map, build);
#else
            G_fatal_error(_("GRASS is not compiled with OGR support"));
#endif
        }
        else if (Map->format == GV_FORMAT_POSTGIS) {
#ifdef HAVE_POSTGRES
            build_pg(Map, build);
#else
            G_fatal_error(_("GRASS is not compiled with PostgreSQL support"));
#endif
        }
        else {
            G_fatal_error(_("%s: Native format unsupported"),
                          "Vect__build_sfa()");
        }
    }
    
    plus->built = build;
    
    return 1;
}

/*!
   \brief Dump feature index to file

   \param Map pointer to Map_info struct
   \param out file for output (stdout/stderr for example)

   \return 1 on success
   \return 0 on error
 */
int Vect_fidx_dump(const struct Map_info *Map, FILE *out)
{
    int i;
    const struct Format_info_offset *offset;

    if (Map->format != GV_FORMAT_OGR &&
	Map->format != GV_FORMAT_POSTGIS) {
	G_warning(_("Feature index is built only for non-native formats. "
		    "Nothing to dump."));
	return 0;
    }

    if (Map->format == GV_FORMAT_OGR)
	offset = &(Map->fInfo.ogr.offset);
    else
	offset = &(Map->fInfo.pg.offset);
    
    fprintf(out, "---------- FEATURE INDEX DUMP ----------\n");
    
    fprintf(out, "format: %s\n", Vect_maptype_info(Map));
    if (Vect_maptype(Map) == GV_FORMAT_POSTGIS &&
        Map->fInfo.pg.toposchema_name)
        fprintf(out, "topology: PostGIS\n");
    else
        fprintf(out, "topology: pseudo\n");
    fprintf(out, "feature type: %s\n", 
	    Vect_get_finfo_geometry_type(Map));
    fprintf(out, "number of features: %d\n\noffset : value (fid or part idx):\n",
	    Vect_get_num_lines(Map));
    for (i = 0; i < offset->array_num; i++) {
	fprintf(out, "%6d : %d\n", i, offset->array[i]);
    }

    return 1;
}

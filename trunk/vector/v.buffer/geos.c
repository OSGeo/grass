#include <float.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>
#include "local_proto.h"

#ifdef HAVE_GEOS

static int ring2pts(const GEOSGeometry *geom, struct line_pnts *Points)
{
    int i, ncoords;
    double x, y, z;
    const GEOSCoordSequence *seq = NULL;

    G_debug(3, "ring2pts()");

    Vect_reset_line(Points);
    if (!geom) {
	G_warning(_("Invalid GEOS geometry!"));
	return 0;
    }
    z = 0.0;
    ncoords = GEOSGetNumCoordinates(geom);
    if (!ncoords) {
	G_warning(_("No coordinates in GEOS geometry (can be ok for negative distance)!"));
	return 0;
    }
    seq = GEOSGeom_getCoordSeq(geom);
    for (i = 0; i < ncoords; i++) {
	GEOSCoordSeq_getX(seq, i, &x);
	GEOSCoordSeq_getY(seq, i, &y);
	if (x != x || x > DBL_MAX || x < -DBL_MAX)
	    G_fatal_error(_("Invalid x coordinate %f"), x);
	if (y != y || y > DBL_MAX || y < -DBL_MAX)
	    G_fatal_error(_("Invalid y coordinate %f"), y);
	Vect_append_point(Points, x, y, z);
    }

    return 1;
}

static int geom2ring(GEOSGeometry *geom, struct Map_info *Out,
                     struct Map_info *Buf,
                     struct spatial_index *si,
		     struct line_cats *Cats,
		     struct buf_contours **arr_bc,
		     int *buffers_count, int *arr_bc_alloc)
{
    int i, nrings, ngeoms, line_id;
    const GEOSGeometry *geom2;
    struct bound_box bbox;
    static struct line_pnts *Points = NULL;
    static struct line_cats *BCats = NULL;
    struct buf_contours *p = *arr_bc;

    G_debug(3, "geom2ring(): GEOS %s", GEOSGeomType(geom));

    if (!Points)
	Points = Vect_new_line_struct();
    if (!BCats)
	BCats = Vect_new_cats_struct();

    if (GEOSGeomTypeId(geom) == GEOS_LINESTRING ||
        GEOSGeomTypeId(geom) == GEOS_LINEARRING) {

	if (!ring2pts(geom, Points))
	    return 0;

	Vect_write_line(Out, GV_BOUNDARY, Points, BCats);
	line_id = Vect_write_line(Buf, GV_BOUNDARY, Points, Cats);
	/* add buffer to spatial index */
	Vect_get_line_box(Buf, line_id, &bbox);
	Vect_spatial_index_add_item(si, *buffers_count, &bbox);
	p[*buffers_count].outer = line_id;

	p[*buffers_count].inner_count = 0;
	*buffers_count += 1;
	if (*buffers_count >= *arr_bc_alloc) {
	    *arr_bc_alloc += 100;
	    p = G_realloc(p, *arr_bc_alloc * sizeof(struct buf_contours));
	    *arr_bc = p;
	}
    }
    else if (GEOSGeomTypeId(geom) == GEOS_POLYGON) {
	geom2 = GEOSGetExteriorRing(geom);
	if (!ring2pts(geom2, Points))
	    return 0;

	Vect_write_line(Out, GV_BOUNDARY, Points, BCats);
	line_id = Vect_write_line(Buf, GV_BOUNDARY, Points, Cats);
	/* add buffer to spatial index */
	Vect_get_line_box(Buf, line_id, &bbox);
	Vect_spatial_index_add_item(si, *buffers_count, &bbox);
	p[*buffers_count].outer = line_id;
	p[*buffers_count].inner_count = 0;

	nrings = GEOSGetNumInteriorRings(geom);
	
	if (nrings > 0) {

	    p[*buffers_count].inner_count = nrings;
	    p[*buffers_count].inner = G_malloc(nrings * sizeof(int));

	    for (i = 0; i < nrings; i++) {
		geom2 = GEOSGetInteriorRingN(geom, i);
		if (!ring2pts(geom2, Points)) {
		    G_fatal_error(_("Corrupt GEOS geometry"));
		}
		Vect_write_line(Out, GV_BOUNDARY, Points, BCats);
		line_id = Vect_write_line(Buf, GV_BOUNDARY, Points, BCats);
		p[*buffers_count].inner[i] = line_id;
	    }
	}
	*buffers_count += 1;
	if (*buffers_count >= *arr_bc_alloc) {
	    *arr_bc_alloc += 100;
	    p = G_realloc(p, *arr_bc_alloc * sizeof(struct buf_contours));
	    *arr_bc = p;
	}
    }
    else if (GEOSGeomTypeId(geom) == GEOS_MULTILINESTRING ||
             GEOSGeomTypeId(geom) == GEOS_MULTIPOLYGON ||
	     GEOSGeomTypeId(geom) == GEOS_GEOMETRYCOLLECTION) {

	G_debug(3, "GEOS %s", GEOSGeomType(geom));

	ngeoms = GEOSGetNumGeometries(geom);
	for (i = 0; i < ngeoms; i++) {
	    geom2 = GEOSGetGeometryN(geom, i);
	    geom2ring((GEOSGeometry *)geom2, Out, Buf, si, Cats,
	              arr_bc, buffers_count, arr_bc_alloc);
	}
    }
    else
	G_fatal_error(_("Unknown GEOS geometry type"));

    return 1;
}

int geos_buffer(struct Map_info *In, struct Map_info *Out,
                struct Map_info *Buf, int id, int type, double da,
		struct spatial_index *si,
		struct line_cats *Cats,
		struct buf_contours **arr_bc,
		int *buffers_count, int *arr_bc_alloc, int flat, int no_caps)
{
    GEOSGeometry *IGeom = NULL;
    GEOSGeometry *OGeom = NULL;
    
    G_debug(3, "geos_buffer(): id=%d", id);

    if (type == GV_AREA)
	IGeom = Vect_read_area_geos(In, id);
    else
	IGeom = Vect_read_line_geos(In, id, &type);

    /* GEOS code comment on the number of quadrant segments:
     * A value of 8 gives less than 2% max error in the buffer distance.
     * For a max error of < 1%, use QS = 12.
     * For a max error of < 0.1%, use QS = 18. */
#ifdef GEOS_3_3
    if (flat || no_caps) {
        GEOSBufferParams* geos_params = GEOSBufferParams_create();
        GEOSBufferParams_setEndCapStyle(geos_params,
                                        no_caps ? GEOSBUF_CAP_FLAT : GEOSBUF_CAP_SQUARE);

        OGeom = GEOSBufferWithParams(IGeom, geos_params, da);
        GEOSBufferParams_destroy(geos_params);
    }
    else {
        OGeom = GEOSBuffer(IGeom, da, 12);
    }
#else
    OGeom = GEOSBuffer(IGeom, da, 12);
#endif
    
    if (!OGeom) {
        G_fatal_error(_("Buffering failed (feature %d)"), id);
    }
    
    geom2ring(OGeom, Out, Buf, si, Cats, arr_bc, buffers_count, arr_bc_alloc);

    if (IGeom)
	GEOSGeom_destroy(IGeom);
    if (OGeom)
	GEOSGeom_destroy(OGeom);

    return 1;
}

#endif /* HAVE_GEOS */

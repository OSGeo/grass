#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>

#ifdef HAVE_GEOS

static int ring2pts(const GEOSGeometry *geom, struct line_pnts *Points)
{
    int i, ncoords;
    double x, y, z;
    const GEOSCoordSequence *seq = NULL;

    G_debug(3, "ring2pts()");

    z = 0.0;
    ncoords = GEOSGetNumCoordinates(geom);
    if (!ncoords)
	G_warning(_("No coordinates in GEOS geometry!"));
    seq = GEOSGeom_getCoordSeq(geom);
    for (i = 0; i < ncoords; i++) {
	GEOSCoordSeq_getX(seq, i, &x);
	GEOSCoordSeq_getY(seq, i, &y);
	Vect_append_point(Points, x, y, z);
    }

    return 1;
}

static int geom2ring(GEOSGeometry *geom, struct line_pnts **oPoints,
		       struct line_pnts ***iPoints, int *inner_count)
{
    int i, nrings, ngeoms, count;
    const GEOSGeometry *geom2;
    struct line_pnts *Points, **arrPoints = *iPoints;

    if (GEOSGeomTypeId(geom) == GEOS_LINESTRING ||
        GEOSGeomTypeId(geom) == GEOS_LINEARRING) {

	Points = *oPoints;
	if (Points->n_points == 0)
	    ring2pts(geom, Points);
	else {
	    count = *inner_count + 1;
	    arrPoints = G_realloc(arrPoints, count * sizeof(struct line_pnts *));
	    arrPoints[*inner_count] = Vect_new_line_struct();
	    ring2pts(geom, arrPoints[*inner_count]);
	    *inner_count = count;
	    *iPoints = arrPoints;
	}
    }
    else if (GEOSGeomTypeId(geom) == GEOS_POLYGON) {
	geom2 = GEOSGetExteriorRing(geom);
	Points = *oPoints;
	if (Points->n_points == 0)
	    ring2pts(geom2, Points);
	else {
	    count = *inner_count + 1;
	    arrPoints = G_realloc(arrPoints, count * sizeof(struct line_pnts *));
	    arrPoints[*inner_count] = Vect_new_line_struct();
	    ring2pts(geom2, arrPoints[*inner_count]);
	    *inner_count = count;
	    *iPoints = arrPoints;
	}

	nrings = GEOSGetNumInteriorRings(geom);
	
	if (nrings > 0) {

	    count = *inner_count + nrings;
	    arrPoints = G_realloc(arrPoints, count * sizeof(struct line_pnts *));

	    for (i = 0; i < nrings; i++) {
		geom2 = GEOSGetInteriorRingN(geom, i);
		arrPoints[*inner_count + i] = Vect_new_line_struct();
		ring2pts(geom2, arrPoints[*inner_count + i]);
	    }
	    *inner_count = count;
	    *iPoints = arrPoints;
	}
    }
    else if (GEOSGeomTypeId(geom) == GEOS_MULTILINESTRING ||
             GEOSGeomTypeId(geom) == GEOS_MULTIPOLYGON ||
	     GEOSGeomTypeId(geom) == GEOS_GEOMETRYCOLLECTION) {

	ngeoms = GEOSGetNumGeometries(geom);
	for (i = 0; i < ngeoms; i++) {
	    geom2 = GEOSGetGeometryN(geom, i);
	    geom2ring((GEOSGeometry *)geom2, oPoints, iPoints, inner_count);
	}
    }
    else
	G_fatal_error(_("Unknown GEOS geometry type"));

    return 1;
}

int geos_buffer(struct Map_info *In, int id, int type, double da,
                GEOSBufferParams *buffer_params, struct line_pnts **oPoints,
		struct line_pnts ***iPoints, int *inner_count)
{
    GEOSGeometry *IGeom = NULL;
    GEOSGeometry *OGeom = NULL;
    
    *oPoints = Vect_new_line_struct();
    *iPoints = NULL;
    *inner_count = 0;

    if (type == GV_AREA)
	IGeom = Vect_read_area_geos(In, id);
    else
	IGeom = Vect_read_line_geos(In, id, &type);

    OGeom = GEOSBufferWithParams(IGeom, buffer_params, da);
    
    if (!OGeom)
	G_warning(_("Buffering failed"));
    
    geom2ring(OGeom, oPoints, iPoints, inner_count);

    if (IGeom)
	GEOSGeom_destroy(IGeom);
    if (OGeom)
	GEOSGeom_destroy(OGeom);

    return 1;
}

#endif /* HAVE_GEOS */

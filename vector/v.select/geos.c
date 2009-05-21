#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/Vect.h>
#include <grass/glocale.h>

#include "proto.h"

#ifdef HAVE_GEOS

static int relate_geos(struct Map_info *, const GEOSGeometry *,
		       int, int, const char *, int);

int line_relate_geos(struct Map_info *BIn, const GEOSGeometry * AGeom,
		     int bline, int operator, const char *relate)
{
    return relate_geos(BIn, AGeom, bline, operator, relate, 0);
}

int area_relate_geos(struct Map_info *BIn, const GEOSGeometry * AGeom,
		     int barea, int operator, const char *relate)
{
    return relate_geos(BIn, AGeom, barea, operator, relate, 1);
}

int relate_geos(struct Map_info *BIn, const GEOSGeometry * AGeom,
		int bfid, int operator, const char *relate, int area)
{
    GEOSGeometry *BGeom = NULL;
    int found;

    found = 0;
    if (area)
	BGeom = Vect_read_area_geos(BIn, bfid);
    else
	BGeom = Vect_read_line_geos(BIn, bfid, NULL);

    if (!BGeom)
	return 0;

    /* 
       if (!BGeom) {
       if (area)
       G_fatal_error(_("Unable to read area id %d from vector map <%s>"),
       bfid, Vect_get_full_name(BIn));
       else
       G_fatal_error(_("Unable to read line id %d from vector map <%s>"),
       bfid, Vect_get_full_name(BIn));
       }
     */
    switch (operator) {
    case OP_EQUALS:{
	    if (GEOSEquals(AGeom, BGeom)) {
		found = 1;
		break;
	    }
	    break;
	}
    case OP_DISJOINT:{
	    if (GEOSDisjoint(AGeom, BGeom)) {
		found = 1;
		break;
	    }
	    break;
	}
    case OP_INTERSECTS:{
	    if (GEOSIntersects(AGeom, BGeom)) {
		found = 1;
		break;
	    }
	    break;
	}
    case OP_TOUCHES:{
	    if (GEOSTouches(AGeom, BGeom)) {
		found = 1;
		break;
	    }
	    break;
	}
    case OP_CROSSES:{
	    if (GEOSCrosses(AGeom, BGeom)) {
		found = 1;
		break;
	    }
	    break;
	}
    case OP_WITHIN:{
	    if (GEOSWithin(AGeom, BGeom)) {
		found = 1;
		break;
	    }
	    break;
	}
    case OP_CONTAINS:{
	    if (GEOSContains(AGeom, BGeom)) {
		found = 1;
		break;
	    }
	    break;
	}
    case OP_OVERLAPS:{
	    if (GEOSOverlaps(AGeom, BGeom)) {
		found = 1;
		break;
	    }
	    break;
	}
    case OP_RELATE:{
	    if (GEOSRelatePattern(AGeom, BGeom, relate)) {
		found = 1;
		break;
	    }
	    break;
	}
    default:
	break;
    }

    if (BGeom)
	GEOSGeom_destroy(BGeom);

    return found;
}
#endif /* HAVE_GEOS */

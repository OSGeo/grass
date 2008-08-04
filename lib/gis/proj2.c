#include <grass/gis.h>
#include <grass/glocale.h>

int G__projection_units(int n)
{
    switch (n) {
    case PROJECTION_XY:
	return 0;
    case PROJECTION_UTM:
	return METERS;
    case PROJECTION_SP:
	return FEET;
    case PROJECTION_LL:
	return DEGREES;
    default:
	return -1;
    }
}

char *G__unit_name(int unit, int plural)
{
    switch (unit) {
    case 0:
	return plural ? "units" : "unit";
    case METERS:
	return plural ? "meters" : "meter";
    case FEET:
	return plural ? "feet" : "foot";
    case DEGREES:
	return plural ? "degrees" : "degree";
    default:
	return NULL;
    }
}

char *G__projection_name(int n)
{
    switch (n) {
    case PROJECTION_XY:
	return "x,y";
    case PROJECTION_UTM:
	return "UTM";
    case PROJECTION_SP:
	return "State Plane";
    case PROJECTION_LL:
	return _("Latitude-Longitude");
    case PROJECTION_OTHER:
	return _("Other Projection");
    default:
	return NULL;
    }
}

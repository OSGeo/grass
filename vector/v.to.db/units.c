#include <string.h>
#include "global.h"

int conv_units(void)
{
    int i, rad;
    double f, sq_f;

    rad = 0;
    f = G_meters_to_units_factor(options.units);
    sq_f = G_meters_to_units_factor_sq(options.units);

    if (options.units == U_RADIANS)
        rad = 1;

    switch (options.option) {
    case O_LENGTH:
    case O_PERIMETER:
        for (i = 0; i < vstat.rcat; i++)
            Values[i].d1 *= f;
        break;
    case O_AREA:
        for (i = 0; i < vstat.rcat; i++)
            Values[i].d1 *= sq_f;
        break;
    case O_COMPACT:
    case O_FD:
        for (i = 0; i < vstat.rcat; i++) {
            Values[i].d1 *= sq_f;
            Values[i].d2 *= f;
        }
        break;
    case O_AZIMUTH:
        if (rad == 0) {
            for (i = 0; i < vstat.rcat; i++) {
                if (Values[i].d1 > 0)
                    Values[i].d1 = Values[i].d1 * (180 / M_PI);
            }
        }
        break;
    }

    return 0;
}

void get_unit_name(char *unit_name)
{
    char *tmp;
    switch (options.option) {
    case O_AREA:
        switch (options.units) {
        case U_MILES:
            tmp = "square miles";
            break;
        case U_FEET:
            tmp = "square feet";
            break;
        case U_KILOMETERS:
            tmp = "square kilometers";
            break;
        case U_ACRES:
            tmp = "acres";
            break;
        case U_HECTARES:
            tmp = "hectares";
            break;
        case U_METERS:
        default:
            tmp = "square meters";
            break;
        }
        break;
    case O_LENGTH:
    case O_PERIMETER:
        switch (options.units) {
        case U_MILES:
            tmp = "miles";
            break;
        case U_FEET:
            tmp = "feet";
            break;
        case U_KILOMETERS:
            tmp = "kilometers";
            break;
        case U_METERS:
        default:
            tmp = "meters";
            break;
        }
        break;
    case O_AZIMUTH:
        switch (options.units) {
        case U_DEGREES:
            tmp = "degrees";
            break;
        case U_RADIANS:
        default:
            tmp = "radians";
            break;
        }
        break;
    default:
        tmp = "";
        break;
    }
    strncpy(unit_name, tmp, 20);
}

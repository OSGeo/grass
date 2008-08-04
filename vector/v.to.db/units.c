#include <string.h>
#include "global.h"

int conv_units()
{
    int i;
    double f = 1.0, sq_f = 1.0;

    switch (options.units) {
    case U_METERS:
	f = 1.0;
	sq_f = 1.0;
	break;

    case U_KILOMETERS:
	f = 1.0e-3;
	sq_f = 1.0e-6;
	break;

    case U_ACRES:
	sq_f = 2.47105381467165e-4;	/* 640 acres in a sq mile */
	break;

    case U_HECTARES:
	sq_f = 1.0e-4;
	break;

    case U_MILES:
	f = 6.21371192237334e-4;	/*  1 / (0.0254 * 12 * 5280)    */
	sq_f = 3.86102158542446e-7;	/*  1 / (0.0254 * 12 * 5280)^2  */
	break;

    case U_FEET:
	f = 3.28083989501312;	/*  1 / (0.0254 * 12)    */
	sq_f = 10.7639104167097;	/*  1 / (0.0254 * 12)^2  */
	break;
    }

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

    }

    return 0;
}

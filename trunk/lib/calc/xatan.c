
#include <stdlib.h>
#include <math.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/calc.h>

/**********************************************************************
atan(x)     range [-90,90]
atan(x,y) = atan(y/x) range[0,360]

  if floating point exception occurs during the evaluation of atan(x)
  the result is NULL

  note: result is in degrees
**********************************************************************/

#define RADIANS_TO_DEGREES (180.0 / M_PI)

int f_atan(int argc, const int *argt, void **args)
{
    DCELL *res = args[0];
    DCELL *arg1 = args[1];
    DCELL *arg2;
    int i;

    if (argc < 1)
	return E_ARG_LO;
    if (argc > 2)
	return E_ARG_HI;

    if (argt[0] != DCELL_TYPE)
	return E_RES_TYPE;

    if (argt[1] != DCELL_TYPE)
	return E_ARG_TYPE;

    if (argc > 1 && argt[2] != DCELL_TYPE)
	return E_ARG_TYPE;

    arg2 = (argc > 1) ? args[2] : NULL;

    for (i = 0; i < columns; i++)
	if (IS_NULL_D(&arg1[i]))
	    SET_NULL_D(&res[i]);
	else if (argc > 1 && IS_NULL_D(&arg2[i]))
	    SET_NULL_D(&res[i]);

	else {
	    floating_point_exception = 0;
	    if (argc == 1)
		res[i] = RADIANS_TO_DEGREES * atan(arg1[i]);
	    else {
		res[i] = RADIANS_TO_DEGREES * atan2(arg2[i], arg1[i]);
		if (res[i] < 0)
		    res[i] += 360.0;
	    }
	    if (floating_point_exception)
		SET_NULL_D(&res[i]);
	}

    return 0;
}

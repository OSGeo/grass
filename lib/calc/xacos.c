
#include <stdlib.h>
#include <math.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/calc.h>

/**********************************************************************
acos(x)  [0 and PI]

  if floating point exception occurs during the evaluation of acos(x)
  the result is NULL

  note: result is in degrees
**********************************************************************/

#define RADIANS_TO_DEGREES (180.0 / M_PI)

int f_acos(int argc, const int *argt, void **args)
{
    DCELL *res = args[0];
    DCELL *arg1 = args[1];
    int i;

    if (argc < 1)
	return E_ARG_LO;
    if (argc > 1)
	return E_ARG_HI;

    if (argt[0] != DCELL_TYPE)
	return E_RES_TYPE;

    if (argt[1] != DCELL_TYPE)
	return E_ARG_TYPE;

    for (i = 0; i < columns; i++)
	if (IS_NULL_D(&arg1[i]))
	    SET_NULL_D(&res[i]);
	else {
	    floating_point_exception = 0;
	    res[i] = RADIANS_TO_DEGREES * acos(arg1[i]);
	    if (floating_point_exception)
		SET_NULL_D(&res[i]);
	}

    return 0;
}

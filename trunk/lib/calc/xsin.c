
#include <math.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/calc.h>

/**********************************************************************
sin(x) 

  if floating point exception occurs during the evaluation of sin(x)
  the result is NULL

  note: x is in degrees.
**********************************************************************/

#define DEGREES_TO_RADIANS (M_PI / 180.0)

int f_sin(int argc, const int *argt, void **args)
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
	    res[i] = sin(arg1[i] * DEGREES_TO_RADIANS);
	    if (floating_point_exception)
		SET_NULL_D(&res[i]);
	}

    return 0;
}

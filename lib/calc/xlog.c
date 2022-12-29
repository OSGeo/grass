
#include <math.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/calc.h>

/**********************************************************************
log(x) 
log(x,b)

  first form computes the natural log of x = ln(x)
  second form computes log of x base b = ln(x)/ln(b)

  if x is non-positive, or floating point exception occurs while
  computing ln(x), the result is NULL

  if b is non-positive, or 1.0, or floating point exception occurs while
  computing ln(b), the result is NULL
**********************************************************************/

int f_log(int argc, const int *argt, void **args)
{
    DCELL *res = args[0];
    DCELL *arg1 = args[1];
    DCELL *arg2 = (argc >= 2) ? args[2] : (DCELL *) 0;
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

    for (i = 0; i < columns; i++)
	if (IS_NULL_D(&arg1[i]) || (arg1[i] <= 0.0))
	    SET_NULL_D(&res[i]);
	else if (argc > 1 && (IS_NULL_D(&arg2[i]) || (arg2[i] <= 0.0)))
	    SET_NULL_D(&res[i]);
	else {
	    floating_point_exception = 0;
	    res[i] = (argc > 1)
		? log(arg1[i]) / log(arg2[i])
		: log(arg1[i]);
	    if (floating_point_exception)
		SET_NULL_D(&res[i]);
	}

    return 0;
}

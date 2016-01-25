
#include <math.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/calc.h>

/**********************************************************************
sqrt(x) 

  if floating point exception occurs during the evaluation of sqrt(x)
  the result is NULL
**********************************************************************/

int f_sqrt(int argc, const int *argt, void **args)
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
	if (IS_NULL_D(&arg1[i]) || (arg1[i] < 0.0))
	    SET_NULL_D(&res[i]);
	else {
	    floating_point_exception = 0;
	    res[i] = sqrt(arg1[i]);
	    if (floating_point_exception)
		SET_NULL_D(&res[i]);
	}

    return 0;
}

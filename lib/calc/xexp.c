#include <math.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/calc.h>

/**********************************************************************
exp(x)   computes e raised to power x
exp(x,y) computes x raised to power y

  if x is negative and y is not an integer, exp(x,y) is NULL
**********************************************************************/

int f_exp(int argc, const int *argt, void **args)
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

    arg2 = (argc > 1) ? args[2] : NULL;

    for (i = 0; i < columns; i++)
        if (IS_NULL_D(&arg1[i]))
            SET_NULL_D(&res[i]);
        else if (argc > 1 && IS_NULL_D(&arg2[i]))
            SET_NULL_D(&res[i]);
        else if (argc > 1 && arg1[i] < 0 && arg2[i] != ceil(arg2[i]))
            SET_NULL_D(&res[i]);
        else
            res[i] = (argc > 1) ? pow(arg1[i], arg2[i]) : exp(arg1[i]);

    return 0;
}

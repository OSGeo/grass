
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/calc.h>

/**********************************************************************
isnull(x)
  return 1 if x is null, 0 otherwise
**********************************************************************/

int f_isnull(int argc, const int *argt, void **args)
{
    int *res = args[0];
    int i;

    if (argc < 1)
	return E_ARG_LO;
    if (argc > 1)
	return E_ARG_HI;

    if (argt[0] != CELL_TYPE)
	return E_RES_TYPE;

    switch (argt[1]) {
    case CELL_TYPE:
	{
	    CELL *arg1 = args[1];

	    for (i = 0; i < columns; i++)
		res[i] = IS_NULL_C(&arg1[i]) ? 1 : 0;
	    return 0;
	}
    case FCELL_TYPE:
	{
	    FCELL *arg1 = args[1];

	    for (i = 0; i < columns; i++)
		res[i] = IS_NULL_F(&arg1[i]) ? 1 : 0;
	    return 0;
	}
    case DCELL_TYPE:
	{
	    DCELL *arg1 = args[1];

	    for (i = 0; i < columns; i++)
		res[i] = IS_NULL_D(&arg1[i]) ? 1 : 0;
	    return 0;
	}
    default:
	return E_INV_TYPE;
    }
}

int c_isnull(int argc, int *argt)
{
    if (argc < 1)
	return E_ARG_LO;
    if (argc > 1)
	return E_ARG_HI;

    argt[0] = CELL_TYPE;

    return 0;
}


#include <stdlib.h>

#include <grass/config.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/calc.h>

/****************************************************************
rand(lo,hi) random values between a and b
****************************************************************/

int f_rand(int argc, const int *argt, void **args)
{
    int i;

    if (argc < 2)
	return E_ARG_LO;
    if (argc > 2)
	return E_ARG_HI;

    switch (argt[0]) {
    case CELL_TYPE:
	{
	    CELL *res = args[0];
	    CELL *arg1 = args[1];
	    CELL *arg2 = args[2];

	    for (i = 0; i < columns; i++) {
		unsigned int x = (unsigned int)G_mrand48();
		int lo = arg1[i];
		int hi = arg2[i];

		if (lo > hi) {
		    int tmp = lo;

		    lo = hi;
		    hi = tmp;
		}
		res[i] = (lo == hi) ? lo : lo + x % (unsigned int)(hi - lo);
	    }
	    return 0;
	}
    case FCELL_TYPE:
	{
	    FCELL *res = args[0];
	    FCELL *arg1 = args[1];
	    FCELL *arg2 = args[2];

	    for (i = 0; i < columns; i++) {
		double x = G_drand48();
		FCELL lo = arg1[i];
		FCELL hi = arg2[i];

		if (lo > hi) {
		    FCELL tmp = lo;

		    lo = hi;
		    hi = tmp;
		}
		res[i] = (FCELL) (lo + x * (hi - lo));
	    }
	    return 0;
	}
    case DCELL_TYPE:
	{
	    DCELL *res = args[0];
	    DCELL *arg1 = args[1];
	    DCELL *arg2 = args[2];

	    for (i = 0; i < columns; i++) {
		double x = G_drand48();
		DCELL lo = arg1[i];
		DCELL hi = arg2[i];

		if (lo > hi) {
		    DCELL tmp = lo;

		    lo = hi;
		    hi = tmp;
		}
		res[i] = lo + x * (hi - lo);
	    }
	    return 0;
	}
    default:
	return E_INV_TYPE;
    }
}

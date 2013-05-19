
#include <limits.h>
#include <math.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include "globals.h"
#include "expression.h"
#include "func_proto.h"

/**********************************************************************
round(x) rounds x to nearest integer
round(x, y) rounds x to y decimal places

  if input is CELL (which is an integer already)
  and the number of decimal places is 0
  the input argument (argv[0]) is simply copied to the output cell.

  if the input is double, the input is rounded by adding .5 to positive
  numbers, and subtracting .5 from negatives.
**********************************************************************/

/* d_round(x) rounds x to nearest integer value, handles negative correctly */

static double d_round(double x)
{
    if (!IS_NULL_D(&x)) {
	x = floor(x + 0.5);
    }

    return x;
}

/**********************************************************************/

/* d_roundd(x, y) rounds x to y decimal places, handles negative correctly */

static double d_roundd(double x, int y)
{
    if (!IS_NULL_D(&x)) {
	double pow10, intx, sgn = 1.;

	if (x < 0.) {
	    sgn = -1.;
	    x = -x;
	}
	if (y == 0)
	    return (double)(sgn * d_round(x));
	else if (y > 0) {
	    pow10 = pow(10., y);
	    intx = floor(x);
	    return (double)(sgn * (intx + d_round((double)((x - intx) * pow10)) / pow10));
	}
	else {
	    pow10 = pow(10., -y);
	    return (double)(sgn * d_round((double)(x / pow10)) * pow10);
	}
    }

    return x;
}

/**********************************************************************/
int f_round(int argc, const int *argt, void **args)
{
    int i;

    if (argc < 1)
	return E_ARG_LO;
    if (argc > 2)
	return E_ARG_HI;

    if (argc == 1) {
	switch (argt[1]) {
	case CELL_TYPE:
	    {
		CELL *arg1 = args[1];
		CELL *res = args[0];

		for (i = 0; i < columns; i++)
		    if (IS_NULL_C(&arg1[i]))
			SET_NULL_C(&res[i]);
		    else
			res[i] = arg1[i];
		return 0;
	    }
	case FCELL_TYPE:
	    {
		FCELL *arg1 = args[1];
		FCELL *res = args[0];

		for (i = 0; i < columns; i++)
		    if (IS_NULL_F(&arg1[i]))
			SET_NULL_F(&res[i]);
		    else
			res[i] = d_round(arg1[i]);
		return 0;
	    }
	case DCELL_TYPE:
	    {
		DCELL *arg1 = args[1];
		DCELL *res = args[0];

		for (i = 0; i < columns; i++)
		    if (IS_NULL_D(&arg1[i]))
			SET_NULL_D(&res[i]);
		    else
			res[i] = d_round(arg1[i]);
		return 0;
	    }
	default:
	    return E_INV_TYPE;
	}
    }
    else {    /* argc == 2 */
	int digits;
	DCELL *arg2;

	switch (argt[1]) {
	case CELL_TYPE:
	    {
		CELL *arg1 = args[1];
		CELL *res = args[0];

		arg2 = args[2];

		for (i = 0; i < columns; i++)
		    if (IS_NULL_C(&arg1[i]))
			SET_NULL_C(&res[i]);
		    else {
			if (arg2[i] >= 0)
			    digits = d_round(arg2[i]) + 0.5;
			else
			    digits = d_round(arg2[i]) - 0.5;
			if (digits >= 0)
			    res[i] = arg1[i];
			else {
			    if (arg1[i] >= 0)
				res[i] = d_roundd(arg1[i], digits) + 0.5;
			    else
				res[i] = d_roundd(arg1[i], digits) - 0.5;
			}
		    }
		return 0;
	    }
	case FCELL_TYPE:
	    {
		FCELL *arg1 = args[1];
		FCELL *res = args[0];

		arg2 = args[2];

		for (i = 0; i < columns; i++)
		    if (IS_NULL_F(&arg1[i]))
			SET_NULL_F(&res[i]);
		    else {
			if (arg2[i] >= 0)
			    digits = d_round(arg2[i]) + 0.5;
			else
			    digits = d_round(arg2[i]) - 0.5;
			res[i] = d_roundd(arg1[i], digits);
		    }
		return 0;
	    }
	case DCELL_TYPE:
	    {
		DCELL *arg1 = args[1];
		DCELL *res = args[0];

		arg2 = args[2];

		for (i = 0; i < columns; i++)
		    if (IS_NULL_D(&arg1[i]))
			SET_NULL_D(&res[i]);
		    else {
			if (arg2[i] >= 0)
			    digits = d_round(arg2[i]) + 0.5;
			else
			    digits = d_round(arg2[i]) - 0.5;
			res[i] = d_roundd(arg1[i], digits);
		    }
		return 0;
	    }
	default:
	    return E_INV_TYPE;
	}
    }
}

int c_round(int argc, int *argt)
{
    if (argc < 1)
	return E_ARG_LO;
    if (argc > 2)
	return E_ARG_HI;

    argt[0] = argt[1];
    
    if (argc == 2)
	argt[2] = DCELL_TYPE;

    return 0;
}

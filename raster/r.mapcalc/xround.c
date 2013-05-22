
#include <limits.h>
#include <math.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include "globals.h"
#include "expression.h"
#include "func_proto.h"

/**********************************************************************
round(x, step, start)

  rounds x to nearest value in the sequence
    y[i] = i * step + start

**********************************************************************/

/* i_round(x) rounds x to nearest value, handles negative correctly */

static double i_round(double x, double step, double start)
{
    x -= start;
    x /= step;
    x = floor(x + 0.5);
    x *= step;
    x += start;
    return x;
}

/**********************************************************************/

int f_round(int argc, const int *argt, void **args)
{
    const DCELL *arg1 = args[1];
    int i;

    if (argc < 1)
	return E_ARG_LO;
    if (argc > 3)
	return E_ARG_HI;

    if (argc == 1 && argt[0] != CELL_TYPE)
	return E_RES_TYPE;
    if (argt[1] != DCELL_TYPE)
	return E_ARG_TYPE;
    if (argc > 1 && argt[2] != DCELL_TYPE)
	return E_ARG_TYPE;
    if (argc > 2 && argt[3] != DCELL_TYPE)
	return E_ARG_TYPE;

    if (argc == 1) {
	CELL *res = args[0];

	for (i = 0; i < columns; i++) {
	    if (IS_NULL_D(&arg1[i]))
		SET_NULL_C(&res[i]);
	    else {
		DCELL x = i_round(arg1[i], 1.0, 0.0);
		if (x > 2147483647.0 || x < -2147483647.0)
		    SET_NULL_C(&res[i]);
		else
		    res[i] = (CELL) x;
	    }
	}
	return 0;
    }
    else if (argc == 2) {
	const DCELL *arg2 = args[2];

	switch (argt[0]) {
        case CELL_TYPE:
            {
                CELL *res = args[0];
    
                for (i = 0; i < columns; i++) {
                    if (IS_NULL_D(&arg1[i]))
                        SET_NULL_C(&res[i]);
                    else if (IS_NULL_D(&arg2[i]))
                        SET_NULL_C(&res[i]);
                    else {
                        DCELL x = i_round(arg1[i], arg2[i], 0.0);
                        if (x > 2147483647.0 || x < -2147483647.0)
                            SET_NULL_C(&res[i]);
                        else
                            res[i] = (CELL) x;
                    }
                }
                return 0;
            }
        case FCELL_TYPE:
            {
                FCELL *res = args[0];
    
                for (i = 0; i < columns; i++)
                    if (IS_NULL_D(&arg1[i]))
                        SET_NULL_F(&res[i]);
                    else if (IS_NULL_D(&arg2[i]))
                        SET_NULL_F(&res[i]);
                    else
                        res[i] = (FCELL) i_round(arg1[i], arg2[i], 0.0);
                return 0;
            }
        case DCELL_TYPE:
            {
                DCELL *res = args[0];
    
                for (i = 0; i < columns; i++)
                    if (IS_NULL_D(&arg1[i]))
                        SET_NULL_D(&res[i]);
                    else if (IS_NULL_D(&arg2[i]))
                        SET_NULL_D(&res[i]);
                    else
                        res[i] = (DCELL) i_round(arg1[i], arg2[i], 0.0);
                return 0;
            }
        default:
            return E_INV_TYPE;
        }
    }
    else if (argc == 3) {
	const DCELL *arg2 = args[2];
	const DCELL *arg3 = args[3];

	switch (argt[0]) {
        case CELL_TYPE:
            {
                CELL *res = args[0];
    
                for (i = 0; i < columns; i++) {
                    if (IS_NULL_D(&arg1[i]))
                        SET_NULL_C(&res[i]);
                    else if (IS_NULL_D(&arg2[i]))
                        SET_NULL_C(&res[i]);
                    else if (IS_NULL_D(&arg3[i]))
                        SET_NULL_C(&res[i]);
                    else {
                        DCELL x = i_round(arg1[i], arg2[i], arg3[i]);
                        if (x > 2147483647.0 || x < -2147483647.0)
                            SET_NULL_C(&res[i]);
                        else
                            res[i] = (CELL) x;
                    }
                }
                return 0;
            }
        case FCELL_TYPE:
            {
                FCELL *res = args[0];
    
                for (i = 0; i < columns; i++)
                    if (IS_NULL_D(&arg1[i]))
                        SET_NULL_F(&res[i]);
                    else if (IS_NULL_D(&arg2[i]))
                        SET_NULL_F(&res[i]);
                    else if (IS_NULL_D(&arg3[i]))
                        SET_NULL_F(&res[i]);
                    else
                        res[i] = (FCELL) i_round(arg1[i], arg2[i], arg3[i]);
                return 0;
            }
        case DCELL_TYPE:
            {
                DCELL *res = args[0];
    
                for (i = 0; i < columns; i++)
                    if (IS_NULL_D(&arg1[i]))
                        SET_NULL_D(&res[i]);
                    else if (IS_NULL_D(&arg2[i]))
                        SET_NULL_D(&res[i]);
                    else if (IS_NULL_D(&arg3[i]))
                        SET_NULL_D(&res[i]);
                    else
                        res[i] = (DCELL) i_round(arg1[i], arg2[i], arg3[i]);
                return 0;
            }
        default:
            return E_INV_TYPE;
        }
    }
    else
	return E_WTF;
}

int c_round(int argc, int *argt)
{
    if (argc < 1)
	return E_ARG_LO;
    if (argc > 3)
	return E_ARG_HI;

    argt[0] = CELL_TYPE;
    if (argc > 1 && argt[0] < argt[2])
	argt[0] = argt[2];
    if (argc > 2 && argt[0] < argt[3])
	argt[0] = argt[3];

    argt[1] = DCELL_TYPE;
    if (argc > 1)
	argt[2] = DCELL_TYPE;
    if (argc > 2)
	argt[3] = DCELL_TYPE;

    return 0;
}


#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/calc.h>

/********************************************************************
 if(a)        1,0,1  1 if a is non zero, 0 otherwise
 if(a,b)      b,0,b  b if a is non zero, 0 otherwise
 if(a,b,c)    b,c,b  b if a is non zero, c otherwise
 if(a,b,c,d)  d,c,b  b if a is positive, c if a is zero, d if a is negative
********************************************************************/

static int f_if_i(int argc, const int *argt, void **args)
{
    CELL *res = args[0];
    DCELL *arg1 = args[1];
    CELL *arg2 = (argc >= 2) ? args[2] : NULL;
    CELL *arg3 = (argc >= 3) ? args[3] : NULL;
    CELL *arg4 = (argc >= 4) ? args[4] : NULL;
    int i;

    switch (argc) {
    case 0:
	return E_ARG_LO;
    case 1:
	for (i = 0; i < columns; i++)
	    if (IS_NULL_D(&arg1[i]))
		SET_NULL_C(&res[i]);
	    else
		res[i] = arg1[i] != 0.0 ? 1 : 0;
	break;
    case 2:
	for (i = 0; i < columns; i++)
	    if (IS_NULL_D(&arg1[i]))
		SET_NULL_C(&res[i]);
	    else if (arg1[i] == 0.0)
		res[i] = 0;
	    else {
		if (IS_NULL_C(&arg2[i]))
		    SET_NULL_C(&res[i]);
		else
		    res[i] = arg2[i];
	    }
	break;
    case 3:
	for (i = 0; i < columns; i++)
	    if (IS_NULL_D(&arg1[i]))
		SET_NULL_C(&res[i]);
	    else if (arg1[i] == 0.0) {
		if (IS_NULL_C(&arg3[i]))
		    SET_NULL_C(&res[i]);
		else
		    res[i] = arg3[i];
	    }
	    else {
		if (IS_NULL_C(&arg2[i]))
		    SET_NULL_C(&res[i]);
		else
		    res[i] = arg2[i];
	    }
	break;
    case 4:
	for (i = 0; i < columns; i++)
	    if (IS_NULL_D(&arg1[i]))
		SET_NULL_C(&res[i]);
	    else if (arg1[i] == 0.0) {
		if (IS_NULL_C(&arg3[i]))
		    SET_NULL_C(&res[i]);
		else
		    res[i] = arg3[i];
	    }
	    else if (arg1[i] > 0.0) {
		if (IS_NULL_C(&arg2[i]))
		    SET_NULL_C(&res[i]);
		else
		    res[i] = arg2[i];
	    }
	    else {		/* (arg1[i] < 0.0) */

		if (IS_NULL_C(&arg4[i]))
		    SET_NULL_C(&res[i]);
		else
		    res[i] = arg4[i];
	    }
	break;
    default:
	return E_ARG_HI;
    }

    return 0;
}

static int f_if_f(int argc, const int *argt, void **args)
{
    FCELL *res = args[0];
    DCELL *arg1 = args[1];
    FCELL *arg2 = (argc >= 2) ? args[2] : NULL;
    FCELL *arg3 = (argc >= 3) ? args[3] : NULL;
    FCELL *arg4 = (argc >= 4) ? args[4] : NULL;
    int i;

    switch (argc) {
    case 0:
	return E_ARG_LO;
    case 1:
	return E_ARG_TYPE;
    case 2:
	for (i = 0; i < columns; i++)
	    if (IS_NULL_D(&arg1[i]))
		SET_NULL_F(&res[i]);
	    else if (arg1[i] == 0.0)
		res[i] = 0.0;
	    else {
		if (IS_NULL_F(&arg2[i]))
		    SET_NULL_F(&res[i]);
		else
		    res[i] = arg2[i];
	    }
	break;
    case 3:
	for (i = 0; i < columns; i++)
	    if (IS_NULL_D(&arg1[i]))
		SET_NULL_F(&res[i]);
	    else if (arg1[i] == 0.0) {
		if (IS_NULL_F(&arg3[i]))
		    SET_NULL_F(&res[i]);
		else
		    res[i] = arg3[i];
	    }
	    else {
		if (IS_NULL_F(&arg2[i]))
		    SET_NULL_F(&res[i]);
		else
		    res[i] = arg2[i];
	    }
	break;
    case 4:
	for (i = 0; i < columns; i++)
	    if (IS_NULL_D(&arg1[i]))
		SET_NULL_F(&res[i]);
	    else if (arg1[i] == 0.0) {
		if (IS_NULL_F(&arg3[i]))
		    SET_NULL_F(&res[i]);
		else
		    res[i] = arg3[i];
	    }
	    else if (arg1[i] > 0.0) {
		if (IS_NULL_F(&arg2[i]))
		    SET_NULL_F(&res[i]);
		else
		    res[i] = arg2[i];
	    }
	    else {		/* (arg1[i] < 0.0) */

		if (IS_NULL_F(&arg4[i]))
		    SET_NULL_F(&res[i]);
		else
		    res[i] = arg4[i];
	    }
	break;
    default:
	return E_ARG_HI;
    }

    return 0;
}

static int f_if_d(int argc, const int *argt, void **args)
{
    DCELL *res = args[0];
    DCELL *arg1 = args[1];
    DCELL *arg2 = (argc >= 2) ? args[2] : NULL;
    DCELL *arg3 = (argc >= 3) ? args[3] : NULL;
    DCELL *arg4 = (argc >= 4) ? args[4] : NULL;
    int i;

    switch (argc) {
    case 0:
	return E_ARG_LO;
    case 1:
	return E_ARG_TYPE;
    case 2:
	for (i = 0; i < columns; i++)
	    if (IS_NULL_D(&arg1[i]))
		SET_NULL_D(&res[i]);
	    else if (arg1[i] == 0.0)
		res[i] = 0.0;
	    else {
		if (IS_NULL_D(&arg2[i]))
		    SET_NULL_D(&res[i]);
		else
		    res[i] = arg2[i];
	    }
	break;
    case 3:
	for (i = 0; i < columns; i++)
	    if (IS_NULL_D(&arg1[i]))
		SET_NULL_D(&res[i]);
	    else if (arg1[i] == 0.0) {
		if (IS_NULL_D(&arg3[i]))
		    SET_NULL_D(&res[i]);
		else
		    res[i] = arg3[i];
	    }
	    else {
		if (IS_NULL_D(&arg2[i]))
		    SET_NULL_D(&res[i]);
		else
		    res[i] = arg2[i];
	    }
	break;
    case 4:
	for (i = 0; i < columns; i++)
	    if (IS_NULL_D(&arg1[i]))
		SET_NULL_D(&res[i]);
	    else if (arg1[i] == 0.0) {
		if (IS_NULL_D(&arg3[i]))
		    SET_NULL_D(&res[i]);
		else
		    res[i] = arg3[i];
	    }
	    else if (arg1[i] > 0.0) {
		if (IS_NULL_D(&arg2[i]))
		    SET_NULL_D(&res[i]);
		else
		    res[i] = arg2[i];
	    }
	    else {		/* (arg1[i] < 0.0) */

		if (IS_NULL_D(&arg4[i]))
		    SET_NULL_D(&res[i]);
		else
		    res[i] = arg4[i];
	    }
	break;
    default:
	return E_ARG_HI;
    }

    return 0;
}

int f_if(int argc, const int *argt, void **args)
{
    if (argc < 1)
	return E_ARG_LO;
    if (argc > 4)
	return E_ARG_HI;

    if (argt[1] != DCELL_TYPE)
	return E_ARG_TYPE;
    if (argc >= 2 && argt[2] != argt[0])
	return E_ARG_TYPE;
    if (argc >= 3 && argt[3] != argt[0])
	return E_ARG_TYPE;
    if (argc >= 4 && argt[4] != argt[0])
	return E_ARG_TYPE;

    switch (argt[0]) {
    case CELL_TYPE:
	return f_if_i(argc, argt, args);
    case FCELL_TYPE:
	return f_if_f(argc, argt, args);
    case DCELL_TYPE:
	return f_if_d(argc, argt, args);
    default:
	return E_INV_TYPE;
    }
}

int c_if(int argc, int *argt)
{
    if (argc < 1)
	return E_ARG_LO;
    if (argc > 4)
	return E_ARG_HI;

    argt[0] = CELL_TYPE;

    if (argc >= 2 && argt[2] == FCELL_TYPE)
	argt[0] = FCELL_TYPE;
    if (argc >= 3 && argt[3] == FCELL_TYPE)
	argt[0] = FCELL_TYPE;
    if (argc >= 4 && argt[4] == FCELL_TYPE)
	argt[0] = FCELL_TYPE;

    if (argc >= 2 && argt[2] == DCELL_TYPE)
	argt[0] = DCELL_TYPE;
    if (argc >= 3 && argt[3] == DCELL_TYPE)
	argt[0] = DCELL_TYPE;
    if (argc >= 4 && argt[4] == DCELL_TYPE)
	argt[0] = DCELL_TYPE;

    argt[1] = DCELL_TYPE;
    if (argc >= 2)
	argt[2] = argt[0];
    if (argc >= 3)
	argt[3] = argt[0];
    if (argc >= 4)
	argt[4] = argt[0];

    return 0;
}

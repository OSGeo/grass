
#include <stdlib.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/calc.h>

/**********************************************************************
mode(x1,x2,..,xn)
   return mode of arguments
**********************************************************************/

static int dcmp(const void *aa, const void *bb)
{
    const double *a = aa;
    const double *b = bb;

    if (*a < *b)
	return -1;
    if (*a > *b)
	return 1;
    return 0;
}

static double mode(double *value, int argc)
{
    double mode_v;
    int mode_n = 0;
    int i;

    qsort(value, argc, sizeof(double), dcmp);

    for (i = 0; i < argc;) {
	int n = 1;
	double v = value[i];

	for (i++; i < argc; i++) {
	    if (value[i] != v)
		break;
	    n++;
	}

	if (n < mode_n)
	    continue;

	mode_v = v;
	mode_n = n;
    }

    return mode_v;
}

int f_mode(int argc, const int *argt, void **args)
{
    static double *value;
    static int value_size;
    int size = argc * sizeof(double);
    int i, j;

    if (argc < 1)
	return E_ARG_LO;

    for (i = 1; i <= argc; i++)
	if (argt[i] != argt[0])
	    return E_ARG_TYPE;

    if (size > value_size) {
	value_size = size;
	value = G_realloc(value, value_size);
    }

    switch (argt[argc]) {
    case CELL_TYPE:
	{
	    CELL *res = args[0];
	    CELL **argv = (CELL **) & args[1];

	    for (i = 0; i < columns; i++) {
		int nv = 0;

		for (j = 0; j < argc && !nv; j++) {
		    if (IS_NULL_C(&argv[j][i]))
			nv = 1;
		    else
			value[j] = (double)argv[j][i];
		}

		if (nv)
		    SET_NULL_C(&res[i]);
		else
		    res[i] = (CELL) mode(value, argc);
	    }
	    return 0;
	}
    case FCELL_TYPE:
	{
	    FCELL *res = args[0];
	    FCELL **argv = (FCELL **) & args[1];

	    for (i = 0; i < columns; i++) {
		int nv = 0;

		for (j = 0; j < argc && !nv; j++) {
		    if (IS_NULL_F(&argv[j][i]))
			nv = 1;
		    else
			value[j] = (double)argv[j][i];
		}

		if (nv)
		    SET_NULL_F(&res[i]);
		else
		    res[i] = (FCELL) mode(value, argc);
	    }
	    return 0;
	}
    case DCELL_TYPE:
	{
	    DCELL *res = args[0];
	    DCELL **argv = (DCELL **) & args[1];

	    for (i = 0; i < columns; i++) {
		int nv = 0;

		for (j = 0; j < argc && !nv; j++) {
		    if (IS_NULL_D(&argv[j][i]))
			nv = 1;
		    else
			value[j] = (double)argv[j][i];
		}

		if (nv)
		    SET_NULL_D(&res[i]);
		else
		    res[i] = (DCELL) mode(value, argc);
	    }
	    return 0;
	}
    default:
	return E_INV_TYPE;
    }
}

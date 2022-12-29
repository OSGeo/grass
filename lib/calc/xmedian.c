
#include <stdlib.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/raster.h>
#include <grass/calc.h>

/**********************************************************************
median(x1,x2,..,xn)
   return median of arguments
**********************************************************************/

static int icmp(const void *aa, const void *bb)
{
    const CELL *a = aa;
    const CELL *b = bb;

    return *a - *b;
}

static int fcmp(const void *aa, const void *bb)
{
    const FCELL *a = aa;
    const FCELL *b = bb;

    if (*a < *b)
	return -1;
    if (*a > *b)
	return 1;
    return 0;
}

static int dcmp(const void *aa, const void *bb)
{
    const DCELL *a = aa;
    const DCELL *b = bb;

    if (*a < *b)
	return -1;
    if (*a > *b)
	return 1;
    return 0;
}

int f_median(int argc, const int *argt, void **args)
{
    static void *array;
    static int alloc;
    int size = argc * Rast_cell_size(argt[0]);
    int i, j;

    if (argc < 1)
	return E_ARG_LO;

    for (i = 1; i <= argc; i++)
	if (argt[i] != argt[0])
	    return E_ARG_TYPE;

    if (size > alloc) {
	alloc = size;
	array = G_realloc(array, size);
    }

    switch (argt[0]) {
    case CELL_TYPE:
	{
	    CELL *res = args[0];
	    CELL **argv = (CELL **) &args[1];
	    CELL *a = array;
	    CELL *a1 = &a[(argc - 1) / 2];
	    CELL *a2 = &a[argc / 2];

	    for (i = 0; i < columns; i++) {
		int nv = 0;

		for (j = 0; j < argc && !nv; j++) {
		    if (IS_NULL_C(&argv[j][i]))
			nv = 1;
		    else
			a[j] = argv[j][i];
		}

		if (nv)
		    SET_NULL_C(&res[i]);
		else {
		    qsort(a, argc, sizeof(CELL), icmp);
		    res[i] = (*a1 + *a2) / 2;
		}
	    }

	    return 0;
	}
    case FCELL_TYPE:
	{
	    FCELL *res = args[0];
	    FCELL **argv = (FCELL **) &args[1];
	    FCELL *a = array;
	    FCELL *a1 = &a[(argc - 1) / 2];
	    FCELL *a2 = &a[argc / 2];

	    for (i = 0; i < columns; i++) {
		int nv = 0;

		for (j = 0; j < argc && !nv; j++) {
		    if (IS_NULL_F(&argv[j][i]))
			nv = 1;
		    else
			a[j] = argv[j][i];
		}

		if (nv)
		    SET_NULL_F(&res[i]);
		else {
		    qsort(a, argc, sizeof(FCELL), fcmp);
		    res[i] = (*a1 + *a2) / 2;
		}
	    }

	    return 0;
	}
    case DCELL_TYPE:
	{
	    DCELL *res = args[0];
	    DCELL **argv = (DCELL **) &args[1];
	    DCELL *a = array;
	    DCELL *a1 = &a[(argc - 1) / 2];
	    DCELL *a2 = &a[argc / 2];

	    for (i = 0; i < columns; i++) {
		int nv = 0;

		for (j = 0; j < argc && !nv; j++) {
		    if (IS_NULL_D(&argv[j][i]))
			nv = 1;
		    else
			a[j] = argv[j][i];
		}

		if (nv)
		    SET_NULL_D(&res[i]);
		else {
		    qsort(a, argc, sizeof(DCELL), dcmp);
		    res[i] = (*a1 + *a2) / 2;
		}
	    }

	    return 0;
	}
    default:
	return E_INV_TYPE;
    }
}

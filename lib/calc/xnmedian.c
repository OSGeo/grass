#include <stdlib.h>
#include <stdbool.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/raster.h>
#include <grass/calc.h>

/**********************************************************************
median(x1,x2,..,xn)
   return median of arguments
**********************************************************************/

#define SIZE_THRESHOLD 32

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

int f_nmedian(int argc, const int *argt, void **args)
{
    int size = argc * Rast_cell_size(argt[0]);
    int i, j;
    bool use_heap = false;

    if (argc < 1)
        return E_ARG_LO;

    for (i = 1; i <= argc; i++)
        if (argt[i] != argt[0])
            return E_ARG_TYPE;

    switch (argt[0]) {
    case CELL_TYPE: {
        CELL stack_array[SIZE_THRESHOLD];
        CELL *a = stack_array;

        if (argc > SIZE_THRESHOLD) {
            a = G_malloc(size);
            use_heap = true;
        }

        CELL *res = args[0];
        CELL **argv = (CELL **)&args[1];
        CELL a1;
        CELL *resc;

        for (i = 0; i < columns; i++) {
            int n = 0;

            for (j = 0; j < argc; j++) {
                if (IS_NULL_C(&argv[j][i]))
                    continue;
                a[n++] = argv[j][i];
            }

            resc = &res[i];

            if (!n)
                SET_NULL_C(resc);
            else {
                qsort(a, n, sizeof(CELL), icmp);
                *resc = a[n / 2];
                if ((n & 1) == 0) {
                    a1 = a[(n - 1) / 2];
                    if (*resc != a1)
                        *resc = (*resc + a1) / 2;
                }
            }
        }

        if (use_heap) {
            G_free(a);
        }
        return 0;
    }

    case FCELL_TYPE: {
        FCELL stack_array[SIZE_THRESHOLD];
        FCELL *a = stack_array;

        if (argc > SIZE_THRESHOLD) {
            a = G_malloc(size);
            use_heap = true;
        }

        FCELL *res = args[0];
        FCELL **argv = (FCELL **)&args[1];
        FCELL a1;
        FCELL *resc;

        for (i = 0; i < columns; i++) {
            int n = 0;

            for (j = 0; j < argc; j++) {
                if (IS_NULL_F(&argv[j][i]))
                    continue;
                a[n++] = argv[j][i];
            }

            resc = &res[i];

            if (!n)
                SET_NULL_F(resc);
            else {
                qsort(a, n, sizeof(FCELL), fcmp);
                *resc = a[n / 2];
                if ((n & 1) == 0) {
                    a1 = a[(n - 1) / 2];
                    if (*resc != a1)
                        *resc = (*resc + a1) / 2;
                }
            }
        }

        if (use_heap) {
            G_free(a);
        }
        return 0;
    }

    case DCELL_TYPE: {
        DCELL stack_array[SIZE_THRESHOLD];
        DCELL *a = stack_array;

        if (argc > SIZE_THRESHOLD) {
            a = G_malloc(size);
            use_heap = true;
        }

        DCELL *res = args[0];
        DCELL **argv = (DCELL **)&args[1];
        DCELL a1;
        DCELL *resc;

        for (i = 0; i < columns; i++) {
            int n = 0;

            for (j = 0; j < argc; j++) {
                if (IS_NULL_D(&argv[j][i]))
                    continue;
                a[n++] = argv[j][i];
            }

            resc = &res[i];

            if (!n)
                SET_NULL_D(resc);
            else {
                qsort(a, n, sizeof(DCELL), dcmp);
                *resc = a[n / 2];
                if ((n & 1) == 0) {
                    a1 = a[(n - 1) / 2];
                    if (*resc != a1)
                        *resc = (*resc + a1) / 2;
                }
            }
        }

        if (use_heap) {
            G_free(a);
        }
        return 0;
    }

    default:
        return E_INV_TYPE;
    }
}

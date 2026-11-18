#include <stdbool.h>
#if defined(_OPENMP)
#include <omp.h>
#endif

#include <grass/gis.h>
#include <grass/raster.h>
#include "globals.h"
#include "expression.h"
#include "func_proto.h"

/**********************************************************************
area() area of a cell in square meters
**********************************************************************/

int f_area(int argc, const int *argt, void **args)
{
    int tid = 0;
#if defined(_OPENMP)
    tid = omp_get_thread_num();
#endif

    DCELL *res = args[0];
    int i;
    static bool initialized = false;

    if (argc > 0)
        return E_ARG_HI;

    if (argt[0] != DCELL_TYPE)
        return E_RES_TYPE;

    // The initialization writes library state which the threads then read
    // without locking, so it must be finished before any thread asks for
    // an area.
#pragma omp critical(area_initialization)
    if (!initialized) {
        G_begin_cell_area_calculations();
        initialized = true;
    }

    double cell_area = G_area_of_cell_at_row(current_row[tid]);

    for (i = 0; i < columns; i++)
        res[i] = cell_area;

    return 0;
}

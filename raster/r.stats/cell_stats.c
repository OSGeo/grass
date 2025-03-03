#include <stdlib.h>
#include <grass/glocale.h>
#include "global.h"

int cell_stats(int fd[], int with_percents, int with_counts, int with_areas,
               int do_sort, int with_labels, char *fmt)
{
    CELL **cell;
    int i;
    int row;
    double unit_area;
    int planimetric = 0;
    int compute_areas;

    /* allocate i/o buffers for each raster map */
    cell = (CELL **)G_calloc(nfiles, sizeof(CELL *));
    for (i = 0; i < nfiles; i++)
        cell[i] = Rast_allocate_c_buf();

    /* if we want area totals, set this up.
     * distinguish projections which are planimetric (all cells same size)
     * from those which are not (e.g., lat-long)
     */
    unit_area = 0.0;
    if (with_areas) {
        switch (G_begin_cell_area_calculations()) {
        case 0: /* areas don't make sense, but ignore this for now */
        case 1:
            planimetric = 1;
            unit_area = G_area_of_cell_at_row(0);
            break;
        default:
            planimetric = 0;
            break;
        }
    }
    compute_areas = with_areas && !planimetric;

    /* here we go */
    initialize_cell_stats(nfiles);

    for (row = 0; row < nrows; row++) {
        if (compute_areas)
            unit_area = G_area_of_cell_at_row(row);

        G_percent(row, nrows, 2);

        for (i = 0; i < nfiles; i++) {
            Rast_get_c_row(fd[i], cell[i], row);

            /* include max FP value in nsteps'th bin */
            if (is_fp[i])
                fix_max_fp_val(cell[i], ncols);

            /* we can't compute hash on null values, so we change all
               nulls to max+1, set NULL_CELL to max+1, and later compare
               with NULL_CELL to check for nulls */
            reset_null_vals(cell[i], ncols);
        }

        update_cell_stats(cell, ncols, unit_area);
    }

    G_percent(row, nrows, 2);

    sort_cell_stats(do_sort);
    print_cell_stats(fmt, with_percents, with_counts, with_areas, with_labels,
                     fs);
    for (i = 0; i < nfiles; i++) {
        G_free(cell[i]);
    }
    G_free(cell);

    return 0;
}

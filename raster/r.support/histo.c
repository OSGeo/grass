#include <stdlib.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include "local_proto.h"

/*
 * do_histogram() - Creates histogram for CELL
 *
 * RETURN: EXIT_SUCCESS / EXIT_FAILURE
 */
int do_histogram(const char *name)
{
    RASTER_MAP_TYPE data_type;
    CELL *cell;
    struct Cell_head cellhd;
    struct Cell_stats statf;
    int nrows, ncols;
    int row;
    int fd;

    Rast_get_cellhd(name, "", &cellhd);
    data_type = Rast_map_type(name, "");

    Rast_set_window(&cellhd);
    fd = Rast_open_old(name, "");

    nrows = Rast_window_rows();
    ncols = Rast_window_cols();
    cell = Rast_allocate_c_buf();

    Rast_init_cell_stats(&statf);
    if (data_type == CELL_TYPE) {
        struct Range range;

        Rast_init_range(&range);
        for (row = 0; row < nrows; row++) {
            Rast_get_c_row_nomask(fd, cell, row);
            Rast_update_cell_stats(cell, ncols, &statf);
            Rast_row_update_range(cell, ncols, &range);
        }
        Rast_write_range(name, &range);
    }
    else {
        DCELL *dcell;
        struct FPRange fprange;
        int col;

        /* histogram for FCELL / DCELL type ? */

        Rast_init_fp_range(&fprange);
        dcell = Rast_allocate_d_buf();
        for (row = 0; row < nrows; row++) {
            Rast_get_d_row_nomask(fd, dcell, row);
            /* Rast_update_cell_stats wants CELL values */
            for (col = 0; col < ncols; col++) {
                if (Rast_is_d_null_value(&dcell[col]))
                    Rast_set_c_null_value(&cell[col], 1);
                else
                    cell[col] = (CELL)dcell[col];
            }
            Rast_update_cell_stats(cell, ncols, &statf);
            Rast_row_update_fp_range(dcell, ncols, &fprange, data_type);
        }
        Rast_write_fp_range(name, &fprange);
        G_free(dcell);
    }

    Rast_write_histogram_cs(name, &statf);

    Rast_free_cell_stats(&statf);
    Rast_close(fd);
    G_free(cell);

    return 0;
}

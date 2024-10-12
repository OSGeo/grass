#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

/** Generate random values in a raster map
 *
 * @param out Name of raster maps to be opened
 * @param min Minimum cell value
 * @param min Maximum cell value
 * @param int_map TRUE for a CELL map, FALSE for DCELL
 */
int randsurf(char *out, double min, double max, int int_map)
{
    int nrows, ncols; /* Number of cell rows and columns      */

    DCELL *row_out_D; /* Buffer just large enough to hold one */
    CELL *row_out_C;  /* row of the raster map layer.         */

    int fd_out; /* File descriptor - used to identify   */

    /* open raster maps.                    */
    int row_count, col_count;

    /****** OPEN CELL FILES AND GET CELL DETAILS ******/
    fd_out = Rast_open_new(out, int_map ? CELL_TYPE : DCELL_TYPE);

    nrows = Rast_window_rows();
    ncols = Rast_window_cols();

    if (int_map)
        row_out_C = Rast_allocate_c_buf();
    else
        row_out_D = Rast_allocate_d_buf();

    /****** PASS THROUGH EACH CELL ASSIGNING RANDOM VALUE ******/
    for (row_count = 0; row_count < nrows; row_count++) {
        G_percent(row_count, nrows, 2);
        for (col_count = 0; col_count < ncols; col_count++) {
            if (int_map) {
                unsigned int x = (unsigned int)G_mrand48();

                *(row_out_C + col_count) =
                    (CELL)(min + x % (unsigned int)(max + 1 - min));
            }
            else {
                *(row_out_D + col_count) =
                    (DCELL)(min + G_drand48() * (max - min));
            }
        }

        /* Write contents row by row */
        if (int_map)
            Rast_put_c_row(fd_out, (CELL *)row_out_C);
        else
            Rast_put_d_row(fd_out, (DCELL *)row_out_D);
    }
    G_percent(1, 1, 1);

    Rast_close(fd_out);

    return 0;
}

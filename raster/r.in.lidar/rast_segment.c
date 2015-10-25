
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/raster.h>
#include <grass/segment.h>

#include "rast_segment.h"

static void rast_segment_load(SEGMENT * segment, int rowio,
                              RASTER_MAP_TYPE map_type)
{
    void *raster_row = Rast_allocate_input_buf(map_type);
    int row;

    for (row = 0; row < Rast_input_window_rows(); row++) {
        /* TODO: free mem */
        Rast_get_row(rowio, raster_row, row, map_type);
        Segment_put_row(segment, raster_row, row);
    }
}

/* TODO: close function */

void rast_segment_open(SEGMENT * segment, const char *name,
                       RASTER_MAP_TYPE * map_type)
{
    /* TODO: check if not passing the mapset is OK */
    int rowio = Rast_open_old(name, "");

    *map_type = Rast_get_map_type(rowio);
    int segment_rows = 64;

    /* we use long segments because this is how the values a binned */
    int segment_cols = Rast_input_window_cols();
    int segments_in_memory = 4;

    if (Segment_open(segment, G_tempfile(), Rast_input_window_rows(),
                     Rast_input_window_cols(), segment_rows, segment_cols,
                     Rast_cell_size(*map_type), segments_in_memory) != 1)
        G_fatal_error(_("Cannot create temporary file with segments of a raster map"));
    rast_segment_load(segment, rowio, *map_type);
    Rast_close(rowio);          /* we won't need the raster again */
}


/* 0 on out of region or NULL, 1 on success */
int rast_segment_get_value_xy(SEGMENT * base_segment,
                              struct Cell_head *input_region,
                              RASTER_MAP_TYPE rtype, double x, double y,
                              double *value)
{
    /* Rast gives double, Segment needs off_t */
    off_t base_row = Rast_northing_to_row(y, input_region);
    off_t base_col = Rast_easting_to_col(x, input_region);

    /* skip points which are outside the base raster
     * (null propagation) */
    if (base_row < 0 || base_col < 0 ||
        base_row >= input_region->rows || base_col >= input_region->cols)
        return 0;
    if (rtype == DCELL_TYPE) {
        DCELL tmp;

        Segment_get(base_segment, &tmp, base_row, base_col);
        if (Rast_is_d_null_value(&tmp))
            return 0;
        *value = (double)tmp;
    }
    else if (rtype == FCELL_TYPE) {
        FCELL tmp;

        Segment_get(base_segment, &tmp, base_row, base_col);
        if (Rast_is_f_null_value(&tmp))
            return 0;
        *value = (double)tmp;
    }
    else {
        CELL tmp;

        Segment_get(base_segment, &tmp, base_row, base_col);
        if (Rast_is_c_null_value(&tmp))
            return 0;
        *value = (double)tmp;
    }
    return 1;
}

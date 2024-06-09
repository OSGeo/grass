#include <grass/gis.h>
#include <grass/raster.h>
#include "h_measure.h"

typedef struct dimensions {
    int size;
    int dist;
    int nrows;
    int ncols;
    int n_outputs;
    int n_measures;
} dimensions;

typedef struct output_setting {
    int *outfd;
    RASTER_MAP_TYPE out_data_type;
    struct Flag *flag_null;
    struct Flag *flag_ind;
} output_setting;

int execute_texture(CELL **data, struct dimensions *dim,
                    struct menu *measure_menu, int *measure_idx,
                    struct output_setting *out_set);

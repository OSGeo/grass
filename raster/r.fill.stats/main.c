
/***************************************************************************
 *
 * MODULE:    r.fill.stats
 * AUTHOR(S): Benjamin Ducke
 *            Anna Petrasova (speedup with -p, fix of median, mode computation)
 * PURPOSE:   Rapidly fill "no data" cells in a raster map by simple interpolation.
 *
 * COPYRIGHT: (C) 2011-2018 by the GRASS Development Team
 *
 *            This program is free software under the GPL (>=v2)
 *            Read the file COPYING that comes with GRASS for details.
 *
 ****************************************************************************
 */

/*

   DOCS:
   - docs need a lot of updates!
   - dropped medoid (since we always have an odd number of cells, median=medoid).
   - flag -p(reserve) reversed (preservation is now default)
   - flag for including center cell has been dropped; center always included
   - "method" renamed to "mode"
   - by default, neighborhood size is now assumed to be in cells,
   unless -m(ap units) is given
   - only -m(ap units) will result in an exact, circular neighborhood;
   if (default) cells size specification is used, then the neighborhood
   shape is a less exact rectangle; in that case the neighborhood dimensions
   will also be different for x and y if the cell dimensions are different
   - lat/lon data is allowed, but distance measure for -m(ap units) is straight line! 
   - cell weights are now normalized to be in range [0;1]
   - center cell weight for "wmean" is now "1.0"

   BUGS:
   - mode, median and mode produce large areas of "no data" where there is input data!!!

   NEXT VERSION:
   - add lat/lon distance and cost map distance measures (lat/lon currently throws an error).
   - allow user to set which cell value should be filled (i.e. interpreted as "no data")

 */

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <grass/gis.h>
#include <grass/glocale.h>

#include "cell_funcs.h"


/* Global variables :-) */
double **WEIGHTS;               /* neighborhood weights */
double SUM_WEIGHTS;             /* sum of weights in all cells (with or without data) of neighborhood */

unsigned long WINDOW_WIDTH = 0; /* dimensions of neighborhood window */
unsigned long WINDOW_HEIGHT = 0;
unsigned long DATA_WIDTH = 0;
unsigned long DATA_HEIGHT = 0;
unsigned long PADDING_WIDTH = 0;
unsigned long PADDING_HEIGHT = 0;

void **CELL_INPUT = NULL;
void **CELL_INPUT_HANDLES = NULL;
void *CELL_OUTPUT = NULL;
FCELL *ERR_OUTPUT = NULL;

/* holds statistics of cells within the neighborhood */
typedef struct
{
    unsigned long num_values;   /* number of cells with values in input raster */
    double *values;             /* individual values of all cells */
    double *weights;            /* individual weights of all cells */
    double result;              /* weighted mean of values in entire neighborhood */
    double certainty;           /* certainty measure, always between 0 (lowest) and 1 (highest) */
    unsigned long *frequencies; /* frequency count for each value */
    double *overwrite_value;    /* will be set to non-null to overwrite the statistical result with this value */
    int overwrite;              /* 1 to overwrite the statistical result with the original value */
} stats_struct;


/* function pointers for operation modes */
void (*GET_STATS) (unsigned long, unsigned long, double, double, int,
                   stats_struct *);
void (*COLLECT_DATA) (double, double, double, double, stats_struct *);


/*
 * Returns a rough estimate of the amount of RAM
 * (in bytes) that this program will require.
 */
//TODO: this also needs to take into account additional input and output maps (once implemented).
long int estimate_mem_needed(long int cols, char *mode)
{
    long int mem_count = 0;
    long int in_bytes = 0;
    long int out_bytes = 0;
    long int stat_bytes = 0;


    /* memory for neighborhood weights and statistics */
    if (!strcmp(mode, "wmean")) {
        stat_bytes += sizeof(double) * (DATA_WIDTH * DATA_HEIGHT);      /* weights matrix */
    }
    stat_bytes += sizeof(double) * (DATA_WIDTH * DATA_HEIGHT);  /* max. cell values */
    stat_bytes += sizeof(double) * (DATA_WIDTH * DATA_HEIGHT);  /* max. cell weights */
    stat_bytes += sizeof(unsigned long) * (DATA_WIDTH * DATA_HEIGHT);   /* max. cell frequencies */

    /* input data rows with padded buffers */
    in_bytes = (unsigned long)(WINDOW_HEIGHT * (cols + (PADDING_WIDTH * 2)));
    if (IN_TYPE == CELL_TYPE) {
        in_bytes += in_bytes * sizeof(CELL);
    }
    if (IN_TYPE == FCELL_TYPE) {
        in_bytes += in_bytes * sizeof(FCELL);
    }
    if (IN_TYPE == DCELL_TYPE) {
        in_bytes += in_bytes * sizeof(DCELL);
    }

    /* output data row */
    out_bytes = (unsigned long)cols;
    if (OUT_TYPE == CELL_TYPE) {
        out_bytes += out_bytes * sizeof(CELL);
    }
    if (OUT_TYPE == FCELL_TYPE) {
        out_bytes += out_bytes * sizeof(FCELL);
    }
    if (OUT_TYPE == DCELL_TYPE) {
        out_bytes += out_bytes * sizeof(DCELL);
    }

    mem_count = stat_bytes + in_bytes + out_bytes;

    return (mem_count);
}


/*
 * Prints the spatial weights matrix to the console.
 * This uses a fixed layout which may not be able to print very
 * large matrices.
 */
void print_weights_matrix(long int rows, long int cols)
{
    int i, j;
    int weight_matrix_line_length = 80;
    int weight_matrix_weight_length = 7;
    char weight_matrix_line_buf[weight_matrix_line_length + 1];
    char weight_matrix_weight_buf[weight_matrix_line_length + 1];

    G_message(_("Spatial weights neighborhood (cells):"));
    for (i = 0; i < rows; i++) {
        weight_matrix_line_buf[0] = '\0';
        for (j = 0; j < cols; j++) {
            if (WEIGHTS[i][j] != -1.0) {
                snprintf(weight_matrix_weight_buf, weight_matrix_line_length,
                         "%06.2f ", WEIGHTS[i][j]);
                if (strlen(weight_matrix_weight_buf) >
                    (weight_matrix_weight_length)) {
                    snprintf(weight_matrix_weight_buf,
                             weight_matrix_line_length, "[????] ",
                             WEIGHTS[i][j]);
                }
            }
            else {
                snprintf(weight_matrix_weight_buf, weight_matrix_line_length,
                         "...... ");
            }
            if (strlen(weight_matrix_weight_buf) +
                strlen(weight_matrix_line_buf) > weight_matrix_line_length) {
                strncpy(weight_matrix_line_buf, "[line too long to print]",
                        weight_matrix_line_length);
                break;
            }
            else {
                strcat(weight_matrix_line_buf, weight_matrix_weight_buf);
            }
        }
        fprintf(stdout, "%s\n", weight_matrix_line_buf);
    }
}


/*
 * Returns a void* that points to the first valid data cell in the
 * input data row corresponding to "row_idx".
 */
void *get_input_row(unsigned long row_idx)
{
    unsigned long i;
    void *my_cell = NULL;


    my_cell = CELL_INPUT_HANDLES[row_idx];

    for (i = 0; i < PADDING_WIDTH; i++)
        my_cell += CELL_IN_SIZE;


    return (my_cell);
}


/* NEIGHBORHOOD STATISTICS
 *
 * The following function provide the different neighborhood statistics (interpolators)
 * that have been implemented in this software.
 *
 * Only those cells go into the statistics that are within the circular neighborhood window.
 *
 * Since the input buffers are padded with NULL data for rows lying to the South or North,
 * West or East of the current region, these functions can just blindly read values above and
 * below the current position on the W-E axis.
 *
 * The parameter "col" must be set to actual GRASS region column number on the W-E axis.
 *
 * "min" and "max" define the smallest and largest values to use for interpolation. Set
 * filter_min and filter_max to !=0 to use these for range filtering the data.
 *
 * The results will be stored in the cell_stats object passed to this function. This object
 * must have been properly initialized before passing it to any of the functions below!
 */

/*
 * The different types of neighborhood statistics required different
 * types of information to be collected.
 */

void collect_values_unfiltered(double val1, double val2, double min,
                               double max, stats_struct * stats)
{
    stats->values[stats->num_values] = val1;
    stats->certainty += val2;
    stats->num_values++;
}

void collect_values_filtered(double val1, double val2, double min, double max,
                             stats_struct * stats)
{
    if (val1 >= min && val1 <= max) {
        collect_values_unfiltered(val1, val2, min, max, stats);
    }
}

void collect_values_and_weights_unfiltered(double val1, double val2,
                                           double min, double max,
                                           stats_struct * stats)
{
    stats->values[stats->num_values] = val1;
    stats->weights[stats->num_values] = val2;
    stats->certainty += val2;
    stats->num_values++;
}

void collect_values_and_weights_filtered(double val1, double val2, double min,
                                         double max, stats_struct * stats)
{
    if (val1 >= min && val1 <= max) {
        collect_values_and_weights_unfiltered(val1, val2, min, max, stats);
    }
}

void collect_values_and_frequencies_unfiltered(double val1, double val2,
                                               double min, double max,
                                               stats_struct * stats)
{
    unsigned long i;

    stats->certainty += val2;

    /* extreme case: no values collected yet */
    if (stats->num_values == 0) {
        stats->values[0] = val1;
        stats->frequencies[0] = 1;
        stats->num_values++;
        return;
    }

    /* at least one value already collected */
    for (i = 0; i < stats->num_values; i++) {
        /* does this value already exist in the stats object? */
        if (stats->values[i] == val1) {
            /* yes: increase its counter and abort search */
            stats->frequencies[i]++;
            stats->values[stats->num_values] = val1;
            stats->num_values++;
            return;
        }
    }
    /* no: first occurrence of this value: store as new entry */
    stats->values[i] = val1;
    stats->frequencies[i] = 1;
    stats->num_values++;
}

void collect_values_and_frequencies_filtered(double val1, double val2,
                                             double min, double max,
                                             stats_struct * stats)
{
    if (val1 >= min && val1 <= max) {
        collect_values_and_frequencies_unfiltered(val1, val2, min, max,
                                                  stats);
    }
}


/*
 * Simple double comparison function for use by qsort().
 * This is needed for calculating median statistics.
 */
int compare_dbl(const void *val1, const void *val2)
{
    if (*(double *)val1 == *(double *)val2)
        return 0;
    if (*(double *)val1 < *(double *)val2)
        return -1;
    return 1;
}

/*
 * Collecting the cell data from the neighborhood is the same
 * basic loop for every type of statistics: collect only non-null
 * cells, and collect only within the neighborhood mask.
 */
void read_neighborhood(unsigned long row_index, unsigned long col,
                       double min, double max, int preserve,
                       stats_struct * stats)
{
    unsigned long i, j;
    void *cell;
    double cell_value;
    stats->overwrite = 0;
    if (preserve == TRUE) {
        cell = CELL_INPUT_HANDLES[row_index];
        cell += CELL_IN_SIZE * col;
        cell += CELL_IN_SIZE * ((DATA_WIDTH - 1) / 2);
        if (!IS_NULL(cell)) {
            stats->overwrite = 1;
            *stats->overwrite_value =
                    (double)Rast_get_d_value(cell, IN_TYPE);
            return;
        }
    }

    /* read data */
    unsigned long row_position = row_index - PADDING_HEIGHT;

    stats->num_values = 0;
    stats->certainty = 0.0;
    for (i = 0; i < DATA_HEIGHT; i++) {
        cell = CELL_INPUT_HANDLES[i + row_position];
        cell += CELL_IN_SIZE * col;

        for (j = 0; j < DATA_WIDTH; j++) {
            /* read cell from input buffer */
            if (!IS_NULL(cell)) {
                /* only add non-null cells to stats */
                cell_value = (double)Rast_get_d_value(cell, IN_TYPE);
                /* only add if within neighborhood */
                if (WEIGHTS[i][j] != -1.0) {
                    /* get data needed for chosen statistic */
                    COLLECT_DATA(cell_value, WEIGHTS[i][j], min, max,
                                 stats);
                }
            }
            /* go to next cell on current row */
            cell += CELL_IN_SIZE;
        }
    }
}


/*
 * NEIGHBORHOOD STATISTICS FUNCTION WMEAN
 * Spatially weighted mean.
 *
 * The cell values are multiplied by their spatial weights before they are stored.
 */
void get_statistics_wmean(unsigned long row_index, unsigned long col,
                          double min, double max, int preserve,
                          stats_struct * stats)
{
    unsigned long i;
    double total;
    double total_weight;

    read_neighborhood(row_index, col, min, max, preserve, stats);
    if (stats->overwrite)
        return;

    /* compute weighted average of all valid input cells */
    total = 0;
    total_weight = 0;
    for (i = 0; i < stats->num_values; i++) {
        total += stats->values[i] * stats->weights[i];
        total_weight += stats->weights[i];
    }
    stats->result = total / total_weight;
}


/*
 * NEIGHBORHOOD STATISTICS FUNCTION MEAN
 * Simple, unweighted mean.
 */
void get_statistics_mean(unsigned long row_index, unsigned long col,
                         double min, double max, int preserve,
                         stats_struct * stats)
{
    unsigned long i;
    double total;

    read_neighborhood(row_index, col, min, max, preserve, stats);
    if (stats->overwrite)
        return;

    /* compute total of all valid input cells */
    total = 0;
    for (i = 0; i < stats->num_values; i++) {
        total += stats->values[i];
    }
    stats->result = total / ((double)stats->num_values);
}


/*
 * NEIGHBORHOOD STATISTICS FUNCTION MEDIAN
 * Simple, unweighted median. For an even number of data points, the median is the
 * average of the two central elements in the sorted data list.
 */
void get_statistics_median(unsigned long row_index, unsigned long col,
                           double min, double max, int preserve,
                           stats_struct * stats)
{
    read_neighborhood(row_index, col, min, max, preserve, stats);
    if (stats->overwrite)
        return;

    /* sort list of values */
    qsort(&stats->values[0], stats->num_values, sizeof(double), &compare_dbl);

    if (stats->num_values % 2 == 0.0) {
        /* even number of elements: result is average of the two central values */
        stats->result =
            (stats->values[stats->num_values / 2 - 1] +
             stats->values[(stats->num_values / 2)]) / 2.0;
    }
    else {
        /* odd number of elements: result is the central element */
        stats->result = stats->values[(stats->num_values / 2)];
    }
}


/*
 * NEIGHBORHOOD STATISTICS FUNCTION MODE
 * Simple, unweighted mode. Mathematically, the mode is not always unique. If there is more than
 * one value with highest frequency, the smallest one is chosen to represent the mode.
 */
void get_statistics_mode(unsigned long row_index, unsigned long col,
                         double min, double max, int preserve,
                         stats_struct * stats)
{
    unsigned long i;
    double mode;
    unsigned long freq;

    read_neighborhood(row_index, col, min, max, preserve, stats);
    if (stats->overwrite)
        return;

    if (stats->num_values < 1)
        return;

    mode = stats->values[0];
    freq = stats->frequencies[0];
    for (i = 1; i < stats->num_values; i++) {
        if (stats->frequencies[i] > freq) {
            mode = stats->values[i];
            freq = stats->frequencies[i];
        }
    }
    stats->result = mode;
    /* need to initialize, otherwise old values sometimes stay */
    for (i = 0; i < WINDOW_WIDTH * WINDOW_HEIGHT; i++)
        stats->frequencies[i] = 0;
}


/*
 * Initializes handlers to point to corresponding data rows.
 */
void init_handles()
{
    unsigned long i;

    for (i = 0; i < WINDOW_HEIGHT; i++) {
        CELL_INPUT_HANDLES[i] = CELL_INPUT[i];
    }
}


/*
 * Replaces upper-most data row in input buffer with
 * a new row from disk.
 * Re-shuffles the data row handlers, so that the new row
 * becomes the last row in the index and all other rows move
 * up one. This procedure is a bit complex, but it is a lot faster
 * to move around a few pointers in memory, than to re-read all
 * the data rows for the neighborhood every time we go down one
 * row in the current region.
 */
void advance_one_row(int file_desc, long current_row)
{
    unsigned long i, j;
    void *cell_input;
    static unsigned long replace_row = 0;       /* points to the row which will be replaced next */
    unsigned long replace_pos = 0;


    /* the actual replacement position needs to consider the "no data" padding offset, as well */
    replace_pos = replace_row + PADDING_HEIGHT;

    /* get address of data row to replace */
    cell_input = CELL_INPUT[replace_pos];
    for (i = 0; i < PADDING_WIDTH; i++)
        cell_input += CELL_IN_SIZE;

    /* get next row from disk */
    Rast_get_row(file_desc, cell_input, current_row + DATA_HEIGHT, IN_TYPE);

    /* re-assign all row handlers below current replacement position */
    j = PADDING_HEIGHT;
    for (i = 0; i < DATA_HEIGHT - (replace_row + 1); i++) {
        CELL_INPUT_HANDLES[j] = CELL_INPUT[replace_pos + 1 + i];
        j++;
    }

    /* re-assign all row handlers up to and including replacement position */
    for (i = 0; i <= replace_row; i++) {
        CELL_INPUT_HANDLES[j] = CELL_INPUT[PADDING_HEIGHT + i];
        j++;
    }

    replace_row++;
    if (replace_row > (DATA_HEIGHT - 1)) {
        /* start over once end of data area has been reached */
        replace_row = 0;
    }
}


/*
 * Interpolates one row of input data, stores result in CELL_OUTPUT (global var)
 */
void interpolate_row(unsigned long row_index, unsigned long cols,
                     double min, double max, int preserve,
                     unsigned long min_cells,
                     stats_struct * stats, int write_err)
{
    unsigned long j;
    void *cell_output;
    FCELL *err_output;

    cell_output = CELL_OUTPUT;
    err_output = ERR_OUTPUT;

    for (j = 0; j < cols; j++) {
        /* get neighborhood statistics */
        GET_STATS(row_index, j, min, max, preserve, stats);
        /* original value is preserved */
        if (stats->overwrite) {
            WRITE_DOUBLE_VAL(cell_output, *stats->overwrite_value);
            /* write error/uncertainty output map? */
            if (write_err) {
                Rast_set_f_value(err_output, 0,
                                 FCELL_TYPE);
            }
        }
        /* enough reachable cells in input map? */
        else if (stats->num_values < min_cells) {
            SET_NULL(cell_output, 1);
            if (write_err)
                Rast_set_f_null_value(err_output, 1);
        }
        else {
            /* write interpolation result into output map */
            WRITE_DOUBLE_VAL(cell_output, stats->result);

            /* write error/uncertainty output map? */
            if (write_err) {
                Rast_set_f_value(err_output,
                                 (FCELL) 1.0 -
                                 (stats->certainty / SUM_WEIGHTS),
                                 FCELL_TYPE);
            }
        }
        /* advance cell pointers by one cell size */
        cell_output += CELL_OUT_SIZE;
        err_output++;
    }
}


/*
 * Pre-computes the matrix of spatial weights.
 * for operation mode "wmean" (spatially weighted mean), "constant" is passed as "0"
 * and distance-dependent weigths are calculated. For all other modes, "constant" is
 * passed as "1" and all cells within the circular neighborhood will be set to "1.0".
 * In both casses, all cells outside the neighborhood will be set to "-1.0".
 */
void build_weights_matrix(double radius, double power, double res_x,
                          double res_y, int constant, int use_map_units)
{

    unsigned long i, j;
    double p1_x, p1_y, p2_x, p2_y, A, B, C, W;
    double tolerance;


    /* alloc enough mem for weights matrix */
    WEIGHTS = G_malloc(sizeof(double *) * DATA_HEIGHT);
    for (i = 0; i < DATA_HEIGHT; i++) {
        WEIGHTS[i] = G_malloc(sizeof(double) * DATA_WIDTH);
    }

    /* center of the neighborhood window in real map units */
    p1_x = (DATA_WIDTH / 2 * res_x) + (res_x / 2.0);
    p1_y = (DATA_HEIGHT / 2 * res_y) + (res_y / 2.0);

    /* tolerance for including half cells in the neighborhood */
    tolerance = (sqrt(pow(res_x, 2) + pow(res_y, 2))) / 2;

    /* 1st pass: get largest possible weight for normalization */
    double max = -1.0;

    for (i = 0; i < DATA_HEIGHT; i++) {
        for (j = 0; j < DATA_WIDTH; j++) {
            p2_x = (j * res_x) + (res_x / 2.0);
            p2_y = (i * res_y) + (res_y / 2.0);
            A = fabs(p2_x - p1_x);
            B = fabs(p2_y - p1_y);
            C = sqrt(pow(A, 2) + pow(B, 2));
            if (use_map_units) {
                if (C > radius + tolerance) {
                    WEIGHTS[i][j] = -1.0;
                }
                else {
                    WEIGHTS[i][j] = C;
                }
            }
            else {
                WEIGHTS[i][j] = C;
            }
            if (WEIGHTS[i][j] > max) {
                /* update max value */
                max = WEIGHTS[i][j];
            }
        }
    }

    /* Build the weights matrix */
    SUM_WEIGHTS = 0.0;
    for (i = 0; i < DATA_HEIGHT; i++) {
        for (j = 0; j < DATA_WIDTH; j++) {
            /* Assign neighborhood coordinates with 0/0
               at the top left of the cell neighborhood matrix. */
            p2_x = (j * res_x) + (res_x / 2.0);
            p2_y = (i * res_y) + (res_y / 2.0);
            /* get distance from window center */
            A = fabs(p2_x - p1_x);
            B = fabs(p2_y - p1_y);
            C = sqrt(pow(A, 2) + pow(B, 2));
            if (constant) {
                W = 1.0;
            }
            else {
                W = ((pow(1 - (C / max), power)));
            }
            /* exclude neighborhood locations that are farther
               from the center than the user-defined distance
               plus a tolerance of half the current region's
               cell diagonal */
            if (use_map_units) {
                if (C > radius + tolerance) {
                    WEIGHTS[i][j] = -1.0;
                }
                else {
                    WEIGHTS[i][j] = W;
                    WEIGHTS[DATA_HEIGHT / 2][DATA_WIDTH / 2] = 0.0;     /* safeguard against total weight growing by center cell weight (InF) */
                    SUM_WEIGHTS += WEIGHTS[i][j];
                }
            }
            else {
                WEIGHTS[i][j] = W;
                WEIGHTS[DATA_HEIGHT / 2][DATA_WIDTH / 2] = 0.0; /* safeguard against total weight growing by center cell weight (InF) */
                SUM_WEIGHTS += WEIGHTS[i][j];
            }
        }
    }

    /* weight of center cell is always = 1 */
    WEIGHTS[DATA_HEIGHT / 2][DATA_WIDTH / 2] = 1.0;
}


/*
 *
 * MAIN FUNCTION
 *
 */
int main(int argc, char *argv[])
{
    /* processing time keepers */
    time_t start, finish;

    /* GRASS region properties */
    struct Cell_head cellhd, region;
    struct Range int_range;
    struct FPRange fp_range;
    struct History hist;
    unsigned long rows = 0;
    unsigned long cols = 0;
    double res_x, res_y = 0.0;
    int projection;

    /* GRASS module options */
    struct GModule *module;
    struct
    {
        struct Option
            *input, *output, *error,
            *radius, *mode, *power, *min, *max, *minpts;
        struct Flag
            *dist_m, *preserve, *print_w, *print_u, *center,
            *single_precision;
    } parm;

    /* program settings */
    char *input;
    char *output;
    char *mapset;
    double radius = 1.0;
    unsigned long min_cells = 12;
    double power = 2.0;
    double min = 0.0;
    double max = 0.0;
    int filter_min = 0;
    int filter_max = 0;
    int write_error;

    /* file handlers */
    void *cell_input;
    int in_fd;
    int out_fd;
    int err_fd;

    /* cell statistics object */
    stats_struct cell_stats;

    /* generic indices, loop counters, etc. */
    unsigned long i, j;
    long l;


    start = time(NULL);

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("surface"));
    G_add_keyword(_("interpolation"));
    G_add_keyword(_("IDW"));
    G_add_keyword(_("no-data filling"));
    module->description =
        _("Rapidly fills 'no data' cells (NULLs) of a raster map with interpolated values (IDW).");

    /* parameters */

    parm.input = G_define_standard_option(G_OPT_R_INPUT);
    parm.input->key = "input";
    parm.input->required = YES;
    parm.input->multiple = NO;
    parm.input->description = _("Raster map with data gaps to fill");

    parm.output = G_define_standard_option(G_OPT_R_OUTPUT);
    parm.output->required = YES;
    parm.output->key = "output";
    parm.output->description = _("Name of result output map");

    parm.error = G_define_standard_option(G_OPT_R_OUTPUT);
    parm.error->required = NO;
    parm.error->key = "uncertainty";
    parm.error->description = _("Name of uncertainty output map");

    parm.radius = G_define_option();
    parm.radius->key = "distance";
    parm.radius->key_desc = "value";
    parm.radius->required = YES;
    parm.radius->multiple = NO;
    parm.radius->type = TYPE_DOUBLE;
    parm.radius->description =
        _("Distance threshold (default: in cells) for interpolation");
    parm.radius->answer = "3";

    parm.mode = G_define_option();
    parm.mode->key = "mode";
    parm.mode->key_desc = "name";
    parm.mode->required = YES;
    parm.mode->multiple = NO;
    parm.mode->type = TYPE_STRING;
    parm.mode->description = _("Statistic for interpolated cell values");
    parm.mode->options = "wmean,mean,median,mode";
    parm.mode->answer = "wmean";

    parm.min = G_define_option();
    parm.min->key = "minimum";
    parm.min->key_desc = "value";
    parm.min->required = NO;
    parm.min->multiple = NO;
    parm.min->type = TYPE_DOUBLE;
    parm.min->description =
        _("Minimum input data value to include in interpolation");

    parm.max = G_define_option();
    parm.max->key = "maximum";
    parm.max->key_desc = "value";
    parm.max->required = NO;
    parm.max->multiple = NO;
    parm.max->type = TYPE_DOUBLE;
    parm.max->description =
        _("Maximum input data value to include in interpolation");

    parm.power = G_define_option();
    parm.power->key = "power";
    parm.power->key_desc = "value";
    parm.power->required = YES;
    parm.power->multiple = NO;
    parm.power->type = TYPE_DOUBLE;
    parm.power->answer = "2.0";
    parm.power->description = _("Power coefficient for IDW interpolation");

    parm.minpts = G_define_option();
    parm.minpts->key = "cells";
    parm.minpts->key_desc = "value";
    parm.minpts->required = YES;
    parm.minpts->multiple = NO;
    parm.minpts->type = TYPE_INTEGER;
    parm.minpts->answer = "8";
    parm.minpts->description =
        _("Minimum number of data cells within search radius");

    parm.dist_m = G_define_flag();
    parm.dist_m->key = 'm';
    parm.dist_m->description =
        _("Interpret distance as map units, not number of cells");

    parm.preserve = G_define_flag();
    parm.preserve->key = 'k';
    parm.preserve->label = _("Keep (preserve) original cell values");
    parm.preserve->description =
        _("By default original values are smoothed");

    parm.print_w = G_define_flag();
    parm.print_w->key = 'w';
    parm.print_w->description = _("Just print the spatial weights matrix");

    parm.print_u = G_define_flag();
    parm.print_u->key = 'u';
    parm.print_u->description = _("Just print estimated memory usage");

    parm.single_precision = G_define_flag();
    parm.single_precision->key = 's';
    parm.single_precision->description =
        _("Single precision floating point output");


    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    input = parm.input->answer;
    output = parm.output->answer;

    /* get setting of current GRASS region */
    G_get_window(&region);
    projection = region.proj;
    if (projection == PROJECTION_LL && parm.dist_m->answer) {
        G_warning(_("You are working with lat/lon data."));
        G_warning(_("This module uses a straight-line distance metric."));
        G_warning(_("Expect inaccuracies."));
    }
    rows = (unsigned long)region.rows;
    cols = (unsigned long)region.cols;
    res_x = (double)region.ew_res;
    res_y = (double)region.ns_res;

    /* get user parameters */
    radius = strtod(parm.radius->answer, 0);
    power = strtod(parm.power->answer, 0);
    min_cells = atol(parm.minpts->answer);
    write_error = 0;
    if (parm.error->answer) {
        write_error = 1;
        if (!strcmp(parm.error->answer, parm.output->answer)) {
            G_fatal_error(_("Result map name cannot be identical with uncertainty map name."));
        }
    }

    /* validate user parameters */
    double max_dist = 0.0;

    if (parm.dist_m->answer) {
        if (radius < 0) {
            G_fatal_error(_("Maximum distance must be larger than zero."));
        }
        if (res_x < res_y) {
            if (radius < res_x)
                G_fatal_error(_("Maximum distance must be at least '%.6f' (W-E resolution)."),
                              res_x);
        }
        if (res_y < res_x) {
            if (radius < res_y)
                G_fatal_error(_("Maximum distance must be at least '%.6f' (S-N resolution)."),
                              res_y);
        }
        if (res_y == res_x) {
            if (radius < res_y)
                G_fatal_error(_("Maximum distance must be at least '%.6f' (W-E and S-N resolution)."),
                              res_y);
        }
        max_dist = sqrt(pow((cols * res_x), 2) + pow((rows * res_y), 2));
        if (radius > max_dist) {
            G_warning(_("Maximum distance too large. Adjusted to '%.6f' (diagonal of current region)."),
                      max_dist);
            radius = max_dist;
        }
    }
    else {
        unsigned long radius_i = (unsigned long)radius;

        radius = (double)radius_i;      //truncate to whole cell number
        if (radius < 1) {
            G_fatal_error(_("Maximum distance must be at least one cell."));
        }
        unsigned long max_dist_i =
            (unsigned long)(sqrt(pow((cols), 2) + pow((rows), 2)));
        max_dist = (double)max_dist_i;
        if (radius > max_dist) {
            G_warning(_("Maximum distance too large. Adjusted to '%lu' cells (diagonal of current region)."),
                      max_dist_i);
            radius = (double)max_dist_i;
        }
    }

    if (min_cells < 1) {
        G_fatal_error(_("Minimum number of cells must be at least '1'."));
    }
    if (min_cells > ((DATA_WIDTH * DATA_HEIGHT) - 1)) {
        G_fatal_error(_("Specified minimum number of cells unreachable with current settings."));
    }

    if (filter_min != 0 && filter_max != 0) {
        if (min >= max) {
            G_fatal_error(_("Value for 'minimum' must be smaller than value for 'maximum'."));
        }
    }
    if (parm.power->answer && strcmp(parm.mode->answer, "wmean")) {
        G_warning(_("The 'power' option has no effect in any mode other than 'wmean'."));
        parm.power->answer = 0;
    }


    /* rounded dimensions of weight matrix (in map cells) */
    if (parm.dist_m->answer) {
        DATA_WIDTH = ((unsigned long)ceil(radius / res_x)) * 2 + 1;
        DATA_HEIGHT = ((unsigned long)ceil(radius / res_y)) * 2 + 1;
        if ((!parm.print_w->answer) &&
            (fmod(radius, res_x) != 0 || fmod(radius, res_y) != 0)) {
            G_warning(_("The specified maximum distance cannot be resolved to whole cells\n at the current resolution settings."));
        }
    }
    else {
        DATA_WIDTH = (unsigned long)radius *2 + 1;
        DATA_HEIGHT = (unsigned long)radius *2 + 1;
    }
    PADDING_WIDTH = (DATA_WIDTH - 1) / 2;
    PADDING_HEIGHT = (DATA_HEIGHT - 1) / 2;
    WINDOW_WIDTH = (PADDING_WIDTH * 2) + DATA_WIDTH;
    WINDOW_HEIGHT = (PADDING_HEIGHT * 2) + DATA_HEIGHT;
    G_message(_("W-E size of neighborhood is %lu cells."), DATA_WIDTH);
    G_message(_("S-N size of neighborhood is %lu cells."), DATA_HEIGHT);

    if (DATA_WIDTH < 3 || DATA_HEIGHT < 3) {
        G_fatal_error(_("Neighborhood cannot be smaller than 3 cells in X or Y direction."));
    }

    if (parm.print_w->answer) {
        if (!strcmp(parm.mode->answer, "wmean")) {
            build_weights_matrix(radius, power, res_x, res_y, 0,
                                 parm.dist_m->answer);
        }
        if (!strcmp(parm.mode->answer, "mean")) {
            build_weights_matrix(radius, power, res_x, res_y, 1,
                                 parm.dist_m->answer);
        }
        if (!strcmp(parm.mode->answer, "median")) {
            build_weights_matrix(radius, power, res_x, res_y, 1,
                                 parm.dist_m->answer);
        }
        if (!strcmp(parm.mode->answer, "mode")) {
            build_weights_matrix(radius, power, res_x, res_y, 1,
                                 parm.dist_m->answer);
        }
        print_weights_matrix(DATA_HEIGHT, DATA_WIDTH);
        /* continue only if "-u" flag has also been given */
        if (!parm.print_u->answer)
            exit(0);
    }

    /* open raster input map and get its storage type */
    mapset = G_find_raster(input, "");
    if (!mapset)
        G_fatal_error(_("Raster map <%s> not found"), input);
    Rast_get_cellhd(input, mapset, &cellhd);
    in_fd = Rast_open_old(input, mapset);
    IN_TYPE = Rast_get_map_type(in_fd);

    /* minimum and maximum values for interpolating range */
    if (IN_TYPE == CELL_TYPE) {
        Rast_read_range(input, mapset, &int_range);
        min = (double)int_range.min;
        max = (double)int_range.max;
    }
    else {
        Rast_read_fp_range(input, mapset, &fp_range);
        min = (double)fp_range.min;
        max = (double)fp_range.max;
    }
    if (parm.min->answer) {
        min = strtod(parm.min->answer, 0);
        filter_min = 1;
    }
    if (parm.max->answer) {
        max = strtod(parm.max->answer, 0);
        filter_max = 1;
    }
    G_message(_("Input data range is %f to %f.\n"), min, max);

    /* determine input and output data types, advise user */
    OUT_TYPE = IN_TYPE;
    if (IN_TYPE == DCELL_TYPE) {
        if (parm.single_precision->answer) {
            OUT_TYPE = FCELL_TYPE;
        }
        else {
            OUT_TYPE = DCELL_TYPE;
        }
    }
    if (IN_TYPE == CELL_TYPE) {
        if ((!strcmp(parm.mode->answer, "wmean")) ||
            (!strcmp(parm.mode->answer, "mean"))
            || (!strcmp(parm.mode->answer, "median"))) {
            G_warning(_("Input data type is integer but interpolation mode is '%s'."),
                      parm.mode->answer);
            if (parm.single_precision->answer) {
                OUT_TYPE = FCELL_TYPE;
                G_warning(_("Output type changed to floating point (single)."));
            }
            else {
                OUT_TYPE = DCELL_TYPE;
                G_warning(_("Output type changed to floating point (double)."));
            }
        }
        else {
            if (parm.single_precision->answer) {
                G_warning(_("Ignoring '%c' flag. Output data type will be integer."),
                          parm.single_precision->key);
            }
        }
    }
    char *data_type_string_in;
    char *data_type_string_out;

    if (IN_TYPE == CELL_TYPE) {
        data_type_string_in = "integer";
    }
    else if (IN_TYPE == FCELL_TYPE) {
        data_type_string_in = "single";
    }
    else if (IN_TYPE == DCELL_TYPE) {
        data_type_string_in = "double";
    }
    if (OUT_TYPE == CELL_TYPE) {
        data_type_string_out = "integer";
    }
    else if (OUT_TYPE == FCELL_TYPE) {
        data_type_string_out = "single";
    }
    else if (OUT_TYPE == DCELL_TYPE) {
        data_type_string_out = "double";
    }

    /* initialize data type dependent cell handling functions */
    init_cell_funcs();

    G_message(_("Input data type is '%s' (%i bytes) and output data type is '%s' (%i bytes)."),
              data_type_string_in, CELL_IN_SIZE, data_type_string_out,
              CELL_OUT_SIZE);

    /* just print projected mem usage if user wants it so */
    G_message("Minimal estimated memory usage is %.3f MB.",
              ((double)estimate_mem_needed(cols, parm.mode->answer)) / 1024 /
              1024);
    if (parm.print_u->answer) {
        exit(0);
    }

    /* Allocate enough memory to read n="distance"x2+1 rows of input map data,
     * plus a buffer of "distance" size above and below the actual data
     * rows, and to the right and left.
     * The buffer will be filled with "no data" cells and has the
     * effect that we can later read data anywhere within the search
     * neighborhood, without having to worry about alignment problems.
     * */
    CELL_INPUT = G_malloc(CELL_IN_PTR_SIZE * WINDOW_HEIGHT);
    for (i = 0; i < WINDOW_HEIGHT; i++) {
        CELL_INPUT[i] = G_malloc(CELL_IN_SIZE * (cols + (PADDING_WIDTH * 2)));
    }
    for (i = 0; i < WINDOW_HEIGHT; i++) {
        Rast_set_null_value(CELL_INPUT[i], cols + (PADDING_WIDTH * 2),
                            IN_TYPE);
    }

    /*
     * Allocate array of raster row data handlers.
     * When reading rows from the input data buffer, we can use these
     * handlers instead of the original data pointers. That way, we
     * only need to read one new row of data from the disk each time
     * the neighborhood advances down one row in the region. Then, we
     * re-shuffle the row handlers, so that the first handler always
     * points to the first row of the neighborhood data, and the following
     * ones to the subsequent rows.
     * This should save a lot of disk access time.
     */
    CELL_INPUT_HANDLES = G_malloc(CELL_IN_PTR_SIZE * WINDOW_HEIGHT);

    /* create statistics object */
    cell_stats.values =
        G_malloc(sizeof(double) * WINDOW_WIDTH * WINDOW_HEIGHT);
    cell_stats.weights =
        G_malloc(sizeof(double) * WINDOW_WIDTH * WINDOW_HEIGHT);
    cell_stats.frequencies =
        G_malloc(sizeof(unsigned long) * WINDOW_WIDTH * WINDOW_HEIGHT);
    cell_stats.overwrite_value = G_malloc(sizeof(double));
    cell_stats.overwrite = 0;

    /* set statistics functions according to user option setting */
    if (!strcmp(parm.mode->answer, "wmean")) {
        build_weights_matrix(radius, power, res_x, res_y, 0,
                             parm.dist_m->answer);
        GET_STATS = &get_statistics_wmean;
        if (filter_min == 1 || filter_max == 1) {
            COLLECT_DATA = &collect_values_and_weights_filtered;
        }
        else {
            COLLECT_DATA = &collect_values_and_weights_unfiltered;
        }
    }
    if (!strcmp(parm.mode->answer, "mean")) {
        build_weights_matrix(radius, power, res_x, res_y, 1,
                             parm.dist_m->answer);
        GET_STATS = &get_statistics_mean;
        if (filter_min == 1 || filter_max == 1) {
            COLLECT_DATA = &collect_values_filtered;
        }
        else {
            COLLECT_DATA = &collect_values_unfiltered;
        }
    }
    if (!strcmp(parm.mode->answer, "median")) {
        build_weights_matrix(radius, power, res_x, res_y, 1,
                             parm.dist_m->answer);
        GET_STATS = &get_statistics_median;
        if (filter_min == 1 || filter_max == 1) {
            COLLECT_DATA = &collect_values_and_frequencies_filtered;
        }
        else {
            COLLECT_DATA = &collect_values_and_frequencies_unfiltered;
        }
    }
    if (!strcmp(parm.mode->answer, "mode")) {
        build_weights_matrix(radius, power, res_x, res_y, 1,
                             parm.dist_m->answer);
        GET_STATS = &get_statistics_mode;
        if (filter_min == 1 || filter_max == 1) {
            COLLECT_DATA = &collect_values_and_frequencies_filtered;
        }
        else {
            COLLECT_DATA = &collect_values_and_frequencies_unfiltered;
        }
    }

    /*
     *
     * MAIN LOOP
     *
     */

    /* Open output map with right data type */
    out_fd = Rast_open_new(output, OUT_TYPE);
    if (out_fd < 0) {
        G_fatal_error("Cannot open output map.");
        exit(EXIT_FAILURE);
    }

    /* Reserve memory for one output row buffer */
    CELL_OUTPUT = Rast_allocate_buf(OUT_TYPE);

    /* initialize output row */
    SET_NULL(CELL_OUTPUT, cols);

    /* produce uncertainty output map? */
    if (parm.error->answer) {
        /* Open output map with right data type */
        err_fd = Rast_open_new(parm.error->answer, FCELL_TYPE);
        if (err_fd < 0) {
            G_fatal_error("Cannot open uncertainty output map.");
            exit(EXIT_FAILURE);
        }
        ERR_OUTPUT = Rast_allocate_buf(FCELL_TYPE);
        /* initialize output row */
        Rast_set_f_null_value(ERR_OUTPUT, cols);
    }

    /* row indices to handle input data buffer */
    unsigned long center_row = (PADDING_HEIGHT * 2);
    unsigned long row_idx = 0;

    /* Visit every row in the input dataset.
     * To avoid making this code complex and having a lot of
     * if/then boundary checks while looping, processing has
     * been split into upper edge, main part and lower edge.
     * The upper and lower edge comprise the diameter of the
     * neighborhood window.
     * */

    G_message(_("Interpolating:"));
    unsigned long current_row = 0;

    /* first part: upper edge of region */
    init_handles();
    for (i = 0; i < DATA_HEIGHT; i++) {
        cell_input = get_input_row(PADDING_HEIGHT + i);

        cell_input = CELL_INPUT[PADDING_HEIGHT + i];
        for (j = 0; j < PADDING_WIDTH; j++) {
            cell_input += CELL_IN_SIZE;
        }
        Rast_get_row(in_fd, cell_input, i, IN_TYPE);
    }
    for (i = 0; i <= PADDING_HEIGHT; i++) {
        row_idx = PADDING_HEIGHT + i;
        interpolate_row(row_idx, cols, min, max, parm.preserve->answer,
                        min_cells, &cell_stats, write_error);
        /* write output row buffer to disk */
        Rast_put_row(out_fd, CELL_OUTPUT, OUT_TYPE);
        if (parm.error->answer)
            Rast_put_row(err_fd, ERR_OUTPUT, FCELL_TYPE);
        G_percent(current_row + 1, rows, 2);
        current_row++;
    }

    /* second part: region between upper and lower edge */
    for (i = 0; i < rows - (DATA_HEIGHT + 1); i++) {
        l = i;
        advance_one_row(in_fd, l);
        l++;
        row_idx = center_row;
        interpolate_row(row_idx, cols, min, max, parm.preserve->answer,
                        min_cells, &cell_stats, write_error);
        /* write output row buffer to disk */
        Rast_put_row(out_fd, CELL_OUTPUT, OUT_TYPE);
        if (parm.error->answer)
            Rast_put_row(err_fd, ERR_OUTPUT, FCELL_TYPE);
        G_percent(current_row + 1, rows, 2);
        current_row++;
    }

    /* third part: lower edge */
    init_handles();
    for (i = rows - DATA_HEIGHT; i < rows; i++) {
        row_idx = (DATA_HEIGHT + PADDING_HEIGHT) - (rows - i);
        cell_input = get_input_row(row_idx);
        Rast_get_row(in_fd, cell_input, i, IN_TYPE);
    }
    for (i = rows - PADDING_HEIGHT - 1; i < rows; i++) {
        row_idx = PADDING_HEIGHT + (DATA_HEIGHT) - (rows - i);
        interpolate_row(row_idx, cols, min, max, parm.preserve->answer,
                        min_cells, &cell_stats, write_error);
        /* write output row buffer to disk */
        Rast_put_row(out_fd, CELL_OUTPUT, OUT_TYPE);
        if (parm.error->answer)
            Rast_put_row(err_fd, ERR_OUTPUT, FCELL_TYPE);
        G_percent(current_row + 1, rows, 2);
        current_row++;
    }

    /* close all maps */
    Rast_close(out_fd);
    Rast_close(in_fd);
    if (parm.error->answer) {
        Rast_close(err_fd);
    }

    /* Free memory */
    for (i = 0; i < DATA_HEIGHT; i++) {
        G_free(WEIGHTS[i]);
    }
    G_free(WEIGHTS);

    if (CELL_INPUT != NULL) {
        for (i = 0; i < DATA_HEIGHT; i++) {
            G_free(CELL_INPUT[i]);
        }
        G_free(CELL_INPUT);
    }

    if (CELL_OUTPUT != NULL)
        G_free(CELL_OUTPUT);

    if (parm.error->answer)
        G_free(ERR_OUTPUT);

    G_free(cell_stats.values);
    G_free(cell_stats.weights);
    G_free(cell_stats.frequencies);
    G_free(cell_stats.overwrite_value);

    /* write metadata into result and error maps */
    Rast_short_history(parm.output->answer, "raster", &hist);
    Rast_put_cell_title(parm.output->answer,
                        "Result of interpolation/gap filling");
    if (parm.dist_m->answer) {
        Rast_append_format_history(&hist,
                                   "Settings: mode=%s, distance (map units)=%.6f, power=%.3f",
                                   parm.mode->answer, radius, power);
    }
    else {
        Rast_append_format_history(&hist,
                                   "Settings: mode=%s, distance (cells)=%lu, power=%.3f",
                                   parm.mode->answer, (unsigned long)radius,
                                   power);
    }
    Rast_append_format_history(&hist,
                               "          min=%.3f, max=%.3f, min. points=%lu",
                               min, max, min_cells);
    Rast_write_history(parm.output->answer, &hist);

    if (parm.error->answer) {
        Rast_short_history(parm.error->answer, "raster", &hist);
        Rast_put_cell_title(parm.error->answer,
                            "Uncertainty of interpolation/gap filling");
        Rast_append_format_history(&hist, "Result map: %s",
                                   parm.output->answer);
        Rast_append_format_history(&hist,
                                   "Theoretic range is '0' (lowest) to '1' (highest).");
        Rast_write_history(parm.error->answer, &hist);
    }

    finish = time(NULL);
    double ticks = difftime(finish, start);
    int hours = trunc(ticks / 3600);

    ticks = ticks - (hours * 3600);
    int minutes = trunc(ticks / 60);

    ticks = ticks - (minutes * 60);
    int seconds = trunc(ticks);

    /* DONE */
    G_done_msg(_("Processing time was %ih%im%is."), hours, minutes, seconds);

    return (EXIT_SUCCESS);
}

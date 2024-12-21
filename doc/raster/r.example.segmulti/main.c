/****************************************************************************
 *
 * MODULE:    r.example.segmulti
 * AUTHOR(S): Vaclav Petras
 *
 * PURPOSE:   Code explains use of Segment Library with multiple rasters
 *
 * COPYRIGHT: (C) 2019 by Vaclav Petras the GRASS Development Team
 *
 *            This program is free software under the GNU General Public
 *            License (>=v2). Read the file COPYING that comes with
 *            GRASS for details.
 *
 *****************************************************************************/

#include <stdlib.h>

#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/raster.h>
#include <grass/segment.h>

/* function declaration */
static void process(SEGMENT *raster_seg, int ninputs);

/* main function driving the execution */
int main(int argc, char *argv[])
{
    /* input and output raster names and file descriptors */
    char *output_name;
    int output_fd;

    /* type of the map (CELL/DCELL/...) */
    RASTER_MAP_TYPE map_type;

    /* variables for current and maximum dimensions */
    int nrows, ncols;
    int row, col;
    int input, ninputs;

    /* history structure holds meta-data (title, comments,..) */
    struct History history;

    /* options and description */
    struct GModule *module;
    struct Option *opt_inputs;
    struct Option *opt_output;

    /* initialize GRASS GIS library */
    G_gisinit(argv[0]);

    /* initialize module and its description */
    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("example"));
    G_add_keyword(_("segment library"));
    G_add_keyword(_("random access"));
    module->description =
        _("Code explains use of Segment Library with multiple rasters");

    /* define parameters */
    opt_inputs = G_define_standard_option(G_OPT_R_INPUTS);
    opt_output = G_define_standard_option(G_OPT_R_OUTPUT);

    /* options and flags parser */
    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    /* stores options and flags to variables */
    output_name = opt_output->answer;

    /* count input rasters */
    for (input = 0; opt_inputs->answers[input] != NULL; input++)
        ;
    ninputs = input;

    nrows = Rast_window_rows();
    ncols = Rast_window_cols();

    /* size of a segment */
    int srows = 64;
    int scols = 64;

    /* number of segments in memory */
    int nsegs = 4;

    map_type = DCELL_TYPE;
    size_t cell_size = Rast_cell_size(map_type);
    size_t segment_cell_size = cell_size * ninputs;

    /* open the raster for writing (checks if it possible) */
    output_fd = Rast_open_new(output_name, map_type);

    /* segment structure */
    SEGMENT raster_seg;

    /* initialize the segment structures */
    if (Segment_open(&raster_seg, G_tempfile(), nrows, ncols, srows, scols,
                     segment_cell_size, nsegs) != 1)
        G_fatal_error(_("Unable to create temporary segment file"));

    /* array to store file descriptors */
    int *input_fds = G_malloc(ninputs * sizeof(int));

    G_message(_("Loading %d raster maps"), ninputs);

    /* open existing raster maps for reading */
    for (input = 0; input < ninputs; input++) {
        input_fds[input] = Rast_open_old(opt_inputs->answers[input], "");
    }

    /* allocate input buffer */
    DCELL *row_buffer = Rast_allocate_d_buf();
    DCELL *seg_buffer = G_malloc(ncols * ninputs * sizeof(DCELL));

    for (row = 0; row < nrows; row++) {
        for (input = 0; input < ninputs; input++) {
            Rast_get_d_row(input_fds[input], row_buffer, row);
            for (col = 0; col < ncols; col++) {
                seg_buffer[col * ninputs + input] = row_buffer[col];
            }
        }
        if (Segment_put_row(&raster_seg, seg_buffer, row) < 1)
            G_fatal_error(_("Unable to write temporary segment file"));
    }

    /* now run the actual processing */
    process(&raster_seg, ninputs);

    /* make sure any pending disk operations take place */
    Segment_flush(&raster_seg);
    /* store the data permanently in a raster map */

    for (row = 0; row < nrows; row++) {
        for (col = 0; col < ncols; col++) {
            row_buffer[col] = 0;
        }
        Segment_get_row(&raster_seg, seg_buffer, row);
        for (input = 0; input < ninputs; input++) {
            for (col = 0; col < ncols; col++) {
                row_buffer[col] += seg_buffer[col * ninputs + input];
            }
        }
        Rast_put_row(output_fd, row_buffer, map_type);
    }

    /* memory cleanup */
    G_free(row_buffer);
    G_free(seg_buffer);

    /* closing raster maps and segment structures */
    Segment_close(&raster_seg);
    for (input = 0; input < ninputs; input++) {
        Rast_close(input_fds[input]);
    }
    Rast_close(output_fd);

    /* add command line incantation to history file */
    Rast_short_history(output_name, "raster", &history);
    Rast_command_history(&history);
    Rast_write_history(output_name, &history);

    exit(EXIT_SUCCESS);
}

/* This would be the main processing function.
 * Here we just hardcode a cell to modify.
 */
static void process(SEGMENT *raster_seg, int ninputs)
{
    /* buffer we use to hold the values */
    DCELL *values = G_malloc(ninputs * sizeof(DCELL *));

    /* row and column to access */
    int row = 1;
    int col = 3;

    /* pass the pointer, get the value */
    Segment_get(raster_seg, values, row, col);

    for (int input = 0; input < ninputs; input++) {
        values[input] = values[input] + 10000;
    }

    /* pass the pointer, set the value */
    Segment_put(raster_seg, values, row, col);
}


/****************************************************************************
 *
 * MODULE:    r.example.segment
 * AUTHOR(S): Vaclav Petras
 *
 * PURPOSE:   Slightly modifies the input data and stores the result
 *            (Code explains use of Segment Library)
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
static void process(SEGMENT *raster_seg);

/* main function driving the execution */
int main(int argc, char *argv[])
{
    /* input and output raster names and file descriptors */
    char *input_name;
    char *output_name;
    int input_fd;
    int output_fd;

    /* buffer for reading and writing rasters */
    void *buffer;

    /* type of the map (CELL/DCELL/...) */
    RASTER_MAP_TYPE map_type;

    /* variables for current and maximum rows and columns */
    int nrows, ncols;
    int row;

    /* history structure holds meta-data (title, comments,..) */
    struct History history;

    /* options and description */
    struct GModule *module;
    struct Option *input;
    struct Option *output;

    /* initialize GRASS GIS library */
    G_gisinit(argv[0]);

    /* initialize module and its description */
    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("example"));
    G_add_keyword(_("segment library"));
    G_add_keyword(_("random access"));
    module->description =
        _("Random access to raster using the Segment Library");

    /* define parameters */
    input = G_define_standard_option(G_OPT_R_INPUT);
    output = G_define_standard_option(G_OPT_R_OUTPUT);

    /* options and flags parser */
    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    /* stores options and flags to variables */
    input_name = input->answer;
    output_name = output->answer;

    /* determine the input map type (CELL/FCELL/DCELL) */
    map_type = Rast_map_type(input_name, "");
    size_t cell_size = Rast_cell_size(map_type);

    /* open existing raster map for reading */
    input_fd = Rast_open_old(input_name, "");

    /* open the raster for writing (checks if it possible) */
    output_fd = Rast_open_new(output_name, map_type);

    /* allocate input buffer */
    buffer = Rast_allocate_buf(map_type);

    nrows = Rast_window_rows();
    ncols = Rast_window_cols();

    /* size of a segment */
    int srows = 64;
    int scols = 64;

    /* number of segments in memory */
    int num_seg = 4;

    /* segment structure */
    SEGMENT raster_seg;

    /* initialize the segment structures */
    if (Segment_open(&raster_seg, G_tempfile(), nrows, ncols, srows, scols,
                     cell_size, num_seg) != 1)
        G_fatal_error("Unable to create temporary segment file");

    /* load data into the segment structures */
    for (row = 0; row < Rast_window_rows(); row++) {
        Rast_get_row(input_fd, buffer, row, map_type);
        if (Segment_put_row(&raster_seg, buffer, row) < 1)
            G_fatal_error(_("Unable to write temporary segment file"));
    }

    /* run the actual processing */
    process(&raster_seg);

    /* make sure any pending disk operations take place */
    Segment_flush(&raster_seg);
    /* store the data permanently in a raster map */
    for (row = 0; row < Rast_window_rows(); row++) {
        Segment_get_row(&raster_seg, buffer, row);
        Rast_put_row(output_fd, buffer, map_type);
    }

    /* memory cleanup */
    G_free(buffer);

    /* closing raster maps and segment structures */
    Segment_close(&raster_seg);
    Rast_close(input_fd);
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
static void process(SEGMENT *raster_seg)
{
    /* variable we use to hold the value */
    DCELL value;

    /* row and column to access */
    int row = 4;
    int col = 2;

    /* pass the pointer, get the value */
    Segment_get(raster_seg, (void *)&value, row, col);

    value = value + 100;

    /* pass the pointer, set the value */
    Segment_put(raster_seg, (void *)&value, row, col);
}

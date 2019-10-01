
/****************************************************************************
 *
 * MODULE:       i.zc
 * AUTHOR(S):    David B. Satnik Central Washington University GIS Laboratory 
 *               (original contributor), based on code provided by Bill Hoff
 *               at University of Illinois
 *               Markus Neteler <neteler itc.it>,
 *               Bernhard Reiter <bernhard intevation.de>, 
 *               Brad Douglas <rez touchofmadness.com>, 
 *               Glynn Clements <glynn gclements.plus.com>, 
 *               Jan-Oliver Wagner <jan intevation.de>
 * PURPOSE:      edge detection for imagery using zero crossings method
 * COPYRIGHT:    (C) 1999-2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/gmath.h>
#include <grass/glocale.h>


int main(int argc, char *argv[])
{
    /* Global variable & function declarations */
    double Thresh;
    int NumOrients;
    int inputfd, zcfd;		/* the input and output file descriptors */
    struct Cell_head window;
    CELL *cell_row;
    float Width;

    size_t i, j;			/* Loop control variables */
    int or, oc;			/* Original dimensions of image */
    int rows, cols;		/* Smallest powers of 2 >= number of rows & columns */
    size_t size;			/* the length of one side */
    size_t totsize;		/* the Total number of data points */
    double *data[2];		/* Data structure containing real & complex values of FFT */
    struct GModule *module;
    struct Option *input_map, *output_map, *width, *threshold, *orientations;
    struct History hist;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("imagery"));
    G_add_keyword(_("edges"));
    module->description =
	_("Zero-crossing \"edge detection\" raster "
	  "function for image processing.");

    /* define options */
    input_map = G_define_option();
    input_map->key = "input";
    input_map->type = TYPE_STRING;
    input_map->required = YES;
    input_map->multiple = NO;
    input_map->gisprompt = "old,cell,raster";
    input_map->description = _("Name of input raster map");

    output_map = G_define_option();
    output_map->key = "output";
    output_map->type = TYPE_STRING;
    output_map->required = YES;
    output_map->multiple = NO;
    output_map->gisprompt = "new,cell,raster";
    output_map->description = _("Zero crossing raster map");

    width = G_define_option();
    width->key = "width";
    width->type = TYPE_INTEGER;
    width->required = NO;
    width->multiple = NO;
    width->description = _("x-y extent of the Gaussian filter");
    width->answer = "9";

    threshold = G_define_option();
    threshold->key = "threshold";
    threshold->type = TYPE_DOUBLE;
    threshold->required = NO;
    threshold->multiple = NO;
    threshold->description = _("Sensitivity of Gaussian filter");
    threshold->answer = "1.0";

    orientations = G_define_option();
    orientations->key = "orientations";
    orientations->type = TYPE_INTEGER;
    orientations->required = NO;
    orientations->multiple = NO;
    orientations->description = _("Number of azimuth directions categorized");
    orientations->answer = "1";

    /* call parser */
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    /* open input cell map */
    inputfd = Rast_open_old(input_map->answer, "");

    Thresh = atof(threshold->answer);
    if (Thresh <= 0.0)
	G_fatal_error(_("Threshold less than or equal to zero not allowed"));
    /* A threshold value of 0.01 seems to give fairly good results on average.  */
    /* So we divide the threshold by 100 to get a value of 0.01 for the default */ 
    /* parameter value = 1. */

    Thresh = Thresh / 100.0;

    sscanf(width->answer, "%f", &Width);

    if (Width <= 0.0)
	G_fatal_error(_("Width less than or equal to zero not allowed"));

    sscanf(orientations->answer, "%d", &NumOrients);
    if (NumOrients < 1)
	G_fatal_error(_("Fewer than 1 orientation classes not allowed"));


    /* get the current window for later */
    G_get_set_window(&window);

    /* get the rows and columns in the current window */
    or = Rast_window_rows();
    oc = Rast_window_cols();
    rows = G_math_max_pow2((long)or);
    cols = G_math_max_pow2((long)oc);
    size = (rows > cols) ? rows : cols;
    totsize = size * size;

    G_message(_("Power 2 values : %d rows %d columns"), rows, cols);
    /* for del2g() below, size * size must fit into a 32bit signed integer
     * max value of a 32bit signed integer is 2^31 - 1
     * size, being a power of 2, must thus be not larger than 
     * 2^15 because 2^16 * 2^16 = 2^32 > 2^31 - 1 */
    if (size > 32768)
	G_fatal_error(_("The computational region is too large. "
	                "Please reduce the number of rows and/or columns to <= 32768."));

    /* Allocate appropriate memory for the structure containing
       the real and complex components of the FFT.  DATA[0] will
       contain the real, and DATA[1] the complex component.
     */
    data[0] = (double *)G_malloc(totsize * sizeof(double));
    data[1] = (double *)G_malloc(totsize * sizeof(double));

    /* Initialize real & complex components to zero */
    G_message(_("Initializing data..."));
    for (i = 0; i < (totsize); i++) {
	*(data[0] + i) = 0.0;
	*(data[1] + i) = 0.0;
    }

    /* allocate the space for one row of cell map data */
    cell_row = Rast_allocate_c_buf();

    /* Read in cell map values */
    G_message(_("Reading raster map..."));
    for (i = 0; i < or; i++) {
	Rast_get_c_row(inputfd, cell_row, i);

	for (j = 0; j < oc; j++)
	    *(data[0] + (i * size) + j) = (double)cell_row[j];
    }
    /* close input cell map and release the row buffer */
    Rast_close(inputfd);
    G_free(cell_row);

    /* take the del**2g of image */
    del2g(data, size, Width);

    /* find the zero crossings:  Here are several notes -
       1) this routine only uses the real values
       2) it places the zero crossings in the imaginary array */
    G_math_findzc(data[0], size, data[1], Thresh, NumOrients);

    /* open the output cell maps and allocate cell row buffers */
    G_message(_("Writing transformed data to file..."));
    zcfd = Rast_open_c_new(output_map->answer);

    cell_row = Rast_allocate_c_buf();

    /* Write out result to a new cell map */
    for (i = 0; i < or; i++) {
	for (j = 0; j < oc; j++) {
	    *(cell_row + j) = (CELL) (*(data[1] + i * cols + j));
	}
	Rast_put_row(zcfd, cell_row, CELL_TYPE);
    }
    Rast_close(zcfd);
    Rast_short_history(output_map->answer, "raster", &hist);
    Rast_command_history(&hist);
    Rast_write_history(output_map->answer, &hist);

    G_free(cell_row);

    /* Release memory resources */
    for (i = 0; i < 2; i++)
	G_free(data[i]);

    G_done_msg(_("Transform successful"));
    exit(EXIT_SUCCESS);
}

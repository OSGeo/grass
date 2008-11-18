
/****************************************************************************
 *
 * MODULE:       i.fft
 * AUTHOR(S):    David B. Satnik and Ali R. Vali (original contributors),
 *               Markus Neteler <neteler itc.it>
 *               Bernhard Reiter <bernhard intevation.de>, 
 *               Brad Douglas <rez touchofmadness.com>, 
 *               Glynn Clements <glynn gclements.plus.com>
 * PURPOSE:      processes a single input raster map layer
 *               and constructs the real and imaginary Fourier
 *               components in frequency space
 * COPYRIGHT:    (C) 1999-2007 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
/*
   FFT for GRASS by:
   Central Washington University GIS Laboratory
   Programmer: David B. Satnik

   Original FFT function provided by:
   Programmer : Ali R. Vali
   Center for Space Research
   WRW 402
   University of Texas
   Austin, TX 78712-1085

   (512) 471-6824

 */

#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/gmath.h>
#include <grass/glocale.h>

static void fft_colors(const char *name)
{
    struct Colors wave, colors;
    struct FPRange range;
    DCELL min, max;

    G_read_fp_range(name, G_mapset(), &range);
    G_get_fp_range_min_max(&range, &min, &max);
    G_make_wave_colors(&wave, min, max);
    G_abs_log_colors(&colors, &wave, 100);
    G_write_colors(name, G_mapset(), &colors);
    G_free_colors(&colors);
}

int main(int argc, char *argv[])
{
    /* Global variable & function declarations */
    struct GModule *module;
    struct {
	struct Option *orig, *real, *imag;
    } opt;
    const char *Cellmap_real, *Cellmap_imag;
    const char *Cellmap_orig;
    int inputfd, realfd, imagfd;	/* the input and output file descriptors */
    struct Cell_head window;
    DCELL *cell_real, *cell_imag;
    int rows, cols;		/* number of rows & columns */
    long totsize;		/* Total number of data points */
    double (*data)[2];		/* Data structure containing real & complex values of FFT */
    int i, j;			/* Loop control variables */

    G_gisinit(argv[0]);

    module = G_define_module();
    module->keywords = _("imagery");
    module->description =
	_("Fast Fourier Transform (FFT) for image processing.");

    /* define options */
    opt.orig = G_define_option();
    opt.orig->key = "input_image";
    opt.orig->type = TYPE_STRING;
    opt.orig->required = YES;
    opt.orig->multiple = NO;
    opt.orig->gisprompt = "old,cell,raster";
    opt.orig->description = _("Input raster map being fft");

    opt.real = G_define_option();
    opt.real->key = "real_image";
    opt.real->type = TYPE_STRING;
    opt.real->required = YES;
    opt.real->multiple = NO;
    opt.real->gisprompt = "new,cell,raster";
    opt.real->description = _("Output real part arrays stored as raster map");

    opt.imag = G_define_option();
    opt.imag->key = "imaginary_image";
    opt.imag->type = TYPE_STRING;
    opt.imag->required = YES;
    opt.imag->multiple = NO;
    opt.imag->gisprompt = "new,cell,raster";
    opt.imag->description = _("Output imaginary part arrays stored as raster map");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    Cellmap_orig = opt.orig->answer;
    Cellmap_real = opt.real->answer;
    Cellmap_imag = opt.imag->answer;

    inputfd = G_open_cell_old(Cellmap_orig, "");
    if (inputfd < 0)
	G_fatal_error(_("Unable to open input map <%s>"), Cellmap_orig);

    if (G_maskfd() >= 0)
	G_warning(_("Raster MASK found, consider to remove "
		    "(see man-page). Will continue..."));

    G_get_set_window(&window);	/* get the current window for later */

    /* get the rows and columns in the current window */
    rows = G_window_rows();
    cols = G_window_cols();
    totsize = rows * cols;

    /* Allocate appropriate memory for the structure containing
       the real and complex components of the FFT.  data[...][0] will
       contain the real, and data[...][1] the complex component.
     */
    data = G_malloc(rows * cols * 2 * sizeof(double));

    /* allocate the space for one row of cell map data */
    cell_real = G_allocate_d_raster_buf();
    cell_imag = G_allocate_d_raster_buf();

#define C(i, j) ((i) * cols + (j))

    /* Read in cell map values */
    G_message(_("Reading the raster map..."));
    for (i = 0; i < rows; i++) {
	if (G_get_d_raster_row(inputfd, cell_real, i) < 0)
	    G_fatal_error(_("Error while reading input raster map."));
	for (j = 0; j < cols; j++) {
	    data[C(i, j)][0] = cell_real[j];
	    data[C(i, j)][1] = 0.0;
	}
    }

    /* close input cell map */
    G_close_cell(inputfd);

    /* perform FFT */
    G_message(_("Starting FFT..."));
    fft2(-1, data, totsize, cols, rows);
    G_message(_("FFT completed..."));

    /* open the output cell maps */
    if ((realfd = G_open_fp_cell_new(Cellmap_real)) < 0)
	G_fatal_error(_("Unable to open real output map <%s>"), Cellmap_real);
    if ((imagfd = G_open_fp_cell_new(Cellmap_imag)) < 0)
	G_fatal_error(_("Unable to open imaginary output map <%s>"), Cellmap_imag);

#define SWAP1(a, b)				\
    do {					\
	double temp = (a);			\
	(a) = (b);				\
	(b) = temp;				\
    } while (0)

#define SWAP2(a, b)				\
    do {					\
	SWAP1(data[(a)][0], data[(b)][0]);	\
	SWAP1(data[(a)][1], data[(b)][1]);	\
    } while (0)

    /* rotate the data array for standard display */
    G_message(_("Rotating data..."));
    for (i = 0; i < rows; i++)
	for (j = 0; j < cols / 2; j++)
	    SWAP2(C(i, j), C(i, j + cols / 2));
    for (i = 0; i < rows / 2; i++)
	for (j = 0; j < cols; j++)
	    SWAP2(C(i, j), C(i + rows / 2, j));

    G_message(_("Writing transformed data..."));

    for (i = 0; i < rows; i++) {
	for (j = 0; j < cols; j++) {
	    cell_real[j] = data[C(i, j)][0];
	    cell_imag[j] = data[C(i, j)][1];
	}
	G_put_d_raster_row(realfd, cell_real);
	G_put_d_raster_row(imagfd, cell_imag);
    }

    G_close_cell(realfd);
    G_close_cell(imagfd);

    G_free(cell_real);
    G_free(cell_imag);

    /* set up the color tables */
    fft_colors(Cellmap_real);
    fft_colors(Cellmap_imag);

    /* Release memory resources */
    G_free(data);

    G_done_msg(_("Transform successful."));

    exit(EXIT_SUCCESS);
}

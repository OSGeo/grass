
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
 * COPYRIGHT:    (C) 1999-2008 by the GRASS Development Team
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
#include <grass/raster.h>
#include <grass/gmath.h>
#include <grass/glocale.h>

static void fft_colors(const char *name)
{
    struct Colors wave, colors;
    struct FPRange range;
    DCELL min, max;

    Rast_read_fp_range(name, G_mapset(), &range);
    Rast_get_fp_range_min_max(&range, &min, &max);
    Rast_make_wave_colors(&wave, min, max);
    Rast_abs_log_colors(&colors, &wave, 100);
    Rast_write_colors(name, G_mapset(), &colors);
    Rast_free_colors(&colors);
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
    G_add_keyword(_("imagery"));
    G_add_keyword(_("transformation"));
    G_add_keyword(_("Fast Fourier Transform"));
    module->description =
	_("Fast Fourier Transform (FFT) for image processing.");

    /* define options */
    /* define options */
    opt.orig = G_define_standard_option(G_OPT_R_INPUT);
    opt.orig->key = "input_image";

    opt.real = G_define_standard_option(G_OPT_R_OUTPUT);
    opt.real->key = "real_image";
    opt.real->description = _("Name for output real part arrays stored as raster map");

    opt.imag = G_define_standard_option(G_OPT_R_OUTPUT);
    opt.imag->key = "imaginary_image";
    opt.imag->description = _("Name for output imaginary part arrays stored as raster map");
    
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    Cellmap_orig = opt.orig->answer;
    Cellmap_real = opt.real->answer;
    Cellmap_imag = opt.imag->answer;

    inputfd = Rast_open_old(Cellmap_orig, "");

    if (Rast_maskfd() >= 0)
	G_warning(_("Raster MASK found, consider to remove "
		    "(see man-page). Will continue..."));

    G_get_set_window(&window);	/* get the current window for later */

    /* get the rows and columns in the current window */
    rows = Rast_window_rows();
    cols = Rast_window_cols();
    totsize = rows * cols;

    /* Allocate appropriate memory for the structure containing
       the real and complex components of the FFT.  data[...][0] will
       contain the real, and data[...][1] the complex component.
     */
    data = G_malloc(rows * cols * 2 * sizeof(double));

    /* allocate the space for one row of cell map data */
    cell_real = Rast_allocate_d_buf();
    cell_imag = Rast_allocate_d_buf();

#define C(i, j) ((i) * cols + (j))

    /* Read in cell map values */
    G_message(_("Reading the raster map <%s>..."),
	      Cellmap_orig);
    for (i = 0; i < rows; i++) {
	Rast_get_d_row(inputfd, cell_real, i);
	for (j = 0; j < cols; j++) {
	    data[C(i, j)][0] = cell_real[j];
	    data[C(i, j)][1] = 0.0;
	}

	G_percent(i+1, rows, 2);
    }

    /* close input cell map */
    Rast_close(inputfd);

    /* perform FFT */
    G_message(_("Starting FFT..."));
    fft2(-1, data, totsize, cols, rows);

    /* open the output cell maps */
    realfd = Rast_open_fp_new(Cellmap_real);
    imagfd = Rast_open_fp_new(Cellmap_imag);

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
	Rast_put_d_row(realfd, cell_real);
	Rast_put_d_row(imagfd, cell_imag);

	G_percent(i+1, rows, 2);
    }

    Rast_close(realfd);
    Rast_close(imagfd);

    G_free(cell_real);
    G_free(cell_imag);

    /* set up the color tables */
    fft_colors(Cellmap_real);
    fft_colors(Cellmap_imag);

    /* Release memory resources */
    G_free(data);

    G_done_msg(_(" "));

    exit(EXIT_SUCCESS);
}

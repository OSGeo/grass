
/****************************************************************************
 *
 * MODULE:       i.ifft
 * AUTHOR(S):    David B. Satnik and Ali R. Vali (original contributors),
 *               Markus Neteler <neteler itc.it>
 *               Bernhard Reiter <bernhard intevation.de>, 
 *               Brad Douglas <rez touchofmadness.com>, 
 *               Glynn Clements <glynn gclements.plus.com>
 * PURPOSE:      processes the real and imaginary Fourier
 *               components in frequency space and constract raster map
 * COPYRIGHT:    (C) 1999-2008 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
/*
   Central Washington University GIS Laboratory
   Programmer: David B. Satnik
   and
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
#include <grass/glocale.h>
#include <grass/gmath.h>

static void fft_colors(const char *name)
{
    struct Colors colors;
    struct FPRange range;
    DCELL min, max;

    /* make a real component color table */
    Rast_read_fp_range(name, G_mapset(), &range);
    Rast_get_fp_range_min_max(&range, &min, &max);
    Rast_make_grey_scale_fp_colors(&colors, min, max);
    Rast_write_colors(name, G_mapset(), &colors);
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
    int realfd, imagfd,  outputfd, maskfd;	/* the input and output file descriptors */
    struct Cell_head realhead, imaghead;
    DCELL *cell_real, *cell_imag;
    CELL *maskbuf;

    int i, j;			/* Loop control variables */
    int rows, cols;		/* number of rows & columns */
    long totsize;		/* Total number of data points */
    double (*data)[2];		/* Data structure containing real & complex values of FFT */

    G_gisinit(argv[0]);

    /* Set description */
    module = G_define_module();
    G_add_keyword(_("imagery"));
    G_add_keyword(_("transformation"));
    G_add_keyword(_("Fast Fourier Transform"));
    module->description =
	_("Inverse Fast Fourier Transform (IFFT) for image processing.");

    /* define options */
    opt.real = G_define_standard_option(G_OPT_R_INPUT);
    opt.real->key = "real_image";
    opt.real->description = _("Name of input raster map (image fft, real part)");

    opt.imag = G_define_standard_option(G_OPT_R_INPUT);
    opt.imag->key = "imaginary_image";
    opt.imag->description = _("Name of input raster map (image fft, imaginary part");

    opt.orig = G_define_standard_option(G_OPT_R_OUTPUT);
    opt.orig->key = "output_image";
    opt.orig->description = _("Name for output raster map");
    
    /*call parser */
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    Cellmap_real = opt.real->answer;
    Cellmap_imag = opt.imag->answer;
    Cellmap_orig = opt.orig->answer;

    /* get and compare the original window data */
    Rast_get_cellhd(Cellmap_real, "", &realhead);
    Rast_get_cellhd(Cellmap_imag, "", &imaghead);

    if (realhead.proj   != imaghead.proj   ||
	realhead.zone   != imaghead.zone   ||
	realhead.north  != imaghead.north  ||
	realhead.south  != imaghead.south  ||
	realhead.east   != imaghead.east   ||
	realhead.west   != imaghead.west   ||
	realhead.ew_res != imaghead.ew_res ||
	realhead.ns_res != imaghead.ns_res)
	G_fatal_error(_("The real and imaginary original windows did not match"));

    Rast_set_window(&realhead);	/* set the window to the whole cell map */

    /* open input raster map */
    realfd = Rast_open_old(Cellmap_real, "");
    imagfd = Rast_open_old(Cellmap_imag, "");

    /* get the rows and columns in the current window */
    rows = Rast_window_rows();
    cols = Rast_window_cols();
    totsize = rows * cols;

    /* Allocate appropriate memory for the structure containing
       the real and complex components of the FFT.  DATA[0] will
       contain the real, and DATA[1] the complex component.
     */
    data = G_malloc(rows * cols * 2 * sizeof(double));

    /* allocate the space for one row of cell map data */
    cell_real = Rast_allocate_d_buf();
    cell_imag = Rast_allocate_d_buf();
    
#define C(i, j) ((i) * cols + (j))

    /* Read in cell map values */
    G_message(_("Reading raster maps..."));
    for (i = 0; i < rows; i++) {
	Rast_get_d_row(realfd, cell_real, i);
	Rast_get_d_row(imagfd, cell_imag, i);
	for (j = 0; j < cols; j++) {
	    data[C(i, j)][0] = cell_real[j];
	    data[C(i, j)][1] = cell_imag[j];
	}
	G_percent(i+1, rows, 2);
    }

    /* close input cell maps */
    Rast_close(realfd);
    Rast_close(imagfd);

    /* Read in cell map values */
    G_message(_("Masking raster maps..."));
    maskfd = Rast_maskfd();
    if (maskfd >= 0) {
	maskbuf = Rast_allocate_c_buf();

	for (i = 0; i < rows; i++) {
	    Rast_get_c_row(maskfd, maskbuf, i);
	    for (j = 0; j < cols; j++) {
		if (maskbuf[j] == 0) {
		    data[C(i, j)][0] = 0.0;
		    data[C(i, j)][1] = 0.0;
		}
	    }
	    G_percent(i+1, rows, 2);
	}

	Rast_close(maskfd);
	G_free(maskbuf);
    }

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

    /* perform inverse FFT */
    G_message(_("Starting Inverse FFT..."));
    fft2(1, data, totsize, cols, rows);

    /* open the output cell map */
    outputfd = Rast_open_fp_new(Cellmap_orig);

    /* Write out result to a new cell map */
    G_message(_("Writing raster map <%s>..."),
	      Cellmap_orig);
    for (i = 0; i < rows; i++) {
	for (j = 0; j < cols; j++)
	    cell_real[j] = data[C(i, j)][0];
	Rast_put_d_row(outputfd, cell_real);

	G_percent(i+1, rows, 2);
    }

    Rast_close(outputfd);

    G_free(cell_real);
    G_free(cell_imag);

    fft_colors(Cellmap_orig);

    /* Release memory resources */
    G_free(data);

    G_done_msg(" ");

    exit(EXIT_SUCCESS);
}

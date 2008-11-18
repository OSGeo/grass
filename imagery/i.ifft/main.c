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
#include <grass/glocale.h>
#include <grass/gmath.h>

static void fft_colors(const char *name)
{
    struct Colors colors;
    struct FPRange range;
    DCELL min, max;

    /* make a real component color table */
    G_read_fp_range(name, G_mapset(), &range);
    G_get_fp_range_min_max(&range, &min, &max);
    G_make_grey_scale_fp_colors(&colors, min, max);
    G_write_colors(name, G_mapset(), &colors);
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
    module->keywords = _("imagery");
    module->description =
	_("Inverse Fast Fourier Transform (IFFT) for image processing.");

    /* define options */
    opt.real = G_define_option();
    opt.real->key = "real_image";
    opt.real->type = TYPE_STRING;
    opt.real->required = YES;
    opt.real->multiple = NO;
    opt.real->gisprompt = "old,cell,raster";
    opt.real->description = _("Input raster map (image fft, real part)");

    opt.imag = G_define_option();
    opt.imag->key = "imaginary_image";
    opt.imag->type = TYPE_STRING;
    opt.imag->required = YES;
    opt.imag->multiple = NO;
    opt.imag->gisprompt = "old,cell,raster";
    opt.imag->description = _("Input raster map (image fft, imaginary part");

    opt.orig = G_define_option();
    opt.orig->key = "output_image";
    opt.orig->type = TYPE_STRING;
    opt.orig->required = YES;
    opt.orig->multiple = NO;
    opt.orig->gisprompt = "new,cell,raster";
    opt.orig->description = _("Output inverse raster map after IFFT");

    /*call parser */
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    Cellmap_real = opt.real->answer;
    Cellmap_imag = opt.imag->answer;
    Cellmap_orig = opt.orig->answer;

    /* open input raster map */
    realfd = G_open_cell_old(Cellmap_real, "");
    if (realfd < 0)
	G_fatal_error(_("Unable to open real input map <%s>"), Cellmap_real);

    imagfd = G_open_cell_old(Cellmap_imag, "");
    if (imagfd < 0)
	G_fatal_error(_("Unable to open imaginary input map <%s>"), Cellmap_imag);

    /* get and compare the original window data */
    G_get_cellhd(Cellmap_real, "", &realhead);
    G_get_cellhd(Cellmap_imag, "", &imaghead);

    if (realhead.proj   != imaghead.proj   ||
	realhead.zone   != imaghead.zone   ||
	realhead.north  != imaghead.north  ||
	realhead.south  != imaghead.south  ||
	realhead.east   != imaghead.east   ||
	realhead.west   != imaghead.west   ||
	realhead.ew_res != imaghead.ew_res ||
	realhead.ns_res != imaghead.ns_res)
	G_fatal_error(_("The real and imaginary original windows did not match."));

    G_set_window(&realhead);	/* set the window to the whole cell map */

    /* get the rows and columns in the current window */
    rows = G_window_rows();
    cols = G_window_cols();
    totsize = rows * cols;

    /* Allocate appropriate memory for the structure containing
       the real and complex components of the FFT.  DATA[0] will
       contain the real, and DATA[1] the complex component.
     */
    data = G_malloc(rows * cols * 2 * sizeof(double));

    /* allocate the space for one row of cell map data */
    cell_real = G_allocate_d_raster_buf();
    cell_imag = G_allocate_d_raster_buf();

#define C(i, j) ((i) * cols + (j))

    /* Read in cell map values */
    G_message(_("Reading the raster maps..."));
    for (i = 0; i < rows; i++) {
	if (G_get_d_raster_row(realfd, cell_real, i) < 0)
	    G_fatal_error(_("Error while reading real input raster map."));
	if (G_get_d_raster_row(imagfd, cell_imag, i) < 0)
	    G_fatal_error(_("Error while reading imaginary input raster map."));
	for (j = 0; j < cols; j++) {
	    data[C(i, j)][0] = cell_real[j];
	    data[C(i, j)][1] = cell_imag[j];
	}
    }

    /* close input cell maps */
    G_close_cell(realfd);
    G_close_cell(imagfd);

    /* Read in cell map values */
    G_message(_("Masking the raster maps..."));
    maskfd = G_maskfd();
    if (maskfd >= 0) {
	maskbuf = G_allocate_cell_buf();

	for (i = 0; i < rows; i++) {
	    G_get_map_row(maskfd, maskbuf, i);
	    for (j = 0; j < cols; j++) {
		if (maskbuf[j] == 0) {
		    data[C(i, j)][0] = 0.0;
		    data[C(i, j)][1] = 0.0;
		}
	    }
	}

	G_close_cell(maskfd);
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
    G_message(_("Rotating data arrays..."));
    for (i = 0; i < rows; i++)
	for (j = 0; j < cols / 2; j++)
	    SWAP2(C(i, j), C(i, j + cols / 2));
    for (i = 0; i < rows / 2; i++)
	for (j = 0; j < cols; j++)
	    SWAP2(C(i, j), C(i + rows / 2, j));

    /* perform inverse FFT */
    G_message(_("Starting Inverse FFT..."));
    fft2(1, data, totsize, cols, rows);
    G_message(_("Inverse FFT completed..."));

    /* open the output cell map */
    if ((outputfd = G_open_fp_cell_new(Cellmap_orig)) < 0)
	G_fatal_error(_("Unable to open output map <%s>"), Cellmap_orig);

    /* Write out result to a new cell map */
    G_message(_("Writing output map..."));
    for (i = 0; i < rows; i++) {
	for (j = 0; j < cols; j++)
	    cell_real[j] = data[C(i, j)][0];
	G_put_d_raster_row(outputfd, cell_real);
    }

    G_close_cell(outputfd);

    G_free(cell_real);
    G_free(cell_imag);

    fft_colors(Cellmap_orig);

    /* Release memory resources */
    G_free(data);

    G_done_msg(_("Transform successful."));

    exit(EXIT_SUCCESS);
}

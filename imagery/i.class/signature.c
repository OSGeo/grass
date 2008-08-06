#include <string.h>
#include <unistd.h>
#include <math.h>
#include <stdio.h>
#include <signal.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "globals.h"
#include "local_proto.h"

#define PN Region.perimeter_npoints
#define P  Region.perimeter

#define SN  Sigs.nsigs
#define SIG Sigs.sig[SN-1]

#define MEAN(b)		(Band_sum[b]/np)
#define STD_DEV(b)	((float) sqrt ((double) VAR(b,b) / np))
#define VAR(b1,b2)	(Band_product[b1][b2] - Band_sum[b1]*Band_sum[b2]/np)

static int *Band_min;
static int *Band_max;
static int *Range_max, *Range_min;
static float *Band_sum;		/* for means, variances */
static float **Band_product;
static int **Band_histo;
static int np;			/* number of points in sample */
static int usable_signature = 0;
static int dont_save(void);
static int yes_save(void);
static int done(void);


/**********************************************************************/

/************************** ###### ************************************/

/**********************************************************************/
int init_sig_routines(size_t nbands)
{
    int i;

    if ((Range_min = (int *)G_calloc(nbands, sizeof(int))) == NULL ||
	(Range_max = (int *)G_calloc(nbands, sizeof(int))) == NULL ||
	(Band_min = (int *)G_calloc(nbands, sizeof(int))) == NULL ||
	(Band_max = (int *)G_calloc(nbands, sizeof(int))) == NULL ||
	(Band_sum = (float *)G_calloc(nbands, sizeof(float))) == NULL ||
	(Band_product = (float **)G_calloc(nbands, sizeof(float *))) == NULL
	|| (Band_histo = (int **)G_calloc(nbands, sizeof(int *))) == NULL)
	G_fatal_error(_("Unable to allocate space for signature statistics."));
    for (i = 0; i < nbands; i++) {
	if ((Band_product[i] =
	     (float *)G_calloc(nbands, sizeof(float))) == NULL ||
	    (Band_histo[i] = (int *)G_calloc(MAX_CATS, sizeof(int))) == NULL)
	    G_fatal_error(_("Unable to allocate space for signature statistics."));
    }

    return 0;
}


/**********************************************************************/

/************************** ###### ************************************/

/**********************************************************************/
int prepare_signature(int nbands)
{
    char msg[100];
    int b, b2;
    int n;
    int i;
    int x0, x1;
    int x;
    int y;
    void (*prev_sigalarm) ();

    Menu_msg("Preparing signature...");

    usable_signature = 0;
    if (PN % 2) {
	G_warning(_("prepare_signature: outline has odd number of points."));
	return (0);
    }

    for (b = 0; b < nbands; b++) {
	Band_sum[b] = 0.0;
	for (b2 = 0; b2 < nbands; b2++)
	    Band_product[b][b2] = 0.0;
	for (b2 = 0; b2 < MAX_CATS; b2++)
	    Band_histo[b][b2] = 0;
    }
    np = 0;

    signalflag.alarm = 0;
    prev_sigalarm = signal(SIGALRM, sigalarm);
    alarm(10);
    for (i = 1; i < PN; i += 2) {
	if (signalflag.interrupt)
	    break;

	if (signalflag.alarm) {
	    alarm(0);
	    signalflag.alarm = 0;
	    sprintf(msg, "Preparing signature... %.0f%% complete",
		    (float)i / PN * 100.0);
	    Menu_msg(msg);
	    alarm(10);
	}

	y = P[i].y;
	if (y != P[i - 1].y) {
	    G_warning(_("prepare_signature: scan line %d has odd number of points."),
		      (i + 1) / 2);
	    return (0);
	}
	readbands(nbands, y);

	x0 = P[i - 1].x - 1;
	x1 = P[i].x - 1;

	if (x0 > x1) {
	    G_warning(_("signature: perimeter points out of order."));
	    return (0);
	}

	for (x = x0; x <= x1; x++) {
	    np++;		/* count interior points */
	    for (b = 0; b < nbands; b++) {
		n = Bandbuf[b][x];
		if (n < 0 || n > MAX_CATS - 1) {
		    G_warning(_("prepare_signature: data error."));
		    return (0);
		}
		Band_sum[b] += n;	/* sum for means */
		Band_histo[b][n]++;	/* histogram */
		if (np == 1)
		    Band_min[b] = Band_max[b] = n;
		if (Band_min[b] > n)
		    Band_min[b] = n;	/* absolute min, max */
		if (Band_max[b] < n)
		    Band_max[b] = n;

		for (b2 = 0; b2 <= b; b2++)	/* products for variance */
		    Band_product[b][b2] += n * Bandbuf[b2][x];
	    }
	}
    }
    alarm(0);
    signal(SIGALRM, prev_sigalarm);
    Menu_msg("");

    return (usable_signature = !signalflag.interrupt);
}

/**********************************************************************/

/************************** ###### ************************************/

/**********************************************************************/

int first_display;
extern int Display_color;
extern char *Display_color_name;
extern float Nstd;

#define INP_STD 1
#define INP_COLOR 2
#define DISPLAY 3
#define DONE 4

int show_signature(int nbands, double default_nstd)
{
    int b;
    int selection = 0;
    static int use = 1;
    char std_str[50], color_str[50];
    float dist, mean;

    static Objects objects[] = {
	INFO("Signature Menu:", &use),
	MENU("", input_std, &use),
	MENU("", input_color, &use),
	MENU(" Display matches ", display_signature, &use),
	MENU(" Done ", done, &use),
	{0}
    };
    objects[1].label = std_str;
    objects[2].label = color_str;

    Menu_msg("Drawing histograms...");
    histograms(nbands, Band_sum, Band_product, Band_histo, np,
	       Band_min, Band_max, Nstd, BEFORE_STD);
    Nstd = 1.5;
    for (b = 0; b < nbands; b++) {
	dist = Nstd * STD_DEV(b);
	mean = MEAN(b);
	Range_min[b] = mean - dist + 0.5;
	Range_max[b] = mean + dist + 0.5;
    }
    first_display = 1;

    while (selection != DONE) {
	sprintf(std_str, " Set std dev's (%5.2f) ", Nstd);
	sprintf(color_str, " Set color (%s) ", Display_color_name);
	selection = Input_pointer(objects);
	switch (selection) {
	case INP_STD:		/* Input Number of Std. Deviations */
	    /* set min/max for each band - Nstd standard deviations from mean
	       not exceed actual min and max */
	    for (b = 0; b < nbands; b++) {
		dist = Nstd * STD_DEV(b);
		mean = MEAN(b);
		Range_min[b] = mean - dist + 0.5;
		Range_max[b] = mean + dist + 0.5;
	    }
	    Menu_msg("Drawing histograms...");
	    histograms(nbands, Band_sum, Band_product, Band_histo, np,
		       Range_min, Range_max, Nstd, AFTER_STD);

	    /* remove the mask file, if it exists */
	    first_display = 1;
	    remove_mask();
	    break;
	case INP_COLOR:	/* Input Color for Display */
	    break;
	case DISPLAY:		/* Display the matching cells */
	    first_display = 0;
	    break;
	case DONE:		/* done here, go back to command menu */
	    Menu_msg("");
	    break;
	default:
	    G_warning(_("Unknown Menu selection in show_signature()."));
	}

    }

    /* remove the created mask file */
    remove_mask();

    return 0;
}

static int done(void)
{
    return (DONE);
}

/**********************************************************************/

/************************** ###### ************************************/

/**********************************************************************/
int display_signature(void)
{
    int fd;
    CELL *buffer;
    struct Cell_head cellhd;
    register int n;
    register int col;
    register int nbands;
    int row, nrows, ncols;
    struct Colors mask_colors;

    if (first_display) {
	Menu_msg("Finding cells that match the signature...");

	nbands = Refer.nfiles;

	/* build new mask based on current signature and Nstd */
	G_set_window(&VIEW_MAP1->cell.head);
	open_band_files();

	if ((fd = G_open_cell_new(MASK)) < 0)
	    G_fatal_error(_("Unable to open the cell map MASK."));
	if ((buffer = G_allocate_cell_buf()) == NULL)
	    G_fatal_error(_("Unable to allocate the cell buffer in display_signature()."));
	nrows = G_window_rows();
	ncols = G_window_cols();

	for (row = 0; row < nrows; row++) {
	    readbands(nbands, row);
	    for (col = 0; col < ncols; col++) {
		buffer[col] = (CELL) 0;
		for (n = 0; n < nbands; n++) {
		    if (Bandbuf[n][col] < Range_min[n] ||
			Bandbuf[n][col] > Range_max[n])
			goto past;	/* if not in range jump out past the assignment */
		}
		buffer[col] = (CELL) 1;
	      past:;
	    }
	    G_put_raster_row(fd, buffer, CELL_TYPE);
	}

	G_close_cell(fd);
	close_band_files();
    }				/* end of if first_display */

    /* generate and write the color table for the mask */
    G_init_colors(&mask_colors);
    G_set_color((CELL) 1, Color_table[Display_color].red,
		Color_table[Display_color].grn,
		Color_table[Display_color].blue, &mask_colors);
    G_write_colors(MASK, G_mapset(), &mask_colors);

    /* display new mask */
    if (G_get_cellhd(MASK, G_mapset(), &cellhd) != 0)
	G_fatal_error(_("Did not find input cell map MASK."));
    G_adjust_window_to_box(&cellhd, &VIEW_MASK1->cell.head, VIEW_MASK1->nrows,
			   VIEW_MASK1->ncols);
    draw_cell(VIEW_MASK1, OVER_LAY);

    return (DISPLAY);
}


/**********************************************************************/

/************************** ###### ************************************/

/**********************************************************************/
/* for menu to know if we have a signature */
int have_signature(void)
{
    return (usable_signature);
}

/**********************************************************************/

/************************** ###### ************************************/

/**********************************************************************/
/* routine to save the signature into the signature structure */
static int use2 = 1;

int save_signature(void)
{
    static Objects objects[] = {
	INFO("Do you want to save this Signature?", &use2),
	MENU(" Yes ", yes_save, &use2),
	MENU(" No ", dont_save, &use2),
	{0}
    };

    Input_pointer(objects);
    Menu_msg("");

    return 0;
}

static int yes_save(void)
{
    int b, b2;
    char tempstr[100];

    /* get a new signature */
    I_new_signature(&Sigs);

    /* get signature name */
    tempstr[0] = '\0';
    Menu_msg("Input signature description on keyboard...");
    Curses_prompt_gets("Signature Description? ", tempstr);
    strcpy(SIG.desc, tempstr);
    use_mouse_msg();

    /* save the signature in a Sig structure */
    SIG.npoints = np;
    SIG.status = 1;
    for (b = 0; b < Sigs.nbands; b++) {
	SIG.mean[b] = MEAN(b);
	for (b2 = 0; b2 <= b; b2++)
	    SIG.var[b][b2] = VAR(b, b2) / (np - 1);
    }
    Menu_msg("");

    return (1);
}


static int dont_save(void)
{
    return (1);
}


/**********************************************************************/

/************************** ###### ************************************/

/**********************************************************************/
/* routine to write out the signature structure */
int write_signatures(void)
{
    Menu_msg("Saving Signature File...");
    I_write_signatures(outsig_fd, &Sigs);
    fclose(outsig_fd);
    G_sleep(1);
    Menu_msg("Done.");

    return 0;
}

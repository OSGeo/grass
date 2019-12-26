
/***************************************************************************
                 atcorr - atmospheric correction for Grass GIS

                                    was
        6s - Second Simulation of Satellite Signal in the Solar Spectrum.
                             -------------------
    begin                : Fri Jan 10 2003
    copyright            : (C) 2003 by Christo Zietsman
    email                : 13422863@sun.ac.za

    This program has been rewriten in C/C++ from the fortran source found
    on http://www.ltid.inpe.br/dsr/mauro/6s/index.html. This code is
    provided as is, with no implied warranty and under the conditions
    and restraint as placed on it by previous authors.

    atcorr is an atmospheric correction module for Grass GIS.  Limited testing
    has been done and therefore it should not be assumed that it will work
    for all possible inputs.

    Care has been taken to test new functionality brought to the module by
    Grass such as the command-line options and flags. The extra feature of
    supplying an elevation map has not been run to completion, because it
    takes to long and no sensible data for the test data was at hand.
    Testing would be welcomed. :)  
**********

* Code clean-up and port to GRASS 6.3, 15.12.2006:
  Yann Chemin, ychemin(at)gmail.com 

* Addition of IRS-1C LISS, Feb 2009: Markus Neteler

* input elevation/visibility map: efficient cache with dynamic memory 
* allocation: Markus Metz, Apr 2011

***************************************************************************/

#include <cstdlib>
#include <map>
#include <cmath>

extern "C"
{
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include <grass/rbtree.h>
}

#include "transform.h"
#include "6s.h"

/* MODIS surface reflectance (MOD09) uses pre-computed 6S parameters
 * for 10 aerosol optical depths. Here we use finer grain. */

/* TICache: create bins for altitude and visibility */
/* altitude range: appr. 0 - 9000
 * bin 10: change between bins is 0.05 - 0.16%
 * bin 100: change between bins is 0.5 - 1.5% 
 * bin 1000: change between bins is 6 - 13%*/
#define BIN_ALT 10	/* unit is meters */
/* visibility range: 0 - 50000
 * bin 10: change between bins is 0.004 - 0.16%
 * bin 100: change between bins is 0.01 - 1.6% 
 * bin 1000: change between bins is 0.1 - 13%*/
#define BIN_VIS 100	/* unit is meters */

/* uncomment to disable cache usage */
/* #define _NO_OPTIMIZE_ */

/* Input options and flags */
struct Options
{
    /* options */
    struct Option *iimg;	/* input satellite image */
    struct Option *iscl;	/* input data is scaled to this range */
    struct Option *ialt;	/* an input elevation map in meters used to increase */
    /* atmospheric correction accuracy, including this */
    /* will make computations take much, much longer */
    struct Option *ivis;	/* an input visibility map in km (same purpose and effect as ialt) */
    struct Option *icnd;	/* the input conditions file */
    struct Option *oimg;	/* output image name */
    struct Option *oscl;	/* scale the output data (reflectance values) to this range */

    /* flags */
    struct Flag *oint;		/* output data as integer */
    struct Flag *irad;		/* treat input values as reflectance instead of radiance values */
    struct Flag *etmafter;	/* treat input data as a satellite image of type etm+ taken after July 1, 2000 */
    struct Flag *etmbefore;	/* treat input data as a satellite image of type etm+ taken before July 1, 2000 */
};

struct ScaleRange
{
    int min;
    int max;
};

struct RBitem
{
    int alt;		/* elevation */
    int vis;		/* visibility */
    TransformInput ti;	/* transformation parameters */
};

/* function prototypes */
static void adjust_region(const char *);
static void write_fp_to_cell(int, FCELL *);
static void process_raster(int, InputMask, ScaleRange, int, int, int, bool,
			   ScaleRange);
static void copy_colors(const char *, char *);
static void define_module(void);
static struct Options define_options(void);
static void read_scale(Option *, ScaleRange &);


/* 
   Adjust the region to that of the input raster.
   Atmospheric corrections should be done on the whole
   satellite image, not just portions.
 */
static void adjust_region(const char *name)
{
    struct Cell_head iimg_head;	/* the input image header file */

    Rast_get_cellhd(name, "", &iimg_head);

    Rast_set_window(&iimg_head);
}


/* Rounds a floating point cell value */
static CELL round_c(FCELL x)
{
    if (x >= 0.0)
	return (CELL) (x + .5);

    return (CELL) (-(-x + .5));
}

/* round height, input and output unit is meters */
static int round_h(double x)
{
    x /= BIN_ALT;
    x = floor(x + 0.5);
    x *= BIN_ALT;

    return x;
}

/* round visibility, input unit is km, output unit is meters */
static int round_v(double x)
{
    x *= 1000. / BIN_VIS;
    x = floor(x + 0.5);
    x *= BIN_VIS;

    return x;
}

/* Converts the buffer to cell and write it to disk */
static void write_fp_to_cell(int ofd, FCELL *buf)
{
    CELL *cbuf;
    int col;

    cbuf = (CELL *) Rast_allocate_buf(CELL_TYPE);

    for (col = 0; col < Rast_window_cols(); col++)
	cbuf[col] = round_c(buf[col]);
    Rast_put_row(ofd, cbuf, CELL_TYPE);
}

/* compare function for RB tree */
static int compare_hv(const void *ti_a, const void *ti_b)
{
    struct RBitem *a, *b;

    a = (struct RBitem *)ti_a;
    b = (struct RBitem *)ti_b;

    /* check most common case first
     * also faster if either an altitude or a visibility map is given,
     * but not both */
    if (a->alt == b->alt) {
	if (a->vis == b->vis)
	    return 0;
	if (a->vis > b->vis)
	    return 1;
	else if (a->vis < b->vis)
	    return -1;
    }
    else if (a->alt > b->alt)
	return 1;
    else if (a->alt < b->alt)
	return -1;

    /* should not be reached */
    return 0;
}

/* Cache for transformation input parameters */
class TICache
{
    struct RB_TREE *RBTree;
    unsigned int tree_size;

  private:
    struct RBitem set_alt_vis(double alt, double vis)
    {
	struct RBitem rbitem;

	/* alt and vis must be in meters */
	rbitem.alt = (alt < 0 ? (int) (alt - 0.5) : (int) (alt + 0.5));
	rbitem.vis = (int) (vis + 0.5);
	
	return rbitem;
    }

  public:
      TICache()
    {
	RBTree = rbtree_create(compare_hv, sizeof(struct RBitem));
	tree_size = 0;
    }
    int search(double alt, double vis, TransformInput *ti)
    {
	struct RBitem search_ti = set_alt_vis(alt, vis);
	struct RBitem *found_ti;

	found_ti = (struct RBitem *)rbtree_find(RBTree, &search_ti);
	if (found_ti) {
	    *ti = found_ti->ti;
	    return 1;
	}
	else
	    return 0;

    }

    void add(TransformInput ti, double alt, double vis)
    {
	struct RBitem insert_ti = set_alt_vis(alt, vis);

	/* add safety check here */
	tree_size++;

	insert_ti.ti = ti;

	rbtree_insert(RBTree, &insert_ti);
    }
};


/* Process the raster and do atmospheric corrections.
   Params:
   * INPUT FILE
   ifd: input file descriptor
   iref: input file has radiance values (default is reflectance) ?
   iscale: input file's range (default is min = 0, max = 255)
   ialt_fd: height map file descriptor, negative if global value is used
   ivis_fd: visibility map file descriptor, negative if global value is used

   * OUTPUT FILE
   ofd: output file descriptor
   oflt: if true use FCELL_TYPE for output
   oscale: output file's range (default is min = 0, max = 255)
 */
static void process_raster(int ifd, InputMask imask, ScaleRange iscale,
			   int ialt_fd, int ivis_fd, int ofd, bool oint,
			   ScaleRange oscale)
{
    FCELL *buf;			/* buffer for the input values */
    FCELL *alt = NULL;		/* buffer for the elevation values */
    FCELL *vis = NULL;		/* buffer for the visibility values */
    int cur_alt = -100000;
    int cur_vis = -100000;
    int row, col, nrows, ncols;
    /* switch on optimization automatically if elevation and/or visibility map is given */
    bool optimize = (ialt_fd >= 0 || ivis_fd >= 0);
    
#ifdef _NO_OPTIMIZE_
    optimize = false;
#endif

    /* do initial computation with global elevation and visibility values */
    TransformInput ti;

    ti = compute();

    /* use a cache to increase computation speed when an elevation map 
     * and/or a visibility map is given */
    TICache ticache;

    /* allocate memory for buffers */
    buf = (FCELL *) Rast_allocate_buf(FCELL_TYPE);
    if (ialt_fd >= 0)
	alt = (FCELL *) Rast_allocate_buf(FCELL_TYPE);
    if (ivis_fd >= 0)
	vis = (FCELL *) Rast_allocate_buf(FCELL_TYPE);

    nrows = Rast_window_rows();
    ncols = Rast_window_cols();

    for (row = 0; row < nrows; row++) {
	G_percent(row, nrows, 1);	/* keep the user informed of our progress */

	/* read the next row */
	Rast_get_row(ifd, buf, row, FCELL_TYPE);

	/* read the next row of elevation values */
	if (ialt_fd >= 0)
	    Rast_get_row(ialt_fd, alt, row, FCELL_TYPE);

	/* read the next row of elevation values */
	if (ivis_fd >= 0)
	    Rast_get_row(ivis_fd, vis, row, FCELL_TYPE);

	/* loop over all the values in the row */
	for (col = 0; col < ncols; col++) {
	    if ((vis && Rast_is_f_null_value(&vis[col])) ||
		(alt && Rast_is_f_null_value(&alt[col])) ||
		Rast_is_f_null_value(&buf[col])) {
		Rast_set_f_null_value(&buf[col], 1);
		continue;
	    }
	    if (ialt_fd >= 0) {
		alt[col] = round_h(alt[col]); /* in meters */
	    }
	    if (ivis_fd >= 0) {
		if (vis[col] < 0) {
		     /* negative visibility is invalid */
		    G_warning(_("Negative visibility!"));
		    vis[col] = 0;
		}
		if (vis[col] < 5.0) {
		     /* too low visibility, text comes from 6S main.f L109-113 */
		    G_warning(_("The visibility must be better than 5.0km, "
				"for smaller values calculations might be no more valid!"));
		}
		vis[col] = round_v(vis[col]); /* from km to meters */
	    }

	    /* check if altitude or visibility has changed */
	    if ((ialt_fd >= 0) && (ivis_fd >= 0) &&
		((cur_vis != vis[col]) || (cur_alt != alt[col]))) {
		cur_alt = alt[col];
		cur_vis = vis[col];
		if (optimize) {
		    int in_cache = ticache.search(cur_alt, cur_vis, &ti);

		    if (!in_cache) {
			pre_compute_hv(alt[col] / 1000., vis[col] / 1000.);	/* re-compute transformation inputs */
			ti = compute();	/* ... */

			ticache.add(ti, cur_alt, cur_vis);
		    }
		}
		else {
		    pre_compute_hv(alt[col] / 1000., vis[col] / 1000.);	/* re-compute transformation inputs */
		    ti = compute();	/* ... */
		}
	    }
	    else {
		if ((ivis_fd >= 0) && (cur_vis != vis[col])) {
		    cur_vis = vis[col];

		    if (optimize) {
			int in_cache = ticache.search(cur_alt, cur_vis, &ti);

			if (!in_cache) {
			    pre_compute_v(vis[col] / 1000.);	/* re-compute transformation inputs */
			    ti = compute();	/* ... */

			    ticache.add(ti, cur_alt, cur_vis);
			}
		    }
		    else {
			pre_compute_v(vis[col] / 1000.);	/* re-compute transformation inputs */
			ti = compute();	/* ... */
		    }
		}

		if ((ialt_fd >= 0) && (cur_alt != alt[col])) {
		    cur_alt = alt[col];

		    if (optimize) {
			int in_cache = ticache.search(alt[col], cur_vis, &ti);

			if (!in_cache) {
			    pre_compute_h(alt[col] / 1000.);	/* re-compute transformation inputs */
			    ti = compute();	/* ... */

			    ticache.add(ti, cur_alt, cur_vis);
			}
		    }
		    else {
			pre_compute_h(alt[col] / 1000.);	/* re-compute transformation inputs */
			ti = compute();	/* ... */
		    }
		}
	    }
	    G_debug(3, "Computed r%d (%d), c%d (%d)", row, nrows, col, ncols);
	    /* transform from iscale.[min,max] to [0,1] */
	    buf[col] =
		(buf[col] - iscale.min) / ((double)iscale.max -
					   (double)iscale.min);
	    buf[col] = transform(ti, imask, buf[col]);
	    if (Rast_is_f_null_value(&buf[col]))
		G_fatal_error(_("Numerical instability in 6S"));
	    /* transform from [0,1] to oscale.[min,max] */
	    buf[col] =
		buf[col] * ((double)oscale.max - (double)oscale.min) +
		oscale.min;

	    if (oint && (buf[col] > (double)oscale.max))
		G_warning(_("The output data will overflow. Reflectance > 100%%"));
	}

	/* write output */
	if (oint)
	    write_fp_to_cell(ofd, buf);
	else
	    Rast_put_row(ofd, buf, FCELL_TYPE);
    }
    G_percent(1, 1, 1);

    /* free allocated memory */
    G_free(buf);
    if (ialt_fd >= 0)
	G_free(alt);
    if (ivis_fd >= 0)
	G_free(vis);
}



/* Copy the colors from map named iname to the map named oname */
static void copy_colors(const char *iname, char *oname)
{
    struct Colors colors;

    Rast_read_colors(iname, "", &colors);
    Rast_write_colors(oname, G_mapset(), &colors);
}


/* Define our module so that Grass can print it if the user wants to know more. */
static void define_module(void)
{
    struct GModule *module;

    module = G_define_module();
    module->label =
	_("Performs atmospheric correction using the 6S algorithm.");
    module->description =
	_("6S - Second Simulation of Satellite Signal in the Solar Spectrum.");
    G_add_keyword(_("imagery"));
    G_add_keyword(_("atmospheric correction"));
    G_add_keyword(_("radiometric conversion"));
    G_add_keyword(_("radiance"));
    G_add_keyword(_("reflectance"));
    G_add_keyword(_("satellite"));

    /* 
       " Incorporated into Grass by Christo A. Zietsman, January 2003.\n"
       " Converted from Fortran to C by Christo A. Zietsman, November 2002.\n\n"
       " Adapted by Mauro A. Homem Antunes for atmopheric corrections of\n"
       " remotely sensed images in raw format (.RAW) of 8 bits.\n"
       " April 4, 2001.\n\n"
       " Please refer to the following paper and acknowledge the authors of\n"
       " the model:\n"
       " Vermote, E.F., Tanre, D., Deuze, J.L., Herman, M., and Morcrette,\n"
       "    J.J., (1997), Second simulation of the satellite signal in\n"
       "    the solar spectrum, 6S: An overview., IEEE Trans. Geosc.\n"
       "    and Remote Sens. 35(3):675-686.\n"
       " The code is provided as is and is not to be sold. See notes on\n"
       " http://loasys.univ-lille1.fr/informatique/sixs_gb.html\n"
       " http://www.ltid.inpe.br/dsr/mauro/6s/index.html\n"
       " and on http://www.cs.sun.ac.za/~caz/index.html\n"; */
}


/* Define the options and flags */
static struct Options define_options(void)
{
    struct Options opts;

    opts.iimg = G_define_standard_option(G_OPT_R_INPUT);

    opts.iscl = G_define_option();
    opts.iscl->key = "range";
    opts.iscl->type = TYPE_INTEGER;
    opts.iscl->key_desc = "min,max";
    opts.iscl->required = NO;
    opts.iscl->answer = G_store("0,255");
    opts.iscl->description = _("Input range");
    opts.iscl->guisection = _("Input");

    opts.ialt = G_define_standard_option(G_OPT_R_ELEV);
    opts.ialt->required = NO;
    opts.ialt->description = _("Name of input elevation raster map (in m)");
    opts.ialt->guisection = _("Input");

    opts.ivis = G_define_standard_option(G_OPT_R_INPUT);
    opts.ivis->key = "visibility";
    opts.ivis->required = NO;
    opts.ivis->description = _("Name of input visibility raster map (in km)");
    opts.ivis->guisection = _("Input");

    opts.icnd = G_define_standard_option(G_OPT_F_INPUT);
    opts.icnd->key = "parameters";
    opts.icnd->required = YES;
    opts.icnd->description = _("Name of input text file with 6S parameters");

    opts.oimg = G_define_standard_option(G_OPT_R_OUTPUT);

    opts.oscl = G_define_option();
    opts.oscl->key = "rescale";
    opts.oscl->type = TYPE_INTEGER;
    opts.oscl->key_desc = "min,max";
    opts.oscl->answer = G_store("0,255");
    opts.oscl->required = NO;
    opts.oscl->description = _("Rescale output raster map");
    opts.oscl->guisection = _("Output");

    opts.oint = G_define_flag();
    opts.oint->key = 'i';
    opts.oint->description = _("Output raster map as integer");
    opts.oint->guisection = _("Output");

    opts.irad = G_define_flag();
    opts.irad->key = 'r';
    opts.irad->description =
	_("Input raster map converted to reflectance (default is radiance)");
    opts.irad->guisection = _("Input");

    opts.etmafter = G_define_flag();
    opts.etmafter->key = 'a';
    opts.etmafter->description =
	_("Input from ETM+ image taken after July 1, 2000");
    opts.etmafter->guisection = _("Input");

    opts.etmbefore = G_define_flag();
    opts.etmbefore->key = 'b';
    opts.etmbefore->description =
	_("Input from ETM+ image taken before July 1, 2000");
    opts.etmbefore->guisection = _("Input");

    return opts;
}

/* Read the min and max values from the iscl and oscl options */
void read_scale(Option * scl, ScaleRange & range)
{
    /* set default values */
    range.min = 0;
    range.max = 255;

    if (scl->answer) {
	sscanf(scl->answers[0], "%d", &range.min);
	sscanf(scl->answers[1], "%d", &range.max);

	if (range.min == range.max) {
	    G_warning(_("Scale range length should be > 0; Using default values: [0,255]"));

	    range.min = 0;
	    range.max = 255;
	}
    }

    /* swap values if max is smaller than min */
    if (range.max < range.min) {
	int temp;

	temp = range.max;
	range.max = range.min;
	range.min = temp;
    }
}


int main(int argc, char *argv[])
{
    struct Options opts;
    struct ScaleRange iscale;	/* input file's data is scaled to this interval */
    struct ScaleRange oscale;	/* output file's scale */
    int iimg_fd;		/* input image's file descriptor */
    int oimg_fd;		/* output image's file descriptor */
    int ialt_fd = -1;		/* input elevation map's file descriptor */
    int ivis_fd = -1;		/* input visibility map's file descriptor */
    struct History hist;
    struct Cell_head orig_window;

    /* Define module */
    define_module();

    /* Define the different input options */
    opts = define_options();

    /**** Start ****/
    G_gisinit(argv[0]);
    if (G_parser(argc, argv) < 0)
	exit(EXIT_FAILURE);

    G_get_set_window(&orig_window);
    adjust_region(opts.iimg->answer);

    /* open input raster */
    if ((iimg_fd = Rast_open_old(opts.iimg->answer, "")) < 0)
	G_fatal_error(_("Unable to open raster map <%s>"), opts.iimg->answer);

    if (opts.ialt->answer) {
	if ((ialt_fd = Rast_open_old(opts.ialt->answer, "")) < 0)
	    G_fatal_error(_("Unable to open raster map <%s>"),
			  opts.ialt->answer);
    }

    if (opts.ivis->answer) {
	if ((ivis_fd = Rast_open_old(opts.ivis->answer, "")) < 0)
	    G_fatal_error(_("Unable to open raster map <%s>"),
			  opts.ivis->answer);
    }

    /* open a doubleing point raster or not? */
    if (opts.oint->answer) {
	if ((oimg_fd = Rast_open_new(opts.oimg->answer, CELL_TYPE)) < 0)
	    G_fatal_error(_("Unable to create raster map <%s>"),
			  opts.oimg->answer);
    }
    else {
	if ((oimg_fd = Rast_open_fp_new(opts.oimg->answer)) < 0)
	    G_fatal_error(_("Unable to create raster map <%s>"),
			  opts.oimg->answer);
    }

    /* read the scale parameters */
    read_scale(opts.iscl, iscale);
    read_scale(opts.oscl, oscale);

    /* initialize this 6s computation and parse the input conditions file */
    init_6S(opts.icnd->answer);

    InputMask imask = RADIANCE;	/* the input mask tells us what transformations if any
				   needs to be done to make our input values, reflectance
				   values scaled between 0 and 1 */
    if (opts.irad->answer)
	imask = REFLECTANCE;
    if (opts.etmbefore->answer)
	imask = (InputMask) (imask | ETM_BEFORE);
    if (opts.etmafter->answer)
	imask = (InputMask) (imask | ETM_AFTER);

    /* process the input raster and produce our atmospheric corrected output raster. */
    G_message(_("Atmospheric correction..."));
    process_raster(iimg_fd, imask, iscale, ialt_fd, ivis_fd,
		   oimg_fd, opts.oint->answer, oscale);


    /* Close the input and output file descriptors */
    Rast_short_history(opts.oimg->answer, "raster", &hist);
    Rast_close(iimg_fd);
    if (opts.ialt->answer)
	Rast_close(ialt_fd);
    if (opts.ivis->answer)
	Rast_close(ivis_fd);
    Rast_close(oimg_fd);

    Rast_command_history(&hist);
    Rast_write_history(opts.oimg->answer, &hist);

    /* Copy the colors of the input raster to the output raster.
       Scaling is ignored and color ranges might not be correct. */
    copy_colors(opts.iimg->answer, opts.oimg->answer);

    Rast_set_window(&orig_window);
    G_message(_("Atmospheric correction complete."));

    exit(EXIT_SUCCESS);
}


/****************************************************************************
 *
 * MODULE:       v.surf.rst
 * AUTHOR(S):    H. Mitasova, I. Kosinovsky, D. Gerdes Fall 1993
 *               University of Illinois
 *               I. Kosinovsky, (USA-CERL), and D.Gerdes (USA-CERL)
 *               Michael Shapiro, U.S. Army Construction Engineering Research Laboratory
 *               modified by McCauley in August 1995
 *               modified by Mitasova in August 1995
 *               modified by Mitasova in November 1999 (dmax, timestamp update)
 *               dnorm independent tension - -t flag
 *               cross-validation -v flag by Jaro Hofierka 2004
 *
 * PURPOSE:      Surface interpolation from vector point data by splines
 * COPYRIGHT:    (C) 2003-2009, 2013 by the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2). Read the file COPYING that
 *               comes with GRASS for details.
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/site.h>
#include <grass/glocale.h>
#include <grass/linkm.h>
#include <grass/bitmap.h>
#include <grass/interpf.h>

#include <grass/qtree.h>
#include <grass/dataquad.h>
#include <grass/gmath.h>

#include "surf.h"

#define SCIK1 1			/*100000 */
#define SCIK2 1			/*100000 */
#define SCIK3 1			/*100000 */

static double /* pargr */ ns_res, ew_res;
static double dmin, dmax, ertre;
static int KMAX2, KMIN, KMAX, totsegm, deriv, dtens, cv;
static struct Map_info Map;
static struct Map_info TreeMap, OverMap;

static struct interp_params params;
static struct tree_info *info;

static void create_temp_files(void);
static void clean(void);

static double *az = NULL, *adx = NULL, *ady = NULL, *adxx = NULL, *adyy = NULL,
    *adxy = NULL;
static double /* error */ ertot, ertre, zminac, zmaxac, zmult;
struct multtree *root;

static int NPOINT = 0;

static int cond1, cond2;

static char *treefile = NULL;
static char *overfile = NULL;

static FCELL *zero_array_cell;

static char *input;
static int field;
static char *zcol;
static char *scol;
static char *wheresql;
static char *elev = NULL;
static char *slope = NULL;
static char *aspect = NULL;
static char *pcurv = NULL;
static char *tcurv = NULL;
static char *mcurv = NULL;
static char *maskmap = NULL;
static char *devi = NULL;
static char *cvdev = NULL;
static int sdisk, disk, ddisk, sddisk;
static FILE *Tmp_fd_z = NULL;
static char *Tmp_file_z = NULL;
static FILE *Tmp_fd_dx = NULL;
static char *Tmp_file_dx = NULL;
static FILE *Tmp_fd_dy = NULL;
static char *Tmp_file_dy = NULL;
static FILE *Tmp_fd_xx = NULL;
static char *Tmp_file_xx = NULL;
static FILE *Tmp_fd_yy = NULL;
static char *Tmp_file_yy = NULL;
static FILE *Tmp_fd_xy = NULL;
static char *Tmp_file_xy = NULL;

static double gmin, gmax, c1min, c1max, c2min, c2max, fi, rsm;
static double xmin, xmax, ymin, ymax, zmin, zmax;
static double theta, scalex;

static struct BM *bitmask;
static struct Cell_head cellhd;

static int n_rows, n_cols;

int main(int argc, char *argv[])
{
    int npmin;
    int ii;
    double x_orig, y_orig, dnorm, deltx, delty, xm, ym;
    char dmaxchar[200];
    char dminchar[200];
    Site_head inhead;
    struct quaddata *data;
    struct multfunc *functions;
    struct multtree *tree;
    int open_check, with_z;
    char buf[1024];

    struct GModule *module;
    struct
    {
	struct Option *input, *field, *zcol, *wheresql, *scol, *elev, *slope,
	    *aspect, *pcurv, *tcurv, *mcurv, *treefile, *overfile, *maskmap,
	    *dmin, *dmax, *zmult, *fi, *rsm, *segmax, *npmin, *cvdev, *devi,
	    *theta, *scalex;
    } parm;
    struct
    {
	struct Flag *deriv, *cprght, *cv;
    } flag;


    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("surface"));
    G_add_keyword(_("interpolation"));
    G_add_keyword(_("3D"));
    module->label = _("Performs surface interpolation from vector points map by splines.");
    module->description =
	_("Spatial approximation and topographic analysis from given "
	  "point or isoline data in vector format to floating point "
	  "raster format using regularized spline with tension.");

    flag.cv = G_define_flag();
    flag.cv->key = 'c';
    flag.cv->description =
	_("Perform cross-validation procedure without raster approximation");
    flag.cv->guisection = _("Parameters");

    flag.cprght = G_define_flag();
    flag.cprght->key = 't';
    flag.cprght->description = _("Use scale dependent tension");
    flag.cprght->guisection = _("Parameters");

    flag.deriv = G_define_flag();
    flag.deriv->key = 'd';
    flag.deriv->description =
	_("Output partial derivatives instead of topographic parameters");
    flag.deriv->guisection = _("Outputs");

    parm.input = G_define_standard_option(G_OPT_V_INPUT);
    
    parm.field = G_define_standard_option(G_OPT_V_FIELD);
    parm.field->answer = "1";
    parm.field->guisection = _("Selection");

    parm.zcol = G_define_standard_option(G_OPT_DB_COLUMN);
    parm.zcol->key = "zcolumn";
    parm.zcol->required = NO;
    parm.zcol->label =
	_("Name of the attribute column with values to be used for approximation");
    parm.zcol->description = _("If not given and input is 2D vector map then category values are used. "
                               "If input is 3D vector map then z-coordinates are used.");
    parm.zcol->guisection = _("Parameters");

    parm.wheresql = G_define_standard_option(G_OPT_DB_WHERE);
    parm.wheresql->guisection = _("Selection");

    parm.elev = G_define_standard_option(G_OPT_R_OUTPUT);
    parm.elev->key = "elevation";
    parm.elev->required = NO;
    parm.elev->description = _("Name for output surface elevation raster map");
    parm.elev->guisection = _("Outputs");

    parm.slope = G_define_standard_option(G_OPT_R_OUTPUT);
    parm.slope->key = "slope";
    parm.slope->required = NO;
    parm.slope->description = _("Name for output slope raster map");
    parm.slope->guisection = _("Outputs");

    parm.aspect = G_define_standard_option(G_OPT_R_OUTPUT);
    parm.aspect->key = "aspect";
    parm.aspect->required = NO;
    parm.aspect->description = _("Name for output aspect raster map");
    parm.aspect->guisection = _("Outputs");

    parm.pcurv = G_define_standard_option(G_OPT_R_OUTPUT);
    parm.pcurv->key = "pcurv";
    parm.pcurv->required = NO;
    parm.pcurv->description = _("Name for output profile curvature raster map");
    parm.pcurv->guisection = _("Outputs");

    parm.tcurv = G_define_standard_option(G_OPT_R_OUTPUT);
    parm.tcurv->key = "tcurv";
    parm.tcurv->required = NO;
    parm.tcurv->description = _("Name for output tangential curvature raster map");
    parm.tcurv->guisection = _("Outputs");

    parm.mcurv = G_define_standard_option(G_OPT_R_OUTPUT);
    parm.mcurv->key = "mcurv";
    parm.mcurv->required = NO;
    parm.mcurv->description = _("Name for output mean curvature raster map");
    parm.mcurv->guisection = _("Outputs");

    parm.devi = G_define_standard_option(G_OPT_V_OUTPUT);
    parm.devi->key = "devi";
    parm.devi->required = NO;
    parm.devi->description = _("Name for output deviations vector point map");
    parm.devi->guisection = _("Outputs");

    parm.cvdev = G_define_standard_option(G_OPT_V_OUTPUT);
    parm.cvdev->key = "cvdev";
    parm.cvdev->required = NO;
    parm.cvdev->description =
	_("Name for output cross-validation errors vector point map");
    parm.cvdev->guisection = _("Outputs");

    parm.treefile = G_define_standard_option(G_OPT_V_OUTPUT);
    parm.treefile->key = "treeseg";
    parm.treefile->required = NO;
    parm.treefile->description =
	_("Name for output vector map showing quadtree segmentation");
    parm.treefile->guisection = _("Outputs");

    parm.overfile = G_define_standard_option(G_OPT_V_OUTPUT);
    parm.overfile->key = "overwin";
    parm.overfile->required = NO;
    parm.overfile->description =
	_("Name for output vector map showing overlapping windows");
    parm.overfile->guisection = _("Outputs");

    parm.maskmap = G_define_standard_option(G_OPT_R_INPUT);
    parm.maskmap->key = "mask";
    parm.maskmap->required = NO;
    parm.maskmap->description = _("Name of raster map used as mask");
    parm.maskmap->guisection = _("Parameters");

    parm.fi = G_define_option();
    parm.fi->key = "tension";
    parm.fi->type = TYPE_DOUBLE;
    parm.fi->answer = TENSION;
    parm.fi->required = NO;
    parm.fi->description = _("Tension parameter");
    parm.fi->guisection = _("Parameters");

    parm.rsm = G_define_option();
    parm.rsm->key = "smooth";
    parm.rsm->type = TYPE_DOUBLE;
    parm.rsm->required = NO;
    parm.rsm->description = _("Smoothing parameter");
    parm.rsm->guisection = _("Parameters");

    parm.scol = G_define_option();
    parm.scol->key = "scolumn";
    parm.scol->type = TYPE_STRING;
    parm.scol->required = NO;
    parm.scol->description =
	_("Name of the attribute column with smoothing parameters");
    parm.scol->guisection = _("Parameters");

    parm.segmax = G_define_option();
    parm.segmax->key = "segmax";
    parm.segmax->type = TYPE_INTEGER;
    parm.segmax->answer = MAXSEGM;
    parm.segmax->required = NO;
    parm.segmax->description = _("Maximum number of points in a segment");
    parm.segmax->guisection = _("Parameters");

    parm.npmin = G_define_option();
    parm.npmin->key = "npmin";
    parm.npmin->type = TYPE_INTEGER;
    parm.npmin->answer = MINPOINTS;
    parm.npmin->required = NO;
    parm.npmin->description =
	_("Minimum number of points for approximation in a segment (>segmax)");
    parm.npmin->guisection = _("Parameters");

    parm.dmin = G_define_option();
    parm.dmin->key = "dmin";
    parm.dmin->type = TYPE_DOUBLE;
    parm.dmin->required = NO;
    parm.dmin->description =
	_("Minimum distance between points (to remove almost identical points)");
    parm.dmin->guisection = _("Parameters");

    parm.dmax = G_define_option();
    parm.dmax->key = "dmax";
    parm.dmax->type = TYPE_DOUBLE;
    parm.dmax->required = NO;
    parm.dmax->description =
	_("Maximum distance between points on isoline (to insert additional points)");
    parm.dmax->guisection = _("Parameters");

    parm.zmult = G_define_option();
    parm.zmult->key = "zmult";
    parm.zmult->type = TYPE_DOUBLE;
    parm.zmult->answer = ZMULT;
    parm.zmult->required = NO;
    parm.zmult->description =
	_("Conversion factor for values used for approximation");
    parm.zmult->guisection = _("Parameters");

    parm.theta = G_define_option();
    parm.theta->key = "theta";
    parm.theta->type = TYPE_DOUBLE;
    parm.theta->required = NO;
    parm.theta->description =
	_("Anisotropy angle (in degrees counterclockwise from East)");
    parm.theta->guisection = _("Parameters");

    parm.scalex = G_define_option();
    parm.scalex->key = "scalex";
    parm.scalex->type = TYPE_DOUBLE;
    parm.scalex->required = NO;
    parm.scalex->description = _("Anisotropy scaling factor");
    parm.scalex->guisection = _("Parameters");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    G_get_set_window(&cellhd);

    ew_res = cellhd.ew_res;
    ns_res = cellhd.ns_res;
    n_cols = cellhd.cols;
    n_rows = cellhd.rows;
    x_orig = cellhd.west;
    y_orig = cellhd.south;
    xm = cellhd.east;
    ym = cellhd.north;
    if (ew_res < ns_res)
	dmin = ew_res / 2;
    else
	dmin = ns_res / 2;
    disk = n_rows * n_cols * sizeof(int);
    sdisk = n_rows * n_cols * sizeof(short int);
    sprintf(dmaxchar, "%f", dmin * 5);
    sprintf(dminchar, "%f", dmin);

    if (!parm.dmin->answer) {
	parm.dmin->answer = G_store(dminchar);
	parm.dmin->answers = (char **) G_malloc(2 * sizeof(char *));
	parm.dmin->answers[0] = G_store(dminchar);
	parm.dmin->answers[1] = NULL;
    }
    if (!parm.dmax->answer) {
	parm.dmax->answer = G_store(dmaxchar);
	parm.dmax->answers = (char **) G_malloc(2 * sizeof(char *));
	parm.dmax->answers[0] = G_store(dmaxchar);
	parm.dmax->answers[1] = NULL;
    }
    
    input = parm.input->answer;
    zcol = parm.zcol->answer;
    scol = parm.scol->answer;
    wheresql = parm.wheresql->answer;
    maskmap = parm.maskmap->answer;
    elev = parm.elev->answer;
    devi = parm.devi->answer;
    cvdev = parm.cvdev->answer;
    slope = parm.slope->answer;
    aspect = parm.aspect->answer;
    pcurv = parm.pcurv->answer;
    tcurv = parm.tcurv->answer;
    mcurv = parm.mcurv->answer;
    treefile = parm.treefile->answer;
    overfile = parm.overfile->answer;

    if (devi) {
	if (Vect_legal_filename(devi) == -1)
	    G_fatal_error(_("Output vector map name <%s> is not valid map name"),
			  devi);
    }
    if (cvdev) {
	if (Vect_legal_filename(cvdev) == -1)
	    G_fatal_error(_("Output vector map name <%s> is not valid map name"),
			  cvdev);
    }
    if (treefile) {
	if (Vect_legal_filename(treefile) == -1)
	    G_fatal_error(_("Output vector map name <%s> is not valid map name"),
			  treefile);
    }
    if (overfile) {
	if (Vect_legal_filename(overfile) == -1)
	    G_fatal_error(_("Output vector map name <%s> is not valid map name"),
			  overfile);
    }
    /*    if (treefile)
       Vect_check_input_output_name(input, treefile, G_FATAL_EXIT);

       if (overfile)
       Vect_check_input_output_name(input, overfile, G_FATAL_EXIT);
     */
    if ((elev == NULL) && (pcurv == NULL) && (tcurv == NULL)
	&& (mcurv == NULL)
	&& (slope == NULL) && (aspect == NULL) && (devi == NULL)
	&& (cvdev == NULL))
	G_warning(_("You are not outputting any raster or vector maps"));
    
    cond2 = ((pcurv != NULL) || (tcurv != NULL) || (mcurv != NULL));
    cond1 = ((slope != NULL) || (aspect != NULL) || cond2);
    deriv = flag.deriv->answer;
    dtens = flag.cprght->answer;
    cv = flag.cv->answer;

    if ((cv && cvdev == NULL) || (!(cv) && cvdev != NULL))
	G_fatal_error(_("Both cross-validation options (-c flag and cvdev vector output) must be specified"));

    if ((elev != NULL || cond1 || cond2 || devi != NULL) && cv)
	G_fatal_error(_("The cross-validation cannot be computed simultaneously with output raster or devi file"));

    ertre = 0.1;
    sscanf(parm.dmax->answer, "%lf", &dmax);
    sscanf(parm.dmin->answer, "%lf", &dmin);
    sscanf(parm.fi->answer, "%lf", &fi);
    sscanf(parm.segmax->answer, "%d", &KMAX);
    sscanf(parm.npmin->answer, "%d", &npmin);
    sscanf(parm.zmult->answer, "%lf", &zmult);

    /* if (fi=0.000000)  G_fatal_error("Tension must be > 0.000000") */

    if (parm.theta->answer)
	sscanf(parm.theta->answer, "%lf", &theta);

    if (parm.scalex->answer) {
	sscanf(parm.scalex->answer, "%lf", &scalex);
	if (!parm.theta->answer)
	    G_fatal_error(_("Using anisotropy - both theta and scalex have to be specified"));
    }

    if (parm.rsm->answer) {
	sscanf(parm.rsm->answer, "%lf", &rsm);
	if (rsm < 0.0)
	    G_fatal_error("Smoothing must be a positive value");
	if (scol != NULL)
	    G_warning(_("Both smatt and smooth options specified - using constant"));
    }
    else {
	sscanf(SMOOTH, "%lf", &rsm);
	if (scol != NULL)
	    rsm = -1;		/* used in InterpLib to indicate variable smoothing */
    }


    if (npmin > MAXPOINTS - 50) {
	G_warning(_("The computation will last too long - lower npmin is suggested"));
	KMAX2 = 2 * npmin;	/* was: KMAX2 = npmin + 50; */
    }
    else
	KMAX2 = 2 * npmin;	/* was: KMAX2 = MAXPOINTS; fixed by JH in 12/01 */

    /* handling of KMAX2 in GRASS4 v.surf.rst
       if (npmin > MAXPOINTS - 50)
       KMAX2 = npmin + 50;
       else
       KMAX2 = MAXPOINTS;
     */

    dmin = dmin * dmin;
    KMIN = npmin;

    az = G_alloc_vector(n_cols + 1);
    if (!az) {
	G_fatal_error(_("Not enough memory for az"));
    }
    if (cond1) {
	adx = G_alloc_vector(n_cols + 1);
	if (!adx) {
	    G_fatal_error(_("Not enough memory for adx"));
	}
	ady = G_alloc_vector(n_cols + 1);
	if (!ady) {
	    G_fatal_error(_("Not enough memory for ady"));
	}
	if (cond2) {
	    adxx = G_alloc_vector(n_cols + 1);
	    if (!adxx) {
		G_fatal_error(_("Not enough memory for adxx"));
	    }
	    adyy = G_alloc_vector(n_cols + 1);
	    if (!adyy) {
		G_fatal_error(_("Not enough memory for adyy"));
	    }
	    adxy = G_alloc_vector(n_cols + 1);
	    if (!adxy) {
		G_fatal_error(_("Not enough memory for adxy"));
	    }
	}
    }
    if ((data =
	 quad_data_new(x_orig, y_orig, xm, ym, n_rows, n_cols, 0,
		       KMAX)) == NULL)
	G_fatal_error(_("Cannot create quaddata"));
    if ((functions =
	 MT_functions_new(quad_compare, quad_divide_data, quad_add_data,
			  quad_intersect, quad_division_check,
			  quad_get_points)) == NULL)

	G_fatal_error(_("Cannot create quadfunc"));

    if ((tree = MT_tree_new(data, NULL, NULL, 0)) == NULL)
	G_fatal_error(_("Cannot create tree"));
    root = tree;

    if ((info = MT_tree_info_new(root, functions, dmin, KMAX)) == NULL)
	G_fatal_error(_("Cannot create tree info"));

    open_check = Vect_open_old2(&Map, input, "", parm.field->answer);
    if (open_check < 1)
	G_fatal_error(_("Unable to open vector map <%s>"), input);
    /*    if (open_check < 2)
          G_fatal_error(_("You first need to run v.build on vector map <%s>"), input);
    */

    /* get value used for approximation */
    with_z = !parm.zcol->answer && Vect_is_3d(&Map);
    field = Vect_get_field_number(&Map, parm.field->answer);
    if (!with_z && field < 1)
	G_fatal_error(_("Layer <%s> not found"), parm.field->answer);

    if (Vect_is_3d(&Map)) {
        if (!with_z)
            G_verbose_message(_("Input is 3D: using attribute values instead of z-coordinates for approximation"));
        else
            G_verbose_message(_("Input is 3D: using z-coordinates for approximation"));
    }
    else { /* 2D */
        if (parm.zcol->answer)
            G_verbose_message(_("Input is 2D: using attribute values for approximation"));
        else
            G_verbose_message(_("Input is 2D: using category values for approximation"));
    }
        
    /* we can't read the input file's timestamp as they don't exist in   */
    /*   the new vector format. Even so, a TimeStamp structure is needed */
    /*   for IL_init_params_2d(), so we set it to NULL.                  */
    /* If anyone is ever motivated to add it, the Plus_head struct has   */
    /*  'long coor_mtime' and dig_head has 'char *date; char *source_date;' */
    /*   which could be read in.                                         */
    inhead.time = (struct TimeStamp *)NULL;
    inhead.stime = NULL;

    if (devi != NULL || cvdev != NULL) {

	Pnts = Vect_new_line_struct();
	Cats2 = Vect_new_cats_struct();
	db_init_string(&sql2);

	if (devi != NULL)
	    Vect_open_new(&Map2, devi, 1);
	else
	    Vect_open_new(&Map2, cvdev, 1);
	Vect_hist_command(&Map2);
	ff = Vect_default_field_info(&Map2, 1, NULL, GV_1TABLE);
	Vect_map_add_dblink(&Map2, 1, NULL, ff->table, GV_KEY_COLUMN, ff->database,
			    ff->driver);

	/* Create new table */
	db_zero_string(&sql2);
	sprintf(buf, "create table %s ( ", ff->table);
	db_append_string(&sql2, buf);
	db_append_string(&sql2, "cat integer");
	db_append_string(&sql2, ", flt1 double precision");
	db_append_string(&sql2, ")");
	G_debug(1, db_get_string(&sql2));
	driver2 = db_start_driver_open_database(ff->driver, ff->database);
	if (driver2 == NULL)
	    G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
			  ff->database, ff->driver);

	if (db_execute_immediate(driver2, &sql2) != DB_OK) {
	    db_close_database(driver2);
	    db_shutdown_driver(driver2);
	    G_fatal_error(_("Unable to create table: '%s'"),
			  db_get_string(&sql2));
	}
	count = 1;

    }

    ertot = 0.;
    
    create_temp_files();

    IL_init_params_2d(&params, NULL, 1, 1, zmult, KMIN, KMAX, maskmap, n_rows,
		      n_cols, az, adx, ady, adxx, adyy, adxy, fi, KMAX2,
		      SCIK1, SCIK2, SCIK3, rsm, elev, slope, aspect, pcurv,
		      tcurv, mcurv, dmin, x_orig, y_orig, deriv, theta,
		      scalex, Tmp_fd_z, Tmp_fd_dx, Tmp_fd_dy, Tmp_fd_xx,
		      Tmp_fd_yy, Tmp_fd_xy, devi, inhead.time, cv,
		      parm.wheresql->answer);

    IL_init_func_2d(&params, IL_grid_calc_2d, IL_matrix_create,
		    IL_check_at_points_2d, IL_secpar_loop_2d, IL_crst,
		    IL_crstg, IL_write_temp_2d);

    totsegm =
	IL_vector_input_data_2d(&params, &Map, with_z ? 0 : field,
				zcol, scol,
				info, &xmin, &xmax,
				&ymin, &ymax, &zmin, &zmax, &NPOINT, &dmax);
    if (totsegm <= 0) {
	clean();
	G_fatal_error(_("Input failed"));
    }

    /*Vect_set_release_support(&Map); */
    Vect_close(&Map);

    if (treefile != NULL) {
	if (0 > Vect_open_new(&TreeMap, treefile, 0)) {
	    clean();
	    G_fatal_error(_("Unable to open vector map <%s>"), treefile);
	}
	Vect_hist_command(&TreeMap);

	/*
	   sprintf (TreeMap.head.your_name, "grass");
	   sprintf (TreeMap.head.map_name, "Quad tree for %s", input);
	   TreeMap.head.orig_scale = 100000;
	   TreeMap.head.plani_zone = G_zone ();
	 */
	print_tree(root, x_orig, y_orig, &TreeMap);
	Vect_build(&TreeMap);
	Vect_close(&TreeMap);
    }

    disk = disk + totsegm * sizeof(int) * 4;
    sdisk = sdisk + totsegm * sizeof(int) * 4;
    if (elev != NULL)
	ddisk += disk;
    if (slope != NULL)
	sddisk += sdisk;
    if (aspect != NULL)
	sddisk += sdisk;
    if (pcurv != NULL)
	ddisk += disk;
    if (tcurv != NULL)
	ddisk += disk;
    if (mcurv != NULL)
	ddisk += disk;
    ddisk += sddisk;
    G_verbose_message(_("Processing all selected output files "
			"will require %d bytes of disk space for temp files"), ddisk);

    deltx = xmax - xmin;
    delty = ymax - ymin;
    dnorm = sqrt((deltx * delty * KMIN) / NPOINT);

    if (dtens) {
	params.fi = params.fi * dnorm / 1000.;
	G_debug(1, "dnorm = %f, rescaled tension = %f", dnorm, params.fi);
    }
    
    bitmask = IL_create_bitmask(&params);
    
    if (totsegm <= 0) {
	clean();
	G_fatal_error(_("Input failed"));
    }

    ertot = 0.;
    G_message(_("Processing segments..."));    
    if (IL_interp_segments_2d(&params, info, info->root, bitmask,
			      zmin, zmax, &zminac, &zmaxac, &gmin, &gmax,
			      &c1min, &c1max, &c2min, &c2max, &ertot, totsegm,
			      n_cols, dnorm) < 0) {
	clean();
	G_fatal_error(_("Interp_segmets failed"));
    }

    G_free_vector(az);
    if (cond1) {
	G_free_vector(adx);
	G_free_vector(ady);
	if (cond2) {
	    G_free_vector(adxx);
	    G_free_vector(adyy);
	    G_free_vector(adxy);
	}
    }
    ii = IL_output_2d(&params, &cellhd, zmin, zmax, zminac, zmaxac, c1min,
		      c1max, c2min, c2max, gmin, gmax, ertot, input, dnorm,
		      dtens, 1, NPOINT);
    if (ii < 0) {
	clean();
	G_fatal_error(_("Unable to write raster maps - try to increase resolution"));
    }

    G_free(zero_array_cell);
    if (elev != NULL)
	fclose(Tmp_fd_z);
    if (slope != NULL)
	fclose(Tmp_fd_dx);
    if (aspect != NULL)
	fclose(Tmp_fd_dy);
    if (pcurv != NULL)
	fclose(Tmp_fd_xx);
    if (tcurv != NULL)
	fclose(Tmp_fd_yy);
    if (mcurv != NULL)
	fclose(Tmp_fd_xy);

    if (overfile != NULL) {
	if (0 > Vect_open_new(&OverMap, overfile, 0)) {
	    clean();
	    G_fatal_error(_("Unable to create vector map <%s>"), overfile);
	}
	Vect_hist_command(&OverMap);

	/*
	   sprintf (OverMap.head.your_name, "grass");
	   sprintf (OverMap.head.map_name, "Overlap segments for %s", input);
	   OverMap.head.orig_scale = 100000;
	   OverMap.head.plani_zone = G_zone ();
	 */
	print_tree(root, x_orig, y_orig, &OverMap);
	Vect_build(&OverMap);
	Vect_close(&OverMap);
    }

    if (elev != NULL)
	unlink(Tmp_file_z);
    if (slope != NULL)
	unlink(Tmp_file_dx);
    if (aspect != NULL)
	unlink(Tmp_file_dy);
    if (pcurv != NULL)
	unlink(Tmp_file_xx);
    if (tcurv != NULL)
	unlink(Tmp_file_yy);
    if (mcurv != NULL)
	unlink(Tmp_file_xy);

    if (cvdev != NULL || devi != NULL) {
	/*  db_close_database_shutdown_driver ( driver2 ); */
	db_close_database(driver2);
	Vect_build(&Map2);
	Vect_close(&Map2);
    }

    G_done_msg(" ");
    exit(EXIT_SUCCESS);
}



int print_tree(struct multtree *tree,
	       double x_orig, double y_orig, struct Map_info *Map)
{
    double xarray[5], yarray[5], zarray[5];
    struct line_pnts *Points;
    struct line_cats *Cats;
    int j;
    int type = GV_LINE;

    if (tree == NULL)
	return 0;
    if (tree->data == NULL)
	return 0;
    if (tree->leafs != NULL) {
	for (j = 0; j < 4; j++) {
	    print_tree(tree->leafs[j], x_orig, y_orig, Map);
	}
    }
    else {
	Points = Vect_new_line_struct();
	Cats = Vect_new_cats_struct();
	xarray[0] = ((struct quaddata *)(tree->data))->x_orig + x_orig;
	yarray[0] = ((struct quaddata *)(tree->data))->y_orig + y_orig;
	xarray[1] = xarray[0];
	yarray[3] = yarray[0];
	xarray[3] = ((struct quaddata *)(tree->data))->xmax + x_orig;
	yarray[1] = ((struct quaddata *)(tree->data))->ymax + y_orig;
	yarray[2] = yarray[1];
	xarray[2] = xarray[3];
	yarray[4] = yarray[0];
	xarray[4] = xarray[0];
	if (Vect_copy_xyz_to_pnts(Points, xarray, yarray, zarray, 5) < 0) {
	    clean();
	    G_fatal_error(_("Out of memory"));
	}
	Vect_write_line(Map, (unsigned int)type, Points, Cats);

	G_free(Points);
    }
    return 1;
}

static FILE *create_temp_file(const char *name, char **tmpname)
{
    FILE *fp;
    char *tmp;
    int i;

    if (!name)
	return NULL;

    *tmpname = tmp = G_tempfile();
    fp = fopen(tmp, "w+");
    if (!fp)
	G_fatal_error(_("Unable to open temporary file <%s>"), tmpname);

    for (i = 0; i < n_rows; i++) {
	if (fwrite(zero_array_cell, sizeof(FCELL), n_cols, fp) != n_cols) {
	    clean();
	    G_fatal_error(_("Error writing temporary file <%s>"), tmpname);
	}
    }

    return fp;
}

static void create_temp_files(void)
{
    zero_array_cell = (FCELL *) G_calloc(n_cols, sizeof(FCELL));

    Tmp_fd_z  = create_temp_file(elev,   &Tmp_file_z );
    Tmp_fd_dx = create_temp_file(slope,  &Tmp_file_dx);
    Tmp_fd_dy = create_temp_file(aspect, &Tmp_file_dy);
    Tmp_fd_xx = create_temp_file(pcurv,  &Tmp_file_xx);
    Tmp_fd_yy = create_temp_file(tcurv,  &Tmp_file_yy);
    Tmp_fd_xy = create_temp_file(mcurv,  &Tmp_file_xy);
}

static void clean(void)
{
    if (Tmp_fd_z)	fclose(Tmp_fd_z);
    if (Tmp_fd_dx)	fclose(Tmp_fd_dx);
    if (Tmp_fd_dy)	fclose(Tmp_fd_dy);
    if (Tmp_fd_xx)	fclose(Tmp_fd_xx);
    if (Tmp_fd_yy)	fclose(Tmp_fd_yy);
    if (Tmp_fd_xy)	fclose(Tmp_fd_xy);

    if (Tmp_file_z)	unlink(Tmp_file_z);
    if (Tmp_file_dx)	unlink(Tmp_file_dx);
    if (Tmp_file_dy)	unlink(Tmp_file_dy);
    if (Tmp_file_xx)	unlink(Tmp_file_xx);
    if (Tmp_file_yy)	unlink(Tmp_file_yy);
    if (Tmp_file_xy)	unlink(Tmp_file_xy);
}

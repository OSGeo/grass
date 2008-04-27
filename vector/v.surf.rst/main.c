/*-
 * Written by H. Mitasova, I. Kosinovsky, D. Gerdes Fall 1993
 * University of Illinois
 * US Army Construction Engineering Research Lab
 * Copyright 1993, H. Mitasova (University of Illinois),
 * I. Kosinovsky, (USA-CERL), and D.Gerdes (USA-CERL)
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   as published by the Free Software Foundation; either version 2
 *   of the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 *
 * modified by McCauley in August 1995
 * modified by Mitasova in August 1995
 * modified by Mitasova in November 1999 (dmax, timestamp update)
 * dnorm independent tension - -t flag
 * cross-validation -v flag by Jaro Hofierka 2004
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#include <grass/gis.h>
#include <grass/Vect.h>
#include <grass/dbmi.h>
#include <grass/site.h>
#include <grass/glocale.h>
#include <grass/linkm.h>
#include <grass/bitmap.h>
#include <grass/interpf.h>

#include <grass/qtree.h>
#include "surf.h"
#include <grass/dataquad.h>


#define SCIK1 1			/*100000 */
#define SCIK2 1			/*100000 */
#define SCIK3 1			/*100000 */

double /* pargr */ ns_res, ew_res;
double dmin, dmax, ertre;
int KMAX2, KMIN, KMAX, totsegm, deriv, dtens, cv;
struct Map_info Map;
struct Map_info TreeMap, OverMap;
struct Categories cats;

struct interp_params params;
struct tree_info *info;

void clean_fatal_error();

double *az = NULL, *adx = NULL, *ady = NULL, *adxx = NULL, *adyy = NULL,
    *adxy = NULL;
double /* error */ ertot, ertre, zminac, zmaxac, zmult;
struct multtree *root;

int total = 0;
int NPOINT = 0;
int OUTRANGE = 0;
int NPT = 0;

double DETERM;
int NERROR, cond1, cond2;
char fncdsm[32];
char filnam[10];

char *treefile = NULL;
char *overfile = NULL;

FILE *fdinp, *fdredinp, *fdzout, *fddxout, *fddyout, *fdxxout, *fdyyout,
    *fd4, *fxyout, *fddevi = NULL, *fdcvdev = NULL;

FCELL *zero_array_cell;

char *input;
int field;
char *zcol;
char *scol;
char *wheresql;
char *mapset = NULL;
char *mapset1 = NULL;
char *elev = NULL;
char *slope = NULL;
char *aspect = NULL;
char *pcurv = NULL;
char *tcurv = NULL;
char *mcurv = NULL;
char *maskmap = NULL;
char *redinp = NULL;
char *devi = NULL;
char *cvdev = NULL;
int sdisk, disk, ddisk, sddisk;
FILE *Tmp_fd_z = NULL;
char *Tmp_file_z = NULL;
FILE *Tmp_fd_dx = NULL;
char *Tmp_file_dx = NULL;
FILE *Tmp_fd_dy = NULL;
char *Tmp_file_dy = NULL;
FILE *Tmp_fd_xx = NULL;
char *Tmp_file_xx = NULL;
FILE *Tmp_fd_yy = NULL;
char *Tmp_file_yy = NULL;
FILE *Tmp_fd_xy = NULL;
char *Tmp_file_xy = NULL;

double gmin, gmax, c1min, c1max, c2min, c2max, fi, rsm;
double xmin, xmax, ymin, ymax, zmin, zmax;
double theta, scalex;

struct BM *bitmask;
struct Cell_head cellhd;

char msg[1024];


int main(int argc, char *argv[])
{
    int per, npmin;
    int ii, i, n_rows, n_cols;
    double x_orig, y_orig, dnorm, deltx, delty, xm, ym;
    char dmaxchar[200];
    char dminchar[200];
    Site_head inhead;
    struct quaddata *data;
    struct multfunc *functions;
    struct multtree *tree;
    int open_check;
    char   buf[1024];

    struct GModule *module;
    struct
    {
	struct Option *input, *field, *zcol, *wheresql, *scol, *elev, *slope, *aspect,
	    *pcurv, *tcurv, *mcurv, *treefile, *overfile, *maskmap, *dmin,
	    *dmax, *zmult, *fi, *rsm, *segmax, *npmin, *cvdev, *devi, 
	    *theta, *scalex;
    } parm;
    struct
    {
	struct Flag *deriv, *cprght, *cv;
    } flag;


    G_gisinit(argv[0]);

    module = G_define_module();
    module->keywords = _("vector");
    module->description =
	_("Spatial approximation and topographic analysis from given "
	"point or isoline data in vector format to floating point "
	"raster format using regularized spline with tension.");

    if (G_get_set_window(&cellhd) == -1)
	G_fatal_error("G_get_set_window() failed");
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


    flag.cv = G_define_flag ();
    flag.cv->key = 'c';
    flag.cv->description =
	_("Perform cross-validation procedure without raster approximation");

    flag.cprght = G_define_flag();
    flag.cprght->key = 't';
    flag.cprght->description = _("Use scale dependent tension");
    flag.cprght->guisection  = _("Settings");

    flag.deriv = G_define_flag();
    flag.deriv->key = 'd';
    flag.deriv->description =
	_("Output partial derivatives instead of topographic parameters");
    flag.deriv->guisection  = _("Output_options");

    parm.input = G_define_standard_option(G_OPT_V_INPUT);

    parm.field = G_define_standard_option(G_OPT_V_FIELD);
    parm.field->description =
	_("Field value. If set to 0, z coordinates are used. (3D vector only)");
    parm.field->answer = "1";

    parm.zcol = G_define_option();
    parm.zcol->key = "zcolumn";
    parm.zcol->type = TYPE_STRING;
    parm.zcol->required = NO;
    parm.zcol->description =
	_("Name of the attribute column with values to be used for approximation (if layer>0)");

    parm.wheresql = G_define_standard_option(G_OPT_WHERE);

    parm.elev = G_define_standard_option(G_OPT_R_OUTPUT);
    parm.elev->key = "elev";
    parm.elev->required = NO;
    parm.elev->description = _("Output surface raster map (elevation)");

    parm.slope = G_define_standard_option(G_OPT_R_OUTPUT);
    parm.slope->key = "slope";
    parm.slope->required = NO;
    parm.slope->description = _("Output slope raster map");
    parm.slope->guisection  = _("Output_options");

    parm.aspect = G_define_standard_option(G_OPT_R_OUTPUT);
    parm.aspect->key = "aspect";
    parm.aspect->required = NO;
    parm.aspect->description = _("Output aspect raster map");
    parm.aspect->guisection  = _("Output_options");

    parm.pcurv = G_define_standard_option(G_OPT_R_OUTPUT);
    parm.pcurv->key = "pcurv";
    parm.pcurv->required = NO;
    parm.pcurv->description = _("Output profile curvature raster map");
    parm.pcurv->guisection  = _("Output_options");

    parm.tcurv = G_define_standard_option(G_OPT_R_OUTPUT);
    parm.tcurv->key = "tcurv";
    parm.tcurv->required = NO;
    parm.tcurv->description = _("Output tangential curvature raster map");
    parm.tcurv->guisection  = _("Output_options");

    parm.mcurv = G_define_standard_option(G_OPT_R_OUTPUT);
    parm.mcurv->key = "mcurv";
    parm.mcurv->required = NO;
    parm.mcurv->description = _("Output mean curvature raster map");
    parm.mcurv->guisection  = _("Output_options");

    parm.maskmap = G_define_standard_option(G_OPT_R_INPUT);
    parm.maskmap->key = "maskmap";
    parm.maskmap->required = NO;
    parm.maskmap->description = _("Name of the raster map used as mask");

    parm.fi = G_define_option();
    parm.fi->key = "tension";
    parm.fi->type = TYPE_DOUBLE;
    parm.fi->answer = TENSION;
    parm.fi->required = NO;
    parm.fi->description = _("Tension parameter");
    parm.fi->guisection  = _("Settings");

    parm.rsm = G_define_option();
    parm.rsm->key = "smooth";
    parm.rsm->type = TYPE_DOUBLE;
    parm.rsm->required = NO;
    parm.rsm->description = _("Smoothing parameter");
    parm.rsm->guisection  = _("Settings");

    parm.scol = G_define_option();
    parm.scol->key = "scolumn";
    parm.scol->type = TYPE_STRING;
    parm.scol->required = NO;
    parm.scol->description =
	_("Name of the attribute column with smoothing parameters");
    parm.scol->guisection  = _("Settings");

    parm.segmax = G_define_option();
    parm.segmax->key = "segmax";
    parm.segmax->type = TYPE_INTEGER;
    parm.segmax->answer = MAXSEGM;
    parm.segmax->required = NO;
    parm.segmax->description = _("Maximum number of points in a segment");
    parm.segmax->guisection  = _("Settings");

    parm.npmin = G_define_option();
    parm.npmin->key = "npmin";
    parm.npmin->type = TYPE_INTEGER;
    parm.npmin->answer = MINPOINTS;
    parm.npmin->required = NO;
    parm.npmin->description =
	_("Minimum number of points for approximation in a segment (>segmax)");
    parm.npmin->guisection  = _("Settings");

    parm.dmin = G_define_option();
    parm.dmin->key = "dmin";
    parm.dmin->type = TYPE_DOUBLE;
    parm.dmin->required = NO;
    parm.dmin->answer = dminchar;
    parm.dmin->description =
	_("Minimum distance between points (to remove almost identical points)");
    parm.dmin->guisection  = _("Settings");

    parm.dmax = G_define_option();
    parm.dmax->key = "dmax";
    parm.dmax->type = TYPE_DOUBLE;
    parm.dmax->required = NO;
    parm.dmax->answer = dmaxchar; 
    parm.dmax->description =
	_("Maximum distance between points on isoline (to insert additional points)");
    parm.dmax->guisection  = _("Settings");

    parm.zmult = G_define_option();
    parm.zmult->key = "zmult";
    parm.zmult->type = TYPE_DOUBLE;
    parm.zmult->answer = ZMULT;
    parm.zmult->required = NO;
    parm.zmult->description =
	_("Conversion factor for values used for approximation");
    parm.zmult->guisection  = _("Settings");

    parm.devi = G_define_option();
    parm.devi->key = "devi";
    parm.devi->type = TYPE_STRING;
    parm.devi->required = NO;
    parm.devi->gisprompt = "new,vector,vector";
    parm.devi->description = _("Output deviations vector point file");
    parm.devi->guisection  = _("Analysis");

    parm.cvdev = G_define_standard_option(G_OPT_V_OUTPUT);
    parm.cvdev->key = "cvdev";
    parm.cvdev->required = NO;
    parm.cvdev->description = _("Output cross-validation errors vector point file");
    parm.cvdev->guisection  = _("Analysis");

    parm.treefile = G_define_standard_option(G_OPT_V_OUTPUT);
    parm.treefile->key = "treefile";
    parm.treefile->required = NO;
    parm.treefile->description =
	_("Output vector map showing quadtree segmentation");
    parm.treefile->guisection  = _("Analysis");

    parm.overfile = G_define_standard_option(G_OPT_V_OUTPUT);
    parm.overfile->key = "overfile";
    parm.overfile->required = NO;
    parm.overfile->description =
	_("Output vector map showing overlapping windows");
    parm.overfile->guisection  = _("Analysis");

    parm.theta = G_define_option();
    parm.theta->key = "theta";
    parm.theta->type = TYPE_DOUBLE;
    parm.theta->required = NO;
    parm.theta->description =
	_("Anisotropy angle (in degrees counterclockwise from East)");
    parm.theta->guisection  = _("Anisotropy");

    parm.scalex = G_define_option();
    parm.scalex->key = "scalex";
    parm.scalex->type = TYPE_DOUBLE;
    parm.scalex->required = NO;
    parm.scalex->description = _("Anisotropy scaling factor");
    parm.scalex->guisection  = _("Anisotropy");


    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);


    per = 1;
    input = parm.input->answer;
    field = atoi(parm.field->answer);
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

/*    if (treefile)
	Vect_check_input_output_name(input, treefile, GV_FATAL_EXIT);

    if (overfile)
	Vect_check_input_output_name(input, overfile, GV_FATAL_EXIT);
*/
    if ((elev == NULL) && (pcurv == NULL) && (tcurv == NULL)
	  && (mcurv == NULL)
	  && (slope == NULL) && (aspect == NULL) && (devi == NULL)
	  && (cvdev == NULL) )
	G_warning(_("You are not outputing any raster or vector maps"));

    if ( parm.wheresql->answer != NULL ) {
        if ( field < 1 )
            G_fatal_error ( _("'layer' must be > 0 for 'where'.") );
    }
    cond2 = ((pcurv != NULL) || (tcurv != NULL) || (mcurv != NULL));
    cond1 = ((slope != NULL) || (aspect != NULL) || cond2);
    deriv = flag.deriv->answer;
    dtens = flag.cprght->answer;
    cv = flag.cv->answer;

    if ((cv && cvdev == NULL) || (!(cv) && cvdev != NULL))
	G_fatal_error(_("Both cross-validation options (-c flag and cvdev vector output) must be specified"));

    if((elev != NULL || cond1 || cond2 || devi != NULL) && cv )
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

    if(parm.rsm->answer){
    	sscanf(parm.rsm->answer, "%lf", &rsm);
    	if(rsm < 0.0) G_fatal_error("Smoothing must be a positive value");
    	if(scol != NULL)
	    G_warning(_("Both smatt and smooth options specified - using constant"));
    }
    else {
	sscanf (SMOOTH, "%lf", &rsm);
    	if (scol != NULL) rsm = -1; /* used in InterpLib to indicate variable smoothing */
    }


    if (npmin > MAXPOINTS - 50){
        G_warning(_("The computation will last too long - lower npmin is suggested"));
        KMAX2 = 2 * npmin; /* was: KMAX2 = npmin + 50;*/
    }
    else
        KMAX2 = 2 * npmin; /* was: KMAX2 = MAXPOINTS; fixed by JH in 12/01*/

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

    if ((mapset = G_find_vector2(input, "")) == NULL)
	G_fatal_error(_("Vector map <%s> not found"), input);

    open_check = Vect_open_old(&Map, input, mapset);
    if (open_check < 1)
	G_fatal_error(_("Unable to open vector map <%s>"), input);
/*    if (open_check < 2)
	G_fatal_error(_("You first need to run v.build on vector map <%s>"), input);*/

    /* we can't read the input file's timestamp as they don't exist in   */
    /*   the new vector format. Even so, a TimeStamp structure is needed */
    /*   for IL_init_params_2d(), so we set it to NULL.                  */
    /* If anyone is ever motivated to add it, the Plus_head struct has   */
    /*  'long coor_mtime' and dig_head has 'char *date; char *source_date;' */
    /*   which could be read in.                                         */
    inhead.time = (struct TimeStamp *) NULL;
    inhead.stime = NULL;

    if (devi != NULL || cvdev != NULL) {

          Pnts = Vect_new_line_struct();
          Cats2 = Vect_new_cats_struct ();
          db_init_string (&sql2);

          if (devi != NULL) Vect_open_new (&Map2, devi, 1);
	  else
		  Vect_open_new (&Map2, cvdev, 1);
          Vect_hist_command ( &Map2 );
          ff = Vect_default_field_info ( &Map2, 1, NULL, GV_1TABLE );
          Vect_map_add_dblink ( &Map2, 1, NULL, ff->table, "cat", ff->database, ff->driver);

          /* Create new table */
          db_zero_string (&sql2);
          sprintf ( buf, "create table %s ( ", ff->table );
          db_append_string ( &sql2, buf);
          db_append_string ( &sql2, "cat integer" );
          db_append_string ( &sql2, ", flt1 double precision" );
          db_append_string ( &sql2, ")" );
          G_debug ( 1, db_get_string ( &sql2 ) );
          driver2 = db_start_driver_open_database ( ff->driver, ff->database );
          if ( driver2 == NULL )
             G_fatal_error (_("Cannot open database %s by driver %s"), ff->database,ff->driver );

          if (db_execute_immediate (driver2, &sql2) != DB_OK ) {
                  db_close_database(driver2);
                  db_shutdown_driver(driver2);
                  G_fatal_error (_("Cannot create table: %s"), db_get_string ( &sql2 )  );
          }
          count = 1;

    }

    ertot = 0.;
    if (per)
	G_message( _("Percent complete: "));
    if (elev != NULL)
	Tmp_file_z = G_tempfile();
    if (slope != NULL)
	Tmp_file_dx = G_tempfile();
    if (aspect != NULL)
	Tmp_file_dy = G_tempfile();
    if (pcurv != NULL)
	Tmp_file_xx = G_tempfile();
    if (tcurv != NULL)
	Tmp_file_yy = G_tempfile();
    if (mcurv != NULL)
	Tmp_file_xy = G_tempfile();

    zero_array_cell = (FCELL *) G_malloc (sizeof(FCELL) * n_cols);
    if (!zero_array_cell)
	G_fatal_error(_("Not enough memory for zero_array_cell"));

    for (i = 0; i < n_cols; i++) {
	zero_array_cell[i] = (FCELL) 0;
    }

    if (Tmp_file_z != NULL) {
	if (NULL == (Tmp_fd_z = fopen(Tmp_file_z, "w+")))
	    G_fatal_error(_("Unable to open temporary file <%s>"), Tmp_file_z);
	for (i = 0; i < n_rows; i++) {
	    if (!(fwrite(zero_array_cell, sizeof(FCELL), n_cols, Tmp_fd_z)))
		G_fatal_error(_("Not enough disk space -- cannot write files"));
	}
    }
    if (Tmp_file_dx != NULL) {
	if (NULL == (Tmp_fd_dx = fopen(Tmp_file_dx, "w+")))
	    G_fatal_error(_("Unable to open temporary file <%s>"), Tmp_file_dx);
	for (i = 0; i < n_rows; i++) {
	    if (!(fwrite(zero_array_cell, sizeof(FCELL), n_cols, Tmp_fd_dx)))
		G_fatal_error(_("Not enough disk space -- cannot write files"));
	}
    }
    if (Tmp_file_dy != NULL) {
	if (NULL == (Tmp_fd_dy = fopen(Tmp_file_dy, "w+")))
	    G_fatal_error(_("Unable to open temporary file <%s>"), Tmp_file_dy);
	for (i = 0; i < n_rows; i++) {
	    if (!(fwrite(zero_array_cell, sizeof(FCELL), n_cols, Tmp_fd_dy)))
		G_fatal_error(_("Not enough disk space -- cannot write files"));
	}
    }

    if (Tmp_file_xx != NULL) {
	if (NULL == (Tmp_fd_xx = fopen(Tmp_file_xx, "w+")))
	    G_fatal_error(_("Unable to open temporary file <%s>"), Tmp_file_xx);
	for (i = 0; i < n_rows; i++) {
	    if (!(fwrite(zero_array_cell, sizeof(FCELL), n_cols, Tmp_fd_xx)))
		G_fatal_error(_("Not enough disk space -- cannot write files"));
	}
    }
    if (Tmp_file_yy != NULL) {
	if (NULL == (Tmp_fd_yy = fopen(Tmp_file_yy, "w+")))
	    G_fatal_error(_("Unable to open temporary file <%s>"), Tmp_file_yy);
	for (i = 0; i < n_rows; i++) {
	    if (!(fwrite(zero_array_cell, sizeof(FCELL), n_cols, Tmp_fd_yy)))
		G_fatal_error(_("Not enough disk space -- cannot write files"));
	}
    }
    if (Tmp_file_xy != NULL) {
	if (NULL == (Tmp_fd_xy = fopen(Tmp_file_xy, "w+")))
	    G_fatal_error(_("Unable to open temporary file <%s>"), Tmp_file_xy);
	for (i = 0; i < n_rows; i++) {
	    if (!(fwrite(zero_array_cell, sizeof(FCELL), n_cols, Tmp_fd_xy)))
		G_fatal_error(_("Not enough disk space -- cannot write files"));
	}
    }

    IL_init_params_2d(&params, NULL, 1, 1, zmult, KMIN, KMAX, maskmap, n_rows,
		      n_cols, az, adx, ady, adxx, adyy, adxy, fi, KMAX2,
		      SCIK1, SCIK2, SCIK3, rsm, elev, slope, aspect, pcurv,
		      tcurv, mcurv, dmin, x_orig, y_orig, deriv, theta,
		      scalex, Tmp_fd_z, Tmp_fd_dx, Tmp_fd_dy, Tmp_fd_xx,
		      Tmp_fd_yy, Tmp_fd_xy, devi, inhead.time, cv, parm.wheresql->answer);

    IL_init_func_2d(&params, IL_grid_calc_2d, IL_matrix_create,
		    IL_check_at_points_2d, IL_secpar_loop_2d, IL_crst,
		    IL_crstg, IL_write_temp_2d);

    totsegm =
	IL_vector_input_data_2d(&params, &Map, field, zcol, scol,
				info, &xmin, &xmax,
				&ymin, &ymax, &zmin, &zmax, &NPOINT, &dmax);
    if (totsegm <= 0)
	clean_fatal_error(_("Input failed"));

    /*Vect_set_release_support(&Map);*/
    Vect_close(&Map);

    if (treefile != NULL) {
	if (0 > Vect_open_new(&TreeMap, treefile, 0)) {
	    sprintf(msg, _("Unable to open vector map <%s>"), treefile);
	    clean_fatal_error(msg);
	}
	Vect_hist_command(&TreeMap);

	/*
	   sprintf (TreeMap.head.your_name, "grass");
	   sprintf (TreeMap.head.map_name, "Quad tree for %s", input);
	   TreeMap.head.orig_scale = 100000;
	   TreeMap.head.plani_zone = G_zone ();
	 */
	print_tree(root, x_orig, y_orig, &TreeMap);
	Vect_build(&TreeMap, NULL);
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
    G_message(_("Processing all selected output files\n"
                "will require %d bytes of disk space for temp files"),
	    ddisk);

    deltx = xmax - xmin;
    delty = ymax - ymin;
    dnorm = sqrt((deltx * delty * KMIN) / NPOINT);

    if (dtens) {
	params.fi = params.fi * dnorm / 1000.;
	G_message("dnorm = %f, rescaled tension = %f", dnorm,
		params.fi);
    }

    bitmask = IL_create_bitmask(&params);
    if (totsegm <= 0)
	clean_fatal_error(_("Input failed"));

    ertot = 0.;
    if (per)
	G_message( _("Percent complete: "));
    if (IL_interp_segments_2d(&params, info, info->root, bitmask,
			      zmin, zmax, &zminac, &zmaxac, &gmin, &gmax,
			      &c1min, &c1max, &c2min, &c2max, &ertot, totsegm,
			      n_cols, dnorm) < 0)

	clean_fatal_error(_("Interp_segmets failed"));

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
    if (ii < 0)
	clean_fatal_error
	    (_("Cannot write raster maps -- try to increase resolution"));
    G_free (zero_array_cell);
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
	    sprintf(msg, _("Unable to open vector map <%s>"), overfile);
	    clean_fatal_error(msg);
	}
	Vect_hist_command(&OverMap);

	/*
	   sprintf (OverMap.head.your_name, "grass");
	   sprintf (OverMap.head.map_name, "Overlap segments for %s", input);
	   OverMap.head.orig_scale = 100000;
	   OverMap.head.plani_zone = G_zone ();
	 */
	print_tree(root, x_orig, y_orig, &OverMap);
	Vect_build(&OverMap, NULL);
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
	    /*  db_close_database_shutdown_driver ( driver2 );*/
	    db_close_database (driver2);
	    Vect_build (&Map2, stderr);
	    Vect_close (&Map2);
    }

    G_done_msg("\n");
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
	xarray[0] = ((struct quaddata *) (tree->data))->x_orig + x_orig;
	yarray[0] = ((struct quaddata *) (tree->data))->y_orig + y_orig;
	xarray[1] = xarray[0];
	yarray[3] = yarray[0];
	xarray[3] = ((struct quaddata *) (tree->data))->xmax + x_orig;
	yarray[1] = ((struct quaddata *) (tree->data))->ymax + y_orig;
	yarray[2] = yarray[1];
	xarray[2] = xarray[3];
	yarray[4] = yarray[0];
	xarray[4] = xarray[0];
	if (0 > Vect_copy_xyz_to_pnts(Points, xarray, yarray, zarray, 5))
	    clean_fatal_error(_("Out of memory"));
	Vect_write_line(Map, (unsigned int) type, Points, Cats);

	G_free (Points);
    }
    return 1;
}



void clean_fatal_error(char *str)
{
    if (Tmp_fd_z) {
	fclose(Tmp_fd_z);
	unlink(Tmp_file_z);
    }
    if (Tmp_fd_dx) {
	fclose(Tmp_fd_dx);
	unlink(Tmp_file_dx);
    }
    if (Tmp_fd_dy) {
	fclose(Tmp_fd_dy);
	unlink(Tmp_file_dy);
    }
    if (Tmp_fd_xx) {
	fclose(Tmp_fd_xx);
	unlink(Tmp_file_xx);
    }
    if (Tmp_fd_yy) {
	fclose(Tmp_fd_yy);
	unlink(Tmp_file_yy);
    }
    if (Tmp_fd_xy) {
	fclose(Tmp_fd_xy);
	unlink(Tmp_file_xy);
    }
    G_fatal_error(str);
}

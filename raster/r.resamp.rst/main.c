/* r.resamp.rst - flexible, normalized segmented processing surface analysis
 *    program with tension and smoothing.
 *
 * Written by H. Mitasova, I. Kosinovsky, D. Gerdes Summer 1993
 * University of Illinois
 * US Army Construction Engineering Research Lab
 * Copyright 1993, H. Mitasova (University of Illinois),
 * I. Kosinovsky, (USA-CERL), and D.Gerdes (USA-CERL)
 * 
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 *
 *
 * modified by McCauley in August 1995
 * modified by Mitasova in August 1995
 * modified by Mitasova in November 1999
 *
 */


#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>

#define MAIN

#include <grass/dbmi.h>
#include <grass/gis.h>
#include <grass/linkm.h>
#include <grass/bitmap.h>
#include "surf.h"
#include <grass/interpf.h>
#include <grass/glocale.h>

#include "local_proto.h"


double /* pargr */ ns_res, ew_res, inp_ew_res, inp_ns_res;
int inp_rows, inp_cols;
double x_orig, y_orig;
double inp_x_orig, inp_y_orig;
double dmin, ertre, deltx, delty;
int nsizr, nsizc;
int KMAX2 /* , KMIN, KMAX */ ;

double /* datgr */ *az, *adx, *ady, *adxx, *adyy, *adxy;
double /* error */ ertot, ertre, zminac, zmaxac, zmult;

int total = 0;
int NPOINT = 0;
int OUTRANGE = 0;
int NPT = 0;
int deriv, overlap, cursegm, dtens;
double fi;

double DETERM;
int NERROR, cond1, cond2;
char fncdsm[32];
char filnam[10];
char msg[1024];
double fstar2, tfsta2, xmin, xmax, ymin, ymax, zmin, zmax, gmin, gmax, c1min,
    c1max, c2min, c2max;
double dnorm;
double smc;
double theta, scalex;

FCELL *zero_array_cell;
struct interp_params params;

FILE *fdredinp, *fdzout, *fddxout, *fddyout, *fdxxout, *fdyyout, *fxyout;
FILE *fd4; /* unused? */
int fdinp, fdsmooth = -1;

/*
 * x,y,z - input data npoint - number of input data fi - tension parameter
 * b - coef. of int. function a - matrix of system of linear equations az-
 * interpolated values z for output grid adx,ady, ... - estimation of
 * derivatives for output grid nsizr,nsizc - number of rows and columns
 * for output grid xmin ... - coordinates of corners of output grid
 * 
 * subroutines input_data - input of data x,y,z (test function or measured
 * data) iterpolate - interpolation of z-values and derivatives to grid
 * secpar_loop- computation of secondary(morphometric) parameters output -
 * output of gridded data and derivatives/sec.parameters check_at_points -
 * interpolation of z-values to given point x,y
 */

char *input;
char *smooth = NULL;
char *mapset;
char *elev = NULL;
char *slope = NULL;
char *aspect = NULL;
char *pcurv = NULL;
char *tcurv = NULL;
char *mcurv = NULL;
char *maskmap = NULL;
char *redinp = NULL;
int sdisk, disk;
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

struct BM *bitmask;
struct Cell_head winhd;
struct Cell_head inphd;
struct Cell_head outhd;
struct Cell_head smhd;

int main(int argc, char *argv[])
{
    int m1, ret_val;
    struct FPRange range;
    DCELL cellmin, cellmax;
    FCELL *cellrow, fcellmin;

    struct GModule *module;
    struct
    {
	struct Option *input, *elev, *slope, *aspect, *pcurv, *tcurv, *mcurv,
	    *smooth, *maskmap, *zmult, *fi, *segmax, *npmin, *res_ew, *res_ns,
	    *overlap, *theta, *scalex;
    } parm;
    struct
    {
	struct Flag *deriv, *cprght;
    } flag;


    G_gisinit(argv[0]);

    module = G_define_module();
    module->keywords = _("raster");
    module->description =
	_("Reinterpolates and optionally computes topographic analysis from "
	  "input raster map to a new raster map (possibly with "
	  "different resolution) using regularized spline with "
	  "tension and smoothing.");

    parm.input = G_define_option();
    parm.input->key = "input";
    parm.input->type = TYPE_STRING;
    parm.input->required = YES;
    parm.input->gisprompt = "old,cell,raster";
    parm.input->description = _("Name of input raster map");

    parm.res_ew = G_define_option();
    parm.res_ew->key = "ew_res";
    parm.res_ew->type = TYPE_DOUBLE;
    parm.res_ew->required = YES;
    parm.res_ew->description = _("Desired east-west resolution");

    parm.res_ns = G_define_option();
    parm.res_ns->key = "ns_res";
    parm.res_ns->type = TYPE_DOUBLE;
    parm.res_ns->required = YES;
    parm.res_ns->description = _("Desired north-south resolution");

    parm.elev = G_define_option();
    parm.elev->key = "elev";
    parm.elev->type = TYPE_STRING;
    parm.elev->required = NO;
    parm.elev->gisprompt = "new,cell,raster";
    parm.elev->description = _("Output z-file (elevation) map");

    parm.slope = G_define_option();
    parm.slope->key = "slope";
    parm.slope->type = TYPE_STRING;
    parm.slope->required = NO;
    parm.slope->gisprompt = "new,cell,raster";
    parm.slope->description = _("Output slope map (or fx)");
    parm.slope->guisection  = _("Output_options");

    parm.aspect = G_define_option();
    parm.aspect->key = "aspect";
    parm.aspect->type = TYPE_STRING;
    parm.aspect->required = NO;
    parm.aspect->gisprompt = "new,cell,raster";
    parm.aspect->description = _("Output aspect map (or fy)");
    parm.aspect->guisection  = _("Output_options");

    parm.pcurv = G_define_option();
    parm.pcurv->key = "pcurv";
    parm.pcurv->type = TYPE_STRING;
    parm.pcurv->required = NO;
    parm.pcurv->gisprompt = "new,cell,raster";
    parm.pcurv->description = _("Output profile curvature map (or fxx)");
    parm.pcurv->guisection  = _("Output_options");

    parm.tcurv = G_define_option();
    parm.tcurv->key = "tcurv";
    parm.tcurv->type = TYPE_STRING;
    parm.tcurv->required = NO;
    parm.tcurv->gisprompt = "new,cell,raster";
    parm.tcurv->description = _("Output tangential curvature map (or fyy)");
    parm.tcurv->guisection  = _("Output_options");

    parm.mcurv = G_define_option();
    parm.mcurv->key = "mcurv";
    parm.mcurv->type = TYPE_STRING;
    parm.mcurv->required = NO;
    parm.mcurv->gisprompt = "new,cell,raster";
    parm.mcurv->description = _("Output mean curvature map (or fxy)");
    parm.mcurv->guisection  = _("Output_options");

    parm.smooth = G_define_option();
    parm.smooth->key = "smooth";
    parm.smooth->type = TYPE_STRING;
    parm.smooth->required = NO;
    parm.smooth->gisprompt = "old,cell,raster";
    parm.smooth->description = _("Name of raster map containing smoothing");
    parm.smooth->guisection  = _("Settings");

    parm.maskmap = G_define_option();
    parm.maskmap->key = "maskmap";
    parm.maskmap->type = TYPE_STRING;
    parm.maskmap->required = NO;
    parm.maskmap->gisprompt = "old,cell,raster";
    parm.maskmap->description = _("Name of raster map to be used as mask");
    parm.maskmap->guisection  = _("Settings");

    parm.overlap = G_define_option();
    parm.overlap->key = "overlap";
    parm.overlap->type = TYPE_INTEGER;
    parm.overlap->required = NO;
    parm.overlap->answer = OVERLAP;
    parm.overlap->description = _("Rows/columns overlap for segmentation");
    parm.overlap->guisection  = _("Settings");

    parm.zmult = G_define_option();
    parm.zmult->key = "zmult";
    parm.zmult->type = TYPE_DOUBLE;
    parm.zmult->answer = ZMULT;
    parm.zmult->required = NO;
    parm.zmult->description = _("Multiplier for z-values");
    parm.zmult->guisection  = _("Settings");

    parm.fi = G_define_option();
    parm.fi->key = "tension";
    parm.fi->type = TYPE_DOUBLE;
    parm.fi->answer = TENSION;
    parm.fi->required = NO;
    parm.fi->description = _("Spline tension value");
    parm.fi->guisection  = _("Settings");

    parm.theta = G_define_option();
    parm.theta->key = "theta";
    parm.theta->type = TYPE_DOUBLE;
    parm.theta->required = NO;
    parm.theta->description = _("Anisotropy angle (in degrees)");
    parm.theta->guisection  = _("Anisotropy");

    parm.scalex = G_define_option();
    parm.scalex->key = "scalex";
    parm.scalex->type = TYPE_DOUBLE;
    parm.scalex->required = NO;
    parm.scalex->description = _("Anisotropy scaling factor");
    parm.scalex->guisection  = _("Anisotropy");

    flag.cprght = G_define_flag();
    flag.cprght->key = 't';
    flag.cprght->description = _("Use dnorm independent tension");

    flag.deriv = G_define_flag();
    flag.deriv->key = 'd';
    flag.deriv->description =
	_("Output partial derivatives instead of topographic parameters");
    flag.deriv->guisection  = _("Output_options");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    if (G_get_set_window(&winhd) == -1)
	G_fatal_error(_("Retrieving and setting region failed"));

    inp_ew_res = winhd.ew_res;
    inp_ns_res = winhd.ns_res;
    inp_cols = winhd.cols;
    inp_rows = winhd.rows;
    inp_x_orig = winhd.west;
    inp_y_orig = winhd.south;

    input = parm.input->answer;
    smooth = parm.smooth->answer;
    maskmap = parm.maskmap->answer;

    elev = parm.elev->answer;
    slope = parm.slope->answer;
    aspect = parm.aspect->answer;
    pcurv = parm.pcurv->answer;
    tcurv = parm.tcurv->answer;
    mcurv = parm.mcurv->answer;

    cond2 = ((pcurv != NULL) || (tcurv != NULL) || (mcurv != NULL));
    cond1 = ((slope != NULL) || (aspect != NULL) || cond2);
    deriv = flag.deriv->answer;
    dtens = flag.cprght->answer;

    ertre = 0.1;

    if (!G_scan_resolution(parm.res_ew->answer, &ew_res, winhd.proj))
	G_fatal_error(_("Cannot read ew_res value"));

    if (!G_scan_resolution(parm.res_ns->answer, &ns_res, winhd.proj))
	G_fatal_error(_("Cannot read ns_res value"));

    if( sscanf(parm.fi->answer, "%lf", &fi) != 1)
	G_fatal_error(_("Invalid value for tension"));

    if( sscanf(parm.zmult->answer, "%lf", &zmult) != 1)
	G_fatal_error(_("Invalid value for zmult"));

    if( sscanf(parm.overlap->answer, "%d", &overlap) != 1)
	G_fatal_error(_("Invalid value for overlap"));

    if (parm.theta->answer) {
	if( sscanf(parm.theta->answer, "%lf", &theta) != 1)
	    G_fatal_error(_("Invalid value for theta"));
    }
    if (parm.scalex->answer) {
	if( sscanf(parm.scalex->answer, "%lf", &scalex) != 1)
	    G_fatal_error(_("Invalid value for scalex"));
	if (!parm.theta->answer)
	    G_fatal_error(_("When using anisotropy both theta and scalex must be specified"));
    }

    /*
     * G_set_embedded_null_value_mode(1);
     */
    outhd.ew_res = ew_res;
    outhd.ns_res = ns_res;
    outhd.east = winhd.east;
    outhd.west = winhd.west;
    outhd.north = winhd.north;
    outhd.south = winhd.south;
    outhd.proj = winhd.proj;
    outhd.zone = winhd.zone;
    G_adjust_Cell_head(&outhd, 0, 0);
    ew_res = outhd.ew_res;
    ns_res = outhd.ns_res;
    nsizc = outhd.cols;
    nsizr = outhd.rows;
    disk = nsizc * nsizr * sizeof(int);

    az = G_alloc_vector(nsizc + 1);
    if (!az)
	G_fatal_error(_("Not enough memory for az"));

    if (cond1) {
	adx = G_alloc_vector(nsizc + 1);
	if (!adx)
	    G_fatal_error(_("Not enough memory for adx"));

	ady = G_alloc_vector(nsizc + 1);
	if (!ady)
	    G_fatal_error(_("Not enough memory for ady"));

	if (cond2) {
	    adxx = G_alloc_vector(nsizc + 1);
	    if (!adxx)
		G_fatal_error(_("Not enough memory for adxx"));

	    adyy = G_alloc_vector(nsizc + 1);
	    if (!adyy)
		G_fatal_error(_("Not enough memory for adyy"));

	    adxy = G_alloc_vector(nsizc + 1);
	    if (!adxy)
		G_fatal_error(_("Not enough memory for adxy"));
	}
    }
    mapset = NULL;
    if (smooth != NULL) {

	mapset = G_find_file("cell", smooth, "");

	if (mapset == NULL)
	    G_fatal_error(_("Raster map <%s> not found"), smooth);

	G_debug(1, "mapset for smooth map is [%s]", mapset);

	if ((fdsmooth = G_open_cell_old(smooth, mapset)) < 0)
	    G_fatal_error(_("Unable to open raster map <%s>"), smooth);

	if (G_get_cellhd(smooth, mapset, &smhd) < 0)
	    G_fatal_error(_("[%s]: Cannot read map header"), smooth);

	if ((winhd.ew_res != smhd.ew_res) || (winhd.ns_res != smhd.ns_res))
	    G_fatal_error(_("[%s]: Map is the wrong resolution"), smooth);

	if (G_read_fp_range(smooth, mapset, &range) >= 0)
	    G_get_fp_range_min_max(&range, &cellmin, &cellmax);

	fcellmin = (float)cellmin;

	if (G_is_f_null_value(&fcellmin) || fcellmin < 0.0)
	    G_fatal_error(_("Smoothing values can not be negative or NULL"));
    }

    mapset = NULL;
    mapset = G_find_file("cell", input, "");

    if (mapset == NULL)
	G_fatal_error(_("Raster map <%s> not found"), input);

    G_debug(1, "mapset for input map is [%s]", mapset);

    if (G_get_cellhd(input, mapset, &inphd) < 0)
	G_fatal_error(_("[%s]: Cannot read map header"), input);

    if ((winhd.ew_res != inphd.ew_res) || (winhd.ns_res != inphd.ns_res))
	G_fatal_error(_("Input map resolution differs from current region resolution!"));

    if ((fdinp = G_open_cell_old(input, mapset)) < 0)
	G_fatal_error(_("Unable to open raster map <%s>"), input);


    sdisk = 0;
    if (elev != NULL)
	sdisk += disk;
    if (slope != NULL)
	sdisk += disk;
    if (aspect != NULL)
	sdisk += disk;
    if (pcurv != NULL)
	sdisk += disk;
    if (tcurv != NULL)
	sdisk += disk;
    if (mcurv != NULL)
	sdisk += disk;

    G_message(_("Processing all selected output files will require"));
    G_message(_("%d bytes of disk space for temp files."), sdisk);


    fstar2 = fi * fi / 4.;
    tfsta2 = fstar2 + fstar2;
    deltx = winhd.east - winhd.west;
    delty = winhd.north - winhd.south;
    xmin = winhd.west;
    xmax = winhd.east;
    ymin = winhd.south;
    ymax = winhd.north;
    if (smooth != NULL)
	smc = -9999;
    else
	smc = 0.01;


    if (G_read_fp_range(input, mapset, &range) >= 0) {
	G_get_fp_range_min_max(&range, &cellmin, &cellmax);
    }
    else {
	cellrow = G_allocate_f_raster_buf();
	for (m1 = 0; m1 < inp_rows; m1++) {
	    ret_val = G_get_f_raster_row(fdinp, cellrow, m1);
	    if (ret_val < 0)
		G_fatal_error(_("Cannot get row %d (error = %d)"), m1, ret_val);

	    G_row_update_fp_range(cellrow, m1, &range, FCELL_TYPE);
	}
	G_get_fp_range_min_max(&range, &cellmin, &cellmax);
    }

    fcellmin = (float)cellmin;
    if (G_is_f_null_value(&fcellmin))
	G_fatal_error(_("Maximum value of a raster map is NULL."));

    zmin = (double)cellmin *zmult;
    zmax = (double)cellmax *zmult;

    G_debug(1, "zmin=%f, zmax=%f", zmin, zmax);

    if (fd4 != NULL)
	fprintf(fd4, "deltx,delty %f %f \n", deltx, delty);
    create_temp_files();

    IL_init_params_2d(&params, NULL, 1, 1, zmult, KMIN, KMAX, maskmap,
		      outhd.rows, outhd.cols, az, adx, ady, adxx, adyy, adxy,
		      fi, MAXPOINTS, SCIK1, SCIK2, SCIK3, smc, elev, slope,
		      aspect, pcurv, tcurv, mcurv, dmin, inp_x_orig, inp_y_orig,
		      deriv, theta, scalex, Tmp_fd_z, Tmp_fd_dx, Tmp_fd_dy,
		      Tmp_fd_xx, Tmp_fd_yy, Tmp_fd_xy, NULL, NULL, 0, NULL);

    /*  In the above line, the penultimate argument is supposed to be a 
     * deviations file pointer.  None is obvious, so I used NULL. */
    /*  The 3rd and 4th argument are int-s, elatt and smatt (from the function
     * definition.  The value 1 seemed like a good placeholder...  or not. */

    IL_init_func_2d(&params, IL_grid_calc_2d, IL_matrix_create,
		    IL_check_at_points_2d,
		    IL_secpar_loop_2d, IL_crst, IL_crstg, IL_write_temp_2d);

    G_message(_("Temporarily changing the region to desired resolution ..."));
    if (G_set_window(&outhd) < 0)
	G_fatal_error("Cannot set region to output region!");

    bitmask = IL_create_bitmask(&params);
    /* change region to initial region */
    G_message(_("Changing back to the original region ..."));
    if (G_set_window(&winhd) < 0)
	G_fatal_error(_("Cannot set region to back to the initial region !!!"));

    ertot = 0.;
    cursegm = 0;
    G_message(_("Percent complete: "));


    NPOINT =
	IL_resample_interp_segments_2d(&params, bitmask, zmin, zmax, &zminac,
				       &zmaxac, &gmin, &gmax, &c1min, &c1max,
				       &c2min, &c2max, &ertot, nsizc, &dnorm,
				       overlap, inp_rows, inp_cols, fdsmooth,
				       fdinp, ns_res, ew_res, inp_ns_res,
				       inp_ew_res, dtens);


    G_message(_( "dnorm in mainc after grid before out1= %f"), dnorm);

    if (NPOINT < 0)
	clean_fatal_error("split_and_interpolate() failed");

    if (fd4 != NULL)
	fprintf(fd4, "max. error found = %f \n", ertot);
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
    G_message(_("dnorm in mainc after grid before out2= %f"), dnorm);

    if (IL_resample_output_2d(&params, zmin, zmax, zminac, zmaxac, c1min,
			      c1max, c2min, c2max, gmin, gmax, ertot, input,
			      &dnorm, &outhd, &winhd, smooth, NPOINT) < 0)
	clean_fatal_error("Can not write raster maps -- try increasing cell size");

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
    if (fd4)
	fclose(fd4);
    G_close_cell(fdinp);
    if (smooth != NULL)
	G_close_cell(fdsmooth);

    G_done_msg("");
    exit(EXIT_SUCCESS);
}


void create_temp_files(void)
{
    int i;

    zero_array_cell = (FCELL *) G_malloc (sizeof(FCELL) * nsizc);
    if (!zero_array_cell)
	G_fatal_error(_("Not enough memory for zero_array_cell"));

    for (i = 0; i < nsizc; i++) {
	zero_array_cell[i] = (FCELL) 0;
    }

    if (elev != NULL) {
	Tmp_file_z = G_tempfile();
	if (NULL == (Tmp_fd_z = fopen(Tmp_file_z, "w+")))
	    G_fatal_error(_("Unable to open temporary file <%s>"), Tmp_file_z);

	for (i = 0; i < nsizr; i++) {
	    if (!(fwrite(zero_array_cell, sizeof(FCELL), nsizc, Tmp_fd_z)))
		clean_fatal_error
		    (_("Not enough disk space -- cannot write files"));
	}
    }
    if (slope != NULL) {
	Tmp_file_dx = G_tempfile();
	if (NULL == (Tmp_fd_dx = fopen(Tmp_file_dx, "w+"))) {
	    sprintf(msg, _("Unable to open temporary file <%s>"), Tmp_file_dx);
	    clean_fatal_error(msg);
	}
	for (i = 0; i < nsizr; i++) {
	    if (!(fwrite(zero_array_cell, sizeof(FCELL), nsizc, Tmp_fd_dx)))
		clean_fatal_error
		    (_("Not enough disk space -- cannot write files"));
	}
    }
    if (aspect != NULL) {
	Tmp_file_dy = G_tempfile();
	if (NULL == (Tmp_fd_dy = fopen(Tmp_file_dy, "w+"))) {
	    sprintf(msg, _("Unable to open temporary file <%s>"), Tmp_file_dy);
	    clean_fatal_error(msg);
	}
	for (i = 0; i < nsizr; i++) {
	    if (!(fwrite(zero_array_cell, sizeof(FCELL), nsizc, Tmp_fd_dy)))
		clean_fatal_error
		    (_("Not enough disk space -- cannot write files"));
	}
    }

    if (pcurv != NULL) {
	Tmp_file_xx = G_tempfile();
	if (NULL == (Tmp_fd_xx = fopen(Tmp_file_xx, "w+"))) {
	    sprintf(msg, _("Unable to open temporary file <%s>"), Tmp_file_xx);
	    clean_fatal_error(msg);
	}
	for (i = 0; i < nsizr; i++) {
	    if (!(fwrite(zero_array_cell, sizeof(FCELL), nsizc, Tmp_fd_xx)))
		clean_fatal_error
		    (_("Not enough disk space -- cannot write files"));
	}
    }
    if (tcurv != NULL) {
	Tmp_file_yy = G_tempfile();
	if (NULL == (Tmp_fd_yy = fopen(Tmp_file_yy, "w+"))) {
	    sprintf(msg, _("Unable to open temporary file <%s>"), Tmp_file_yy);
	    clean_fatal_error(msg);
	}
	for (i = 0; i < nsizr; i++) {
	    if (!(fwrite(zero_array_cell, sizeof(FCELL), nsizc, Tmp_fd_yy)))
		clean_fatal_error
		    (_("Not enough disk space -- cannot write files"));
	}
    }
    if (mcurv != NULL) {
	Tmp_file_xy = G_tempfile();
	if (NULL == (Tmp_fd_xy = fopen(Tmp_file_xy, "w+"))) {
	    sprintf(msg, _("Unable to open temporary file <%s>"), Tmp_file_xy);
	    clean_fatal_error(msg);
	}
	for (i = 0; i < nsizr; i++) {
	    if (!(fwrite(zero_array_cell, sizeof(FCELL), nsizc, Tmp_fd_xy)))
		clean_fatal_error
		    (_("Not enough disk space -- cannot write files"));
	}
    }

    return;
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

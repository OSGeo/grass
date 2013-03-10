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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
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

#include <grass/dbmi.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/linkm.h>
#include <grass/bitmap.h>
#include "surf.h"
#include <grass/interpf.h>
#include <grass/glocale.h>
#include <grass/gmath.h>

static double /* pargr */ ns_res, ew_res, inp_ew_res, inp_ns_res;
static int inp_rows, inp_cols;

static double inp_x_orig, inp_y_orig;
static double dmin, ertre, deltx, delty;
static int nsizr, nsizc;

static double /* datgr */ *az, *adx, *ady, *adxx, *adyy, *adxy;
static double /* error */ ertot, ertre, zminac, zmaxac, zmult;

static int NPOINT;
static int deriv, overlap, cursegm, dtens;
static double fi;

static int cond1, cond2;
static double fstar2, tfsta2, xmin, xmax, ymin, ymax, zmin, zmax, gmin, gmax,
    c1min, c1max, c2min, c2max;
static double dnorm;
static double smc;
static double theta, scalex;

static FCELL *zero_array_cell;
static struct interp_params params;

static FILE *fd4;			/* unused? */
static int fdinp, fdsmooth = -1;

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

static char *input;
static char *smooth;
static char *elev;
static char *slope;
static char *aspect;
static char *pcurv;
static char *tcurv;
static char *mcurv;
static char *maskmap;

static off_t sdisk, disk;

static char *Tmp_file_z;
static char *Tmp_file_dx;
static char *Tmp_file_dy;
static char *Tmp_file_xx;
static char *Tmp_file_yy;
static char *Tmp_file_xy;

static FILE *Tmp_fd_z;
static FILE *Tmp_fd_dx;
static FILE *Tmp_fd_dy;
static FILE *Tmp_fd_xx;
static FILE *Tmp_fd_yy;
static FILE *Tmp_fd_xy;
static FILE *Tmp_fd_z;

static struct BM *bitmask;
static struct Cell_head winhd;
static struct Cell_head inphd;
static struct Cell_head outhd;
static struct Cell_head smhd;

static void create_temp_files(void);
static void clean(void);

int main(int argc, char *argv[])
{
    int m1;
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
    G_add_keyword(_("raster"));
    G_add_keyword(_("resample"));
    module->description =
	_("Reinterpolates and optionally computes topographic analysis from "
	  "input raster map to a new raster map (possibly with "
	  "different resolution) using regularized spline with "
	  "tension and smoothing.");

    parm.input = G_define_standard_option(G_OPT_R_INPUT);

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
    parm.elev->guisection = _("Output");

    parm.slope = G_define_option();
    parm.slope->key = "slope";
    parm.slope->type = TYPE_STRING;
    parm.slope->required = NO;
    parm.slope->gisprompt = "new,cell,raster";
    parm.slope->description = _("Output slope map (or fx)");
    parm.slope->guisection = _("Output");

    parm.aspect = G_define_option();
    parm.aspect->key = "aspect";
    parm.aspect->type = TYPE_STRING;
    parm.aspect->required = NO;
    parm.aspect->gisprompt = "new,cell,raster";
    parm.aspect->description = _("Output aspect map (or fy)");
    parm.aspect->guisection = _("Output");

    parm.pcurv = G_define_option();
    parm.pcurv->key = "pcurv";
    parm.pcurv->type = TYPE_STRING;
    parm.pcurv->required = NO;
    parm.pcurv->gisprompt = "new,cell,raster";
    parm.pcurv->description = _("Output profile curvature map (or fxx)");
    parm.pcurv->guisection = _("Output");

    parm.tcurv = G_define_option();
    parm.tcurv->key = "tcurv";
    parm.tcurv->type = TYPE_STRING;
    parm.tcurv->required = NO;
    parm.tcurv->gisprompt = "new,cell,raster";
    parm.tcurv->description = _("Output tangential curvature map (or fyy)");
    parm.tcurv->guisection = _("Output");

    parm.mcurv = G_define_option();
    parm.mcurv->key = "mcurv";
    parm.mcurv->type = TYPE_STRING;
    parm.mcurv->required = NO;
    parm.mcurv->gisprompt = "new,cell,raster";
    parm.mcurv->description = _("Output mean curvature map (or fxy)");
    parm.mcurv->guisection = _("Output");

    parm.smooth = G_define_option();
    parm.smooth->key = "smooth";
    parm.smooth->type = TYPE_STRING;
    parm.smooth->required = NO;
    parm.smooth->gisprompt = "old,cell,raster";
    parm.smooth->description = _("Name of raster map containing smoothing");
    parm.smooth->guisection = _("Settings");

    parm.maskmap = G_define_option();
    parm.maskmap->key = "maskmap";
    parm.maskmap->type = TYPE_STRING;
    parm.maskmap->required = NO;
    parm.maskmap->gisprompt = "old,cell,raster";
    parm.maskmap->description = _("Name of raster map to be used as mask");
    parm.maskmap->guisection = _("Settings");

    parm.overlap = G_define_option();
    parm.overlap->key = "overlap";
    parm.overlap->type = TYPE_INTEGER;
    parm.overlap->required = NO;
    parm.overlap->answer = OVERLAP;
    parm.overlap->description = _("Rows/columns overlap for segmentation");
    parm.overlap->guisection = _("Settings");

    parm.zmult = G_define_option();
    parm.zmult->key = "zmult";
    parm.zmult->type = TYPE_DOUBLE;
    parm.zmult->answer = ZMULT;
    parm.zmult->required = NO;
    parm.zmult->description = _("Multiplier for z-values");
    parm.zmult->guisection = _("Settings");

    parm.fi = G_define_option();
    parm.fi->key = "tension";
    parm.fi->type = TYPE_DOUBLE;
    parm.fi->answer = TENSION;
    parm.fi->required = NO;
    parm.fi->description = _("Spline tension value");
    parm.fi->guisection = _("Settings");

    parm.theta = G_define_option();
    parm.theta->key = "theta";
    parm.theta->type = TYPE_DOUBLE;
    parm.theta->required = NO;
    parm.theta->description = _("Anisotropy angle (in degrees)");
    parm.theta->guisection = _("Anisotropy");

    parm.scalex = G_define_option();
    parm.scalex->key = "scalex";
    parm.scalex->type = TYPE_DOUBLE;
    parm.scalex->required = NO;
    parm.scalex->description = _("Anisotropy scaling factor");
    parm.scalex->guisection = _("Anisotropy");

    flag.cprght = G_define_flag();
    flag.cprght->key = 't';
    flag.cprght->description = _("Use dnorm independent tension");

    flag.deriv = G_define_flag();
    flag.deriv->key = 'd';
    flag.deriv->description =
	_("Output partial derivatives instead of topographic parameters");
    flag.deriv->guisection = _("Output");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    G_get_set_window(&winhd);

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
	G_fatal_error(_("Unable to read ew_res value"));

    if (!G_scan_resolution(parm.res_ns->answer, &ns_res, winhd.proj))
	G_fatal_error(_("Unable to read ns_res value"));

    if (sscanf(parm.fi->answer, "%lf", &fi) != 1)
	G_fatal_error(_("Invalid value for tension"));

    if (sscanf(parm.zmult->answer, "%lf", &zmult) != 1)
	G_fatal_error(_("Invalid value for zmult"));

    if (sscanf(parm.overlap->answer, "%d", &overlap) != 1)
	G_fatal_error(_("Invalid value for overlap"));

    if (parm.theta->answer) {
	if (sscanf(parm.theta->answer, "%lf", &theta) != 1)
	    G_fatal_error(_("Invalid value for theta"));
    }
    if (parm.scalex->answer) {
	if (sscanf(parm.scalex->answer, "%lf", &scalex) != 1)
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
    disk = (off_t)nsizc * nsizr * sizeof(int);

    az = G_alloc_vector(nsizc + 1);

    if (cond1) {
	adx = G_alloc_vector(nsizc + 1);
	ady = G_alloc_vector(nsizc + 1);
	if (cond2) {
	    adxx = G_alloc_vector(nsizc + 1);
	    adyy = G_alloc_vector(nsizc + 1);
	    adxy = G_alloc_vector(nsizc + 1);
	}
    }

    if (smooth != NULL) {

	fdsmooth = Rast_open_old(smooth, "");

	Rast_get_cellhd(smooth, "", &smhd);

	if ((winhd.ew_res != smhd.ew_res) || (winhd.ns_res != smhd.ns_res))
	    G_fatal_error(_("Map <%s> is the wrong resolution"), smooth);

	if (Rast_read_fp_range(smooth, "", &range) >= 0)
	    Rast_get_fp_range_min_max(&range, &cellmin, &cellmax);

	fcellmin = (float)cellmin;

	if (Rast_is_f_null_value(&fcellmin) || fcellmin < 0.0)
	    G_fatal_error(_("Smoothing values can not be negative or NULL"));
    }

    Rast_get_cellhd(input, "", &inphd);

    if ((winhd.ew_res != inphd.ew_res) || (winhd.ns_res != inphd.ns_res))
	G_fatal_error(_("Input map resolution differs from current region resolution!"));

    fdinp = Rast_open_old(input, "");

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
    if (sdisk > 1024) {
	if (sdisk > 1024 * 1024) {
	    if (sdisk > 1024 * 1024 * 1024) {
		G_message(_("%.2f GB of disk space for temp files."), sdisk / (1024. * 1024. * 1024.));
	    }
	    else
		G_message(_("%.2f MB of disk space for temp files."), sdisk / (1024. * 1024.));
	}
	else
	    G_message(_("%.2f KB of disk space for temp files."), sdisk / 1024.);
    }
    else
	G_message(_("%d bytes of disk space for temp files."), (int)sdisk);


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


    if (Rast_read_fp_range(input, "", &range) >= 0) {
	Rast_get_fp_range_min_max(&range, &cellmin, &cellmax);
    }
    else {
	cellrow = Rast_allocate_f_buf();
	for (m1 = 0; m1 < inp_rows; m1++) {
	    Rast_get_f_row(fdinp, cellrow, m1);
	    Rast_row_update_fp_range(cellrow, m1, &range, FCELL_TYPE);
	}
	Rast_get_fp_range_min_max(&range, &cellmin, &cellmax);
    }

    fcellmin = (float)cellmin;
    if (Rast_is_f_null_value(&fcellmin))
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
		      aspect, pcurv, tcurv, mcurv, dmin, inp_x_orig,
		      inp_y_orig, deriv, theta, scalex, Tmp_fd_z, Tmp_fd_dx,
		      Tmp_fd_dy, Tmp_fd_xx, Tmp_fd_yy, Tmp_fd_xy, NULL, NULL,
		      0, NULL);

    /*  In the above line, the penultimate argument is supposed to be a 
     * deviations file pointer.  None is obvious, so I used NULL. */
    /*  The 3rd and 4th argument are int-s, elatt and smatt (from the function
     * definition.  The value 1 seemed like a good placeholder...  or not. */

    IL_init_func_2d(&params, IL_grid_calc_2d, IL_matrix_create,
		    IL_check_at_points_2d,
		    IL_secpar_loop_2d, IL_crst, IL_crstg, IL_write_temp_2d);

    G_message(_("Temporarily changing the region to desired resolution ..."));
    Rast_set_window(&outhd);

    bitmask = IL_create_bitmask(&params);
    /* change region to initial region */
    G_message(_("Changing back to the original region ..."));
    Rast_set_window(&winhd);

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


    G_message(_("dnorm in mainc after grid before out1= %f"), dnorm);

    if (NPOINT < 0) {
	clean();
	G_fatal_error(_("split_and_interpolate() failed"));
    }

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
			      &dnorm, &outhd, &winhd, smooth, NPOINT) < 0) {
	clean();
	G_fatal_error(_("Unable to write raster maps -- try increasing cell size"));
    }

    G_free(zero_array_cell);
    clean();
    if (fd4)
	fclose(fd4);
    Rast_close(fdinp);
    if (smooth != NULL)
	Rast_close(fdsmooth);

    G_done_msg(" ");
    exit(EXIT_SUCCESS);
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
	G_fatal_error(_("Unable to open temporary file <%s>"), *tmpname);

    for (i = 0; i < nsizr; i++) {
	if (fwrite(zero_array_cell, sizeof(FCELL), nsizc, fp) != nsizc) {
	    clean();
	    G_fatal_error(_("Error writing temporary file <%s>"), *tmpname);
	}
    }

    return fp;
}

static void create_temp_files(void)
{
    zero_array_cell = (FCELL *) G_calloc(nsizc, sizeof(FCELL));

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

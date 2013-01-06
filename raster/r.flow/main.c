/*
 **  Original Algorithm:    H. Mitasova, L. Mitas, J. Hofierka, M. Zlocha 
 **  New GRASS Implementation:  J. Caplan, M. Ruesink  1995
 **
 **  US Army Construction Engineering Research Lab, University of Illinois 
 **
 **  Copyright  J. Caplan, H. Mitasova, L. Mitas, M.Ruesink, J. Hofierka, 
 **     M. Zlocha  1995
 **
 **This program is free software; you can redistribute it and/or
 **modify it under the terms of the GNU General Public License
 **as published by the Free Software Foundation; either version 2
 **of the License, or (at your option) any later version.
 **
 **This program is distributed in the hope that it will be useful,
 **but WITHOUT ANY WARRANTY; without even the implied warranty of
 **MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **GNU General Public License for more details.
 **
 **You should have received a copy of the GNU General Public License
 **along with this program; if not, write to the Free Software
 **Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 **
 **  version 13 for GRASS5.0
 **  FP related bugs in slope length output fixed by Helena oct. 1999)
 **  Update MN: commented line 387
 */

#include <stdlib.h>		/* for the random number generation */
#include <time.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include "r.flow.h"
#include "mem.h"
#include "io.h"
#include "aspect.h"
#include "precomp.h"

#define HORIZ	1		/* \            */
#define VERT	0		/* |            */
#define EAST	1		/* |            */
#define WEST	0		/* |_ magic     */
#define NORTH	1		/* |  numbers   */
#define SOUTH	0		/* |            */
#define ROW	1		/* |            */
#define COL	0		/* /            */

CELL v;				/* address for segment retrieval macros */

/* heap memory */
struct Cell_head region;	/* resolution and boundaries            */
struct Map_info fl;		/* output vector file header            */
struct BM *bitbar;		/* space-efficient barrier matrix       */
int lgfd;			/* output length file descriptor        */
char string[1024];		/* space for strings                    */
layer el, as, ds;		/* elevation, aspect, density (accumulation) */
double *ew_dist;		/* east-west distances for rows         */
double *epsilon[2];		/* quantization errors for rows         */

/* command-line parameters */
params parm;

typedef struct
{
    int row, col;		/* current matrix address       */
}
addr;

typedef int bbox[2][2];		/* current bounding box         */

typedef struct
{
    double x, y, z;		/* exact earth coordinates      */
    double theta;		/* aspect                       */
    double r, c;		/* cell matrix coordinates      */
}
point;

typedef struct
{
    double *px, *py;		/* x/y point arrays             */
    int index;			/* index of next point          */
}
flowline;

/***************************** CALCULATION ******************************/

/*
 * height_angle_bounding_box: averages matrix values at sub between
 *      floor(cut) and ceil(cut), based on proximity; adjusts bbox
 * globals r: o, z, parm
 * params  w: p, b
 */
static void
height_angle_bounding_box(int sub, double cut, int horiz, point * p, bbox b)
{
    int c, f = (int)cut;
    double r = cut - (double)f;
    double a1, a2, a, d;

    b[horiz][horiz] = sub - 1;
    b[horiz][!horiz] = sub + 1;
    b[!horiz][horiz] = f + 1;
    c = (b[!horiz][!horiz] = f - (r == 0.)) + 1;

    if (horiz) {
	a1 = (double)aspect(sub, f);
	a2 = (double)aspect(sub, c);
	p->z = (double)get(el, sub, f) * (1. - r) +
	    (double)get(el, sub, c) * r;
    }
    else {
	a1 = (double)aspect(f, sub);
	a2 = (double)aspect(c, sub);
	p->z = (double)get(el, f, sub) * (1. - r) +
	    (double)get(el, c, sub) * r;
    }

    if (!(a1 == UNDEF || a2 == UNDEF) &&
	!(Rast_is_d_null_value(&a1) || Rast_is_d_null_value(&a2)))
	/*    if (!(Rast_is_d_null_value(&a1) || Rast_is_d_null_value(&a2))) */
    {
	if ((d = a1 - a2) >= D_PI || d <= -D_PI) {
	    if (a2 > D_PI)
		a2 -= D2_PI;
	    else
		a1 -= D2_PI;
	}
	a = r * a2 + (1. - r) * a1;
	p->theta = a + (a < 0.) * D2_PI;
    }
    else
	p->theta = UNDEF;

    return;
}

/*
 * sloping: returns elevation condition for continuing current line
 */
#define sloping(z1, z2) (z1 > z2)

/*
 * on_map: returns map boundary condition for continuing current line
 * globals r:  region
 */
static int on_map(int sub, double cut, int horiz)
{
    return
	(sub >= 0 && cut >= 0.0 &&
	 ((horiz && sub < region.rows && cut <= (double)(region.cols - 1)) ||
	  (!horiz && sub < region.cols && cut <= (double)(region.rows - 1))));
}

/*
 * add_to_line: puts a new point on the end of the current flowline
 * globals r:  parm
 * params  w:  f
 */
static void add_to_line(point * p, flowline * f)
{
    if (parm.flout) {
	f->px[f->index] = (double)p->x;
	f->py[f->index] = (double)p->y;
    }
    ++f->index;
}

/*
 * rectify: correct quantization problems (designed for speed, not elegance)
 */
static double rectify(double delta, double bd[2], double e)
{
    if (delta > 0.) {
	if (delta > bd[1] + e)
	    return delta;
    }
    else {
	if (delta < bd[0] - e)
	    return delta;
    }
    if (delta < bd[1] - e)
	if (delta > bd[0] + e)
	    return delta;
	else
	    return bd[0];
    else
	return bd[1];
}

/*
 * next_point: computes next point based on current point, z, and o
 *      returns continuation condition
 * globals r:  region, bitbar, parm
 * params  w:  p, a, l
 * globals w:  density
 */
static int next_point(point * p,	/* current/next point               */
		      addr * a,	/* current/next matrix address      */
		      bbox b,	/* current/next bounding box        */
		      double *l	/* current/eventual length          */
    )
{
    int sub;
    double cut;
    int horiz;
    int semi;
    double length, delta;
    double deltaz;
    double tangent;

    double oldz = p->z;
    double oldtheta = p->theta;
    double oldr = p->r;
    double oldc = p->c;

    addr ads;
    double bdy[2], bdx[2];

    ads = *a;
    bdy[SOUTH] = (double)(oldr - b[ROW][SOUTH]) * region.ns_res;
    bdy[NORTH] = (double)(oldr - b[ROW][NORTH]) * region.ns_res;
    bdx[WEST] = (double)(b[COL][WEST] - oldc) * ew_dist[ads.row];
    bdx[EAST] = (double)(b[COL][EAST] - oldc) * ew_dist[ads.row];

    semi = oldtheta < 90 || oldtheta >= 270;
    tangent = tan(oldtheta * DEG2RAD);

    if (oldtheta != 90 && oldtheta != 270 &&	/* north/south */
	(delta = (bdy[semi] * tangent)) < bdx[EAST] && delta > bdx[WEST]) {
	delta = rectify(delta, bdx, epsilon[HORIZ][ads.row]);
	p->x += delta;
	p->y += bdy[semi];
	p->r = (double)b[ROW][semi];
	p->c += delta / ew_dist[ads.row];
	a->row = b[ROW][semi];
	a->col = ROUND(p->c);
	sub = b[ROW][semi];
	cut = p->c;
	horiz = HORIZ;
	if (parm.lgout)
	    length = hypot(delta, bdy[semi]);
    }
    else {			/*  east/west  */

	semi = oldtheta < 180;
	if (oldtheta == 90 || oldtheta == 270)
	    delta = 0;
	else {
	    /* I don't know if this is right case.
	     * Anyway, should be avoid from dividing by zero.
	     * Any hydrologic idea?
	     */
	    if (tangent == 0.0)
		tangent = 0.000001;

	    delta = bdx[semi] / tangent;
	}

	delta = rectify(delta, bdy, epsilon[VERT][ads.row]);
	p->y += delta;
	p->x += bdx[semi];
	p->r -= delta / region.ns_res;
	p->c = (double)b[COL][semi];
	a->row = ROUND(p->r);
	a->col = b[COL][semi];
	sub = b[COL][semi];
	cut = p->r;
	horiz = VERT;
	if (parm.lgout)
	    length = hypot(bdx[semi], delta);
    }

    if (on_map(sub, cut, horiz) &&
	(height_angle_bounding_box(sub, cut, horiz, p, b),
	 sloping(oldz, p->z)) &&
	!(parm.barin && BM_get(bitbar, a->col, a->row))) {
	if (parm.dsout && (ads.row != a->row || ads.col != a->col))
	    put(ds, a->row, a->col, get(ds, a->row, a->col) + 1);
	/*      if (parm.lgout) 
	 *      *l += parm.l3d ? hypot(length, oldz - p->z) : length; this did not work, helena*/
	if (parm.lgout) {
	    if (parm.l3d) {
		deltaz = oldz - p->z;	/*fix by helena Dec. 06 */
		*l += hypot(length, deltaz);
	    }
	    else
		*l += length;
	}
	return 1;
    }

    return 0;
}

/*
 * calculate: create a flowline for each cell
 * globals r: region, bitbar, parm, lgfd
 */
static void calculate(void)
{
    point pts;
    addr ads;
    bbox bbs;
    flowline fls;
    int row, col;

    /*    double     x, y, length, xstep, ystep, roffset, coffset; */
    double x, y, length, xstep, ystep;
    FCELL *lg = Rast_allocate_f_buf();
    struct line_pnts *points = Vect_new_line_struct();
    struct line_cats *cats = Vect_new_cats_struct();
    int loopstep = (!parm.dsout && !parm.lgout && parm.flout) ? parm.skip : 1;

    G_important_message(_("Calculating..."));

    fls.px = (double *)G_calloc(parm.bound, sizeof(double));
    fls.py = (double *)G_calloc(parm.bound, sizeof(double));

    ystep = region.ns_res * (double)loopstep;

    srand(time(0));

    for (row = 0, y = (double)region.north - (region.ns_res * .5);
	 row < region.rows; row += loopstep, y -= ystep) {
	xstep = ew_dist[row] * (double)loopstep;
	G_percent(row, region.rows, 2);

	for (col = 0, x = (double)region.west + (ew_dist[row] * .5);
	     col < region.cols; col += loopstep, x += xstep) {

	    length = 0.0;
	    fls.index = 0;

	    if (!(parm.barin && BM_get(bitbar, col, row))) {
#ifdef OFFSET
		/* disabled by helena June 2005 */
		roffset = parm.offset * (double)region.ew_res
		    * ((2. * (double)rand() / (double)RAND_MAX) - 1.);
		coffset = parm.offset * (double)region.ns_res
		    * ((2. * (double)rand() / (double)RAND_MAX) - 1.);
#endif
		pts.x = x;
		pts.y = y;
		pts.z = (double)get(el, row, col);
		pts.theta = (double)aspect(row, col);
		pts.r = (double)row;	/* + roffset; */
		pts.c = (double)col;	/*+ coffset; */

		ads.row = row;
		ads.col = col;

#ifdef OFFSET
		G_debug(3, "dx: %f  x: %f %f  row: %f %f\n",
			roffset, x, pts.x, (double)row, pts.r);
		G_debug(3, "dy: %f  y: %f %f  col: %f %f\n",
			roffset, y, pts.y, (double)col, pts.c);
#endif

		bbs[ROW][SOUTH] = row + 1;
		bbs[ROW][NORTH] = row - 1;
		bbs[COL][WEST] = col - 1;
		bbs[COL][EAST] = col + 1;

		do
		    add_to_line(&pts, &fls);
		while (fls.index <= parm.bound &&
		       (pts.z != UNDEFZ && pts.theta >= 0 && pts.theta <= 360)
		       &&
		       /*  (!Rast_is_d_null_value(&pts.z) && pts.theta != UNDEF) && */
		       next_point(&pts, &ads, bbs, &length));
	    }

	    if (fls.index > 1 && parm.flout &&
		(loopstep == parm.skip ||
		 !(row % parm.skip || col % parm.skip))) {
		Vect_copy_xyz_to_pnts(points, fls.px, fls.py, NULL,
				      fls.index);
		Vect_write_line(&fl, GV_LINE, points, cats);
	    }

	    if (parm.lgout)
		lg[col] = (float)length;
	}

	if (parm.lgout)
	    Rast_put_f_row(lgfd, lg);
    }
    G_percent (1, 1, 1);
    
    G_free(fls.px);
    G_free(fls.py);
    /*    G_free (fls); *//* commented 19/10/99 MN */
    G_free(lg);
    Vect_destroy_line_struct(points);
    Vect_destroy_cats_struct(cats);

    if (parm.lgout)
	Rast_close(lgfd);
}

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct Option *pelevin, *paspin, *pbarin, *pskip, *pbound,
	*pflout, *plgout, *pdsout;
    struct Flag *fup, *flg, *fmem;
    int default_skip, larger, default_bound;

#ifdef OFFSET
    char *default_offset_ans, *offset_opt;
#endif
    char *default_skip_ans, *default_bound_ans, *skip_opt;
    struct History history;


    /* Initialize GIS engine */
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("hydrology"));
    module->label = _("Constructs flowlines.");
    module->description =
	_("Computes flowlines, flowpath lengths, "
	  "and flowaccumulation (contributing areas) from a elevation raster "
	  "map.");

    pelevin = G_define_standard_option(G_OPT_R_ELEV);
    
    paspin = G_define_standard_option(G_OPT_R_INPUT);
    paspin->key = "aspect";
    paspin->required = NO;
    paspin->description = _("Name of input aspect raster map");

    pbarin = G_define_standard_option(G_OPT_R_INPUT);
    pbarin->key = "barrier";
    pbarin->required = NO;
    pbarin->description = _("Name of input barrier raster map");

    pskip = G_define_option();
    pskip->key = "skip";
    pskip->type = TYPE_INTEGER;
    pskip->required = NO;
    pskip->description = _("Number of cells between flowlines");

    pbound = G_define_option();
    pbound->key = "bound";
    pbound->type = TYPE_INTEGER;
    pbound->required = NO;
    pbound->description = _("Maximum number of segments per flowline");

    pflout = G_define_standard_option(G_OPT_V_OUTPUT);
    pflout->key = "flowline";
    pflout->required = NO;
    pflout->description = _("Name for output flowline vector map");

    plgout = G_define_standard_option(G_OPT_R_OUTPUT);
    plgout->key = "flowlength";
    plgout->required = NO;
    plgout->description = _("Name for output flowpath length raster map");

    pdsout = G_define_standard_option(G_OPT_R_OUTPUT);
    pdsout->key = "flowaccumulation";
    pdsout->required = NO;
    pdsout->description = _("Name for output flowaccumulation raster map");

    fup = G_define_flag();
    fup->key = 'u';
    fup->description =
	_("Compute upslope flowlines instead of default downhill flowlines");

    flg = G_define_flag();
    flg->key = '3';
    flg->description = _("3D lengths instead of 2D");

    fmem = G_define_flag();
    fmem->key = 'm';
    fmem->description = _("Use less memory, at a performance penalty");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    G_get_set_window(&region);

    larger = ((region.cols < region.rows) ? region.rows : region.cols);
    default_skip = (larger < 50) ? 1 : (int)(larger / 50);

    default_skip_ans =
	G_calloc((int)log10((double)default_skip) + 2, sizeof(char));
    skip_opt = G_calloc((int)log10((double)larger) + 4, sizeof(char));

    sprintf(default_skip_ans, "%d", default_skip);
    sprintf(skip_opt, "1-%d", larger);

    default_bound = (int)(4. * hypot((double)region.rows,
				     (double)region.cols));
    default_bound_ans =
	G_calloc((int)log10((double)default_bound) + 4, sizeof(char));
    sprintf(default_bound_ans, "0-%d", default_bound);

#ifdef OFFSET
    /* below fix changed from 0.0 to 1.0 and its effect disabled in 
     * calc.c, Helena June 2005 */

    default_offset = 1.0;	/* fixed 20. May 2001 Helena */
    default_offset_ans =
	G_calloc((int)log10(default_offset) + 2, sizeof(char));
    sprintf(default_offset_ans, "%f", default_offset);

    offset_opt = G_calloc((int)log10(default_offset) + 4, sizeof(char));
    sprintf(offset_opt, "0.0-500.0");
#endif

    if (!pskip->answer)
	pskip->answer = default_skip_ans;

    if (!pbound->answer)
	pbound->answer = default_bound_ans + 2;

    parm.elevin = pelevin->answer;
    parm.aspin = paspin->answer;
    parm.barin = pbarin->answer;
    parm.skip = atoi(pskip->answer);
    parm.bound = atoi(pbound->answer);
    parm.flout = pflout->answer;
    parm.lgout = plgout->answer;
    parm.dsout = pdsout->answer;
    parm.up = fup->answer;
    parm.l3d = flg->answer;
    parm.mem = fmem->answer;

    if (!pflout->answer && !plgout->answer && !pdsout->answer)
	G_fatal_error(_("You must select one or more output maps (flout, lgout, dsout)"));

    if (parm.seg)
	parm.mem = '\0';
    else if (parm.mem)
	parm.aspin = NULL;

    el.name = parm.elevin;
    as.name = (parm.aspin) ? parm.aspin : "internal aspects";

    ds.name = parm.dsout;
    el.row_offset = el.col_offset = 1;
    as.row_offset = as.col_offset = 0;
    ds.row_offset = ds.col_offset = 0;

    if ((G_projection() == PROJECTION_LL))	/* added MN 2005 */
	G_fatal_error(_("lat/long projection not supported by "
			"r.flow. Please use 'r.watershed' for calculating "
			"flow accumulation."));

    if (parm.flout || parm.dsout || parm.lgout) {
	open_output_files();
	allocate_heap();
	read_input_files();

	precompute();
	calculate();
	if (parm.dsout)
	    write_density_file();

	close_files();
	deallocate_heap();
    }

    if (parm.dsout) {
	Rast_short_history(parm.dsout, "raster", &history);
	Rast_command_history(&history);
	Rast_write_history(parm.dsout, &history);
    }
    if (parm.lgout) {
	Rast_short_history(parm.lgout, "raster", &history);
	Rast_command_history(&history);
	Rast_write_history(parm.lgout, &history);
    }

    exit(EXIT_SUCCESS);
}

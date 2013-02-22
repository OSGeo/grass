/*******************************************************************************
r.horizon: This module does one of two things:

1) Using a map of the terrain elevation it calculates for a set of points 
the angle height of the horizon for each point, using an angle interval given
by the user.

2) For a given minimum angle it calculates one or more raster map giving the mazimum
distance to a point on the horizon.  

This program was written in 2006 by Tfomas Huld and Tomas Cebecauer, 
Joint Research Centre of the European Commission, based on bits of the r.sun module by Jaro Hofierka

*******************************************************************************/
/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/gprojects.h>
#include <grass/glocale.h>

#define WHOLE_RASTER 1
#define SINGLE_POINT 0
#define RAD      (180. / M_PI)
#define DEG      ((M_PI)/180.)
#define EARTHRADIUS 6371000.
#define UNDEF    0.		/* undefined value for terrain aspect */
#define UNDEFZ   -9999.		/* undefined value for elevation */
#define BIG      1.e20
#define SMALL    1.e-20
#define EPS      1.e-4
#define DIST     "1.0"
#define DEGREEINMETERS 111120.	/* 1852m/nm * 60nm/degree = 111120 m/deg */
#define TANMINANGLE 0.008727	/* tan of minimum horizon angle (0.5 deg) */

#define AMAX1(arg1, arg2) ((arg1) >= (arg2) ? (arg1) : (arg2))
#define DISTANCE1(x1, x2, y1, y2) (sqrt((x1 - x2)*(x1 - x2) + (y1 - y2)*(y1 - y2)))


FILE *fw;

const double rad2deg = 180. / M_PI;
const double deg2rad = M_PI / 180.;
const double pihalf = M_PI * 0.5;
const double twopi = 2. * M_PI;
const double invEarth = 1. / EARTHRADIUS;

const double minAngle = DEG;

const char *elevin;
const char *latin = NULL;
const char *horizon = NULL;
const char *mapset = NULL;
const char *per;
char shad_filename[GNAME_MAX];

struct Cell_head cellhd;
struct Key_value *in_proj_info, *in_unit_info;
struct pj_info iproj;
struct pj_info oproj;

struct Cell_head new_cellhd;
double bufferZone = 0., ebufferZone = 0., wbufferZone = 0.,
       nbufferZone = 0., sbufferZone = 0.;

int INPUT(void);
int OUTGR(int numrows, int numcols);
double amax1(double, double);
double amin1(double, double);
int min(int, int);
int max(int, int);
void com_par(double angle);
int is_shadow(void);
double horizon_height(void);
void calculate_shadow();
double calculate_shadow_onedirection(double shadow_angle);

int new_point();
double searching();
int test_low_res();

/*void where_is_point();
   void cube(int, int);
 */

void calculate(double xcoord, double ycoord, int buffer_e, int buffer_w,
	       int buffer_s, int buffer_n);


int ip, jp, ip100, jp100;
int n, m, m100, n100;
int degreeOutput = FALSE;
float **z, **z100, **horizon_raster;
double stepx, stepy, stepxhalf, stepyhalf, stepxy, xp, yp, op, dp, xg0, xx0,
    yg0, yy0, deltx, delty;
double invstepx, invstepy, distxy;
double offsetx, offsety;
double single_direction;

/*int arrayNumInt; */
double xmin, xmax, ymin, ymax, zmax = 0.;
int d, day, tien = 0;

double length, maxlength = BIG, zmult = 1.0, step = 0.0, dist;
double fixedMaxLength = BIG;
char *tt, *lt;
double z_orig, zp;
double h0, tanh0, angle;
double stepsinangle, stepcosangle, sinangle, cosangle, distsinangle,
    distcosangle;
double TOLER;

int mode;
int isMode()
{
    return mode;
}
void setMode(int val)
{
    mode = val;
}

int ll_correction = FALSE;
double coslatsq;

/* use G_distance() instead ??!?! */
double distance(double x1, double x2, double y1, double y2)
{
    if (ll_correction) {
	return DEGREEINMETERS * sqrt(coslatsq * (x1 - x2) * (x1 - x2)
				     + (y1 - y2) * (y1 - y2));
    }
    else {
	return sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
    }
}



int main(int argc, char *argv[])
{
    double xcoord, ycoord;

    struct GModule *module;
    struct
    {
	struct Option *elevin, *dist, *coord, *direction, *horizon, *step,
	    *bufferzone, *e_buff, *w_buff, *n_buff, *s_buff, *maxdistance;
    } parm;

    struct
    {
	struct Flag *degreeOutput;
    }
    flag;

    G_gisinit(argv[0]);
    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("solar"));
    G_add_keyword(_("sun position"));
    module->label =
	_("Horizon angle computation from a digital elevation model.");
    module->description =
	_("Computes horizon angle height from a digital elevation model. The module has two"
	 " different modes of operation:  "
	 "1. Computes the entire horizon around a single point whose coordinates are"
	 " given with the 'coord' option. The horizon height (in radians). "
	 "2. Computes one or more raster maps of the horizon height in a single direction. "
	 " The input for this is the angle (in degrees), which is measured "
	 " counterclockwise with east=0, north=90 etc. The output is the horizon height in radians.");

    parm.elevin = G_define_option();
    parm.elevin->key = "elevin";
    parm.elevin->type = TYPE_STRING;
    parm.elevin->required = YES;
    parm.elevin->gisprompt = "old,cell,raster";
    parm.elevin->description =
	_("Name of the input elevation raster map [meters]");
    parm.elevin->guisection = _("Input options");


    parm.direction = G_define_option();
    parm.direction->key = "direction";
    parm.direction->type = TYPE_DOUBLE;
    parm.direction->required = NO;
    parm.direction->description =
	_("Direction in which you want to know the horizon height");
    parm.direction->guisection = _("Input options");

    parm.step = G_define_option();
    parm.step->key = "horizonstep";
    parm.step->type = TYPE_DOUBLE;
    parm.step->required = NO;
    parm.step->description =
	_("Angle step size for multidirectional horizon [degrees]");
    parm.step->guisection = _("Input options");

    parm.bufferzone = G_define_option();
    parm.bufferzone->key = "bufferzone";
    parm.bufferzone->type = TYPE_DOUBLE;
    parm.bufferzone->required = NO;
    parm.bufferzone->description =
	_("For horizon rasters, read from the DEM an extra buffer around the present region");
    parm.bufferzone->guisection = _("Input options");

    parm.e_buff = G_define_option();
    parm.e_buff->key = "e_buff";
    parm.e_buff->type = TYPE_DOUBLE;
    parm.e_buff->required = NO;
    parm.e_buff->description =
	_("For horizon rasters, read from the DEM an extra buffer eastward the present region");
    parm.e_buff->guisection = _("Input options");

    parm.w_buff = G_define_option();
    parm.w_buff->key = "w_buff";
    parm.w_buff->type = TYPE_DOUBLE;
    parm.w_buff->required = NO;
    parm.w_buff->description =
	_("For horizon rasters, read from the DEM an extra buffer westward the present region");
    parm.w_buff->guisection = _("Input options");

    parm.n_buff = G_define_option();
    parm.n_buff->key = "n_buff";
    parm.n_buff->type = TYPE_DOUBLE;
    parm.n_buff->required = NO;
    parm.n_buff->description =
	_("For horizon rasters, read from the DEM an extra buffer northward the present region");
    parm.n_buff->guisection = _("Input options");

    parm.s_buff = G_define_option();
    parm.s_buff->key = "s_buff";
    parm.s_buff->type = TYPE_DOUBLE;
    parm.s_buff->required = NO;
    parm.s_buff->description =
	_("For horizon rasters, read from the DEM an extra buffer southward the present region");
    parm.s_buff->guisection = _("Input options");

    parm.maxdistance = G_define_option();
    parm.maxdistance->key = "maxdistance";
    parm.maxdistance->type = TYPE_DOUBLE;
    parm.maxdistance->required = NO;
    parm.maxdistance->description =
	_("The maximum distance to consider when finding the horizon height");
    parm.maxdistance->guisection = _("Input options");


    parm.horizon = G_define_option();
    parm.horizon->key = "horizon";
    parm.horizon->type = TYPE_STRING;
    parm.horizon->required = NO;
    parm.horizon->gisprompt = "old,cell,raster";
    parm.horizon->description = _("Prefix of the horizon raster output maps");
    parm.horizon->guisection = _("Output options");


    parm.coord = G_define_option();
    parm.coord->key = "coord";
    parm.coord->type = TYPE_DOUBLE;
    parm.coord->key_desc = "east,north";
    parm.coord->required = NO;
    parm.coord->description =
	_("Coordinate for which you want to calculate the horizon");
    parm.coord->guisection = _("Output options");

    parm.dist = G_define_option();
    parm.dist->key = "dist";
    parm.dist->type = TYPE_DOUBLE;
    parm.dist->answer = DIST;
    parm.dist->required = NO;
    parm.dist->description = _("Sampling distance step coefficient (0.5-1.5)");
    parm.dist->guisection = _("Output options");


    flag.degreeOutput = G_define_flag();
    flag.degreeOutput->key = 'd';
    flag.degreeOutput->description =
	_("Write output in degrees (default is radians)");


    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    G_get_set_window(&cellhd);

    stepx = cellhd.ew_res;
    stepy = cellhd.ns_res;
    stepxhalf = stepx / 2.;
    stepyhalf = stepy / 2.;
    invstepx = 1. / stepx;
    invstepy = 1. / stepy;
    /*
       offsetx = 2. *  invstepx;
       offsety = 2. * invstepy;
       offsetx = 0.5*stepx;
       offsety = 0.5*stepy;
     */
    offsetx = 0.5;
    offsety = 0.5;

    n /*n_cols */  = cellhd.cols;
    m /*n_rows */  = cellhd.rows;

    n100 = ceil(n / 100.);
    m100 = ceil(m / 100.);

    xmin = cellhd.west;
    ymin = cellhd.south;
    xmax = cellhd.east;
    ymax = cellhd.north;
    deltx = fabs(cellhd.east - cellhd.west);
    delty = fabs(cellhd.north - cellhd.south);

    degreeOutput = flag.degreeOutput->answer;


    elevin = parm.elevin->answer;

    if (parm.coord->answer == NULL) {
	setMode(WHOLE_RASTER);
    }
    else {
	setMode(SINGLE_POINT);
	if (sscanf(parm.coord->answer, "%lf,%lf", &xcoord, &ycoord) != 2) {
	    G_fatal_error(
		_("Can't read the coordinates from the \"coord\" option."));
	}

	/* Transform the coordinates to row/column */

	/*
	   xcoord = (int) ((xcoord-xmin)/stepx);
	   ycoord = (int) ((ycoord-ymin)/stepy);
	 */
    }

    if (parm.direction->answer != NULL)
	sscanf(parm.direction->answer, "%lf", &single_direction);


    if (isMode(WHOLE_RASTER)) {
	if ((parm.direction->answer == NULL) && (parm.step->answer == NULL)) {
	    G_fatal_error(
		_("You didn't specify a direction value or step size. Aborting."));
	}

	if (parm.horizon->answer == NULL) {
	    G_fatal_error(
		_("You didn't specify a horizon raster name. Aborting."));
	}
	horizon = parm.horizon->answer;
	if (parm.step->answer != NULL)
	    sscanf(parm.step->answer, "%lf", &step);
    }
    else {

	if (parm.step->answer == NULL) {
	    G_fatal_error(
		_("You didn't specify an angle step size. Aborting."));
	}
	sscanf(parm.step->answer, "%lf", &step);


    }

    if (step == 0.0) {
	step = 360.;
    }

    if (parm.bufferzone->answer != NULL) {
	if (sscanf(parm.bufferzone->answer, "%lf", &bufferZone) != 1)
	    G_fatal_error(_("Could not read bufferzone size. Aborting."));
    }

    if (parm.e_buff->answer != NULL) {
	if (sscanf(parm.e_buff->answer, "%lf", &ebufferZone) != 1)
	    G_fatal_error(_("Could not read %s bufferzone size. Aborting."),
			  _("east"));
    }

    if (parm.w_buff->answer != NULL) {
	if (sscanf(parm.w_buff->answer, "%lf", &wbufferZone) != 1)
	    G_fatal_error(_("Could not read %s bufferzone size. Aborting."),
			  _("west"));
    }

    if (parm.s_buff->answer != NULL) {
	if (sscanf(parm.s_buff->answer, "%lf", &sbufferZone) != 1)
	    G_fatal_error(
		_("Could not read %s bufferzone size. Aborting."),
		_("south"));
    }

    if (parm.n_buff->answer != NULL) {
	if (sscanf(parm.n_buff->answer, "%lf", &nbufferZone) != 1)
	    G_fatal_error(
		_("Could not read %s bufferzone size. Aborting."),
		_("north"));
    }

    if (parm.maxdistance->answer != NULL) {
	if (sscanf(parm.maxdistance->answer, "%lf", &fixedMaxLength) != 1)
	    G_fatal_error(_("Could not read maximum distance. Aborting."));
    }


    sscanf(parm.dist->answer, "%lf", &dist);

    stepxy = dist * 0.5 * (stepx + stepy);
    distxy = dist;
    TOLER = stepxy * EPS;

    if (bufferZone > 0. || ebufferZone > 0. || wbufferZone > 0. ||
	sbufferZone > 0. || nbufferZone > 0.) {
	new_cellhd = cellhd;

	if (ebufferZone == 0.)
	    ebufferZone = bufferZone;
	if (wbufferZone == 0.)
	    wbufferZone = bufferZone;
	if (sbufferZone == 0.)
	    sbufferZone = bufferZone;
	if (nbufferZone == 0.)
	    nbufferZone = bufferZone;

	new_cellhd.rows += (int)((nbufferZone + sbufferZone) / stepy);
	new_cellhd.cols += (int)((ebufferZone + wbufferZone) / stepx);

	new_cellhd.north += nbufferZone;
	new_cellhd.south -= sbufferZone;
	new_cellhd.east += ebufferZone;
	new_cellhd.west -= wbufferZone;

	xmin = new_cellhd.west;
	ymin = new_cellhd.south;
	xmax = new_cellhd.east;
	ymax = new_cellhd.north;
	deltx = fabs(new_cellhd.east - new_cellhd.west);
	delty = fabs(new_cellhd.north - new_cellhd.south);

	n /* n_cols */ = new_cellhd.cols;
	m /* n_rows */ = new_cellhd.rows;
	/* G_debug(3,"%lf %lf %lf %lf \n",ymax, ymin, xmin,xmax); */
	n100 = ceil(n / 100.);
	m100 = ceil(m / 100.);

	Rast_set_window(&new_cellhd);
    }

    struct Key_Value *in_proj_info, *in_unit_info;

    if ((in_proj_info = G_get_projinfo()) == NULL)
	G_fatal_error(
	    _("Can't get projection info of current location: "
	      "please set latitude via 'lat' or 'latin' option!"));

    if ((in_unit_info = G_get_projunits()) == NULL)
	G_fatal_error(_("Can't get projection units of current location"));

    if (pj_get_kv(&iproj, in_proj_info, in_unit_info) < 0)
	G_fatal_error(
	    _("Can't get projection key values of current location"));

    G_free_key_value(in_proj_info);
    G_free_key_value(in_unit_info);

    /* Set output projection to latlong w/ same ellipsoid */
    oproj.zone = 0;
    oproj.meters = 1.;
    sprintf(oproj.proj, "ll");
    if ((oproj.pj = pj_latlong_from_proj(iproj.pj)) == NULL)
	G_fatal_error(_("Unable to set up lat/long projection parameters"));


/**********end of parser - ******************************/



    INPUT();
    calculate(xcoord, ycoord, (int)(ebufferZone / stepx),
	      (int)(wbufferZone / stepx), (int)(sbufferZone / stepy),
	      (int)(nbufferZone / stepy));

    exit(EXIT_SUCCESS);
}

/**********************end of main.c*****************/


int INPUT(void)
{
    FCELL *cell1;
    int fd1, row, row_rev;
    int l, i, j, k;
    int lmax, kmax;

    cell1 = Rast_allocate_f_buf();

    z = (float **)G_malloc(sizeof(float *) * (m));
    z100 = (float **)G_malloc(sizeof(float *) * (m100));

    for (l = 0; l < m; l++) {
	z[l] = (float *)G_malloc(sizeof(float) * (n));
    }
    for (l = 0; l < m100; l++) {
	z100[l] = (float *)G_malloc(sizeof(float) * (n100));
    }
    /*read Z raster */

    fd1 = Rast_open_old(elevin, "");

    for (row = 0; row < m; row++) {
	Rast_get_f_row(fd1, cell1, row);

	for (j = 0; j < n; j++) {
	    row_rev = m - row - 1;

	    if (!Rast_is_f_null_value(cell1 + j))
		z[row_rev][j] = (float)cell1[j];
	    else
		z[row_rev][j] = UNDEFZ;

	}
    }
    Rast_close(fd1);

    /* create low resolution array 100 */
    for (i = 0; i < m100; i++) {
	lmax = (i + 1) * 100;
	if (lmax > m)
	    lmax = m;

	for (j = 0; j < n100; j++) {
	    zmax = SMALL;
	    kmax = (j + 1) * 100;
	    if (kmax > n)
		kmax = n;
	    for (l = (i * 100); l < lmax; l++) {
		for (k = (j * 100); k < kmax; k++) {
		    zmax = amax1(zmax, z[l][k]);
		}
	    }
	    z100[i][j] = zmax;
	    /* G_debug(3,"%d %d %lf\n", i, j, z100[i][j]); */
	}
    }


    /* find max Z */
    for (i = 0; i < m; i++) {
	for (j = 0; j < n; j++) {
	    zmax = amax1(zmax, z[i][j]);
	}
    }

    return 1;
}




int OUTGR(int numrows, int numcols)
{
    FCELL *cell1 = NULL;

    int fd1 = 0;
    int i, iarc, j;

    Rast_set_window(&cellhd);

    if (horizon != NULL) {
	cell1 = Rast_allocate_f_buf();
	fd1 = Rast_open_fp_new(shad_filename);
    }

    if (numrows != Rast_window_rows())
	G_fatal_error(_("OOPS: rows changed from %d to %d"), numrows,
		      Rast_window_rows());

    if (numcols != Rast_window_cols())
	G_fatal_error(_("OOPS: cols changed from %d to %d"), numcols,
		      Rast_window_cols());

    for (iarc = 0; iarc < numrows; iarc++) {
	i = numrows - iarc - 1;

	if (horizon != NULL) {
	    for (j = 0; j < numcols; j++) {
		if (horizon_raster[i][j] == UNDEFZ)
		    Rast_set_f_null_value(cell1 + j, 1);
		else
		    cell1[j] = (FCELL) horizon_raster[i][j];
	    }
	    Rast_put_f_row(fd1, cell1);
	}
    }				/* End loop over rows. */

    Rast_close(fd1);

    return 1;
}


double amax1(arg1, arg2)
     double arg1;
     double arg2;
{
    double res;

    if (arg1 >= arg2) {
	res = arg1;
    }
    else {
	res = arg2;
    }
    return res;
}

double amin1(arg1, arg2)
     double arg1;
     double arg2;
{
    double res;

    if (arg1 <= arg2) {
	res = arg1;
    }
    else {
	res = arg2;
    }
    return res;
}


int min(arg1, arg2)
     int arg1;
     int arg2;
{
    int res;

    if (arg1 <= arg2) {
	res = arg1;
    }
    else {
	res = arg2;
    }
    return res;
}

int max(arg1, arg2)
     int arg1;
     int arg2;
{
    int res;

    if (arg1 >= arg2) {
	res = arg1;
    }
    else {
	res = arg2;
    }
    return res;
}



/**********************************************************/

void com_par(double angle)
{
    sinangle = sin(angle);
    if (fabs(sinangle) < 0.0000001) {
	sinangle = 0.;
    }
    cosangle = cos(angle);
    if (fabs(cosangle) < 0.0000001) {
	cosangle = 0.;
    }
    distsinangle = 32000;
    distcosangle = 32000;

    if (sinangle != 0.) {
	distsinangle = 100. / (distxy * sinangle);
    }
    if (cosangle != 0.) {
	distcosangle = 100. / (distxy * cosangle);
    }

    stepsinangle = stepxy * sinangle;
    stepcosangle = stepxy * cosangle;
}

double horizon_height(void)
{
    double height;

    tanh0 = 0.;
    length = 0;

    height = searching();

    xx0 = xg0;
    yy0 = yg0;

    return height;
}


double calculate_shadow_onedirection(double shadow_angle)
{
    shadow_angle = horizon_height();

    return shadow_angle;
}



void calculate_shadow()
{
    double dfr_rad;

    int i;
    int printCount;
    double shadow_angle;
    double printangle;
    double sx, sy;
    double xp, yp;
    double latitude, longitude;
    double delt_lat, delt_lon;
    double delt_east, delt_nor;
    double delt_dist;

    double angle;

    printCount = 360. / fabs(step);


    if (printCount < 1)
	printCount = 1;


    dfr_rad = step * deg2rad;

    xp = xmin + xx0;
    yp = ymin + yy0;

    angle = (single_direction * deg2rad) + pihalf;


    maxlength = fixedMaxLength;

    for (i = 0; i < printCount; i++) {

	ip = jp = 0;


	sx = xx0 * invstepx;
	sy = yy0 * invstepy;
	ip100 = floor(sx / 100.);
	jp100 = floor(sy / 100.);


	if ((G_projection() != PROJECTION_LL)) {

	    longitude = xp;
	    latitude = yp;

	    if (pj_do_proj(&longitude, &latitude, &iproj, &oproj) < 0) {
		G_fatal_error(_("Error in pj_do_proj"));
	    }
	}
	else {			/* ll projection */
	    latitude = yp;
	    longitude = xp;
	}
	latitude *= deg2rad;
	longitude *= deg2rad;


	delt_lat = -0.0001 * cos(angle);
	delt_lon = 0.0001 * sin(angle) / cos(latitude);

	latitude = (latitude + delt_lat) * rad2deg;
	longitude = (longitude + delt_lon) * rad2deg;

	if (pj_do_proj(&longitude, &latitude, &oproj, &iproj) < 0) {
	    G_fatal_error(_("Error in pj_do_proj"));
	}

	delt_east = longitude - xp;
	delt_nor = latitude - yp;

	delt_dist = sqrt(delt_east * delt_east + delt_nor * delt_nor);

	stepsinangle = stepxy * delt_nor / delt_dist;
	stepcosangle = stepxy * delt_east / delt_dist;


	shadow_angle = horizon_height();


	if (degreeOutput) {
	    shadow_angle *= rad2deg;
	}
	printangle = angle * rad2deg - 90.;
	if (printangle < 0.)
	    printangle += 360;
	else if (printangle >= 360.)
	    printangle -= 360;

	G_message("%lf, %lf", printangle, shadow_angle);

	angle += dfr_rad;

	if (angle < 0.)
	    angle += twopi;
	else if (angle > twopi)
	    angle -= twopi;
    }				/* end of for loop over angles */
}

/*////////////////////////////////////////////////////////////////////// */


int new_point()
{
    int iold, jold;
    int succes = 1, succes2 = 1;
    double sx, sy;
    double dx, dy;

    iold = ip;
    jold = jp;

    while (succes) {
	yy0 += stepsinangle;
	xx0 += stepcosangle;


	/* offset 0.5 cell size to get the right cell i, j */
	sx = xx0 * invstepx + offsetx;
	sy = yy0 * invstepy + offsety;
	ip = (int)sx;
	jp = (int)sy;

	/* test outside of raster */
	if ((ip < 0) || (ip >= n) || (jp < 0) || (jp >= m))
	    return (3);

	if ((ip != iold) || (jp != jold)) {
	    dx = (double)ip *stepx;
	    dy = (double)jp *stepy;

	    length = distance(xg0, dx, yg0, dy);  /* dist from orig. grid point to the current grid point */
	    succes2 = test_low_res();
	    if (succes2 == 1) {
		zp = z[jp][ip];
		return (1);
	    }
	}
    }
    return -1;
}


int test_low_res()
{
    int iold100, jold100;
    double sx, sy;
    int delx, dely, mindel;
    double zp100, z2, curvature_diff;

    iold100 = ip100;
    jold100 = jp100;
    ip100 = floor(ip / 100.);
    jp100 = floor(jp / 100.);
    /*test the new position with low resolution */
    if ((ip100 != iold100) || (jp100 != jold100)) {
	/* G_debug(3,"%d %d %d %d\n",ip,jp, iold100,jold100); */
	/*  replace with approximate version
	   curvature_diff = EARTHRADIUS*(1.-cos(length/EARTHRADIUS));
	 */
	curvature_diff = 0.5 * length * length * invEarth;
	z2 = z_orig + curvature_diff + length * tanh0;
	zp100 = z100[jp100][ip100];
	/*G_debug(3,"%d %d %lf %lf \n",ip,jp,z2,zp100); */

	if (zp100 <= z2)
	    /*skip to the next lowres cell */
	{
	    delx = 32000;
	    dely = 32000;
	    if (cosangle > 0.) {
		sx = xx0 * invstepx + offsetx;
		delx =
		    floor(fabs
			  ((ceil(sx / 100.) - (sx / 100.)) * distcosangle));
	    }
	    if (cosangle < 0.) {
		sx = xx0 * invstepx + offsetx;
		delx =
		    floor(fabs
			  ((floor(sx / 100.) - (sx / 100.)) * distcosangle));
	    }
	    if (sinangle > 0.) {
		sy = yy0 * invstepy + offsety;
		dely =
		    floor(fabs
			  ((ceil(sy / 100.) - (sy / 100.)) * distsinangle));
	    }
	    else if (sinangle < 0.) {
		sy = yy0 * invstepy + offsety;
		dely =
		    floor(fabs
			  ((floor(jp / 100.) - (sy / 100.)) * distsinangle));
	    }

	    mindel = min(delx, dely);
	    /* G_debug(3,"%d %d %d %lf %lf\n",ip, jp, mindel,xg0, yg0);*/

	    yy0 = yy0 + (mindel * stepsinangle);
	    xx0 = xx0 + (mindel * stepcosangle);
	    /* G_debug(3,"  %lf %lf\n",xx0,yy0);*/

	    return (3);
	}
	else {
	    return (1);	/* change of low res array - new cell is reaching limit for high resolution processing */
	}
    }
    else {
	return (1);	/* no change of low res array */
    }
}


double searching()
{
    double z2;
    double curvature_diff;
    int succes = 1;

    if (zp == UNDEFZ)
	return 0;

    while (1) {
	succes = new_point();

	if (succes != 1) {
	    break;
	}

	/* curvature_diff = EARTHRADIUS*(1.-cos(length/EARTHRADIUS)); */
	curvature_diff = 0.5 * length * length * invEarth;

	z2 = z_orig + curvature_diff + length * tanh0;

	if (z2 < zp) {
	    tanh0 = (zp - z_orig - curvature_diff) / length;
	}


	if (z2 >= zmax) {
	    break;
	}

	if (length >= maxlength) {
	    break;
	}

    }

    return atan(tanh0);
}



/*////////////////////////////////////////////////////////////////////// */

void calculate(double xcoord, double ycoord, int buffer_e, int buffer_w,
	       int buffer_s, int buffer_n)
{
    int i, j, l, k = 0;
    int numDigits;

    int xindex, yindex;
    double shadow_angle;
    double coslat;

    double latitude, longitude;
    double xp, yp;
    double inputAngle;
    double delt_lat, delt_lon;
    double delt_east, delt_nor;
    double delt_dist;

    char formatString[10];
    char msg_buff[256];

    int hor_row_start = buffer_s;
    int hor_row_end = m - buffer_n;

    int hor_col_start = buffer_w;
    int hor_col_end = n - buffer_e;

    int hor_numrows = m - (buffer_s + buffer_n);
    int hor_numcols = n - (buffer_e + buffer_w);

    int arrayNumInt;
    double dfr_rad;

    xindex = (int)((xcoord - xmin) / stepx);
    yindex = (int)((ycoord - ymin) / stepy);

    if ((G_projection() == PROJECTION_LL)) {
	ll_correction = TRUE;
    }


    if (isMode() == SINGLE_POINT) {
	/* Calculate the horizon for one single point */

	/* 
	   xg0 = xx0 = (double)xcoord * stepx;
	   yg0 = yy0 = (double)ycoord * stepy;
	   xg0 = xx0 = xcoord -0.5*stepx -xmin;
	   yg0 = yy0 = ycoord -0.5*stepy-ymin;
	   xg0 = xx0 = xindex*stepx -0.5*stepx;
	   yg0 = yy0 = yindex*stepy -0.5*stepy;
	 */
	xg0 = xx0 = xindex * stepx;
	yg0 = yy0 = yindex * stepy;


	if (ll_correction) {
	    coslat = cos(deg2rad * (ymin + yy0));
	    coslatsq = coslat * coslat;
	}

	G_debug(3, "yindex: %d, xindex %d", yindex, xindex);
	z_orig = zp = z[yindex][xindex];

	calculate_shadow();

    }
    else {

	/****************************************************************/
	/*  The loop over raster points starts here!                    */
	/****************************************************************/

	if (horizon != NULL) {
	    horizon_raster = (float **)G_malloc(sizeof(float *) * (hor_numrows));
	    for (l = 0; l < hor_numrows; l++) {
		horizon_raster[l] =
		    (float *)G_malloc(sizeof(float) * (hor_numcols));
	    }

	    for (j = 0; j < hor_numrows; j++) {
		for (i = 0; i < hor_numcols; i++)
		    horizon_raster[j][i] = 0.;
	    }
	}

	/* definition of horizon angle in loop */
	if (step == 0.0) {
	    dfr_rad = 0;
	    arrayNumInt = 1;
	    sprintf(shad_filename, "%s", horizon);
	}
	else {
	    dfr_rad = step * deg2rad;
	    arrayNumInt = (int)(360. / fabs(step));
	}

	numDigits = (int)(log10(1. * arrayNumInt)) + 1;
	sprintf(formatString, "%%s_%%0%dd", numDigits);

	for (k = 0; k < arrayNumInt; k++) {
	   struct History history; 

	    if (step != 0.0)
		sprintf(shad_filename, formatString, horizon, k);

	    angle = (single_direction * deg2rad) + (dfr_rad * k);
	    /*              
	       com_par(angle);
	     */
	    G_message(_("Calculating map %01d of %01d (angle %.2f, raster map <%s>)"),
		     (k + 1), arrayNumInt, angle * rad2deg, shad_filename);

	    for (j = hor_row_start; j < hor_row_end; j++) {
		G_percent(j - hor_row_start, hor_numrows - 1, 2);
		shadow_angle = 15 * deg2rad;
		for (i = hor_col_start; i < hor_col_end; i++) {
		    ip100 = floor(i / 100.);
		    jp100 = floor(j / 100.);
		    ip = jp = 0;
		    xg0 = xx0 = (double)i *stepx;

		    xp = xmin + xx0;
		    yg0 = yy0 = (double)j *stepy;

		    yp = ymin + yy0;
		    length = 0;
		    if (ll_correction) {
			coslat = cos(deg2rad * yp);
			coslatsq = coslat * coslat;
		    }

		    longitude = xp;
		    latitude = yp;


		    if ((G_projection() != PROJECTION_LL)) {
			if (pj_do_proj(&longitude, &latitude, &iproj, &oproj) <	0)
			    G_fatal_error("Error in pj_do_proj");
		    }

		    latitude *= deg2rad;
		    longitude *= deg2rad;

		    inputAngle = angle + pihalf;
		    inputAngle =
			(inputAngle >=
			 twopi) ? inputAngle - twopi : inputAngle;


		    delt_lat = -0.0001 * cos(inputAngle);  /* Arbitrary small distance in latitude */
		    delt_lon = 0.0001 * sin(inputAngle) / cos(latitude);

		    latitude = (latitude + delt_lat) * rad2deg;
		    longitude = (longitude + delt_lon) * rad2deg;

		    if ((G_projection() != PROJECTION_LL)) {
			if (pj_do_proj(&longitude, &latitude, &oproj, &iproj) < 0)
			    G_fatal_error("Error in pj_do_proj");
		    }

		    delt_east = longitude - xp;
		    delt_nor = latitude - yp;

		    delt_dist =
			sqrt(delt_east * delt_east + delt_nor * delt_nor);

		    sinangle = delt_nor / delt_dist;
		    if (fabs(sinangle) < 0.0000001) {
			sinangle = 0.;
		    }
		    cosangle = delt_east / delt_dist;
		    if (fabs(cosangle) < 0.0000001) {
			cosangle = 0.;
		    }
		    distsinangle = 32000;
		    distcosangle = 32000;

		    if (sinangle != 0.) {
			distsinangle = 100. / (distxy * sinangle);
		    }
		    if (cosangle != 0.) {
			distcosangle = 100. / (distxy * cosangle);
		    }

		    stepsinangle = stepxy * sinangle;
		    stepcosangle = stepxy * cosangle;


		    z_orig = zp = z[j][i];
		    maxlength = (zmax - z_orig) / TANMINANGLE;
		    maxlength =
			(maxlength <
			 fixedMaxLength) ? maxlength : fixedMaxLength;

		    if (z_orig != UNDEFZ) {

			/* G_debug(3,"**************new line %d %d\n", i, j); */
			shadow_angle = horizon_height();

			if (degreeOutput) {
			    shadow_angle *= rad2deg;
			}

			/*
			   if((j==1400)&&(i==1400))
			   {
			   G_debug(3, "coordinates=%f,%f, raster no. %d, horizon=%f\n",
			   xp, yp, k, shadow_angle);
			   }
			 */
			horizon_raster[j - buffer_s][i - buffer_w] =
			    shadow_angle;

		    }	/* undefs */
		}
	    }

	    OUTGR(cellhd.rows, cellhd.cols);

	    /* empty array */
	    for (j = 0; j < hor_numrows; j++) {
		for (i = 0; i < hor_numcols; i++)
		    horizon_raster[j][i] = 0.;
	    }

	    /* return back the buffered region */
	    if (bufferZone > 0.)
		Rast_set_window(&new_cellhd);

	    /* write metadata */
	    Rast_short_history(shad_filename, "raster", &history);

	    sprintf(msg_buff,
		    "Angular height of terrain horizon, map %01d of %01d",
		    (k + 1), arrayNumInt);
	    Rast_put_cell_title(shad_filename, msg_buff);

	    if (degreeOutput)
		Rast_write_units(shad_filename, "degrees");
	    else
		Rast_write_units(shad_filename, "radians");

	    Rast_command_history(&history);

	    /* insert a blank line */
	    Rast_append_history(&history, "");

	    Rast_append_format_history(
		&history,
		"Horizon view from azimuth angle %.2f degrees CCW from East",
		angle * rad2deg);

	    Rast_write_history(shad_filename, &history);
	}
    }
}

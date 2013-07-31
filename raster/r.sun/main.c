
/*******************************************************************************
r.sun: This program was writen by Jaro Hofierka in Summer 1993 and re-engineered
in 1996-1999. In cooperation with Marcel Suri and Thomas Huld from JRC in Ispra
a new version of r.sun was prepared using ESRA solar radiation formulas.
See manual pages for details.
(C) 2002 Copyright Jaro Hofierka, Gresaka 22, 085 01 Bardejov, Slovakia, 
              and GeoModel, s.r.o., Bratislava, Slovakia
email: hofierka@geomodel.sk,marcel.suri@jrc.it,suri@geomodel.sk Thomas.Huld@jrc.it
(c) 2003-2013 by The GRASS Development Team
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
 * along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/*v. 2.0 July 2002, NULL data handling, JH */
/*v. 2.1 January 2003, code optimization by Thomas Huld, JH */
/*v. 3.0 February 2006, several changes (shadowing algorithm, earth's curvature JH */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#ifdef USE_OPENCL
  #ifdef __APPLE__
    #include <OpenCL/opencl.h>
  #else
    #include <CL/cl.h>
  #endif
#endif

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/gprojects.h>
#include <grass/glocale.h>
#include "sunradstruct.h"
#include "local_proto.h"
#include "rsunglobals.h"

/* default values */
#define NUM_PARTITIONS "1"
#define SKIP    "1"
#define BIG      1.e20
#define LINKE    "3.0"
#define SLOPE    "0.0"
#define ASPECT   "270"
#define ALB      "0.2"
#define STEP     "0.5"
#define BSKY      1.0
#define DSKY      1.0
#define DIST     "1.0"

#define SCALING_FACTOR 150.
const double invScale = 1. / SCALING_FACTOR;

#define AMAX1(arg1, arg2) ((arg1) >= (arg2) ? (arg1) : (arg2))
#define AMIN1(arg1, arg2) ((arg1) <= (arg2) ? (arg1) : (arg2))
#define DISTANCE1(x1, x2, y1, y2) (sqrt((x1 - x2)*(x1 - x2) + (y1 - y2)*(y1 - y2)))
#define DISTANCE2(x00, y00) ((xx0 - x00)*(xx0 - x00) + (yy0 - y00)*(yy0 - y00))

const double pihalf = M_PI * 0.5;
const double pi2 = M_PI * 2.;
const double deg2rad = M_PI / 180.;
const double rad2deg = 180. / M_PI;

FILE *felevin, *faspin, *fslopein, *flinkein, *falbedo, *flatin;
FILE *fincidout, *fbeam_rad, *finsol_time, *fdiff_rad, *frefl_rad;

const char *elevin;
const char *aspin;
const char *slopein;
const char *civiltime = NULL;
const char *linkein = NULL;
const char *albedo = NULL;
const char *latin = NULL;
const char *coefbh = NULL;
const char *coefdh = NULL;
const char *incidout = NULL;
const char *longin = NULL;
const char *horizon = NULL;
const char *beam_rad = NULL;
const char *insol_time = NULL;
const char *diff_rad = NULL;
const char *refl_rad = NULL;
const char *glob_rad = NULL;
const char *mapset = NULL;
const char *per;

struct Cell_head cellhd;
struct pj_info iproj;
struct pj_info oproj;
struct History hist;


int INPUT_part(int offset, double *zmax);
int OUTGR(void);
int min(int, int);
int max(int, int);

void cube(int, int);
void (*func) (int, int);

void joules2(struct SunGeometryConstDay *sunGeom,
	     struct SunGeometryVarDay *sunVarGeom,
	     struct SunGeometryVarSlope *sunSlopeGeom,
	     struct SolarRadVar *sunRadVar,
	     struct GridGeometry *gridGeom,
	     unsigned char *horizonpointer, double latitude,
	     double longitude);


void calculate(double singleSlope, double singleAspect,
	       double singleAlbedo, double singleLinke,
	       struct GridGeometry gridGeom);
double com_declin(int);

int n, m, ip, jp;
int d, day;
int saveMemory, numPartitions = 1;
long int shadowoffset = 0;
int varCount_global = 0;
int bitCount_global = 0;
int arrayNumInt = 1;
float **z = NULL, **o = NULL, **s = NULL, **li = NULL, **a = NULL,
    **latitudeArray = NULL, **longitudeArray, **cbhr = NULL, **cdhr = NULL;
double op, dp;
double invstepx, invstepy;
double sunrise_min = 24., sunrise_max = 0., sunset_min = 24., sunset_max = 0.;

float **lumcl, **beam, **insol, **diff, **refl, **globrad;
unsigned char *horizonarray = NULL;
double civilTime;

/*
 * double startTime, endTime;
 */
double xmin, xmax, ymin, ymax;
double declin, step, dist;
double linke_max = 0., linke_min = 100., albedo_max = 0., albedo_min = 1.0,
    lat_max = -90., lat_min = 90.;
double offsetx = 0.5, offsety = 0.5;
char *ttime;

/*
 * double slope;
 */
double o_orig, z1;

/*
 * double lum_C11, lum_C13, lum_C22, lum_C31, lum_C33;
 * double sinSolarAltitude; */

/* Sine of the solar altitude (angle above horizon) 
 */
/*
 * double sunrise_time, sunset_time;
 * double solarAltitude;
 * double tanSolarAlt, solarAzimuth;
 * double stepsinangle, stepcosangle;
 * double angle;
 */
double horizonStep;
double ltime, tim, timo;
double declination;	/* Contains the negative of the declination at the chosen day */

/*
 * double lum_C31_l, lum_C33_l;
 */
double beam_e, diff_e, refl_e, rr, insol_t;
double cbh, cdh;
double TOLER;

/* 1852m/nm * 60nm/degree = 111120 m/deg */
#define DEGREEINMETERS 111120.

int ll_correction = FALSE;
double coslatsq;

/* why not use G_distance() here which switches to geodesic/great
  circle distace as needed? */
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
    double singleSlope;
    double singleAspect;
    double singleAlbedo;
    double singleLinke;

    struct GModule *module;
    struct
    {
	struct Option *elevin, *aspin, *aspect, *slopein, *slope, *linkein,
	    *lin, *albedo, *longin, *alb, *latin, *coefbh, *coefdh,
	    *incidout, *beam_rad, *insol_time, *diff_rad, *refl_rad,
	    *glob_rad, *day, *step, *declin, *ltime, *dist, *horizon,
	    *horizonstep, *numPartitions, *civilTime;
    }
    parm;

    struct
    {
	struct Flag *noshade, *saveMemory;
    }
    flag;

    struct GridGeometry gridGeom;


    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("solar"));
    G_add_keyword(_("sun energy"));
    module->label = _("Solar irradiance and irradiation model.");
    module->description =
	_("Computes direct (beam), diffuse and reflected solar irradiation raster "
	 "maps for given day, latitude, surface and atmospheric conditions. Solar "
	 "parameters (e.g. sunrise, sunset times, declination, extraterrestrial "
	 "irradiance, daylight length) are saved in the map history file. "
	 "Alternatively, a local time can be specified to compute solar "
	 "incidence angle and/or irradiance raster maps. The shadowing effect of "
	 "the topography is optionally incorporated.");

    parm.elevin = G_define_option();
    parm.elevin->key = "elev_in";
    parm.elevin->type = TYPE_STRING;
    parm.elevin->required = YES;
    parm.elevin->gisprompt = "old,cell,raster";
    parm.elevin->description =
	_("Name of the input elevation raster map [meters]");
    parm.elevin->guisection = _("Input options");

    parm.aspin = G_define_option();
    parm.aspin->key = "asp_in";
    parm.aspin->type = TYPE_STRING;
    parm.aspin->required = NO;
    parm.aspin->gisprompt = "old,cell,raster";
    parm.aspin->description =
	_("Name of the input aspect map (terrain aspect or azimuth of the solar panel) [decimal degrees]");
    parm.aspin->guisection = _("Input options");

    parm.aspect = G_define_option();
    parm.aspect->key = "aspect";
    parm.aspect->type = TYPE_DOUBLE;
    parm.aspect->answer = ASPECT;
    parm.aspect->required = NO;
    parm.aspect->description =
	_("A single value of the orientation (aspect), 270 is south");
    parm.aspect->guisection = _("Input options");

    parm.slopein = G_define_option();
    parm.slopein->key = "slope_in";
    parm.slopein->type = TYPE_STRING;
    parm.slopein->required = NO;
    parm.slopein->gisprompt = "old,cell,raster";
    parm.slopein->description =
	_("Name of the input slope raster map (terrain slope or solar panel inclination) [decimal degrees]");
    parm.slopein->guisection = _("Input options");

    parm.slope = G_define_option();
    parm.slope->key = "slope";
    parm.slope->type = TYPE_DOUBLE;
    parm.slope->answer = SLOPE;
    parm.slope->required = NO;
    parm.slope->description = _("A single value of inclination (slope)");
    parm.slope->guisection = _("Input options");

    parm.linkein = G_define_option();
    parm.linkein->key = "linke_in";
    parm.linkein->type = TYPE_STRING;
    parm.linkein->required = NO;
    parm.linkein->gisprompt = "old,cell,raster";
    parm.linkein->description =
	_("Name of the Linke atmospheric turbidity coefficient input raster map [-]");
    parm.linkein->guisection = _("Input options");

    if (parm.linkein->answer == NULL) {
	parm.lin = G_define_option();
	parm.lin->key = "lin";
	parm.lin->type = TYPE_DOUBLE;
	parm.lin->answer = LINKE;
	parm.lin->required = NO;
	parm.lin->description =
	    _("A single value of the Linke atmospheric turbidity coefficient [-]");
	parm.lin->guisection = _("Input options");
    }

    parm.albedo = G_define_option();
    parm.albedo->key = "albedo";
    parm.albedo->type = TYPE_STRING;
    parm.albedo->required = NO;
    parm.albedo->gisprompt = "old,cell,raster";
    parm.albedo->description =
	_("Name of the ground albedo coefficient input raster map [-]");
    parm.albedo->guisection = _("Input options");

    if (parm.albedo->answer == NULL) {
	parm.alb = G_define_option();
	parm.alb->key = "alb";
	parm.alb->type = TYPE_DOUBLE;
	parm.alb->answer = ALB;
	parm.alb->required = NO;
	parm.alb->description =
	    _("A single value of the ground albedo coefficient [-]");
	parm.alb->guisection = _("Input options");
    }

    parm.latin = G_define_option();
    parm.latin->key = "lat_in";
    parm.latin->type = TYPE_STRING;
    parm.latin->required = NO;
    parm.latin->gisprompt = "old,cell,raster";
    parm.latin->description =
	_("Name of input raster map containing latitudes [decimal degrees]");
    parm.latin->guisection = _("Input options");

    parm.longin = G_define_option();
    parm.longin->key = "long_in";
    parm.longin->type = TYPE_STRING;
    parm.longin->required = NO;
    parm.longin->gisprompt = "old,cell,raster";
    parm.longin->description =
	_("Name of input raster map containing longitudes [decimal degrees]");
    parm.longin->guisection = _("Input options");

    parm.coefbh = G_define_option();
    parm.coefbh->key = "coef_bh";
    parm.coefbh->type = TYPE_STRING;
    parm.coefbh->required = NO;
    parm.coefbh->gisprompt = "old,cell,raster";
    parm.coefbh->description =
	_("Name of real-sky beam radiation coefficient (thick cloud) input raster map [0-1]");
    parm.coefbh->guisection = _("Input options");

    parm.coefdh = G_define_option();
    parm.coefdh->key = "coef_dh";
    parm.coefdh->type = TYPE_STRING;
    parm.coefdh->required = NO;
    parm.coefdh->gisprompt = "old,cell,raster";
    parm.coefdh->description =
	_("Name of real-sky diffuse radiation coefficient (haze) input raster map [0-1]");
    parm.coefdh->guisection = _("Input options");

    parm.horizon = G_define_option();
    parm.horizon->key = "horizon";
    parm.horizon->type = TYPE_STRING;
    parm.horizon->required = NO;
    parm.horizon->gisprompt = "old,cell,raster";
    parm.horizon->description = _("The horizon information input map prefix");
    parm.horizon->guisection = _("Input options");

    parm.horizonstep = G_define_option();
    parm.horizonstep->key = "horizon_step";
    parm.horizonstep->type = TYPE_DOUBLE;
    parm.horizonstep->required = NO;
    parm.horizonstep->description =
	_("Angle step size for multidirectional horizon [degrees]");
    parm.horizonstep->guisection = _("Input options");

    parm.incidout = G_define_option();
    parm.incidout->key = "incidout";
    parm.incidout->type = TYPE_STRING;
    parm.incidout->required = NO;
    parm.incidout->gisprompt = "new,cell,raster";
    parm.incidout->description =
	_("Output incidence angle raster map (mode 1 only)");
    parm.incidout->guisection = _("Output options");

    parm.beam_rad = G_define_option();
    parm.beam_rad->key = "beam_rad";
    parm.beam_rad->type = TYPE_STRING;
    parm.beam_rad->required = NO;
    parm.beam_rad->gisprompt = "new,cell,raster";
    parm.beam_rad->description =
	_("Output beam irradiance [W.m-2] (mode 1) or irradiation raster map [Wh.m-2.day-1] (mode 2)");
    parm.beam_rad->guisection = _("Output options");

    parm.diff_rad = G_define_option();
    parm.diff_rad->key = "diff_rad";
    parm.diff_rad->type = TYPE_STRING;
    parm.diff_rad->required = NO;
    parm.diff_rad->gisprompt = "new,cell,raster";
    parm.diff_rad->description =
	_("Output diffuse irradiance [W.m-2] (mode 1) or irradiation raster map [Wh.m-2.day-1] (mode 2)");
    parm.diff_rad->guisection = _("Output options");

    parm.refl_rad = G_define_option();
    parm.refl_rad->key = "refl_rad";
    parm.refl_rad->type = TYPE_STRING;
    parm.refl_rad->required = NO;
    parm.refl_rad->gisprompt = "new,cell,raster";
    parm.refl_rad->description =
	_("Output ground reflected irradiance [W.m-2] (mode 1) or irradiation raster map [Wh.m-2.day-1] (mode 2)");
    parm.refl_rad->guisection = _("Output options");

    parm.glob_rad = G_define_option();
    parm.glob_rad->key = "glob_rad";
    parm.glob_rad->type = TYPE_STRING;
    parm.glob_rad->required = NO;
    parm.glob_rad->gisprompt = "new,cell,raster";
    parm.glob_rad->description =
	_("Output global (total) irradiance/irradiation [W.m-2] (mode 1) or irradiance/irradiation raster map [Wh.m-2.day-1] (mode 2)");
    parm.glob_rad->guisection = _("Output options");

    parm.insol_time = G_define_option();
    parm.insol_time->key = "insol_time";
    parm.insol_time->type = TYPE_STRING;
    parm.insol_time->required = NO;
    parm.insol_time->gisprompt = "new,cell,raster";
    parm.insol_time->description =
	_("Output insolation time raster map [h] (mode 2 only)");
    parm.insol_time->guisection = _("Output options");

    parm.day = G_define_option();
    parm.day->key = "day";
    parm.day->type = TYPE_INTEGER;
    parm.day->required = YES;
    parm.day->description = _("No. of day of the year (1-365)");
    parm.day->options = "1-365";

    parm.step = G_define_option();
    parm.step->key = "step";
    parm.step->type = TYPE_DOUBLE;
    parm.step->answer = STEP;
    parm.step->required = NO;
    parm.step->description =
	_("Time step when computing all-day radiation sums [decimal hours]");

    parm.declin = G_define_option();
    parm.declin->key = "declination";
    parm.declin->type = TYPE_DOUBLE;
    parm.declin->required = NO;
    parm.declin->description =
	_("Declination value (overriding the internally computed value) [radians]");

    parm.ltime = G_define_option();
    parm.ltime->key = "time";
    parm.ltime->type = TYPE_DOUBLE;
    /*          parm.ltime->answer = TIME; */
    parm.ltime->required = NO;
    parm.ltime->description =
	_("Local (solar) time (to be set for mode 1 only) [decimal hours]");
    parm.ltime->options = "0-24";

    /*
     * parm.startTime = G_define_option();
     * parm.startTime->key = "start_time";
     * parm.startTime->type = TYPE_DOUBLE;
     * parm.startTime->required = NO;
     * parm.startTime->description = _("Starting time for calculating results for several different times.");
     * 
     * parm.endTime = G_define_option();
     * parm.endTime->key = "end_time";
     * parm.endTime->type = TYPE_DOUBLE;
     * parm.endTime->required = NO;
     * parm.endTime->description = _("End time for calculating results for several different times.)";
     */

    parm.dist = G_define_option();
    parm.dist->key = "distance_step";
    parm.dist->type = TYPE_DOUBLE;
    parm.dist->answer = DIST;
    parm.dist->required = NO;
    parm.dist->description =
	_("Sampling distance step coefficient (0.5-1.5)");

    parm.numPartitions = G_define_option();
    parm.numPartitions->key = "num_partitions";
    parm.numPartitions->type = TYPE_INTEGER;
    parm.numPartitions->answer = NUM_PARTITIONS;
    parm.numPartitions->required = NO;
    parm.numPartitions->description =
	_("Read the input files in this number of chunks");

    parm.civilTime = G_define_option();
    parm.civilTime->key = "civil_time";
    parm.civilTime->type = TYPE_DOUBLE;
    parm.civilTime->required = NO;
    parm.civilTime->description =
	_("Civil time zone value, if none, the time will be local solar time");


    flag.noshade = G_define_flag();
    flag.noshade->key = 'p';
    flag.noshade->description =
	_("Do not incorporate the shadowing effect of terrain");

    flag.saveMemory = G_define_flag();
    flag.saveMemory->key = 'm';
    flag.saveMemory->description =
	_("Use the low-memory version of the program");


    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);


    G_get_set_window(&cellhd);

    gridGeom.stepx = cellhd.ew_res;
    gridGeom.stepy = cellhd.ns_res;
    invstepx = 1. / gridGeom.stepx;
    invstepy = 1. / gridGeom.stepy;
    n /*n_cols */  = cellhd.cols;
    m /*n_rows */  = cellhd.rows;
    xmin = cellhd.west;
    ymin = cellhd.south;
    xmax = cellhd.east;
    ymax = cellhd.north;
    gridGeom.deltx = fabs(cellhd.east - cellhd.west);
    gridGeom.delty = fabs(cellhd.north - cellhd.south);

    setUseShadow(!flag.noshade->answer);

    saveMemory = flag.saveMemory->answer;

    elevin = parm.elevin->answer;
    aspin = parm.aspin->answer;
    slopein = parm.slopein->answer;
    linkein = parm.linkein->answer;
    albedo = parm.albedo->answer;
    latin = parm.latin->answer;
    longin = parm.longin->answer;

    civiltime = parm.civilTime->answer;

    if (civiltime != NULL) {
	setUseCivilTime(TRUE);

	if (longin == NULL)
	    G_fatal_error(
		_("You must give the longitude raster if you use civil time"));

	if (sscanf(parm.civilTime->answer, "%lf", &civilTime) != 1)
	    G_fatal_error(_("Error reading civil time zone value"));

	if (civilTime < -24. || civilTime > 24.)
	    G_fatal_error(_("Invalid civil time zone value"));

	/* Normalize if somebody should be weird enough to give more than +- 12 
	 * hours offset. */
	if (civilTime < -12.)
	    civilTime += 24.;
	else if (civilTime > 12.)
	    civilTime -= 24;
    }
    else {
	setUseCivilTime(FALSE);
    }

    coefbh = parm.coefbh->answer;
    coefdh = parm.coefdh->answer;
    incidout = parm.incidout->answer;
    horizon = parm.horizon->answer;
    setUseHorizonData(horizon != NULL);
    beam_rad = parm.beam_rad->answer;
    insol_time = parm.insol_time->answer;
    diff_rad = parm.diff_rad->answer;
    refl_rad = parm.refl_rad->answer;
    glob_rad = parm.glob_rad->answer;

    if ((insol_time != NULL) && (incidout != NULL))
	G_fatal_error(_("insol_time and incidout are incompatible options"));

    sscanf(parm.day->answer, "%d", &day);

    if (sscanf(parm.step->answer, "%lf", &step) != 1)
	G_fatal_error(_("Error reading time step size"));
    if (step <= 0.0 || step > 24.0)
	G_fatal_error(_("Invalid time step size"));

    if (parm.horizonstep->answer != NULL) {
	if (sscanf(parm.horizonstep->answer, "%lf", &horizonStep) != 1)
	    G_fatal_error(_("Error reading horizon step size"));
	if (horizonStep > 0.)
	    setHorizonInterval(deg2rad * horizonStep);
	else
	    G_fatal_error(_("The horizon step size must be greater than 0."));
    }
    else if (useHorizonData()) {
	G_fatal_error(_("If you use the horizon option you must also set the 'horizonstep' parameter."));
    }

    ttime = parm.ltime->answer;
    if (parm.ltime->answer != NULL) {
	if (insol_time != NULL)
	    G_fatal_error(_("Time and insol_time are incompatible options"));

	G_message(_("Mode 1: instantaneous solar incidence angle & irradiance using a set local time"));
	sscanf(parm.ltime->answer, "%lf", &timo);
    }
    else {
	if (incidout != NULL)
	    G_fatal_error(_("incidout requires time parameter to be set"));

	G_message(_("Mode 2: integrated daily irradiation for a given day of the year"));
    }

    /*      
     * if (parm.startTime->answer != NULL) sscanf(parm.startTime->answer, "%lf", &startTime);
     * if (parm.endTime->answer != NULL) sscanf(parm.endTime->answer, "%lf", &endTime);
     * 
     * if((parm.startTime->answer != NULL) ||(parm.endTime->answer != NULL))
     * {
     */
    /* The start and end times should only be defined if the single
     * time is not defined, and if the step size *is* defined. */
    /*      
     * if(parm.step->answer==NULL)
     *    G_fatal_error("If you want to use a time interval you must also define a step size.");
     * if(parm.ltime->answer!=NULL)
     *    G_fatal_error("If you want to use a time interval you can't define a single time value.\n");
     * if((parm.startTime->answer==NULL)||(parm.endTime->answer==NULL))
     *    G_fatal_error("If you want to use a time interval both the start and end times must be defined.\n");
     * }
     */
    if (parm.linkein->answer == NULL)
	sscanf(parm.lin->answer, "%lf", &singleLinke);
    if (parm.albedo->answer == NULL)
	sscanf(parm.alb->answer, "%lf", &singleAlbedo);

    if (parm.slopein->answer == NULL)
	sscanf(parm.slope->answer, "%lf", &singleSlope);
    singleSlope *= deg2rad;

    if (parm.aspin->answer == NULL)
	sscanf(parm.aspect->answer, "%lf", &singleAspect);
    singleAspect *= deg2rad;

    if (parm.coefbh->answer == NULL)
	cbh = BSKY;
    if (parm.coefdh->answer == NULL)
	cdh = DSKY;
    sscanf(parm.dist->answer, "%lf", &dist);

    if (parm.numPartitions->answer != NULL) {
	sscanf(parm.numPartitions->answer, "%d", &numPartitions);
	if (useShadow() && (!useHorizonData()) && (numPartitions != 1)) {
	    /* If you calculate shadows on the fly, the number of partitions
	     * must be one.
	     */
	    G_fatal_error(_("If you use -s and no horizon rasters, numpartitions must be =1"));
	}
    }

    gridGeom.stepxy = dist * 0.5 * (gridGeom.stepx + gridGeom.stepy);
    TOLER = gridGeom.stepxy * EPS;

    /* The save memory scheme will not work if you want to calculate shadows
     * on the fly. If you calculate without shadow effects or if you have the
     * shadows pre-calculated, there is no problem. */

    if (saveMemory && useShadow() && (!useHorizonData()))
	G_fatal_error(
	    _("If you want to save memory and to use shadows, "
	      "you must use pre-calculated horizons."));

    if (parm.declin->answer == NULL)
	declination = com_declin(day);
    else {
	sscanf(parm.declin->answer, "%lf", &declin);
	declination = -declin;
    }

    if (ttime != 0) {
	/* Shadow for just one time during the day */
	if (horizon == NULL) {
	    arrayNumInt = 1;
	}
	else if (useHorizonData()) {
	    arrayNumInt = (int)(360. / horizonStep);
	}
    }
    else {
	if (useHorizonData()) {
	    /*        Number of bytes holding the horizon information */
	    arrayNumInt = (int)(360. / horizonStep);
	}
    }

    if (ttime != NULL) {

	tim = (timo - 12) * 15;
	/* converting to degrees */
	/* Jenco (12-timeAngle) * 15 */
	if (tim < 0)
	    tim += 360;
	tim = M_PI * tim / 180;
	/* conv. to radians */
    }

    /* Set up parameters for projection to lat/long if necessary */
    /*  we need to do this even if latin= and longin= were given because
	rsunlib's com_par() needs iproj and oproj */
    if ((G_projection() != PROJECTION_LL)) {
	struct Key_Value *in_proj_info, *in_unit_info;

	if ((in_proj_info = G_get_projinfo()) == NULL)
	    G_fatal_error(
		_("Can't get projection info of current location"));

	if ((in_unit_info = G_get_projunits()) == NULL)
	    G_fatal_error(
		_("Can't get projection units of current location"));

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
    }

    if ((latin != NULL || longin != NULL) && (G_projection() == PROJECTION_LL))
	G_warning(_("latin and longin raster maps have no effect when in a Lat/Lon location"));
	/* true about longin= when civiltime is used? */
	/* civiltime needs longin= but not latin= for non-LL projections -
	     better would be it just use pj_proj() if it needs those?? */
    if (latin != NULL && longin == NULL)
	G_fatal_error(_("Both latin and longin raster maps must be given, or neither"));

/**********end of parser - ******************************/


    if ((G_projection() == PROJECTION_LL))
	ll_correction = TRUE;

    G_debug(3, "calculate() starts...");
    calculate(singleSlope, singleAspect, singleAlbedo, singleLinke, gridGeom);
    G_debug(3, "OUTGR() starts...");
    OUTGR();

    exit(EXIT_SUCCESS);
}


int INPUT_part(int offset, double *zmax)
{
    int finalRow, rowrevoffset;
    int numRows;
    int numDigits;
    FCELL *cell1 = NULL, *cell2 = NULL;
    FCELL *cell3 = NULL, *cell4 = NULL, *cell5 = NULL, *cell6 = NULL, *cell7 =
	NULL;
    FCELL *rast1 = NULL, *rast2 = NULL;
    static FCELL **horizonbuf;
    unsigned char *horizonpointer;
    int fd1 = -1, fd2 = -1, fd3 = -1, fd4 = -1, fd5 = -1, fd6 = -1,
	fd7 = -1, row, row_rev;
    static int *fd_shad;
    int fr1 = -1, fr2 = -1;
    int l, i, j;
    char shad_filename[256];
    char formatString[10];

    finalRow = m - offset - m / numPartitions;
    if (finalRow < 0) {
	finalRow = 0;
    }

    numRows = m / numPartitions;

    cell1 = Rast_allocate_f_buf();

    if (z == NULL) {
	z = (float **)G_malloc(sizeof(float *) * (numRows));


	for (l = 0; l < numRows; l++) {
	    z[l] = (float *)G_malloc(sizeof(float) * (n));
	}
    }

    fd1 = Rast_open_old(elevin, "");

    if (slopein != NULL) {
	cell3 = Rast_allocate_f_buf();
	if (s == NULL) {
	    s = (float **)G_malloc(sizeof(float *) * (numRows));

	    for (l = 0; l < numRows; l++) {
		s[l] = (float *)G_malloc(sizeof(float) * (n));
	    }
	}
	fd3 = Rast_open_old(slopein, "");

    }

    if (aspin != NULL) {
	cell2 = Rast_allocate_f_buf();

	if (o == NULL) {
	    o = (float **)G_malloc(sizeof(float *) * (numRows));

	    for (l = 0; l < numRows; l++) {
		o[l] = (float *)G_malloc(sizeof(float) * (n));
	    }
	}
	fd2 = Rast_open_old(aspin, "");

    }

    if (linkein != NULL) {
	cell4 = Rast_allocate_f_buf();
	if (li == NULL) {
	    li = (float **)G_malloc(sizeof(float *) * (numRows));
	    for (l = 0; l < numRows; l++)
		li[l] = (float *)G_malloc(sizeof(float) * (n));

	}
	fd4 = Rast_open_old(linkein, "");
    }

    if (albedo != NULL) {
	cell5 = Rast_allocate_f_buf();
	if (a == NULL) {
	    a = (float **)G_malloc(sizeof(float *) * (numRows));
	    for (l = 0; l < numRows; l++)
		a[l] = (float *)G_malloc(sizeof(float) * (n));
	}
	fd5 = Rast_open_old(albedo, "");
    }

    if (latin != NULL) {
	cell6 = Rast_allocate_f_buf();
	if (latitudeArray == NULL) {
	    latitudeArray = (float **)G_malloc(sizeof(float *) * (numRows));
	    for (l = 0; l < numRows; l++)
		latitudeArray[l] = (float *)G_malloc(sizeof(float) * (n));
	}
	fd6 = Rast_open_old(latin, "");
    }

    if (longin != NULL) {
	cell7 = Rast_allocate_f_buf();
	longitudeArray = (float **)G_malloc(sizeof(float *) * (numRows));
	for (l = 0; l < numRows; l++)
	    longitudeArray[l] = (float *)G_malloc(sizeof(float) * (n));

	fd7 = Rast_open_old(longin, "");
    }

    if (coefbh != NULL) {
	rast1 = Rast_allocate_f_buf();
	if (cbhr == NULL) {
	    cbhr = (float **)G_malloc(sizeof(float *) * (numRows));
	    for (l = 0; l < numRows; l++)
		cbhr[l] = (float *)G_malloc(sizeof(float) * (n));
	}
	fr1 = Rast_open_old(coefbh, "");
    }

    if (coefdh != NULL) {
	rast2 = Rast_allocate_f_buf();
	if (cdhr == NULL) {
	    cdhr = (float **)G_malloc(sizeof(float *) * (numRows));
	    for (l = 0; l < numRows; l++)
		cdhr[l] = (float *)G_malloc(sizeof(float) * (n));
	}
	fr2 = Rast_open_old(coefdh, "");
    }

    if (useHorizonData()) {
	if (horizonarray == NULL) {
	    horizonarray =
		(unsigned char *)G_malloc(sizeof(char) * arrayNumInt *
					  numRows * n);

	    horizonbuf = (FCELL **) G_malloc(sizeof(FCELL *) * arrayNumInt);
	    fd_shad = (int *)G_malloc(sizeof(int) * arrayNumInt);
	}
	/*
	 * if(ttime != NULL)
	 * {
	 * 
	 * horizonbuf[0]=Rast_allocate_f_buf();
	 * sprintf(shad_filename, "%s_%02d", horizon, arrayNumInt);
	 * if((mapset=G_find_raster2(shad_filename,""))==NULL)
	 * G_message("Horizon file no. %d not found\n", arrayNumInt);
	 * 
	 * fd_shad[0] = Rast_open_old(shad_filename,mapset);
	 * }
	 * else
	 * {
	 */
	numDigits = (int)(log10(1. * arrayNumInt)) + 1;
	sprintf(formatString, "%%s_%%0%dd", numDigits);
	for (i = 0; i < arrayNumInt; i++) {
	    horizonbuf[i] = Rast_allocate_f_buf();
	    sprintf(shad_filename, formatString, horizon, i);
	    fd_shad[i] = Rast_open_old(shad_filename, "");
	}
    }
    /*
     * }
     */

    if (useHorizonData()) {

	for (i = 0; i < arrayNumInt; i++) {
	    for (row = m - offset - 1; row >= finalRow; row--) {

		row_rev = m - row - 1;
		rowrevoffset = row_rev - offset;
		Rast_get_f_row(fd_shad[i], horizonbuf[i], row);
		horizonpointer =
		    horizonarray + (ssize_t) arrayNumInt *n * rowrevoffset;
		for (j = 0; j < n; j++) {

		    horizonpointer[i] = (char)(rint(SCALING_FACTOR *
						    AMIN1(horizonbuf[i][j],
							  256 * invScale)));
		    horizonpointer += arrayNumInt;
		}
	    }
	}


    }


    for (row = m - offset - 1; row >= finalRow; row--) {
	Rast_get_f_row(fd1, cell1, row);
	if (aspin != NULL)
	    Rast_get_f_row(fd2, cell2, row);
	if (slopein != NULL)
	    Rast_get_f_row(fd3, cell3, row);
	if (linkein != NULL)
	    Rast_get_f_row(fd4, cell4, row);
	if (albedo != NULL)
	    Rast_get_f_row(fd5, cell5, row);
	if (latin != NULL)
	    Rast_get_f_row(fd6, cell6, row);
	if (longin != NULL)
	    Rast_get_f_row(fd7, cell7, row);
	if (coefbh != NULL)
	    Rast_get_f_row(fr1, rast1, row);
	if (coefdh != NULL)
	    Rast_get_f_row(fr2, rast2, row);

	row_rev = m - row - 1;
	rowrevoffset = row_rev - offset;

	for (j = 0; j < n; j++) {
	    if (!Rast_is_f_null_value(cell1 + j))
		z[rowrevoffset][j] = (float)cell1[j];
	    else
		z[rowrevoffset][j] = UNDEFZ;

	    if (aspin != NULL) {
		if (!Rast_is_f_null_value(cell2 + j))
		    o[rowrevoffset][j] = (float)cell2[j];
		else
		    o[rowrevoffset][j] = UNDEFZ;
	    }
	    if (slopein != NULL) {
		if (!Rast_is_f_null_value(cell3 + j))
		    s[rowrevoffset][j] = (float)cell3[j];
		else
		    s[rowrevoffset][j] = UNDEFZ;
	    }

	    if (linkein != NULL) {
		if (!Rast_is_f_null_value(cell4 + j))
		    li[rowrevoffset][j] = (float)cell4[j];
		else
		    li[rowrevoffset][j] = UNDEFZ;
	    }

	    if (albedo != NULL) {
		if (!Rast_is_f_null_value(cell5 + j))
		    a[rowrevoffset][j] = (float)cell5[j];
		else
		    a[rowrevoffset][j] = UNDEFZ;
	    }

	    if (latin != NULL) {
		if (!Rast_is_f_null_value(cell6 + j))
		    latitudeArray[rowrevoffset][j] = (float)cell6[j];
		else
		    latitudeArray[rowrevoffset][j] = UNDEFZ;
	    }

	    if (longin != NULL) {
		if (!Rast_is_f_null_value(cell7 + j))
		    longitudeArray[rowrevoffset][j] = (float)cell7[j];
		else
		    longitudeArray[rowrevoffset][j] = UNDEFZ;
	    }

	    if (coefbh != NULL) {
		if (!Rast_is_f_null_value(rast1 + j))
		    cbhr[rowrevoffset][j] = (float)rast1[j];
		else
		    cbhr[rowrevoffset][j] = UNDEFZ;
	    }

	    if (coefdh != NULL) {
		if (!Rast_is_f_null_value(rast2 + j))
		    cdhr[rowrevoffset][j] = (float)rast2[j];
		else
		    cdhr[rowrevoffset][j] = UNDEFZ;
	    }
	}
    }

    Rast_close(fd1);
    G_free(cell1);

    if (aspin != NULL) {
	G_free(cell2);
	Rast_close(fd2);
    }
    if (slopein != NULL) {
	G_free(cell3);
	Rast_close(fd3);
    }
    if (linkein != NULL) {
	G_free(cell4);
	Rast_close(fd4);
    }
    if (albedo != NULL) {
	G_free(cell5);
	Rast_close(fd5);
    }
    if (latin != NULL) {
	G_free(cell6);
	Rast_close(fd6);
    }
    if (longin != NULL) {
	G_free(cell7);
	Rast_close(fd7);
    }
    if (coefbh != NULL) {
	G_free(rast1);
	Rast_close(fr1);
    }
    if (coefdh != NULL) {
	G_free(rast2);
	Rast_close(fr2);
    }


    if (useHorizonData()) {
	for (i = 0; i < arrayNumInt; i++) {
	    Rast_close(fd_shad[i]);
	    G_free(horizonbuf[i]);
	}
    }


/*******transformation of angles from 0 to east counterclock
        to 0 to north clocwise, for ori=0 upslope flowlines
        turn the orientation 2*M_PI ************/

    /* needs to be eliminated */

    /*for (i = 0; i < m; ++i) */
    for (i = 0; i < numRows; i++) {
	for (j = 0; j < n; j++) {
	    *zmax = AMAX1(*zmax, z[i][j]);
	    if (aspin != NULL) {
		if (o[i][j] != 0.) {
		    if (o[i][j] < 90.)
			o[i][j] = 90. - o[i][j];
		    else
			o[i][j] = 450. - o[i][j];
		}
		/*   G_debug(3,"o,z = %d  %d i,j, %d %d \n", o[i][j],z[i][j],i,j); */

		if ((aspin != NULL) && o[i][j] == UNDEFZ)
		    z[i][j] = UNDEFZ;
		if ((slopein != NULL) && s[i][j] == UNDEFZ)
		    z[i][j] = UNDEFZ;
		if (linkein != NULL && li[i][j] == UNDEFZ)
		    z[i][j] = UNDEFZ;
		if (albedo != NULL && a[i][j] == UNDEFZ)
		    z[i][j] = UNDEFZ;
		if (latin != NULL && latitudeArray[i][j] == UNDEFZ)
		    z[i][j] = UNDEFZ;
		if (coefbh != NULL && cbhr[i][j] == UNDEFZ)
		    z[i][j] = UNDEFZ;
		if (coefdh != NULL && cdhr[i][j] == UNDEFZ)
		    z[i][j] = UNDEFZ;
	    }

	}
    }

    return 1;
}

int OUTGR(void)
{
    FCELL *cell7 = NULL, *cell8 = NULL, *cell9 = NULL, *cell10 =
	NULL, *cell11 = NULL, *cell12 = NULL;
    int fd7 = -1, fd8 = -1, fd9 = -1, fd10 = -1, fd11 = -1, fd12 = -1;
    int i, iarc, j;

    if (incidout != NULL) {
	cell7 = Rast_allocate_f_buf();
	fd7 = Rast_open_fp_new(incidout);
    }

    if (beam_rad != NULL) {
	cell8 = Rast_allocate_f_buf();
	fd8 = Rast_open_fp_new(beam_rad);
    }

    if (insol_time != NULL) {
	cell11 = Rast_allocate_f_buf();
	fd11 = Rast_open_fp_new(insol_time);
    }

    if (diff_rad != NULL) {
	cell9 = Rast_allocate_f_buf();
	fd9 = Rast_open_fp_new(diff_rad);
    }

    if (refl_rad != NULL) {
	cell10 = Rast_allocate_f_buf();
	fd10 = Rast_open_fp_new(refl_rad);
    }

    if (glob_rad != NULL) {
	cell12 = Rast_allocate_f_buf();
	fd12 = Rast_open_fp_new(glob_rad);
    }

    if (m != Rast_window_rows())
	G_fatal_error("OOPS: rows changed from %d to %d", m, Rast_window_rows());
    if (n != Rast_window_cols())
	G_fatal_error("OOPS: cols changed from %d to %d", n, Rast_window_cols());

    for (iarc = 0; iarc < m; iarc++) {
	i = m - iarc - 1;
	if (incidout != NULL) {
	    for (j = 0; j < n; j++) {
		if (lumcl[i][j] == UNDEFZ)
		    Rast_set_f_null_value(cell7 + j, 1);
		else
		    cell7[j] = (FCELL) lumcl[i][j];
	    }
	    Rast_put_f_row(fd7, cell7);
	}

	if (beam_rad != NULL) {
	    for (j = 0; j < n; j++) {
		if (beam[i][j] == UNDEFZ)
		    Rast_set_f_null_value(cell8 + j, 1);
		else
		    cell8[j] = (FCELL) beam[i][j];
	    }
	    Rast_put_f_row(fd8, cell8);
	}

	if (glob_rad != NULL) {
	    for (j = 0; j < n; j++) {
		if (globrad[i][j] == UNDEFZ)
		    Rast_set_f_null_value(cell12 + j, 1);
		else
		    cell12[j] = (FCELL) globrad[i][j];
	    }
	    Rast_put_f_row(fd12, cell12);
	}


	if (insol_time != NULL) {
	    for (j = 0; j < n; j++) {
		if (insol[i][j] == UNDEFZ)
		    Rast_set_f_null_value(cell11 + j, 1);
		else
		    cell11[j] = (FCELL) insol[i][j];
	    }
	    Rast_put_f_row(fd11, cell11);
	}


	if (diff_rad != NULL) {
	    for (j = 0; j < n; j++) {
		if (diff[i][j] == UNDEFZ)
		    Rast_set_f_null_value(cell9 + j, 1);
		else
		    cell9[j] = (FCELL) diff[i][j];
	    }
	    Rast_put_f_row(fd9, cell9);
	}

	if (refl_rad != NULL) {
	    for (j = 0; j < n; j++) {
		if (refl[i][j] == UNDEFZ)
		    Rast_set_f_null_value(cell10 + j, 1);
		else
		    cell10[j] = (FCELL) refl[i][j];
	    }
	    Rast_put_f_row(fd10, cell10);
	}

    }

    if (incidout != NULL) {
	Rast_close(fd7);
	Rast_write_history(incidout, &hist);
    }
    if (beam_rad != NULL) {
	Rast_close(fd8);
	Rast_write_history(beam_rad, &hist);
    }
    if (diff_rad != NULL) {
	Rast_close(fd9);
	Rast_write_history(diff_rad, &hist);
    }
    if (refl_rad != NULL) {
	Rast_close(fd10);
	Rast_write_history(refl_rad, &hist);
    }
    if (insol_time != NULL) {
	Rast_close(fd11);
	Rast_write_history(insol_time, &hist);
    }
    if (glob_rad != NULL) {
	Rast_close(fd12);
	Rast_write_history(glob_rad, &hist);
    }

    return 1;
}


/**********************************************************/

void joules2(struct SunGeometryConstDay *sunGeom,
	     struct SunGeometryVarDay *sunVarGeom,
	     struct SunGeometryVarSlope *sunSlopeGeom,
	     struct SolarRadVar *sunRadVar,
	     struct GridGeometry *gridGeom, unsigned char *horizonpointer,
	     double latitude, double longitude)
{

    double s0, dfr, dfr_rad;
    double ra, dra;
    int ss = 1;
    double firstTime;
    double firstAngle, lastAngle;
    int srStepNo;
    double bh;
    double rr;

    beam_e = 0.;
    diff_e = 0.;
    refl_e = 0.;
    insol_t = 0.;


    com_par(sunGeom, sunVarGeom, gridGeom, latitude, longitude);

    if (ttime != NULL) {		/*irradiance */

	s0 = lumcline2(sunGeom, sunVarGeom, sunSlopeGeom, gridGeom,
		       horizonpointer);

	if (sunVarGeom->solarAltitude > 0.) {
	    if ((!sunVarGeom->isShadow) && (s0 > 0.)) {
		ra = brad(s0, &bh, sunVarGeom, sunSlopeGeom, sunRadVar);	/* beam radiation */
		beam_e += ra;
	    }
	    else {
		beam_e = 0.;
		bh = 0.;
	    }

	    if ((diff_rad != NULL) || (glob_rad != NULL)) {
		dra = drad(s0, bh, &rr, sunVarGeom, sunSlopeGeom, sunRadVar);	/* diffuse rad. */
		diff_e += dra;
	    }
	    if ((refl_rad != NULL) || (glob_rad != NULL)) {
		if ((diff_rad == NULL) && (glob_rad == NULL))
		    drad(s0, bh, &rr, sunVarGeom, sunSlopeGeom, sunRadVar);
		refl_e += rr;	/* reflected rad. */
	    }
	}			/* solarAltitude */
    }
    else {
	/* all-day radiation */

	srStepNo = (int)(sunGeom->sunrise_time / step);

	if ((sunGeom->sunrise_time - srStepNo * step) > 0.5 * step) {
	    firstTime = (srStepNo + 1.5) * step;
	}
	else {
	    firstTime = (srStepNo + 0.5) * step;
	}


	firstAngle = (firstTime - 12) * HOURANGLE;
	lastAngle = (sunGeom->sunset_time - 12) * HOURANGLE;

	dfr_rad = step * HOURANGLE;

	sunGeom->timeAngle = firstAngle;

	varCount_global = 0;

	dfr = step;

	while (ss == 1) {

	    com_par(sunGeom, sunVarGeom, gridGeom, latitude, longitude);
	    s0 = lumcline2(sunGeom, sunVarGeom, sunSlopeGeom, gridGeom,
			   horizonpointer);

	    if (sunVarGeom->solarAltitude > 0.) {

		if ((!sunVarGeom->isShadow) && (s0 > 0.)) {
		    insol_t += dfr;
		    ra = brad(s0, &bh, sunVarGeom, sunSlopeGeom, sunRadVar);
		    beam_e += dfr * ra;
		    ra = 0.;
		}
		else {
		    bh = 0.;
		}
		if ((diff_rad != NULL) || (glob_rad != NULL)) {
		    dra =
			drad(s0, bh, &rr, sunVarGeom, sunSlopeGeom,
			     sunRadVar);
		    diff_e += dfr * dra;
		    dra = 0.;
		}
		if ((refl_rad != NULL) || (glob_rad != NULL)) {
		    if ((diff_rad == NULL) && (glob_rad == NULL)) {
			drad(s0, bh, &rr, sunVarGeom, sunSlopeGeom,
			     sunRadVar);
		    }
		    refl_e += dfr * rr;
		    rr = 0.;
		}
	    }			/* illuminated */


	    sunGeom->timeAngle = sunGeom->timeAngle + dfr_rad;

	    if (sunGeom->timeAngle > lastAngle) {
		ss = 0;		/* we've got the sunset */
	    }
	}			/* end of while */
    }				/* all-day radiation */

}

/*////////////////////////////////////////////////////////////////////// */


/*
 * void where_is_point(void)
 * {
 * double sx, sy;
 * double dx, dy;
 * double adx, ady;
 * int i, j;
 * 
 * sx = xx0 * invstepx + TOLER;
 * sy = yy0 * invstepy + TOLER;
 * 
 * i = (int)sx;
 * j = (int)sy;
 * if (i < n - 1 && j < m - 1) {
 * 
 * dx = xx0 - (double)i *stepx;
 * dy = yy0 - (double)j *stepy;
 * 
 * adx = fabs(dx);
 * ady = fabs(dy);
 * 
 * if ((adx > TOLER) && (ady > TOLER)) {
 * cube(j, i);
 * return;
 * }
 * else if ((adx > TOLER) && (ady < TOLER)) {
 * line_x(j, i);
 * return;
 * }
 * else if ((adx < TOLER) && (ady > TOLER)) {
 * line_y(j, i);
 * return;
 * }
 * else if ((adx < TOLER) && (ady < TOLER)) {
 * vertex(j, i);
 * return;
 * }
 * 
 * 
 * }
 * else {
 * func = NULL;
 * }
 * }
 * 
 */

void where_is_point(double *length, struct SunGeometryVarDay *sunVarGeom,
		    struct GridGeometry *gridGeom)
{
    double sx, sy;
    double dx, dy;

    /*              double adx, ady; */
    int i, j;

    sx = gridGeom->xx0 * invstepx + offsetx;	/* offset 0.5 cell size to get the right cell i, j */
    sy = gridGeom->yy0 * invstepy + offsety;

    i = (int)sx;
    j = (int)sy;

    /*      if (i < n-1  && j < m-1) {    to include last row/col */
    if (i <= n - 1 && j <= m - 1) {

	dx = (double)i *gridGeom->stepx;
	dy = (double)j *gridGeom->stepy;

	*length = distance(gridGeom->xg0, dx, gridGeom->yg0, dy);	/* dist from orig. grid point to the current grid point */

	sunVarGeom->zp = z[j][i];

	/*
	 * cube(j, i);
	 */
    }
}

/*
 * void vertex(jmin, imin)
 * int jmin, imin;
 * {
 * zp = z[jmin][imin];
 * if ((zp == UNDEFZ))
 * func = NULL;
 * }
 * void line_x(jmin, imin)
 * int jmin, imin;
 * {
 * double c1, c2;
 * double d1, d2, e1, e2;
 * e1 = (double)imin *stepx;
 * e2 = (double)(imin + 1) * stepx;
 * 
 * c1 = z[jmin][imin];
 * c2 = z[jmin][imin + 1];
 * if (!((c1 == UNDEFZ) || (c2 == UNDEFZ))) {
 * 
 * if (dist <= 1.0) {
 * d1 = (xx0 - e1) / (e2 - e1);
 * d2 = 1 - d1;
 * if (d1 < d2)
 * zp = c1;
 * else
 * zp = c2;
 * }
 * 
 * if (dist > 1.0)
 * zp = AMAX1(c1, c2);
 * }
 * else
 * func = NULL;
 * }
 * 
 * 
 * void line_y(jmin, imin)
 * int jmin, imin;
 * {
 * double c1, c2;
 * double d1, d2, e1, e2;
 * e1 = (double)jmin *stepy;
 * e2 = (double)(jmin + 1) * stepy;
 * 
 * c1 = z[jmin][imin];
 * c2 = z[jmin + 1][imin];
 * if (!((c1 == UNDEFZ) || (c2 == UNDEFZ))) {
 * 
 * if (dist <= 1.0) {
 * d1 = (yy0 - e1) / (e2 - e1);
 * d2 = 1 - d1;
 * if (d1 < d2)
 * zp = c1;
 * else
 * zp = c2;
 * }
 * 
 * if (dist > 1.0)
 * zp = AMAX1(c1, c2);
 * 
 * }
 * else
 * func = NULL;
 * 
 * }
 * 
 * void cube(jmin, imin)
 * int jmin, imin;
 * {
 * int i, ig = 0;
 * double x1, x2, y1, y2;
 * double v[4], vmin = BIG;
 * double c[4], cmax = -BIG;
 * 
 * x1 = (double)imin *stepx;
 * x2 = x1 + stepx;
 * 
 * y1 = (double)jmin *stepy;
 * y2 = y1 + stepy;
 * 
 * v[0] = DISTANCE2(x1, y1);
 * 
 * if (v[0] < vmin) {
 * ig = 0;
 * vmin = v[0];
 * }
 * v[1] = DISTANCE2(x2, y1);
 * 
 * if (v[1] < vmin) {
 * ig = 1;
 * vmin = v[1];
 * }
 * 
 * v[2] = DISTANCE2(x2, y2);
 * if (v[2] < vmin) {
 * ig = 2;
 * vmin = v[2];
 * }
 * 
 * v[3] = DISTANCE2(x1, y2);
 * if (v[3] < vmin) {
 * ig = 3;
 * vmin = v[3];
 * }
 * 
 * c[0] = z[jmin][imin];
 * c[1] = z[jmin][imin + 1];
 * c[2] = z[jmin + 1][imin + 1];
 * c[3] = z[jmin + 1][imin];
 * 
 * 
 * if (dist <= 1.0) {
 * 
 * if (c[ig] != UNDEFZ)
 * zp = c[ig];
 * else
 * func = NULL;
 * return;
 * }
 * 
 * if (dist > 1.0) {
 * for (i = 0; i < 4; i++) {
 * if (c[i] != UNDEFZ) {
 * cmax = AMAX1(cmax, c[i]);
 * zp = cmax;
 * }
 * else
 * func = NULL;
 * }
 * }
 * }
 */

/*
 * void cube(jmin, imin)
 * int jmin, imin;
 * {
 * zp = z[jmin][imin];
 * 
 * }
 */

void cube(jmin, imin)
{
}


/*////////////////////////////////////////////////////////////////////// */

void calculate(double singleSlope, double singleAspect, double singleAlbedo,
	       double singleLinke, struct GridGeometry gridGeom)
{
    int i, j, l;

    /*                      double energy; */
    int someRadiation;
    int numRows;
    int arrayOffset;
    double lum, q1;
    double dayRad;
    double latid_l, cos_u, cos_v, sin_u, sin_v;
    double sin_phi_l, tan_lam_l;
    double zmax;
    double longitTime = 0.;
    double locTimeOffset;
    double latitude, longitude;
    double coslat;


    struct SunGeometryConstDay sunGeom;
    struct SunGeometryVarDay sunVarGeom;
    struct SunGeometryVarSlope sunSlopeGeom;
    struct SolarRadVar sunRadVar;

    sunSlopeGeom.slope = singleSlope;
    sunSlopeGeom.aspect = singleAspect;
    sunRadVar.alb = singleAlbedo;
    sunRadVar.linke = singleLinke;
    sunRadVar.cbh = 1.0;
    sunRadVar.cdh = 1.0;

    sunGeom.sindecl = sin(declination);
    sunGeom.cosdecl = cos(declination);


    someRadiation = (beam_rad != NULL) || (insol_time != NULL) ||
	(diff_rad != NULL) || (refl_rad != NULL) || (glob_rad != NULL);


    if (incidout != NULL) {
	lumcl = (float **)G_malloc(sizeof(float *) * (m));
	for (l = 0; l < m; l++) {
	    lumcl[l] = (float *)G_malloc(sizeof(float) * (n));
	}
	for (j = 0; j < m; j++) {
	    for (i = 0; i < n; i++)
		lumcl[j][i] = UNDEFZ;
	}
    }

    if (beam_rad != NULL) {
	beam = (float **)G_malloc(sizeof(float *) * (m));
	for (l = 0; l < m; l++) {
	    beam[l] = (float *)G_malloc(sizeof(float) * (n));
	}

	for (j = 0; j < m; j++) {
	    for (i = 0; i < n; i++)
		beam[j][i] = UNDEFZ;
	}
    }

    if (insol_time != NULL) {
	insol = (float **)G_malloc(sizeof(float *) * (m));
	for (l = 0; l < m; l++) {
	    insol[l] = (float *)G_malloc(sizeof(float) * (n));
	}

	for (j = 0; j < m; j++) {
	    for (i = 0; i < n; i++)
		insol[j][i] = UNDEFZ;
	}
    }

    if (diff_rad != NULL) {
	diff = (float **)G_malloc(sizeof(float *) * (m));
	for (l = 0; l < m; l++) {
	    diff[l] = (float *)G_malloc(sizeof(float) * (n));
	}

	for (j = 0; j < m; j++) {
	    for (i = 0; i < n; i++)
		diff[j][i] = UNDEFZ;
	}
    }

    if (refl_rad != NULL) {
	refl = (float **)G_malloc(sizeof(float *) * (m));
	for (l = 0; l < m; l++) {
	    refl[l] = (float *)G_malloc(sizeof(float) * (n));
	}

	for (j = 0; j < m; j++) {
	    for (i = 0; i < n; i++)
		refl[j][i] = UNDEFZ;
	}
    }

    if (glob_rad != NULL) {
	globrad = (float **)G_malloc(sizeof(float *) * (m));
	for (l = 0; l < m; l++) {
	    globrad[l] = (float *)G_malloc(sizeof(float) * (n));
	}

	for (j = 0; j < m; j++) {
	    for (i = 0; i < n; i++)
		globrad[j][i] = UNDEFZ;
	}
    }



    sunRadVar.G_norm_extra = com_sol_const(day);

    numRows = m / numPartitions;

    if (useCivilTime()) {
	/* We need to calculate the deviation of the local solar time from the 
	 * "local clock time". */
	dayRad = 2. * M_PI * day / 365.25;
	locTimeOffset =
	    -0.128 * sin(dayRad - 0.04887) - 0.165 * sin(2 * dayRad +
							 0.34383);

	/* Time offset due to timezone as input by user */

	locTimeOffset += civilTime;
	setTimeOffset(locTimeOffset);
    }
    else {
	setTimeOffset(0.);
    }

    for (j = 0; j < m; j++) {
	G_percent(j, m - 1, 2);

	if (j % (numRows) == 0) {
	    INPUT_part(j, &zmax);
	    arrayOffset = 0;
	    shadowoffset = 0;

	}
	sunVarGeom.zmax = zmax;


	for (i = 0; i < n; i++) {

	    if (useCivilTime()) {
		/* sun travels 15deg per hour, so 1 TZ every 15 deg and 15 TZs * 24hrs = 360deg */
		longitTime = -longitudeArray[arrayOffset][i] / 15.;
	    }

	    gridGeom.xg0 = gridGeom.xx0 = (double)i *gridGeom.stepx;
	    gridGeom.yg0 = gridGeom.yy0 = (double)j *gridGeom.stepy;

	    gridGeom.xp = xmin + gridGeom.xx0;
	    gridGeom.yp = ymin + gridGeom.yy0;

	    if (ll_correction) {
		coslat = cos(deg2rad * gridGeom.yp);
		coslatsq = coslat * coslat;
	    }

	    func = NULL;

	    sunVarGeom.z_orig = z1 = sunVarGeom.zp = z[arrayOffset][i];

	    if (sunVarGeom.z_orig != UNDEFZ) {
		if (aspin != NULL) {
		    o_orig = o[arrayOffset][i];
		    if (o[arrayOffset][i] != 0.)
			sunSlopeGeom.aspect = o[arrayOffset][i] * deg2rad;
		    else
			sunSlopeGeom.aspect = UNDEF;
		}
		if (slopein != NULL) {
		    sunSlopeGeom.slope = s[arrayOffset][i] * deg2rad;
		}
		if (linkein != NULL) {
		    sunRadVar.linke = li[arrayOffset][i];
		    linke_max = AMAX1(linke_max, sunRadVar.linke);
		    linke_min = AMIN1(linke_min, sunRadVar.linke);
		}
		if (albedo != NULL) {
		    sunRadVar.alb = a[arrayOffset][i];
		    albedo_max = AMAX1(albedo_max, sunRadVar.alb);
		    albedo_min = AMIN1(albedo_min, sunRadVar.alb);
		}
		if (latin != NULL) {
		    latitude = latitudeArray[arrayOffset][i];
		    lat_max = AMAX1(lat_max, latitude);
		    lat_min = AMIN1(lat_min, latitude);
		    latitude *= deg2rad;
		}
		if (longin != NULL) {
		    longitude = longitudeArray[arrayOffset][i];
		    /* lon_max = AMAX1(lon_max, longitude); */
		    /* lon_min = AMIN1(lon_min, longitude); */
		    longitude *= deg2rad;
		}

		if ((G_projection() != PROJECTION_LL)) {

		    if (latin == NULL || longin == NULL) {
			/* if either is missing we have to calc both from current projection */
			longitude = gridGeom.xp;
			latitude = gridGeom.yp;

			if (pj_do_proj(&longitude, &latitude, &iproj, &oproj) < 0) {
			    G_fatal_error("Error in pj_do_proj");
			}

			lat_max = AMAX1(lat_max, latitude);
			lat_min = AMIN1(lat_min, latitude);
			latitude *= deg2rad;
			longitude *= deg2rad;
		    }
		}
		else {		/* ll projection */
		    latitude = gridGeom.yp;
		    longitude = gridGeom.xp;
		    lat_max = AMAX1(lat_max, latitude);
		    lat_min = AMIN1(lat_min, latitude);
		    latitude *= deg2rad;
		    longitude *= deg2rad;
		}

		if (coefbh != NULL) {
		    sunRadVar.cbh = cbhr[arrayOffset][i];
		}
		if (coefdh != NULL) {
		    sunRadVar.cdh = cdhr[arrayOffset][i];
		}
		cos_u = cos(M_PI / 2 - sunSlopeGeom.slope);	/* = sin(slope) */
		sin_u = sin(M_PI / 2 - sunSlopeGeom.slope);	/* = cos(slope) */
		cos_v = cos(M_PI / 2 + sunSlopeGeom.aspect);
		sin_v = sin(M_PI / 2 + sunSlopeGeom.aspect);

		if (ttime != NULL)
		    sunGeom.timeAngle = tim;

		gridGeom.sinlat = sin(-latitude);
		gridGeom.coslat = cos(-latitude);

		sin_phi_l =
		    -gridGeom.coslat * cos_u * sin_v +
		    gridGeom.sinlat * sin_u;
		latid_l = asin(sin_phi_l);

		q1 = gridGeom.sinlat * cos_u * sin_v +
		    gridGeom.coslat * sin_u;
		tan_lam_l = -cos_u * cos_v / q1;
		sunSlopeGeom.longit_l = atan(tan_lam_l);
		sunSlopeGeom.lum_C31_l = cos(latid_l) * sunGeom.cosdecl;
		sunSlopeGeom.lum_C33_l = sin_phi_l * sunGeom.sindecl;

		if ((incidout != NULL) || someRadiation) {
		    com_par_const(longitTime, &sunGeom, &gridGeom);
		    sunrise_min = AMIN1(sunrise_min, sunGeom.sunrise_time);
		    sunrise_max = AMAX1(sunrise_max, sunGeom.sunrise_time);
		    sunset_min = AMIN1(sunset_min, sunGeom.sunset_time);
		    sunset_max = AMAX1(sunset_max, sunGeom.sunset_time);

		}

		if (incidout != NULL) {
		    com_par(&sunGeom, &sunVarGeom, &gridGeom, latitude,
			    longitude);
		    lum =
			lumcline2(&sunGeom, &sunVarGeom, &sunSlopeGeom,
				  &gridGeom, horizonarray + shadowoffset);
		    if (lum > 0.) {
			lum = rad2deg * asin(lum);
			lumcl[j][i] = (float)lum;
		    }
		    else lumcl[j][i] = UNDEFZ;
		}

		if (someRadiation) {
		    joules2(&sunGeom, &sunVarGeom, &sunSlopeGeom, &sunRadVar,
			    &gridGeom, horizonarray + shadowoffset, latitude,
			    longitude);
		    if (beam_rad != NULL)
			beam[j][i] = (float)beam_e;
		    if (insol_time != NULL)
			insol[j][i] = (float)insol_t;
		    /*  G_debug(3,"\n %f",insol[j][i]); */
		    if (diff_rad != NULL)
			diff[j][i] = (float)diff_e;
		    if (refl_rad != NULL)
			refl[j][i] = (float)refl_e;
		    if (glob_rad != NULL)
			globrad[j][i] = (float)(beam_e + diff_e + refl_e);
		}

	    }			/* undefs */
	    shadowoffset += arrayNumInt;
	}
	arrayOffset++;
    }

    /* re-use &hist, but try all to initiate it for any case */
    /*   note this will result in incorrect map titles       */
    if (incidout != NULL) {
	Rast_short_history(incidout, "raster", &hist);
    }
    else if (beam_rad != NULL) {
	Rast_short_history(beam_rad, "raster", &hist);
    }
    else if (diff_rad != NULL) {
	Rast_short_history(diff_rad, "raster", &hist);
    }
    else if (refl_rad != NULL) {
	Rast_short_history(refl_rad, "raster", &hist);
    }
    else if (insol_time != NULL) {
	Rast_short_history(insol_time, "raster", &hist);
    }
    else if (glob_rad != NULL) {
	Rast_short_history(glob_rad, "raster", &hist);
    }
    else
	G_fatal_error
	    ("Failed to init map history: no output maps requested!");

    Rast_append_format_history(
	&hist,
	" ----------------------------------------------------------------");
    Rast_append_format_history(
	&hist,
	" Day [1-365]:                              %d",
	day);

    if (ttime != NULL)
	Rast_append_format_history(
	    &hist,
	    " Local (solar) time (decimal hr.):         %.4f", timo);

    Rast_append_format_history(
	&hist,
	" Solar constant (W/m^2):                   1367");
    Rast_append_format_history(
	&hist,
	" Extraterrestrial irradiance (W/m^2):      %f",
	sunRadVar.G_norm_extra);
    Rast_append_format_history(
	&hist,
	" Declination (rad):                        %f", -declination);

    Rast_append_format_history(
	&hist,
	" Latitude min-max(deg):                    %.4f - %.4f",
	lat_min, lat_max);

    if (ttime != NULL) {
	Rast_append_format_history(
	    &hist,
	    " Sunrise time (hr.):                       %.2f",
	    sunGeom.sunrise_time);
	Rast_append_format_history(
	    &hist,
	    " Sunset time (hr.):                        %.2f",
	    sunGeom.sunset_time);
	Rast_append_format_history(
	    &hist,
	    " Daylight time (hr.):                      %.2f",
	    sunGeom.sunset_time - sunGeom.sunrise_time);
    }
    else {
	Rast_append_format_history(
	    &hist,
	    " Sunrise time min-max (hr.):               %.2f - %.2f",
	    sunrise_min, sunrise_max);
	Rast_append_format_history(
	    &hist,
	    " Sunset time min-max (hr.):                %.2f - %.2f",
	    sunset_min, sunset_max);
	Rast_append_format_history(
	    &hist,
	    " Time step (hr.):                          %.4f", step);
    }

    if (incidout != NULL || ttime != NULL) {
	Rast_append_format_history(
	    &hist,
	    " Solar altitude (deg):                     %.4f",
	    sunVarGeom.solarAltitude * rad2deg);
	Rast_append_format_history(
	    &hist,
	    " Solar azimuth (deg):                      %.4f",
	    sunVarGeom.solarAzimuth * rad2deg);
    }

    if (linkein == NULL)
	Rast_append_format_history(
	    &hist,
	    " Linke turbidity factor:                   %.1f",
	    sunRadVar.linke);
    else
	Rast_append_format_history(
	    &hist,
	    " Linke turbidity factor min-max:           %.1f-%.1f",
	    linke_min, linke_max);

    if (albedo == NULL)
	Rast_append_format_history(
	    &hist,
	    " Ground albedo:                            %.3f",
	    sunRadVar.alb);
    else
	Rast_append_format_history(
	    &hist,
	    " Ground albedo min-max:                    %.3f-%.3f",
	    albedo_min, albedo_max);

    Rast_append_format_history(
	&hist,
	" -----------------------------------------------------------------");

    Rast_command_history(&hist);
    /* don't call Rast_write_history() until after Rast_close() or it just gets overwritten */

}				/* End of ) function */



double com_declin(int no_of_day)
{
    double d1, decl;

    /* stretch day number in the following calculation for siderial effect? */
    /*   ? double siderial_day = no_of_day + ((no_of_day * 0.25) / 365.) ? */
    /* or just change d1 to : d1 = pi2 * no_of_day / 365.0;  ? */
    d1 = pi2 * no_of_day / 365.25;
    decl = asin(0.3978 * sin(d1 - 1.4 + 0.0355 * sin(d1 - 0.0489)));
    decl = -decl;
    /*      G_debug(3," declination : %lf\n", decl); */

    return (decl);
}



int test(void)
{
    /* not finshed yet */
    int dej;

    G_message("\n ddd: %f", declin);
    dej = asin(-declin / 0.4093) * 365. / pi2 + 81;
    /*        dej = asin(-declin/23.35 * deg2rad) / 0.9856 - 284; */
    /*      dej = dej - 365; */
    G_message("\n d: %d ", dej);
    if (dej < day - 5 || dej > day + 5)
	return 0;
    else
	return 1;
}

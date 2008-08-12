
/*******************************************************************************
r.sun: This program was writen by Jaro Hofierka in Summer 1993 and re-engineered
in 1996-1999. In cooperation with Marcel Suri and Thomas Huld from JRC in Ispra
a new version of r.sun was prepared using ESRA solar radiation formulas.
See manual pages for details.
(C) 2002 Copyright Jaro Hofierka, Gresaka 22, 085 01 Bardejov, Slovakia, 
              and GeoModel, s.r.o., Bratislava, Slovakia
email: hofierka@geomodel.sk,marcel.suri@jrc.it,suri@geomodel.sk,
       Thomas Huld <Thomas.Huld@jrc.it> (lat/long fix)
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
 *   59 Temple Place - Suite 330,
 *   Boston, MA  02111-1307, USA.
 */

/*v. 2.0 July 2002, NULL data handling, JH */
/*v. 2.1 January 2003, code optimization by Thomas Huld, JH */
/*v. 3.0 February 2006, several changes (shadowing algorithm, earth's curvature JH */

#define M2_PI    2. * M_PI
#define RAD      360. / (2. * M_PI)
#define DEG      (2. * M_PI)/360.
#define EARTHRADIUS 6371000.	/* appx. for most ellipsoids or projections */
#define UNDEF    0.		/* undefined value for terrain aspect */
#define UNDEFZ   -9999.		/* internal undefined value for NULL */
#define SKIP    "1"
#define BIG      1.e20
#define IBIG     32767
#define EPS      1.e-4
#define LINKE    "3.0"
#define ALB      "0.2"
#define STEP     "0.5"
#define BSKY	  1.0
#define DSKY	  1.0
#define DIST      0.8

#define AMAX1(arg1, arg2) ((arg1) >= (arg2) ? (arg1) : (arg2))
#define AMIN1(arg1, arg2) ((arg1) <= (arg2) ? (arg1) : (arg2))
#define DISTANCE1(x1, x2, y1, y2) (sqrt((x1 - x2)*(x1 - x2) + (y1 - y2)*(y1 - y2)))

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/gprojects.h>
#include <grass/glocale.h>

const double deg2rad = M_PI / 180.;
const double rad2deg = 180. / M_PI;

FILE *felevin, *faspin, *fslopein, *flinkein, *falbedo, *flatin;
FILE *fincidout, *fbeam_rad, *finsol_time, *fdiff_rad, *frefl_rad;

char *elevin;
char *aspin;
char *slopein;
char *linkein = NULL;
char *albedo = NULL;
char *latin = NULL;
char *coefbh = NULL;
char *coefdh = NULL;
char *incidout = NULL;
char *beam_rad = NULL;
char *insol_time = NULL;
char *diff_rad = NULL;
char *refl_rad = NULL;
char *mapset1 = NULL, *mapset2 = NULL, *mapset3 = NULL, *mapset4 = NULL,
    *mapset5 = NULL, *mapset6 = NULL, *mapset7 = NULL, *mapset8 = NULL;
char *per;
char *shade;

struct Cell_head cellhd;
struct pj_info iproj;
struct pj_info oproj;
struct History hist;


int INPUT(void);
int OUTGR(void);
int min(int, int);
int max(int, int);
void com_par(void);
void com_par_const(void);
double lumcline2(void);
void joules2(void);

int new_point(void);
int searching(void);
void where_is_point(void);
void cube(int, int);
void (*func) (int, int);

void calculate(void);
double com_sol_const(int);
double com_declin(int);
double brad(double);
double drad(double);

int shd, typ, n, m, ip, jp;
float **z, **o, **s, **li, **a, **la, **cbhr, **cdhr;
double stepx, stepy, stepxy, xp, yp, op, dp, xg0, xx0, yg0, yy0, deltx, delty;
double invstepx, invstepy;
double sr_min = 24., sr_max = 0., ss_min = 24., ss_max = 0.;

float **lumcl, **beam, **insol, **diff, **refl;
double xmin, xmax, ymin, ymax, zmax = 0.;
int d, day, tien = 0;
double length, zmult = 1.0, c, declin, linke, alb, step;
double li_max = 0., li_min = 100., al_max = 0., al_min = 1.0, la_max = -90.,
    la_min = 90.;
char *tt, *lt;
double z_orig, o_orig, slope, aspect, z1, zp;
double lum_C11, lum_C13, lum_C22, lum_C31, lum_C33, lum_Lx, lum_Ly, lum_Lz;
double lum_p, sunrise_time, sunset_time, h0, tanh0, A0, angle;
double stepsinangle, stepcosangle;
double longitude, latitude, lum_time, ltime, tim, timo, declination;
double sinlat, coslat, sindecl, cosdecl;
double longit_l, latid_l, cos_u, cos_v, sin_u, sin_v;
double sin_phi_l, tan_lam_l, lum_C31_l, lum_C33_l;
double beam_e, diff_e, refl_e, bh, dh, rr, insol_t;
double cbh, cdh;

#define DEGREEINMETERS 111120.

int ll_correction = 0;
double coslatsq;

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

    struct GModule *module;
    struct
    {
	struct Option *elevin, *aspin, *slopein, *linkein, *lin, *albedo,
	    *alb, *latin, *lat, *coefbh, *coefdh, *incidout, *beam_rad,
	    *insol_time, *diff_rad, *refl_rad, *day, *step, *declin, *ltime;
    }
    parm;

    struct
    {
	struct Flag *shade;
    }
    flag;

    G_gisinit(argv[0]);
    module = G_define_module();

    module->keywords = _("raster");
    module->label = _("Solar irradiance and irradiation model.");
    module->description =
	_("Computes direct (beam), diffuse and reflected solar irradiation raster "
	 "maps for given day, latitude, surface and atmospheric conditions. Solar "
	 "parameters (e.g. sunrise, sunset times, declination, extraterrestrial "
	 "irradiance, daylight length) are saved in the map history file. "
	 "Alternatively, a local time can be specified to compute solar "
	 "incidence angle and/or irradiance raster maps. The shadowing effect of "
	 "the topography is optionally incorporated.");

    if (G_get_set_window(&cellhd) == -1)
	G_fatal_error("G_get_set_window() failed");

    stepx = cellhd.ew_res;
    stepy = cellhd.ns_res;
    invstepx = 1. / stepx;
    invstepy = 1. / stepy;
    n /*n_cols */  = cellhd.cols;
    m /*n_rows */  = cellhd.rows;
    xmin = cellhd.west;
    ymin = cellhd.south;
    xmax = cellhd.east;
    ymax = cellhd.north;
    deltx = fabs(cellhd.east - cellhd.west);
    delty = fabs(cellhd.north - cellhd.south);

    parm.elevin = G_define_option();
    parm.elevin->key = "elevin";
    parm.elevin->type = TYPE_STRING;
    parm.elevin->required = YES;
    parm.elevin->gisprompt = "old,cell,raster";
    parm.elevin->description =
	_("Name of the input elevation raster map [meters]");
    parm.elevin->guisection = _("Input_options");

    parm.aspin = G_define_option();
    parm.aspin->key = "aspin";
    parm.aspin->type = TYPE_STRING;
    parm.aspin->required = YES;
    parm.aspin->gisprompt = "old,cell,raster";
    parm.aspin->description =
	_("Name of the input aspect map (terrain aspect or azimuth of the solar panel) [decimal degrees]");
    parm.aspin->guisection = _("Input_options");


    parm.slopein = G_define_option();
    parm.slopein->key = "slopein";
    parm.slopein->type = TYPE_STRING;
    parm.slopein->required = YES;
    parm.slopein->gisprompt = "old,cell,raster";
    parm.slopein->description =
	_("Name of the input slope raster map (terrain slope or solar panel inclination) [decimal degrees]");
    parm.slopein->guisection = _("Input_options");

    parm.linkein = G_define_option();
    parm.linkein->key = "linkein";
    parm.linkein->type = TYPE_STRING;
    parm.linkein->required = NO;
    parm.linkein->gisprompt = "old,cell,raster";
    parm.linkein->description =
	_("Name of the Linke atmospheric turbidity coefficient input raster map [-]");
    parm.linkein->guisection = _("Input_options");

    if (parm.linkein->answer == NULL) {
	parm.lin = G_define_option();
	parm.lin->key = "lin";
	parm.lin->type = TYPE_DOUBLE;
	parm.lin->answer = LINKE;
	parm.lin->required = NO;
	parm.lin->guisection = _("Input_options");
	parm.lin->description =
	    _("A single value of the Linke atmospheric turbidity coefficient [-]");
    }

    parm.albedo = G_define_option();
    parm.albedo->key = "albedo";
    parm.albedo->type = TYPE_STRING;
    parm.albedo->required = NO;
    parm.albedo->gisprompt = "old,cell,raster";
    parm.albedo->description =
	_("Name of the ground albedo coefficient input raster map [-]");
    parm.albedo->guisection = _("Input_options");

    if (parm.albedo->answer == NULL) {
	parm.alb = G_define_option();
	parm.alb->key = "alb";
	parm.alb->type = TYPE_DOUBLE;
	parm.alb->answer = ALB;
	parm.alb->required = NO;
	parm.alb->guisection = _("Input_options");
	parm.alb->description =
	    _("A single value of the ground albedo coefficient [-]");
    }

    parm.latin = G_define_option();
    parm.latin->key = "latin";
    parm.latin->type = TYPE_STRING;
    parm.latin->required = NO;
    parm.latin->gisprompt = "old,cell,raster";
    parm.latin->description =
	_("Name of the latitudes input raster map [decimal degrees]");
    parm.latin->guisection = _("Input_options");

    if (parm.latin->answer == NULL) {
	parm.lat = G_define_option();
	parm.lat->key = "lat";
	parm.lat->type = TYPE_DOUBLE;
	parm.lat->required = NO;
	parm.lat->guisection = _("Input_options");
	parm.lat->description =
	    _("A single value of latitude [decimal degrees]");
    }

    parm.coefbh = G_define_option();
    parm.coefbh->key = "coefbh";
    parm.coefbh->type = TYPE_STRING;
    parm.coefbh->required = NO;
    parm.coefbh->gisprompt = "old,cell,raster";
    parm.coefbh->description =
	_("Name of real-sky beam radiation coefficient raster map [-]");
    parm.coefbh->guisection = _("Input_options");

    parm.coefdh = G_define_option();
    parm.coefdh->key = "coefdh";
    parm.coefdh->type = TYPE_STRING;
    parm.coefdh->required = NO;
    parm.coefdh->gisprompt = "old,cell,raster";
    parm.coefdh->description =
	_("Name of real-sky diffuse radiation coefficient raster map [-]");
    parm.coefdh->guisection = _("Input_options");

    parm.incidout = G_define_option();
    parm.incidout->key = "incidout";
    parm.incidout->type = TYPE_STRING;
    parm.incidout->required = NO;
    parm.incidout->gisprompt = "new,cell,raster";
    parm.incidout->description =
	_("Output incidence angle raster map (mode 1 only)");
    parm.incidout->guisection = _("Output_options");

    parm.beam_rad = G_define_option();
    parm.beam_rad->key = "beam_rad";
    parm.beam_rad->type = TYPE_STRING;
    parm.beam_rad->required = NO;
    parm.beam_rad->gisprompt = "new,cell,raster";
    parm.beam_rad->description =
	_("Output beam irradiance [W.m-2] (mode 1) or irradiation raster map [Wh.m-2.day-1] (mode 2)");
    parm.beam_rad->guisection = _("Output_options");

    parm.insol_time = G_define_option();
    parm.insol_time->key = "insol_time";
    parm.insol_time->type = TYPE_STRING;
    parm.insol_time->required = NO;
    parm.insol_time->gisprompt = "new,cell,raster";
    parm.insol_time->description =
	_("Output insolation time raster map [h] (mode 2 only)");
    parm.insol_time->guisection = _("Output_options");

    parm.diff_rad = G_define_option();
    parm.diff_rad->key = "diff_rad";
    parm.diff_rad->type = TYPE_STRING;
    parm.diff_rad->required = NO;
    parm.diff_rad->gisprompt = "new,cell,raster";
    parm.diff_rad->description =
	_("Output diffuse irradiance [W.m-2] (mode 1) or irradiation raster map [Wh.m-2.day-1] (mode 2)");
    parm.diff_rad->guisection = _("Output_options");

    parm.refl_rad = G_define_option();
    parm.refl_rad->key = "refl_rad";
    parm.refl_rad->type = TYPE_STRING;
    parm.refl_rad->required = NO;
    parm.refl_rad->gisprompt = "new,cell,raster";
    parm.refl_rad->description =
	_("Output ground reflected irradiance [W.m-2] (mode 1) or irradiation raster map [Wh.m-2.day-1] (mode 2)");
    parm.refl_rad->guisection = _("Output_options");

    parm.day = G_define_option();
    parm.day->key = "day";
    parm.day->type = TYPE_INTEGER;
    parm.day->required = YES;
    parm.day->description = _("No. of day of the year (1-365)");

    parm.step = G_define_option();
    parm.step->key = "step";
    parm.step->type = TYPE_DOUBLE;
    parm.step->answer = STEP;
    parm.step->required = NO;
    parm.step->description =
	_("Time step when computing all-day radiation sums [decimal hours]");

    parm.declin = G_define_option();
    parm.declin->key = "declin";
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

    flag.shade = G_define_flag();
    flag.shade->key = 's';
    flag.shade->description =
	_("Incorporate the shadowing effect of terrain");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    shd = flag.shade->answer;

    elevin = parm.elevin->answer;
    aspin = parm.aspin->answer;
    slopein = parm.slopein->answer;
    linkein = parm.linkein->answer;
    albedo = parm.albedo->answer;
    latin = parm.latin->answer;
    coefbh = parm.coefbh->answer;
    coefdh = parm.coefdh->answer;
    incidout = parm.incidout->answer;
    beam_rad = parm.beam_rad->answer;
    insol_time = parm.insol_time->answer;
    diff_rad = parm.diff_rad->answer;
    refl_rad = parm.refl_rad->answer;

    if ((G_projection() == PROJECTION_LL)) {
	ll_correction = 1;
    }

    if ((insol_time != NULL) && (incidout != NULL))
	G_fatal_error(_("insol_time and incidout are incompatible options"));

    sscanf(parm.day->answer, "%d", &day);
    sscanf(parm.step->answer, "%lf", &step);

    tt = parm.ltime->answer;
    if (parm.ltime->answer != NULL) {
	if (insol_time != NULL)
	    G_fatal_error(_("time and insol_time are incompatible options"));
	G_message(_("Mode 1: instantaneous solar incidence angle & irradiance given a set local time"));
	sscanf(parm.ltime->answer, "%lf", &timo);
    }
    else {
	if (incidout != NULL)
	    G_fatal_error(_("incidout requires time parameter to be set"));
	G_message(_("Mode 2: integrated daily irradiation for a given day of the year"));
    }

    if (parm.linkein->answer == NULL)
	sscanf(parm.lin->answer, "%lf", &linke);
    if (parm.albedo->answer == NULL)
	sscanf(parm.alb->answer, "%lf", &alb);
    lt = parm.lat->answer;
    if (parm.lat->answer != NULL)
	sscanf(parm.lat->answer, "%lf", &latitude);
    if (parm.coefbh->answer == NULL)
	cbh = BSKY;
    if (parm.coefdh->answer == NULL)
	cdh = DSKY;

    stepxy = DIST * 0.5 * (stepx + stepy);

    if (parm.declin->answer == NULL)
	declination = com_declin(day);
    else {
	sscanf(parm.declin->answer, "%lf", &declin);
	declination = -declin;
    }

    sindecl = sin(declination);
    cosdecl = cos(declination);

    if (lt != NULL)
	latitude = -latitude * DEG;

    if (tt != NULL) {

	tim = (timo - 12) * 15;
	/* converting to degrees */
	/* Jenco (12-lum_time) * 15 */
	if (tim < 0)
	    tim += 360;
	tim = M_PI * tim / 180;
	/* conv. to radians */
    }

    /* Set up parameters for projection to lat/long if necessary */

    if (latin == NULL && lt == NULL && (G_projection() != PROJECTION_LL)) {

	struct Key_Value *in_proj_info, *in_unit_info;

	if ((in_proj_info = G_get_projinfo()) == NULL)
	    G_fatal_error
		(_("Can't get projection info of current location: please set latitude via 'lat' or 'latin' option!"));

	if ((in_unit_info = G_get_projunits()) == NULL)
	    G_fatal_error(_("Can't get projection units of current location"));

	if (pj_get_kv(&iproj, in_proj_info, in_unit_info) < 0)
	    G_fatal_error
		(_("Can't get projection key values of current location"));

	G_free_key_value(in_proj_info);
	G_free_key_value(in_unit_info);

	/* Set output projection to latlong w/ same ellipsoid */
	oproj.zone = 0;
	oproj.meters = 1.;
	sprintf(oproj.proj, "ll");
	if ((oproj.pj = pj_latlong_from_proj(iproj.pj)) == NULL)
	    G_fatal_error(_("Unable to set up lat/long projection parameters"));

    }

/**********end of parser - ******************************/

    INPUT();
    calculate();
    OUTGR();
    exit(EXIT_SUCCESS);
}


int INPUT(void)
{
    FCELL *cell1, *cell2;
    FCELL *cell3, *cell4, *cell5, *cell6;
    FCELL *rast1, *rast2;
    int fd1, fd2, fd3, fd4, fd5, fd6, row, row_rev;
    int fr1, fr2;
    int l, i, j;

    cell1 = G_allocate_f_raster_buf();
    cell2 = G_allocate_f_raster_buf();
    cell3 = G_allocate_f_raster_buf();

    z = (float **)G_malloc(sizeof(float *) * (m));
    o = (float **)G_malloc(sizeof(float *) * (m));
    s = (float **)G_malloc(sizeof(float *) * (m));

    for (l = 0; l < m; l++) {
	z[l] = (float *)G_malloc(sizeof(float) * (n));
	o[l] = (float *)G_malloc(sizeof(float) * (n));
	s[l] = (float *)G_malloc(sizeof(float) * (n));

    }

    if ((mapset1 = G_find_cell(elevin, "")) == NULL)
	G_fatal_error(_("elevin raster map <%s> not found"), elevin);

    if ((mapset2 = G_find_cell(aspin, "")) == NULL)
	G_fatal_error(_("aspin raster map <%s> not found"), aspin);

    if ((mapset3 = G_find_cell(slopein, "")) == NULL)
	G_fatal_error(_("slopein raster map <%s> not found"), slopein);

    fd1 = G_open_cell_old(elevin, mapset1);
    fd2 = G_open_cell_old(aspin, mapset2);
    fd3 = G_open_cell_old(slopein, mapset3);

    if (linkein != NULL) {
	cell4 = G_allocate_f_raster_buf();
	li = (float **)G_malloc(sizeof(float *) * (m));
	for (l = 0; l < m; l++)
	    li[l] = (float *)G_malloc(sizeof(float) * (n));

	if ((mapset4 = G_find_cell(linkein, "")) == NULL)
	    G_fatal_error(_("linkein raster map <%s> not found"), linkein);

	fd4 = G_open_cell_old(linkein, mapset4);
    }

    if (albedo != NULL) {
	cell5 = G_allocate_f_raster_buf();
	a = (float **)G_malloc(sizeof(float *) * (m));
	for (l = 0; l < m; l++)
	    a[l] = (float *)G_malloc(sizeof(float) * (n));

	if ((mapset5 = G_find_cell(albedo, "")) == NULL)
	    G_fatal_error(_("albedo raster map <%s> not found"), albedo);

	fd5 = G_open_cell_old(albedo, mapset5);
    }

    if (latin != NULL) {
	cell6 = G_allocate_f_raster_buf();
	la = (float **)G_malloc(sizeof(float *) * (m));
	for (l = 0; l < m; l++)
	    la[l] = (float *)G_malloc(sizeof(float) * (n));

	if ((mapset6 = G_find_cell(latin, "")) == NULL)
	    G_fatal_error(_("latin raster map <%s> not found"), latin);

	fd6 = G_open_cell_old(latin, mapset6);
    }

    if (coefbh != NULL) {
	rast1 = G_allocate_f_raster_buf();
	cbhr = (float **)G_malloc(sizeof(float *) * (m));
	for (l = 0; l < m; l++)
	    cbhr[l] = (float *)G_malloc(sizeof(float) * (n));

	if ((mapset7 = G_find_cell(coefbh, "")) == NULL)
	    G_fatal_error(_("coefbh raster map <%s> not found"), coefbh);

	fr1 = G_open_cell_old(coefbh, mapset7);
    }

    if (coefdh != NULL) {
	rast2 = G_allocate_f_raster_buf();
	cdhr = (float **)G_malloc(sizeof(float *) * (m));
	for (l = 0; l < m; l++)
	    cdhr[l] = (float *)G_malloc(sizeof(float) * (n));

	if ((mapset8 = G_find_cell(coefdh, "")) == NULL)
	    G_fatal_error(_("coefdh raster map <%s> not found"), coefdh);

	fr2 = G_open_cell_old(coefdh, mapset8);
    }


    for (row = 0; row < m; row++) {
	G_get_f_raster_row(fd1, cell1, row);
	G_get_f_raster_row(fd2, cell2, row);
	G_get_f_raster_row(fd3, cell3, row);
	if (linkein != NULL)
	    G_get_f_raster_row(fd4, cell4, row);
	if (albedo != NULL)
	    G_get_f_raster_row(fd5, cell5, row);
	if (latin != NULL)
	    G_get_f_raster_row(fd6, cell6, row);
	if (coefbh != NULL)
	    G_get_f_raster_row(fr1, rast1, row);
	if (coefdh != NULL)
	    G_get_f_raster_row(fr2, rast2, row);


	for (j = 0; j < n; j++) {
	    row_rev = m - row - 1;

	    if (!G_is_f_null_value(cell1 + j))
		z[row_rev][j] = (float)cell1[j];
	    else
		z[row_rev][j] = UNDEFZ;

	    if (!G_is_f_null_value(cell2 + j))
		o[row_rev][j] = (float)cell2[j];
	    else
		o[row_rev][j] = UNDEFZ;

	    if (!G_is_f_null_value(cell3 + j))
		s[row_rev][j] = (float)cell3[j];
	    else
		s[row_rev][j] = UNDEFZ;


	    if (linkein != NULL) {
		if (!G_is_f_null_value(cell4 + j))
		    li[row_rev][j] = (float)cell4[j];
		else
		    li[row_rev][j] = UNDEFZ;
	    }

	    if (albedo != NULL) {
		if (!G_is_f_null_value(cell5 + j))
		    a[row_rev][j] = (float)cell5[j];
		else
		    a[row_rev][j] = UNDEFZ;
	    }

	    if (latin != NULL) {
		if (!G_is_f_null_value(cell6 + j))
		    la[row_rev][j] = (float)cell6[j];
		else
		    la[row_rev][j] = UNDEFZ;
	    }

	    if (coefbh != NULL) {
		if (!G_is_f_null_value(rast1 + j))
		    cbhr[row_rev][j] = (float)rast1[j];
		else
		    cbhr[row_rev][j] = UNDEFZ;
	    }

	    if (coefdh != NULL) {
		if (!G_is_f_null_value(rast2 + j))
		    cdhr[row_rev][j] = (float)rast2[j];
		else
		    cdhr[row_rev][j] = UNDEFZ;
	    }


	}
    }
    G_close_cell(fd1);
    G_close_cell(fd2);
    G_close_cell(fd3);
    if (linkein != NULL)
	G_close_cell(fd4);
    if (albedo != NULL)
	G_close_cell(fd5);
    if (latin != NULL)
	G_close_cell(fd6);
    if (coefbh != NULL)
	G_close_cell(fr1);
    if (coefdh != NULL)
	G_close_cell(fr2);

/*******transformation of angles from 0 to east counterclock
		to 0 to north clocwise, for ori=0 upslope flowlines
		turn the orientation 2*M_PI ************/

    /* needs to be eliminated */

    for (i = 0; i < m; i++) {
	for (j = 0; j < n; j++) {
	    zmax = AMAX1(zmax, z[i][j]);
	    if (o[i][j] != 0.) {
		if (o[i][j] < 90.)
		    o[i][j] = 90. - o[i][j];
		else
		    o[i][j] = 450. - o[i][j];
		G_debug(3, "o:%f  z:%f  i:%d  j:%d", o[i][j], z[i][j], i, j);

		if (z[i][j] == UNDEFZ || o[i][j] == UNDEFZ ||
		    s[i][j] == UNDEFZ)
		    z[i][j] = UNDEFZ;
		if (linkein != NULL && li[i][j] == UNDEFZ)
		    z[i][j] = UNDEFZ;
		if (albedo != NULL && a[i][j] == UNDEFZ)
		    z[i][j] = UNDEFZ;
		if (latin != NULL && la[i][j] == UNDEFZ)
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
    FCELL *cell7, *cell8, *cell9, *cell10, *cell11;
    int fd7, fd8, fd9, fd10, fd11;
    int i, iarc, j;

    if (incidout != NULL) {
	cell7 = G_allocate_f_raster_buf();
	fd7 = G_open_fp_cell_new(incidout);
	if (fd7 < 0)
	    G_fatal_error(_("Unable to create raster map %s"), incidout);
    }

    if (beam_rad != NULL) {
	cell8 = G_allocate_f_raster_buf();
	fd8 = G_open_fp_cell_new(beam_rad);
	if (fd8 < 0)
	    G_fatal_error(_("Unable to create raster map %s"), beam_rad);
    }

    if (insol_time != NULL) {
	cell11 = G_allocate_f_raster_buf();
	fd11 = G_open_fp_cell_new(insol_time);
	if (fd11 < 0)
	    G_fatal_error(_("Unable to create raster map %s"), insol_time);
    }

    if (diff_rad != NULL) {
	cell9 = G_allocate_f_raster_buf();
	fd9 = G_open_fp_cell_new(diff_rad);
	if (fd9 < 0)
	    G_fatal_error(_("Unable to create raster map %s"), diff_rad);
    }

    if (refl_rad != NULL) {
	cell10 = G_allocate_f_raster_buf();
	fd10 = G_open_fp_cell_new(refl_rad);
	if (fd10 < 0)
	    G_fatal_error(_("Unable to create raster map %s"), refl_rad);
    }


    if (G_set_window(&cellhd) < 0)
	G_fatal_error("region error");

    if (m != G_window_rows())
	G_fatal_error(_("rows changed from %d to %d"), m, G_window_rows());

    if (n != G_window_cols())
	G_fatal_error(_("cols changed from %d to %d"), n, G_window_cols());


    for (iarc = 0; iarc < m; iarc++) {
	i = m - iarc - 1;
	if (incidout != NULL) {
	    for (j = 0; j < n; j++) {
		if (lumcl[i][j] == UNDEFZ)
		    G_set_f_null_value(cell7 + j, 1);
		else
		    cell7[j] = (FCELL) lumcl[i][j];
	    }
	    G_put_f_raster_row(fd7, cell7);
	}

	if (beam_rad != NULL) {
	    for (j = 0; j < n; j++) {
		if (beam[i][j] == UNDEFZ)
		    G_set_f_null_value(cell8 + j, 1);
		else
		    cell8[j] = (FCELL) beam[i][j];

	    }
	    G_put_f_raster_row(fd8, cell8);
	}

	if (insol_time != NULL) {
	    for (j = 0; j < n; j++) {
		if (insol[i][j] == UNDEFZ)
		    G_set_f_null_value(cell11 + j, 1);
		else
		    cell11[j] = (FCELL) insol[i][j];
	    }
	    G_put_f_raster_row(fd11, cell11);
	}


	if (diff_rad != NULL) {
	    for (j = 0; j < n; j++) {
		if (diff[i][j] == UNDEFZ)
		    G_set_f_null_value(cell9 + j, 1);
		else
		    cell9[j] = (FCELL) diff[i][j];
	    }
	    G_put_f_raster_row(fd9, cell9);
	}

	if (refl_rad != NULL) {
	    for (j = 0; j < n; j++) {
		if (refl[i][j] == UNDEFZ)
		    G_set_f_null_value(cell10 + j, 1);
		else
		    cell10[j] = (FCELL) refl[i][j];
	    }
	    G_put_f_raster_row(fd10, cell10);
	}

    }

    if (incidout != NULL) {
	G_close_cell(fd7);
	G_write_history(incidout, &hist);
    }
    if (beam_rad != NULL) {
	G_close_cell(fd8);
	G_write_history(beam_rad, &hist);
    }
    if (diff_rad != NULL) {
	G_close_cell(fd9);
	G_write_history(diff_rad, &hist);
    }
    if (refl_rad != NULL) {
	G_close_cell(fd10);
	G_write_history(refl_rad, &hist);
    }
    if (insol_time != NULL) {
	G_close_cell(fd11);
	G_write_history(insol_time, &hist);
    }

    return 1;
}


void com_par_const(void)
{
    double pom;

    lum_C11 = sinlat * cosdecl;
    lum_C13 = -coslat * sindecl;
    lum_C22 = cosdecl;
    lum_C31 = coslat * cosdecl;
    lum_C33 = sinlat * sindecl;

    if (fabs(lum_C31) >= EPS) {
	pom = -lum_C33 / lum_C31;
	if (fabs(pom) <= 1) {
	    pom = acos(pom);
	    pom = (pom * 180) / M_PI;
	    sunrise_time = (90 - pom) / 15 + 6;
	    sunset_time = (pom - 90) / 15 + 18;
	}
	else {
	    if (pom < 0) {
		/*        printf("\n Sun is ABOVE the surface during the whole day\n"); */
		sunrise_time = 0;
		sunset_time = 24;
		if (fabs(pom) - 1 <= EPS)
		    printf("\texcept at midnight is sun ON THE HORIZONT\n");
	    }
	    else {
		/*                printf("\n The sun is BELOW the surface during the whole day\n"); */
		if (fabs(pom) - 1 <= EPS) {
		    printf("\texcept at noon is sun ON HORIZONT\n");
		    sunrise_time = 12;
		    sunset_time = 12;
		}
	    }
	}
    }

}


void com_par(void)
{
    double old_time, pom, xpom, ypom;

    double coslum_time;

    coslum_time = cos(lum_time);

    old_time = lum_time;


    lum_Lx = -lum_C22 * sin(lum_time);
    lum_Ly = lum_C11 * coslum_time + lum_C13;
    lum_Lz = lum_C31 * coslum_time + lum_C33;

    if (fabs(lum_C31) < EPS) {
	if (fabs(lum_Lz) >= EPS) {
	    if (lum_Lz > 0) {
		/*                        printf("\tSun is ABOVE area during the whole day\n"); */
		sunrise_time = 0;
		sunset_time = 24;
	    }
	    else {
		h0 = 0.;
		A0 = UNDEF;
		return;
	    }
	}
	else {
	    /*                      printf("\tThe Sun is ON HORIZON during the whole day\n"); */
	    sunrise_time = 0;
	    sunset_time = 24;
	}
    }

    h0 = asin(lum_Lz);		/* vertical angle of the sun */
    /* lum_Lz is sin(h0) */

    xpom = lum_Lx * lum_Lx;
    ypom = lum_Ly * lum_Ly;
    pom = sqrt(xpom + ypom);

    sr_min = AMIN1(sr_min, sunrise_time);
    sr_max = AMAX1(sr_max, sunrise_time);
    ss_min = AMIN1(ss_min, sunset_time);
    ss_max = AMAX1(ss_max, sunset_time);

    if (fabs(pom) > EPS) {
	A0 = lum_Ly / pom;
	A0 = acos(A0);		/* horiz. angle of the Sun */
	/*                      A0 *= RAD; */
	if (lum_Lx < 0)
	    A0 = M2_PI - A0;
    }
    else {
	A0 = UNDEF;
	if (h0 > 0)
	    printf("A0 = Zenit\n");
	else
	    printf("A0 = Nadir\n");
    }

    if (A0 < 0.5 * M_PI)
	angle = 0.5 * M_PI - A0;
    else
	angle = 2.5 * M_PI - A0;

    stepsinangle = stepxy * sin(angle);
    stepcosangle = stepxy * cos(angle);
    tanh0 = tan(h0);

}

/**********************************************************/

double lumcline2(void)
{
    double s = 0;
    int r = 0;


    func = cube;
    tien = 0;

    if (shd == 1) {
	length = 0;

	while ((r = searching()) == 1) {
	    if (r == 3)
		break;		/* no test is needed */
	}
    }

    xx0 = xg0;
    yy0 = yg0;


    if (r == 2) {
	tien = 1;		/* shadow */
    }
    else {

	if (z_orig != UNDEFZ) {
	    s = lum_C31_l * cos(-lum_time - longit_l) + lum_C33_l;	/* Jenco */
	}
	else {
	    s = lum_Lz;
	}
    }

    if (s < 0)
	return 0.;
    return (s);
}

void joules2(void)
{

    double s0, dfr, dfr_rad, dfr1_rad, dfr2_rad, fr1, fr2, dfr1, dfr2;
    double ra, dra, ss_rad = 0., sr_rad;
    int i1, i2, ss = 1, ss0 = 1;

    beam_e = 0.;
    diff_e = 0.;
    refl_e = 0.;
    insol_t = 0.;
    tien = 0;

    if (tt == NULL)
	lum_time = 0.;

    com_par_const();
    com_par();

    if (tt != NULL) {		/*irradiance */

	s0 = lumcline2();

	if (h0 > 0.) {
	    if (tien != 1 && s0 > 0.) {
		ra = brad(s0);	/* beam radiation */
		beam_e += ra;
	    }
	    else {
		beam_e = 0.;
		bh = 0.;
	    }

	    if (diff_rad != NULL) {
		dra = drad(s0);	/* diffuse rad. */
		diff_e += dra;
	    }
	    if (refl_rad != NULL) {
		if (diff_rad == NULL)
		    drad(s0);
		refl_e += rr;	/* reflected rad. */
	    }
	}			/* h0 */
    }
    else {
	/* all-day radiation */


	i1 = (int)sunrise_time;
	fr1 = sunrise_time - i1;
	if (fr1 > 0.)
	    fr1 = 1 - fr1;
	else
	    fr1 = -fr1;

	dfr1 = fr1;
	while (dfr1 > step) {
	    dfr1 = dfr1 - step;
	}

	i2 = (int)sunset_time;
	fr2 = sunset_time - i2;

	dfr2 = fr2;
	while (dfr2 > step) {
	    dfr2 = dfr2 - step;
	}

	sr_rad = (sunrise_time - 12.) * 15.;
	if (ss_rad < 0)
	    sr_rad += 360;
	sr_rad = sr_rad * DEG;
	ss_rad = (sunset_time - 12.) * 15.;
	if (ss_rad < 0)
	    ss_rad += 360;
	ss_rad = ss_rad * DEG;

	dfr1_rad = dfr1 * 15. * DEG;
	dfr2_rad = dfr2 * 15. * DEG;
	dfr_rad = step * 15. * DEG;

	lum_time = sr_rad + dfr1_rad / 2.;
	dfr = dfr1;

	while (ss == 1) {

	    com_par();
	    s0 = lumcline2();

	    if (h0 > 0.) {

		if (tien != 1 && s0 > 0.) {
		    insol_t += dfr;
		    ra = brad(s0);
		    beam_e += dfr * ra;
		    ra = 0.;
		}
		else
		    bh = 0.;
		if (diff_rad != NULL) {
		    dra = drad(s0);
		    diff_e += dfr * dra;
		    dra = 0.;
		}
		if (refl_rad != NULL) {
		    if (diff_rad == NULL)
			drad(s0);
		    refl_e += dfr * rr;
		    rr = 0.;
		}
	    }			/* illuminated */

	    if (ss0 == 0)
		return;

	    if (dfr < step) {
		dfr = step;
		lum_time = lum_time + dfr1_rad / 2. + dfr_rad / 2.;
	    }
	    else {
		lum_time = lum_time + dfr_rad;
	    }
	    if (lum_time > ss_rad - dfr2_rad / 2.) {
		dfr = dfr2;
		lum_time = ss_rad - dfr2_rad / 2.;
		ss0 = 0;	/* we've got the sunset */
	    }
	}			/* end of while */
    }				/* all-day radiation */

}

/*////////////////////////////////////////////////////////////////////// */


int new_point(void)
{

    yy0 += stepsinangle;
    xx0 += stepcosangle;
    if ((xx0 < 0) || (xx0 > deltx) || (yy0 < 0) || (yy0 > delty))
	return (3);
    else
	return (1);
}

void where_is_point(void)
{
    double sx, sy;
    double dx, dy;
    int i, j;

    sx = xx0 * invstepx + 0.5;	/* offset 0.5 cell size to get the right cell i, j */
    sy = yy0 * invstepy + 0.5;

    i = (int)sx;
    j = (int)sy;

    if (i <= n - 1 && j <= m - 1) {

	dx = (double)i *stepx;
	dy = (double)j *stepy;

	length = distance(xg0, dx, yg0, dy);	/* dist from orig. grid point to the current grid point */

	cube(j, i);
	return;

    }
    else
	func = NULL;
}

void cube(int jmin, int imin)
{
    zp = z[jmin][imin];
    if ((zp == UNDEFZ))
	func = NULL;
}

int searching(void)
{
    double z2;
    double curvature_diff;
    int succes = 0;

    if (zp == UNDEFZ)
	return 0;

    succes = new_point();
    if (succes == 1) {
	where_is_point();
	if (func == NULL)
	    return (3);
	/*      length += stepxy; */

	curvature_diff = EARTHRADIUS * (1. - cos(length / EARTHRADIUS));
	z2 = z_orig + curvature_diff + length * tanh0;	/* also corrected to the earth's curvature */

	if (z2 < zp)
	    succes = 2;		/* shadow */
	if (z2 > zmax)
	    succes = 3;		/* no test needed all visible */
    }

    return (succes);
}



/*////////////////////////////////////////////////////////////////////// */

void calculate(void)
{
    int i, j, l;

    /* double energy; */
    double lum, q1;

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

    c = com_sol_const(day);



    for (j = 0; j < m; j++) {
	G_percent(j, m - 1, 2);
	for (i = 0; i < n; i++) {

	    xg0 = xx0 = (double)i *stepx;

	    xp = xmin + xx0;
	    yg0 = yy0 = (double)j *stepy;

	    yp = ymin + yy0;
	    func = NULL;
	    length = 0;

	    if (ll_correction) {
		coslat = cos(deg2rad * yp);
		coslatsq = coslat * coslat;
	    }

	    z_orig = z1 = zp = z[j][i];
	    o_orig = o[j][i];

	    if (z_orig != UNDEFZ) {
		if (o[j][i] != 0.)
		    aspect = o[j][i] * DEG;
		else
		    aspect = UNDEF;
		slope = s[j][i] * DEG;
		if (linkein != NULL) {
		    linke = li[j][i];
		    li_max = AMAX1(li_max, linke);
		    li_min = AMIN1(li_min, linke);
		}
		if (albedo != NULL) {
		    alb = a[j][i];
		    al_max = AMAX1(al_max, alb);
		    al_min = AMIN1(al_min, alb);
		}
		if (latin != NULL) {
		    latitude = la[j][i];
		    la_max = AMAX1(la_max, latitude);
		    la_min = AMIN1(la_min, latitude);
		    latitude = -latitude * DEG;
		}
		if (latin == NULL && lt == NULL) {
		    if ((G_projection() != PROJECTION_LL)) {

			longitude = xp;
			latitude = yp;

			if (pj_do_proj(&longitude, &latitude, &iproj, &oproj)
			    < 0) {
			    G_fatal_error(_("Error in pj_do_proj"));
			}

			la_max = AMAX1(la_max, latitude);
			la_min = AMIN1(la_min, latitude);
			latitude = -latitude * DEG;
		    }
		    else {	/* ll projection */
			latitude = yp;
			la_max = AMAX1(la_max, latitude);
			la_min = AMIN1(la_min, latitude);
			latitude = -latitude * DEG;
		    }
		}

		if (coefbh != NULL) {
		    cbh = cbhr[j][i];
		}
		if (coefdh != NULL) {
		    cdh = cdhr[j][i];
		}
		cos_u = cos(M_PI / 2 - slope);
		sin_u = sin(M_PI / 2 - slope);
		cos_v = cos(M_PI / 2 + aspect);
		sin_v = sin(M_PI / 2 + aspect);

		if (tt != NULL)
		    lum_time = tim;

		sinlat = sin(latitude);
		coslat = cos(latitude);

		sin_phi_l = -coslat * cos_u * sin_v + sinlat * sin_u;
		latid_l = asin(sin_phi_l);

		q1 = sinlat * cos_u * sin_v + coslat * sin_u;
		tan_lam_l = -cos_u * cos_v / q1;
		longit_l = atan(tan_lam_l);
		lum_C31_l = cos(latid_l) * cosdecl;
		lum_C33_l = sin_phi_l * sindecl;


		if (incidout != NULL) {
		    com_par_const();
		    com_par();
		    lum = lumcline2();
		    lum = RAD * asin(lum);
		    lumcl[j][i] = (float)lum;
		}
		if ((beam_rad != NULL) || (insol_time != NULL) ||
		    (diff_rad != NULL) || (refl_rad != NULL)) {
		    joules2();
		    if (beam_rad != NULL)
			beam[j][i] = (float)beam_e;
		    if (insol_time != NULL)
			insol[j][i] = (float)insol_t;
		    /*      printf("\n %f",insol[j][i]); */
		    if (diff_rad != NULL)
			diff[j][i] = (float)diff_e;
		    if (refl_rad != NULL)
			refl[j][i] = (float)refl_e;
		}

	    }			/* undefs */
	}

    }

    G_short_history("r.sun solar model output", "raster", &hist);

    sprintf(hist.edhist[0],
	    " ----------------------------------------------------------------");
    sprintf(hist.edhist[1], " Day [1-365]:                              %d",
	    day);
    hist.edlinecnt = 2;

    if (tt != NULL) {
	sprintf(hist.edhist[hist.edlinecnt],
		" Local (solar) time (decimal hr.):         %.4f", timo);
	hist.edlinecnt++;
    }

    sprintf(hist.edhist[hist.edlinecnt],
	    " Solar constant (W/m^2):                   1367");
    sprintf(hist.edhist[hist.edlinecnt + 1],
	    " Extraterrestrial irradiance (W/m^2):      %f", c);
    sprintf(hist.edhist[hist.edlinecnt + 2],
	    " Declination (rad):                        %f", -declination);
    hist.edlinecnt += 3;

    if (lt != NULL)
	sprintf(hist.edhist[hist.edlinecnt],
		" Latitude (deg):                           %.4f",
		-latitude * RAD);
    else
	sprintf(hist.edhist[hist.edlinecnt],
		" Latitude min-max(deg):                    %.4f - %.4f",
		la_min, la_max);
    hist.edlinecnt++;

    if (tt != NULL) {
	sprintf(hist.edhist[hist.edlinecnt],
		" Sunrise time (hr.):                       %.2f",
		sunrise_time);
	sprintf(hist.edhist[hist.edlinecnt + 1],
		" Sunset time (hr.):                        %.2f",
		sunset_time);
	sprintf(hist.edhist[hist.edlinecnt + 2],
		" Daylight time (hr.):                      %.2f",
		sunset_time - sunrise_time);
    }
    else {
	sprintf(hist.edhist[hist.edlinecnt],
		" Sunrise time min-max (hr.):               %.2f - %.2f",
		sr_min, sr_max);
	sprintf(hist.edhist[hist.edlinecnt + 1],
		" Sunset time min-max (hr.):                %.2f - %.2f",
		ss_min, ss_max);
	sprintf(hist.edhist[hist.edlinecnt + 2],
		" Time step (hr.):                          %.4f", step);
    }
    hist.edlinecnt += 3;

    if (incidout != NULL || tt != NULL) {
	sprintf(hist.edhist[hist.edlinecnt],
		" Solar altitude (deg):                     %.4f", h0 * RAD);
	sprintf(hist.edhist[hist.edlinecnt + 1],
		" Solar azimuth (deg):                      %.4f", A0 * RAD);
	hist.edlinecnt += 2;
    }

    if (linkein == NULL)
	sprintf(hist.edhist[hist.edlinecnt],
		" Linke turbidity factor:                   %.1f", linke);
    else
	sprintf(hist.edhist[hist.edlinecnt],
		" Linke turbidity factor min-max:           %.1f-%.1f",
		li_min, li_max);
    hist.edlinecnt++;

    if (albedo == NULL)
	sprintf(hist.edhist[hist.edlinecnt],
		" Ground albedo:                            %.3f", alb);
    else
	sprintf(hist.edhist[hist.edlinecnt],
		" Ground albedo min-max:                    %.3f-%.3f",
		al_min, al_max);
    hist.edlinecnt++;

    sprintf(hist.edhist[hist.edlinecnt],
	    " -----------------------------------------------------------------");
    hist.edlinecnt++;

    G_command_history(&hist);
    /* don't call G_write_history() until after G_close_cell() or it just gets overwritten */
}

double com_sol_const(int no_of_day)
{
    double I0, d1;

    /*  v W/(m*m) */
    d1 = M2_PI * no_of_day / 365.25;
    I0 = 1367. * (1 + 0.03344 * cos(d1 - 0.048869));

    return I0;
}


double com_declin(int no_of_day)
{
    double d1, decl;

    d1 = M2_PI * no_of_day / 365.25;
    decl = asin(0.3978 * sin(d1 - 1.4 + 0.0355 * sin(d1 - 0.0489)));
    decl = -decl;
    /*      printf(" declination : %lf\n", decl); */

    return (decl);
}

double brad(double sh)
{
    double p, lm, tl, rayl, br;
    double drefract, temp1, temp2, h0refract;

    p = exp(-z_orig / 8434.5);
    temp1 = 0.1594 + h0 * (1.123 + 0.065656 * h0);
    temp2 = 1. + h0 * (28.9344 + 277.3971 * h0);
    drefract = 0.061359 * temp1 / temp2;	/* in radians */
    h0refract = h0 + drefract;
    lm = p / (sin(h0refract) +
	      0.50572 * pow(h0refract * RAD + 6.07995, -1.6364));
    tl = 0.8662 * linke;
    if (lm <= 20.)
	rayl =
	    1. / (6.6296 +
		  lm * (1.7513 +
			lm * (-0.1202 + lm * (0.0065 - lm * 0.00013))));
    else
	rayl = 1. / (10.4 + 0.718 * lm);
    bh = cbh * c * lum_Lz * exp(-rayl * lm * tl);
    if (aspect != UNDEF && slope != 0.)
	br = bh * sh / lum_Lz;
    else
	br = bh;

    return (br);
}

double drad(double sh)
{
    double tn, fd, fx = 0., A1, A2, A3, A1b;
    double r_sky, kb, dr, gh, a_ln, ln, fg;
    double cosslope, sinslope;

    cosslope = cos(slope);
    sinslope = sin(slope);

    tn = -0.015843 + linke * (0.030543 + 0.0003797 * linke);
    A1b = 0.26463 + linke * (-0.061581 + 0.0031408 * linke);
    if (A1b * tn < 0.0022)
	A1 = 0.0022 / tn;
    else
	A1 = A1b;
    A2 = 2.04020 + linke * (0.018945 - 0.011161 * linke);
    A3 = -1.3025 + linke * (0.039231 + 0.0085079 * linke);

    fd = A1 + A2 * lum_Lz + A3 * lum_Lz * lum_Lz;
    dh = cdh * c * fd * tn;
    gh = bh + dh;
    if (aspect != UNDEF && slope != 0.) {
	kb = bh / (c * lum_Lz);
	r_sky = (1. + cosslope) / 2.;
	a_ln = A0 - aspect;
	ln = a_ln;
	if (a_ln > M_PI)
	    ln = a_ln - M2_PI;
	else if (a_ln < -M_PI)
	    ln = a_ln + M2_PI;
	a_ln = ln;
	fg = sinslope - slope * cosslope -
	    M_PI * sin(slope / 2.) * sin(slope / 2.);
	if (tien == 1 || sh <= 0.)
	    fx = r_sky + fg * 0.252271;
	else if (h0 >= 0.1) {
	    fx = ((0.00263 - kb * (0.712 + 0.6883 * kb)) * fg + r_sky) * (1. -
									  kb)
		+ kb * sh / lum_Lz;
	}
	else if (h0 < 0.1)
	    fx = ((0.00263 - 0.712 * kb - 0.6883 * kb * kb) * fg +
		  r_sky) * (1. - kb) + kb * sin(slope) * cos(a_ln) / (0.1 -
								      0.008 *
								      h0);
	dr = dh * fx;
	/* refl. rad */
	rr = alb * gh * (1 - cos(slope)) / 2.;
    }
    else {			/* plane */
	dr = dh;
	rr = 0.;
    }
    return (dr);
}

int test(void)
{
    /* not finshed yet */
    int dej;

    printf("\n ddd: %f", declin);
    dej = asin(-declin / 0.4093) * 365. / M2_PI + 81;
    /*        dej = asin(-declin/23.35 * DEG) / 0.9856 - 284; */
    /*      dej = dej - 365; */
    printf("\n d: %d ", dej);
    if (dej < day - 5 || dej > day + 5)
	return 0;
    else
	return 1;
}

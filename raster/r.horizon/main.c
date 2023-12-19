/*******************************************************************************
r.horizon: This module does one of two things:

1) Using a map of the terrain elevation it calculates for a set of points
the angle height of the horizon for each point, using an angle interval given
by the user.

2) For a given minimum angle it calculates one or more raster map giving the
mazimum distance to a point on the horizon.

This program was written in 2006 by Thomas Huld and Tomas Cebecauer,
Joint Research Centre of the European Commission, based on bits of the r.sun
module by Jaro Hofierka

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/gprojects.h>
#include <grass/glocale.h>

#define WHOLE_RASTER      1
#define SINGLE_POINT      0
#define RAD               (180. / M_PI)
#define DEG               ((M_PI) / 180.)
#define EARTHRADIUS       6371000.
#define UNDEF             0.     /* undefined value for terrain aspect */
#define UNDEFZ            -9999. /* undefined value for elevation */
#define BIG               1.e20
#define SMALL             1.e-20
#define EPS               1.e-4
#define DIST              "1.0"
#define DEGREEINMETERS    111120.  /* 1852m/nm * 60nm/degree = 111120 m/deg */
#define TANMINANGLE       0.008727 /* tan of minimum horizon angle (0.5 deg) */

#define AMAX1(arg1, arg2) ((arg1) >= (arg2) ? (arg1) : (arg2))
#define DISTANCE1(x1, x2, y1, y2) \
    (sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2)))

FILE *fp;

const double pihalf = M_PI * 0.5;
const double twopi = M_PI * 2.;
const double invEarth = 1. / EARTHRADIUS;
const double deg2rad = M_PI / 180.;
const double rad2deg = 180. / M_PI;
const double minAngle = DEG;

const char *elevin;
const char *horizon_basename = NULL;
const char *mapset = NULL;
const char *per;
char *shad_filename;
char *outfile;

struct Cell_head cellhd;
struct pj_info iproj, oproj, tproj;

struct Cell_head new_cellhd;
typedef struct {
    double xg0, yg0;
    double z_orig;
    double coslatsq;
} OriginPoint;

typedef struct {
    double stepsinangle, stepcosangle;
    double sinangle, cosangle;
    double distsinangle, distcosangle;
} OriginAngle;

typedef struct {
    double xx0, yy0;
    int ip, jp;
    int ip100, jp100;
    double zp;
} SearchPoint;

typedef struct {
    double tanh0;
    double length;
} HorizonProperties;

int INPUT(void);
int OUTGR(int numrows, int numcols);
double amax1(double, double);
int min(int, int);
void com_par(OriginAngle *origin_angle, double, double, double);
double horizon_height(const OriginPoint *origin_point,
                      const OriginAngle *origin_angle);
void calculate_point_mode(double xcoord, double ycoord,
                          double single_direction);
int new_point(const OriginPoint *origin_point, const OriginAngle *origin_angle,
              SearchPoint *search_point, HorizonProperties *horizon);
int test_low_res(const OriginPoint *origin_point,
                 const OriginAngle *origin_angle, SearchPoint *search_point,
                 const HorizonProperties *horizon);
void calculate_raster_mode(int buffer_e, int buffer_w, int buffer_s,
                           int buffer_n, double bufferZone,
                           double single_direction);

int n, m, m100, n100;
int degreeOutput, compassOutput = FALSE;
float **z, **z100, **horizon_raster;
double stepx, stepy, stepxy;
double invstepx, invstepy, distxy;
double offsetx, offsety;

/*int arrayNumInt; */
double xmin, xmax, ymin, ymax, zmax = 0.;

double maxlength = BIG;
double fixedMaxLength = BIG, step = 0.0, start = 0.0, end = 0.0;
const char *str_step;
int ll_correction = FALSE;

/* why not use G_distance() here which switches to geodesic/great
   circle distance as needed? */
double distance(double x1, double x2, double y1, double y2, double coslatsq)
{
    if (ll_correction) {
        return DEGREEINMETERS *
               sqrt(coslatsq * (x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
    }
    else {
        return sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
    }
}

int main(int argc, char *argv[])
{
    double xcoord, ycoord;

    struct GModule *module;
    struct {
        struct Option *elevin, *dist, *coord, *direction, *horizon, *step,
            *start, *end, *bufferzone, *e_buff, *w_buff, *n_buff, *s_buff,
            *maxdistance, *output;
    } parm;

    struct {
        struct Flag *degreeOutput, *compassOutput;
    } flag;

    G_gisinit(argv[0]);
    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("solar"));
    G_add_keyword(_("sun position"));
    module->label =
        _("Computes horizon angle height from a digital elevation model.");
    module->description =
        _("The module has two "
          "different modes of operation: "
          "1. Computes the entire horizon around a single point whose "
          "coordinates are "
          "given with the 'coord' option. The horizon height (in radians). "
          "2. Computes one or more raster maps of the horizon height in a "
          "single direction. "
          "The input for this is the angle (in degrees), which is measured "
          "counterclockwise with east=0, north=90 etc. The output is the "
          "horizon height in radians.");

    parm.elevin = G_define_standard_option(G_OPT_R_ELEV);
    parm.elevin->guisection = _("Input");

    parm.direction = G_define_option();
    parm.direction->key = "direction";
    parm.direction->type = TYPE_DOUBLE;
    parm.direction->required = NO;
    parm.direction->description =
        _("Direction in which you want to know the horizon height");
    parm.direction->guisection = _("Input");

    parm.step = G_define_option();
    parm.step->key = "step";
    parm.step->type = TYPE_DOUBLE;
    parm.step->required = NO;
    parm.step->description =
        _("Angle step size for multidirectional horizon [degrees]");
    parm.step->guisection = _("Input");

    parm.start = G_define_option();
    parm.start->key = "start";
    parm.start->type = TYPE_DOUBLE;
    parm.start->answer = "0.0";
    parm.start->required = NO;
    parm.start->description =
        _("Start angle for multidirectional horizon [degrees]");
    parm.start->guisection = _("Raster mode");

    parm.end = G_define_option();
    parm.end->key = "end";
    parm.end->type = TYPE_DOUBLE;
    parm.end->answer = "360.0";
    parm.end->required = NO;
    parm.end->description =
        _("End angle for multidirectional horizon [degrees]");
    parm.end->guisection = _("Raster mode");

    parm.bufferzone = G_define_option();
    parm.bufferzone->key = "bufferzone";
    parm.bufferzone->type = TYPE_DOUBLE;
    parm.bufferzone->required = NO;
    parm.bufferzone->description =
        _("For horizon rasters, read from the DEM an extra buffer around the "
          "present region");
    parm.bufferzone->guisection = _("Raster mode");

    parm.e_buff = G_define_option();
    parm.e_buff->key = "e_buff";
    parm.e_buff->type = TYPE_DOUBLE;
    parm.e_buff->required = NO;
    parm.e_buff->description = _("For horizon rasters, read from the DEM an "
                                 "extra buffer eastward the present region");
    parm.e_buff->guisection = _("Raster mode");

    parm.w_buff = G_define_option();
    parm.w_buff->key = "w_buff";
    parm.w_buff->type = TYPE_DOUBLE;
    parm.w_buff->required = NO;
    parm.w_buff->description = _("For horizon rasters, read from the DEM an "
                                 "extra buffer westward the present region");
    parm.w_buff->guisection = _("Raster mode");

    parm.n_buff = G_define_option();
    parm.n_buff->key = "n_buff";
    parm.n_buff->type = TYPE_DOUBLE;
    parm.n_buff->required = NO;
    parm.n_buff->description = _("For horizon rasters, read from the DEM an "
                                 "extra buffer northward the present region");
    parm.n_buff->guisection = _("Raster mode");

    parm.s_buff = G_define_option();
    parm.s_buff->key = "s_buff";
    parm.s_buff->type = TYPE_DOUBLE;
    parm.s_buff->required = NO;
    parm.s_buff->description = _("For horizon rasters, read from the DEM an "
                                 "extra buffer southward the present region");
    parm.s_buff->guisection = _("Raster mode");

    parm.maxdistance = G_define_option();
    parm.maxdistance->key = "maxdistance";
    parm.maxdistance->type = TYPE_DOUBLE;
    parm.maxdistance->required = NO;
    parm.maxdistance->description =
        _("The maximum distance to consider when finding the horizon height");
    parm.maxdistance->guisection = _("Optional");

    parm.horizon = G_define_standard_option(G_OPT_R_BASENAME_OUTPUT);
    parm.horizon->required = NO;
    parm.horizon->guisection = _("Raster mode");

    parm.coord = G_define_standard_option(G_OPT_M_COORDS);
    parm.coord->description =
        _("Coordinate for which you want to calculate the horizon");
    parm.coord->guisection = _("Point mode");

    parm.dist = G_define_option();
    parm.dist->key = "distance";
    parm.dist->type = TYPE_DOUBLE;
    parm.dist->answer = DIST;
    parm.dist->required = NO;
    parm.dist->description = _("Sampling distance step coefficient (0.5-1.5)");
    parm.dist->guisection = _("Optional");

    parm.output = G_define_standard_option(G_OPT_F_OUTPUT);
    parm.output->key = "file";
    parm.output->required = NO;
    parm.output->answer = "-";
    parm.output->description =
        _("Name of file for output (use output=- for stdout)");
    parm.output->guisection = _("Point mode");

    flag.degreeOutput = G_define_flag();
    flag.degreeOutput->key = 'd';
    flag.degreeOutput->description =
        _("Write output in degrees (default is radians)");

    flag.compassOutput = G_define_flag();
    flag.compassOutput->key = 'c';
    flag.compassOutput->description =
        _("Write output in compass orientation (default is CCW, East=0)");

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    G_get_set_window(&cellhd);

    stepx = cellhd.ew_res;
    stepy = cellhd.ns_res;
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

    n /*n_cols */ = cellhd.cols;
    m /*n_rows */ = cellhd.rows;

    n100 = ceil(n / 100.);
    m100 = ceil(m / 100.);

    xmin = cellhd.west;
    ymin = cellhd.south;
    xmax = cellhd.east;
    ymax = cellhd.north;

    degreeOutput = flag.degreeOutput->answer;
    compassOutput = flag.compassOutput->answer;

    if (G_projection() == PROJECTION_LL)
        G_important_message(_("Note: In latitude-longitude coordinate system "
                              "specify buffers in degree unit"));

    elevin = parm.elevin->answer;

    int mode;
    if (parm.coord->answer == NULL) {
        G_debug(1, "Setting mode: WHOLE_RASTER");
        mode = WHOLE_RASTER;
    }
    else {
        G_debug(1, "Setting mode: SINGLE_POINT");
        mode = SINGLE_POINT;
        if (sscanf(parm.coord->answer, "%lf,%lf", &xcoord, &ycoord) != 2) {
            G_fatal_error(_(
                "Can't read the coordinates from the \"coordinate\" option."));
        }

        if (xcoord < cellhd.west || xcoord >= cellhd.east ||
            ycoord <= cellhd.south || ycoord > cellhd.north) {
            G_fatal_error(_("Coordinates are outside of the current region"));
        }

        /* Transform the coordinates to row/column */

        /*
           xcoord = (int) ((xcoord-xmin)/stepx);
           ycoord = (int) ((ycoord-ymin)/stepy);
         */

        /* Open ASCII file for output or stdout */
        outfile = parm.output->answer;

        if ((strcmp("-", outfile)) == 0) {
            fp = stdout;
        }
        else if (NULL == (fp = fopen(outfile, "w")))
            G_fatal_error(_("Unable to open file <%s>"), outfile);
    }
    double single_direction = 0;
    if (parm.direction->answer != NULL)
        sscanf(parm.direction->answer, "%lf", &single_direction);

    if (WHOLE_RASTER == mode) {
        if ((parm.direction->answer == NULL) && (parm.step->answer == NULL)) {
            G_fatal_error(_("You didn't specify a direction value or step "
                            "size. Aborting."));
        }

        if (parm.horizon->answer == NULL) {
            G_fatal_error(
                _("You didn't specify a horizon raster name. Aborting."));
        }
        horizon_basename = parm.horizon->answer;
        if (parm.step->answer != NULL) {
            str_step = parm.step->answer;
            sscanf(parm.step->answer, "%lf", &step);
        }
        else {
            step = 0;
            str_step = "0";
        }
        sscanf(parm.start->answer, "%lf", &start);
        sscanf(parm.end->answer, "%lf", &end);
        if (start < 0.0) {
            G_fatal_error(
                _("Negative values of start angle are not allowed. Aborting."));
        }
        if (end < 0.0 || end > 360.0) {
            G_fatal_error(_("End angle is not between 0 and 360. Aborting."));
        }
        if (start >= end) {
            G_fatal_error(
                _("You specify a start grater than the end angle. Aborting."));
        }
        G_debug(1, "Angle step: %g, start: %g, end: %g", step, start, end);
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
    double bufferZone = 0., ebufferZone = 0., wbufferZone = 0.,
           nbufferZone = 0., sbufferZone = 0.;
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
            G_fatal_error(_("Could not read %s bufferzone size. Aborting."),
                          _("south"));
    }

    if (parm.n_buff->answer != NULL) {
        if (sscanf(parm.n_buff->answer, "%lf", &nbufferZone) != 1)
            G_fatal_error(_("Could not read %s bufferzone size. Aborting."),
                          _("north"));
    }

    if (parm.maxdistance->answer != NULL) {
        if (sscanf(parm.maxdistance->answer, "%lf", &fixedMaxLength) != 1)
            G_fatal_error(_("Could not read maximum distance. Aborting."));
    }
    G_debug(1, "Using maxdistance %f", fixedMaxLength); /* predefined as BIG */

    /* TODO: fixing BIG, there is a bug with distant mountains not being seen:
       attempt to contrain to current region

       fixedMaxLength = (fixedMaxLength < AMAX1(deltx, delty)) ? fixedMaxLength
       : AMAX1(deltx, delty); G_debug(1,"Using maxdistance %f", fixedMaxLength);
     */
    double dist;
    sscanf(parm.dist->answer, "%lf", &dist);
    if (dist < 0.5 || dist > 1.5)
        G_fatal_error(_("The distance value must be 0.5-1.5. Aborting."));

    stepxy = dist * 0.5 * (stepx + stepy);
    distxy = dist;

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

        n /* n_cols */ = new_cellhd.cols;
        m /* n_rows */ = new_cellhd.rows;
        G_debug(1, "%lf %lf %lf %lf \n", ymax, ymin, xmin, xmax);
        n100 = ceil(n / 100.);
        m100 = ceil(m / 100.);

        Rast_set_window(&new_cellhd);
    }

    struct Key_Value *in_proj_info, *in_unit_info;

    if ((in_proj_info = G_get_projinfo()) == NULL)
        G_fatal_error(_("Can't get projection info of current location"));

    if ((in_unit_info = G_get_projunits()) == NULL)
        G_fatal_error(_("Can't get projection units of current location"));

    if (pj_get_kv(&iproj, in_proj_info, in_unit_info) < 0)
        G_fatal_error(_("Can't get projection key values of current location"));

    G_free_key_value(in_proj_info);
    G_free_key_value(in_unit_info);

    /* Set output projection to latlong w/ same ellipsoid */
    oproj.pj = NULL;
    tproj.def = NULL;
    if (GPJ_init_transform(&iproj, &oproj, &tproj) < 0)
        G_fatal_error(_("Unable to initialize coordinate transformation"));

    /**********end of parser - ******************************/

    INPUT();
    if (mode == SINGLE_POINT) {
        /* Calculate the horizon for one single point */
        calculate_point_mode(xcoord, ycoord, single_direction);
    }
    else {
        calculate_raster_mode(
            (int)(ebufferZone / stepx), (int)(wbufferZone / stepx),
            (int)(sbufferZone / stepy), (int)(nbufferZone / stepy), bufferZone,
            single_direction);
    }

    exit(EXIT_SUCCESS);
}

/**********************end of main.c*****************/

int INPUT(void)
{
    FCELL *cell1 = Rast_allocate_f_buf();

    z = (float **)G_malloc(sizeof(float *) * (m));
    z100 = (float **)G_malloc(sizeof(float *) * (m100));

    for (int l = 0; l < m; l++) {
        z[l] = (float *)G_malloc(sizeof(float) * (n));
    }
    for (int l = 0; l < m100; l++) {
        z100[l] = (float *)G_malloc(sizeof(float) * (n100));
    }
    /*read Z raster */

    int fd1 = Rast_open_old(elevin, "");

    for (int row = 0; row < m; row++) {
        Rast_get_f_row(fd1, cell1, row);

        for (int j = 0; j < n; j++) {
            int row_rev = m - row - 1;

            if (!Rast_is_f_null_value(cell1 + j))
                z[row_rev][j] = (float)cell1[j];
            else
                z[row_rev][j] = UNDEFZ;
        }
    }
    Rast_close(fd1);

    /* create low resolution array 100 */
    for (int i = 0; i < m100; i++) {
        int lmax = (i + 1) * 100;
        if (lmax > m)
            lmax = m;

        for (int j = 0; j < n100; j++) {
            zmax = SMALL;
            int kmax = (j + 1) * 100;
            if (kmax > n)
                kmax = n;
            for (int l = (i * 100); l < lmax; l++) {
                for (int k = (j * 100); k < kmax; k++) {
                    zmax = amax1(zmax, z[l][k]);
                }
            }
            z100[i][j] = zmax;
            G_debug(3, "%d %d %lf\n", i, j, z100[i][j]);
        }
    }

    /* find max Z */
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
            zmax = amax1(zmax, z[i][j]);
        }
    }

    return 1;
}

int OUTGR(int numrows, int numcols)
{
    FCELL *cell1 = NULL;
    int fd1 = 0;

    Rast_set_window(&cellhd);

    if (horizon_basename != NULL) {
        cell1 = Rast_allocate_f_buf();
        fd1 = Rast_open_fp_new(shad_filename);
    }

    if (numrows != Rast_window_rows())
        G_fatal_error(_("OOPS: rows changed from %d to %d"), numrows,
                      Rast_window_rows());

    if (numcols != Rast_window_cols())
        G_fatal_error(_("OOPS: cols changed from %d to %d"), numcols,
                      Rast_window_cols());

    for (int iarc = 0; iarc < numrows; iarc++) {
        int i = numrows - iarc - 1;

        if (horizon_basename != NULL) {
            for (int j = 0; j < numcols; j++) {
                if (horizon_raster[i][j] == UNDEFZ)
                    Rast_set_f_null_value(cell1 + j, 1);
                else
                    cell1[j] = (FCELL)horizon_raster[i][j];
            }
            Rast_put_f_row(fd1, cell1);
        }
    } /* End loop over rows. */

    Rast_close(fd1);

    return 1;
}

double amax1(double arg1, double arg2)
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

int min(int arg1, int arg2)
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

/**********************************************************/

void com_par(OriginAngle *origin_angle, double angle, double xp, double yp)
{
    double longitude = xp;
    double latitude = yp;
    if (G_projection() != PROJECTION_LL) {
        if (GPJ_transform(&iproj, &oproj, &tproj, PJ_FWD, &longitude, &latitude,
                          NULL) < 0)
            G_fatal_error(_("Error in %s"), "GPJ_transform()");
    }
    latitude *= deg2rad;
    longitude *= deg2rad;

    double delt_lat =
        -0.0001 * cos(angle); /* Arbitrary small distance in latitude */
    double delt_lon = 0.0001 * sin(angle) / cos(latitude);

    latitude = (latitude + delt_lat) * rad2deg;
    longitude = (longitude + delt_lon) * rad2deg;

    if (G_projection() != PROJECTION_LL) {
        if (GPJ_transform(&iproj, &oproj, &tproj, PJ_INV, &longitude, &latitude,
                          NULL) < 0)
            G_fatal_error(_("Error in %s"), "GPJ_transform()");
    }
    double delt_east = longitude - xp;
    double delt_nor = latitude - yp;

    double delt_dist = sqrt(delt_east * delt_east + delt_nor * delt_nor);

    origin_angle->sinangle = delt_nor / delt_dist;
    origin_angle->cosangle = delt_east / delt_dist;

    if (fabs(origin_angle->sinangle) < 0.0000001) {
        origin_angle->sinangle = 0.;
    }
    if (fabs(origin_angle->cosangle) < 0.0000001) {
        origin_angle->cosangle = 0.;
    }
    origin_angle->distsinangle = 32000;
    origin_angle->distcosangle = 32000;

    if (origin_angle->sinangle != 0.) {
        origin_angle->distsinangle = 100. / (distxy * origin_angle->sinangle);
    }
    if (origin_angle->cosangle != 0.) {
        origin_angle->distcosangle = 100. / (distxy * origin_angle->cosangle);
    }

    origin_angle->stepsinangle = stepxy * origin_angle->sinangle;
    origin_angle->stepcosangle = stepxy * origin_angle->cosangle;
}

void calculate_point_mode(double xcoord, double ycoord, double single_direction)
{
    /*
       xg0 = xx0 = (double)xcoord * stepx;
       yg0 = yy0 = (double)ycoord * stepy;
       xg0 = xx0 = xcoord -0.5*stepx -xmin;
       yg0 = yy0 = ycoord -0.5*stepy-ymin;
       xg0 = xx0 = xindex*stepx -0.5*stepx;
       yg0 = yy0 = yindex*stepy -0.5*stepy;
     */
    OriginPoint origin_point;
    int xindex = (int)((xcoord - xmin) / stepx);
    int yindex = (int)((ycoord - ymin) / stepy);
    origin_point.xg0 = xindex * stepx;
    origin_point.yg0 = yindex * stepy;
    origin_point.coslatsq = 0;
    if ((G_projection() == PROJECTION_LL)) {
        ll_correction = TRUE;
    }
    if (ll_correction) {
        double coslat = cos(deg2rad * (ymin + origin_point.yg0));
        origin_point.coslatsq = coslat * coslat;
    }

    origin_point.z_orig = z[yindex][xindex];
    G_debug(1, "yindex: %d, xindex %d, z_orig %.2f", yindex, xindex,
            origin_point.z_orig);

    int printCount = 360. / fabs(step);

    if (printCount < 1)
        printCount = 1;
    double dfr_rad = step * deg2rad;

    double xp = xmin + origin_point.xg0;
    double yp = ymin + origin_point.yg0;

    double angle = (single_direction * deg2rad) + pihalf;

    maxlength = fixedMaxLength;
    fprintf(fp, "azimuth,horizon_height\n");

    for (int i = 0; i < printCount; i++) {
        OriginAngle origin_angle;
        com_par(&origin_angle, angle, xp, yp);

        double shadow_angle = horizon_height(&origin_point, &origin_angle);

        if (degreeOutput) {
            shadow_angle *= rad2deg;
        }
        double printangle = angle * rad2deg - 90.;
        if (printangle < 0.)
            printangle += 360;
        else if (printangle >= 360.)
            printangle -= 360;

        if (compassOutput) {
            double tmpangle;

            tmpangle = 360. - printangle + 90.;
            if (tmpangle >= 360.)
                tmpangle = tmpangle - 360.;
            fprintf(fp, "%lf,%lf\n", tmpangle, shadow_angle);
        }
        else {
            fprintf(fp, "%lf,%lf\n", printangle, shadow_angle);
        }

        angle += dfr_rad;

        if (angle < 0.)
            angle += twopi;
        else if (angle > twopi)
            angle -= twopi;
    } /* end of for loop over angles */
    fclose(fp);
}

/*////////////////////////////////////////////////////////////////////// */

int new_point(const OriginPoint *origin_point, const OriginAngle *origin_angle,
              SearchPoint *search_point, HorizonProperties *horizon)
{
    int iold = search_point->ip;
    int jold = search_point->jp;

    while (TRUE) {
        search_point->yy0 += origin_angle->stepsinangle;
        search_point->xx0 += origin_angle->stepcosangle;

        /* offset 0.5 cell size to get the right cell i, j */
        double sx = search_point->xx0 * invstepx + offsetx;
        double sy = search_point->yy0 * invstepy + offsety;
        search_point->ip = (int)sx;
        search_point->jp = (int)sy;

        /* test outside of raster */
        if ((search_point->ip < 0) || (search_point->ip >= n) ||
            (search_point->jp < 0) || (search_point->jp >= m))
            return (3);

        if ((search_point->ip != iold) || (search_point->jp != jold)) {
            double dx = (double)search_point->ip * stepx;
            double dy = (double)search_point->jp * stepy;

            horizon->length =
                distance(origin_point->xg0, dx, origin_point->yg0, dy,
                         origin_point->coslatsq); /* dist from orig. grid point
                                              to the current grid point */
            int succes2 =
                test_low_res(origin_point, origin_angle, search_point, horizon);
            if (succes2 == 1) {
                search_point->zp = z[search_point->jp][search_point->ip];
                return (1);
            }
        }
    }
    return -1;
}

int test_low_res(const OriginPoint *origin_point,
                 const OriginAngle *origin_angle, SearchPoint *search_point,
                 const HorizonProperties *horizon)
{
    int iold100 = search_point->ip100;
    int jold100 = search_point->jp100;
    search_point->ip100 = floor(search_point->ip / 100.);
    search_point->jp100 = floor(search_point->jp / 100.);
    /*test the new position with low resolution */
    if ((search_point->ip100 != iold100) || (search_point->jp100 != jold100)) {
        G_debug(2, "ip:%d jp:%d iold100:%d jold100:%d\n", search_point->ip,
                search_point->jp, iold100, jold100);
        /*  replace with approximate version
           curvature_diff = EARTHRADIUS*(1.-cos(length/EARTHRADIUS));
         */
        double curvature_diff =
            0.5 * horizon->length * horizon->length * invEarth;
        double z2 = origin_point->z_orig + curvature_diff +
                    horizon->length * horizon->tanh0;
        double zp100 = z100[search_point->jp100][search_point->ip100];
        G_debug(2, "ip:%d jp:%d z2:%lf zp100:%lf \n", search_point->ip,
                search_point->jp, z2, zp100);

        if (zp100 <= z2)
        /*skip to the next lowres cell */
        {
            int delx = 32000;
            int dely = 32000;
            if (origin_angle->cosangle > 0.) {
                double sx = search_point->xx0 * invstepx + offsetx;
                delx = floor(fabs((ceil(sx / 100.) - (sx / 100.)) *
                                  origin_angle->distcosangle));
            }
            if (origin_angle->cosangle < 0.) {
                double sx = search_point->xx0 * invstepx + offsetx;
                delx = floor(fabs((floor(sx / 100.) - (sx / 100.)) *
                                  origin_angle->distcosangle));
            }
            if (origin_angle->sinangle > 0.) {
                double sy = search_point->yy0 * invstepy + offsety;
                dely = floor(fabs((ceil(sy / 100.) - (sy / 100.)) *
                                  origin_angle->distsinangle));
            }
            else if (origin_angle->sinangle < 0.) {
                double sy = search_point->yy0 * invstepy + offsety;
                dely =
                    floor(fabs((floor(search_point->jp / 100.) - (sy / 100.)) *
                               origin_angle->distsinangle));
            }

            int mindel = min(delx, dely);
            G_debug(2, "%d %d %d %lf %lf\n", search_point->ip, search_point->jp,
                    mindel, origin_point->xg0, origin_point->yg0);

            search_point->yy0 =
                search_point->yy0 + (mindel * origin_angle->stepsinangle);
            search_point->xx0 =
                search_point->xx0 + (mindel * origin_angle->stepcosangle);
            G_debug(2, "  %lf %lf\n", search_point->xx0, search_point->yy0);

            return (3);
        }
        else {
            return (1); /* change of low res array - new cell is reaching limit
                           for high resolution processing */
        }
    }
    else {
        return (1); /* no change of low res array */
    }
}

double horizon_height(const OriginPoint *origin_point,
                      const OriginAngle *origin_angle)
{
    SearchPoint search_point;
    HorizonProperties horizon;

    search_point.ip = 0;
    search_point.jp = 0;
    search_point.xx0 = origin_point->xg0;
    search_point.yy0 = origin_point->yg0;
    search_point.zp = origin_point->z_orig;
    search_point.ip100 = floor(origin_point->xg0 * invstepx / 100.);
    search_point.jp100 = floor(origin_point->yg0 * invstepy / 100.);

    horizon.length = 0;
    horizon.tanh0 = 0;

    if (search_point.zp == UNDEFZ)
        return 0;

    while (1) {
        int succes =
            new_point(origin_point, origin_angle, &search_point, &horizon);

        if (succes != 1) {
            break;
        }

        /* curvature_diff = EARTHRADIUS*(1.-cos(length/EARTHRADIUS)); */
        double curvature_diff =
            0.5 * horizon.length * horizon.length * invEarth;

        double z2 = origin_point->z_orig + curvature_diff +
                    horizon.length * horizon.tanh0;

        if (z2 < search_point.zp) {
            horizon.tanh0 =
                (search_point.zp - origin_point->z_orig - curvature_diff) /
                horizon.length;
        }

        if (z2 >= zmax) {
            break;
        }

        if (horizon.length >= maxlength) {
            break;
        }
    }

    return atan(horizon.tanh0);
}

/*////////////////////////////////////////////////////////////////////// */

void calculate_raster_mode(int buffer_e, int buffer_w, int buffer_s,
                           int buffer_n, double bufferZone,
                           double single_direction)
{
    int hor_row_start = buffer_s;
    int hor_row_end = m - buffer_n;

    int hor_col_start = buffer_w;
    int hor_col_end = n - buffer_e;

    int hor_numrows = m - (buffer_s + buffer_n);
    int hor_numcols = n - (buffer_e + buffer_w);

    if ((G_projection() == PROJECTION_LL)) {
        ll_correction = TRUE;
    }

    /****************************************************************/
    /*  The loop over raster points starts here!                    */

    /****************************************************************/

    if (horizon_basename != NULL) {
        horizon_raster = (float **)G_malloc(sizeof(float *) * (hor_numrows));
        for (int l = 0; l < hor_numrows; l++) {
            horizon_raster[l] =
                (float *)G_malloc(sizeof(float) * (hor_numcols));
        }

        for (int j = 0; j < hor_numrows; j++) {
            for (int i = 0; i < hor_numcols; i++)
                horizon_raster[j][i] = 0.;
        }
    }
    double dfr_rad;
    int arrayNumInt;
    /* definition of horizon angle in loop */
    if (step == 0.0) {
        dfr_rad = 0;
        arrayNumInt = 1;
        sprintf(shad_filename, "%s", horizon_basename);
    }
    else {
        dfr_rad = step * deg2rad;
        arrayNumInt = (int)((end - start) / fabs(step));
    }

    size_t decimals = G_get_num_decimals(str_step);

    for (int k = 0; k < arrayNumInt; k++) {
        struct History history;

        double angle = (start + single_direction) * deg2rad + (dfr_rad * k);
        double angle_deg = angle * rad2deg + 0.0001;

        if (step != 0.0)
            shad_filename =
                G_generate_basename(horizon_basename, angle_deg, 3, decimals);
        G_message(
            _("Calculating map %01d of %01d (angle %.2f, raster map <%s>)"),
            (k + 1), arrayNumInt, angle_deg, shad_filename);

        for (int j = hor_row_start; j < hor_row_end; j++) {
            G_percent(j - hor_row_start, hor_numrows - 1, 2);
            for (int i = hor_col_start; i < hor_col_end; i++) {
                OriginPoint origin_point;
                OriginAngle origin_angle;
                origin_point.xg0 = (double)i * stepx;

                double xp = xmin + origin_point.xg0;
                origin_point.yg0 = (double)j * stepy;

                double yp = ymin + origin_point.yg0;
                origin_point.coslatsq = 0;
                if (ll_correction) {
                    double coslat = cos(deg2rad * yp);
                    origin_point.coslatsq = coslat * coslat;
                }

                double inputAngle = angle + pihalf;
                inputAngle =
                    (inputAngle >= twopi) ? inputAngle - twopi : inputAngle;
                com_par(&origin_angle, inputAngle, xp, yp);

                origin_point.z_orig = z[j][i];
                maxlength = (zmax - origin_point.z_orig) / TANMINANGLE;
                maxlength =
                    (maxlength < fixedMaxLength) ? maxlength : fixedMaxLength;

                if (origin_point.z_orig != UNDEFZ) {

                    G_debug(4, "**************new line %d %d\n", i, j);
                    double shadow_angle =
                        horizon_height(&origin_point, &origin_angle);

                    if (degreeOutput) {
                        shadow_angle *= rad2deg;
                    }

                    horizon_raster[j - buffer_s][i - buffer_w] = shadow_angle;

                } /* undefs */
            }
        }

        G_debug(1, "OUTGR() starts...");
        OUTGR(cellhd.rows, cellhd.cols);

        /* empty array */
        for (int j = 0; j < hor_numrows; j++) {
            for (int i = 0; i < hor_numcols; i++)
                horizon_raster[j][i] = 0.;
        }

        /* return back the buffered region */
        if (bufferZone > 0.)
            Rast_set_window(&new_cellhd);

        /* write metadata */
        Rast_short_history(shad_filename, "raster", &history);

        char msg_buff[256];
        sprintf(msg_buff, "Angular height of terrain horizon, map %01d of %01d",
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
        G_free(shad_filename);
    }
}

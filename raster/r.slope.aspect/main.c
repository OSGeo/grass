/****************************************************************************
 *
 * MODULE:       r.slope.aspect
 * AUTHOR(S):    Michael Shapiro and
 *               Olga Waupotitsch (original CERL contributors),
 *               Markus Neteler <neteler itc.it>,
 *               Bernhard Reiter <bernhard intevation.de>,
 *               Brad Douglas <rez touchofmadness.com>,
 *               Glynn Clements <glynn gclements.plus.com>,
 *               Hamish Bowman <hamish_b yahoo.com>,
 *               Jachym Cepicky <jachym les-ejk.cz>,
 *               Jan-Oliver Wagner <jan intevation.de>,
 *               Radim Blazek <radim.blazek gmail.com>
 *               Aaron Saw Min Sern (OpenMP parallelization)
 * PURPOSE:      generates raster maps of slope, aspect, curvatures and
 *               first and second order partial derivatives from a raster map
 *               of true elevation values
 * COPYRIGHT:    (C) 1999-2022 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#if defined(_OPENMP)
#include <omp.h>
#endif

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/raster.h>

/* 10/99 from GMSL, updated to new GRASS 5 code style , changed default "prec"
 * to float */

#define abs(x)       ((x) < 0 ? -(x) : (x))
#define FIRST_THREAD 0

/**************************************************************************
 * input is from command line.
 * arguments are elevation file, slope file, aspect file, profile curvature
 * file and tangential curvature file
 * elevation filename required
 * either slope filename or aspect filename or profile curvature filename
 * or tangential curvature filename required
 * usage: r.slope.aspect [-av] elevation=input slope=output1 aspect=output2
 *        pcurv=output3 tcurv=output4 format=name prec=name zfactor=value
 *        min_slope=value dx=output5 dy=output6 dxx=output7
 *      dyy=output8 dxy=output9
 * -a don't align window
 * -q quiet
 **************************************************************************/

/*  some changes made to code to retrieve correct distances when using
   lat/lon projection.  changes involve recalculating H and V. see
   comments within code.                                           */
/*  added colortables for topographic parameters and fixed
 *  the sign bug for the second order derivatives (jh) */

/* convert aspect from CCW from East to CW from North
 * aspect for flat areas is set to -9999 */
static double aspect_cw_n(double aspect)
{
    /* aspect == 0: flat */
    if (aspect == 0)
        return -9999;

    /* no modulus because of fp values */
    aspect = (450 - aspect);
    if (aspect >= 360)
        aspect -= 360;

    return aspect;
}

int main(int argc, char *argv[])
{
    struct Categories cats;
    int *elevation_fd;
    int aspect_fd;
    int slope_fd;
    int pcurv_fd;
    int tcurv_fd;
    int dx_fd;
    int dy_fd;
    int dxx_fd;
    int dyy_fd;
    int dxy_fd;
    DCELL *(*elev_cell)[3];
    DCELL tmp1, tmp2;
    FCELL dat1, dat2;
    CELL cat;
    void *asp_raster, *asp_ptr = NULL;
    void *slp_raster, *slp_ptr = NULL;
    void *pcurv_raster, *pcurv_ptr = NULL;
    void *tcurv_raster, *tcurv_ptr = NULL;
    void *dx_raster, *dx_ptr = NULL;
    void *dy_raster, *dy_ptr = NULL;
    void *dxx_raster, *dxx_ptr = NULL;
    void *dyy_raster, *dyy_ptr = NULL;
    void *dxy_raster, *dxy_ptr = NULL;
    int i, t;
    size_t size;
    RASTER_MAP_TYPE out_type, data_type;
    int Wrap; /* global wraparound */
    struct Cell_head window, cellhd;
    struct History hist;
    struct Colors colors;

    const char *elev_name;
    const char *aspect_name;
    const char *slope_name;
    const char *pcurv_name;
    const char *tcurv_name;
    const char *dx_name;
    const char *dy_name;
    const char *dxx_name;
    const char *dyy_name;
    const char *dxy_name;
    char buf[300];
    int nrows, row;
    int ncols, col;
    int nprocs;
    int bufrows;

    double north, east, south, west, ns_med;

    double radians_to_degrees;
    double degrees_to_radians;
    double H, V;

    double scik1 = 100000.;
    double zfactor;
    double factor;
    double min_asp = 360., max_asp = 0.;
    double gradmin = 0.001;
    double c1min = 0., c1max = 0., c2min = 0., c2max = 0.;

    double answer[92];
    double degrees;
    double tan_ans;
    double min_slp = 900., max_slp = 0., min_slope;
    int deg = 0;
    int perc = 0;
    char *slope_fmt;
    struct GModule *module;
    struct {
        struct Option *elevation, *slope_fmt, *slope, *aspect, *pcurv, *tcurv,
            *zfactor, *min_slope, *out_precision, *dx, *dy, *dxx, *dyy, *dxy,
            *nprocs, *memory;
    } parm;
    struct {
        struct Flag *a, *n, *e;
    } flag;
    int compute_at_edges;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("terrain"));
    G_add_keyword(_("aspect"));
    G_add_keyword(_("slope"));
    G_add_keyword(_("curvature"));
    G_add_keyword(_("parallel"));
    module->label = _("Generates raster maps of slope, aspect, curvatures and "
                      "partial derivatives from an elevation raster map.");
    module->description = _("Aspect is calculated counterclockwise from east.");

    parm.elevation = G_define_standard_option(G_OPT_R_ELEV);

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

    parm.slope_fmt = G_define_option();
    parm.slope_fmt->key = "format";
    parm.slope_fmt->type = TYPE_STRING;
    parm.slope_fmt->required = NO;
    parm.slope_fmt->answer = "degrees";
    parm.slope_fmt->options = "degrees,percent";
    parm.slope_fmt->description = _("Format for reporting the slope");
    parm.slope_fmt->guisection = _("Settings");

    parm.out_precision = G_define_standard_option(G_OPT_R_TYPE);
    parm.out_precision->key = "precision";
    parm.out_precision->required = NO;
    parm.out_precision->label = _("Type of output aspect and slope maps");
    parm.out_precision->answer = "FCELL";
    parm.out_precision->guisection = _("Settings");

    parm.pcurv = G_define_standard_option(G_OPT_R_OUTPUT);
    parm.pcurv->key = "pcurvature";
    parm.pcurv->required = NO;
    parm.pcurv->description = _("Name for output profile curvature raster map");
    parm.pcurv->guisection = _("Outputs");

    parm.tcurv = G_define_standard_option(G_OPT_R_OUTPUT);
    parm.tcurv->key = "tcurvature";
    parm.tcurv->required = NO;
    parm.tcurv->description =
        _("Name for output tangential curvature raster map");
    parm.tcurv->guisection = _("Outputs");

    parm.dx = G_define_standard_option(G_OPT_R_OUTPUT);
    parm.dx->key = "dx";
    parm.dx->required = NO;
    parm.dx->description = _("Name for output first order partial derivative "
                             "dx (E-W slope) raster map");
    parm.dx->guisection = _("Outputs");

    parm.dy = G_define_standard_option(G_OPT_R_OUTPUT);
    parm.dy->key = "dy";
    parm.dy->required = NO;
    parm.dy->description = _("Name for output first order partial derivative "
                             "dy (N-S slope) raster map");
    parm.dy->guisection = _("Outputs");

    parm.dxx = G_define_standard_option(G_OPT_R_OUTPUT);
    parm.dxx->key = "dxx";
    parm.dxx->required = NO;
    parm.dxx->description =
        _("Name for output second order partial derivative dxx raster map");
    parm.dxx->guisection = _("Outputs");

    parm.dyy = G_define_standard_option(G_OPT_R_OUTPUT);
    parm.dyy->key = "dyy";
    parm.dyy->required = NO;
    parm.dyy->description =
        _("Name for output second order partial derivative dyy raster map");
    parm.dyy->guisection = _("Outputs");

    parm.dxy = G_define_standard_option(G_OPT_R_OUTPUT);
    parm.dxy->key = "dxy";
    parm.dxy->required = NO;
    parm.dxy->description =
        _("Name for output second order partial derivative dxy raster map");
    parm.dxy->guisection = _("Outputs");

    parm.zfactor = G_define_option();
    parm.zfactor->key = "zscale";
    parm.zfactor->description = _(
        "Multiplicative factor to convert elevation units to horizontal units");
    parm.zfactor->type = TYPE_DOUBLE;
    parm.zfactor->required = NO;
    parm.zfactor->answer = "1.0";
    parm.zfactor->guisection = _("Settings");

    parm.min_slope = G_define_option();
    parm.min_slope->key = "min_slope";
    parm.min_slope->description =
        _("Minimum slope value (in percent) for which aspect is computed");
    parm.min_slope->type = TYPE_DOUBLE;
    parm.min_slope->required = NO;
    parm.min_slope->answer = "0.0";
    parm.min_slope->guisection = _("Settings");

    parm.nprocs = G_define_standard_option(G_OPT_M_NPROCS);
    parm.memory = G_define_standard_option(G_OPT_MEMORYMB);

    flag.a = G_define_flag();
    flag.a->key = 'a';
    flag.a->description =
        _("Do not align the current region to the raster elevation map");
    flag.a->guisection = _("Settings");

    flag.e = G_define_flag();
    flag.e->key = 'e';
    flag.e->description = _("Compute output at edges and near NULL values");
    flag.e->guisection = _("Settings");

    flag.n = G_define_flag();
    flag.n->key = 'n';
    flag.n->label = _("Create aspect as degrees clockwise from North "
                      "(azimuth), with flat = -9999");
    flag.n->description =
        _("Default: degrees counter-clockwise from East, with flat = 0");
    flag.n->guisection = _("Settings");

    G_option_required(parm.slope, parm.aspect, parm.pcurv, parm.tcurv, parm.dx,
                      parm.dy, parm.dxx, parm.dyy, parm.dxy, NULL);

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    sscanf(parm.nprocs->answer, "%d", &nprocs);
    if (nprocs < 1) {
        G_fatal_error(_("<%d> is not valid number of nprocs."), nprocs);
    }
#if defined(_OPENMP)
    omp_set_num_threads(nprocs);
#else
    if (nprocs != 1)
        G_warning(_("GRASS is compiled without OpenMP support. Ignoring "
                    "threads setting."));
    nprocs = 1;
#endif
    if (nprocs > 1 && Rast_mask_is_present()) {
        G_warning(_("Parallel processing disabled due to active mask."));
        nprocs = 1;
    }
    radians_to_degrees = 180.0 / M_PI;
    degrees_to_radians = M_PI / 180.0;

    compute_at_edges = flag.e->answer;

    /* INC BY ONE
       answer[0] = 0.0;
       answer[91] = 15000.0;

       for (i = 1; i < 91; i++)
       {
       degrees = i - .5;
       tan_ans = tan ( degrees  / radians_to_degrees );
       answer[i] = tan_ans * tan_ans;
       }
     */
    answer[0] = 0.0;
    answer[90] = 15000.0;

    for (i = 0; i < 90; i++) {
        degrees = i + .5;
        tan_ans = tan(degrees / radians_to_degrees);
        answer[i] = tan_ans * tan_ans;
    }

    elev_name = parm.elevation->answer;
    slope_name = parm.slope->answer;
    aspect_name = parm.aspect->answer;
    pcurv_name = parm.pcurv->answer;
    tcurv_name = parm.tcurv->answer;
    dx_name = parm.dx->answer;
    dy_name = parm.dy->answer;
    dxx_name = parm.dxx->answer;
    dyy_name = parm.dyy->answer;
    dxy_name = parm.dxy->answer;

    G_check_input_output_name(elev_name, slope_name, G_FATAL_EXIT);
    G_check_input_output_name(elev_name, aspect_name, G_FATAL_EXIT);
    G_check_input_output_name(elev_name, pcurv_name, G_FATAL_EXIT);
    G_check_input_output_name(elev_name, tcurv_name, G_FATAL_EXIT);
    G_check_input_output_name(elev_name, dx_name, G_FATAL_EXIT);
    G_check_input_output_name(elev_name, dy_name, G_FATAL_EXIT);
    G_check_input_output_name(elev_name, dxx_name, G_FATAL_EXIT);
    G_check_input_output_name(elev_name, dyy_name, G_FATAL_EXIT);
    G_check_input_output_name(elev_name, dxy_name, G_FATAL_EXIT);

    if (sscanf(parm.zfactor->answer, "%lf", &zfactor) != 1 || zfactor <= 0.0) {
        G_fatal_error(_("%s=%s - must be a positive number"), parm.zfactor->key,
                      parm.zfactor->answer);
    }

    if (sscanf(parm.min_slope->answer, "%lf", &min_slope) != 1 ||
        min_slope < 0.0) {
        G_fatal_error(_("%s=%s - must be a non-negative number"),
                      parm.min_slope->key, parm.min_slope->answer);
    }

    slope_fmt = parm.slope_fmt->answer;
    if (strcmp(slope_fmt, "percent") == 0)
        perc = 1;
    else if (strcmp(slope_fmt, "degrees") == 0)
        deg = 1;

    G_get_window(&window);

    /* set the window from the header for the elevation file */
    if (!flag.a->answer) {
        Rast_get_cellhd(elev_name, "", &cellhd);
        Rast_align_window(&window, &cellhd);
        Rast_set_window(&window);
        /* probably not needed, just to make sure
         * G_get_window() and Rast_get_window()
         * return the same */
        G_set_window(&window);
    }

    if (strcmp(parm.out_precision->answer, "DCELL") == 0)
        out_type = DCELL_TYPE;
    else if (strcmp(parm.out_precision->answer, "FCELL") == 0)
        out_type = FCELL_TYPE;
    else if (strcmp(parm.out_precision->answer, "CELL") == 0)
        out_type = CELL_TYPE;
    else
        G_fatal_error(_("Wrong raster type: %s"), parm.out_precision->answer);

    data_type = out_type;
    /* data type is the type of data being processed,
       out_type is type of map being created */
    /* ? why not use Rast_map_type() then ? */

    nrows = Rast_window_rows();
    ncols = Rast_window_cols();

    bufrows = atoi(parm.memory->answer) *
              (((1 << 20) / Rast_cell_size(data_type)) / ncols);
    /* set the output buffer rows to be at most covering the entire map */
    if (bufrows > nrows) {
        bufrows = nrows;
    }
    /* but at least the number of threads */
    if (bufrows < nprocs) {
        bufrows = nprocs;
    }

    if (((window.west == (window.east - 360.)) ||
         (window.east == (window.west - 360.))) &&
        (G_projection() == PROJECTION_LL)) {
        Wrap = 1;
        ncols += 2;
    }
    else
        Wrap = 0;

    /* H = window.ew_res * 4 * 2/ zfactor; */ /* horizontal (east-west) run
        times 4 for weighted difference */
    /* V = window.ns_res * 4 * 2/ zfactor; */ /* vertical (north-south) run
        times 4 for weighted difference */

    /* we don't assume vertical units to be meters any more */
    factor = G_database_units_to_meters_factor();
    if (factor != 1.0 && zfactor != 1.0)
        G_warning(_("r.slope.aspect does not convert horizontal units "
                    "to meters in this version, see manual page."));

    G_begin_distance_calculations();
    north = Rast_row_to_northing(0.5, &window);
    ns_med = Rast_row_to_northing(1.5, &window);
    south = Rast_row_to_northing(2.5, &window);
    east = Rast_col_to_easting(2.5, &window);
    west = Rast_col_to_easting(0.5, &window);
    V = G_distance(east, north, east, south) * 4 / (factor * zfactor);
    H = G_distance(east, ns_med, west, ns_med) * 4 / (factor * zfactor);
    /* ____________________________
       |c1      |c2      |c3      |
       |        |        |        |
       |        |  north |        |
       |        |        |        |
       |________|________|________|
       |c4      |c5      |c6      |
       |        |        |        |
       |  west  | ns_med |  east  |
       |        |        |        |
       |________|________|________|
       |c7      |c8      |c9      |
       |        |        |        |
       |        |  south |        |
       |        |        |        |
       |________|________|________|
     */

    /* open the elevation file for reading */
    elevation_fd = G_malloc(nprocs * sizeof *elevation_fd);
    elev_cell = G_malloc(nprocs * sizeof *elev_cell);
    for (t = 0; t < nprocs; t++) {
        elevation_fd[t] = Rast_open_old(elev_name, "");
        elev_cell[t][0] = (DCELL *)G_calloc(ncols + 2, sizeof(DCELL));
        elev_cell[t][1] = (DCELL *)G_calloc(ncols + 2, sizeof(DCELL));
        elev_cell[t][2] = (DCELL *)G_calloc(ncols + 2, sizeof(DCELL));
    }

    if (slope_name != NULL) {
        slope_fd = Rast_open_new(slope_name, out_type);
        slp_raster =
            G_calloc((size_t)bufrows * ncols, Rast_cell_size(data_type));
    }
    else {
        slp_raster = NULL;
        slope_fd = -1;
    }

    if (aspect_name != NULL) {
        aspect_fd = Rast_open_new(aspect_name, out_type);
        asp_raster =
            G_calloc((size_t)bufrows * ncols, Rast_cell_size(data_type));
    }
    else {
        asp_raster = NULL;
        aspect_fd = -1;
    }

    if (pcurv_name != NULL) {
        pcurv_fd = Rast_open_new(pcurv_name, out_type);
        pcurv_raster =
            G_calloc((size_t)bufrows * ncols, Rast_cell_size(data_type));
    }
    else {
        pcurv_raster = NULL;
        pcurv_fd = -1;
    }

    if (tcurv_name != NULL) {
        tcurv_fd = Rast_open_new(tcurv_name, out_type);
        tcurv_raster =
            G_calloc((size_t)bufrows * ncols, Rast_cell_size(data_type));
    }
    else {
        tcurv_raster = NULL;
        tcurv_fd = -1;
    }

    if (dx_name != NULL) {
        dx_fd = Rast_open_new(dx_name, out_type);
        dx_raster =
            G_calloc((size_t)bufrows * ncols, Rast_cell_size(data_type));
    }
    else {
        dx_raster = NULL;
        dx_fd = -1;
    }

    if (dy_name != NULL) {
        dy_fd = Rast_open_new(dy_name, out_type);
        dy_raster =
            G_calloc((size_t)bufrows * ncols, Rast_cell_size(data_type));
    }
    else {
        dy_raster = NULL;
        dy_fd = -1;
    }

    if (dxx_name != NULL) {
        dxx_fd = Rast_open_new(dxx_name, out_type);
        dxx_raster =
            G_calloc((size_t)bufrows * ncols, Rast_cell_size(data_type));
    }
    else {
        dxx_raster = NULL;
        dxx_fd = -1;
    }

    if (dyy_name != NULL) {
        dyy_fd = Rast_open_new(dyy_name, out_type);
        dyy_raster =
            G_calloc((size_t)bufrows * ncols, Rast_cell_size(data_type));
    }
    else {
        dyy_raster = NULL;
        dyy_fd = -1;
    }

    if (dxy_name != NULL) {
        dxy_fd = Rast_open_new(dxy_name, out_type);
        dxy_raster =
            G_calloc((size_t)bufrows * ncols, Rast_cell_size(data_type));
    }
    else {
        dxy_raster = NULL;
        dxy_fd = -1;
    }

    if (aspect_fd < 0 && slope_fd < 0 && pcurv_fd < 0 && tcurv_fd < 0 &&
        dx_fd < 0 && dy_fd < 0 && dxx_fd < 0 && dyy_fd < 0 && dxy_fd < 0)
        exit(EXIT_FAILURE);

    G_verbose_message(_("Percent complete..."));

    int computed = 0; /* for computing progress */
    int written = 0;

    while (written < nrows) {

        int range = bufrows;

        if (range > nrows - written) {
            range = nrows - written;
        }
        int start = written;
        int end = written + range;

#pragma omp parallel if (nprocs > 1)                                        \
    firstprivate(north, east, south, west, ns_med, H, V) private(           \
            row, col, size, slp_ptr, asp_ptr, pcurv_ptr, tcurv_ptr, dx_ptr, \
                dxx_ptr, dxy_ptr, dy_ptr, dyy_ptr)
        {
            int t_id = FIRST_THREAD;

#if defined(_OPENMP)
            t_id = omp_get_thread_num();
#endif

            /* private variables that are only used for computation */
            DCELL *temp;
            DCELL *pc1, *pc2, *pc3;
            DCELL c1, c2, c3, c4, c5, c6, c7, c8, c9;
            double dx; /* partial derivative in ew direction */
            double dy; /* partial derivative in ns direction */
            double dxx, dxy, dyy;
            double s3, s4, s5, s6;
            double pcurv, tcurv;
            double aspect, dnorm1, dx2, dy2, grad2, grad, dxy2;
            double key;
            double slp_in_perc, slp_in_deg;
            int low, hi, test = 0;
            bool initialized = false;

            /* static scheduling is essential for buffer to be initialized
             * properly */
#pragma omp for schedule(static)                    \
    reduction(min : c1min, c2min, min_asp, min_slp) \
    reduction(max : c1max, c2max, max_asp, max_slp)
            for (row = start; row < end; row++) {
                if (!initialized) {
                    initialized = true;
                    Rast_set_d_null_value(elev_cell[t_id][0], ncols + 2);
                    Rast_set_d_null_value(elev_cell[t_id][1], ncols + 2);
                    Rast_set_d_null_value(elev_cell[t_id][2], ncols + 2);

                    if (row - 1 >= 0)
                        Rast_get_d_row_nomask(elevation_fd[t_id],
                                              elev_cell[t_id][1] + 1, row - 1);

                    if (row >= 0)
                        Rast_get_d_row_nomask(elevation_fd[t_id],
                                              elev_cell[t_id][2] + 1, row);

                    if (Wrap) {
                        /* wrap second row */
                        elev_cell[t_id][1][0] = elev_cell[t_id][1][ncols];
                        elev_cell[t_id][1][ncols + 1] = elev_cell[t_id][1][1];
                        /* wrap third row */
                        elev_cell[t_id][2][0] = elev_cell[t_id][2][ncols];
                        elev_cell[t_id][2][ncols + 1] = elev_cell[t_id][2][1];
                    }
                }
                /*  if projection is Lat/Lon, recalculate  V and H   */
                if (G_projection() == PROJECTION_LL) {
                    north = Rast_row_to_northing((row - 1 + 0.5), &window);
                    ns_med = Rast_row_to_northing((row + 0.5), &window);
                    south = Rast_row_to_northing((row + 1 + 0.5), &window);
                    east = Rast_col_to_easting(2.5, &window);
                    west = Rast_col_to_easting(0.5, &window);
                    V = G_distance(east, north, east, south) * 4 /
                        (factor * zfactor);
                    H = G_distance(east, ns_med, west, ns_med) * 4 /
                        (factor * zfactor);
                    /* ____________________________
                       |c1      |c2      |c3      |
                       |        |        |        |
                       |        |  north |        |
                       |        |        |        |
                       |________|________|________|
                       |c4      |c5      |c6      |
                       |        |        |        |
                       |  west  | ns_med |  east  |
                       |        |        |        |
                       |________|________|________|
                       |c7      |c8      |c9      |
                       |        |        |        |
                       |        |  south |        |
                       |        |        |        |
                       |________|________|________|
                     */
                }

                G_percent(computed, nrows, 2);
                temp = elev_cell[t_id][0];
                elev_cell[t_id][0] = elev_cell[t_id][1];
                elev_cell[t_id][1] = elev_cell[t_id][2];
                elev_cell[t_id][2] = temp;

                if (row < nrows - 1)
                    Rast_get_d_row_nomask(elevation_fd[t_id],
                                          elev_cell[t_id][2] + 1, row + 1);
                else
                    Rast_set_d_null_value(elev_cell[t_id][2], ncols + 2);

                if (Wrap) {
                    elev_cell[t_id][2][0] = elev_cell[t_id][2][ncols];
                    elev_cell[t_id][2][ncols + 1] = elev_cell[t_id][2][1];
                }

                pc1 = elev_cell[t_id][0];
                pc2 = elev_cell[t_id][1];
                pc3 = elev_cell[t_id][2];

                size = Rast_cell_size(data_type) * ncols * (row - start);

                if (aspect_fd >= 0) {
                    asp_ptr = G_incr_void_ptr(asp_raster, size);
                }

                if (slope_fd >= 0) {
                    slp_ptr = G_incr_void_ptr(slp_raster, size);
                }

                if (pcurv_fd >= 0) {
                    pcurv_ptr = G_incr_void_ptr(pcurv_raster, size);
                }

                if (tcurv_fd >= 0) {
                    tcurv_ptr = G_incr_void_ptr(tcurv_raster, size);
                }

                if (dx_fd >= 0) {
                    dx_ptr = G_incr_void_ptr(dx_raster, size);
                }

                if (dy_fd >= 0) {
                    dy_ptr = G_incr_void_ptr(dy_raster, size);
                }

                if (dxx_fd >= 0) {
                    dxx_ptr = G_incr_void_ptr(dxx_raster, size);
                }

                if (dyy_fd >= 0) {
                    dyy_ptr = G_incr_void_ptr(dyy_raster, size);
                }

                if (dxy_fd >= 0) {
                    dxy_ptr = G_incr_void_ptr(dxy_raster, size);
                }

                for (col = ncols; col-- > 0; pc1++, pc2++, pc3++) {
                    c1 = *pc1;
                    c2 = *(pc1 + 1);
                    c3 = *(pc1 + 2);
                    c4 = *pc2;
                    c5 = *(pc2 + 1);
                    c6 = *(pc2 + 2);
                    c7 = *pc3;
                    c8 = *(pc3 + 1);
                    c9 = *(pc3 + 2);
                    /*  DEBUG:
                       fprintf(stdout, "\n%.0f %.0f %.0f\n%.0f %.0f %.0f\n%.0f
                       %.0f %.0f\n", c1, c2, c3, c4, c5, c6, c7, c8, c9);
                     */

                    if (Rast_is_d_null_value(&c5) ||
                        (!compute_at_edges && (Rast_is_d_null_value(&c1) ||
                                               Rast_is_d_null_value(&c2) ||
                                               Rast_is_d_null_value(&c3) ||
                                               Rast_is_d_null_value(&c4) ||
                                               Rast_is_d_null_value(&c6) ||
                                               Rast_is_d_null_value(&c7) ||
                                               Rast_is_d_null_value(&c8) ||
                                               Rast_is_d_null_value(&c9)))) {
                        if (slope_fd > 0) {
                            Rast_set_null_value(slp_ptr, 1, data_type);
                            slp_ptr = G_incr_void_ptr(
                                slp_ptr, Rast_cell_size(data_type));
                        }
                        if (aspect_fd > 0) {
                            Rast_set_null_value(asp_ptr, 1, data_type);
                            asp_ptr = G_incr_void_ptr(
                                asp_ptr, Rast_cell_size(data_type));
                        }
                        if (pcurv_fd > 0) {
                            Rast_set_null_value(pcurv_ptr, 1, data_type);
                            pcurv_ptr = G_incr_void_ptr(
                                pcurv_ptr, Rast_cell_size(data_type));
                        }
                        if (tcurv_fd > 0) {
                            Rast_set_null_value(tcurv_ptr, 1, data_type);
                            tcurv_ptr = G_incr_void_ptr(
                                tcurv_ptr, Rast_cell_size(data_type));
                        }
                        if (dx_fd > 0) {
                            Rast_set_null_value(dx_ptr, 1, data_type);
                            dx_ptr = G_incr_void_ptr(dx_ptr,
                                                     Rast_cell_size(data_type));
                        }
                        if (dy_fd > 0) {
                            Rast_set_null_value(dy_ptr, 1, data_type);
                            dy_ptr = G_incr_void_ptr(dy_ptr,
                                                     Rast_cell_size(data_type));
                        }
                        if (dxx_fd > 0) {
                            Rast_set_null_value(dxx_ptr, 1, data_type);
                            dxx_ptr = G_incr_void_ptr(
                                dxx_ptr, Rast_cell_size(data_type));
                        }
                        if (dyy_fd > 0) {
                            Rast_set_null_value(dyy_ptr, 1, data_type);
                            dyy_ptr = G_incr_void_ptr(
                                dyy_ptr, Rast_cell_size(data_type));
                        }
                        if (dxy_fd > 0) {
                            Rast_set_null_value(dxy_ptr, 1, data_type);
                            dxy_ptr = G_incr_void_ptr(
                                dxy_ptr, Rast_cell_size(data_type));
                        }
                        continue;
                    } /* no data */

                    if (compute_at_edges) {
                        /* same method like ComputeVal in gdaldem_lib.cpp */
                        if (Rast_is_d_null_value(&c1))
                            c1 = c5;
                        if (Rast_is_d_null_value(&c2))
                            c2 = c5;
                        if (Rast_is_d_null_value(&c3))
                            c3 = c5;
                        if (Rast_is_d_null_value(&c4))
                            c4 = c5;
                        if (Rast_is_d_null_value(&c6))
                            c6 = c5;
                        if (Rast_is_d_null_value(&c7))
                            c7 = c5;
                        if (Rast_is_d_null_value(&c8))
                            c8 = c5;
                        if (Rast_is_d_null_value(&c9))
                            c9 = c5;
                    }

                    dx = ((c1 + c4 + c4 + c7) - (c3 + c6 + c6 + c9)) / H;
                    dy = ((c7 + c8 + c8 + c9) - (c1 + c2 + c2 + c3)) / V;

                    /* compute topographic parameters */
                    key = dx * dx + dy * dy;
                    slp_in_perc = 100 * sqrt(key);
                    slp_in_deg = atan(sqrt(key)) * radians_to_degrees;

                    /* now update min and max */
                    if (deg) {
                        if (min_slp > slp_in_deg)
                            min_slp = slp_in_deg;
                        if (max_slp < slp_in_deg)
                            max_slp = slp_in_deg;
                    }
                    else {
                        if (min_slp > slp_in_perc)
                            min_slp = slp_in_perc;
                        if (max_slp < slp_in_perc)
                            max_slp = slp_in_perc;
                    }
                    if (slp_in_perc < min_slope)
                        slp_in_perc = 0.;

                    if (deg && out_type == CELL_TYPE) {
                        /* INC BY ONE
                           low = 1;
                           hi = 91;
                         */
                        low = 0;
                        hi = 90;
                        test = 20;

                        while (hi >= low) {
                            if (key >= answer[test])
                                low = test + 1;
                            else if (key < answer[test - 1])
                                hi = test - 1;
                            else
                                break;
                            test = (low + hi) / 2;
                        }
                    }
                    else if (perc && out_type == CELL_TYPE)
                        /* INCR_BY_ONE */
                        /* test = slp_in_perc + 1.5; */ /* All the slope
                            categories are incremented by 1 */
                        test = slp_in_perc + .5;

                    if (slope_fd > 0) {
                        if (data_type == CELL_TYPE)
                            *((CELL *)slp_ptr) = (CELL)test;
                        else {
                            if (deg)
                                Rast_set_d_value(slp_ptr, (DCELL)slp_in_deg,
                                                 data_type);
                            else
                                Rast_set_d_value(slp_ptr, (DCELL)slp_in_perc,
                                                 data_type);
                        }
                        slp_ptr =
                            G_incr_void_ptr(slp_ptr, Rast_cell_size(data_type));
                    } /* computing slope */

                    if (aspect_fd > 0) {
                        double aspect_flat = 0.;

                        if (slp_in_perc == 0.)
                            aspect = 0.;
                        else if (dx == 0) {
                            if (dy > 0)
                                aspect = 90.;
                            else
                                aspect = 270.;
                        }
                        else {
                            aspect = (atan2(dy, dx) / degrees_to_radians);
                            if (aspect <= 0.)
                                aspect = 360. + aspect;
                        }

                        if (flag.n->answer) {
                            aspect_flat = -9999;
                            aspect = aspect_cw_n(aspect);
                        }

                        if (out_type == CELL_TYPE) {
                            if (aspect > 0 && aspect < 0.5)
                                aspect = 360;
                            *((CELL *)asp_ptr) = (CELL)floor(aspect + .5);
                        }
                        else
                            Rast_set_d_value(asp_ptr, (DCELL)aspect, data_type);

                        asp_ptr =
                            G_incr_void_ptr(asp_ptr, Rast_cell_size(data_type));

                        /* now update min and max */
                        if (aspect > aspect_flat && min_asp > aspect)
                            min_asp = aspect;
                        if (max_asp < aspect)
                            max_asp = aspect;
                    } /* computing aspect */

                    if (dx_fd > 0) {
                        if (out_type == CELL_TYPE)
                            *((CELL *)dx_ptr) = (CELL)(scik1 * dx);
                        else
                            Rast_set_d_value(dx_ptr, (DCELL)dx, data_type);
                        dx_ptr =
                            G_incr_void_ptr(dx_ptr, Rast_cell_size(data_type));
                    }

                    if (dy_fd > 0) {
                        if (out_type == CELL_TYPE)
                            *((CELL *)dy_ptr) = (CELL)(scik1 * dy);
                        else
                            Rast_set_d_value(dy_ptr, (DCELL)dy, data_type);
                        dy_ptr =
                            G_incr_void_ptr(dy_ptr, Rast_cell_size(data_type));
                    }

                    if (dxx_fd <= 0 && dxy_fd <= 0 && dyy_fd <= 0 &&
                        pcurv_fd <= 0 && tcurv_fd <= 0)
                        continue;

                    /* compute second order derivatives */
                    s4 = c1 + c3 + c7 + c9 - c5 * 8.;
                    s5 = c4 * 4. + c6 * 4. - c8 * 2. - c2 * 2.;
                    s6 = c8 * 4. + c2 * 4. - c4 * 2. - c6 * 2.;
                    s3 = c7 - c9 + c3 - c1;

                    dxx = -(s4 + s5) / ((3. / 32.) * H * H);
                    dyy = -(s4 + s6) / ((3. / 32.) * V * V);
                    dxy = -s3 / ((1. / 16.) * H * V);

                    if (dxx_fd > 0) {
                        if (out_type == CELL_TYPE)
                            *((CELL *)dxx_ptr) = (CELL)(scik1 * dxx);
                        else
                            Rast_set_d_value(dxx_ptr, (DCELL)dxx, data_type);
                        dxx_ptr =
                            G_incr_void_ptr(dxx_ptr, Rast_cell_size(data_type));
                    }

                    if (dyy_fd > 0) {
                        if (out_type == CELL_TYPE)
                            *((CELL *)dyy_ptr) = (CELL)(scik1 * dyy);
                        else
                            Rast_set_d_value(dyy_ptr, (DCELL)dyy, data_type);
                        dyy_ptr =
                            G_incr_void_ptr(dyy_ptr, Rast_cell_size(data_type));
                    }

                    if (dxy_fd > 0) {
                        if (out_type == CELL_TYPE)
                            *((CELL *)dxy_ptr) = (CELL)(scik1 * dxy);
                        else
                            Rast_set_d_value(dxy_ptr, (DCELL)dxy, data_type);
                        dxy_ptr =
                            G_incr_void_ptr(dxy_ptr, Rast_cell_size(data_type));
                    }

                    /* compute curvature */
                    if (pcurv_fd <= 0 && tcurv_fd <= 0)
                        continue;

                    grad2 = key; /*dx2 + dy2 */
                    grad = sqrt(grad2);
                    if (grad <= gradmin) {
                        pcurv = 0.;
                        tcurv = 0.;
                    }
                    else {
                        dnorm1 = sqrt(grad2 + 1.);
                        dxy2 = 2. * dxy * dx * dy;
                        dx2 = dx * dx;
                        dy2 = dy * dy;
                        pcurv = (dxx * dx2 + dxy2 + dyy * dy2) /
                                (grad2 * dnorm1 * dnorm1 * dnorm1);
                        tcurv =
                            (dxx * dy2 - dxy2 + dyy * dx2) / (grad2 * dnorm1);
                        if (c1min > pcurv)
                            c1min = pcurv;
                        if (c1max < pcurv)
                            c1max = pcurv;
                        if (c2min > tcurv)
                            c2min = tcurv;
                        if (c2max < tcurv)
                            c2max = tcurv;
                    }

                    if (pcurv_fd > 0) {
                        if (out_type == CELL_TYPE)
                            *((CELL *)pcurv_ptr) = (CELL)(scik1 * pcurv);
                        else
                            Rast_set_d_value(pcurv_ptr, (DCELL)pcurv,
                                             data_type);
                        pcurv_ptr = G_incr_void_ptr(pcurv_ptr,
                                                    Rast_cell_size(data_type));
                    }

                    if (tcurv_fd > 0) {
                        if (out_type == CELL_TYPE)
                            *((CELL *)tcurv_ptr) = (CELL)(scik1 * tcurv);
                        else
                            Rast_set_d_value(tcurv_ptr, (DCELL)tcurv,
                                             data_type);
                        tcurv_ptr = G_incr_void_ptr(tcurv_ptr,
                                                    Rast_cell_size(data_type));
                    }

                } /* end column loop */

#pragma omp atomic update
                computed++;
            } /* end row loop */
        } /* end parallel region */

        /* write the computed buffer chunk to disk */
        written = end;
        for (row = start; row < end; row++) {
            size = Rast_cell_size(data_type) * ncols * (row - start);

            if (aspect_fd > 0) {
                asp_ptr = G_incr_void_ptr(asp_raster, size);
                Rast_put_row(aspect_fd, asp_ptr, data_type);
            }

            if (slope_fd > 0) {
                slp_ptr = G_incr_void_ptr(slp_raster, size);
                Rast_put_row(slope_fd, slp_ptr, data_type);
            }

            if (pcurv_fd > 0) {
                pcurv_ptr = G_incr_void_ptr(pcurv_raster, size);
                Rast_put_row(pcurv_fd, pcurv_ptr, data_type);
            }

            if (tcurv_fd > 0) {
                tcurv_ptr = G_incr_void_ptr(tcurv_raster, size);
                Rast_put_row(tcurv_fd, tcurv_ptr, data_type);
            }

            if (dx_fd > 0) {
                dx_ptr = G_incr_void_ptr(dx_raster, size);
                Rast_put_row(dx_fd, dx_ptr, data_type);
            }

            if (dy_fd > 0) {
                dy_ptr = G_incr_void_ptr(dy_raster, size);
                Rast_put_row(dy_fd, dy_ptr, data_type);
            }

            if (dxx_fd > 0) {
                dxx_ptr = G_incr_void_ptr(dxx_raster, size);
                Rast_put_row(dxx_fd, dxx_ptr, data_type);
            }

            if (dyy_fd > 0) {
                dyy_ptr = G_incr_void_ptr(dyy_raster, size);
                Rast_put_row(dyy_fd, dyy_ptr, data_type);
            }

            if (dxy_fd > 0) {
                dxy_ptr = G_incr_void_ptr(dxy_raster, size);
                Rast_put_row(dxy_fd, dxy_ptr, data_type);
            }
        }

    } /* while loop repeats until all chunks are computed and written */

    G_percent(nrows, nrows, 2);

    for (t = 0; t < nprocs; t++)
        Rast_close(elevation_fd[t]);
    G_debug(1, "Creating support files...");

    G_verbose_message(_("Elevation products for mapset <%s> in <%s>"),
                      G_mapset(), G_location());

    if (aspect_fd >= 0) {
        DCELL min, max;
        struct FPRange range;

        Rast_close(aspect_fd);

        if (out_type != CELL_TYPE)
            Rast_quantize_fp_map_range(aspect_name, G_mapset(), 0., 360., 0,
                                       360);

        Rast_read_cats(aspect_name, G_mapset(), &cats);
        if (flag.n->answer)
            Rast_set_cats_title("Aspect clockwise in degrees from north",
                                &cats);
        else
            Rast_set_cats_title("Aspect counterclockwise in degrees from east",
                                &cats);
        G_verbose_message(
            _("Min computed aspect %.4f, max computed aspect %.4f"), min_asp,
            max_asp);
        /* the categries quant intervals are 1.0 long, plus
           we are using reverse order so that the label looked up
           for i-.5 is not the one defined for i-.5, i+.5 interval, but
           the one defile for i-1.5, i-.5 interval which is added later */
        if (!flag.n->answer) {
            for (i = ceil(max_asp); i >= 1; i--) {
                if (i == 360)
                    sprintf(buf, "east");
                else if (i == 45)
                    sprintf(buf, "north ccw of east");
                else if (i == 90)
                    sprintf(buf, "north");
                else if (i == 135)
                    sprintf(buf, "north ccw of west");
                else if (i == 180)
                    sprintf(buf, "west");
                else if (i == 225)
                    sprintf(buf, "south ccw of west");
                else if (i == 270)
                    sprintf(buf, "south");
                else if (i == 315)
                    sprintf(buf, "south ccw of east");
                else
                    sprintf(buf, "%d degree%s ccw from east", i,
                            i == 1 ? "" : "s");
                if (data_type == CELL_TYPE) {
                    Rast_set_c_cat((CELL *)&i, (CELL *)&i, buf, &cats);
                    continue;
                }
                tmp1 = (double)i - .5;
                tmp2 = (double)i + .5;
                Rast_set_d_cat(&tmp1, &tmp2, buf, &cats);
            }
            if (data_type == CELL_TYPE) {
                cat = 0;
                Rast_set_c_cat(&cat, &cat, "no aspect", &cats);
            }
            else {
                tmp1 = 0;
                Rast_set_d_cat(&tmp1, &tmp1, "no aspect", &cats);
            }
        }
        else {
            for (i = ceil(max_asp); i >= 1; i--) {
                if (i == 0 || i == 360)
                    sprintf(buf, "north");
                else if (i == 45)
                    sprintf(buf, "north-east");
                else if (i == 90)
                    sprintf(buf, "east");
                else if (i == 135)
                    sprintf(buf, "south-east");
                else if (i == 180)
                    sprintf(buf, "south");
                else if (i == 225)
                    sprintf(buf, "south-west");
                else if (i == 270)
                    sprintf(buf, "west");
                else if (i == 315)
                    sprintf(buf, "north-west");
                else
                    sprintf(buf, "%d degree%s cw from north", i,
                            i == 1 ? "" : "s");
                if (data_type == CELL_TYPE) {
                    Rast_set_c_cat((CELL *)&i, (CELL *)&i, buf, &cats);
                    continue;
                }
                tmp1 = (double)i - .5;
                tmp2 = (double)i + .5;
                Rast_set_d_cat(&tmp1, &tmp2, buf, &cats);
            }
            if (data_type == CELL_TYPE) {
                cat = -9999;
                Rast_set_c_cat(&cat, &cat, "no aspect", &cats);
            }
            else {
                tmp1 = -9999;
                Rast_set_d_cat(&tmp1, &tmp1, "no aspect", &cats);
            }
        }
        Rast_write_cats(aspect_name, &cats);
        Rast_free_cats(&cats);

        /* write colors for aspect file */
        Rast_init_colors(&colors);
        Rast_read_fp_range(aspect_name, G_mapset(), &range);
        Rast_get_fp_range_min_max(&range, &min, &max);
        if (flag.n->answer)
            Rast_make_aspect_fp_colors(&colors, 0, max);
        else
            Rast_make_aspect_fp_colors(&colors, min, max);
        Rast_write_colors(aspect_name, G_mapset(), &colors);

        /* writing history file */
        Rast_short_history(aspect_name, "raster", &hist);
        Rast_append_format_history(&hist, "aspect map elev = %s", elev_name);
        Rast_append_format_history(&hist, "zfactor = %.2f", zfactor);
        Rast_append_format_history(&hist, "min_slope = %f", min_slope);
        Rast_format_history(&hist, HIST_DATSRC_1, "raster elevation file %s",
                            elev_name);
        Rast_command_history(&hist);
        Rast_write_history(aspect_name, &hist);

        G_message(_("Aspect raster map <%s> complete"), aspect_name);
    }

    if (slope_fd >= 0) {
        /* colortable for slopes */
        CELL val1, val2;

        Rast_init_colors(&colors);
        val1 = 0;
        val2 = 2;
        Rast_add_c_color_rule(&val1, 255, 255, 255, &val2, 255, 255, 0,
                              &colors);
        val1 = 2;
        val2 = 5;
        Rast_add_c_color_rule(&val1, 255, 255, 0, &val2, 0, 255, 0, &colors);
        val1 = 5;
        val2 = 10;
        Rast_add_c_color_rule(&val1, 0, 255, 0, &val2, 0, 255, 255, &colors);
        val1 = 10;
        val2 = 15;
        Rast_add_c_color_rule(&val1, 0, 255, 255, &val2, 0, 0, 255, &colors);
        val1 = 15;
        val2 = 30;
        Rast_add_c_color_rule(&val1, 0, 0, 255, &val2, 255, 0, 255, &colors);
        val1 = 30;
        val2 = 50;
        Rast_add_c_color_rule(&val1, 255, 0, 255, &val2, 255, 0, 0, &colors);
        val1 = 50;
        val2 = 90;
        Rast_add_c_color_rule(&val1, 255, 0, 0, &val2, 0, 0, 0, &colors);

        Rast_close(slope_fd);

        if (out_type != CELL_TYPE) {
            /* INCR_BY_ONE
               if(deg)
               Rast_quantize_fp_map_range(slope_name, G_mapset(), 0., 90., 1,
               91); else Rast_quantize_fp_map_range(slope_name, G_mapset(),
               min_slp, max_slp, (CELL) min_slp + 1, (CELL) ceil(max_slp) + 1);
             */
            Rast_write_colors(slope_name, G_mapset(), &colors);
            if (deg)
                Rast_quantize_fp_map_range(slope_name, G_mapset(), 0., 90., 0,
                                           90);
            else /* percent */
                Rast_quantize_fp_map_range(slope_name, G_mapset(), min_slp,
                                           max_slp, (CELL)min_slp,
                                           (CELL)ceil(max_slp));
        }

        Rast_read_cats(slope_name, G_mapset(), &cats);
        if (deg)
            Rast_set_cats_title("slope in degrees", &cats);
        else if (perc)
            Rast_set_cats_title("percent slope", &cats);

        G_verbose_message(_("Min computed slope %.4f, max computed slope %.4f"),
                          min_slp, max_slp);
        /* the categries quant intervals are 1.0 long, plus
           we are using reverse order so that the label looked up
           for i-.5 is not the one defined for i-.5, i+.5 interval, but
           the one defined for i-1.5, i-.5 interval which is added later */
        for (i = ceil(max_slp); i > /* INC BY ONE >= */ 0; i--) {
            if (deg)
                sprintf(buf, "%d degree%s", i, i == 1 ? "" : "s");
            else if (perc)
                sprintf(buf, "%d percent", i);
            if (data_type == CELL_TYPE) {
                /* INCR_BY_ONE
                   Rast_set_c_cat(i+1, buf, &cats);
                 */
                Rast_set_c_cat(&i, &i, buf, &cats);
                continue;
            }
            /* INCR_BY_ONE
               tmp1 = (DCELL) i+.5;
               tmp2 = (DCELL) i+1.5;
             */
            tmp1 = (DCELL)i - .5;
            tmp2 = (DCELL)i + .5;
            Rast_set_d_cat(&tmp1, &tmp2, buf, &cats);
        }
        if (data_type == CELL_TYPE) {
            cat = 0;
            Rast_set_c_cat(&cat, &cat, "zero slope", &cats);
        }
        /* INCR_BY_ONE
           Rast_set_c_cat(0, "no data", &cats);
         */
        else {
            tmp1 = 0;
            tmp2 = 0.5;
            Rast_set_d_cat(&tmp1, &tmp2, "zero slope", &cats);
        }
        /* INCR_BY_ONE
           Rast_set_d_cat (&tmp1, &tmp1, "no data", &cats);
         */
        Rast_write_cats(slope_name, &cats);

        /* writing history file */
        Rast_short_history(slope_name, "raster", &hist);
        Rast_append_format_history(&hist, "slope map elev = %s", elev_name);
        Rast_append_format_history(&hist, "zfactor = %.2f format = %s", zfactor,
                                   parm.slope_fmt->answer);
        Rast_append_format_history(&hist, "min_slope = %f", min_slope);
        Rast_format_history(&hist, HIST_DATSRC_1, "raster elevation file %s",
                            elev_name);
        Rast_command_history(&hist);
        Rast_write_history(slope_name, &hist);

        G_message(_("Slope raster map <%s> complete"), slope_name);
    }

    /* colortable for curvatures */
    if (pcurv_fd >= 0 || tcurv_fd >= 0) {
        Rast_init_colors(&colors);
        if (c1min < c2min)
            dat1 = (FCELL)c1min;
        else
            dat1 = (FCELL)c2min;

        dat2 = (FCELL)-0.01;
        Rast_add_f_color_rule(&dat1, 127, 0, 255, &dat2, 0, 0, 255, &colors);
        dat1 = dat2;
        dat2 = (FCELL)-0.001;
        Rast_add_f_color_rule(&dat1, 0, 0, 255, &dat2, 0, 127, 255, &colors);
        dat1 = dat2;
        dat2 = (FCELL)-0.00001;
        Rast_add_f_color_rule(&dat1, 0, 127, 255, &dat2, 0, 255, 255, &colors);
        dat1 = dat2;
        dat2 = (FCELL)0.0;
        Rast_add_f_color_rule(&dat1, 0, 255, 255, &dat2, 200, 255, 200,
                              &colors);
        dat1 = dat2;
        dat2 = (FCELL)0.00001;
        Rast_add_f_color_rule(&dat1, 200, 255, 200, &dat2, 255, 255, 0,
                              &colors);
        dat1 = dat2;
        dat2 = (FCELL)0.001;
        Rast_add_f_color_rule(&dat1, 255, 255, 0, &dat2, 255, 127, 0, &colors);
        dat1 = dat2;
        dat2 = (FCELL)0.01;
        Rast_add_f_color_rule(&dat1, 255, 127, 0, &dat2, 255, 0, 0, &colors);
        dat1 = dat2;
        if (c1max > c2max)
            dat2 = (FCELL)c1max;
        else
            dat2 = (FCELL)c2max;

        Rast_add_f_color_rule(&dat1, 255, 0, 0, &dat2, 255, 0, 200, &colors);
    }

    if (pcurv_fd >= 0) {
        Rast_close(pcurv_fd);

        Rast_write_colors(pcurv_name, G_mapset(), &colors);

        if (out_type != CELL_TYPE)
            Rast_round_fp_map(pcurv_name, G_mapset());

        Rast_read_cats(pcurv_name, G_mapset(), &cats);
        Rast_set_cats_title("profile curvature", &cats);
        cat = 0;
        Rast_set_c_cat(&cat, &cat, "no profile curve", &cats);

        /* writing history file */
        Rast_short_history(pcurv_name, "raster", &hist);
        Rast_append_format_history(&hist, "profile curve map elev = %s",
                                   elev_name);
        Rast_append_format_history(&hist, "zfactor = %.2f", zfactor);
        Rast_append_format_history(&hist, "min_slope = %f", min_slope);
        Rast_format_history(&hist, HIST_DATSRC_1, "raster elevation file %s",
                            elev_name);
        Rast_command_history(&hist);
        Rast_write_history(pcurv_name, &hist);

        G_message(_("Profile curve raster map <%s> complete"), pcurv_name);
    }

    if (tcurv_fd >= 0) {
        Rast_close(tcurv_fd);

        Rast_write_colors(tcurv_name, G_mapset(), &colors);

        if (out_type != CELL_TYPE)
            Rast_round_fp_map(tcurv_name, G_mapset());

        Rast_read_cats(tcurv_name, G_mapset(), &cats);
        Rast_set_cats_title("tangential curvature", &cats);
        cat = 0;
        Rast_set_c_cat(&cat, &cat, "no tangential curve", &cats);

        /* writing history file */
        Rast_short_history(tcurv_name, "raster", &hist);
        Rast_append_format_history(&hist, "tangential curve map elev = %s",
                                   elev_name);
        Rast_append_format_history(&hist, "zfactor = %.2f", zfactor);
        Rast_append_format_history(&hist, "min_slope = %f", min_slope);
        Rast_format_history(&hist, HIST_DATSRC_1, "raster elevation file %s",
                            elev_name);
        Rast_command_history(&hist);
        Rast_write_history(tcurv_name, &hist);

        G_message(_("Tangential curve raster map <%s> complete"), tcurv_name);
    }

    if (dx_fd >= 0) {
        Rast_close(dx_fd);

        if (out_type != CELL_TYPE)
            Rast_round_fp_map(dx_name, G_mapset());

        Rast_read_cats(dx_name, G_mapset(), &cats);
        Rast_set_cats_title("E-W slope", &cats);
        cat = 0;
        Rast_set_c_cat(&cat, &cat, "no E-W slope", &cats);

        /* writing history file */
        Rast_short_history(dx_name, "raster", &hist);
        Rast_append_format_history(&hist, "E-W slope map elev = %s", elev_name);
        Rast_append_format_history(&hist, "zfactor = %.2f", zfactor);
        Rast_append_format_history(&hist, "min_slope = %f", min_slope);
        Rast_format_history(&hist, HIST_DATSRC_1, "raster elevation file %s",
                            elev_name);
        Rast_command_history(&hist);
        Rast_write_history(dx_name, &hist);

        G_message(_("E-W slope raster map <%s> complete"), dx_name);
    }

    if (dy_fd >= 0) {
        Rast_close(dy_fd);

        if (out_type != CELL_TYPE)
            Rast_round_fp_map(dy_name, G_mapset());

        Rast_read_cats(dy_name, G_mapset(), &cats);
        Rast_set_cats_title("N-S slope", &cats);
        cat = 0;
        Rast_set_c_cat(&cat, &cat, "no N-S slope", &cats);

        /* writing history file */
        Rast_short_history(dy_name, "raster", &hist);
        Rast_append_format_history(&hist, "N-S slope map elev = %s", elev_name);
        Rast_append_format_history(&hist, "zfactor = %.2f", zfactor);
        Rast_append_format_history(&hist, "min_slope = %f", min_slope);
        Rast_format_history(&hist, HIST_DATSRC_1, "raster elevation file %s",
                            elev_name);
        Rast_command_history(&hist);
        Rast_write_history(dy_name, &hist);

        G_message(_("N-S slope raster map <%s> complete"), dy_name);
    }

    if (dxx_fd >= 0) {
        Rast_close(dxx_fd);

        if (out_type != CELL_TYPE)
            Rast_round_fp_map(dxx_name, G_mapset());

        Rast_read_cats(dxx_name, G_mapset(), &cats);
        Rast_set_cats_title("DXX", &cats);
        cat = 0;
        Rast_set_c_cat(&cat, &cat, "DXX", &cats);

        /* writing history file */
        Rast_short_history(dxx_name, "raster", &hist);
        Rast_append_format_history(&hist, "DXX map elev = %s", elev_name);
        Rast_append_format_history(&hist, "zfactor = %.2f", zfactor);
        Rast_append_format_history(&hist, "min_slope = %f", min_slope);
        Rast_format_history(&hist, HIST_DATSRC_1, "raster elevation file %s",
                            elev_name);
        Rast_command_history(&hist);
        Rast_write_history(dxx_name, &hist);

        G_message(_("Dxx raster map <%s> complete"), dxx_name);
    }

    if (dyy_fd >= 0) {
        Rast_close(dyy_fd);

        if (out_type != CELL_TYPE)
            Rast_round_fp_map(dyy_name, G_mapset());

        Rast_read_cats(dyy_name, G_mapset(), &cats);
        Rast_set_cats_title("DYY", &cats);
        cat = 0;
        Rast_set_c_cat(&cat, &cat, "DYY", &cats);

        /* writing history file */
        Rast_short_history(dyy_name, "raster", &hist);
        Rast_append_format_history(&hist, "DYY map elev = %s", elev_name);
        Rast_append_format_history(&hist, "zfactor = %.2f", zfactor);
        Rast_append_format_history(&hist, "min_slope = %f", min_slope);
        Rast_format_history(&hist, HIST_DATSRC_1, "raster elevation file %s",
                            elev_name);
        Rast_command_history(&hist);
        Rast_write_history(dyy_name, &hist);

        G_message(_("Dyy raster map <%s> complete"), dyy_name);
    }

    if (dxy_fd >= 0) {
        Rast_close(dxy_fd);

        if (out_type != CELL_TYPE)
            Rast_round_fp_map(dxy_name, G_mapset());

        Rast_read_cats(dxy_name, G_mapset(), &cats);
        Rast_set_cats_title("DXY", &cats);
        cat = 0;
        Rast_set_c_cat(&cat, &cat, "DXY", &cats);

        /* writing history file */
        Rast_short_history(dxy_name, "raster", &hist);
        Rast_append_format_history(&hist, "DXY map elev = %s", elev_name);
        Rast_append_format_history(&hist, "zfactor = %.2f", zfactor);
        Rast_append_format_history(&hist, "min_slope = %f", min_slope);
        Rast_format_history(&hist, HIST_DATSRC_1, "raster elevation file %s",
                            elev_name);
        Rast_command_history(&hist);
        Rast_write_history(dxy_name, &hist);

        G_message(_("Dxy raster map <%s> complete"), dxy_name);
    }

    exit(EXIT_SUCCESS);
}

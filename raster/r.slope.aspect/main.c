/****************************************************************************
 *
 * MODULE:       r.slope.aspect
 * AUTHOR(S):    Michael Shapiro and 
 *               Olga Waupotitsch (original CERL contributors), 
 *               Markus Neteler <neteler itc.it>,
 *               Bernhard Reiter <bernhard intevation.de>, 
 *               Brad Douglas <rez touchofmadness.com>,
 *               Glynn Clements <glynn gclements.plus.com>,
 *               Hamish Bowman <hamish_nospam yahoo.com>,
 *               Jachym Cepicky <jachym les-ejk.cz>,
 *               Jan-Oliver Wagner <jan intevation.de>,
 *               Radim Blazek <radim.blazek gmail.com>
 * PURPOSE:      generates raster maps of slope, aspect, curvatures and
 *               first and second order partial derivatives from a raster map
 *               of true elevation values
 * COPYRIGHT:    (C) 1999-2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "local_proto.h"

/* 10/99 from GMSL, updated to new GRASS 5 code style , changed default "prec" to float*/


#define abs(x) ((x)<0?-(x):(x))


/**************************************************************************
 * input is from command line.
 * arguments are elevation file, slope file, aspect file, profile curvature
 * file and tangential curvature file
 * elevation filename required
 * either slope filename or aspect filename or profile curvature filename
 * or tangential curvature filename required
 * usage: r.slope.aspect [-av] elevation=input slope=output1 aspect=output2
 *	pcurv=output3 tcurv=output4 format=name prec=name zfactor=value
 *	min_slp_allowed=value dx=output5 dy=output6 dxx=output7 
 *      dyy=output8 dxy=output9
 * -a don't align window
 * -q quiet
 **************************************************************************/

/*  some changes made to code to retrieve correct distances when using
    lat/lon projection.  changes involve recalculating H and V. see
    comments within code.                                           */
/*  added colortables for topographic parameters and fixed 
 *  the sign bug for the second order derivatives (jh) */


int main (int argc, char *argv[])
{
    struct Categories cats;
    int elevation_fd;
    int aspect_fd ;
    int slope_fd ;
    int pcurv_fd ;
    int tcurv_fd ;
    int dx_fd ;
    int dy_fd ;
    int dxx_fd ;
    int dyy_fd ;
    int dxy_fd ;
    DCELL *elev_cell[3], *temp;
    DCELL *c1, *c2, *c3, *c4, *c5, *c6, *c7, *c8, *c9;
    DCELL tmp1, tmp2;
    FCELL dat1, dat2;
    void * asp_raster, *asp_ptr = NULL;
    void * slp_raster, *slp_ptr = NULL;
    void * pcurv_raster, *pcurv_ptr = NULL;
    void * tcurv_raster, *tcurv_ptr = NULL;
    void * dx_raster, *dx_ptr = NULL;
    void * dy_raster, *dy_ptr = NULL;
    void * dxx_raster, *dxx_ptr = NULL;
    void * dyy_raster, *dyy_ptr = NULL;
    void * dxy_raster, *dxy_ptr = NULL;
    int i;
    RASTER_MAP_TYPE out_type = 0, data_type;
    int Wrap;  /* global wraparound */
    struct Cell_head window, cellhd;
    struct History  hist;
    struct Colors colors;

    char *elev_name;
    char *aspect_name;
    char *slope_name;
    char *pcurv_name;
    char *tcurv_name;
    char *dx_name;
    char *dy_name;
    char *dxx_name;
    char *dyy_name;
    char *dxy_name;
    char buf[300];
    char *mapset;
    int nrows, row;
    int ncols, col;

    double north, east, south, west, ns_med;

    double radians_to_degrees;
    double degrees_to_radians;
    double H,V;
    double dx;              /* partial derivative in ew direction */
    double dy;              /* partial derivative in ns direction */
    double dxx, dxy, dyy;
    double s3, s4, s5, s6;
    double pcurv, tcurv;
    double scik1 = 100000.;
    double zfactor;
    double factor;
    double aspect, min_asp=360., max_asp=0.;
    double dnorm1, dx2, dy2, grad2, grad, dxy2;
    double gradmin = 0.001;
    double c1min=0., c1max=0., c2min=0., c2max=0.;

    double answer[92];
    double degrees;
    double tan_ans;
    double key;
    double slp_in_perc, slp_in_deg;
    double min_slp=900., max_slp=0., min_slp_allowed;
    int low, hi, test = 0;
    int deg=0;
    int perc=0;
    char *slope_fmt;
    struct GModule *module;
    struct
    {
	struct Option *elevation, *slope_fmt, *slope, *aspect, *pcurv, *tcurv,
		*zfactor, *min_slp_allowed, *out_precision,
		*dx, *dy, *dxx, *dyy, *dxy;
    } parm;
    struct
    {
	struct Flag *a;
    /* please, remove before GRASS 7 released */
	struct Flag *q;
    } flag;

    G_gisinit (argv[0]);

    module = G_define_module();
    module->keywords = _("raster");
    module->description =
	_("Generates raster map layers of slope, aspect, curvatures and "
	  "partial derivatives from a raster map layer of true elevation "
	  "values. Aspect is calculated counterclockwise from east.");

    parm.elevation = G_define_option() ;
    parm.elevation->key        = "elevation" ;
    parm.elevation->type       = TYPE_STRING ;
    parm.elevation->required   = YES ;
    parm.elevation->gisprompt  = "old,cell,raster" ;
    parm.elevation->description= _("Raster elevation file name");

    parm.slope = G_define_option() ;
    parm.slope->key        = "slope" ;
    parm.slope->type       = TYPE_STRING ;
    parm.slope->required   = NO ;
    parm.slope->answer     = NULL ;
    parm.slope->gisprompt  = "new,cell,raster" ;
    parm.slope->description= _("Output slope filename") ;

    parm.aspect = G_define_option() ;
    parm.aspect->key        = "aspect" ;
    parm.aspect->type       = TYPE_STRING ;
    parm.aspect->required   = NO ;
    parm.aspect->answer     = NULL ;
    parm.aspect->gisprompt  = "new,cell,raster" ;
    parm.aspect->description= _("Output aspect filename") ;

    parm.slope_fmt = G_define_option() ;
    parm.slope_fmt->key        = "format" ;
    parm.slope_fmt->type       = TYPE_STRING ;
    parm.slope_fmt->required   = NO ;
    parm.slope_fmt->answer     = "degrees";
    parm.slope_fmt->options    = "degrees,percent";
    parm.slope_fmt->description= _("Format for reporting the slope") ;
    parm.slope_fmt->guisection = _("Settings");

    parm.out_precision = G_define_option() ;
    parm.out_precision->key        = "prec";
    parm.out_precision->type       = TYPE_STRING ;
    parm.out_precision->required   = NO ;
    parm.out_precision->answer     = "float";
    parm.out_precision->options    = "default,double,float,int";
    parm.out_precision->description= _("Type of output aspect and slope maps") ;
    parm.out_precision->guisection = _("Settings");

    parm.pcurv = G_define_option() ;
    parm.pcurv->key        = "pcurv" ;
    parm.pcurv->type       = TYPE_STRING ;
    parm.pcurv->required   = NO ;
    parm.pcurv->answer     = NULL ;
    parm.pcurv->gisprompt  = "new,cell,raster" ;
    parm.pcurv->description= _("Output profile curvature filename" );
    parm.pcurv->guisection = _("Advanced");

    parm.tcurv = G_define_option() ;
    parm.tcurv->key        = "tcurv" ;
    parm.tcurv->type       = TYPE_STRING ;
    parm.tcurv->required   = NO ;
    parm.tcurv->answer     = NULL ;
    parm.tcurv->gisprompt  = "new,cell,raster" ;
    parm.tcurv->description= _("Output tangential curvature filename") ;
    parm.tcurv->guisection = _("Advanced");

    parm.dx = G_define_option() ;
    parm.dx->key        = "dx" ;
    parm.dx->type       = TYPE_STRING ;
    parm.dx->required   = NO ;
    parm.dx->answer     = NULL ;
    parm.dx->gisprompt  = "new,cell,raster" ;
    parm.dx->description= _("Output first order partial derivative dx (E-W slope) filename") ;
    parm.dx->guisection = _("Advanced");

    parm.dy = G_define_option() ;
    parm.dy->key        = "dy" ;
    parm.dy->type       = TYPE_STRING ;
    parm.dy->required   = NO ;
    parm.dy->answer     = NULL ;
    parm.dy->gisprompt  = "new,cell,raster" ;
    parm.dy->description= _("Output first order partial derivative dy (N-S slope) filename") ;
    parm.dy->guisection = _("Advanced");

    parm.dxx = G_define_option() ;
    parm.dxx->key        = "dxx" ;
    parm.dxx->type       = TYPE_STRING ;
    parm.dxx->required   = NO ;
    parm.dxx->answer     = NULL ;
    parm.dxx->gisprompt  = "new,cell,raster" ;
    parm.dxx->description= _("Output second order partial derivative dxx filename") ;
    parm.dxx->guisection = _("Advanced");

    parm.dyy = G_define_option() ;
    parm.dyy->key        = "dyy" ;
    parm.dyy->type       = TYPE_STRING ;
    parm.dyy->required   = NO ;
    parm.dyy->answer     = NULL ;
    parm.dyy->gisprompt  = "new,cell,raster" ;
    parm.dyy->description= _("Output second order partial derivative dyy filename") ;
    parm.dyy->guisection = _("Advanced");

    parm.dxy = G_define_option() ;
    parm.dxy->key        = "dxy" ;
    parm.dxy->type       = TYPE_STRING ;
    parm.dxy->required   = NO ;
    parm.dxy->answer     = NULL ;
    parm.dxy->gisprompt  = "new,cell,raster" ;
    parm.dxy->description= _("Output second order partial derivative dxy filename") ;
    parm.dxy->guisection = _("Advanced");

    parm.zfactor = G_define_option();
    parm.zfactor->key         = "zfactor";
    parm.zfactor->description = _("Multiplicative factor to convert elevation units to meters");
    parm.zfactor->type        = TYPE_DOUBLE;
    parm.zfactor->required    = NO;
    parm.zfactor->answer      = "1.0";
    parm.zfactor->guisection  = _("Settings");

    parm.min_slp_allowed = G_define_option();
    parm.min_slp_allowed->key         = "min_slp_allowed";
    parm.min_slp_allowed->description = _("Minimum slope val. (in percent) for which aspect is computed");
    parm.min_slp_allowed->type        = TYPE_DOUBLE;
    parm.min_slp_allowed->required    = NO;
    parm.min_slp_allowed->answer      = "0.0";
    parm.min_slp_allowed->guisection  = _("Settings");

    /* please, remove before GRASS 7 released */
    flag.q = G_define_flag() ;
    flag.q->key         = 'q' ;
    flag.q->description = _("Quiet") ;

    flag.a = G_define_flag() ;
    flag.a->key         = 'a' ;
    flag.a->description = _("Do not align the current region to the elevation layer") ;
    flag.a->guisection  = _("Settings");


    radians_to_degrees = 180.0 / M_PI ;
    degrees_to_radians = M_PI / 180.0 ;

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

    for (i = 0; i < 90; i++)
    {
        degrees = i + .5;
        tan_ans = tan ( degrees  / radians_to_degrees );
        answer[i] = tan_ans * tan_ans;
    }

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    /* please, remove before GRASS 7 released */
    if(flag.q->answer) {
        putenv("GRASS_VERBOSE=0");
        G_warning(_("The '-q' flag is superseded and will be removed "
            "in future. Please use '--quiet' instead."));
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

    G_check_input_output_name ( elev_name, slope_name, GR_FATAL_EXIT );
    G_check_input_output_name ( elev_name, aspect_name, GR_FATAL_EXIT );
    G_check_input_output_name ( elev_name, pcurv_name, GR_FATAL_EXIT );
    G_check_input_output_name ( elev_name, tcurv_name, GR_FATAL_EXIT );
    G_check_input_output_name ( elev_name, dx_name, GR_FATAL_EXIT );
    G_check_input_output_name ( elev_name, dy_name, GR_FATAL_EXIT );
    G_check_input_output_name ( elev_name, dxx_name, GR_FATAL_EXIT );
    G_check_input_output_name ( elev_name, dyy_name, GR_FATAL_EXIT );
    G_check_input_output_name ( elev_name, dxy_name, GR_FATAL_EXIT );

    if (sscanf (parm.zfactor->answer, "%lf", &zfactor) != 1 || zfactor <= 0.0)
    {
        G_warning("%s=%s - must be a postive number", parm.zfactor->key,
                       parm.zfactor->answer);
        G_usage();
        exit(EXIT_FAILURE);
    }

    if (sscanf (parm.min_slp_allowed->answer, "%lf", &min_slp_allowed) != 1 || min_slp_allowed < 0.0)
    {
        G_warning("%s=%s - must be a non-negative number", parm.min_slp_allowed->key,
                       parm.min_slp_allowed->answer);
        G_usage();
        exit(EXIT_FAILURE);
    }

    slope_fmt = parm.slope_fmt->answer;
    if(strcmp(slope_fmt,"percent")==0)perc=1;
    else if(strcmp(slope_fmt,"degrees")==0)deg=1;

    if (slope_name == NULL && aspect_name == NULL
        && pcurv_name == NULL && tcurv_name == NULL
	&& dx_name == NULL && dy_name == NULL 
	&& dxx_name == NULL && dyy_name == NULL && dxy_name == NULL)
    {
	G_warning("You must specify at least one of the parameters:"
		"\n<%s>, <%s>, <%s>, <%s>, <%s>, <%s>, <%s>, <%s> or <%s>.\n", 
		parm.slope->key, parm.aspect->key, parm.pcurv->key, 
		parm.tcurv->key, parm.dx->key, parm.dy->key, 
		parm.dxx->key, parm.dyy->key, parm.dxy->key);
	G_usage();
	exit(EXIT_FAILURE);
    }

    /* check elevation file existence */
    mapset = G_find_cell2(elev_name, "");
    if (!mapset)
        G_fatal_error (_("Raster map <%s> not found"), elev_name);

    /* set the window from the header for the elevation file */
    if (!flag.a->answer)
    {
	G_get_window (&window);
	if (G_get_cellhd (elev_name, mapset, &cellhd) >= 0)
	{
	    G_align_window (&window, &cellhd);
	    G_set_window (&window);
	}
    }
    
   if(strcmp(parm.out_precision->answer, "double") == 0)
       out_type = DCELL_TYPE;
   else if(strcmp(parm.out_precision->answer, "float") == 0)
       out_type = FCELL_TYPE;
   else if(strcmp(parm.out_precision->answer, "int") == 0)
       out_type = CELL_TYPE;
   else if(strcmp(parm.out_precision->answer, "default") == 0)
       out_type = -1;
   else
        G_fatal_error(_("wrong type: %s"), parm.out_precision->answer);

   data_type = out_type;
   if(data_type < 0) data_type = DCELL_TYPE;
   /* data type is the type of data being processed,
      out_type is type of map being created */

    G_get_set_window (&window);

    nrows = G_window_rows();
    ncols = G_window_cols();

    if (((window.west==(window.east-360.)) 
          ||(window.east==(window.west-360.)))&&
	  (G_projection()==PROJECTION_LL))
    {
       Wrap = 1;
       ncols+=2; 
    }
    else Wrap = 0;

    /* H = window.ew_res * 4 * 2/ zfactor;*/  /* horizontal (east-west) run 
                                   times 4 for weighted difference */
    /* V = window.ns_res * 4 * 2/ zfactor;*/  /* vertical (north-south) run 
                                   times 4 for weighted difference */

   /* give warning if location units are different from meters and zfactor=1*/
    factor = G_database_units_to_meters_factor();
    if (factor != 1.0)
        G_warning("Converting units to meters, factor=%.6f", factor);

    G_begin_distance_calculations();
    north = G_row_to_northing(0.5, &window);
    ns_med = G_row_to_northing(1.5, &window);
    south = G_row_to_northing(2.5, &window);
    east =  G_col_to_easting(2.5, &window);
    west =  G_col_to_easting(0.5, &window);
    V = G_distance(east, north, east, south) * 4 / zfactor;
    H = G_distance(east, ns_med, west, ns_med) * 4 / zfactor;
    /*    ____________________________
	  |c1      |c2      |c3      |
	  |        |        |        |
	  |        |  north |        |        
	  |        |        |        |
	  |________|________|________|          
	  |c4      |c5      |c6      |
	  |        |        |        |
	  |  east  | ns_med |  west  |
	  |        |        |        |
	  |________|________|________|
	  |c7      |c8      |c9      |
	  |        |        |        |
	  |        |  south |        |
	  |        |        |        |
	  |________|________|________|
    */

    /* open the elevation file for reading */
    elevation_fd = G_open_cell_old (elev_name, mapset);
    if (elevation_fd < 0) exit(EXIT_FAILURE);
    elev_cell[0] = (DCELL *) G_calloc (ncols + 1, sizeof(DCELL));
    G_set_d_null_value(elev_cell[0], ncols);
    elev_cell[1] = (DCELL *) G_calloc (ncols + 1, sizeof(DCELL));
    G_set_d_null_value(elev_cell[1], ncols);
    elev_cell[2] = (DCELL *) G_calloc (ncols + 1, sizeof(DCELL));
    G_set_d_null_value(elev_cell[2], ncols);

    if (slope_name != NULL)
    {
        slope_fd = opennew (slope_name, out_type);
	slp_raster = G_allocate_raster_buf(data_type);
	G_set_null_value(slp_raster, G_window_cols(), data_type);
	G_put_raster_row(slope_fd, slp_raster, data_type);
    }
    else
    {
	slp_raster = NULL;
	slope_fd = -1;
    }

    if (aspect_name != NULL)
    {
        aspect_fd = opennew (aspect_name, out_type);
	asp_raster = G_allocate_raster_buf(data_type);
	G_set_null_value(asp_raster, G_window_cols(), data_type);
	G_put_raster_row(aspect_fd, asp_raster, data_type);
    }
    else
    {
	asp_raster = NULL;
	aspect_fd = -1;
    }

    if (pcurv_name != NULL)
    {
        pcurv_fd = opennew (pcurv_name, out_type);
        pcurv_raster = G_allocate_raster_buf(data_type);
        G_set_null_value (pcurv_raster, G_window_cols(), data_type);
        G_put_raster_row (pcurv_fd, pcurv_raster, data_type);
    }
    else
    {
        pcurv_raster = NULL;
        pcurv_fd = -1;
    }

    if (tcurv_name != NULL)
    {
        tcurv_fd = opennew (tcurv_name, out_type);
        tcurv_raster = G_allocate_raster_buf(data_type);
        G_set_null_value (tcurv_raster, G_window_cols(), data_type);
        G_put_raster_row (tcurv_fd, tcurv_raster, data_type);
    }
    else
    {
        tcurv_raster = NULL;
        tcurv_fd = -1;
    }

    if (dx_name != NULL)
    {
        dx_fd = opennew (dx_name, out_type);
        dx_raster = G_allocate_raster_buf(data_type);
        G_set_null_value (dx_raster, G_window_cols(), data_type);
        G_put_raster_row (dx_fd, dx_raster, data_type);
    }
    else
    {
        dx_raster = NULL;
        dx_fd = -1;
    }

    if (dy_name != NULL)
    {
        dy_fd = opennew (dy_name, out_type);
        dy_raster = G_allocate_raster_buf(data_type);
        G_set_null_value (dy_raster, G_window_cols(), data_type);
        G_put_raster_row (dy_fd, dy_raster, data_type);
    }
    else
    {
        dy_raster = NULL;
        dy_fd = -1;
    }

    if (dxx_name != NULL)
    {
        dxx_fd = opennew (dxx_name, out_type);
        dxx_raster = G_allocate_raster_buf(data_type);
        G_set_null_value (dxx_raster, G_window_cols(), data_type);
        G_put_raster_row (dxx_fd, dxx_raster, data_type);
    }
    else
    {
        dxx_raster = NULL;
        dxx_fd = -1;
    }

    if (dyy_name != NULL)
    {
        dyy_fd = opennew (dyy_name, out_type);
        dyy_raster = G_allocate_raster_buf(data_type);
        G_set_null_value (dyy_raster, G_window_cols(), data_type);
        G_put_raster_row (dyy_fd, dyy_raster, data_type);
    }
    else
    {
        dyy_raster = NULL;
        dyy_fd = -1;
    }

    if (dxy_name != NULL)
    {
        dxy_fd = opennew (dxy_name, out_type);
        dxy_raster = G_allocate_raster_buf(data_type);
        G_set_null_value (dxy_raster, G_window_cols(), data_type);
        G_put_raster_row (dxy_fd, dxy_raster, data_type);
    }
    else
    {
        dxy_raster = NULL;
        dxy_fd = -1;
    }

    if (aspect_fd < 0 && slope_fd < 0 && pcurv_fd < 0 && tcurv_fd < 0
	&& dx_fd < 0 && dy_fd < 0 && dxx_fd < 0 && dyy_fd < 0 && dxy_fd < 0)
        exit(EXIT_FAILURE);

    if(Wrap)
    {
       G_get_d_raster_row_nomask (elevation_fd, elev_cell[1]+1,0);
       elev_cell[1][0] = elev_cell[1][G_window_cols()-1];
       elev_cell[1][G_window_cols()+1]=elev_cell[1][2];
    }
    else G_get_d_raster_row_nomask (elevation_fd, elev_cell[1],0);

    if(Wrap)
    {
       G_get_d_raster_row_nomask (elevation_fd, elev_cell[2]+1,1);
       elev_cell[2][0] = elev_cell[2][G_window_cols()-1];
       elev_cell[2][G_window_cols()+1]=elev_cell[2][2];
    }
    else G_get_d_raster_row_nomask (elevation_fd, elev_cell[2],1);

    G_message (_("Percent complete: "));
    for (row = 2; row < nrows; row++)
    {
        /*  if projection is Lat/Lon, recalculate  V and H   */
	if (G_projection()==PROJECTION_LL)
	{
          north = G_row_to_northing((row-2 + 0.5), &window);
          ns_med = G_row_to_northing((row-1 + 0.5), &window);
          south = G_row_to_northing((row + 0.5), &window);
          east =  G_col_to_easting(2.5, &window);
          west =  G_col_to_easting(0.5, &window);
          V = G_distance(east, north, east, south) * 4 / zfactor;
          H = G_distance(east, ns_med, west, ns_med) * 4 / zfactor;
/*        ____________________________
	  |c1      |c2      |c3      |
	  |        |        |        |
	  |        |  north |        |        
	  |        |        |        |
	  |________|________|________|          
	  |c4      |c5      |c6      |
	  |        |        |        |
	  |  east  | ns_med |  west  |
	  |        |        |        |
	  |________|________|________|
	  |c7      |c8      |c9      |
	  |        |        |        |
	  |        |  south |        |
	  |        |        |        |
	  |________|________|________|
*/
	}

        G_percent (row, nrows, 2);
        temp = elev_cell[0];
        elev_cell[0] = elev_cell[1];
        elev_cell[1] = elev_cell[2];
	elev_cell[2] = temp;

        if(Wrap)
        {
           G_get_d_raster_row_nomask (elevation_fd, elev_cell[2] + 1, row);
           elev_cell[2][0] = elev_cell[2][G_window_cols()-1];
           elev_cell[2][G_window_cols()+1]=elev_cell[2][2];
        }
        else G_get_d_raster_row_nomask (elevation_fd, elev_cell[2], row);

        c1 = elev_cell[0];
        c2 = c1+1;
        c3 = c1+2;
        c4 = elev_cell[1];
        c5 = c4+1;
        c6 = c4+2;
        c7 = elev_cell[2];
        c8 = c7+1;
        c9 = c7+2;

	if (aspect_fd >= 0)
	{
	    if(Wrap)
	       asp_ptr = asp_raster;
            else 
	       asp_ptr = G_incr_void_ptr(asp_raster, G_raster_size(data_type));
        }
	if (slope_fd >= 0)
	{
	    if(Wrap)
	       slp_ptr = slp_raster;
            else 
	       slp_ptr = G_incr_void_ptr(slp_raster, G_raster_size(data_type));
        }

        if (pcurv_fd >= 0)
        {
            if(Wrap)
               pcurv_ptr = pcurv_raster;
            else
               pcurv_ptr = G_incr_void_ptr(pcurv_raster, G_raster_size(data_type));
        }

        if (tcurv_fd >= 0)
        {
            if(Wrap)
               tcurv_ptr = tcurv_raster;
            else
               tcurv_ptr = G_incr_void_ptr(tcurv_raster, G_raster_size(data_type));
        }

        if (dx_fd >= 0)
        {
            if(Wrap)
               dx_ptr = dx_raster;
            else
               dx_ptr = G_incr_void_ptr(dx_raster, G_raster_size(data_type));
        }

        if (dy_fd >= 0)
        {
            if(Wrap)
               dy_ptr = dy_raster;
            else
               dy_ptr = G_incr_void_ptr(dy_raster, G_raster_size(data_type));
        }

        if (dxx_fd >= 0)
        {
            if(Wrap)
               dxx_ptr = dxx_raster;
            else
               dxx_ptr = G_incr_void_ptr(dxx_raster, G_raster_size(data_type));
        }

        if (dyy_fd >= 0)
        {
            if(Wrap)
               dyy_ptr = dyy_raster;
            else
               dyy_ptr = G_incr_void_ptr(dyy_raster, G_raster_size(data_type));
        }

        if (dxy_fd >= 0)
        {
            if(Wrap)
               dxy_ptr = dxy_raster;
            else
               dxy_ptr = G_incr_void_ptr(dxy_raster, G_raster_size(data_type));
        }


        /*skip first cell of the row*/

        for (col = ncols-2; col-- > 0; c1++,c2++,c3++,c4++,c5++,c6++,c7++,c8++,c9++)
        {
            /*  DEBUG:
        fprintf(stdout, "\n%.0f %.0f %.0f\n%.0f %.0f %.0f\n%.0f %.0f %.0f\n",
                 *c1, *c2, *c3, *c4, *c5, *c6, *c7, *c8, *c9);
            */

             if(G_is_d_null_value(c1) || G_is_d_null_value(c2) ||
                G_is_d_null_value(c3) || G_is_d_null_value(c4) || 
                G_is_d_null_value(c5) || G_is_d_null_value(c6) || 
                G_is_d_null_value(c7) || G_is_d_null_value(c8) || 
                G_is_d_null_value(c9))
	    {
		if(slope_fd > 0)
                {
                   G_set_null_value(slp_ptr, 1, data_type);
		   slp_ptr = G_incr_void_ptr(slp_ptr, G_raster_size(data_type));
                }
		if (aspect_fd > 0)
                {
                   G_set_null_value(asp_ptr, 1, data_type);
		   asp_ptr = G_incr_void_ptr(asp_ptr, G_raster_size(data_type));
                }
                if(pcurv_fd > 0)
                {
                   G_set_null_value(pcurv_ptr, 1, data_type);
                   pcurv_ptr = G_incr_void_ptr(pcurv_ptr, G_raster_size(data_type));
                }
                if (tcurv_fd > 0)
                {
                   G_set_null_value(tcurv_ptr, 1, data_type);
                   tcurv_ptr = G_incr_void_ptr(tcurv_ptr, G_raster_size(data_type));
                }
                if (dx_fd > 0)
                {
                   G_set_null_value(dx_ptr, 1, data_type);
                   dx_ptr = G_incr_void_ptr(dx_ptr, G_raster_size(data_type));
                }
                if (dy_fd > 0)
                {
                   G_set_null_value(dy_ptr, 1, data_type);
                   dy_ptr = G_incr_void_ptr(dy_ptr, G_raster_size(data_type));
                }
                if (dxx_fd > 0)
                {
                   G_set_null_value(dxx_ptr, 1, data_type);
                   dxx_ptr = G_incr_void_ptr(dxx_ptr, G_raster_size(data_type));
                }
                if (dyy_fd > 0)
                {
                   G_set_null_value(dyy_ptr, 1, data_type);
                   dyy_ptr = G_incr_void_ptr(dyy_ptr, G_raster_size(data_type));
                }
                if (dxy_fd > 0)
                {
                   G_set_null_value(dxy_ptr, 1, data_type);
                   dxy_ptr = G_incr_void_ptr(dxy_ptr, G_raster_size(data_type));
                }
		continue;
	    } /* no data */

	    dx = ((*c1 + *c4 + *c4 + *c7) - (*c3 + *c6 + *c6 + *c9)) / H;
	    dy = ((*c7 + *c8 + *c8 + *c9) - (*c1 + *c2 + *c2 + *c3)) / V;

	    /* compute topographic parameters */
            key = dx*dx + dy*dy;
	    slp_in_perc = 100*sqrt(key);  
            slp_in_deg = atan(sqrt(key)) * radians_to_degrees;

	    /* now update min and max */
	    if(deg)
	    {
	       if(min_slp > slp_in_deg) min_slp = slp_in_deg;
	       if(max_slp < slp_in_deg) max_slp = slp_in_deg;
            }
	    else
	    {
	       if(min_slp > slp_in_perc) min_slp = slp_in_perc;
	       if(max_slp < slp_in_perc) max_slp = slp_in_perc;
            }
	    if(slp_in_perc < min_slp_allowed) slp_in_perc = 0.;

	    if(deg && out_type == CELL_TYPE)
	    {
	    /* INC BY ONE
               low = 1;
               hi = 91;
             */
               low = 0;
               hi = 90;
               test = 20;

               while (hi >= low)
               {
                   if ( key >= answer[test] )
                       low = test + 1;
                   else if ( key < answer[test-1] )
                       hi = test - 1;
                   else
                       break;
                   test = (low + hi) / 2;
               }
            }
	    else if(perc && out_type == CELL_TYPE) 
	/* INCR_BY_ONE*/
                   /* test = slp_in_perc + 1.5;*/  /* All the slope categories are
						        incremented by 1 */
                   test = slp_in_perc + .5;

            if (slope_fd > 0)
            {
               if(data_type == CELL_TYPE)
                   *((CELL *) slp_ptr) = (CELL) test;
               else 
               {
                   if(deg) G_set_raster_value_d(slp_ptr, 
					   (DCELL)slp_in_deg, data_type);
                   else    G_set_raster_value_d(slp_ptr,
					   (DCELL ) slp_in_perc, data_type);
               }
	       slp_ptr = G_incr_void_ptr(slp_ptr, G_raster_size(data_type));
            } /* computing slope */

            if (aspect_fd > 0)
            {
                if (key == 0.) aspect = 0.;  
                else if (dx == 0)
                {
                    if (dy > 0) aspect = 90.;  
                    else aspect = 270.;   
                }
                else 
		{
		   aspect = (atan2(dy,dx)/degrees_to_radians);
		   if((aspect<=0.5)&&(aspect>0)&& out_type == CELL_TYPE) 
                                               aspect=360.;
		   if(aspect<=0.)aspect=360.+aspect;
                }

		/* if it's not the case that the slope for this cell 
		is below specified minimum */
                if(!((slope_fd > 0)&&(slp_in_perc < min_slp_allowed)))
		{
                    if(out_type == CELL_TYPE)
                        *((CELL *) asp_ptr) = (CELL) (aspect + .5);
                    else
                        G_set_raster_value_d(asp_ptr,
					   (DCELL ) aspect, data_type);
                }
		else
		    G_set_null_value(asp_ptr, 1, data_type);
	        asp_ptr = G_incr_void_ptr(asp_ptr, G_raster_size(data_type));

		/* now update min and max */
		if(min_asp > aspect) min_asp = aspect;
		if(max_asp < aspect) max_asp = aspect;
             } /* computing aspect */

	     if(dx_fd > 0)
	     {
                if (out_type == CELL_TYPE)
                    *((CELL *) dx_ptr) = (CELL) (scik1 * dx);
                else
                    G_set_raster_value_d(dx_ptr, (DCELL) dx, data_type);
                dx_ptr= G_incr_void_ptr(dx_ptr, G_raster_size(data_type));
	     }

	     if(dy_fd > 0)
	     {
                if (out_type == CELL_TYPE)
                    *((CELL *) dy_ptr) = (CELL) (scik1 * dy);
                else
                    G_set_raster_value_d(dy_ptr, (DCELL) dy, data_type);
                dy_ptr= G_incr_void_ptr(dy_ptr, G_raster_size(data_type));
	     }

             if(dxx_fd <= 0 && dxy_fd <= 0 && dyy_fd <= 0 &&
		pcurv_fd <= 0 && tcurv_fd <= 0 )
	       continue;

	     /* compute second order derivatives */
	     s4 = *c1 + *c3 + *c7 + *c9 - *c5 * 8.;
             s5 = *c4 * 4. + *c6 * 4. - *c8 * 2. - *c2 * 2.;
             s6 = *c8 * 4. + *c2 * 4. - *c4 * 2. - *c6 * 2.;
             s3 = *c7 - *c9 + *c3 - *c1;

             dxx = - (s4 + s5) / ((3./32.)*H*H);
             dyy = - (s4 + s6) / ((3./32.)*V*V);
             dxy = - s3 / ((1./16.)*H*V);

	     if(dxx_fd > 0)
	     {
                if (out_type == CELL_TYPE)
                    *((CELL *) dxx_ptr) = (CELL) (scik1 * dxx);
                else
                    G_set_raster_value_d(dxx_ptr, (DCELL) dxx, data_type);
                dxx_ptr= G_incr_void_ptr(dxx_ptr, G_raster_size(data_type));
	     }

	     if(dyy_fd > 0)
	     {
                if (out_type == CELL_TYPE)
                    *((CELL *) dyy_ptr) = (CELL) (scik1 * dyy);
                else
                    G_set_raster_value_d(dyy_ptr, (DCELL) dyy, data_type);
                dyy_ptr= G_incr_void_ptr(dyy_ptr, G_raster_size(data_type));
	     }

	     if(dxy_fd > 0)
	     {
                if (out_type == CELL_TYPE)
                    *((CELL *) dxy_ptr) = (CELL) (scik1 * dxy);
                else
                    G_set_raster_value_d(dxy_ptr, (DCELL) dxy, data_type);
                dxy_ptr= G_incr_void_ptr(dxy_ptr, G_raster_size(data_type));
	     }

	     /* compute curvature */
             if(pcurv_fd <= 0 && tcurv_fd <= 0 )
	       continue;

	     grad2 = key;         /*dx2 + dy2*/
	     grad = sqrt (grad2);
	     if (grad <= gradmin)
	     {
	       pcurv = 0.;
	       tcurv = 0.;
	     }
	     else
	     {
	       dnorm1 =  sqrt (grad2 + 1.);
	       dxy2 = 2. * dxy * dx * dy;
	       dx2 = dx * dx;
	       dy2 = dy * dy;
	       pcurv = (dxx * dx2 + dxy2 + dyy * dy2) / 
		 (grad2 * dnorm1*dnorm1*dnorm1);
	       tcurv = (dxx * dy2 - dxy2 + dyy * dx2) / 
		 (grad2 * dnorm1);
	       if(c1min > pcurv) c1min = pcurv;
	       if(c1max < pcurv) c1max = pcurv;
               if(c2min > tcurv) c2min = tcurv;
               if(c2max < tcurv) c2max = tcurv;
	     }

	     if (pcurv_fd > 0)
	     {
	        if (out_type == CELL_TYPE)
	            *((CELL *) pcurv_ptr) = (CELL) (scik1 * pcurv);
	        else
	            G_set_raster_value_d(pcurv_ptr, (DCELL) pcurv, data_type);
	        pcurv_ptr= G_incr_void_ptr(pcurv_ptr, G_raster_size(data_type));
	     }

	     if (tcurv_fd > 0)
	     {
	        if (out_type == CELL_TYPE)
	            *((CELL *) tcurv_ptr) = (CELL) (scik1 * tcurv);
	        else
	            G_set_raster_value_d(tcurv_ptr, (DCELL) tcurv, data_type);
	        tcurv_ptr= G_incr_void_ptr(tcurv_ptr, G_raster_size(data_type));
	     }

        } /* column for loop */

        if (aspect_fd > 0)
	    G_put_raster_row(aspect_fd, asp_raster, data_type);

        if (slope_fd > 0)
	    G_put_raster_row(slope_fd, slp_raster, data_type);

        if (pcurv_fd > 0)
            G_put_raster_row(pcurv_fd, pcurv_raster, data_type);

        if (tcurv_fd > 0)
            G_put_raster_row(tcurv_fd, tcurv_raster, data_type);

        if (dx_fd > 0)
            G_put_raster_row(dx_fd, dx_raster, data_type);

        if (dy_fd > 0)
            G_put_raster_row(dy_fd, dy_raster, data_type);

        if (dxx_fd > 0)
            G_put_raster_row(dxx_fd, dxx_raster, data_type);

        if (dyy_fd > 0)
            G_put_raster_row(dyy_fd, dyy_raster, data_type);

        if (dxy_fd > 0)
            G_put_raster_row(dxy_fd, dxy_raster, data_type);

    } /* row loop */

    G_percent (row, nrows, 2);

    G_close_cell (elevation_fd);
    G_message(_("Creating support files..."));

    G_message(_("Elevation products for mapset [%s] in [%s]"),
        G_mapset(), G_location());

    if (aspect_fd >= 0)
    {
        DCELL min, max;
        struct FPRange range;

        G_set_null_value(asp_raster, G_window_cols(), data_type);
        G_put_raster_row (aspect_fd, asp_raster, data_type);
        G_close_cell (aspect_fd);

        if(out_type != CELL_TYPE)
           G_quantize_fp_map_range(aspect_name, G_mapset(), 0., 360., 0, 360);

        G_read_raster_cats (aspect_name, G_mapset(), &cats);
        G_set_raster_cats_title ("Aspect counterclockwise in degrees from east", &cats);

	G_message(_("Min computed aspect %.4f, max computed aspect %.4f"), min_asp, max_asp);
	/* the categries quant intervals are 1.0 long, plus
	   we are using reverse order so that the label looked up
	   for i-.5 is not the one defined for i-.5, i+.5 interval, but
	   the one defile for i-1.5, i-.5 interval which is added later */
	for(i = ceil(max_asp); i >= 1; i--)
	{
	       if(i==360)sprintf(buf,"east");
	       else if(i==360)sprintf(buf,"east");
	       else if(i==45)sprintf(buf,"north ccw of east");
	       else if(i==90)sprintf(buf,"north");
	       else if(i==135)sprintf(buf,"north ccw of west");
	       else if(i==180)sprintf(buf,"west");
	       else if(i==225)sprintf(buf,"south ccw of west");
	       else if(i==270)sprintf(buf,"south");
	       else if(i==315)sprintf(buf,"south ccw of east");
               else sprintf (buf, "%d degree%s ccw from east", i, i==1?"":"s");
	       if(data_type==CELL_TYPE) 
	       {
		  G_set_cat(i, buf, &cats);
		  continue;
               }
	       tmp1 = (double) i - .5;
	       tmp2 = (double) i + .5;
               G_set_d_raster_cat (&tmp1, &tmp2, buf, &cats);
	}
	if(data_type==CELL_TYPE)
	       G_set_cat(0, "no aspect", &cats);
        else
	{
	   tmp1 = 0.;
	   tmp2 = .5;
           G_set_d_raster_cat (&tmp1, &tmp2, "no aspect", &cats);
	}
        G_write_raster_cats (aspect_name, &cats);
        G_free_raster_cats (&cats);

        /* write colors for aspect file */
        G_init_colors (&colors);
        G_read_fp_range (aspect_name, G_mapset(), &range);
        G_get_fp_range_min_max (&range, &min, &max);
        G_make_aspect_fp_colors (&colors, min, max);
        G_write_colors (aspect_name, G_mapset(), &colors);

        /* writing history file */
        G_short_history(aspect_name, "raster", &hist);
        sprintf(hist.edhist[0], "aspect map elev = %s", elev_name); 
        sprintf(hist.edhist[1], "zfactor = %.2f", zfactor); 
        sprintf(hist.edhist[2], "min_slp_allowed = %f", min_slp_allowed); 
        sprintf(hist.datsrc_1,"raster elevation file %s", elev_name);
        hist.edlinecnt = 3;
        G_write_history (aspect_name, &hist);

        G_message(_("ASPECT [%s] COMPLETE"), aspect_name);
    }

    if (slope_fd >= 0)
    {
      /* colortable for slopes */
      G_init_colors (&colors);
      G_add_color_rule (0, 255, 255, 255, 2, 255, 255, 0, &colors);
      G_add_color_rule (2, 255, 255, 0, 5, 0, 255, 0, &colors);
      G_add_color_rule (5, 0, 255, 0, 10, 0, 255, 255, &colors);
      G_add_color_rule (10, 0, 255, 255, 15, 0, 0, 255, &colors);
      G_add_color_rule (15, 0, 0, 255, 30, 255, 0, 255, &colors);
      G_add_color_rule (30, 255, 0, 255, 50, 255, 0, 0, &colors);
      G_add_color_rule (50, 255, 0, 0, 90, 0, 0, 0, &colors);
	    
        G_set_null_value(slp_raster, G_window_cols(), data_type);
        G_put_raster_row (slope_fd, slp_raster, data_type);
        G_close_cell (slope_fd);

        if (out_type != CELL_TYPE)
        {
	/* INCR_BY_ONE
           if(deg)
               G_quantize_fp_map_range(slope_name, G_mapset(), 0., 90., 1, 91);
           else
               G_quantize_fp_map_range(slope_name, G_mapset(), min_slp, max_slp, 
				  (CELL) min_slp + 1, (CELL) ceil(max_slp) + 1);
        */
           G_write_colors (slope_name, G_mapset(), &colors);
           if(deg)
               G_quantize_fp_map_range(slope_name, G_mapset(), 0., 90., 0, 90);
           else /* percent */
               G_quantize_fp_map_range(slope_name, G_mapset(), min_slp, max_slp, 
				  (CELL) min_slp, (CELL) ceil(max_slp));
        }

        G_read_raster_cats (slope_name, G_mapset(), &cats);
        if(deg) G_set_raster_cats_title ("slope in degrees", &cats);
        else if(perc) G_set_raster_cats_title ("percent slope", &cats);

	G_message(_("Min computed slope %.4f, max computed slope %.4f"), min_slp, max_slp);
	/* the categries quant intervals are 1.0 long, plus
	   we are using reverse order so that the label looked up
	   for i-.5 is not the one defined for i-.5, i+.5 interval, but
	   the one defined for i-1.5, i-.5 interval which is added later */
        for (i = ceil(max_slp); i>/* INC BY ONE >= */ 0; i--)
        {
            if(deg)sprintf (buf, "%d degree%s", i, i==1?"":"s");
            else if(perc)sprintf (buf, "%d percent", i);
	    if(data_type==CELL_TYPE)
	    {
	/* INCR_BY_ONE
		G_set_cat(i+1, buf, &cats);
		*/
		G_set_cat(i, buf, &cats);
		continue;
            }
	/* INCR_BY_ONE
	    tmp1 = (DCELL) i+.5;
	    tmp2 = (DCELL) i+1.5;
        */
	    tmp1 = (DCELL) i-.5;
	    tmp2 = (DCELL) i+.5;
            G_set_d_raster_cat (&tmp1, &tmp2, buf, &cats);
        }
	if(data_type==CELL_TYPE)
	       G_set_cat(0, "zero slope", &cats);
	/* INCR_BY_ONE
	       G_set_cat(0, "no data", &cats);
	       */
        else
        {
	   tmp1 = 0;
	   tmp2 = 0.5;
           G_set_d_raster_cat (&tmp1, &tmp2, "zero slope", &cats);
	}
	/* INCR_BY_ONE
        G_set_d_raster_cat (&tmp1, &tmp1, "no data", &cats);
	*/
        G_write_raster_cats (slope_name, &cats);

        /* writing history file */
        G_short_history(slope_name, "raster", &hist);
        sprintf(hist.edhist[0], "slope map elev = %s", elev_name); 
        sprintf(hist.edhist[1], "zfactor = %.2f format = %s", zfactor, parm.slope_fmt->answer); 
        sprintf(hist.edhist[2], "min_slp_allowed = %f", min_slp_allowed); 
        sprintf(hist.datsrc_1,"raster elevation file %s", elev_name);
        hist.edlinecnt = 3;
        G_write_history (slope_name, &hist);

        G_message(_("SLOPE [%s] COMPLETE"), slope_name);
    }

    /* colortable for curvatures */
        if (pcurv_fd >= 0 || tcurv_fd >= 0)
	{
      G_init_colors (&colors);
      if (c1min < c2min) dat1 = (FCELL) c1min;
      else dat1 = (FCELL) c2min;
                                              
      dat2 = (FCELL) - 0.01;
      G_add_f_raster_color_rule (&dat1, 127, 0, 255,
                                 &dat2, 0, 0, 255, &colors);
      dat1 = dat2;
      dat2 = (FCELL) - 0.001;
      G_add_f_raster_color_rule (&dat1, 0, 0, 255,
                                 &dat2, 0, 127, 255, &colors);
      dat1 = dat2;
      dat2 = (FCELL) - 0.00001;
      G_add_f_raster_color_rule (&dat1, 0, 127, 255,
                                 &dat2, 0, 255, 255, &colors);
      dat1 = dat2;
      dat2 = (FCELL) 0.0;
      G_add_f_raster_color_rule (&dat1, 0, 255, 255,
                                 &dat2, 200, 255, 200, &colors);
      dat1 = dat2;
      dat2 = (FCELL) 0.00001;
      G_add_f_raster_color_rule (&dat1, 200, 255, 200,
                                 &dat2, 255, 255, 0, &colors);
      dat1 = dat2;
      dat2 = (FCELL) 0.001;
      G_add_f_raster_color_rule (&dat1, 255, 255, 0,
                                 &dat2, 255, 127, 0, &colors);
      dat1 = dat2;
      dat2 = (FCELL) 0.01;
      G_add_f_raster_color_rule (&dat1, 255, 127, 0,
                                 &dat2, 255, 0, 0, &colors);
      dat1 = dat2;
      if(c1max > c2max) dat2 = (FCELL) c1max;
      else dat2 = (FCELL) c2max;
                                      
      G_add_f_raster_color_rule (&dat1, 255, 0, 0,
                                 &dat2, 255, 0, 200, &colors);
	}

    if (pcurv_fd >= 0)
    {
        G_set_null_value(pcurv_raster, G_window_cols(), data_type);
        G_put_raster_row (pcurv_fd, pcurv_raster, data_type);
        G_close_cell (pcurv_fd);

        G_write_colors (pcurv_name, G_mapset(), &colors);

	if (out_type != CELL_TYPE)
           G_round_fp_map(pcurv_name, G_mapset());

	G_read_cats (pcurv_name, G_mapset(), &cats);
        G_set_cats_title ("profile curvature", &cats);
	G_set_cat ((CELL)0, "no profile curve", &cats);

        /* writing history file */
        G_short_history(pcurv_name, "raster", &hist);
        sprintf(hist.edhist[0], "profile curve map elev = %s", elev_name);
        sprintf(hist.edhist[1], "zfactor = %.2f", zfactor);
        sprintf(hist.edhist[2], "min_slp_allowed = %f", min_slp_allowed);
        sprintf(hist.datsrc_1,"raster elevation file %s", elev_name);
        hist.edlinecnt = 3;
        G_write_history (pcurv_name, &hist);

        G_message(_("PROFILE CURVE [%s] COMPLETE"), pcurv_name);
    }

    if (tcurv_fd >= 0)
    {
        G_set_null_value(tcurv_raster, G_window_cols(), data_type);
        G_put_raster_row (tcurv_fd, tcurv_raster, data_type);
        G_close_cell (tcurv_fd);

        G_write_colors (tcurv_name, G_mapset(), &colors);
	
	if (out_type != CELL_TYPE)
           G_round_fp_map(tcurv_name, G_mapset());

	G_read_cats (tcurv_name, G_mapset(), &cats);
        G_set_cats_title ("tangential curvature", &cats);
	G_set_cat ((CELL)0, "no tangential curve", &cats);

        /* writing history file */
        G_short_history(tcurv_name, "raster", &hist);
        sprintf(hist.edhist[0], "tangential curve map elev = %s", elev_name);
        sprintf(hist.edhist[1], "zfactor = %.2f", zfactor);
        sprintf(hist.edhist[2], "min_slp_allowed = %f", min_slp_allowed);
        sprintf(hist.datsrc_1,"raster elevation file %s", elev_name);
        hist.edlinecnt = 3;
        G_write_history (tcurv_name, &hist);

        G_message(_("TANGENTIAL CURVE [%s] COMPLETE"), tcurv_name);
    }   

    if (dx_fd >= 0)
    {
        G_set_null_value(dx_raster, G_window_cols(), data_type);
        G_put_raster_row (dx_fd, dx_raster, data_type);
        G_close_cell (dx_fd);

	if (out_type != CELL_TYPE)
           G_round_fp_map(dx_name, G_mapset());

	G_read_cats (dx_name, G_mapset(), &cats);
        G_set_cats_title ("E-W slope", &cats);
	G_set_cat ((CELL)0, "no E-W slope", &cats);

        /* writing history file */
        G_short_history(dx_name, "raster", &hist);
        sprintf(hist.edhist[0], "E-W slope map elev = %s", elev_name);
        sprintf(hist.edhist[1], "zfactor = %.2f", zfactor);
        sprintf(hist.edhist[2], "min_slp_allowed = %f", min_slp_allowed);
        sprintf(hist.datsrc_1,"raster elevation file %s", elev_name);
        hist.edlinecnt = 3;
        G_write_history (dx_name, &hist);

        G_message(_("E-W SLOPE [%s] COMPLETE"), dx_name);
    }   

    if (dy_fd >= 0)
    {
        G_set_null_value(dy_raster, G_window_cols(), data_type);
        G_put_raster_row (dy_fd, dy_raster, data_type);
        G_close_cell (dy_fd);

	if (out_type != CELL_TYPE)
           G_round_fp_map(dy_name, G_mapset());

	G_read_cats (dy_name, G_mapset(), &cats);
        G_set_cats_title ("N-S slope", &cats);
	G_set_cat ((CELL)0, "no N-S slope", &cats);

        /* writing history file */
        G_short_history(dy_name, "raster", &hist);
        sprintf(hist.edhist[0], "N-S slope map elev = %s", elev_name);
        sprintf(hist.edhist[1], "zfactor = %.2f", zfactor);
        sprintf(hist.edhist[2], "min_slp_allowed = %f", min_slp_allowed);
        sprintf(hist.datsrc_1,"raster elevation file %s", elev_name);
        hist.edlinecnt = 3;
        G_write_history (dy_name, &hist);

        G_message(_("N-S SLOPE [%s] COMPLETE"), dy_name);
    }   

    if (dxx_fd >= 0)
    {
        G_set_null_value(dxx_raster, G_window_cols(), data_type);
        G_put_raster_row (dxx_fd, dxx_raster, data_type);
        G_close_cell (dxx_fd);

	if (out_type != CELL_TYPE)
           G_round_fp_map(dxx_name, G_mapset());

	G_read_cats (dxx_name, G_mapset(), &cats);
        G_set_cats_title ("DXX", &cats);
	G_set_cat ((CELL)0, "DXX", &cats);

        /* writing history file */
        G_short_history(dxx_name, "raster", &hist);
        sprintf(hist.edhist[0], "DXX map elev = %s", elev_name);
        sprintf(hist.edhist[1], "zfactor = %.2f", zfactor);
        sprintf(hist.edhist[2], "min_slp_allowed = %f", min_slp_allowed);
        sprintf(hist.datsrc_1,"raster elevation file %s", elev_name);
        hist.edlinecnt = 3;
        G_write_history (dxx_name, &hist);

        G_message(_("DXX [%s] COMPLETE"), dxx_name);
    }   

    if (dyy_fd >= 0)
    {
        G_set_null_value(dyy_raster, G_window_cols(), data_type);
        G_put_raster_row (dyy_fd, dyy_raster, data_type);
        G_close_cell (dyy_fd);

	if (out_type != CELL_TYPE)
           G_round_fp_map(dyy_name, G_mapset());

	G_read_cats (dyy_name, G_mapset(), &cats);
        G_set_cats_title ("DYY", &cats);
	G_set_cat ((CELL)0, "DYY", &cats);

        /* writing history file */
        G_short_history(dyy_name, "raster", &hist);
        sprintf(hist.edhist[0], "DYY map elev = %s", elev_name);
        sprintf(hist.edhist[1], "zfactor = %.2f", zfactor);
        sprintf(hist.edhist[2], "min_slp_allowed = %f", min_slp_allowed);
        sprintf(hist.datsrc_1,"raster elevation file %s", elev_name);
        hist.edlinecnt = 3;
        G_write_history (dyy_name, &hist);

        G_message(_("DYY [%s] COMPLETE"), dyy_name);
    }   

    if (dxy_fd >= 0)
    {
        G_set_null_value(dxy_raster, G_window_cols(), data_type);
        G_put_raster_row (dxy_fd, dxy_raster, data_type);
        G_close_cell (dxy_fd);

	if (out_type != CELL_TYPE)
           G_round_fp_map(dxy_name, G_mapset());

	G_read_cats (dxy_name, G_mapset(), &cats);
        G_set_cats_title ("DXY", &cats);
	G_set_cat ((CELL)0, "DXY", &cats);

        /* writing history file */
        G_short_history(dxy_name, "raster", &hist);
        sprintf(hist.edhist[0], "DXY map elev = %s", elev_name);
        sprintf(hist.edhist[1], "zfactor = %.2f", zfactor);
        sprintf(hist.edhist[2], "min_slp_allowed = %f", min_slp_allowed);
        sprintf(hist.datsrc_1,"raster elevation file %s", elev_name);
        hist.edlinecnt = 3;
        G_write_history (dxy_name, &hist);

        G_message(_("DXY [%s] COMPLETE"), dxy_name);
    }   

    exit(EXIT_SUCCESS);
}




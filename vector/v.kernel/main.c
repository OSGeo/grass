
/****************************************************************************
*
* MODULE:       v.kernel
*
* AUTHOR(S):    Stefano Menegon, ITC-irst, Trento, Italy
*               Radim Blazek (additional kernel functions, network part)
* PURPOSE:      Generates a raster density map from vector points data using 
*               a moving kernel function or
*               optionally generates a vector density map on vector network 
*               with a 1D kernel
* COPYRIGHT:    (C) 2004-2011 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*   	    	License (>=v2). Read the file COPYING that comes with GRASS
*   	    	for details.
*
*****************************************************************************/
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include <grass/gmath.h>
#include <grass/vector.h>
#include "global.h"


static int ndists;		/* number of distances in dists */
static double *dists;		/* array of all distances < dmax */
static int npoints;
int net = 0;
static double dimension = 2.;


/* define score function L(window size) */
double L(double smooth)
{
    int ii;
    double resL, n, term;

    n = npoints;
    resL = 0.;
    term = 1. / pow((2. * M_PI), dimension / 2.);

    for (ii = 0; ii < ndists; ii++) {
	/*    resL+= gaussianFunction(dists[ii]/smooth,2.,dimension) - 2. * gaussianKernel(dists[ii]/smooth,term); */
	resL +=
	    gaussianFunction(dists[ii] / smooth, 2.,
			     dimension) -
	    2. * gaussianFunction(dists[ii] / smooth, 1., dimension);
    }

    if (!net)
	resL *= 2.;

    resL = (1. / (pow(n, 2.) * pow(smooth, dimension))) *
           (resL + n * (gaussianFunction(0., 2., dimension) -
	   2. * gaussianFunction(0., 1., dimension))) + 
	   (2. / (n * pow(smooth, dimension))) *
	   gaussianFunction(0., 1., dimension);

    /* resL = (1./(pow(n,2.)*pow(smooth,dimension))) * (resL + n*( gaussianFunction(0.,2.,dimension) - 2. * gaussianKernel(0.,term)) ) + (2./(n*pow(smooth,dimension)))*gaussianKernel(0.,term);   */
    G_debug(3, "smooth = %e resL = %e", smooth, resL);
    G_message(_("\tScore Value=%f\tsmoothing parameter (standard deviation)=%f"),
	      resL, smooth);

    return (resL);
}


int main(int argc, char **argv)
{
    struct Option *in_opt, *net_opt, *out_opt;
    struct Option *radius_opt, *dsize_opt, *segmax_opt, *netmax_opt,
	*multip_opt, *node_opt, *kernel_opt;
    struct Flag *flag_o, *flag_q, *flag_normalize, *flag_multiply;
    char *desc;

    struct Map_info In, Net, Out;
    int overwrite;
    int fdout = -1, maskfd = -1;
    int node_method, kernel_function;
    int row, col;
    struct Cell_head window;
    double gaussian;
    double N, E;
    CELL *mask = NULL;
    DCELL *output_cell = NULL;
    double sigma, dmax, segmax, netmax, multip;

    double **coordinate;
    double sigmaOptimal;
    struct GModule *module;
    double dsize;
    double term = 0;

    double gausmax = 0;
    int notreachable = 0;

    /* Initialize the GIS calls */
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("kernel density"));
    module->label =
        _("Generates a raster density map from vector points map.");
    module->description = _("Density is computed using a moving kernel. "
                            "Optionally generates a vector density map on a vector network.");

    in_opt = G_define_standard_option(G_OPT_V_INPUT);
    in_opt->label = _("Name of input vector map with training points");
    in_opt->description = NULL;
    
    net_opt = G_define_standard_option(G_OPT_V_INPUT);
    net_opt->key = "net";
    net_opt->label = _("Name of input network vector map");
    net_opt->description = NULL;
    net_opt->required = NO;
    net_opt->guisection = _("Network");

    out_opt = G_define_option();
    out_opt->key = "output";
    out_opt->type = TYPE_STRING;
    out_opt->key_desc = "name";
    out_opt->required = YES;
    out_opt->label = _("Name for output raster/vector map");
    out_opt->description = _("Outputs vector map if network map is given, otherwise raster map");

    radius_opt = G_define_option();
    radius_opt->key = "radius";
    radius_opt->type = TYPE_DOUBLE;
    radius_opt->required = YES;
    radius_opt->description = _("Kernel radius in map units");

    dsize_opt = G_define_option();
    dsize_opt->key = "dsize";
    dsize_opt->type = TYPE_DOUBLE;
    dsize_opt->required = NO;
    dsize_opt->description = _("Discretization error in map units");
    dsize_opt->answer = "0.";

    segmax_opt = G_define_option();
    segmax_opt->key = "segmax";
    segmax_opt->type = TYPE_DOUBLE;
    segmax_opt->required = NO;
    segmax_opt->description = _("Maximum length of segment on network");
    segmax_opt->answer = "100.";
    segmax_opt->guisection = _("Network");

    netmax_opt = G_define_option();
    netmax_opt->key = "distmax";
    netmax_opt->type = TYPE_DOUBLE;
    netmax_opt->required = NO;
    netmax_opt->description = _("Maximum distance from point to network");
    netmax_opt->answer = "100.";
    netmax_opt->guisection = _("Network");

    multip_opt = G_define_option();
    multip_opt->key = "mult";
    multip_opt->type = TYPE_DOUBLE;
    multip_opt->required = NO;
    multip_opt->description = _("Multiply the density result by this number");
    multip_opt->answer = "1.";

    node_opt = G_define_option();
    node_opt->key = "node";
    node_opt->type = TYPE_STRING;
    node_opt->required = NO;
    node_opt->description = _("Node method");
    node_opt->options = "none,split";
    node_opt->answer = "none";
    desc = NULL;
    G_asprintf(&desc,
	       "none;%s;split;%s",
	       _("No method applied at nodes with more than 2 arcs"),
	       _("Equal split (Okabe 2009) applied at nodes"));
    node_opt->descriptions = desc;
    node_opt->guisection = _("Network");

    kernel_opt = G_define_option();
    kernel_opt->key = "kernel";
    kernel_opt->type = TYPE_STRING;
    kernel_opt->required = NO;
    kernel_opt->description = _("Kernel function");
    kernel_opt->options =
	"uniform,triangular,epanechnikov,quartic,triweight,gaussian,cosine";
    kernel_opt->answer = "gaussian";

    flag_o = G_define_flag();
    flag_o->key = 'o';
    flag_o->description =
	_("Try to calculate an optimal standard deviation with 'stddeviation' taken as maximum (experimental)");

    flag_q = G_define_flag();
    flag_q->key = 'q';
    flag_q->description =
	_("Only calculate optimal standard deviation and exit (no map is written)");

    flag_normalize = G_define_flag();
    flag_normalize->key = 'n';
    flag_normalize->description =
	_("In network mode, normalize values by sum of density multiplied by length of each segment. Integral over the output map then gives 1.0 * mult");
    flag_normalize->guisection = _("Network");

    flag_multiply = G_define_flag();
    flag_multiply->key = 'm';
    flag_multiply->description =
	_("In network mode, multiply the result by number of input points");
    flag_multiply->guisection = _("Network");
    
    overwrite = G_check_overwrite(argc, argv);
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    if (net_opt->answer) {
	if (G_find_vector2(out_opt->answer, G_mapset()))
            if (overwrite)
                G_warning(_("Vector map <%s> already exists and will be overwritten"),
                          out_opt->answer);
            else
                G_fatal_error(_("Vector map <%s> already exists"),
                              out_opt->answer);
    }
    else {
	if (G_find_raster(out_opt->answer, G_mapset()))
            if (overwrite)
                G_warning(_("Raster map <%s> already exists and will be overwritten"),
                          out_opt->answer);
            else
                G_fatal_error(_("Raster map <%s> already exists"),
                              out_opt->answer);
    }

    /*read options */
    dmax = atof(radius_opt->answer);
    sigma = dmax;
    dsize = atof(dsize_opt->answer);
    segmax = atof(segmax_opt->answer);
    netmax = atof(netmax_opt->answer);
    multip = atof(multip_opt->answer);

    if (strcmp(node_opt->answer, "none") == 0)
	node_method = NODE_NONE;
    else if (strcmp(node_opt->answer, "split") == 0)
	node_method = NODE_EQUAL_SPLIT;
    else
	G_fatal_error(_("Unknown node method"));

    kernel_function = KERNEL_GAUSSIAN;
    if (strcmp(kernel_opt->answer, "uniform") == 0)
	kernel_function = KERNEL_UNIFORM;
    else if (strcmp(kernel_opt->answer, "triangular") == 0)
	kernel_function = KERNEL_TRIANGULAR;
    else if (strcmp(kernel_opt->answer, "epanechnikov") == 0)
	kernel_function = KERNEL_EPANECHNIKOV;
    else if (strcmp(kernel_opt->answer, "quartic") == 0)
	kernel_function = KERNEL_QUARTIC;
    else if (strcmp(kernel_opt->answer, "triweight") == 0)
	kernel_function = KERNEL_TRIWEIGHT;
    else if (strcmp(kernel_opt->answer, "gaussian") == 0)
	kernel_function = KERNEL_GAUSSIAN;
    else if (strcmp(kernel_opt->answer, "cosine") == 0)
	kernel_function = KERNEL_COSINE;
    else
	G_fatal_error(_("Unknown kernel function"));

    if (flag_o->answer) {
	if (net_opt->answer) {
	    if (node_method != NODE_NONE ||
		kernel_function != KERNEL_GAUSSIAN) {
		G_fatal_error(_("Optimal standard deviation calculation is supported only for node method 'none' and kernel function 'gaussian'."));
	    }
	}
	else if (kernel_function != KERNEL_GAUSSIAN) {
	    G_fatal_error(_("Optimal standard deviation calculation is supported only for kernel function 'gaussian'."));
	}
    }

    if (flag_q->answer) {
	flag_o->answer = 1;
    }

    if (net_opt->answer) {
	Vect_check_input_output_name(in_opt->answer, out_opt->answer,
				     G_FATAL_EXIT);
	Vect_check_input_output_name(net_opt->answer, out_opt->answer,
				     G_FATAL_EXIT);
    }

    G_get_window(&window);

    G_verbose_message(_("Standard deviation: %f"), sigma);
    G_verbose_message(_("Output raster map: res: %f\trows: %d\tcols: %d"),
                      window.ew_res, window.rows, window.cols);
    
    /* Open input vector */
    Vect_set_open_level(2);
    Vect_open_old(&In, in_opt->answer, "");

    if (net_opt->answer) {
	int nlines, line;
	struct line_pnts *Points;

	Points = Vect_new_line_struct();
	net = 1;
	dimension = 1.;
	/* Open input network */
	Vect_set_open_level(2);
	Vect_open_old(&Net, net_opt->answer, "");
	Vect_net_build_graph(&Net, GV_LINES, 0, 0, NULL, NULL, NULL, 0, 0);

	if (!flag_q->answer) {
	    Vect_open_new(&Out, out_opt->answer, 0);
	    Vect_hist_command(&Out);
	}

	/* verify not reachable points */
	nlines = Vect_get_num_lines(&In);
	for (line = 1; line <= nlines; line++) {
	    int ltype;

	    ltype = Vect_read_line(&In, Points, NULL, line);
	    if (!(ltype & GV_POINTS))
		continue;
	    if (Vect_find_line
		(&Net, Points->x[0], Points->y[0], 0.0, GV_LINES, netmax, 0,
		 0) == 0)
		notreachable++;
	}

	if (notreachable > 0)
	    G_warning(_("%d points outside threshold"), notreachable);
    }
    else {
	/* check and open the name of output map */
	if (!flag_q->answer) {
	    fdout = Rast_open_new(out_opt->answer, DCELL_TYPE);

	    /* open mask file */
	    if ((maskfd = Rast_maskfd()) >= 0)
		mask = Rast_allocate_c_buf();
	    else
		mask = NULL;

	    /* allocate output raster */
	    output_cell = Rast_allocate_buf(DCELL_TYPE);
	}
    }

    /* valutazione distanza ottimale */
    if (flag_o->answer) {
	/* Note: sigmaOptimal calculates using ALL points (also those outside the region) */
	G_message(_("Automatic choice of smoothing parameter (standard deviation), maximum possible "
		   "value of standard deviation is set to %f"), sigma);

	/* maximum distance 4*sigma (3.9*sigma ~ 1.0000), keep it small, otherwise it takes 
	 * too much points and calculation on network becomes slow */
	dmax = 4 * sigma;	/* used as maximum value */

	G_message(_("Using maximum distance between points: %f"), dmax);

	if (net_opt->answer) {
	    npoints = Vect_get_num_primitives(&In, GV_POINTS);
	    /* Warning: each distance is registered twice (both directions) */
	    ndists =
		compute_all_net_distances(&In, &Net, netmax, &dists, dmax);
	}
	else {
	    /* Read points */
	    npoints = read_points(&In, &coordinate, dsize);
	    ndists = compute_all_distances(coordinate, &dists, npoints, dmax);
	}

	G_message(_("Number of input points: %d."), npoints);
	G_message(_("%d distances read from the map."), ndists);

	if (ndists == 0)
	    G_fatal_error(_("Distances between all points are beyond %e (4 * "
			    "standard deviation), unable to calculate optimal value."),
			  dmax);

	/*  double iii;
	   for ( iii = 1.; iii <= 10000; iii++){
	   fprintf(stderr,"i=%f v=%.16f \n",iii,R(iii));
	   } */

	/* sigma is used in brent as maximum possible value for sigmaOptimal */
	sigmaOptimal = brent_iterate(L, 0.0, sigma, 1000);
	G_message(_("Optimal smoothing parameter (standard deviation): %f."),
		  sigmaOptimal);

	/* Reset sigma to calculated optimal value */
	sigma = sigmaOptimal;

	if (flag_q->answer) {
	    Vect_close(&In);
	    if (net_opt->answer)
		Vect_close(&Net);

	    exit(EXIT_SUCCESS);
	}
    }

    if (kernel_function == KERNEL_GAUSSIAN)
	sigma /= 4.;

    if (net_opt->answer) {
	setKernelFunction(kernel_function, 1, sigma, &term);
    }
    else {
	setKernelFunction(kernel_function, 2, sigma, &term);
    }

    if (net) {
	int line, nlines;
	struct line_pnts *Points, *SPoints;
	struct line_cats *SCats;
	double total = 0.0;

	G_verbose_message(_("Writing output vector map using smooth parameter %f"),
                          sigma);
	G_verbose_message(_("Normalising factor %f"),
                          1. / gaussianFunction(sigma / 4., sigma, dimension));

	/* Divide lines to segments and calculate gaussian for center of each segment */
	Points = Vect_new_line_struct();
	SPoints = Vect_new_line_struct();
	SCats = Vect_new_cats_struct();

	nlines = Vect_get_num_lines(&Net);
	G_debug(3, "net nlines = %d", nlines);

	for (line = 1; line <= nlines; line++) {
	    int seg, nseg, ltype;
	    double llength, length, x, y;

            G_percent(line, nlines, 5);
	    ltype = Vect_read_line(&Net, Points, NULL, line);
	    if (!(ltype & GV_LINES))
		continue;

	    llength = Vect_line_length(Points);
	    nseg = (int)(1 + llength / segmax);
	    length = llength / nseg;

	    G_debug(3, "net line = %d, nseg = %d, seg length = %f", line,
		    nseg, length);

	    for (seg = 0; seg < nseg; seg++) {
		double offset1, offset2;

		offset1 = (seg + 0.5) * length;
		Vect_point_on_line(Points, offset1, &x, &y, NULL, NULL, NULL);

		G_debug(3, "  segment = %d, offset = %f, xy = %f %f", seg,
			offset1, x, y);

		compute_net_distance(x, y, &In, &Net, netmax, sigma, term,
				     &gaussian, dmax, node_method);
		gaussian *= multip;
		if (gaussian > gausmax)
		    gausmax = gaussian;

		G_debug(3, "  gaussian = %f", gaussian);

		/* Write segment */
		if (gaussian > 0) {
		    offset1 = seg * length;
		    offset2 = (seg + 1) * length;
		    if (offset2 > llength)
			offset2 = llength;
		    Vect_line_segment(Points, offset1, offset2, SPoints);

		    /* TODO!!! remove later
		       if ( SPoints->n_points > 0 ) 
		       Vect_append_point( SPoints, SPoints->x[SPoints->n_points-1], 
		       SPoints->y[SPoints->n_points-1], 0 );
		     */
		    Vect_reset_cats(SCats);
		    Vect_cat_set(SCats, 1, (int)gaussian);

		    Vect_write_line(&Out, GV_LINE, SPoints, SCats);

		    total += length * gaussian;
		}
	    }
	}

	if (flag_normalize->answer || flag_multiply->answer) {
	    double m = multip;

	    if (flag_normalize->answer) {
		m /= total;
	    }
	    if (flag_multiply->answer) {
		m *= (Vect_get_num_primitives(&In, GV_POINT) - notreachable);
	    }

	    Vect_build(&Out);

	    gausmax = 0.0;
	    nlines = Vect_get_num_lines(&Out);
	    for (line = 1; line <= nlines; line++) {
		int cat;
		double gaussian;

		Vect_read_line(&Out, SPoints, SCats, line);

		Vect_cat_get(SCats, 1, &cat);
		gaussian = m * cat;
		Vect_reset_cats(SCats);
		Vect_cat_set(SCats, 1, (int)gaussian);
		Vect_rewrite_line(&Out, line, GV_LINE, SPoints, SCats);
		if (gaussian > gausmax)
		    gausmax = gaussian;
	    }
	    Vect_build_partial(&Out, GV_BUILD_NONE);	/* to force rebuild */
	}

	Vect_close(&Net);

	Vect_build(&Out);
	Vect_close(&Out);
    }
    else {
	/* spatial index handling, borrowed from lib/vector/Vlib/find.c */
	struct bound_box box;
	struct boxlist *NList = Vect_new_boxlist(1);

	G_verbose_message(_("Writing output raster map using smooth parameter %f"),
                          sigma);
	G_verbose_message(_("Normalising factor %f"),
                          1. / gaussianFunction(sigma / 4., sigma, dimension));

	for (row = 0; row < window.rows; row++) {
	    G_percent(row, window.rows, 2);
	    if (mask)
		Rast_get_c_row(maskfd, mask, row);

	    for (col = 0; col < window.cols; col++) {
		/* don't interpolate outside of the mask */
		if (mask && mask[col] == 0) {
		    Rast_set_d_null_value(&output_cell[col], 1);
		    continue;
		}

		N = Rast_row_to_northing(row + 0.5, &window);
		E = Rast_col_to_easting(col + 0.5, &window);

		if ((col & 31) == 0) {

		    /* create bounding box 32x2*dmax size from the current cell center */
		    box.N = N + dmax;
		    box.S = N - dmax;
		    box.E = E + dmax + 32 * window.ew_res;
		    box.W = E - dmax;
		    box.T = HUGE_VAL;
		    box.B = -HUGE_VAL;

		    Vect_select_lines_by_box(&In, &box, GV_POINT, NList);		    
		}
		box.N = N + dmax;
		box.S = N - dmax;
		box.E = E + dmax;
		box.W = E - dmax;
		box.T = HUGE_VAL;
		box.B = -HUGE_VAL;
		/* compute_distance(N, E, &In, sigma, term, &gaussian, dmax); */
		compute_distance(N, E, sigma, term, &gaussian, dmax, &box, NList);

		output_cell[col] = multip * gaussian;
		if (gaussian > gausmax)
		    gausmax = gaussian;
	    }
	    Rast_put_row(fdout, output_cell, DCELL_TYPE);
	}
        G_percent(1, 1, 1);
        
	Rast_close(fdout);
    }

    G_done_msg(_("Maximum value in output: %e."), multip * gausmax);

    Vect_close(&In);

    exit(EXIT_SUCCESS);
}


/* Read points to array return number of points */
int read_points(struct Map_info *In, double ***coordinate, double dsize)
{
    int line, nlines, npoints, ltype, i = 0;
    double **xySites;
    static struct line_pnts *Points = NULL;

    if (!Points)
	Points = Vect_new_line_struct();

    /* Allocate array of pointers */
    npoints = Vect_get_num_primitives(In, GV_POINT);
    xySites = (double **)G_calloc(npoints, sizeof(double *));

    nlines = Vect_get_num_lines(In);

    for (line = 1; line <= nlines; line++) {
	ltype = Vect_read_line(In, Points, NULL, line);
	if (!(ltype & GV_POINT))
	    continue;

	xySites[i] = (double *)G_calloc((size_t) 2, sizeof(double));

	xySites[i][0] = Points->x[0];
	xySites[i][1] = Points->y[0];
	i++;
    }

    *coordinate = xySites;

    return (npoints);
}


/* Calculate distances < dmax between all sites in coordinate 
 * Return: number of distances in dists */
double compute_all_distances(double **coordinate, double **dists, int n,
			     double dmax)
{
    int ii, jj, kk;
    size_t nn;

    nn = n * (n - 1) / 2;
    *dists = (double *)G_calloc(nn, sizeof(double));
    kk = 0;

    for (ii = 0; ii < n - 1; ii++) {
	for (jj = ii + 1; jj < n; jj++) {
	    double dist;

	    dist = euclidean_distance(coordinate[ii], coordinate[jj], 2);
	    G_debug(3, "dist = %f", dist);

	    if (dist <= dmax) {
		(*dists)[kk] = dist;
		kk++;
	    }
	}
    }

    return (kk);
}


/* Calculate distances < dmax between all sites in coordinate 
 * Return: number of distances in dists */
double compute_all_net_distances(struct Map_info *In, struct Map_info *Net,
				 double netmax, double **dists, double dmax)
{
    int nn, kk, nalines, aline;
    double dist;
    struct line_pnts *APoints, *BPoints;
    struct bound_box box;
    struct boxlist *List;

    APoints = Vect_new_line_struct();
    BPoints = Vect_new_line_struct();
    List = Vect_new_boxlist(0);

    nn = Vect_get_num_primitives(In, GV_POINTS);
    nn = nn * (nn - 1);
    *dists = (double *)G_calloc(nn, sizeof(double));
    kk = 0;

    nalines = Vect_get_num_lines(In);
    for (aline = 1; aline <= nalines; aline++) {
	int i, altype;

	G_debug(3, "  aline = %d", aline);

	altype = Vect_read_line(In, APoints, NULL, aline);
	if (!(altype & GV_POINTS))
	    continue;

	box.E = APoints->x[0] + dmax;
	box.W = APoints->x[0] - dmax;
	box.N = APoints->y[0] + dmax;
	box.S = APoints->y[0] - dmax;
	box.T = PORT_DOUBLE_MAX;
	box.B = -PORT_DOUBLE_MAX;

	Vect_select_lines_by_box(In, &box, GV_POINT, List);
	G_debug(3, "  %d points in box", List->n_values);

	for (i = 0; i < List->n_values; i++) {
	    int bline, ret;

	    bline = List->id[i];

	    if (bline == aline)
		continue;

	    G_debug(3, "    bline = %d", bline);
	    Vect_read_line(In, BPoints, NULL, bline);

	    ret =
		Vect_net_shortest_path_coor(Net, APoints->x[0], APoints->y[0],
					    0.0, BPoints->x[0], BPoints->y[0],
					    0.0, netmax, netmax, &dist, NULL,
					    NULL, NULL, NULL, NULL, NULL);

	    G_debug(3, "  SP: %f %f -> %f %f", APoints->x[0], APoints->y[0],
		    BPoints->x[0], BPoints->y[0]);

	    if (ret == 0) {
		G_debug(3, "not reachable");
		continue;	/* Not reachable */
	    }

	    G_debug(3, "  dist = %f", dist);

	    if (dist <= dmax) {
		(*dists)[kk] = dist;
		kk++;
	    }
	    G_debug(3, "  kk = %d", kk);
	}
    }

    return (kk);
}

/* get number of arcs for a node */
int count_node_arcs(struct Map_info *Map, int node)
{
    int i, n, line, type;
    int count = 0;

    n = Vect_get_node_n_lines(Map, node);
    for (i = 0; i < n; i++) {
	line = Vect_get_node_line(Map, node, i);
	type = Vect_get_line_type(Map, abs(line));
	if (type & GV_LINES)
	    count++;
    }
    return count;
}

/* Compute gausian for x, y along Net, using all points in In */
void compute_net_distance(double x, double y, struct Map_info *In,
			  struct Map_info *Net, double netmax, double sigma,
			  double term, double *gaussian, double dmax, int node_method)
{
    int i;
    double dist, kernel;
    static struct line_pnts *FPoints = NULL;
    struct bound_box box;
    static struct boxlist *PointsList = NULL;
    static struct ilist *NodesList = NULL;

    if (!PointsList)
	PointsList = Vect_new_boxlist(1);

    if (node_method == NODE_EQUAL_SPLIT) {
	if (!NodesList)
	    NodesList = Vect_new_list();

	if (!FPoints)
	    FPoints = Vect_new_line_struct();
    }

    *gaussian = .0;

    /* The network is usually much bigger than dmax and to calculate shortest path is slow
     * -> use spatial index to select points
     * enlarge the box by netmax (max permitted distance between a point and net) */
    box.E = x + dmax + netmax;
    box.W = x - dmax - netmax;
    box.N = y + dmax + netmax;
    box.S = y - dmax - netmax;
    box.T = PORT_DOUBLE_MAX;
    box.B = -PORT_DOUBLE_MAX;

    Vect_select_lines_by_box(In, &box, GV_POINT, PointsList);
    G_debug(3, "  %d points in box", PointsList->n_values);

    for (i = 0; i < PointsList->n_values; i++) {
	int line, ret;

	line = PointsList->id[i];

	G_debug(3, "  SP: %f %f -> %f %f", x, y, PointsList->box[i].E, PointsList->box[i].N);
	/*ret = Vect_net_shortest_path_coor(Net, x, y, 0.0, Points->x[0], */
	/*Points->y[0], 0.0, netmax, netmax, */
	/*&dist, NULL, NULL, NULL, NULL, NULL, */
	/*NULL); */
	ret = Vect_net_shortest_path_coor2(Net,
					   PointsList->box[i].E,
					   PointsList->box[i].N, 0.0,
					   x, y, 0.0, netmax, 1.0,
					   &dist, NULL,
					   NULL, NodesList, FPoints, NULL,
					   NULL, NULL);

	if (ret == 0) {
	    G_debug(3, "not reachable");
	    continue;		/* Not reachable */
	}

	/* if (dist <= dmax)
	 *gaussian += gaussianKernel(dist / sigma, term); */

	if (dist > dmax)
	    continue;

	/* kernel = gaussianKernel(dist / sigma, term); */
	kernel = kernelFunction(term, sigma, dist);

	if (node_method == NODE_EQUAL_SPLIT) {
	    int j, node;
	    double ndiv = 1.;
	    int start = 0;

	    /* Count the nodes and arcs on path (n1-1)*(n2-1)* ... (ns-1) */

	    for (j = start; j < NodesList->n_values; j++) {
		node = NodesList->value[j];

		/* Divide into 2/n if point falls on a node */
		if (j == 0 && FPoints->n_points < 3) {
		    ndiv *= count_node_arcs(Net, node) / 2.;
		}
		else {
		    ndiv *= count_node_arcs(Net, node) - 1;
		}
	    }
	    kernel /= ndiv;
	}
	*gaussian += kernel;
	G_debug(3, "  dist = %f gaussian = %f", dist, *gaussian);
    }
}

void compute_distance(double N, double E, double sigma, double term,
                      double *gaussian, double dmax, struct bound_box *box,
		      struct boxlist *NList)
{
    int line, nlines;
    double a[2], b[2];
    double dist;

    a[0] = E;
    a[1] = N;

    /* number of lines within dmax box  */
    nlines = NList->n_values;

    *gaussian = .0;

    for (line = 0; line < nlines; line++) {

	b[0] = NList->box[line].E;
	b[1] = NList->box[line].N;

	if (b[0] <= box->E && b[0] >= box->W &&
	    b[1] <= box->N && b[1] >= box->S)  {
	    dist = euclidean_distance(a, b, 2);

	    if (dist <= dmax)
		/* *gaussian += gaussianKernel(dist / sigma, term); */
		*gaussian += kernelFunction(term, sigma, dist);
	}
    }
}

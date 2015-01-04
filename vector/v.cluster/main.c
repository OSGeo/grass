
/****************************************************************
 *
 * MODULE:     v.cluster
 *
 * AUTHOR(S):  Markus Metz
 *
 * PURPOSE:    Identifies clusters in a point cloud
 *
 * COPYRIGHT:  (C) 2015 by the GRASS Development Team
 *
 *             This program is free software under the
 *             GNU General Public License (>=v2).
 *             Read the file COPYING that comes with GRASS
 *             for details.
 *
 ****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>
#include <grass/kdtree.h>

int main(int argc, char *argv[])
{
    struct Map_info In, Out;
    static struct line_pnts *Points;
    struct line_cats *Cats;
    int i, j, type, cat, is3d;
    struct GModule *module;
    struct Option *input, *output, *dist_opt, *min_opt, *lyr_opt;
    struct Flag *flag_2d, *flag_topo, *flag_attr;
    int clayer;
    int npoints, nlines;
    int *cid, *idx, *renumber, OLD, NEW;
    int nclusters;
    struct kdtree *kdt;
    struct kdtrav trav;
    double c[3];
    int uid;
    double eps;
    int ndims, minpnts;
    double *kddist;
    int kdfound, *kduid;

    /* initialize GIS environment */
    /* reads grass env, stores program name to G_program_name() */
    G_gisinit(argv[0]);

    /* initialize module */
    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("point cloud"));
    G_add_keyword(_("cluster"));
    G_add_keyword(_("clump"));
    module->description = _("Cluster identification");

    /* Define the different options as defined in gis.h */
    input = G_define_standard_option(G_OPT_V_INPUT);

    output = G_define_standard_option(G_OPT_V_OUTPUT);

    lyr_opt = G_define_standard_option(G_OPT_V_FIELD);
    lyr_opt->label = _("Layer number or name for cluster ids");
    lyr_opt->answer = "2";

    dist_opt = G_define_option();
    dist_opt->type = TYPE_DOUBLE;
    dist_opt->key = "distance";
    dist_opt->required = NO;
    dist_opt->label = _("Maximum distance to neighbors");

    min_opt = G_define_option();
    min_opt->type = TYPE_INTEGER;
    min_opt->key = "min";
    min_opt->required = NO;
    min_opt->label = _("Minimum number of points to create a cluster");

    flag_2d = G_define_flag();
    flag_2d->key = '2';
    flag_2d->label = _("Force 2D clustering");

    flag_topo = G_define_standard_flag(G_FLG_V_TOPO);

    flag_attr = G_define_standard_flag(G_FLG_V_TABLE);
    

    /* options and flags parser */
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    Vect_check_input_output_name(input->answer, output->answer, G_FATAL_EXIT);

    if (Vect_set_open_level(1))
	G_fatal_error(_("Unable to set predetermined vector open level"));

    if (1 > Vect_open_old(&In, input->answer, NULL))
	G_fatal_error(_("Unable to open vector map <%s>"), input->answer);

    /* Check if old vector is 3D. We should preserve 3D data. */
    is3d = WITHOUT_Z;
    ndims = 2;
    if (Vect_is_3d(&In)) {
	is3d = WITH_Z;
	ndims = 3;
    }

    if (flag_2d->answer)
	ndims = 2;

    minpnts = ndims;

    if (min_opt->answer) {
	minpnts = atoi(min_opt->answer);
	if (minpnts < 2) {
	    G_warning(_("Minimum number of points must be at least 2"));
	    minpnts = 2;
	}
	minpnts--;
    }

    clayer = atoi(lyr_opt->answer);

    /* count points */
    G_message(_("Counting input points ..."));
    npoints = nlines = 0;
    while ((type = Vect_read_next_line(&In, Points, Cats)) > 0) {
	nlines++;
	if (type == GV_POINT)
	    npoints++;
    }

    if (npoints == 0) {
	G_warning(_("No points in input, nothing to do"));
	Vect_close(&In);
	exit(EXIT_SUCCESS);
    }

    /* Open new vector for reading/writing */
    if (0 > Vect_open_new(&Out, output->answer, is3d)) {
	Vect_close(&In);
	G_fatal_error(_("Unable to create vector map <%s>"), output->answer);
    }

    /* Copy header and history data from old to new map */
    Vect_copy_head_data(&In, &Out);
    Vect_hist_copy(&In, &Out);
    Vect_hist_command(&Out);

    /* create k-d tree */
    G_message(_("Creating search index ..."));
    kdt = kdtree_create(ndims, NULL);
    cid = G_malloc((nlines + 1) * sizeof(int));
    idx = G_malloc((nlines + 1) * sizeof(int));
    Vect_rewind(&In);
    i = 0;
    cid[0] = 0;
    while ((type = Vect_read_next_line(&In, Points, Cats)) > 0) {
	G_percent(i++, nlines, 4);
	cid[i] = 0;
	if (type == GV_POINT) {
	    
	    c[0] = Points->x[0];
	    c[1] = Points->y[0];
	    c[2] = Points->z[0];

	    kdtree_insert(kdt, c, i, 0);
	}
    }
    G_percent(nlines, nlines, 4);

    kdtree_optimize(kdt, 2);

    /* get epsilon */
    if (dist_opt->answer) {
	eps = atof(dist_opt->answer);
	if (eps <= 0)
	    G_fatal_error(_("Option %s must be a positive number"), dist_opt->key);
    }
    else {
	int n;
	double dist, mean, min, max, sum, sumsq, sd;
	double *kd;
	int *ki;

	/* estimate epsilon */
	G_message(_("Estimating maximum distance ..."));
	kdtree_init_trav(&trav, kdt);
	c[2] = 0.0;
	n = 0;
	sum = sumsq = 0;
	min = 1.0 / 0.0;
	max = 0;
	kd = G_malloc(minpnts * sizeof(double));
	ki = G_malloc(minpnts * sizeof(int));
	i = 0;
	while (kdtree_traverse(&trav, c, &uid)) {
	    G_percent(i++, npoints, 4);

	    kdfound = kdtree_knn(kdt, c, ki, kd, minpnts, &uid);
	    if (kdfound) {
		dist = sqrt(kd[kdfound - 1]);
		sum += dist;
		sumsq += dist * dist;
		n++;
		if (min > dist)
		    min = dist;
		if (max < dist)
		    max = dist;
	    }
	}
	G_percent(npoints, npoints, 4);

	G_free(kd);
	G_free(ki);

	if (!n)
	    G_fatal_error(_("No neighbors found"));
	
	mean = sum / n;
	sd = sqrt(sumsq / n - mean * mean);
	eps = mean + 1.644854 * sd; /* 90% CI */
	eps = mean + 2.575829 * sd; /* 99% CI */
	
	if (eps > max)
	    eps = max;

	G_message(_("Distance to the %d nearest neighbor:"), minpnts);
	G_message(_("Min: %g, max: %g"), min, max);
	G_message(_("Mean: %g"), mean);
	G_message(_("Standard deviation: %g"), sd);

	G_message(_("Estimated maximum distance: %g"), eps);
    }

    /* create clusters */
    G_message(_("Building clusters ..."));
    nclusters = 0;
    kdtree_init_trav(&trav, kdt);
    c[2] = 0.0;
    idx[0] = 0;
    i = 0;
    while (kdtree_traverse(&trav, c, &uid)) {
	G_percent(i++, npoints, 4);

	/* radius search */
	kdfound = kdtree_dnn(kdt, c, &kduid, &kddist, eps, &uid);
	
	/* must have min neighbors within radius */
	if (kdfound >= minpnts) {

	    cat = idx[cid[uid]];

	    /* find latest cluster */
	    for (j = 0; j < kdfound; j++) {
		if (idx[cid[kduid[j]]] > cat) {
		    cat = idx[cid[kduid[j]]];
		}
	    }

	    if (cat == 0) {
		/* start new cluster */
		nclusters++;
		cat = nclusters;
		if (nclusters > nlines)
		    G_fatal_error(_("nlines: %d, nclusters: %d"), nlines, nclusters);
		idx[nclusters] = nclusters;
		cid[uid] = nclusters;
	    }

	    /* set or update cluster ids */
	    if (cid[uid] != 0) {
		/* relabel */
		idx[cid[uid]] = cat;
	    }
	    else {
		cid[uid] = cat;
	    }

	    for (j = 0; j < kdfound; j++) {
		if (cid[kduid[j]] != 0) {
		    /* relabel */
		    idx[cid[kduid[j]]] = cat;
		}
		else {
		   cid[kduid[j]] = cat;
		}
	    }
	}
	if (kdfound) {
	    G_free(kddist);
	    G_free(kduid);
	}
    }
    G_percent(npoints, npoints, 4);

    if (nclusters == 0) {
	G_message(_("No clusters found, adjust option %s"), dist_opt->key);
	Vect_close(&In);
	Vect_close(&Out);
	Vect_delete(output->answer);
	exit(EXIT_SUCCESS);
    }

    /* generate a renumbering scheme */
    G_message(_("Generating renumbering scheme..."));
    G_debug(1, "%d initial clusters", nclusters);
    /* allocate final clump ID */
    renumber = (int *) G_malloc((nclusters + 1) * sizeof(int));
    renumber[0] = 0;
    cat = 1;
    G_percent(0, nclusters, 1);
    for (i = 1; i <= nclusters; i++) {
	G_percent(i, nclusters, 4);
	OLD = i;
	NEW = idx[i];
	if (OLD != NEW) {
	    renumber[i] = 0;
	    /* find valid clump ID */
	    while (OLD != NEW) {
		OLD = NEW;
		NEW = idx[OLD];
	    }
	    idx[i] = NEW;
	}
	else
	    /* set final clump id */
	    renumber[i] = cat++;
    }

    nclusters = cat - 1;

    /* write cluster ids */
    G_message(_("Write out cluster ids ..."));
    Vect_rewind(&In);
    i = 0;
    while ((type = Vect_read_next_line(&In, Points, Cats)) > 0) {
	G_percent(i++, nlines, 4);
	if (type == GV_POINT) {
	    cat = renumber[idx[cid[i]]];
	    Vect_cat_set(Cats, clayer, cat);
	    Vect_write_line(&Out, GV_POINT, Points, Cats);
	}
    }
    G_percent(nlines, nlines, 4);

    if (!flag_attr->answer)
	Vect_copy_tables(&In, &Out, 0);

    /* Build topology for vector map and close them */
    Vect_close(&In);
    if (!flag_topo->answer)
	Vect_build(&Out);
    Vect_close(&Out);

    G_message(_("%d clusters found"), nclusters);

    exit(EXIT_SUCCESS);
}

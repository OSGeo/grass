
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
#include <string.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>
#include <grass/kdtree.h>

#ifdef MAX
#undef MAX
#endif
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define CL_DBSCAN	1
#define CL_DBSCAN2	2
#define CL_DENSE	3
#define CL_OPTICS	4
#define CL_OPTICS2	5

#define GET_PARENT(p, c) ((p) = (int) (((c) - 2) / 3 + 1))
#define GET_CHILD(c, p) ((c) = (int) (((p) * 3) - 1))


struct cl_pnt
{
    int uid;
    int prevpnt;
    double cd;
    double reach;
    double c[3];
};

static struct cl_pnt *clp;

static int *heapidx;
static int heapsize;

int add_pt(int idx);
int drop_pt(void);

int main(int argc, char *argv[])
{
    struct Map_info In, Out;
    struct line_pnts *Points;
    struct line_cats *Cats;
    int i, j, type, cat, is3d;
    struct GModule *module;
    struct Option *input, *output, *lyr_opt;
    struct Option *dist_opt, *min_opt, *method_opt;
    struct Flag *flag_2d, *flag_topo, *flag_attr;
    int clayer;
    int npoints, nlines;
    int *cid, *idx, *renumber, OLD, NEW;
    int nclusters, noutliers;
    struct kdtree *kdt;
    struct kdtrav trav;
    double c[3];
    int uid;
    double eps;
    int ndims, minpnts;
    int clmethod;
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
    G_add_keyword(_("level1"));

    module->description = _("Performs cluster identification.");

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

    method_opt = G_define_option();
    method_opt->type = TYPE_STRING;
    method_opt->key = "method";
    method_opt->options = "dbscan,dbscan2,density,optics,optics2";
    method_opt->answer = "dbscan";
    method_opt->required = NO;
    method_opt->label = _("Clustering method");

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
    if (clayer < 1)
	G_fatal_error(_("Option %s must be positive"), lyr_opt->key);

    clmethod = CL_DBSCAN;
    if (!strcmp(method_opt->answer, "dbscan2"))
	clmethod = CL_DBSCAN2;
    else if (!strcmp(method_opt->answer, "density"))
	clmethod = CL_DENSE;
    else if (!strcmp(method_opt->answer, "optics"))
	clmethod = CL_OPTICS;
    else if (!strcmp(method_opt->answer, "optic2"))
	clmethod = CL_OPTICS2;

    /* count points */
    G_message(_("Counting input points ..."));
    npoints = nlines = 0;
    while ((type = Vect_read_next_line(&In, Points, Cats)) > 0) {
	nlines++;
	if (type == GV_POINT) {
	    if (Vect_cat_get(Cats, clayer, &cat))
		G_fatal_error(_("Layer %d is not empty, choose another layer"),
		              clayer);
	    npoints++;
	}
    }

    if (npoints < minpnts + 1) {
	G_warning(_("Not enough points in input, nothing to do"));
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
    idx[0] = 0;
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

    noutliers = nclusters = 0;
    if (clmethod == CL_DBSCAN) {
	/* DBSCAN 
	 * the neighbors of each point 
	 * with at least minpnts neighbors within distance (epsilon)
	 * are added to a cluster */

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
	    /* TODO: use knn search */
	    kdfound = kdtree_dnn(kdt, c, &kduid, &kddist, eps, &uid);

	    /* must have min neighbors within radius */
	    if (kdfound >= minpnts) {

		OLD = cid[uid];
		NEW = idx[OLD];
		while (OLD != NEW) {
		    OLD = NEW;
		    NEW = idx[OLD];
		}
		cat = NEW;

		/* find latest cluster */
		for (j = 0; j < kdfound; j++) {
		    OLD = cid[kduid[j]];
		    NEW = idx[OLD];
		    while (OLD != NEW) {
			OLD = NEW;
			NEW = idx[OLD];
		    }
		    if (cat < NEW) {
			cat = NEW;
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
		    OLD = cid[uid];
		    NEW = idx[OLD];
		    while (OLD != NEW) {
			OLD = NEW;
			NEW = idx[OLD];
		    }
		    idx[NEW] = cat;
		}
		else {
		    cid[uid] = cat;
		}

		for (j = 0; j < kdfound; j++) {
		    if (cid[kduid[j]] != 0) {
			/* relabel */
			OLD = cid[kduid[j]];
			NEW = idx[OLD];
			while (OLD != NEW) {
			    OLD = NEW;
			    NEW = idx[OLD];
			}
			idx[NEW] = cat;
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
	noutliers = 0;
	while ((type = Vect_read_next_line(&In, Points, Cats)) > 0) {
	    G_percent(i++, nlines, 4);
	    if (type == GV_POINT) {
		cat = renumber[idx[cid[i]]];
		if (!cat)
		    noutliers++;
		Vect_cat_set(Cats, clayer, cat);
		Vect_write_line(&Out, GV_POINT, Points, Cats);
	    }
	}
	G_percent(nlines, nlines, 4);
    }
    else if (clmethod == CL_DBSCAN2) {
	/* DBSCAN, but cluster size must be at least minpnts + 1 */
	int *clcnt;

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
	clcnt = G_malloc((nlines + 1) * sizeof(int));
	for (i = 0; i <= nlines; i++)
	    clcnt[i] = 0;
	nclusters = 0;
	kdtree_init_trav(&trav, kdt);
	c[2] = 0.0;
	idx[0] = 0;
	i = 0;
	while (kdtree_traverse(&trav, c, &uid)) {
	    G_percent(i++, npoints, 4);

	    /* radius search */
	    /* TODO: use knn search */
	    kdfound = kdtree_dnn(kdt, c, &kduid, &kddist, eps, &uid);
	    
	    /* any neighbor within radius */
	    if (kdfound > 0) {

		OLD = cid[uid];
		NEW = idx[OLD];
		while (OLD != NEW) {
		    OLD = NEW;
		    NEW = idx[OLD];
		}
		cat = NEW;

		/* find latest cluster */
		for (j = 0; j < kdfound; j++) {
		    OLD = cid[kduid[j]];
		    NEW = idx[OLD];
		    while (OLD != NEW) {
			OLD = NEW;
			NEW = idx[OLD];
		    }
		    if (cat < NEW) {
			cat = NEW;
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
		    clcnt[cat] = 1;
		}

		/* set or update cluster ids */
		if (cid[uid] != 0) {
		    /* relabel */
		    OLD = cid[uid];
		    NEW = idx[OLD];
		    while (OLD != NEW) {
			OLD = NEW;
			NEW = idx[OLD];
		    }
		    idx[NEW] = cat;
		}
		else {
		    cid[uid] = cat;
		    clcnt[cat]++;
		}

		for (j = 0; j < kdfound; j++) {
		    if (cid[kduid[j]] != 0) {
			OLD = cid[kduid[j]];
			NEW = idx[OLD];
			while (OLD != NEW) {
			    OLD = NEW;
			    NEW = idx[OLD];
			}
			/* relabel */
			idx[NEW] = cat;
		    }
		    else {
		        cid[kduid[j]] = cat;
			clcnt[cat]++;
		    }
		}
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
		/* find valid clump ID */
		while (OLD != NEW) {
		    OLD = NEW;
		    NEW = idx[OLD];
		}
		idx[i] = NEW;
		clcnt[NEW] += clcnt[i];
	    }
	}
	for (i = 1; i <= nclusters; i++) {
	    OLD = i;
	    NEW = idx[i];
	    renumber[i] = 0;
	    if (OLD == NEW && clcnt[NEW] > minpnts) {
		/* set final clump id */
		renumber[i] = cat++;
	    }
	}

	nclusters = cat - 1;

	/* write cluster ids */
	G_message(_("Write out cluster ids ..."));
	Vect_rewind(&In);
	i = 0;
	noutliers = 0;
	while ((type = Vect_read_next_line(&In, Points, Cats)) > 0) {
	    G_percent(i++, nlines, 4);
	    if (type == GV_POINT) {
		cat = renumber[idx[cid[i]]];
		if (!cat)
		    noutliers++;
		Vect_cat_set(Cats, clayer, cat);
		Vect_write_line(&Out, GV_POINT, Points, Cats);
	    }
	}
	G_percent(nlines, nlines, 4);
    }
    else if (clmethod == CL_OPTICS) {
	/* OPTICS
	 * each pair of points is either directly connected or 
	 * connected by a chain of other points 
	 * for each unprocessed point p
	 * mark as processed, append to output list
	 * core distance of p: distance to the k-th neighbor
	 * q: neighbor of p
	 * reachability of q: max(dist(p, q), coredist(p))
	 * -> needs epsilon, otherwise always coredist(p)
	 * for each unprocessed neighbor q
	 * if q has not been reached yet, put q in min heap
	 * if q's reachability can be reduced, put q with new reachability in min heap
	 * proceed with point with smallest reachability
	 * clusters:
	 * plot x = position in output list, y = reachability
	 * clusters = valleys of reachability in plot
	 * hierarchical clusters: valleys in valleys
	 */

	double *kd;
	int *ki;
	int k, kdpnts;
	int *clidx;
	int *olist, nout;
	double newrd;
	int isout;

	kd = G_malloc(minpnts * sizeof(double));
	ki = G_malloc(minpnts * sizeof(int));

	clp = G_malloc((npoints + 1) * sizeof(struct cl_pnt));
	heapidx = G_malloc((npoints + 1) * sizeof(int));
	olist = G_malloc((npoints + 1) * sizeof(int));
	clidx = G_malloc((nlines + 1) * sizeof(int));
	
	heapsize = 0;

	/* get epsilon */
	eps = 0;
	if (dist_opt->answer) {
	    eps = atof(dist_opt->answer);
	    if (eps <= 0)
		G_fatal_error(_("Option %s must be a positive number"), dist_opt->key);
	}

	/* loading points */
	G_message(_("Loading points ..."));
	kdtree_init_trav(&trav, kdt);
	c[2] = 0.0;
	i = 0;
	while (kdtree_traverse(&trav, c, &uid)) {
	    G_percent(i, npoints, 4);
	    
	    clp[i].c[0] = c[0];
	    clp[i].c[1] = c[1];
	    clp[i].c[2] = c[2];
	    clp[i].uid = uid;
	    clp[i].cd = -1;
	    clp[i].reach = -1;
	    clp[i].prevpnt = -1;
	    clidx[uid] = i;
	    olist[i] = -1;
	    
	    i++;
	}
	G_percent(npoints, npoints, 4);
	kdpnts = i;
	G_debug(0, "%d points in k-d tree", kdpnts);

	/* reachability network */
	G_message(_("Reachability network ..."));
	nout = 0;
	for (i = 0; i < kdpnts; i++) {
	    G_percent(i, kdpnts, 4);
	    
	    if (clp[i].cd > 0)
		continue;

	    /* knn search */
	    uid = clp[i].uid;
	    kdfound = kdtree_knn(kdt, clp[i].c, ki, kd, minpnts, &uid);
	    if (kdfound < minpnts)
		G_fatal_error(_("Not enough points found"));

	    clp[i].cd = kd[minpnts - 1];
	    /* no reachability for the seed point !!! */
	    clp[i].reach = clp[i].cd; /* ok ? */
	    olist[nout++] = i;
	    
	    /* initialize heap */
	    newrd = clp[i].cd;
	    for (j = 0; j < kdfound; j++) {
		if (clp[clidx[ki[j]]].cd < 0) {
		    /* deviation from OPTICS, 
		     * creates nicer connectivity graph */
		    newrd = kd[j];
		    if (clp[clidx[ki[j]]].reach < 0 || clp[clidx[ki[j]]].reach > newrd) {
			clp[clidx[ki[j]]].reach = newrd;
			clp[clidx[ki[j]]].prevpnt = i;
			add_pt(clidx[ki[j]]);
		    }
		}
	    }

	    while (heapsize) {
		k = drop_pt();
		if (k < 0 || k >= kdpnts)
		    G_fatal_error("Invalid index");
		if (clp[k].cd > 0)
		    continue;

		/* knn search */
		uid = clp[k].uid;
		kdfound = kdtree_knn(kdt, clp[k].c, ki, kd, minpnts, &uid);
		if (kdfound < minpnts)
		    G_fatal_error(_("Not enough points found"));

		clp[k].cd = kd[minpnts - 1];
		olist[nout++] = k;

		newrd = clp[k].cd;
		for (j = 0; j < kdfound; j++) {
		    if (heapsize >= npoints)
			G_fatal_error("Heap is too large");
		    if (clp[clidx[ki[j]]].cd < 0) {
			/* deviation from OPTICS, 
			 * creates nicer connectivity graph */
			newrd = kd[j];
			if (clp[clidx[ki[j]]].reach < 0 || clp[clidx[ki[j]]].reach > newrd) {
			    clp[clidx[ki[j]]].reach = newrd;
			    clp[clidx[ki[j]]].prevpnt = k;
			    add_pt(clidx[ki[j]]);
			}
		    }
		}
	    }
	}
	G_percent(kdpnts, kdpnts, 4);
	G_debug(0, "nout: %d", nout);
	if (nout != kdpnts)
	    G_fatal_error("nout != kdpnts");

	/* set cluster ids */
	G_message(_("Set cluster ids ..."));
	isout = 1;
	nclusters = 0;
	for (i = 0; i < kdpnts; i++) {
	    G_percent(i, kdpnts, 4);

	    if (eps > 0 && clp[olist[i]].reach > eps)
		isout = 1;
	    else {
		if (isout || clp[olist[i]].prevpnt == -1) {
		    isout = 0;
		    nclusters++;
		}
		cid[clp[olist[i]].uid] = nclusters;
	    }
	}
	G_percent(kdpnts, kdpnts, 4);

	/* write cluster ids */
	G_message(_("Write out cluster ids ..."));
	Vect_rewind(&In);
	i = 0;
	noutliers = 0;
	while ((type = Vect_read_next_line(&In, Points, Cats)) > 0) {
	    G_percent(i++, nlines, 4);
	    if (type == GV_POINT) {
		cat = cid[i];
		if (!cat)
		    noutliers++;
		Vect_cat_set(Cats, clayer, cat);
		Vect_write_line(&Out, GV_POINT, Points, Cats);
	    }
	}
	G_percent(nlines, nlines, 4);
    }
    else if (clmethod == CL_OPTICS2) {
	/* OPTICS modified, create separated reachability networks
	 * for each point p
	 *   get p's core distance
	 *   get p's neighbors
	 *   for each neighbor q
	 *     new reachability of q: dist(p, q) 
	 *     if q has been processed
	 *       new reachability of q: max(coredist(q), dist(p, q))
	 *     set or reduce q's reachability 
	 *     connect q to p if q's reachability can be updated
	 */

	double *coredist;
	double *reachability;
	int *nextpnt;
	double *kd;
	int *ki;
	double newrd;

	coredist = G_malloc((nlines + 1) * sizeof(double));
	reachability = G_malloc((nlines + 1) * sizeof(double));
	nextpnt = G_malloc((nlines + 1) * sizeof(int));

	kd = G_malloc(minpnts * sizeof(double));
	ki = G_malloc(minpnts * sizeof(int));

	for (i = 0; i <= nlines; i++) {
	    coredist[i] = -1;
	    reachability[i] = -1;
	    nextpnt[i] = -1;
	    cid[i] = 0;
	    idx[i] = 0;
	}

	/* reachability network */
	G_message(_("Reachability network ..."));
	kdtree_init_trav(&trav, kdt);
	c[2] = 0.0;
	i = 0;
	while (kdtree_traverse(&trav, c, &uid)) {
	    G_percent(i++, npoints, 4);

	    /* knn search */
	    kdfound = kdtree_knn(kdt, c, ki, kd, minpnts, &uid);
	    if (kdfound < minpnts)
		G_fatal_error(_("Not enough points found"));
	    coredist[uid] = kd[minpnts - 1];

	    for (j = 0; j < kdfound; j++) {
		/* new reachability */
		newrd = kd[j];
		if (coredist[ki[j]] > kd[j]) {
		    /* do not connect a point to its own cluster
		     * because points in its own cluster
		     * have already been connected to this point
		     * or reconnected */
		    newrd = coredist[ki[j]];
		}

		if (reachability[ki[j]] == -1 || reachability[ki[j]] > newrd) {
		    reachability[ki[j]] = newrd;
		    nextpnt[ki[j]] = uid;

		    /* no link - back link */
		    if (nextpnt[uid] == ki[j]) {
			if (coredist[ki[j]] == -1) {
			    G_fatal_error(_("Neighbor point's core dist is -1"));
			}
			if (coredist[ki[j]] < coredist[uid]) {
			    nextpnt[ki[j]] = -1;
			    reachability[ki[j]] = -1;
			    nextpnt[uid] = ki[j];
			}
			else {
			    nextpnt[uid] = -1;
			    reachability[uid] = -1;
			    reachability[uid] = -1;
			}
		    }
		}
	    }
	}
	G_percent(npoints, npoints, 4);

	/* create clusters from reachability network */
	G_message(_("Building clusters ..."));
	G_percent(0, nlines, 4);
	for (i = 1; i <= nlines; i++) {

	    G_percent(i, nlines, 4);

	    if (cid[i] > 0 || coredist[i] == -1 || nextpnt[i] == -1)
		continue;

	    if (cid[nextpnt[i]] > 0) {
		cid[i] = idx[cid[nextpnt[i]]];
	    }
	    else {
		/* start new cluster */
		nclusters++;
		cid[i] = nclusters;
		idx[nclusters] = nclusters;
		uid = nextpnt[i];

		while (uid > 0) {
		    if (cid[uid] == 0) {
			cid[uid] = nclusters;

			uid = nextpnt[uid];
		    }
		    else {
			/* relabel */
			OLD = cid[uid];
			NEW = idx[OLD];
			while (OLD != NEW) {
			    OLD = NEW;
			    NEW = idx[OLD];
			}
			idx[NEW] = nclusters;
			uid = nextpnt[uid];
			uid = -1;
		    }
		}
	    }
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
	noutliers = 0;
	while ((type = Vect_read_next_line(&In, Points, Cats)) > 0) {
	    G_percent(i++, nlines, 4);
	    if (type == GV_POINT) {
		cat = renumber[idx[cid[i]]];
		if (!cat)
		    noutliers++;
		Vect_cat_set(Cats, clayer, cat);
		Vect_write_line(&Out, GV_POINT, Points, Cats);
	    }
	}
	G_percent(nlines, nlines, 4);
    }
    else if (clmethod == CL_DENSE) {
	/* MATRUSKA 
	 * clusters in clusters in clusters ...
	 * calculate core density = distance to (minpnts - 1) for each point 
	 * sort points ascending by core density
	 * for each point in sorted list
	 *   if point does not have a cluster id 
	 *     start new cluster, cluster reachability is core density of this point
	 *     connect all points within cluster reachability 
	 *     add all connected points to list
	 *     while list is not empty
	 *       remove last point from list
	 *       connect all points within cluster reachability 
	 *       add all connected points to list
	 *       */
	int *clidx;
	double *kd;
	int *ki;
	double cd;
	int k, kdcount;
	struct ilist *CList;

	clp = G_malloc((nlines + 1) * sizeof(struct cl_pnt));
	clidx = G_malloc((nlines + 1) * sizeof(int));

	kd = G_malloc(minpnts * sizeof(double));
	ki = G_malloc(minpnts * sizeof(int));

	CList = G_new_ilist();

	for (i = 0; i <= nlines; i++) {
	    clp[i].c[0] = 0;
	    clp[i].c[1] = 0;
	    clp[i].c[2] = 0;
	    clp[i].cd = -1;
	    clp[i].uid = -1;
	    clidx[i] = -1;
	}

	/* core density */
	G_message(_("Core density ..."));
	kdtree_init_trav(&trav, kdt);
	c[2] = 0.0;
	i = 0;
	kdcount = 0;
	uid = -1;
	while (kdtree_traverse(&trav, c, &uid)) {
	    G_percent(i++, npoints, 4);

	    /* knn search */
	    kdfound = kdtree_knn(kdt, c, ki, kd, minpnts, &uid);
	    if (kdfound < minpnts)
		G_fatal_error(_("Not enough points found"));

	    cd = kd[minpnts - 1];

	    /* list insert */
	    for (j = kdcount; j > 0; j--) {
		if (clp[j - 1].cd <= cd)
		    break;
		clp[j] = clp[j - 1];
		clidx[clp[j].uid] = j;
	    }
	    clp[j].uid = uid;
	    clp[j].c[0] = c[0];
	    clp[j].c[1] = c[1];
	    clp[j].c[2] = c[2];
	    clp[j].cd = cd;
	    clidx[clp[j].uid] = j;
	    kdcount++;

	}
	G_percent(npoints, npoints, 4);

	/* create clusters */
	G_message(_("Building clusters ..."));
	nclusters = 0;
	for (i = 0; i < kdcount; i++) {
	    G_percent(i, kdcount, 4);

	    if (cid[clp[i].uid] > 0)
		continue;

	    /* knn search */
	    kdfound = kdtree_knn(kdt, clp[i].c, ki, kd, minpnts, &clp[i].uid);
	    if (kdfound < minpnts)
		G_fatal_error(_("Not enough points found"));

	    /* start a new cluster */
	    uid = clp[i].uid;
	    nclusters++;
	    cat = nclusters;
	    cid[uid] = cat;
	    cd = clp[i].cd;
	    CList->n_values = 0;
	    for (j = 0; j < kdfound; j++) {
		if (cid[ki[j]] == 0) {
		    G_ilist_add(CList, clidx[ki[j]]);
		    cid[ki[j]] = cat;
		}
	    }
	    if (CList->n_values < minpnts) {
		CList->n_values = 0;
		nclusters--;
		cid[uid] = 0;
		for (j = 0; j < kdfound; j++) {
		    if (cid[ki[j]] == cat) {
			cid[ki[j]] = 0;
		    }
		}
	    }

	    while (CList->n_values) {
		/* expand cluster */
		CList->n_values--;
		k = CList->value[CList->n_values];
		if (k < 0)
		    G_fatal_error("expand cluster: k < 0");
		if (clp[k].uid < 1)
		    G_fatal_error("expand cluster: clp[k].uid < 1");

		kdfound = kdtree_knn(kdt, clp[k].c, ki, kd, minpnts, &clp[k].uid);
		if (kdfound < minpnts)
		    G_fatal_error(_("Not enough points found"));

		for (j = 0; j < kdfound; j++) {
		    if (kd[j] <= cd && cid[ki[j]] == 0) {
			cid[ki[j]] = cat;
			if (clidx[ki[j]] < 0)
			    G_fatal_error("expand cluster ngbrs: clidx[ki[j]] < 0");
			G_ilist_add(CList, clidx[ki[j]]);
		    }
		}
	    }
	}
	G_percent(kdcount, kdcount, 4);

	/* write cluster ids */
	G_message(_("Write out cluster ids ..."));
	Vect_rewind(&In);
	i = 0;
	noutliers = 0;
	while ((type = Vect_read_next_line(&In, Points, Cats)) > 0) {
	    G_percent(i++, nlines, 4);
	    if (type == GV_POINT) {
		cat = cid[i];
		if (!cat)
		    noutliers++;
		Vect_cat_set(Cats, clayer, cat);
		Vect_write_line(&Out, GV_POINT, Points, Cats);
	    }
	}
	G_percent(nlines, nlines, 4);
    }

    if (!flag_attr->answer)
	Vect_copy_tables(&In, &Out, 0);

    /* Build topology for vector map and close them */
    Vect_close(&In);
    if (!flag_topo->answer)
	Vect_build(&Out);
    Vect_close(&Out);

    G_message(_("%d clusters found"), nclusters);
    G_message(_("%d outliers found"), noutliers);

    exit(EXIT_SUCCESS);
}

/* min heap */

/* compare heap points */
int cmp_pnt(int a, int b)
{
    if (clp[a].reach < clp[b].reach)
	return 1;
    if (clp[a].reach > clp[b].reach)
	return 0;
    if (clp[a].uid < clp[b].uid)
	return 1;
    return 0;
}

/* standard sift-up routine for d-ary min heap */
static int sift_up(int start)
{
    register int parent, child, child_added;

    child = start;
    child_added = heapidx[child];

    while (child > 1) {
	GET_PARENT(parent, child);

	/* child smaller */
	if (cmp_pnt(child_added, heapidx[parent])) {
	    /* push parent point down */
	    heapidx[child] = heapidx[parent];
	    child = parent;
	}
	else
	    /* no more sifting up, found new slot for child */
	    break;
    }

    /* put point in new slot */
    if (child < start) {
	heapidx[child] = child_added;
    }

    return 0;
}

/* add point routine for min heap */
int add_pt(int idx)
{
    /* add point to next free position */
    heapsize++;

    heapidx[heapsize] = idx;

    /* sift up: move new point towards top of heap */
    sift_up(heapsize);

    return 0;
}

/* drop point routine for min heap */
int drop_pt(void)
{
    register int child, childr, parent;
    register int i, idx;

    idx = heapidx[1];
    if (heapsize == 1) {
	heapidx[1] = -1;
	heapsize = 0;
	return idx;
    }

    /* start with root */
    parent = 1;

    /* sift down: move hole back towards bottom of heap */
    while (GET_CHILD(child, parent) <= heapsize) {

	i = child + 3;
	for (childr = child + 1; childr <= heapsize && childr < i; childr++) {
	    if (cmp_pnt(heapidx[childr], heapidx[child])) {
		child = childr;
	    }
	}

	/* move hole down */
	heapidx[parent] = heapidx[child];
	parent = child;
    }

    /* hole is in lowest layer, move to heap end */
    if (parent < heapsize) {
	heapidx[parent] = heapidx[heapsize];

	/* sift up last swapped point, only necessary if hole moved to heap end */
	sift_up(parent);
    }

    /* the actual drop */
    heapsize--;

    return idx;
}

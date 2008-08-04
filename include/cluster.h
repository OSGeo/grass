#ifndef GRASS_CLUSTER_H
#define GRASS_CLUSTER_H

#include <grass/gis.h>
#include <grass/imagery.h>

struct Cluster
{
    int nbands;
    int npoints;
    DCELL **points;
    int np;

    double *band_sum;		/* sum over each band */
    double *band_sum2;		/* sum of squares over each band */

    int *class;			/* class of each point */
    int *reclass;		/* for removing empty classes  */
    int *count;			/* number of points in each class */
    int *countdiff;		/* change in count */
    double **sum;		/* sum over band per class */
    double **sumdiff;		/* change in sum */
    double **sum2;		/* sum of squares per band per class */
    double **mean;		/* initial class means */
    struct Signature S;		/* final signature(s) */

    int nclasses;
    int merge1, merge2;
    int iteration;
    double percent_stable;
};

/* c_assign.c */
int I_cluster_assign(struct Cluster *, int *);

/* c_begin.c */
int I_cluster_begin(struct Cluster *, int);

/* c_clear.c */
int I_cluster_clear(struct Cluster *);

/* c_distinct.c */
int I_cluster_distinct(struct Cluster *, double);

/* c_exec.c */
int I_cluster_exec(struct Cluster *, int, int, double, double, int, int (*)(),
		   int *);
/* c_execmem.c */
int I_cluster_exec_allocate(struct Cluster *);
int I_cluster_exec_free(struct Cluster *);

/* c_means.c */
int I_cluster_means(struct Cluster *);

/* c_merge.c */
int I_cluster_merge(struct Cluster *);

/* c_nclasses.c */
int I_cluster_nclasses(struct Cluster *, int);

/* c_point.c */
int I_cluster_point(struct Cluster *, DCELL *);
int I_cluster_begin_point_set(struct Cluster *, int);
int I_cluster_point_part(struct Cluster *, DCELL, int, int);
int I_cluster_end_point_set(struct Cluster *, int);

/* c_reassign.c */
int I_cluster_reassign(struct Cluster *, int *);

/* c_reclass.c */
int I_cluster_reclass(struct Cluster *, int);

/* c_sep.c */
double I_cluster_separation(struct Cluster *, int, int);

/* c_sig.c */
int I_cluster_signatures(struct Cluster *);

/* c_sum2.c */
int I_cluster_sum2(struct Cluster *);

#endif

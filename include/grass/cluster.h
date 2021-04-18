#ifndef GRASS_CLUSTER_H
#define GRASS_CLUSTER_H

#include <grass/gis.h>
#include <grass/imagery.h>

struct Cluster
{
    int nbands;                 /* number of bands */
    int npoints;                /* number of points */
    DCELL **points;             /* array of points */
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

    int nclasses;               /* number of classes */
    int merge1, merge2;
    int iteration;              /* number of iterations */
    double percent_stable;      /* percentage stable */
};

#include <grass/defs/cluster.h>

#endif

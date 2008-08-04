#include <grass/cluster.h>

/* compute sum of squares for each class */
int I_cluster_sum2(struct Cluster *C)
{
    int p, band, class;
    double q;

    /*
       fprintf (stderr, "I_cluster_sum2(npoints=%d,nclasses=%d,nbands=%d)\n", C->npoints, C->nclasses, C->nbands);
     */
    for (class = 0; class < C->nclasses; class++)
	for (band = 0; band < C->nbands; band++)
	    C->sum2[band][class] = 0;

    for (p = 0; p < C->npoints; p++) {
	class = C->class[p];
	if (class < 0)
	    continue;
	for (band = 0; band < C->nbands; band++) {
	    q = C->points[band][p];
	    C->sum2[band][class] += q * q;
	}
    }

    return 0;
}

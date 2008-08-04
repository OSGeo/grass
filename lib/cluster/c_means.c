#include <math.h>
#include <grass/cluster.h>

int I_cluster_means(struct Cluster *C)
{
    int band;
    int class;
    double m, v;		/* m=mean, v=variance then std dev */
    double s;

    /*
       fprintf(stderr,"I_cluster_means(nbands=%d,nclasses=%d)\n",C->nbands, C->nclasses);
     */
    for (band = 0; band < C->nbands; band++) {
	s = C->band_sum[band];
	m = s / C->npoints;
	v = C->band_sum2[band] - s * m;
	v = sqrt(v / (C->npoints - 1));
	for (class = 0; class < C->nclasses; class++)
	    C->mean[band][class] = m;
	if (C->nclasses > 1)
	    for (class = 0; class < C->nclasses; class++)
		C->mean[band][class] +=
		    ((2.0 * class) / (C->nclasses - 1) - 1.0) * v;
    }

    return 0;
}

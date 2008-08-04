#include <grass/cluster.h>

int I_cluster_merge(struct Cluster *C)
{
    int band, p;
    int c1, c2;

    c1 = C->merge1;
    c2 = C->merge2;

    for (p = 0; p < C->npoints; p++)
	if (C->class[p] == c2)
	    C->class[p] = c1;
    C->count[c1] += C->count[c2];
    C->count[c2] = 0;
    for (band = 0; band < C->nbands; band++) {
	C->sum[band][c1] += C->sum[band][c2];
	C->sum[band][c2] = 0;
    }

    return 0;
}

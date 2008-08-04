#include <grass/cluster.h>

int I_cluster_nclasses(struct Cluster *C, int minsize)
{
    int i, n;

    n = 0;
    for (i = 0; i < C->nclasses; i++)
	if (C->count[i] >= minsize)
	    n++;
    return n;
}

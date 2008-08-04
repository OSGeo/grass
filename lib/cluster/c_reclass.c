#include <grass/cluster.h>

int I_cluster_reclass(struct Cluster *C, int minsize)
{
    int band, c, hole, move, p;

    for (c = 0; c < C->nclasses; c++)
	C->reclass[c] = c;

    /* find first `empty' class */
    for (hole = 0; hole < C->nclasses; hole++)
	if (C->count[hole] < minsize)
	    break;

    /* if none, just return */
    if (hole >= C->nclasses)
	return 1;

    for (move = hole; move < C->nclasses; move++)
	if (C->count[move] >= minsize) {
	    C->reclass[move] = hole;
	    C->count[hole] = C->count[move];
	    for (band = 0; band < C->nbands; band++)
		C->sum[band][hole] = C->sum[band][move];
	    hole++;
	}
	else
	    C->reclass[move] = -1;	/* eliminate this class */

    for (p = 0; p < C->npoints; p++)
	C->class[p] = C->reclass[C->class[p]];
    C->nclasses = hole;

    return 0;
}

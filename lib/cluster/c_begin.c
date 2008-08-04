#include <stdlib.h>
#include <grass/cluster.h>

/****************************************************************
 * I_cluster_begin (C,nbands)
 *
 * initialize the cluster routines for nbands
 *
 * returns 
 *  0 ok
 * -1 out of memory
 *  1 illegal number of bands
 *
 ***************************************************************/

int I_cluster_begin(struct Cluster *C, int nbands)
{
    int band;

    if (C->points != NULL) {
	for (band = 0; band < C->nbands; band++)
	    if (C->points[band] != NULL)
		free(C->points[band]);
	free(C->points);
    }
    if (C->band_sum != NULL)
	free(C->band_sum);
    if (C->band_sum2 != NULL)
	free(C->band_sum2);

    C->points = NULL;
    C->band_sum = NULL;
    C->band_sum2 = NULL;

    I_free_signatures(&C->S);

    /* record the number of bands */
    C->nbands = nbands;
    if (nbands <= 0)
	return 1;

    /* prepare the signatures for nbands */

    I_init_signatures(&C->S, nbands);
    sprintf(C->S.title, "produced by i.cluster");

    /* allocate the data (points) arrays */
    C->points = (DCELL **) malloc(C->nbands * sizeof(DCELL *));
    if (C->points == NULL)
	return -1;
    for (band = 0; band < C->nbands; band++)
	C->points[band] = NULL;

    C->np = 128;
    for (band = 0; band < C->nbands; band++) {
	C->points[band] = (DCELL *) malloc(C->np * sizeof(DCELL));
	if (C->points[band] == NULL)
	    return -1;
    }

    /* initialize the count to zero */
    C->npoints = 0;

    /* allocate the band sums and means */
    C->band_sum = (double *)malloc(C->nbands * sizeof(double));
    if (C->band_sum == NULL)
	return -1;
    C->band_sum2 = (double *)malloc(C->nbands * sizeof(double));
    if (C->band_sum2 == NULL)
	return -1;
    for (band = 0; band < C->nbands; band++) {
	C->band_sum[band] = 0;
	C->band_sum2[band] = 0;
    }

    return 0;
}

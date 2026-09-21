#include <math.h>
#include <stdlib.h>

#include <grass/gis.h>

#include "local_proto.h"

struct ppa_result *ppa_result_alloc(int num_distances)
{
    struct ppa_result *result = G_malloc(sizeof(struct ppa_result));

    result->num_distances = num_distances;
    result->distance = G_malloc(num_distances * sizeof(double));
    result->value = G_malloc(num_distances * sizeof(double));
    result->csr = G_malloc(num_distances * sizeof(double));

    return result;
}

void ppa_result_free(struct ppa_result *result)
{
    G_free(result->distance);
    G_free(result->value);
    G_free(result->csr);
    G_free(result);
}

static int cmp_double(const void *a, const void *b)
{
    double da = *(const double *)a;
    double db = *(const double *)b;

    return (da > db) - (da < db);
}

/* Fill the result with the empirical cumulative distribution of the
 * given distances, evaluated at num_distances equally spaced values up
 * to max_distance, and with the corresponding distribution under CSR,
 * 1 - exp(-intensity * pi * d^2). The distances array is sorted in
 * place. */
void ppa_result_from_distances(struct ppa_result *result, double *distances,
                               int count, double max_distance, double intensity)
{
    double interval = max_distance / result->num_distances;
    int below = 0;

    qsort(distances, count, sizeof(double), cmp_double);

    for (int i = 0; i < result->num_distances; i++) {
        /* The last distance is exactly max_distance so that rounding
         * in the interval multiples cannot exclude the largest
         * observed distance. */
        double d =
            i == result->num_distances - 1 ? max_distance : (i + 1) * interval;

        while (below < count && distances[below] <= d)
            below++;
        result->distance[i] = d;
        result->value[i] = (double)below / count;
        result->csr[i] = 1.0 - exp(-intensity * M_PI * d * d);
    }
}

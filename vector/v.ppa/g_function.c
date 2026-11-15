#include <math.h>
#include <stdlib.h>

#include <grass/gis.h>
#include <grass/glocale.h>

#include "local_proto.h"

/* Estimate the nearest neighbor distance distribution function G. The
 * distances are evaluated up to the largest observed nearest neighbor
 * distance, so the estimate reaches 1 at the last distance. No edge
 * correction is applied. */
struct ppa_result *g_function(struct kdtree *tree,
                              const struct ppa_point *points, int n,
                              double intensity, int num_distances)
{
    struct ppa_result *result;
    double *distances = G_malloc(n * sizeof(double));
    double max_distance = 0.0;

    G_message(_("Computing G function..."));

#pragma omp parallel for reduction(max : max_distance)
    for (int i = 0; i < n; i++) {
        double coords[2] = {points[i].x, points[i].y};
        int self = i;
        int uid;
        double sqdist = 0.0;

        /* Skipping the point's own uid cannot leave the search empty
         * because the caller guarantees at least two points. */
        kdtree_knn(tree, coords, &uid, &sqdist, 1, &self);
        distances[i] = sqrt(sqdist);
        if (distances[i] > max_distance)
            max_distance = distances[i];
    }

    result = ppa_result_alloc(num_distances);
    ppa_result_from_distances(result, distances, n, max_distance, intensity);
    G_free(distances);

    return result;
}

#include <math.h>
#include <stdlib.h>

#include <grass/gis.h>
#include <grass/glocale.h>

#include "local_proto.h"

/* Estimate the empty space function F: the distribution of distances
 * from uniformly random locations in the window to the nearest point
 * of the pattern. The distances are evaluated up to the largest
 * observed empty space distance. No edge correction is applied. */
struct ppa_result *f_function(struct kdtree *tree, int num_random_points,
                              const struct ppa_window *window, double intensity,
                              int num_distances)
{
    struct ppa_result *result;
    struct ppa_point *sample =
        G_malloc(num_random_points * sizeof(struct ppa_point));
    double *distances = G_malloc(num_random_points * sizeof(double));
    double max_distance = 0.0;

    G_message(_("Computing F function..."));

    /* G_drand48() keeps global state, so generate sequentially. */
    for (int i = 0; i < num_random_points; i++) {
        sample[i].x =
            window->west + (window->east - window->west) * G_drand48();
        sample[i].y =
            window->south + (window->north - window->south) * G_drand48();
    }

#pragma omp parallel for reduction(max : max_distance)
    for (int i = 0; i < num_random_points; i++) {
        double coords[2] = {sample[i].x, sample[i].y};
        int uid;
        double sqdist = 0.0;

        kdtree_knn(tree, coords, &uid, &sqdist, 1, NULL);
        distances[i] = sqrt(sqdist);
        if (distances[i] > max_distance)
            max_distance = distances[i];
    }

    result = ppa_result_alloc(num_distances);
    ppa_result_from_distances(result, distances, num_random_points,
                              max_distance, intensity);
    G_free(sample);
    G_free(distances);

    return result;
}

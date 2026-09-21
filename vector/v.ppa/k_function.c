#include <math.h>
#include <stdlib.h>

#include <grass/gis.h>
#include <grass/glocale.h>

#include "local_proto.h"

/* Ripley's isotropic edge correction weight for a rectangular window
 * (Ripley 1977). The weight is 1/e where e is the fraction of the
 * circle centered at the point with the given radius that lies inside
 * the window. The exterior fraction is the sum of the arcs cut off by
 * the four window edges, with the overlap of arcs of adjacent edges
 * beyond the window corners added back (inclusion-exclusion). This is
 * the closed-form solution also used by spatstat's edge.Ripley
 * (Baddeley, Rubak, Turner 2015). */
double isotropic_weight(const struct ppa_point *point, double radius,
                        const struct ppa_window *window)
{
    /* Distances to the two vertical edges, then to the two
     * horizontal edges. */
    double edge[4] = {point->x - window->west, window->east - point->x,
                      point->y - window->south, window->north - point->y};
    double exterior = 0.0;

    for (int i = 0; i < 4; i++) {
        if (edge[i] < radius)
            exterior += 2.0 * acos(edge[i] / radius);
    }

    /* Corners pair one vertical with one horizontal edge. */
    for (int i = 0; i < 2; i++) {
        for (int j = 2; j < 4; j++) {
            if (edge[i] * edge[i] + edge[j] * edge[j] < radius * radius)
                exterior -=
                    acos(edge[i] / radius) + acos(edge[j] / radius) - M_PI_2;
        }
    }

    return 1.0 / (1.0 - exterior / (2.0 * M_PI));
}

/* Estimate Ripley's K function at num_distances equally spaced
 * distances up to max_distance:
 *
 *   K(d) = |W| / (n * (n - 1)) * sum over ordered pairs (i, j) of
 *          w_ij * 1(distance(i, j) <= d)
 *
 * where w_ij is the isotropic edge correction weight, or 1 when no
 * correction is requested. */
struct ppa_result *k_function(struct kdtree *tree,
                              const struct ppa_point *points, int n,
                              const struct ppa_window *window,
                              double max_distance, int num_distances,
                              enum ppa_correction correction)
{
    struct ppa_result *result;
    double interval = max_distance / num_distances;
    double area =
        (window->east - window->west) * (window->north - window->south);
    double *counts = G_calloc(num_distances, sizeof(double));
    double cumulative = 0.0;

    G_message(_("Computing K function..."));

#pragma omp parallel
    {
        double *local_counts = G_calloc(num_distances, sizeof(double));

#pragma omp for
        for (int i = 0; i < n; i++) {
            double coords[2] = {points[i].x, points[i].y};
            int self = i;
            int *uids = NULL;
            double *sqdists = NULL;
            int found =
                kdtree_dnn(tree, coords, &uids, &sqdists, max_distance, &self);

            for (int m = 0; m < found; m++) {
                double d = sqrt(sqdists[m]);
                /* First bin whose distance (bin + 1) * interval
                 * covers d; a pair contributes to all later bins
                 * through the cumulative sum below. */
                int bin = (int)ceil(d / interval) - 1;

                if (bin < 0)
                    bin = 0;
                /* The search radius guarantees d <= max_distance, so
                 * an index beyond the last bin can only come from
                 * rounding. */
                if (bin >= num_distances)
                    bin = num_distances - 1;
                if (correction == PPA_CORRECTION_ISOTROPIC)
                    local_counts[bin] +=
                        isotropic_weight(&points[i], d, window);
                else
                    local_counts[bin] += 1.0;
            }
            G_free(uids);
            G_free(sqdists);
        }

#pragma omp critical
        for (int bin = 0; bin < num_distances; bin++)
            counts[bin] += local_counts[bin];

        G_free(local_counts);
    }

    result = ppa_result_alloc(num_distances);
    for (int bin = 0; bin < num_distances; bin++) {
        double d =
            bin == num_distances - 1 ? max_distance : (bin + 1) * interval;

        cumulative += counts[bin];
        result->distance[bin] = d;
        result->value[bin] = area * cumulative / ((double)n * (n - 1));
        result->csr[bin] = M_PI * d * d;
    }
    G_free(counts);

    return result;
}

/* Estimate the L function, the variance stabilized transformation
 * L(d) = sqrt(K(d) / pi), which is d itself under CSR. */
struct ppa_result *l_function(struct kdtree *tree,
                              const struct ppa_point *points, int n,
                              const struct ppa_window *window,
                              double max_distance, int num_distances,
                              enum ppa_correction correction)
{
    struct ppa_result *result = k_function(
        tree, points, n, window, max_distance, num_distances, correction);

    for (int i = 0; i < num_distances; i++) {
        result->value[i] = sqrt(result->value[i] / M_PI);
        result->csr[i] = result->distance[i];
    }

    return result;
}

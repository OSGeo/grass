#ifndef LOCAL_PROTO_H
#define LOCAL_PROTO_H

#include <grass/kdtree.h>

/* A point of the analyzed pattern. */
struct ppa_point {
    double x, y;
};

/* Rectangular observation window (computational region bounds). */
struct ppa_window {
    double west, east, south, north;
};

/* Edge correction methods for the K and L functions. */
enum ppa_correction {
    PPA_CORRECTION_NONE,
    PPA_CORRECTION_ISOTROPIC,
};

/* A summary function estimated at num_distances equally spaced
 * distances, together with its theoretical values under complete
 * spatial randomness (CSR). */
struct ppa_result {
    int num_distances;
    double *distance;
    double *value;
    double *csr;
};

/* result.c */
struct ppa_result *ppa_result_alloc(int num_distances);
void ppa_result_free(struct ppa_result *result);
void ppa_result_from_distances(struct ppa_result *result, double *distances,
                               int count, double max_distance,
                               double intensity);

/* g_function.c */
struct ppa_result *g_function(struct kdtree *tree,
                              const struct ppa_point *points, int n,
                              double intensity, int num_distances);

/* f_function.c */
struct ppa_result *f_function(struct kdtree *tree, int num_random_points,
                              const struct ppa_window *window, double intensity,
                              int num_distances);

/* k_function.c */
double isotropic_weight(const struct ppa_point *point, double radius,
                        const struct ppa_window *window);
struct ppa_result *k_function(struct kdtree *tree,
                              const struct ppa_point *points, int n,
                              const struct ppa_window *window,
                              double max_distance, int num_distances,
                              enum ppa_correction correction);
struct ppa_result *l_function(struct kdtree *tree,
                              const struct ppa_point *points, int n,
                              const struct ppa_window *window,
                              double max_distance, int num_distances,
                              enum ppa_correction correction);

#endif /* LOCAL_PROTO_H */

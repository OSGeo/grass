#if defined(_OPENMP)
#include <omp.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>
#include <math.h>
#include <grass/kdtree.h>
#include <float.h>

struct Point {
    double x, y;
    int id;
};

void calculate_k_function(struct kdtree *kdtree, struct Point *points, int n,
                          const char *output_file);
void calculate_l_function(struct Point *points, int n, const char *output_file);
void calculate_f_function(struct Point *points, int n, const char *output_file);
void calculate_g_functoin(struct kdtree *kdtree, struct Point *points, int n,
                          const char *output_file);
double euclidean_distance(const struct Point *p1, const struct Point *p2);

int main(int argc, char *argv[])
{
    struct Map_info Map;

    // Initialize the GIS environment
    G_gisinit(argv[0]);

    // Set up module description
    struct GModule *module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("point pattern analysis"));
    module->description =
        _("Point pattern analysis using K, L, F, and G functions.");

    // Define options
    struct Option *input_opt = G_define_standard_option(G_OPT_V_INPUT);
    struct Option *output_opt = G_define_standard_option(G_OPT_F_OUTPUT);
    struct Option *method_opt = G_define_option();
    method_opt->key = "method";
    method_opt->type = TYPE_STRING;
    method_opt->required = YES;
    method_opt->options = "k,l,f,g";
    method_opt->description = _("Method to calculate (k, l, f, g)");

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    const char *input_vector = input_opt->answer;
    const char *output_file = output_opt->answer;
    const char *method = method_opt->answer;

    // Open the vector map
    if (Vect_open_old(&Map, input_vector, "") < 0)
        G_fatal_error(_("Unable to open vector map <%s>"), input_vector);

    // Allocate memory for points
    struct line_pnts *points = Vect_new_line_struct();
    struct line_cats *cats = Vect_new_cats_struct();
    int nlines = Vect_get_num_lines(&Map);

    int n = Vect_get_num_primitives(&Map, GV_POINT);

    // Allocate memory for points array
    struct Point *pts = (struct Point *)malloc(n * sizeof(struct Point));
    int idx = 0;

    // Store points
    // Initialize k-d tree
    struct kdtree *kdtree = kdtree_create(2, NULL);

    for (int line = 1; line <= nlines; line++) {
        if (Vect_read_line(&Map, points, cats, line) == GV_POINT) {
            pts[idx].x = points->x[0];
            pts[idx].y = points->y[0];
            pts[idx].id = idx;
            double coords[2] = {points->x[0], points->y[0]};
            kdtree_insert(kdtree, coords, idx, 0);
            idx++;
        }
    }

    G_message(_("Number of points: %d"), n);
    G_message(_("Method: %s"), method);

    if (strcmp(method, "k") == 0) {
        calculate_k_function(kdtree, pts, n, output_file);
    }
    else if (strcmp(method, "l") == 0) {
        calculate_l_function(pts, n, output_file);
    }
    else if (strcmp(method, "f") == 0) {
        calculate_f_function(pts, n, output_file);
    }
    else if (strcmp(method, "g") == 0) {
        calculate_g_functoin(kdtree, pts, n, output_file);
    }
    else {
        G_fatal_error(_("Method not implemented yet"));
    }

    // Free memory and close the vector map
    free(pts);
    Vect_close(&Map);

    return 0;
}

/**
 * Calculates the k-function for a given set of points using a k-d tree.
 *
 * @param kdtree The k-d tree used for nearest neighbor searches.
 * @param points An array of Point structures representing the points.
 * @param n The number of points in the array.
 * @param output_file The path to the output file where the results will be
 * saved.
 */
void calculate_k_function(struct kdtree *kdtree, struct Point *points, int n,
                          const char *output_file)
{
    FILE *fp = fopen(output_file, "w");
    if (fp == NULL) {
        G_fatal_error(_("Unable to open output file <%s>"), output_file);
    }

    double max_dist = 0.0;
#pragma omp parallel for reduction(max : max_dist)
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            if (i != j) {
                double dist = euclidean_distance(&points[i], &points[j]);
                if (dist > max_dist) {
                    max_dist = dist;
                }
            }
        }
    }

    G_message(_("Max distance: %f"), max_dist);

    fprintf(fp, "Distance,K-value\n");
    for (double d = 0; d <= max_dist; d += max_dist / 100) {
        double k_value = 0.0;

#pragma omp parallel for reduction(+ : k_value)
        for (int i = 0; i < n; i++) {
            double coords[2] = {points[i].x, points[i].y};
            int *puid = NULL;
            double *pd = NULL;
            int count =
                kdtree_dnn(kdtree, coords, &puid, &pd, d, &points[i].id);
            k_value += count - 1; // subtract 1 to exclude the point itself
            free(puid);
            free(pd);
        }

        k_value /= (n * (n - 1));
        fprintf(fp, "%f,%f\n", d, k_value);
    }

    kdtree_destroy(kdtree);
    fclose(fp);
}

double euclidean_distance(const struct Point *p1, const struct Point *p2)
{
    return sqrt((p1->x - p2->x) * (p1->x - p2->x) +
                (p1->y - p2->y) * (p1->y - p2->y));
}

/**
 * Calculates the L function for a given array of points.
 * L(d) = √(K(d) / π)
 *
 * @param points        The array of points.
 * @param n             The number of points in the array.
 * @param output_file   The path to the output file where the results will be
 * written.
 */
void calculate_l_function(struct Point *points, int n, const char *output_file)
{

    FILE *fp = fopen(output_file, "w");
    if (fp == NULL) {
        G_fatal_error(_("Unable to open output file <%s>"), output_file);
    }

    double **distances = (double **)malloc(n * sizeof(double *));
    for (int i = 0; i < n; i++) {
        distances[i] = (double *)malloc(n * sizeof(double));
    }

    double max_dist = 0.0;
#pragma omp parallel for reduction(max : max_dist)
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            if (i != j) {
                double dist = euclidean_distance(&points[i], &points[j]);
                distances[i][j] = dist;
                if (dist > max_dist) {
                    max_dist = dist;
                }
            }
        }
    }

    G_message(_("Max distance: %f"), max_dist);

    fprintf(fp, "Distance,L-value\n");
    for (double d = 0; d <= max_dist; d += max_dist / 100) {
        double k_value = 0.0;

#pragma omp parallel for reduction(+ : k_value)
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                if (i != j && distances[i][j] <= d) {
                    k_value += 1.0;
                }
            }
        }

        k_value /= (n * (n - 1));
        double l_value = sqrt(k_value / M_PI);
        fprintf(fp, "%f,%f\n", d, l_value);
    }

    for (int i = 0; i < n; i++) {
        free(distances[i]);
    }
    free(distances);
    fclose(fp);
}

/**
 * Calculates the f function for a given array of points.
 * F(d) = (1/n) * Σ I(d_ij ≤ d)
 *
 * @param points       The array of points.
 * @param n            The number of points in the array.
 * @param output_file  The output file to write the results to.
 */
void calculate_f_function(struct Point *points, int n, const char *output_file)
{
    FILE *fp = fopen(output_file, "w");
    if (fp == NULL) {
        G_fatal_error(_("Unable to open output file <%s>"), output_file);
    }

    double *distances = (double *)malloc(n * sizeof(double));
    double max_dist = 0.0;
#pragma omp parallel for reduction(max : max_dist)
    for (int i = 0; i < n; i++) {
        double min_dist = DBL_MAX;
        for (int j = 0; j < n; j++) {
            if (i != j) {
                double dist = euclidean_distance(&points[i], &points[j]);
                if (dist < min_dist) {
                    min_dist = dist;
                }
                if (dist > max_dist) {
                    max_dist = dist;
                }
            }
        }
        distances[i] = min_dist;
    }

    G_message(_("Max distance: %f"), max_dist);

    fprintf(fp, "Distance,F-value\n");
    for (double d = 0.0; d <= max_dist; d += max_dist / 100.0) {
        double f_value = 0.0;

#pragma omp parallel for reduction(+ : f_value)
        for (int i = 0; i < n; i++) {
            if (distances[i] <= d) {
                f_value += 1;
            }
        }

        f_value /= n;
        fprintf(fp, "%f,%f\n", d, f_value);
    }

    free(distances);
    fclose(fp);
}

/**
 * Calculates the g function using a k-d tree and an array of points.
 *
 * @param kdtree The k-d tree used for nearest neighbor search.
 * @param points An array of points.
 * @param n The number of points in the array.
 * @param output_file The path to the output file where the results will be
 * written.
 */
void calculate_g_function(struct kdtree *kdtree, struct Point *points, int n,
                          const char *output_file)
{
    G_message(_("G-Function"));
    FILE *fp = fopen(output_file, "w");
    if (fp == NULL) {
        G_fatal_error(_("Unable to open output file <%s>"), output_file);
    }

    double max_dist = 0.0;
#pragma omp parallel for reduction(max : max_dist)
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            if (i != j) {
                double dist = euclidean_distance(&points[i], &points[j]);
                if (dist > max_dist) {
                    max_dist = dist;
                }
            }
        }
    }

    fprintf(fp, "Distance,G-value\n");
    for (double d = 0; d <= max_dist; d += max_dist / 100) {
        G_percent(d, max_dist, 4);
        double g_value = 0.0;

#pragma omp parallel for reduction(+ : g_value)
        for (int i = 0; i < n; i++) {
            double coords[2] = {points[i].x, points[i].y};
            int puid = 0;
            double pd = 0.0;
            kdtree_knn(kdtree, coords, &puid, &pd, 1, &points[i].id);

            if (pd <= d) {
                g_value += 1.0; // add the distance to the nearest neighbor
            }
        }

        // Normalize g-value
        g_value /= n;
        fprintf(fp, "%f,%f\n", d, g_value);
    }

    fclose(fp);
}

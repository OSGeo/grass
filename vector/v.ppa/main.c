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
#include <grass/parson.h>

struct Point {
    double x, y;
    int id;
};

enum OutputFormat { PLAIN, JSON };

void calculate_g_function(struct kdtree *kdtree, struct Point *points, int n,
                          int i, const char *output_file);
void calculate_g_function_values(struct kdtree *kdtree, struct Point *points,
                                 int n, int num_distances, double *values);
void calculate_f_function(struct kdtree *kdtree, struct Point *points, int n,
                          const char *output_file, struct bound_box *box,
                          int num_random_points);
void calculate_f_function_values(struct kdtree *kdtree, struct Point *points,
                                 int n, struct bound_box *box, double *values,
                                 int num_random_points);

void calculate_k_function(struct kdtree *kdtree, struct Point *points, int n,
                          int num_distances, double intensity,
                          const char *output_file, enum OutputFormat format,
                          JSON_Object *root_object);
void calculate_l_function(struct kdtree *kdtree, struct Point *points, int n,
                          int num_distances, double intensity,
                          const char *output_file, enum OutputFormat format,
                          JSON_Object *root_object);
void calculate_k_function_values(struct kdtree *kdtree, struct Point *points,
                                 int n, int num_distances, double max_dist,
                                 double intensity, double *values);
void calculate_l_function_values(struct kdtree *kdtree, struct Point *points,
                                 int n, int num_distances, double max_dist,
                                 double intensity, double *values);
double max_distance(struct Point *points, int n);
double euclidean_distance(const struct Point *p1, const struct Point *p2);
void generate_random_points(struct Point *random_points, int num_random_points,
                            struct bound_box *box);
double csr_g_value(double d, double i);
void calculate_function_values(
    struct kdtree *kdtree, struct Point *points, int n, double *values,
    int num_distances, double max_dist,
    void (*calc_func)(struct kdtree *, struct Point *, int, double *));
void monte_carlo_envelope(struct kdtree *kdtree, struct Point *points,
                          struct bound_box *box, int n, const char *output_file,
                          int num_simulations,
                          void (*calc_func)(struct kdtree *, struct Point *,
                                            int, double *));
int main(int argc, char *argv[])
{
    struct Map_info Map;

    // Initialize the GIS environment
    G_gisinit(argv[0]);

    // Set up module description
    struct GModule *module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("point pattern analysis"));
    G_add_keyword(_("parallel"));
    G_add_keyword(_("statistics"));
    module->description =
        _("Point pattern analysis using G, F, K, and L functions.");

    // Define options
    struct Option *input_opt = G_define_standard_option(G_OPT_V_INPUT);
    struct Option *output_opt = G_define_standard_option(G_OPT_F_OUTPUT);
    output_opt->required = NO;

    struct Option *method_opt = G_define_option();
    method_opt->key = "method";
    method_opt->type = TYPE_STRING;
    method_opt->required = YES;
    method_opt->options = "g,f,k,l";
    method_opt->description = _("Method to calculate (g, f, k, l)");

    struct Option *random_points_opt = G_define_option();
    random_points_opt->key = "random_points";
    random_points_opt->type = TYPE_INTEGER;
    random_points_opt->required = NO;
    random_points_opt->answer = "1000";
    random_points_opt->description =
        _("Number of random points for F-function calculation (default: 1000)");

    struct Option *format_opt = G_define_standard_option(G_OPT_F_FORMAT);
    format_opt->required = NO;

    struct Option *num_distances_opt = G_define_option();
    num_distances_opt->key = "num_distances";
    num_distances_opt->type = TYPE_INTEGER;
    num_distances_opt->required = NO;
    num_distances_opt->answer = "100";
    num_distances_opt->description = _("Number of distances (default: 100)");

    struct Option *simulations_opt = G_define_option();
    simulations_opt->key = "simulations";
    simulations_opt->type = TYPE_INTEGER;
    simulations_opt->required = NO;
    simulations_opt->answer = "99";
    simulations_opt->description =
        _("Number of simulations for Monte Carlo envelope (default: 99)");

    struct Option *random_seed = G_define_option();
    random_seed->key = "seed";
    random_seed->type = TYPE_INTEGER;
    random_seed->required = NO;
    random_seed->label = _("Seed for random number generator");
    random_seed->description =
        _("The same seed can be used to obtain same results"
          " or random seed can be generated by other means.");

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    /****** INITIALISE RANDOM NUMBER GENERATOR ******/
    long seed_value;
    if (random_seed->answer) {
        seed_value = atol(random_seed->answer);
        G_srand48(seed_value);
        G_verbose_message(_("Read random seed from %s option: %ld"),
                          random_seed->key, seed_value);
    }
    else {
        /* default as it used to be */
        seed_value = G_srand48_auto();
        G_verbose_message(_("Autogenerated random seed set to: %ld"),
                          seed_value);
    }

    const char *input_vector = input_opt->answer;
    const char *output_file = output_opt->answer;
    const char *method = method_opt->answer;

    int num_random_points = atoi(random_points_opt->answer);
    int num_distances = atoi(num_distances_opt->answer);
    int num_simulations = atoi(simulations_opt->answer);

    // Open the vector map
    if (Vect_open_old(&Map, input_vector, "") < 0)
        G_fatal_error(_("Unable to open vector map <%s>"), input_vector);

    enum OutputFormat format;
    JSON_Value *root_value;
    JSON_Object *root_object;

    if (strcmp(format_opt->answer, "json") == 0) {
        format = JSON;
        root_value = json_value_init_object();
        if (root_value == NULL) {
            G_fatal_error(_("Unable to initialize JSON object"));
        }
        root_object = json_object(root_value);
    }
    else {
        format = PLAIN;
    }

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

    // Get bounding box
    struct bound_box box;
    Vect_get_map_box(&Map, &box);
    // Calculate mean number of points per unit area (intensity)
    double area = (box.E - box.W) * (box.N - box.S);
    double intensity = (double)n / area;
    G_message(_("intensity: %f"), intensity);

    G_message(_("Number of points: %d"), n);
    G_message(_("Method: %s"), method);

    if (strcmp(method, "k") == 0) {
        calculate_k_function(kdtree, pts, n, num_distances, intensity,
                             output_file, format, root_object);
    }
    else if (strcmp(method, "l") == 0) {
        calculate_l_function(kdtree, pts, n, num_distances, intensity,
                             output_file, format, root_object);
    }
    else if (strcmp(method, "f") == 0) {
        calculate_f_function(kdtree, pts, n, output_file, &box,
                             num_random_points);
    }
    else if (strcmp(method, "g") == 0) {
        calculate_g_function(kdtree, pts, n, intensity, output_file);
    }
    else {
        G_fatal_error(_("Method not implemented yet"));
    }

    // Free memory and close the vector map
    free(pts);
    kdtree_destroy(kdtree); // Ensure k-d tree is destroyed
    Vect_close(&Map);

    if (format == JSON) {
        char *serialized_string = NULL;
        serialized_string = json_serialize_to_string_pretty(root_value);
        if (serialized_string == NULL) {
            G_fatal_error(_("Failed to initialize pretty JSON string."));
        }
        puts(serialized_string);
        if (output_file) {
            FILE *fp = fopen(output_file, "w");
            if (fp == NULL) {
                G_fatal_error(_("Unable to open output file <%s>"),
                              output_file);
            }
            fputs(serialized_string, fp);
            fclose(fp);
        }
        json_free_serialized_string(serialized_string);
        json_value_free(root_value);
    }

    return 0;
}

double max_distance(struct Point *points, int n)
{
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
    max_dist = sqrt(max_dist);
    return max_dist;
}

void calculate_function_values(struct kdtree *kdtree, struct Point *points,
                               int n, double *values, int num_distances,
                               double max_dist,
                               void (*calc_func)(struct kdtree *,
                                                 struct Point *, int, double *))
{
    calc_func(kdtree, points, n, values);
}

/**
 * Calculates the F function for a given set of points and generates an envelope
 * using Monte Carlo simulations.
 *
 * @param kdtree The k-d tree used for nearest neighbor searches.
 * @param points An array of Point structures representing the points.
 * @param box The bounding box of the map.
 * @param n The number of points in the array.
 * @param output_file The path to the output file where the results will be
 * saved.
 * @param num_random_points The number of random points to generate.
 * @param num_simulations The number of Monte Carlo simulations to perform.
 */
void monte_carlo_envelope(struct kdtree *kdtree, struct Point *points,
                          struct bound_box *box, int n, const char *output_file,
                          int num_simulations,
                          void (*calc_func)(struct kdtree *, struct Point *,
                                            int, double *))
{
    FILE *fp = fopen(output_file, "w");
    if (fp == NULL) {
        G_fatal_error(_("Unable to open output file <%s>"), output_file);
    }

    struct Point *random_points =
        (struct Point *)malloc(n * sizeof(struct Point));

    double max_dist = max_distance(points, n);

    int num_distances = 100;
    double *values = (double *)malloc(num_distances * sizeof(double));
    double *lower_envelope = (double *)malloc(num_distances * sizeof(double));
    double *upper_envelope = (double *)malloc(num_distances * sizeof(double));

    for (int d = 0; d < num_distances; d++) {
        lower_envelope[d] = DBL_MAX;
        upper_envelope[d] = DBL_MIN;
    }

#pragma omp parallel for
    for (int sim = 0; sim < num_simulations; sim++) {
        generate_random_points(
            random_points, n,
            box); // Generate same number of random points as observed points
        double *sim_values = (double *)malloc(num_distances * sizeof(double));
        calculate_function_values(kdtree, random_points, n, sim_values,
                                  num_distances, max_dist, calc_func);

#pragma omp critical
        {
            for (int d = 0; d < num_distances; d++) {
                if (sim_values[d] < lower_envelope[d]) {
                    lower_envelope[d] = sim_values[d];
                }
                if (sim_values[d] > upper_envelope[d]) {
                    upper_envelope[d] = sim_values[d];
                }
            }
        }
        free(sim_values);
    }

    fprintf(fp, "Distance,Lower,Upper\n");
    for (int d = 0; d < num_distances; d++) {
        double distance = d * max_dist / num_distances;
        fprintf(fp, "%f,%f,%f\n", distance, lower_envelope[d],
                upper_envelope[d]);
    }

    free(random_points);
    free(values);
    free(lower_envelope);
    free(upper_envelope);
    fclose(fp);
}

double euclidean_distance(const struct Point *p1, const struct Point *p2)
{
    return (p1->x - p2->x) * (p1->x - p2->x) +
           (p1->y - p2->y) * (p1->y - p2->y);
}

void generate_random_points(struct Point *random_points, int num_random_points,
                            struct bound_box *box)
{
    for (int i = 0; i < num_random_points; i++) {
        random_points[i].x = box->W + (box->E - box->W) * (double)G_drand48();
        random_points[i].y = box->S + (box->N - box->S) * (double)G_drand48();
        random_points[i].id = i;
    }
}

/**
 * Calculates the g function for a given array of points.
 * G(d) = 1 - exp(-i * π * d^2)
 *
 * @param d     The distance.
 * @param i     The intensity (points per unit area).
 * @return The g value.
 */
double csr_g_value(double d, double i)
{
    return 1 - exp(-i * M_PI * d * d);
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
void calculate_l_function(struct kdtree *kdtree, struct Point *points, int n,
                          int num_distances, double intensity,
                          const char *output_file, enum OutputFormat format,
                          JSON_Object *root_object)
{
    FILE *fp = NULL;
    double max_dist = max_distance(points, n);
    double interval = max_dist / num_distances;
    double *values = (double *)malloc(num_distances * sizeof(double));
    if (values == NULL) {
        G_fatal_error(_("Unable to allocate memory for values array."));
    }
    calculate_l_function_values(kdtree, points, n, num_distances, max_dist,
                                intensity, values);

    switch (format) {
    case PLAIN:
        fp = fopen(output_file, "w");
        if (fp == NULL) {
            G_fatal_error(_("Unable to open output file <%s>"), output_file);
        }

        fprintf(fp, "Distance,L-value\n");
        for (int d = 0; d < num_distances; d++) {
            fprintf(fp, "%f,%f\n", d * interval, values[d]);
        }
        fclose(fp);
        break;
    case JSON:
        if (root_object == NULL) {
            G_fatal_error(_("root_object is NULL."));
        }
        json_object_set_null(root_object, "distance");
        json_object_set_null(root_object, "l-value");
        JSON_Value *distance = json_value_init_array();
        if (distance == NULL) {
            G_fatal_error(_("Unable to initialize JSON distance array."));
        }
        JSON_Array *distances = json_array(distance);

        JSON_Value *l_value = json_value_init_array();
        if (l_value == NULL) {
            G_fatal_error(_("Unable to initialize JSON l-value array."));
        }
        JSON_Array *l_values = json_array(l_value);
        for (int d = 0; d < num_distances; d++) {
            json_array_append_number(distances, d * interval);
            json_array_append_number(l_values, values[d]);
        }
        json_object_set_value(root_object, "distance", distance);
        json_object_set_value(root_object, "l-value", l_value);
        break;
    }
    free(values);
}

/**
 * Calculates the f function for a given array of points.
 *
 * @param kdtree The k-d tree used for nearest neighbor search.
 * @param points       The array of points.
 * @param n            The number of points in the array.
 * @param output_file  The output file to write the results to.
 * @param num_random_points The number of random points to generate.
 */
void calculate_f_function(struct kdtree *kdtree, struct Point *points, int n,
                          const char *output_file, struct bound_box *box,
                          int num_random_points)
{
    FILE *fp = fopen(output_file, "w");
    if (fp == NULL) {
        G_fatal_error(_("Unable to open output file <%s>"), output_file);
    }

    // Generate random points
    struct Point *random_points =
        (struct Point *)malloc(num_random_points * sizeof(struct Point));

    generate_random_points(random_points, num_random_points, box);

    double max_dist = 0.0;
    double *distances = (double *)malloc(num_random_points * sizeof(double));

#pragma omp parallel for reduction(max : max_dist)
    for (int i = 0; i < num_random_points; i++) {
        double coords[2] = {random_points[i].x, random_points[i].y};
        int puid;
        double pd;
        kdtree_knn(kdtree, coords, &puid, &pd, 1, NULL);
        distances[i] = sqrt(pd);
        if (distances[i] > max_dist) {
            max_dist = distances[i];
        }
    }

    G_message(_("Max distance: %f"), max_dist);

    fprintf(fp, "Distance,F-value\n");
    for (double d = 0.0; d <= max_dist; d += max_dist / 100.0) {
        double f_value = 0.0;

#pragma omp parallel for reduction(+ : f_value)
        for (int i = 0; i < num_random_points; i++) {
            if (distances[i] <= d) {
                f_value += 1;
            }
        }

        f_value /= num_random_points;
        fprintf(fp, "%f,%f\n", d, f_value);
    }

    free(random_points);
    free(distances);
    fclose(fp);
}

/**
 * Calculates the G-Function for a given set of points and writes the results to
 * an output file.
 *
 * @param kdtree The k-d tree data structure used for nearest neighbor search.
 * @param points An array of Point structures representing the input points.
 * @param n The number of points in the input array.
 * @param i The intensity of the point being processed.
 * @param output_file The path to the output file where the results will be
 * written.
 */
void calculate_g_function(struct kdtree *kdtree, struct Point *points, int n,
                          int i, const char *output_file)
{
    G_message(_("G-Function"));
    FILE *fp = fopen(output_file, "w");
    if (fp == NULL) {
        G_fatal_error(_("Unable to open output file <%s>"), output_file);
    }

    double max_dist = 0.0;
    double *nearest_distances = (double *)malloc(n * sizeof(double));

#pragma omp parallel for reduction(max : max_dist)
    for (int i = 0; i < n; i++) {
        double coords[2] = {points[i].x, points[i].y};
        int puid = 0;
        double pd = 0.0;
        kdtree_knn(
            kdtree, coords, &puid, &pd, 2,
            &points[i]
                 .id); // Find the nearest neighbor excluding the point itself

        nearest_distances[i] = sqrt(pd);
        if (nearest_distances[i] > max_dist) {
            max_dist = nearest_distances[i];
        }
    }
    G_message(_("Max distance: %f"), max_dist);
    double g_value_csr = 0.0;
    fprintf(fp, "Distance,G-value,G-value-CSR\n");
    for (double d = 0.0; d <= max_dist; d += max_dist / 100) {
        double g_value = 0.0;

#pragma omp parallel for reduction(+ : g_value)
        for (int i = 0; i < n; i++) {
            if (nearest_distances[i] <= d) {
                g_value += 1.0;
            }
        }
        g_value_csr = csr_g_value(d, i);
        g_value /= n; // Normalize g-value
        fprintf(fp, "%f,%f,%f\n", d, g_value, g_value_csr);
    }

    free(nearest_distances);
    fclose(fp);
}

void calculate_k_function(struct kdtree *kdtree, struct Point *points, int n,
                          int num_distances, double intensity,
                          const char *output_file, enum OutputFormat format,
                          JSON_Object *root_object)
{
    double max_dist = max_distance(points, n);
    double interval = max_dist / num_distances;
    G_message(_("Max distance: %f"), max_dist);
    double *values = (double *)malloc(num_distances * sizeof(double));
    calculate_k_function_values(kdtree, points, n, num_distances, max_dist,
                                intensity, values);

    switch (format) {
    case PLAIN:
        FILE *fp = fopen(output_file, "w");
        if (fp == NULL) {
            G_fatal_error(_("Unable to open output file <%s>"), output_file);
        }

        fprintf(fp, "Distance,K-value\n");
        for (int d = 0; d < num_distances; d++) {
            fprintf(fp, "%f,%f\n", d * interval, values[d]);
        }
        fclose(fp);
        break;
    case JSON:
        JSON_Value *distance = json_value_init_array();
        if (distance == NULL) {
            G_fatal_error(_("Unable to initialize JSON distance array."));
        }
        JSON_Array *distances = json_array(distance);

        JSON_Value *k_value = json_value_init_array();
        if (k_value == NULL) {
            G_fatal_error(_("Unable to initialize JSON k-value array."));
        }
        JSON_Array *k_values = json_array(k_value);

        for (int d = 0; d < num_distances; d++) {
            json_array_append_number(distances, d * interval);
            json_array_append_number(k_values, values[d]);
        }
        json_object_set_value(root_object, "distance", distance);
        json_object_set_value(root_object, "k-value", k_value);
        break;
    }
    free(values);
}

void calculate_g_function_values(struct kdtree *kdtree, struct Point *points,
                                 int n, int num_distances, double *values)
{
    double max_dist = 0.0;
    double *nearest_distances = (double *)malloc(n * sizeof(double));

// Calculate the nearest neighbor distances for each point and find the maximum
// distance
#pragma omp parallel for reduction(max : max_dist)
    for (int i = 0; i < n; i++) {
        double coords[2] = {points[i].x, points[i].y};
        int puid = 0;
        double pd = 0.0;
        kdtree_knn(
            kdtree, coords, &puid, &pd, 2,
            &points[i]
                 .id); // Find the nearest neighbor excluding the point itself

        nearest_distances[i] = sqrt(pd);
        if (nearest_distances[i] > max_dist) {
            max_dist = nearest_distances[i];
        }
    }

    double interval = max_dist / num_distances;

    // Initialize the G-values array
    for (int d = 0; d < num_distances; d++) {
        values[d] = 0.0;
    }

    // Calculate G-values for the specified distances
    for (int d = 0; d < num_distances; d++) {
        double dist = d * interval;
        double g_value = 0.0;

#pragma omp parallel for reduction(+ : g_value)
        for (int i = 0; i < n; i++) {
            if (nearest_distances[i] <= dist) {
                g_value += 1.0;
            }
        }
        values[d] = g_value / n;
    }

    free(nearest_distances);
}

// You need to implement these placeholder functions
void calculate_k_function_values(struct kdtree *kdtree, struct Point *points,
                                 int n, int num_distances, double max_dist,
                                 double intensity, double *values)
{

    double interval = max_dist / num_distances;

    G_percent(0, num_distances, 1);
#pragma omp parallel for
    for (int d = 0; d < num_distances; d++) {
        double k_value = 0.0;
        double radius = d * interval;

#pragma omp parallel for reduction(+ : k_value)
        for (int i = 0; i < n; i++) {
            double coords[2] = {points[i].x, points[i].y};
            int count;
            int *puid = NULL;
            double *pd = NULL;
            count =
                kdtree_dnn(kdtree, coords, &puid, &pd, radius, &points[i].id);

            if (count > 0) {
                k_value += count - 1; // subtract 1 to exclude the point itself
            }

            free(puid);
            free(pd);
        }

        k_value /= (n * intensity);
        values[d] = k_value;

        // Update progress indicator
        G_percent(d, num_distances, 1);
    }

    // Finalize the progress indicator
    G_percent(num_distances, num_distances, 1);
}

void calculate_l_function_values(struct kdtree *kdtree, struct Point *points,
                                 int n, int num_distances, double max_dist,
                                 double intensity, double *values)
{

    calculate_k_function_values(kdtree, points, n, num_distances, max_dist,
                                intensity, values);
#pragma omp parallel for
    for (int i = 0; i < num_distances; i++) {
        values[i] = sqrt(values[i] / M_PI);
    }
}

void calculate_f_function_values(struct kdtree *kdtree, struct Point *points,
                                 int n, struct bound_box *box, double *values,
                                 int num_random_points)
{

    // Generate random points
    struct Point *random_points =
        (struct Point *)malloc(num_random_points * sizeof(struct Point));

    generate_random_points(random_points, num_random_points, box);

    double max_dist = 0.0;
    double *distances = (double *)malloc(num_random_points * sizeof(double));

#pragma omp parallel for reduction(max : max_dist)
    for (int i = 0; i < num_random_points; i++) {
        double coords[2] = {random_points[i].x, random_points[i].y};
        int puid;
        double pd;
        kdtree_knn(kdtree, coords, &puid, &pd, 1, NULL);
        distances[i] = sqrt(pd);
        if (distances[i] > max_dist) {
            max_dist = distances[i];
        }
    }

    for (int d = 0.0; d <= max_dist; d += max_dist / 100.0) {
        double f_value = 0.0;

#pragma omp parallel for reduction(+ : f_value)
        for (int i = 0; i < num_random_points; i++) {
            if (distances[i] <= d) {
                f_value += 1;
            }
        }

        f_value /= num_random_points;
        values[d] = f_value;
    }

    free(random_points);
    free(distances);
}

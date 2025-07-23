/*
 *  Calculates univariate statistics from the non-null cells
 *
 *   Copyright (C) 2004-2007 by the GRASS Development Team
 *   Author(s): Hamish Bowman, University of Otago, New Zealand
 *              Martin Landa and Soeren Gebbert
 *
 *      This program is free software under the GNU General Public
 *      License (>=v2). Read the file COPYING that comes with GRASS
 *      for details.
 *
 */

#include <grass/parson.h>
#include "globals.h"

/* *************************************************************** */
/* **** univar_stat constructor ********************************** */
/* *************************************************************** */
univar_stat *create_univar_stat_struct(int map_type, int n_perc)
{
    univar_stat *stats;
    int i;
    int n_zones = zone_info.n_zones;

    if (n_zones == 0)
        n_zones = 1;

    stats = (univar_stat *)G_calloc(n_zones, sizeof(univar_stat));

    for (i = 0; i < n_zones; i++) {
        stats[i].sum = 0.0;
        stats[i].sumsq = 0.0;
        stats[i].min = NAN;
        stats[i].max = NAN;
        stats[i].n_perc = n_perc;
        if (n_perc > 0)
            stats[i].perc = (double *)G_malloc(n_perc * sizeof(double));
        else
            stats[i].perc = NULL;
        stats[i].sum_abs = 0.0;
        stats[i].n = 0;
        stats[i].size = 0;
        stats[i].dcell_array = NULL;
        stats[i].fcell_array = NULL;
        stats[i].cell_array = NULL;
        stats[i].map_type = map_type;
        stats[i].n_alloc = 0;
        stats[i].first = TRUE;
    }

    return stats;
}

/* *************************************************************** */
/* **** univar_stat destructor *********************************** */
/* *************************************************************** */
void free_univar_stat_struct(univar_stat *stats)
{
    int i;
    int n_zones = zone_info.n_zones;

    if (n_zones == 0)
        n_zones = 1;

    for (i = 0; i < n_zones; i++) {
        if (stats[i].perc)
            G_free(stats[i].perc);
        if (stats[i].dcell_array)
            G_free(stats[i].dcell_array);
        if (stats[i].fcell_array)
            G_free(stats[i].fcell_array);
        if (stats[i].cell_array)
            G_free(stats[i].cell_array);
    }

    G_free(stats);

    return;
}

/* *************************************************************** */
/* **** compute and print univar statistics to stdout ************ */
/* *************************************************************** */
int print_stats(univar_stat *stats, enum OutputFormat format)
{
    JSON_Value *root_value = NULL, *zone_value = NULL;
    JSON_Array *root_array = NULL;
    JSON_Object *zone_object = NULL;

    if (format == JSON) {
        if (zone_info.n_zones) {
            root_value = json_value_init_array();
            if (root_value == NULL) {
                G_fatal_error(
                    _("Failed to initialize JSON array. Out of memory?"));
            }
            root_array = json_array(root_value);
        }
        else {
            zone_value = json_value_init_object();
            if (zone_value == NULL) {
                G_fatal_error(
                    _("Failed to initialize JSON object. Out of memory?"));
            }
            zone_object = json_object(zone_value);
        }
    }

    int z, n_zones = zone_info.n_zones;

    if (n_zones == 0)
        n_zones = 1;

    for (z = 0; z < n_zones; z++) {
        char sum_str[100];
        double mean, variance, stdev, var_coef;

        /* for extended stats */
        double quartile_25 = 0.0, quartile_75 = 0.0, *quartile_perc;
        double median = 0.0;
        unsigned int i;
        size_t qpos_25, qpos_75, *qpos_perc;

        /* stats collected for this zone? */
        if (stats[z].size == 0)
            continue;

        /* all these calculations get promoted to doubles, so any DIV0 becomes
         * nan */
        mean = stats[z].sum / stats[z].n;
        variance = (stats[z].sumsq - stats[z].sum * stats[z].sum / stats[z].n) /
                   stats[z].n;
        if (variance < GRASS_EPSILON)
            variance = 0.0;
        stdev = sqrt(variance);
        var_coef = (stdev / mean) * 100.; /* perhaps stdev/fabs(mean) ? */

        if (stats[z].n == 0)
            stats[z].sum = stats[z].sum_abs = NAN;
        snprintf(sum_str, sizeof(sum_str), "%.15g", stats[z].sum);
        G_trim_decimal(sum_str);

        if (format == JSON && zone_info.n_zones) {
            zone_value = json_value_init_object();
            if (zone_value == NULL) {
                G_fatal_error(
                    _("Failed to initialize JSON object. Out of memory?"));
            }
            zone_object = json_object(zone_value);
        }
        if (zone_info.n_zones) {
            int z_cat = z + zone_info.min;

            switch (format) {
            case PLAIN:
                fprintf(stdout, "\nzone %d %s\n\n", z_cat,
                        Rast_get_c_cat(&z_cat, &(zone_info.cats)));
                break;
            case SHELL:
                fprintf(stdout, "zone=%d;%s\n", z_cat,
                        Rast_get_c_cat(&z_cat, &(zone_info.cats)));
                break;
            case JSON:
                json_object_set_number(zone_object, "zone", z_cat);
                json_object_set_string(
                    zone_object, "zone_label",
                    Rast_get_c_cat(&z_cat, &(zone_info.cats)));
                break;
            case CSV:
                /* already addressed in print_stats_table */
                break;
            }
        }
        switch (format) {
        case PLAIN:
            fprintf(stdout, "total null and non-null cells: %zu\n",
                    stats[z].size);
            fprintf(stdout, "total null cells: %zu\n\n",
                    stats[z].size - stats[z].n);
            fprintf(stdout, "Of the non-null cells:\n----------------------\n");

            fprintf(stdout, "n: %zu\n", stats[z].n);
            fprintf(stdout, "minimum: %g\n", stats[z].min);
            fprintf(stdout, "maximum: %g\n", stats[z].max);
            fprintf(stdout, "range: %g\n", stats[z].max - stats[z].min);
            fprintf(stdout, "mean: %g\n", mean);
            fprintf(stdout, "mean of absolute values: %g\n",
                    stats[z].sum_abs / stats[z].n);
            fprintf(stdout, "standard deviation: %g\n", stdev);
            fprintf(stdout, "variance: %g\n", variance);
            fprintf(stdout, "variation coefficient: %g %%\n", var_coef);
            fprintf(stdout, "sum: %s\n", sum_str);
            break;
        case SHELL:
            fprintf(stdout, "n=%zu\n", stats[z].n);
            fprintf(stdout, "null_cells=%zu\n", stats[z].size - stats[z].n);
            fprintf(stdout, "cells=%zu\n", stats[z].size);
            fprintf(stdout, "min=%.15g\n", stats[z].min);
            fprintf(stdout, "max=%.15g\n", stats[z].max);
            fprintf(stdout, "range=%.15g\n", stats[z].max - stats[z].min);
            fprintf(stdout, "mean=%.15g\n", mean);
            fprintf(stdout, "mean_of_abs=%.15g\n",
                    stats[z].sum_abs / stats[z].n);
            fprintf(stdout, "stddev=%.15g\n", stdev);
            fprintf(stdout, "variance=%.15g\n", variance);
            fprintf(stdout, "coeff_var=%.15g\n", var_coef);
            fprintf(stdout, "sum=%s\n", sum_str);
            break;
        case JSON:
            json_object_set_number(zone_object, "n", stats[z].n);
            json_object_set_number(zone_object, "null_cells",
                                   stats[z].size - stats[z].n);
            json_object_set_number(zone_object, "cells", stats[z].size);

            if (isfinite(stats[z].min))
                json_object_set_number(zone_object, "min", stats[z].min);
            else
                json_object_set_null(zone_object, "min");

            if (isfinite(stats[z].max))
                json_object_set_number(zone_object, "max", stats[z].max);
            else
                json_object_set_null(zone_object, "max");

            if (isfinite(stats[z].max - stats[z].min))
                json_object_set_number(zone_object, "range",
                                       stats[z].max - stats[z].min);
            else
                json_object_set_null(zone_object, "range");

            if (isfinite(mean))
                json_object_set_number(zone_object, "mean", mean);
            else
                json_object_set_null(zone_object, "mean");

            if (isfinite(stats[z].sum_abs / stats[z].n))
                json_object_set_number(zone_object, "mean_of_abs",
                                       stats[z].sum_abs / stats[z].n);
            else
                json_object_set_null(zone_object, "mean_of_abs");

            if (isfinite(stdev))
                json_object_set_number(zone_object, "stddev", stdev);
            else
                json_object_set_null(zone_object, "stddev");

            if (isfinite(variance))
                json_object_set_number(zone_object, "variance", variance);
            else
                json_object_set_null(zone_object, "variance");

            if (isfinite(var_coef))
                json_object_set_number(zone_object, "coeff_var", var_coef);
            else
                json_object_set_null(zone_object, "coeff_var");

            if (isfinite(stats[z].sum))
                json_object_set_number(zone_object, "sum", stats[z].sum);
            else
                json_object_set_null(zone_object, "sum");

            break;
        case CSV:
            /* already addressed in print_stats_table */
            break;
        }

        /* TODO: mode, skewness, kurtosis */
        if (param.extended->answer) {
            qpos_perc = (size_t *)G_calloc(stats[z].n_perc, sizeof(size_t));
            quartile_perc = (double *)G_calloc(stats[z].n_perc, sizeof(double));

            if (stats[z].n == 0) {
                quartile_25 = median = quartile_75 = NAN;
                for (i = 0; i < stats[z].n_perc; i++)
                    quartile_perc[i] = NAN;
            }
            else {
                for (i = 0; i < stats[z].n_perc; i++) {
                    qpos_perc[i] =
                        (size_t)(stats[z].n * 1e-2 * stats[z].perc[i] - 0.5);
                }
                qpos_25 = (size_t)(stats[z].n * 0.25 - 0.5);
                qpos_75 = (size_t)(stats[z].n * 0.75 - 0.5);

                switch (stats[z].map_type) {
                case CELL_TYPE:
                    heapsort_int(stats[z].cell_array, stats[z].n);

                    quartile_25 = (double)stats[z].cell_array[qpos_25];
                    if (stats[z].n % 2) /* odd */
                        median =
                            (double)stats[z].cell_array[(int)(stats[z].n / 2)];
                    else /* even */
                        median =
                            (double)(stats[z].cell_array[stats[z].n / 2 - 1] +
                                     stats[z].cell_array[stats[z].n / 2]) /
                            2.0;
                    quartile_75 = (double)stats[z].cell_array[qpos_75];
                    for (i = 0; i < stats[z].n_perc; i++) {
                        quartile_perc[i] =
                            (double)stats[z].cell_array[qpos_perc[i]];
                    }
                    break;

                case FCELL_TYPE:
                    heapsort_float(stats[z].fcell_array, stats[z].n);

                    quartile_25 = (double)stats[z].fcell_array[qpos_25];
                    if (stats[z].n % 2) /* odd */
                        median =
                            (double)stats[z].fcell_array[(int)(stats[z].n / 2)];
                    else /* even */
                        median =
                            (double)(stats[z].fcell_array[stats[z].n / 2 - 1] +
                                     stats[z].fcell_array[stats[z].n / 2]) /
                            2.0;
                    quartile_75 = (double)stats[z].fcell_array[qpos_75];
                    for (i = 0; i < stats[z].n_perc; i++) {
                        quartile_perc[i] =
                            (double)stats[z].fcell_array[qpos_perc[i]];
                    }
                    break;

                case DCELL_TYPE:
                    heapsort_double(stats[z].dcell_array, stats[z].n);

                    quartile_25 = stats[z].dcell_array[qpos_25];
                    if (stats[z].n % 2) /* odd */
                        median = stats[z].dcell_array[(int)(stats[z].n / 2)];
                    else /* even */
                        median = (stats[z].dcell_array[stats[z].n / 2 - 1] +
                                  stats[z].dcell_array[stats[z].n / 2]) /
                                 2.0;
                    quartile_75 = stats[z].dcell_array[qpos_75];
                    for (i = 0; i < stats[z].n_perc; i++) {
                        quartile_perc[i] = stats[z].dcell_array[qpos_perc[i]];
                    }
                    break;

                default:
                    break;
                }
            }

            switch (format) {
            case PLAIN:
                fprintf(stdout, "1st quartile: %g\n", quartile_25);
                if (stats[z].n % 2)
                    fprintf(stdout, "median (odd number of cells): %g\n",
                            median);
                else
                    fprintf(stdout, "median (even number of cells): %g\n",
                            median);
                fprintf(stdout, "3rd quartile: %g\n", quartile_75);
                break;
            case SHELL:
                fprintf(stdout, "first_quartile=%g\n", quartile_25);
                fprintf(stdout, "median=%g\n", median);
                fprintf(stdout, "third_quartile=%g\n", quartile_75);
                break;
            case JSON:
                if (isfinite(quartile_25))
                    json_object_set_number(zone_object, "first_quartile",
                                           quartile_25);
                else
                    json_object_set_null(zone_object, "first_quartile");

                if (isfinite(median))
                    json_object_set_number(zone_object, "median", median);
                else
                    json_object_set_null(zone_object, "median");

                if (isfinite(quartile_75))
                    json_object_set_number(zone_object, "third_quartile",
                                           quartile_75);
                else
                    json_object_set_null(zone_object, "third_quartile");

                break;
            case CSV:
                /* already addressed in print_stats_table */
                break;
            }

            JSON_Value *percentiles_array_value = NULL,
                       *percentile_value = NULL;
            JSON_Array *percentiles_array = NULL;
            JSON_Object *percentile_object = NULL;

            if (format == JSON) {
                percentiles_array_value = json_value_init_array();
                if (percentiles_array_value == NULL) {
                    G_fatal_error(
                        _("Failed to initialize JSON array. Out of memory?"));
                }
                percentiles_array = json_array(percentiles_array_value);
            }

            for (i = 0; i < stats[z].n_perc; i++) {
                char buf[24];

                snprintf(buf, sizeof(buf), "%.15g", stats[z].perc[i]);
                G_strchg(buf, '.', '_');
                switch (format) {
                case PLAIN:
                    if (stats[z].perc[i] == (int)stats[z].perc[i]) {
                        /* percentile is an exact integer */
                        if ((int)stats[z].perc[i] % 10 == 1 &&
                            (int)stats[z].perc[i] != 11)
                            fprintf(stdout, "%dst percentile: %g\n",
                                    (int)stats[z].perc[i], quartile_perc[i]);
                        else if ((int)stats[z].perc[i] % 10 == 2 &&
                                 (int)stats[z].perc[i] != 12)
                            fprintf(stdout, "%dnd percentile: %g\n",
                                    (int)stats[z].perc[i], quartile_perc[i]);
                        else if ((int)stats[z].perc[i] % 10 == 3 &&
                                 (int)stats[z].perc[i] != 13)
                            fprintf(stdout, "%drd percentile: %g\n",
                                    (int)stats[z].perc[i], quartile_perc[i]);
                        else
                            fprintf(stdout, "%dth percentile: %g\n",
                                    (int)stats[z].perc[i], quartile_perc[i]);
                    }
                    else {
                        /* percentile is not an exact integer */
                        fprintf(stdout, "%.15g percentile: %g\n",
                                stats[z].perc[i], quartile_perc[i]);
                    }
                    break;
                case SHELL:
                    fprintf(stdout, "percentile_%s=%g\n", buf,
                            quartile_perc[i]);
                    break;
                case JSON:
                    percentile_value = json_value_init_object();
                    if (percentile_value == NULL) {
                        G_fatal_error(_("Failed to initialize JSON object. "
                                        "Out of memory?"));
                    }
                    percentile_object = json_object(percentile_value);

                    if (isfinite(stats[z].perc[i]))
                        json_object_set_number(percentile_object, "percentile",
                                               stats[z].perc[i]);
                    else
                        json_object_set_null(percentile_object, "percentile");

                    if (isfinite(quartile_perc[i]))
                        json_object_set_number(percentile_object, "value",
                                               quartile_perc[i]);
                    else
                        json_object_set_null(percentile_object, "value");

                    json_array_append_value(percentiles_array,
                                            percentile_value);
                    break;
                case CSV:
                    /* already addressed in print_stats_table */
                    break;
                }
            }

            if (format == JSON) {
                json_object_set_value(zone_object, "percentiles",
                                      percentiles_array_value);
            }

            G_free((void *)quartile_perc);
            G_free((void *)qpos_perc);
        }

        /* G_message() prints to stderr not stdout: disabled. this \n is printed
         * above with zone */
        /* if (!(param.shell_style->answer))
           G_message("\n"); */
        if (format == JSON && zone_info.n_zones) {
            json_array_append_value(root_array, zone_value);
        }
    }

    if (format == JSON) {
        char *serialized_string = NULL;
        if (zone_info.n_zones) {
            serialized_string = json_serialize_to_string_pretty(root_value);
        }
        else {
            serialized_string = json_serialize_to_string_pretty(zone_value);
        }

        if (serialized_string == NULL) {
            G_fatal_error(_("Failed to initialize pretty JSON string."));
        }
        puts(serialized_string);
        json_free_serialized_string(serialized_string);

        if (zone_info.n_zones) {
            json_value_free(root_value);
        }
        else {
            json_value_free(zone_value);
        }
    }

    return 1;
}

int print_stats_table(univar_stat *stats)
{
    unsigned int i;
    int z, n_zones = zone_info.n_zones;

    if (n_zones == 0)
        n_zones = 1;

    /* print column headers */

    if (zone_info.n_zones) {
        fprintf(stdout, "zone%s", zone_info.sep);
        fprintf(stdout, "label%s", zone_info.sep);
    }
    fprintf(stdout, "non_null_cells%s", zone_info.sep);
    fprintf(stdout, "null_cells%s", zone_info.sep);
    fprintf(stdout, "min%s", zone_info.sep);
    fprintf(stdout, "max%s", zone_info.sep);
    fprintf(stdout, "range%s", zone_info.sep);
    fprintf(stdout, "mean%s", zone_info.sep);
    fprintf(stdout, "mean_of_abs%s", zone_info.sep);
    fprintf(stdout, "stddev%s", zone_info.sep);
    fprintf(stdout, "variance%s", zone_info.sep);
    fprintf(stdout, "coeff_var%s", zone_info.sep);
    fprintf(stdout, "sum%s", zone_info.sep);
    fprintf(stdout, "sum_abs");

    if (param.extended->answer) {
        fprintf(stdout, "%sfirst_quart", zone_info.sep);
        fprintf(stdout, "%smedian", zone_info.sep);
        fprintf(stdout, "%sthird_quart", zone_info.sep);
        for (i = 0; i < stats[0].n_perc; i++) {

            if (stats[0].perc[i] == (int)stats[0].perc[i]) {
                /* percentile is an exact integer */
                fprintf(stdout, "%sperc_%d", zone_info.sep,
                        (int)stats[0].perc[i]);
            }
            else {
                /* percentile is not an exact integer */
                char buf[24];

                snprintf(buf, sizeof(buf), "%.15g", stats[0].perc[i]);
                G_strchg(buf, '.', '_');
                fprintf(stdout, "%sperc_%s", zone_info.sep, buf);
            }
        }
    }
    fprintf(stdout, "\n");

    /* print stats */

    for (z = 0; z < n_zones; z++) {
        char sum_str[100];
        double mean, variance, stdev, var_coef;

        /* for extended stats */
        double quartile_25 = 0.0, quartile_75 = 0.0, *quartile_perc;
        double median = 0.0;
        int qpos_25, qpos_75, *qpos_perc;

        /* stats collected for this zone? */
        if (stats[z].size == 0)
            continue;

        i = 0;

        /* all these calculations get promoted to doubles, so any DIV0 becomes
         * nan */
        mean = stats[z].sum / stats[z].n;
        variance = (stats[z].sumsq - stats[z].sum * stats[z].sum / stats[z].n) /
                   stats[z].n;
        if (variance < GRASS_EPSILON)
            variance = 0.0;
        stdev = sqrt(variance);
        var_coef = (stdev / mean) * 100.; /* perhaps stdev/fabs(mean) ? */

        if (stats[z].n == 0)
            stats[z].sum = stats[z].sum_abs = NAN;

        if (zone_info.n_zones) {
            int z_cat = z + zone_info.min;

            /* zone number */
            fprintf(stdout, "%d%s", z + zone_info.min, zone_info.sep);
            /* zone label */
            fprintf(stdout, "%s%s", Rast_get_c_cat(&z_cat, &(zone_info.cats)),
                    zone_info.sep);
        }

        /* non-null cells cells */
        fprintf(stdout, "%zu%s", stats[z].n, zone_info.sep);
        /* null cells */
        fprintf(stdout, "%zu%s", stats[z].size - stats[z].n, zone_info.sep);
        /* min */
        fprintf(stdout, "%.15g%s", stats[z].min, zone_info.sep);
        /* max */
        fprintf(stdout, "%.15g%s", stats[z].max, zone_info.sep);
        /* range */
        fprintf(stdout, "%.15g%s", stats[z].max - stats[z].min, zone_info.sep);
        /* mean */
        fprintf(stdout, "%.15g%s", mean, zone_info.sep);
        /* mean of abs */
        fprintf(stdout, "%.15g%s", stats[z].sum_abs / stats[z].n,
                zone_info.sep);
        /* stddev */
        fprintf(stdout, "%.15g%s", stdev, zone_info.sep);
        /* variance */
        fprintf(stdout, "%.15g%s", variance, zone_info.sep);
        /* coefficient of variance */
        fprintf(stdout, "%.15g%s", var_coef, zone_info.sep);
        /* sum */
        snprintf(sum_str, sizeof(sum_str), "%.15g", stats[z].sum);
        G_trim_decimal(sum_str);
        fprintf(stdout, "%s%s", sum_str, zone_info.sep);
        /* absolute sum */
        snprintf(sum_str, sizeof(sum_str), "%.15g", stats[z].sum_abs);
        G_trim_decimal(sum_str);
        fprintf(stdout, "%s", sum_str);

        /* TODO: mode, skewness, kurtosis */
        if (param.extended->answer) {
            qpos_perc = (int *)G_calloc(stats[z].n_perc, sizeof(int));
            quartile_perc = (double *)G_calloc(stats[z].n_perc, sizeof(double));

            if (stats[z].n == 0) {
                quartile_25 = median = quartile_75 = NAN;
                for (i = 0; i < stats[z].n_perc; i++)
                    quartile_perc[i] = NAN;
            }
            else {
                for (i = 0; i < stats[z].n_perc; i++) {
                    qpos_perc[i] =
                        (int)(stats[z].n * 1e-2 * stats[z].perc[i] - 0.5);
                }
                qpos_25 = (int)(stats[z].n * 0.25 - 0.5);
                qpos_75 = (int)(stats[z].n * 0.75 - 0.5);

                switch (stats[z].map_type) {
                case CELL_TYPE:
                    heapsort_int(stats[z].cell_array, stats[z].n);

                    quartile_25 = (double)stats[z].cell_array[qpos_25];
                    if (stats[z].n % 2) /* odd */
                        median =
                            (double)stats[z].cell_array[(int)(stats[z].n / 2)];
                    else /* even */
                        median =
                            (double)(stats[z].cell_array[stats[z].n / 2 - 1] +
                                     stats[z].cell_array[stats[z].n / 2]) /
                            2.0;
                    quartile_75 = (double)stats[z].cell_array[qpos_75];
                    for (i = 0; i < stats[z].n_perc; i++) {
                        quartile_perc[i] =
                            (double)stats[z].cell_array[qpos_perc[i]];
                    }
                    break;

                case FCELL_TYPE:
                    heapsort_float(stats[z].fcell_array, stats[z].n);

                    quartile_25 = (double)stats[z].fcell_array[qpos_25];
                    if (stats[z].n % 2) /* odd */
                        median =
                            (double)stats[z].fcell_array[(int)(stats[z].n / 2)];
                    else /* even */
                        median =
                            (double)(stats[z].fcell_array[stats[z].n / 2 - 1] +
                                     stats[z].fcell_array[stats[z].n / 2]) /
                            2.0;
                    quartile_75 = (double)stats[z].fcell_array[qpos_75];
                    for (i = 0; i < stats[z].n_perc; i++) {
                        quartile_perc[i] =
                            (double)stats[z].fcell_array[qpos_perc[i]];
                    }
                    break;

                case DCELL_TYPE:
                    heapsort_double(stats[z].dcell_array, stats[z].n);

                    quartile_25 = stats[z].dcell_array[qpos_25];
                    if (stats[z].n % 2) /* odd */
                        median = stats[z].dcell_array[(int)(stats[z].n / 2)];
                    else /* even */
                        median = (stats[z].dcell_array[stats[z].n / 2 - 1] +
                                  stats[z].dcell_array[stats[z].n / 2]) /
                                 2.0;
                    quartile_75 = stats[z].dcell_array[qpos_75];
                    for (i = 0; i < stats[z].n_perc; i++) {
                        quartile_perc[i] = stats[z].dcell_array[qpos_perc[i]];
                    }
                    break;

                default:
                    break;
                }
            }

            /* first quartile */
            fprintf(stdout, "%s%g", zone_info.sep, quartile_25);
            /* median */
            fprintf(stdout, "%s%g", zone_info.sep, median);
            /* third quartile */
            fprintf(stdout, "%s%g", zone_info.sep, quartile_75);
            /* percentiles */
            for (i = 0; i < stats[z].n_perc; i++) {
                fprintf(stdout, "%s%g", zone_info.sep, quartile_perc[i]);
            }

            G_free((void *)quartile_perc);
            G_free((void *)qpos_perc);
        }

        fprintf(stdout, "\n");

        /* zone z finished */
    }

    return 1;
}

univar_stat *univar_stat_with_percentiles(int map_type)
{
    univar_stat *stats;
    unsigned int i, j;
    unsigned int n_zones = zone_info.n_zones;

    if (n_zones == 0)
        n_zones = 1;

    i = 0;
    while (param.percentile->answers[i])
        i++;
    stats = create_univar_stat_struct(map_type, i);
    for (i = 0; i < n_zones; i++) {
        for (j = 0; j < stats[i].n_perc; j++) {
            sscanf(param.percentile->answers[j], "%lf", &(stats[i].perc[j]));
        }
    }

    return stats;
}

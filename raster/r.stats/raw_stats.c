#include <stdlib.h>
#include <grass/gjson.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include "global.h"

int raw_stats(int fd[], int with_coordinates, int with_xy, int with_labels,
              enum OutputFormat format, G_JSON_Array *array)
{
    CELL null_cell;
    void **rast, **rastp;
    char str1[50];
    register int i;
    int row, col, nulls_found;
    struct Cell_head window;
    char nbuf[100], ebuf[100];
    RASTER_MAP_TYPE *map_type;
    G_JSON_Array *categories;
    G_JSON_Object *object, *category;
    G_JSON_Value *categories_value, *object_value, *category_value;

    /* allocate i/o buffers for each raster map */
    rast = (void **)G_calloc(nfiles, sizeof(void *));
    rastp = (void **)G_calloc(nfiles, sizeof(void *));

    map_type = (RASTER_MAP_TYPE *)G_calloc(nfiles, sizeof(RASTER_MAP_TYPE));

    for (i = 0; i < nfiles; i++) {
        /* if fp map and report real data, not cat indexes, set type to DCELL */
        if (is_fp[i] && !raw_output && !as_int)
            map_type[i] = Rast_get_map_type(fd[i]);
        else
            map_type[i] = CELL_TYPE;

        rast[i] = Rast_allocate_buf(map_type[i]);
    }

    /* get window */
    if (with_coordinates)
        G_get_set_window(&window);

    if (format == CSV) {
        /* CSV Header */
        if (with_coordinates)
            fprintf(stdout, "%s%s%s%s", "east", fs, "north", fs);
        if (with_xy)
            fprintf(stdout, "%s%s%s%s", "col", fs, "row", fs);

        for (i = 0; i < nfiles; i++) {
            fprintf(stdout, "%s%s_%s", i ? fs : "", map_names[i], "cat");
            if (with_labels)
                fprintf(stdout, "%s%s_%s", fs, map_names[i], "label");
        }
        fprintf(stdout, "\n");
    }

    /* here we go */
    Rast_set_c_null_value(&null_cell, 1);
    for (row = 0; row < nrows; row++) {
        G_percent(row, nrows, 2);

        /* read the rows and set the pointers */
        for (i = 0; i < nfiles; i++) {
            Rast_get_row(fd[i], rast[i], row, map_type[i]);
            rastp[i] = rast[i];
        }

        double northing;
        if (with_coordinates) {
            northing = Rast_row_to_northing(row + .5, &window);
            G_format_northing(northing, nbuf,
                              G_projection() == PROJECTION_LL ? -1 : 0);
        }

        for (col = 0; col < ncols; col++) {
            if (format == JSON) {
                object_value = G_json_value_init_object();
                object = G_json_object(object_value);
                categories_value = G_json_value_init_array();
                categories = G_json_array(categories_value);
            }
            if (no_nulls || no_nulls_all) {
                nulls_found = 0;
                for (i = 0; i < nfiles; i++) {
                    /*
                       Rast_set_d_value(zero_val, 0.0, map_type[i]);
                       if (Rast_raster_cmp(rastp[i], zero_val, map_type[i]) !=
                       0) break;
                     */
                    if (Rast_is_null_value(rastp[i], map_type[i]))
                        nulls_found++;
                }

                if ((nulls_found == nfiles) || (nulls_found && no_nulls)) {
                    for (i = 0; i < nfiles; i++)
                        rastp[i] = G_incr_void_ptr(rastp[i],
                                                   Rast_cell_size(map_type[i]));
                    continue;
                }
            }
            switch (format) {
            case JSON:
                if (with_coordinates) {
                    G_json_object_set_number(
                        object, "east", Rast_col_to_easting(col + .5, &window));
                    G_json_object_set_number(object, "north", northing);
                }
                if (with_xy) {
                    G_json_object_set_number(object, "col", col + 1);
                    G_json_object_set_number(object, "row", row + 1);
                }
                break;
            case CSV:
            case PLAIN:
                if (with_coordinates) {
                    G_format_easting(Rast_col_to_easting(col + .5, &window),
                                     ebuf,
                                     G_projection() == PROJECTION_LL ? -1 : 0);
                    fprintf(stdout, "%s%s%s%s", ebuf, fs, nbuf, fs);
                }
                if (with_xy)
                    fprintf(stdout, "%d%s%d%s", col + 1, fs, row + 1, fs);
                break;
            }

            for (i = 0; i < nfiles; i++) {
                if (format == JSON) {
                    category_value = G_json_value_init_object();
                    category = G_json_object(category_value);
                }

                if (Rast_is_null_value(rastp[i], map_type[i])) {
                    switch (format) {
                    case JSON:
                        G_json_object_set_null(category, "category");
                        if (with_labels)
                            G_json_object_set_string(
                                category, "label",
                                Rast_get_c_cat(&null_cell, &labels[i]));
                        break;
                    case CSV:
                    case PLAIN:
                        fprintf(stdout, "%s%s", i ? fs : "", no_data_str);
                        if (with_labels)
                            fprintf(stdout, "%s%s", fs,
                                    Rast_get_c_cat(&null_cell, &labels[i]));
                        break;
                    }
                }
                else if (map_type[i] == CELL_TYPE) {
                    switch (format) {
                    case JSON:
                        G_json_object_set_number(category, "category",
                                                 (long)*((CELL *)rastp[i]));
                        if (with_labels && !is_fp[i]) {
                            G_json_object_set_string(
                                category, "label",
                                Rast_get_c_cat((CELL *)rastp[i], &labels[i]));
                        }
                        break;
                    case CSV:
                        fprintf(stdout, "%s%ld", i ? fs : "",
                                (long)*((CELL *)rastp[i]));
                        if (with_labels)
                            fprintf(stdout, "%s%s", fs,
                                    !is_fp[i] ? Rast_get_c_cat((CELL *)rastp[i],
                                                               &labels[i])
                                              : no_data_str);
                        break;
                    case PLAIN:
                        fprintf(stdout, "%s%ld", i ? fs : "",
                                (long)*((CELL *)rastp[i]));
                        if (with_labels && !is_fp[i])
                            fprintf(
                                stdout, "%s%s", fs,
                                Rast_get_c_cat((CELL *)rastp[i], &labels[i]));
                        break;
                    }
                }
                else if (map_type[i] == FCELL_TYPE) {
                    switch (format) {
                    case JSON:
                        G_json_object_set_number(category, "category",
                                                 *((FCELL *)rastp[i]));
                        if (with_labels)
                            G_json_object_set_string(
                                category, "label",
                                Rast_get_f_cat((FCELL *)rastp[i], &labels[i]));
                        break;
                    case CSV:
                    case PLAIN:
                        snprintf(str1, sizeof(str1), "%.8g",
                                 *((FCELL *)rastp[i]));
                        G_trim_decimal(str1);
                        G_strip(str1);
                        fprintf(stdout, "%s%s", i ? fs : "", str1);
                        if (with_labels)
                            fprintf(
                                stdout, "%s%s", fs,
                                Rast_get_f_cat((FCELL *)rastp[i], &labels[i]));
                        break;
                    }
                }
                else if (map_type[i] == DCELL_TYPE) {
                    switch (format) {
                    case JSON:
                        G_json_object_set_number(category, "category",
                                                 *((DCELL *)rastp[i]));
                        if (with_labels)
                            G_json_object_set_string(
                                category, "label",
                                Rast_get_d_cat((DCELL *)rastp[i], &labels[i]));
                        break;
                    case CSV:
                    case PLAIN:
                        snprintf(str1, sizeof(str1), "%.16g",
                                 *((DCELL *)rastp[i]));
                        G_trim_decimal(str1);
                        G_strip(str1);
                        fprintf(stdout, "%s%s", i ? fs : "", str1);
                        if (with_labels)
                            fprintf(
                                stdout, "%s%s", fs,
                                Rast_get_d_cat((DCELL *)rastp[i], &labels[i]));
                        break;
                    }
                }
                else
                    G_fatal_error(_("Invalid map type"));

                if (format == JSON) {
                    G_json_array_append_value(categories, category_value);
                }

                rastp[i] =
                    G_incr_void_ptr(rastp[i], Rast_cell_size(map_type[i]));
            }

            switch (format) {
            case JSON:
                G_json_object_set_value(object, "categories", categories_value);
                G_json_array_append_value(array, object_value);
                break;
            case CSV:
            case PLAIN:
                fprintf(stdout, "\n");
                break;
            }
        }
    }

    G_percent(row, nrows, 2);
    G_free(map_type);
    for (i = 0; i < nfiles; i++) {
        G_free(rast[i]);
    }
    G_free(rast);
    G_free(rastp);

    return 0;
}

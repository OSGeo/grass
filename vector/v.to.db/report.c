#include <math.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>
#include "global.h"

int report(enum OutputFormat format)
{
    char unit_name[20];

    G_JSON_Value *records_value = NULL, *record_value = NULL,
                 *root_value = NULL, *units_value = NULL, *totals_value = NULL;
    G_JSON_Array *records_array = NULL;
    G_JSON_Object *record = NULL, *root_object = NULL, *units_object = NULL,
                  *totals_object = NULL;
    // todo: add measurement unit

    if (format == JSON) {
        root_value = G_json_value_init_object();
        root_object = G_json_object(root_value);
        records_value = G_json_value_init_array();
        records_array = G_json_array(records_value);

        units_value = G_json_value_init_object();
        units_object = G_json_object(units_value);

        totals_value = G_json_value_init_object();
        totals_object = G_json_object(totals_value);

        get_unit_name(unit_name);
    }

    int i, print_header = G_verbose() > G_verbose_min() || options.print_header;
    char left[20], right[20];
    int sum = 0;
    double fsum = 0.0;

    if (!options.print &&
        !(options.option == O_COUNT || options.option == O_LENGTH ||
          options.option == O_AREA)) {
        G_warning(_("No totals for selected option"));
        return 0;
    }

    switch (options.option) {
    case O_CAT:
        switch (format) {
        case PLAIN:
            if (print_header)
                fprintf(stdout, "cat\n");
            for (i = 0; i < vstat.rcat; i++)
                fprintf(stdout, "%d\n", Values[i].cat);
            break;
        case JSON:
            for (i = 0; i < vstat.rcat; i++) {
                record_value = G_json_value_init_object();
                record = G_json_object(record_value);
                G_json_object_set_number(record, "category", Values[i].cat);
                G_json_array_append_value(records_array, record_value);
            }
        }
        break;

    case O_COUNT:
        switch (format) {
        case PLAIN:
            if (options.print) {
                if (print_header)
                    fprintf(stdout, "cat%scount\n", options.fs);
                for (i = 0; i < vstat.rcat; i++)
                    fprintf(stdout, "%d%s%d\n", Values[i].cat, options.fs,
                            Values[i].count1);
            }
            if (options.total) {
                int sum = 0;

                for (i = 0; i < vstat.rcat; i++) {
                    sum += Values[i].count1;
                }
                fprintf(stdout, "total count%s%d\n", options.fs, sum);
            }
            break;
        case JSON:
            for (i = 0; i < vstat.rcat; i++) {
                record_value = G_json_value_init_object();
                record = G_json_object(record_value);
                G_json_object_set_number(record, "category", Values[i].cat);
                G_json_object_set_number(record, "count", Values[i].count1);
                G_json_array_append_value(records_array, record_value);
                sum += Values[i].count1;
            }
            G_json_object_set_number(totals_object, "count", sum);
            break;
        }
        break;

    case O_AREA:
        switch (format) {
        case PLAIN:
            if (options.print) {
                if (print_header)
                    fprintf(stdout, "cat%sarea\n", options.fs);
                for (i = 0; i < vstat.rcat; i++)
                    fprintf(stdout, "%d%s%.15g\n", Values[i].cat, options.fs,
                            Values[i].d1);
            }
            if (options.total) {
                double sum = 0.0;

                for (i = 0; i < vstat.rcat; i++) {
                    sum += Values[i].d1;
                }
                fprintf(stdout, "total area%s%.15g\n", options.fs, sum);
            }
            break;
        case JSON:
            for (i = 0; i < vstat.rcat; i++) {
                record_value = G_json_value_init_object();
                record = G_json_object(record_value);
                G_json_object_set_number(record, "category", Values[i].cat);
                G_json_object_set_number(record, "area", Values[i].d1);
                G_json_array_append_value(records_array, record_value);
                fsum += Values[i].d1;
            }
            G_json_object_set_string(units_object, "area", unit_name);
            G_json_object_set_number(totals_object, "area", fsum);
            break;
        }
        break;

    case O_COMPACT:
        /* perimeter / perimeter of equivalent circle
         *   perimeter of equivalent circle: 2.0 * sqrt(M_PI * area) */
        switch (format) {
        case PLAIN:
            if (print_header)
                fprintf(stdout, "cat%scompact\n", options.fs);
            for (i = 0; i < vstat.rcat; i++) {
                Values[i].d1 = Values[i].d2 / (2.0 * sqrt(M_PI * Values[i].d1));
                fprintf(stdout, "%d%s%.15g\n", Values[i].cat, options.fs,
                        Values[i].d1);
            }
            break;
        case JSON:
            for (i = 0; i < vstat.rcat; i++) {
                record_value = G_json_value_init_object();
                record = G_json_object(record_value);
                G_json_object_set_number(record, "category", Values[i].cat);

                Values[i].d1 = Values[i].d2 / (2.0 * sqrt(M_PI * Values[i].d1));
                G_json_object_set_number(record, "compact", Values[i].d1);

                G_json_array_append_value(records_array, record_value);
            }
            break;
        }
        break;

    case O_FD:
        /* 2.0 * log(perimeter) / log(area)
         * this is neither
         *   log(perimeter) / log(perimeter of equivalent circle)
         *   perimeter of equivalent circle: 2 * sqrt(M_PI * area)
         * nor
         *   log(area of equivalent circle) / log(area)
         *   area of equivalent circle: (perimeter / (2 * sqrt(M_PI))^2
         *
         * avoid division by zero:
         * 2.0 * log(1 + perimeter) / log(1 + area) */
        switch (format) {
        case PLAIN:
            if (print_header)
                fprintf(stdout, "cat%sfd\n", options.fs);
            for (i = 0; i < vstat.rcat; i++) {
                if (Values[i].d1 == 1) /* log(1) == 0 */
                    Values[i].d1 += 0.000001;
                Values[i].d1 = 2.0 * log(Values[i].d2) / log(Values[i].d1);
                fprintf(stdout, "%d%s%.15g\n", Values[i].cat, options.fs,
                        Values[i].d1);
            }
            break;
        case JSON:
            for (i = 0; i < vstat.rcat; i++) {
                record_value = G_json_value_init_object();
                record = G_json_object(record_value);
                G_json_object_set_number(record, "category", Values[i].cat);

                if (Values[i].d1 == 1) /* log(1) == 0 */
                    Values[i].d1 += 0.000001;
                Values[i].d1 = 2.0 * log(Values[i].d2) / log(Values[i].d1);
                G_json_object_set_number(record, "fd", Values[i].d1);

                G_json_array_append_value(records_array, record_value);
            }
            break;
        }
        break;

    case O_PERIMETER:
        switch (format) {
        case PLAIN:
            if (print_header)
                fprintf(stdout, "cat%sperimeter\n", options.fs);
            for (i = 0; i < vstat.rcat; i++)
                fprintf(stdout, "%d%s%.15g\n", Values[i].cat, options.fs,
                        Values[i].d1);
            break;
        case JSON:
            for (i = 0; i < vstat.rcat; i++) {
                record_value = G_json_value_init_object();
                record = G_json_object(record_value);
                G_json_object_set_number(record, "category", Values[i].cat);
                G_json_object_set_number(record, "perimeter", Values[i].d1);
                G_json_array_append_value(records_array, record_value);
            }
            G_json_object_set_string(units_object, "perimeter", unit_name);
            break;
        }
        break;

    case O_BBOX:
        switch (format) {
        case PLAIN:
            if (print_header)
                fprintf(stdout, "cat%sN%sS%sE%sW\n", options.fs, options.fs,
                        options.fs, options.fs);
            for (i = 0; i < vstat.rcat; i++) {
                fprintf(stdout, "%d%s%.15g%s%.15g%s%.15g%s%.15g\n",
                        Values[i].cat, options.fs, Values[i].d1, options.fs,
                        Values[i].d2, options.fs, Values[i].d3, options.fs,
                        Values[i].d4);
            }
            break;
        case JSON:
            for (i = 0; i < vstat.rcat; i++) {
                record_value = G_json_value_init_object();
                record = G_json_object(record_value);
                G_json_object_set_number(record, "category", Values[i].cat);
                G_json_object_set_number(record, "north", Values[i].d1);
                G_json_object_set_number(record, "south", Values[i].d2);
                G_json_object_set_number(record, "east", Values[i].d3);
                G_json_object_set_number(record, "west", Values[i].d4);
                G_json_array_append_value(records_array, record_value);
            }
            break;
        }
        break;

    case O_LENGTH:
        switch (format) {
        case PLAIN:
            if (options.print) {
                if (print_header)
                    fprintf(stdout, "cat%slength\n", options.fs);
                for (i = 0; i < vstat.rcat; i++)
                    fprintf(stdout, "%d%s%.15g\n", Values[i].cat, options.fs,
                            Values[i].d1);
            }
            if (options.total) {
                double sum = 0.0;

                for (i = 0; i < vstat.rcat; i++) {
                    sum += Values[i].d1;
                }
                fprintf(stdout, "total length%s%.15g\n", options.fs, sum);
            }
            break;
        case JSON:
            for (i = 0; i < vstat.rcat; i++) {
                record_value = G_json_value_init_object();
                record = G_json_object(record_value);
                G_json_object_set_number(record, "category", Values[i].cat);
                G_json_object_set_number(record, "length", Values[i].d1);
                G_json_array_append_value(records_array, record_value);
                fsum += Values[i].d1;
            }
            G_json_object_set_string(units_object, "length", unit_name);
            G_json_object_set_number(totals_object, "length", fsum);
            break;
        }
        break;
    case O_SLOPE:
        switch (format) {
        case PLAIN:
            if (print_header)
                fprintf(stdout, "cat%sslope\n", options.fs);
            for (i = 0; i < vstat.rcat; i++)
                fprintf(stdout, "%d%s%.15g\n", Values[i].cat, options.fs,
                        Values[i].d1);
            break;
        case JSON:
            for (i = 0; i < vstat.rcat; i++) {
                record_value = G_json_value_init_object();
                record = G_json_object(record_value);
                G_json_object_set_number(record, "category", Values[i].cat);
                G_json_object_set_number(record, "slope", Values[i].d1);
                G_json_array_append_value(records_array, record_value);
            }
            break;
        }

        break;
    case O_SINUOUS:
        switch (format) {
        case PLAIN:
            if (print_header)
                fprintf(stdout, "cat%ssinuous\n", options.fs);
            for (i = 0; i < vstat.rcat; i++)
                fprintf(stdout, "%d%s%.15g\n", Values[i].cat, options.fs,
                        Values[i].d1);
            break;
        case JSON:
            for (i = 0; i < vstat.rcat; i++) {
                record_value = G_json_value_init_object();
                record = G_json_object(record_value);
                G_json_object_set_number(record, "category", Values[i].cat);
                G_json_object_set_number(record, "sinuous", Values[i].d1);
                G_json_array_append_value(records_array, record_value);
            }
            break;
        }
        break;
    case O_COOR:
    case O_START:
    case O_END:
        switch (format) {
        case PLAIN:
            if (print_header)
                fprintf(stdout, "cat%sx%sy%sz\n", options.fs, options.fs,
                        options.fs);
            for (i = 0; i < vstat.rcat; i++) {
                if (Values[i].count1 == 1)
                    fprintf(stdout, "%d%s%.15g%s%.15g%s%.15g\n", Values[i].cat,
                            options.fs, Values[i].d1, options.fs, Values[i].d2,
                            options.fs, Values[i].d3);
            }
            break;
        case JSON:
            for (i = 0; i < vstat.rcat; i++) {
                if (Values[i].count1 == 1) {
                    record_value = G_json_value_init_object();
                    record = G_json_object(record_value);
                    G_json_object_set_number(record, "category", Values[i].cat);
                    G_json_object_set_number(record, "x", Values[i].d1);
                    G_json_object_set_number(record, "y", Values[i].d2);
                    G_json_object_set_number(record, "z", Values[i].d3);
                    G_json_array_append_value(records_array, record_value);
                }
            }
            break;
        }
        break;

    case O_SIDES:
        switch (format) {
        case PLAIN:
            if (print_header)
                fprintf(stdout, "cat%sleft%sright\n", options.fs, options.fs);
            for (i = 0; i < vstat.rcat; i++) {
                if (Values[i].count1 == 1) {
                    if (Values[i].i1 >= 0)
                        snprintf(left, sizeof(left), "%d", Values[i].i1);
                    else
                        snprintf(left, sizeof(left),
                                 "-1"); /* NULL, no area/cat */
                }
                else if (Values[i].count1 > 1) {
                    snprintf(left, sizeof(left), "-");
                }
                else { /* Values[i].count1 == 0 */
                    /* It can be OK if the category is assigned to an element
                       type which is not GV_BOUNDARY */
                    /* -> TODO: print only if there is boundary with that cat */
                    snprintf(left, sizeof(left), "-");
                }

                if (Values[i].count2 == 1) {
                    if (Values[i].i2 >= 0)
                        snprintf(right, sizeof(right), "%d", Values[i].i2);
                    else
                        snprintf(right, sizeof(right),
                                 "-1"); /* NULL, no area/cat */
                }
                else if (Values[i].count2 > 1) {
                    snprintf(right, sizeof(right), "-");
                }
                else { /* Values[i].count1 == 0 */
                    snprintf(right, sizeof(right), "-");
                }

                fprintf(stdout, "%d%s%s%s%s\n", Values[i].cat, options.fs, left,
                        options.fs, right);
            }
            break;
        case JSON:
            for (i = 0; i < vstat.rcat; i++) {
                record_value = G_json_value_init_object();
                record = G_json_object(record_value);
                G_json_object_set_number(record, "category", Values[i].cat);

                if (Values[i].count1 == 1) {
                    if (Values[i].i1 >= 0)
                        G_json_object_set_number(record, "left", Values[i].i1);
                    else
                        G_json_object_set_number(record, "left",
                                                 -1); /* NULL, no area/cat */
                }
                else if (Values[i].count1 > 1) {
                    G_json_object_set_null(record, "left");
                }
                else { /* Values[i].count1 == 0 */
                    /* It can be OK if the category is assigned to an element
                       type which is not GV_BOUNDARY */
                    /* -> TODO: print only if there is boundary with that cat */
                    G_json_object_set_null(record, "left");
                }

                if (Values[i].count2 == 1) {
                    if (Values[i].i2 >= 0)
                        G_json_object_set_number(record, "right", Values[i].i2);
                    else
                        G_json_object_set_number(record, "right",
                                                 -1); /* NULL, no area/cat */
                }
                else if (Values[i].count2 > 1) {
                    G_json_object_set_null(record, "right");
                }
                else { /* Values[i].count1 == 0 */
                    G_json_object_set_null(record, "right");
                }
            }
            break;
        }
        break;

    case O_QUERY:
        switch (format) {
        case PLAIN:
            if (print_header)
                fprintf(stdout, "cat%squery\n", options.fs);
            for (i = 0; i < vstat.rcat; i++) {
                if (Values[i].null) {
                    fprintf(stdout, "%d|-\n", Values[i].cat);
                }
                else {
                    switch (vstat.qtype) {
                    case (DB_C_TYPE_INT):
                        fprintf(stdout, "%d%s%d\n", Values[i].cat, options.fs,
                                Values[i].i1);
                        break;
                    case (DB_C_TYPE_DOUBLE):
                        fprintf(stdout, "%d%s%15g\n", Values[i].cat, options.fs,
                                Values[i].d1);
                        break;
                    case (DB_C_TYPE_STRING):
                        fprintf(stdout, "%d%s%s\n", Values[i].cat, options.fs,
                                Values[i].str1);
                        break;
                    }
                }
            }
            break;
        case JSON:
            for (i = 0; i < vstat.rcat; i++) {
                record_value = G_json_value_init_object();
                record = G_json_object(record_value);
                if (Values[i].null) {
                    G_json_object_set_null(record, "query");
                }
                else {
                    switch (vstat.qtype) {
                    case (DB_C_TYPE_INT):
                        G_json_object_set_number(record, "query", Values[i].i1);
                        break;
                    case (DB_C_TYPE_DOUBLE):
                        G_json_object_set_number(record, "query", Values[i].d1);
                        break;
                    case (DB_C_TYPE_STRING):
                        G_json_object_set_string(record, "query",
                                                 Values[i].str1);
                        break;
                    }
                }
            }
        }
        break;
    case O_AZIMUTH:
        switch (format) {
        case PLAIN:
            if (print_header)
                fprintf(stdout, "cat%sazimuth\n", options.fs);
            for (i = 0; i < vstat.rcat; i++)
                fprintf(stdout, "%d%s%.15g\n", Values[i].cat, options.fs,
                        Values[i].d1);
            break;
        case JSON:
            for (i = 0; i < vstat.rcat; i++) {
                record_value = G_json_value_init_object();
                record = G_json_object(record_value);
                G_json_object_set_number(record, "category", Values[i].cat);
                G_json_object_set_number(record, "azimuth", Values[i].d1);
                G_json_array_append_value(records_array, record_value);
            }
            G_json_object_set_string(units_object, "azimuth", unit_name);
            break;
        }
        break;
    }

    if (format == JSON) {
        G_json_object_set_value(root_object, "units", units_value);
        G_json_object_set_value(root_object, "totals", totals_value);
        G_json_object_set_value(root_object, "records", records_value);
        char *serialized_string = G_json_serialize_to_string_pretty(root_value);
        if (serialized_string == NULL) {
            G_fatal_error(_("Failed to initialize pretty JSON string."));
        }
        puts(serialized_string);
        G_json_free_serialized_string(serialized_string);
        G_json_value_free(root_value);
    }

    return 0;
}

int print_stat(void)
{
    if (vstat.rcat > 0) {
        int rcat_report;

        if (find_cat(-1, 0) != -1)
            rcat_report = vstat.rcat - 1;
        else
            rcat_report = vstat.rcat;

        G_message(_("%d categories read from vector map (layer %d)"),
                  rcat_report, options.field); /* don't report cat -1 */
    }
    if (vstat.select > 0)
        G_message(_("%d records selected from table (layer %d)"), vstat.select,
                  options.qfield);
    if (vstat.exist > 0)
        G_message(_("%d categories read from vector map exist in selection "
                    "from table"),
                  vstat.exist);
    if (vstat.notexist > 0)
        G_message(_("%d categories read from vector map don't exist in "
                    "selection from table"),
                  vstat.notexist);
    G_message(_("%d records updated/inserted (layer %d)"), vstat.update,
              options.field);
    if (vstat.error > 0)
        G_message(_("%d update/insert errors (layer %d)"), vstat.error,
                  options.field);
    if (vstat.dupl > 0)
        G_message(_("%d categories with more points (coordinates not loaded)"),
                  vstat.dupl);

    return 0;
}

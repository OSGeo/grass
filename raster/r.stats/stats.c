#include <stdlib.h>
#include "global.h"

#include <grass/gjson.h>

/* hash definitions (these should be prime numbers) ************* */
#define HASHSIZE 7307
#define HASHMOD  89

static CELL *values;
static struct Node *node_pool;
static int node_pool_count;
static CELL *value_pool;
static int value_pool_count;

#define NODE_INCR  32
#define VALUE_INCR 32

static struct Node **sorted_list;

struct Node {
    CELL *values;
    struct Node *left;
    struct Node *right;
    struct Node *list;
    long count;
    double area;
};

static struct Node **hashtable;
static struct Node *node_list = NULL;
static int node_count = 0;
static long total_count = 0;

int initialize_cell_stats(int n)
{
    int i;

    /* record nilfes first */
    nfiles = n;

    /* allocate a pool of value arrays */
    value_pool_count = 0;
    allocate_values();

    /* set Node pool to empty */
    node_pool_count = 0;

    /* empty the has table */
    hashtable = (struct Node **)G_malloc(HASHSIZE * sizeof(struct Node *));
    for (i = 0; i < HASHSIZE; i++)
        hashtable[i] = NULL;

    return 0;
}

int allocate_values(void)
{
    value_pool_count = VALUE_INCR;
    value_pool = (CELL *)G_calloc(nfiles * value_pool_count, sizeof(CELL));
    values = value_pool;

    return 0;
}

struct Node *NewNode(double area)
{
    struct Node *node;

    if (node_pool_count <= 0)
        node_pool = (struct Node *)G_calloc(node_pool_count = NODE_INCR,
                                            sizeof(struct Node));
    node = &node_pool[--node_pool_count];
    node->count = 1;
    node->area = area;
    node->values = values;

    if (--value_pool_count <= 0)
        allocate_values();
    else
        values += nfiles;

    node->left = node->right = NULL;
    node->list = node_list;
    node_list = node;
    node_count++;

    return node;
}

/* Essentially, Rast_quant_add_rule() treats the ranges as half-open,
 *  i.e. the values range from low (inclusive) to high (exclusive).
 *  While half-open ranges are a common concept (e.g. floor() behaves
 *  the same way), the range of a GRASS raster is closed, i.e. both the
 *  low and high values are inclusive.
 *  Therefore the quantized max FP cell gets put in the nsteps+1'th bin
 *  and we need to manually place it back in the previous bin. */
void fix_max_fp_val(CELL *cell, int ncols)
{
    while (ncols-- > 0) {
        if (cell[ncols] > nsteps)
            cell[ncols] = (CELL)nsteps;
        /* { G_debug(5, ". resetting %d to %d", cell[ncols], nsteps); } */
    }
    return;
}

/* we can't compute hash on null values, so we change all
 *  nulls to max+1, set NULL_CELL to max+1, and later compare
 *  with NULL_CELL to check for nulls */
void reset_null_vals(CELL *cell, int ncols)
{
    while (ncols-- > 0) {
        if (Rast_is_c_null_value(&cell[ncols]))
            cell[ncols] = NULL_CELL;
    }
    return;
}

int update_cell_stats(CELL **cell, int ncols, double area)
{
    register int i;
    register int hash;
    register struct Node *q, *p = NULL;
    register int dir = 0;

    while (ncols-- > 0) {
        /* copy this cell to an array, compute hash */

        hash = values[0] = cell[0][ncols];

        for (i = 1; i < nfiles; i++)
            hash = hash * HASHMOD + (values[i] = cell[i][ncols]);

        if (hash < 0)
            hash = -hash;

        hash %= HASHSIZE;

        /* look it up and update/insert */
        if ((q = hashtable[hash]) == NULL) {
            hashtable[hash] = NewNode(area);
        }
        else {
            while (1) {
                for (i = 0; i < nfiles; i++) {
                    if (values[i] < q->values[i]) {
                        dir = -1;
                        p = q->left;
                        break;
                    }
                    if (values[i] > q->values[i]) {
                        dir = 1;
                        p = q->right;
                        break;
                    }
                }

                if (i == nfiles) { /* match */
                    q->count++;
                    q->area += area;
                    total_count++;
                    break;
                }
                else if (p == NULL) {
                    if (dir < 0)
                        q->left = NewNode(area);
                    else
                        q->right = NewNode(area);
                    break;
                }
                else
                    q = p;
            }
        }
    }

    return 0;
}

static int node_compare(const void *pp, const void *qq)
{
    struct Node *const *p = pp, *const *q = qq;
    register int i;
    register const CELL *a, *b;

    a = (*p)->values;
    b = (*q)->values;
    for (i = nfiles; --i >= 0;) {
        if (*a < *b)
            return -1;
        else if (*a > *b)
            return 1;
        a++, b++;
    }

    return 0;
}

static int node_compare_count_asc(const void *pp, const void *qq)
{
    struct Node *const *p = pp, *const *q = qq;
    long a, b;

    a = (*p)->count;
    b = (*q)->count;

    if (a < b)
        return -1;
    return (a > b);
}

static int node_compare_count_desc(const void *pp, const void *qq)
{
    struct Node *const *p = pp, *const *q = qq;
    long a, b;

    a = (*p)->count;
    b = (*q)->count;

    if (a > b)
        return -1;
    return (a < b);
}

int sort_cell_stats(int do_sort)
{
    struct Node **q, *p;

    if (node_count <= 0)
        return 0;

    G_free(hashtable); /* make a bit more room */
    sorted_list = (struct Node **)G_calloc(node_count, sizeof(struct Node *));
    for (q = sorted_list, p = node_list; p; p = p->list)
        *q++ = p;

    if (do_sort == SORT_DEFAULT)
        qsort(sorted_list, node_count, sizeof(struct Node *), node_compare);
    else if (do_sort == SORT_ASC)
        qsort(sorted_list, node_count, sizeof(struct Node *),
              node_compare_count_asc);
    else if (do_sort == SORT_DESC)
        qsort(sorted_list, node_count, sizeof(struct Node *),
              node_compare_count_desc);

    return 0;
}

int print_node_count(void)
{
    fprintf(stdout, "%d nodes\n", node_count);

    return 0;
}

int print_cell_stats(char *fmt, int with_percents, int with_counts,
                     int with_areas, int with_labels, char *fs,
                     enum OutputFormat format, G_JSON_Array *array)
{
    int i, n, nulls_found;
    struct Node *node;
    CELL tmp_cell, null_cell;
    DCELL dLow, dHigh;
    char str1[50], str2[50];

    G_JSON_Object *object, *category;
    G_JSON_Array *categories;
    G_JSON_Value *object_value, *category_value, *categories_value;

    if (no_nulls)
        total_count -= sorted_list[node_count - 1]->count;

    Rast_set_c_null_value(&null_cell, 1);
    if (node_count <= 0) {
        if (format == CSV) {
            /* CSV Header */
            for (i = 0; i < nfiles; i++) {
                fprintf(stdout, "%s%s_%s", i ? fs : "", map_names[i], "cat");
            }
            if (with_areas) {
                fprintf(stdout, "%s%s", fs, "area");
            }
            if (with_counts) {
                fprintf(stdout, "%s%s", fs, "count");
            }
            if (with_percents) {
                fprintf(stdout, "%s%s", fs, "percent");
            }
            if (with_labels)
                fprintf(stdout, "%s%s", fs, "label");
            fprintf(stdout, "\n");
        }

        switch (format) {
        case JSON:
            break;
        case CSV:
        case PLAIN:
            fprintf(stdout, "0");
            for (i = 1; i < nfiles; i++)
                fprintf(stdout, "%s%s", fs, no_data_str);
            if (with_areas)
                fprintf(stdout, "%s0.0", fs);
            if (with_counts)
                fprintf(stdout, "%s0", fs);
            if (with_percents)
                fprintf(stdout, "%s0.00%%", fs);
            if (with_labels)
                fprintf(stdout, "%s%s", fs,
                        Rast_get_c_cat(&null_cell, &labels[i]));
            fprintf(stdout, "\n");
        }
    }
    else {

        if (format == CSV) {
            /* CSV Header */
            for (i = 0; i < nfiles; i++) {
                if (raw_output || !is_fp[i] || as_int)
                    fprintf(stdout, "%s%s_%s", i ? fs : "", map_names[i],
                            "cat");
                else if (averaged)
                    fprintf(stdout, "%s%s_%s", i ? fs : "", map_names[i],
                            "average");
                else
                    fprintf(stdout, "%s%s_%s", i ? fs : "", map_names[i],
                            "range");

                if (with_labels)
                    fprintf(stdout, "%s%s_%s", fs, map_names[i], "label");
            }

            if (with_areas) {
                fprintf(stdout, "%s%s", fs, "area");
            }
            if (with_counts) {
                fprintf(stdout, "%s%s", fs, "count");
            }
            if (with_percents) {
                fprintf(stdout, "%s%s", fs, "percent");
            }
            fprintf(stdout, "\n");
        }

        for (n = 0; n < node_count; n++) {
            if (format == JSON) {
                object_value = G_json_value_init_object();
                object = G_json_object(object_value);
                categories_value = G_json_value_init_array();
                categories = G_json_array(categories_value);
            }

            node = sorted_list[n];

            if (no_nulls || no_nulls_all) {
                nulls_found = 0;
                for (i = 0; i < nfiles; i++)
                    /*
                       if (node->values[i] || (!raw_output && is_fp[i]))
                       break;
                     */
                    if (node->values[i] == NULL_CELL)
                        nulls_found++;

                if (nulls_found == nfiles)
                    continue;

                if (no_nulls && nulls_found)
                    continue;
            }

            for (i = 0; i < nfiles; i++) {
                if (format == JSON) {
                    category_value = G_json_value_init_object();
                    category = G_json_object(category_value);
                }
                if (node->values[i] == NULL_CELL) {
                    switch (format) {
                    case JSON:
                        if (raw_output || !is_fp[i] || as_int)
                            G_json_object_set_null(category, "category");
                        else if (averaged)
                            G_json_object_set_null(category, "average");
                        else
                            G_json_object_set_null(category, "range");
                        if (with_labels && !(raw_output && is_fp[i]))
                            G_json_object_set_string(
                                category, "label",
                                Rast_get_c_cat(&null_cell, &labels[i]));
                        break;
                    case CSV:
                        fprintf(stdout, "%s%s", i ? fs : "", no_data_str);
                        if (with_labels)
                            fprintf(stdout, "%s%s", fs,
                                    !(raw_output && is_fp[i])
                                        ? Rast_get_c_cat(&null_cell, &labels[i])
                                        : no_data_str);
                        break;
                    case PLAIN:
                        fprintf(stdout, "%s%s", i ? fs : "", no_data_str);
                        if (with_labels && !(raw_output && is_fp[i]))
                            fprintf(stdout, "%s%s", fs,
                                    Rast_get_c_cat(&null_cell, &labels[i]));
                        break;
                    }
                }
                else if (raw_output || !is_fp[i] || as_int) {
                    switch (format) {
                    case JSON:
                        G_json_object_set_number(category, "category",
                                                 (long)node->values[i]);
                        if (with_labels && !is_fp[i]) {
                            G_json_object_set_string(
                                category, "label",
                                Rast_get_c_cat((CELL *)&(node->values[i]),
                                               &labels[i]));
                        }
                        break;
                    case CSV:
                        fprintf(stdout, "%s%ld", i ? fs : "",
                                (long)node->values[i]);
                        if (with_labels)
                            fprintf(stdout, "%s%s", fs,
                                    !is_fp[i] ? Rast_get_c_cat(
                                                    (CELL *)&(node->values[i]),
                                                    &labels[i])
                                              : no_data_str);
                        break;
                    case PLAIN:
                        fprintf(stdout, "%s%ld", i ? fs : "",
                                (long)node->values[i]);
                        if (with_labels && !is_fp[i])
                            fprintf(stdout, "%s%s", fs,
                                    Rast_get_c_cat((CELL *)&(node->values[i]),
                                                   &labels[i]));
                        break;
                    }
                }
                else { /* find out which floating point range to print */

                    if (cat_ranges)
                        Rast_quant_get_ith_rule(&labels[i].q, node->values[i],
                                                &dLow, &dHigh, &tmp_cell,
                                                &tmp_cell);
                    else {
                        dLow = (DMAX[i] - DMIN[i]) / nsteps *
                                   (double)(node->values[i] - 1) +
                               DMIN[i];
                        dHigh = (DMAX[i] - DMIN[i]) / nsteps *
                                    (double)node->values[i] +
                                DMIN[i];
                    }
                    if (averaged) {
                        /* print averaged values */
                        double average = (dLow + dHigh) / 2.0;
                        switch (format) {
                        case JSON:
                            G_json_object_set_number(category, "average",
                                                     average);
                            break;
                        case CSV:
                        case PLAIN:
                            snprintf(str1, sizeof(str1), "%10f", average);
                            G_trim_decimal(str1);
                            G_strip(str1);
                            fprintf(stdout, "%s%s", i ? fs : "", str1);
                            break;
                        }
                    }
                    else {
                        switch (format) {
                        case JSON:
                            G_json_object_dotset_number(category, "range.from",
                                                        dLow);
                            G_json_object_dotset_number(category, "range.to",
                                                        dHigh);
                            break;
                        case CSV:
                        case PLAIN:
                            /* print intervals */
                            snprintf(str1, sizeof(str1), "%10f", dLow);
                            snprintf(str2, sizeof(str2), "%10f", dHigh);
                            G_trim_decimal(str1);
                            G_trim_decimal(str2);
                            G_strip(str1);
                            G_strip(str2);
                            fprintf(stdout, "%s%s-%s", i ? fs : "", str1, str2);
                            break;
                        }
                    }
                    switch (format) {
                    case JSON:
                        if (with_labels) {
                            if (cat_ranges) {
                                G_json_object_set_string(
                                    category, "label",
                                    labels[i].labels[node->values[i]]);
                            }
                            else {
                                G_json_object_dotset_string(
                                    category, "label.from",
                                    Rast_get_d_cat(&dLow, &labels[i]));
                                G_json_object_dotset_string(
                                    category, "label.to",
                                    Rast_get_d_cat(&dHigh, &labels[i]));
                            }
                        }
                        break;
                    case CSV:
                    case PLAIN:
                        if (with_labels) {
                            if (cat_ranges)
                                fprintf(stdout, "%s%s", fs,
                                        labels[i].labels[node->values[i]]);
                            else
                                fprintf(stdout, "%sfrom %s to %s", fs,
                                        Rast_get_d_cat(&dLow, &labels[i]),
                                        Rast_get_d_cat(&dHigh, &labels[i]));
                        }
                        break;
                    }
                }

                if (format == JSON) {
                    G_json_array_append_value(categories, category_value);
                }
            }

            if (format == JSON) {
                G_json_object_set_value(object, "categories", categories_value);
            }

            if (with_areas) {
                switch (format) {
                case JSON:
                    G_json_object_set_number(object, "area", node->area);
                    break;
                case CSV:
                case PLAIN:
                    fprintf(stdout, "%s", fs);
                    fprintf(stdout, fmt, node->area);
                    break;
                }
            }
            if (with_counts) {
                switch (format) {
                case JSON:
                    G_json_object_set_number(object, "count", node->count);
                    break;
                case CSV:
                case PLAIN:
                    fprintf(stdout, "%s%ld", fs, (long)node->count);
                    break;
                }
            }
            if (with_percents) {
                double percent = (double)100 * node->count / total_count;
                switch (format) {
                case JSON:
                    G_json_object_set_number(object, "percent", percent);
                    break;
                case CSV:
                case PLAIN:
                    fprintf(stdout, "%s%.2f%%", fs, percent);
                    break;
                }
            }

            switch (format) {
            case JSON:
                G_json_array_append_value(array, object_value);
                break;
            case CSV:
            case PLAIN:
                fprintf(stdout, "\n");
                break;
            }
        }
    }

    return 0;
}

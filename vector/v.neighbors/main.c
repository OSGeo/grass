/***************************************************************
 *
 * MODULE:       v.neighbors
 *
 * AUTHOR(S):    Radim Blazek, original code taken from r.neighbors/main.c
 *               OGR support by Martin Landa <landa.martin gmail.com> (2009)
 *               Choice of methods and cat/where selection implemented
 *               by Moritz Lennert (2020),
 *               original code taken from v.vect.stats/main.c
 *
 * PURPOSE:      Category manipulations
 *
 * COPYRIGHT:    (C) 2001-2020 by the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2).  Read the file COPYING that
 *               comes with GRASS for details.
 *
 **************************************************************/

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/vector.h>
#include <grass/glocale.h>
#include <grass/stats.h>
#include <grass/dbmi.h>

struct menu {
    stat_func *method; /* routine to compute new value */
    int otype;         /* whether the result is an integer (unused) */
    char *name;        /* method name */
    char *text;        /* menu display - full description */
};

enum out_type { T_FLOAT = 1, T_INT = 2, T_COUNT = 3, T_COPY = 4, T_SUM = 5 };

/* modify this table to add new methods */
static struct menu menu[] = {
    {c_count, T_COUNT, "count", "number of points"},
    {c_sum, T_SUM, "sum", "sum of values"},
    {c_ave, T_FLOAT, "average", "average value"},
    {c_median, T_FLOAT, "median", "median value"},
    {c_mode, T_COPY, "mode", "most frequently occurring value"},
    {c_min, T_COPY, "minimum", "lowest value"},
    {c_max, T_COPY, "maximum", "highest value"},
    {c_range, T_COPY, "range", "range of values"},
    {c_stddev, T_FLOAT, "stddev", "standard deviation"},
    {c_var, T_FLOAT, "variance", "statistical variance"},
    {c_divr, T_INT, "diversity", "number of different values"},
    {NULL, 0, NULL, NULL}};

static RASTER_MAP_TYPE output_type(RASTER_MAP_TYPE input_type, int weighted,
                                   int mode)
{
    switch (mode) {
    case T_FLOAT:
        return DCELL_TYPE;
    case T_INT:
        return CELL_TYPE;
    case T_COUNT:
        return weighted ? DCELL_TYPE : CELL_TYPE;
    case T_COPY:
        return input_type;
    case T_SUM:
        return weighted ? DCELL_TYPE : input_type;
    default:
        G_fatal_error(_("Invalid out_type enumeration: %d"), mode);
        return -1;
    }
}

int main(int argc, char *argv[])
{
    int out_fd;
    DCELL *result, *rp;
    int nrows, ncols;
    int row, col, count_sum;
    int field;
    struct GModule *module;
    struct Option *in_opt, *out_opt, *field_opt;
    struct Option *method_opt, *size_opt, *column_opt;
    struct Map_info In;
    double radius, dia;
    struct boxlist *List;
    struct Cell_head region;
    struct bound_box box;
    struct line_pnts *Points;
    struct line_cats *Cats;

    char *p;
    int method;
    RASTER_MAP_TYPE imap_type, omap_type;
    int nrec;
    int i;
    int tmp_cat;
    int ctype;
    struct field_info *Fi;
    struct cat_list *pcat_list = NULL;
    struct Option *point_cats_opt, *point_where_opt;
    struct History history;

    dbDriver *driver;
    dbCatValArray cvarr;
    dbColumn *column;
    dbCatVal *catval;
    double *pvalcats;
    int npvalcats, npvalcatsalloc;
    stat_func *statsvalue = NULL;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("algebra"));
    G_add_keyword(_("statistics"));
    G_add_keyword(_("raster"));
    G_add_keyword(_("aggregation"));
    module->label = _("Neighborhood analysis tool for vector point maps.");
    module->description = _("Makes each cell value a "
                            "function of the attribute values assigned to the "
                            "vector points or centroids "
                            "in a radius around it, and stores new cell values "
                            "in an output raster map.");

    in_opt = G_define_standard_option(G_OPT_V_INPUT);

    field_opt = G_define_standard_option(G_OPT_V_FIELD);

    out_opt = G_define_standard_option(G_OPT_R_OUTPUT);

    method_opt = G_define_option();
    method_opt->key = "method";
    method_opt->type = TYPE_STRING;
    method_opt->required = YES;
    method_opt->multiple = NO;
    p = G_malloc(1024);
    for (i = 0; menu[i].name; i++) {
        if (i)
            strcat(p, ",");
        else
            *p = 0;
        strcat(p, menu[i].name);
    }
    method_opt->options = p;
    method_opt->answer = "count";
    method_opt->description =
        _("Method for aggregate statistics (count if non given)");

    size_opt = G_define_option();
    size_opt->key = "size";
    size_opt->type = TYPE_DOUBLE;
    size_opt->required = YES;
    size_opt->description = _("Neighborhood diameter in map units");

    column_opt = G_define_standard_option(G_OPT_DB_COLUMN);
    column_opt->key = "points_column";
    column_opt->label = _("Column name of points map to use for statistics");
    column_opt->description = _("Column of points map must be numeric");

    point_cats_opt = G_define_standard_option(G_OPT_V_CATS);
    point_where_opt = G_define_standard_option(G_OPT_DB_WHERE);

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    if (strcmp(method_opt->answer, "count") != 0) {
        if (!column_opt->answer) {
            G_fatal_error(
                "Method other than count but no point column selected");
        }
    }

    if (column_opt->answer) {
        if (strcmp(method_opt->answer, "count") == 0) {
            G_warning("Point column ignored for method 'count'");
        }
    }

    radius = atof(size_opt->answer) / 2;

    method = -1;
    if (method_opt->answer) {
        /* get the method */
        for (method = 0; (p = menu[method].name); method++)
            if ((strcmp(p, method_opt->answer) == 0))
                break;
        if (!p) {
            G_warning(_("<%s=%s> unknown %s"), method_opt->key,
                      method_opt->answer, method_opt->answer);
            G_usage();
            exit(EXIT_FAILURE);
        }

        /* establish the statsvalue routine */
        statsvalue = menu[method].method;
    }

    /* open input vector */
    Vect_set_open_level(2);
    if (Vect_open_old2(&In, in_opt->answer, "", field_opt->answer) < 0)
        G_fatal_error(_("Unable to open vector map <%s>"), in_opt->answer);

    field = Vect_get_field_number(&In, field_opt->answer);
    pcat_list = NULL;
    if (field > 0)
        pcat_list = Vect_cats_set_constraint(
            &In, field, point_where_opt->answer, point_cats_opt->answer);

    imap_type = CELL_TYPE;

    if (strcmp(method_opt->answer, "count") != 0) {
        Fi = Vect_get_field(&In, field);
        if (Fi == NULL)
            G_fatal_error(_("Database connection not defined for layer %d"),
                          field);

        driver = db_start_driver_open_database(Fi->driver, Fi->database);
        if (driver == NULL)
            G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
                          Fi->database, Fi->driver);

        /* check if point column exists */
        db_get_column(driver, Fi->table, column_opt->answer, &column);
        if (column) {
            db_free_column(column);
            column = NULL;
        }
        else {
            G_fatal_error(_("Column <%s> not found in table <%s>"),
                          column_opt->answer, Fi->table);
        }

        /* Check column type */
        ctype = db_column_Ctype(driver, Fi->table, column_opt->answer);

        if (ctype != DB_C_TYPE_INT && ctype != DB_C_TYPE_DOUBLE)
            G_fatal_error(
                _("points_column <%s> of points vector <%s> must be numeric"),
                column_opt->answer, Fi->table);

        /* Determine raster type equivalent of ctype */
        switch (ctype) {
        case DB_C_TYPE_INT:
            imap_type = CELL_TYPE;
            break;
        case DB_C_TYPE_DOUBLE:
            imap_type = DCELL_TYPE;
            break;
        }

        db_CatValArray_init(&cvarr);
        nrec = db_select_CatValArray(driver, Fi->table, Fi->key,
                                     column_opt->answer, NULL, &cvarr);
        G_debug(1, "selected values = %d", nrec);

        db_close_database_shutdown_driver(driver);
    }

    /* Determine raster output type */
    omap_type = output_type(imap_type, 0, menu[method].otype);

    G_get_set_window(&region);
    Vect_get_map_box(&In, &box);

    if (box.N > region.north + radius || box.S < region.south - radius ||
        box.E > region.east + radius || box.W < region.west - radius) {
        if (box.S > region.north + radius || box.N < region.south - radius ||
            box.W > region.east + radius || box.E < region.west - radius)
            G_fatal_error(_(
                "All points fall outside of the current computational region"));
        G_warning(_("Input vector and computational region do not overlap"));
    }

    dia = sqrt(region.ns_res * region.ns_res + region.ew_res * region.ew_res);
    if (radius * 2.0 < dia) {
        G_warning(_("The search diameter %g is smaller than cell diagonal %g: "
                    "some points could not be detected"),
                  radius * 2, dia);
    }

    nrows = Rast_window_rows();
    ncols = Rast_window_cols();

    result = Rast_allocate_buf(omap_type);
    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();
    List = Vect_new_boxlist(0);

    /*open the new cellfile */

    out_fd = Rast_open_new(out_opt->answer, omap_type);

    box.T = PORT_DOUBLE_MAX;
    box.B = -PORT_DOUBLE_MAX;

    count_sum = 0;
    for (row = 0; row < nrows; row++) {
        double x, y;

        G_percent(row, nrows, 2);

        y = Rast_row_to_northing(row + 0.5, &region);
        box.N = y + radius;
        box.S = y - radius;

        Rast_set_null_value(result, ncols, omap_type);
        rp = result;

        for (col = 0; col < ncols; col++) {
            int i, count;
            DCELL value;

            npvalcatsalloc = 10;
            npvalcats = 0;
            pvalcats = (double *)G_calloc(npvalcatsalloc, sizeof(double));

            x = Rast_col_to_easting(col + 0.5, &region);

            box.E = x + radius;
            box.W = x - radius;

            Vect_select_lines_by_box(&In, &box, GV_POINTS, List);
            G_debug(3, "  %d lines in box", List->n_values);

            count = 0;

            for (i = 0; i < List->n_values; i++) {

                Vect_read_line(&In, Points, Cats, List->id[i]);

                if (field != -1 && Vect_cat_get(Cats, field, NULL) == 0)
                    continue;

                if (field > 0 &&
                    !Vect_cats_in_constraint(Cats, field, pcat_list))
                    continue;

                if (Vect_points_distance(x, y, 0.0, Points->x[0], Points->y[0],
                                         0.0, 0) <= radius) {

                    count++;

                    if (strcmp(method_opt->answer, "count") != 0) {

                        if (Cats->n_cats > 1)
                            G_warning(_("Several cat values found for point "
                                        "%d. Using only first"),
                                      List->id[i]);
                        tmp_cat = Cats->cat[0];
                        /* find cat in array */
                        db_CatValArray_get_value(&cvarr, tmp_cat, &catval);

                        if (catval) {
                            switch (cvarr.ctype) {
                            case DB_C_TYPE_INT:
                                pvalcats[npvalcats] = catval->val.i;
                                npvalcats++;
                                break;

                            case DB_C_TYPE_DOUBLE:
                                pvalcats[npvalcats] = catval->val.d;
                                npvalcats++;
                                break;
                            }
                            if (npvalcats >= npvalcatsalloc) {
                                npvalcatsalloc += 10;
                                pvalcats = (double *)G_realloc(
                                    pvalcats, npvalcatsalloc * sizeof(double));
                            }
                        }
                    }
                }
            }

            if (count > 0) {
                if (strcmp(method_opt->answer, "count") != 0) {
                    statsvalue(&value, pvalcats, npvalcats, NULL);
                }
                else {
                    value = count;
                }
                Rast_set_d_value(rp, value, omap_type);
            }
            rp = G_incr_void_ptr(rp, Rast_cell_size(omap_type));
            count_sum += count;
        }

        Rast_put_row(out_fd, result, omap_type);
    }
    G_percent(1, 1, 1);

    Vect_close(&In);
    Rast_close(out_fd);

    Rast_short_history(out_opt->answer, "raster", &history);
    Rast_command_history(&history);
    Rast_write_history(out_opt->answer, &history);

    if (count_sum < 1)
        G_warning(_("No points found"));

    exit(EXIT_SUCCESS);
}

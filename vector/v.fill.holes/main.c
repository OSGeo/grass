
/****************************************************************
 *
 * MODULE:     v.fill.holes
 *
 * AUTHOR:     Vaclav Petras
 *
 * PURPOSE:    Fill holes in an area, i.e., preserve only its outer boundary
 *
 * COPYRIGHT:  (C) 2023 by Vaclav Petras and the GRASS Development Team
 *
 *             This program is free software under the GNU General
 *             Public License (>=v2).  Read the file COPYING that
 *             comes with GRASS for details.
 *
 ****************************************************************/

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

struct VFillHolesParameters {
    struct GModule *module;
    struct Option *input;
    struct Option *output;
    struct Option *field;
    struct Option *cats;
    struct Option *where;
};

int main(int argc, char *argv[])
{
    struct Map_info input;
    struct Map_info output;
    int open3d;

    G_gisinit(argv[0]);

    struct VFillHolesParameters options;

    options.module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("geometry"));
    G_add_keyword(_("fill"));
    G_add_keyword(_("exterior"));
    G_add_keyword(_("ring"));
    G_add_keyword(_("perimeter"));
    options.module->description =
        _("Fill holes in areas by keeping only outer boundaries");

    options.input = G_define_standard_option(G_OPT_V_INPUT);

    options.field = G_define_standard_option(G_OPT_V_FIELD);

    options.cats = G_define_standard_option(G_OPT_V_CATS);

    options.where = G_define_standard_option(G_OPT_DB_WHERE);

    options.output = G_define_standard_option(G_OPT_V_OUTPUT);

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    Vect_check_input_output_name(options.input->answer, options.output->answer,
                                 G_FATAL_EXIT);
    Vect_set_open_level(2);

    if (1 > Vect_open_old2(&input, options.input->answer, "",
                           options.field->answer))
        G_fatal_error(_("Unable to open vector map <%s>"),
                      options.input->answer);

    /* Check if old vector is 3D. We should preserve 3D data. */
    if (Vect_is_3d(&input))
        open3d = WITH_Z;
    else
        open3d = WITHOUT_Z;

    /* Set error handler for input vector map */
    Vect_set_error_handler_io(&input, NULL);

    /* Open new vector for reading/writing */
    if (0 > Vect_open_new(&output, options.output->answer, open3d)) {
        G_fatal_error(_("Unable to create vector map <%s>"),
                      options.output->answer);
    }

    /* Set error handler for output vector map */
    Vect_set_error_handler_io(NULL, &output);

    int field = Vect_get_field_number(&input, options.field->answer);

    if (field <= 0 && options.cats->answer)
        G_fatal_error(_("Option %s cannot be combined with %s=%s"),
                      options.cats->key, options.field->key,
                      options.field->answer);
    if (field <= 0 && options.where->answer)
        G_fatal_error(_("Option %s cannot be combined with %s=%s"),
                      options.where->key, options.field->key,
                      options.field->answer);

    /* Copy header and history data from old to new map */
    Vect_copy_head_data(&input, &output);
    Vect_hist_copy(&input, &output);
    Vect_hist_command(&output);

    // Set category constraint.
    struct cat_list *constraint_cat_list = NULL;

    if (field > 0)
        constraint_cat_list = Vect_cats_set_constraint(
            &input, field, options.where->answer, options.cats->answer);

    /* Create and initialize struct's where to store points/lines and categories
     */
    struct line_pnts *points = Vect_new_line_struct();
    struct line_cats *area_cats = Vect_new_cats_struct();
    struct line_cats *boundary_cats = Vect_new_cats_struct();

    struct ilist *all_cats = Vect_new_list();
    struct ilist *field_cats = Vect_new_list();
    struct ilist *area_boundaries = Vect_new_list();

    plus_t num_areas = Vect_get_num_areas(&input);
    plus_t num_lines = Vect_get_num_lines(&input);
    // Used as index. 0th element is unused.
    bool *line_written_out = G_calloc(num_lines + 1, sizeof(bool));

    G_percent(0, num_areas, 1);
    for (plus_t area = 1; area <= num_areas; area++) {
        G_percent(area, num_areas, 1);

        int centroid = Vect_get_area_centroid(&input, area);

        if (!centroid)
            continue;

        Vect_read_line(&input, points, area_cats, centroid);

        if (constraint_cat_list &&
            !Vect_cats_in_constraint(area_cats, field, constraint_cat_list)) {
            continue;
        }
        Vect_write_line(&output, GV_CENTROID, points, area_cats);

        Vect_get_area_boundaries(&input, area, area_boundaries);
        for (int i = 0; i < area_boundaries->n_values; i++) {
            int boundary_id = abs(area_boundaries->value[i]);
            if (line_written_out[boundary_id])
                continue;
            Vect_read_line(&input, points, boundary_cats, boundary_id);
            Vect_write_line(&output, GV_BOUNDARY, points, boundary_cats);
            line_written_out[boundary_id] = true;
        }

        if (field > 0) {
            Vect_field_cat_get(area_cats, field, field_cats);
            Vect_list_append_list(all_cats, field_cats);
        }
    }

    Vect_destroy_cats_struct(boundary_cats);
    Vect_destroy_cats_struct(area_cats);
    Vect_destroy_line_struct(points);

    /* Let's get vector layers db connections information */
    struct field_info *input_info = NULL;

    if (field > 0 && all_cats->n_values)
        input_info = Vect_get_field2(&input, options.field->answer);

    if (input_info) {
        G_verbose_message(_("Copying attributes for layer <%s>"),
                          options.field->answer);

        struct field_info *output_info =
            Vect_default_field_info(&output, field, NULL, GV_1TABLE);
        /* Create database for new vector map */
        dbDriver *driver = db_start_driver_open_database(output_info->driver,
                                                         output_info->database);
        Vect_map_add_dblink(&output, output_info->number, output_info->name,
                            output_info->table, input_info->key,
                            output_info->database, output_info->driver);

        /* Copy attribute table data */
        if (db_copy_table_by_ints(
                input_info->driver, input_info->database, input_info->table,
                output_info->driver,
                Vect_subst_var(output_info->database, &output),
                output_info->table, input_info->key, all_cats->value,
                all_cats->n_values) == DB_FAILED)
            G_fatal_error(
                _("Unable to copy attribute table to vector map <%s>"),
                options.output->answer);
        db_close_database_shutdown_driver(driver);
    }

    Vect_destroy_list(field_cats);
    Vect_destroy_list(all_cats);

    Vect_build(&output);
    Vect_close(&input);

    /* Build topology for vector map and close them */

    Vect_close(&output);

    exit(EXIT_SUCCESS);
}

/*
 ****************************************************************************
 *
 * MODULE:       v.decimate
 * AUTHOR(S):    Vaclav Petras
 * PURPOSE:      Reduce the number of points in a vector map
 * COPYRIGHT:    (C) 2015 by the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2). Read the file COPYING that
 *               comes with GRASS for details.
 *
 *****************************************************************************/


/* using the most-specific-first order of includes */
#include "count_decimation.h"
#include "grid_decimation.h"

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/vector.h>
#include <grass/glocale.h>

#include <stdlib.h>

void copy_tabs(const struct Map_info *In, struct Map_info *Out);

struct DecimationContext
{
    int use_z;                  /*!< TRUE or FALSE */
    double zdiff;
    int unique_cats;            /*!< TRUE or FALSE */
};


static int if_add_point(struct DecimationPoint *point, void *point_data,
                        struct DecimationPoint **point_list, size_t npoints,
                        void *context)
{
    /* according to cat (which could be cluster, return or class) */
    struct DecimationContext *dc = context;
    double zdiff = dc->zdiff;
    int j;

    /* TODO: use something like Vect_cat_in_cat_list? */
    for (j = 0; j < npoints; j++) {
        if (dc->use_z && fabs(point_list[j]->z - point->z) < zdiff)
            return FALSE;
        if (dc->unique_cats && point_list[j]->cat == point->cat)
            return FALSE;
    }
    return TRUE;
}


struct WriteContext
{
    struct Map_info *voutput;
    struct line_pnts *line;
    struct line_cats *cats;
    int write_cats;
};


static void write_point(struct WriteContext *context, int cat, double x,
                        double y, double z, struct line_cats *cats)
{
    if (Vect_append_point(context->line, x, y, z) != 1)
        G_fatal_error
            ("Unable to create a point in vector map (probably out of memory)");
    struct line_cats *cats_to_write = context->cats;
    /* only when writing cats use the ones from parameter, otherwise
     * use the default (which is assumed to be empty) */
    if (context->write_cats && cats)
        cats_to_write = cats;
    Vect_write_line(context->voutput, GV_POINT, context->line, cats_to_write);
    Vect_reset_line(context->line);
}


static void on_add_point(struct DecimationPoint *point, void *point_data, void *context)
{
    write_point((struct WriteContext *)context, point->cat, point->x, point->y,
                point->z, (struct line_cats *)point_data);
}

/* TODO: these have overlap with vector lib, really needed? */
static int point_in_region_2d(struct Cell_head *region, double x, double y)
{
    if (x > region->east || x < region->west || y < region->south ||
        y > region->north)
        return FALSE;
    return TRUE;
}


static int point_in_region_3d(struct Cell_head *region, double x, double y,
                              double z)
{
    if (x > region->east || x < region->west || y < region->south ||
        y > region->north || z > region->top || z < region->bottom)
        return FALSE;
    return TRUE;
}

/* TODO: max tries per grid cell (useful?) */


int main(int argc, char **argv)
{
    struct GModule *module;
    struct Option *map_opt, *voutput_opt;
    struct Option *field_opt, *cats_opt;
    struct Option *skip_opt, *preserve_opt, *offset_opt, *limit_opt;
    struct Option *zdiff_opt, *limit_per_cell_opt, *zrange_opt;
    struct Flag *grid_decimation_flg, *first_point_flg, *cat_in_grid_flg;
    struct Flag *use_z_flg, *nocats_flag, *notopo_flag, *notab_flag;
    struct Map_info vinput, voutput;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("LIDAR"));
    G_add_keyword(_("generalization"));
    G_add_keyword(_("decimation"));
    G_add_keyword(_("extract"));
    G_add_keyword(_("select"));
    G_add_keyword(_("points"));
    G_add_keyword(_("level1"));

    module->label = _("Decimates a point cloud");
    module->description = _("Copies points from one vector to another"
                            " while applying different decimations");

    map_opt = G_define_standard_option(G_OPT_V_INPUT);

    field_opt = G_define_standard_option(G_OPT_V_FIELD_ALL);
    field_opt->required = NO;

    voutput_opt = G_define_standard_option(G_OPT_V_OUTPUT);

    zrange_opt = G_define_option();
    zrange_opt->key = "zrange";
    zrange_opt->type = TYPE_DOUBLE;
    zrange_opt->required = NO;
    zrange_opt->key_desc = "min,max";
    zrange_opt->description = _("Filter range for z data (min,max)");
    zrange_opt->guisection = _("Selection");

    cats_opt = G_define_standard_option(G_OPT_V_CATS);
    cats_opt->guisection = _("Selection");

    /* TODO: -r region only (now enforced for grid), spatial */

    skip_opt = G_define_option();
    skip_opt->key = "skip";
    skip_opt->type = TYPE_INTEGER;
    skip_opt->multiple = NO;
    skip_opt->required = NO;
    skip_opt->label = _("Throw away every n-th point");
    skip_opt->description =
        _("For example, 5 will import 80 percent of points. "
          "If not specified, all points are copied");
    skip_opt->guisection = _("Count");

    preserve_opt = G_define_option();
    preserve_opt->key = "preserve";
    preserve_opt->type = TYPE_INTEGER;
    preserve_opt->multiple = NO;
    preserve_opt->required = NO;
    preserve_opt->label = _("Preserve only every n-th point");
    preserve_opt->description =
        _("For example, 4 will import 25 percent of points. "
          "If not specified, all points are copied");
    preserve_opt->guisection = _("Count");

    offset_opt = G_define_option();
    offset_opt->key = "offset";
    offset_opt->type = TYPE_INTEGER;
    offset_opt->multiple = NO;
    offset_opt->required = NO;
    offset_opt->label = _("Skip first n points");
    offset_opt->description =
        _("Skips the given number of points at the beginning.");
    offset_opt->guisection = _("Count");

    limit_opt = G_define_option();
    limit_opt->key = "limit";
    limit_opt->type = TYPE_INTEGER;
    limit_opt->multiple = NO;
    limit_opt->required = NO;
    limit_opt->label = _("Copy only n points");
    limit_opt->description = _("Copies only the given number of points");
    limit_opt->guisection = _("Count");

    zdiff_opt = G_define_option();
    zdiff_opt->key = "zdiff";
    zdiff_opt->type = TYPE_DOUBLE;
    zdiff_opt->required = NO;
    zdiff_opt->label = _("Minimal difference of z values");
    zdiff_opt->description =
        _("Minimal difference between z values in grid-based decimation");
    zdiff_opt->guisection = _("Grid");

    limit_per_cell_opt = G_define_option();
    limit_per_cell_opt->key = "cell_limit";
    limit_per_cell_opt->type = TYPE_INTEGER;
    limit_per_cell_opt->multiple = NO;
    limit_per_cell_opt->required = NO;
    limit_per_cell_opt->label = _("Preserve only n points per grid cell");
    limit_per_cell_opt->description =
        _("Preserves only the given number of points per grid cell in grid-based decimation");
    limit_per_cell_opt->guisection = _("Grid");

    grid_decimation_flg = G_define_flag();
    grid_decimation_flg->key = 'g';
    grid_decimation_flg->description = _("Apply grid-based decimation");
    grid_decimation_flg->guisection = _("Grid");

    first_point_flg = G_define_flag();
    first_point_flg->key = 'f';
    first_point_flg->description =
        _("Use only first point in grid cell during grid-based decimation");
    first_point_flg->guisection = _("Grid");

    cat_in_grid_flg = G_define_flag();
    cat_in_grid_flg->key = 'c';
    cat_in_grid_flg->description = _("Only one point per cat in grid cell");
    cat_in_grid_flg->guisection = _("Grid");

    use_z_flg = G_define_flag();
    use_z_flg->key = 'z';
    use_z_flg->description = _("Use z in grid decimation");
    use_z_flg->guisection = _("Grid");

    nocats_flag = G_define_flag();
    nocats_flag->key = 'x';
    nocats_flag->label =
        _("Store only the coordinates, throw away categories");
    nocats_flag->description =
        _("Do not story any categories even if they are present in input data");
    nocats_flag->guisection = _("Speed");

    notopo_flag = G_define_standard_flag(G_FLG_V_TOPO);
    notopo_flag->guisection = _("Speed");

    notab_flag = G_define_standard_flag(G_FLG_V_TABLE);
    notab_flag->guisection = _("Speed");

    /* here we have different decimations but also selections/filters */
    G_option_required(skip_opt, preserve_opt, offset_opt, limit_opt,
                      grid_decimation_flg, zrange_opt, cats_opt, NULL);
    /* this doesn't play well with GUI dialog unless we add defaults to options
     * the default values would solve it but looks strange in the manual 
     * this we use explicit check in the code */
    /* G_option_exclusive(skip_opt, preserve_opt, NULL); */
    G_option_requires(grid_decimation_flg, first_point_flg,
                      limit_per_cell_opt, use_z_flg, zdiff_opt,
                      cat_in_grid_flg, NULL);
    G_option_requires(first_point_flg, grid_decimation_flg, NULL);
    G_option_requires(limit_per_cell_opt, grid_decimation_flg, NULL);
    G_option_requires(use_z_flg, grid_decimation_flg, NULL);
    G_option_requires(zdiff_opt, grid_decimation_flg, NULL);
    G_option_requires(cat_in_grid_flg, grid_decimation_flg, NULL);
    G_option_exclusive(zdiff_opt, first_point_flg, limit_per_cell_opt, NULL);

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    Vect_check_input_output_name(map_opt->answer, voutput_opt->answer,
                                 G_FATAL_EXIT);

    if (Vect_open_old2(&vinput, map_opt->answer, "", field_opt->answer) < 0)
        G_fatal_error(_("Unable to open vector map <%s>"), map_opt->answer);
    int layer = Vect_get_field_number(&vinput, field_opt->answer);

    if (layer < 1 && (cats_opt->answer || cat_in_grid_flg->answer))
        G_fatal_error(_("Input layer must be set to a particular layer"
                        ", not <%s>, when using <%s> option or <-%c> flag"),
                      field_opt->answer, cats_opt->key, cat_in_grid_flg->key);

    struct cat_list *allowed_cats = NULL;

    if (layer > 0)
        allowed_cats = Vect_cats_set_constraint(&vinput, layer, NULL,
                                                cats_opt->answer);

    struct line_pnts *line = Vect_new_line_struct();
    struct line_cats *cats = Vect_new_cats_struct();

    double zrange_min, zrange_max;
    int use_zrange = FALSE;

    if (zrange_opt->answer != NULL) {
        if (zrange_opt->answers[0] == NULL || zrange_opt->answers[1] == NULL)
            G_fatal_error(_("Invalid zrange <%s>"), zrange_opt->answer);
        sscanf(zrange_opt->answers[0], "%lf", &zrange_min);
        sscanf(zrange_opt->answers[1], "%lf", &zrange_max);
        /* for convenience, switch order to make valid input */
        if (zrange_min > zrange_max) {
            double tmp = zrange_max;

            zrange_max = zrange_min;
            zrange_min = tmp;
        }
        use_zrange = TRUE;
    }

    int use_z = FALSE;
    double zdiff;

    if (use_z_flg->answer) {
        use_z = TRUE;
        zdiff = atof(zdiff_opt->answer);
    }

    /* z input checks */
    if (!Vect_is_3d(&vinput)) {
        if (use_z)
            G_fatal_error(_("Cannot use z for decimation, input is not 3D"));
        if (use_zrange)
            G_fatal_error(_("Cannot select by z range, input is not 3D"));
    }

    int do_grid_decimation = FALSE;

    if (grid_decimation_flg->answer)
        do_grid_decimation = TRUE;
    int limit_per_cell = 0;

    if (limit_per_cell_opt->answer)
        limit_per_cell = atof(limit_per_cell_opt->answer);
    if (first_point_flg->answer)
        limit_per_cell = 1;

    /* when user enters 1 or zero e.g. for skip, we accept it and use it
     * but the is no advantage, however, we count it as an error when
     * no other options are selected
     */
    struct CountDecimationControl count_decimation_control;

    count_decimation_init_from_str(&count_decimation_control,
                                   skip_opt->answer, preserve_opt->answer,
                                   offset_opt->answer, limit_opt->answer);
    if (!count_decimation_is_valid(&count_decimation_control))
        G_fatal_error(_("Settings for count-based decimation are not valid"));
    /* TODO: implement count_decimation_is_invalid_reason() */
    /* the following must be in sync with required options */
    if (count_decimation_is_noop(&count_decimation_control) &&
        !grid_decimation_flg->answer && !zrange_opt->answer &&
        !cats_opt->answer)
        G_fatal_error(_("Settings for count-based decimation would cause it"
                        " to do nothing and no other options has been set."));

    struct Cell_head comp_region;
    Rast_get_window(&comp_region);
    if (use_zrange) {
        comp_region.bottom = zrange_min;
        comp_region.top = zrange_max;
    }
    struct GridDecimation grid_decimation;
    struct DecimationContext decimation_context;
    struct WriteContext write_context;

    write_context.line = Vect_new_line_struct();
    write_context.cats = Vect_new_cats_struct();
    if (!nocats_flag->answer)
        write_context.write_cats = TRUE;
    else
        write_context.write_cats = FALSE;
    write_context.voutput = &voutput;
    if (do_grid_decimation) {
        grid_decimation_create_from_region(&grid_decimation, &comp_region);
        grid_decimation.max_points = limit_per_cell;

        if (cat_in_grid_flg->answer)
            decimation_context.unique_cats = TRUE;
        else
            decimation_context.unique_cats = FALSE;
        decimation_context.use_z = FALSE;
        if (use_z) {
            decimation_context.use_z = TRUE;
            decimation_context.zdiff = zdiff;
        }
        grid_decimation.if_add_point = if_add_point;
        grid_decimation.if_context = &decimation_context;

        grid_decimation.on_add_point = on_add_point;
        grid_decimation.on_context = &write_context;
    }

    if (Vect_open_new(&voutput, voutput_opt->answer, Vect_is_3d(&vinput)) < 0)
        G_fatal_error(_("Unable to create vector map <%s>"),
                      voutput_opt->answer);

    /* some constraints can be set on the map */
    Vect_set_constraint_type(&vinput, GV_POINT);
    /* noop for layer=-1 and non-native format, skips lines without cat */
    Vect_set_constraint_field(&vinput, layer);
    /* TODO: replace region checks by Vect_set_constraint_region? */

    int ltype;
    int cat;

    while (TRUE) {
        ltype = Vect_read_next_line(&vinput, line, cats);
        if (ltype == -1)
            G_fatal_error(_("Unable to read vector map"));
        if (ltype == -2)
            break;              /* end of the map */

        double x, y, z;
        Vect_line_get_point(line, 0, &x, &y, &z);

        /* selections/filters */
        /* TODO: use region only when actually needed */
        if (!use_zrange && !point_in_region_2d(&comp_region, x, y))
            continue;
        /* TODO: allow zrange to be used without region */
        if (use_zrange && !point_in_region_3d(&comp_region, x, y, z))
            continue;
        if (layer > 0 && allowed_cats &&
            !Vect_cats_in_constraint(cats, layer, allowed_cats))
            continue;

        /* decimation */
        if (count_decimation_is_out(&count_decimation_control))
            continue;

        /* TODO: test: skip points without category, unless layer=-1 */
        /* Use cases:
         * - all points have category (correct)
         * - no categories for any point (correct, layer=-1 required)
         * - some points miss category (not handled)
         * Here we assume that only one cat has meaning for grid decimation.
         * If no layer available, cat contains junk and shouldn't be used.
	 * 
	 * TODO done
         */
	cat = -1;
        if (layer > 0) {
	    if (allowed_cats) {
		int i;

		for (i = 0; i < cats->n_cats; i++) {
		    if (cats->field[i] == layer &&
			Vect_cat_in_cat_list(cats->cat[i], allowed_cats)) {
			cat = cats->cat[i];
			break;
		    }
		}
		return 0;
	    }
	    else
		Vect_cat_get(cats, layer, &cat);
	    if (cat < 0)
		continue;
	}

        /* using callback when using grid, direct call otherwise */
        if (do_grid_decimation)
            grid_decimation_try_add_point(&grid_decimation, cat, x, y, z,
                                          cats);
        else
            write_point(&write_context, cat, x, y, z, cats);

        if (count_decimation_is_end(&count_decimation_control))
            break;
    }

    /* partially unnecessary as deallocated by the system */
    Vect_destroy_line_struct(line);
    Vect_destroy_cats_struct(cats);
    Vect_destroy_line_struct(write_context.line);
    Vect_destroy_cats_struct(write_context.cats);

    Vect_hist_command(&voutput);

    Vect_close(&vinput);
    if (!notopo_flag->answer) {
        Vect_build(&voutput);
	if (write_context.write_cats == TRUE && !notab_flag->answer) {
	    copy_tabs(&vinput, &voutput);
	}
    }
    Vect_close(&voutput);

    if (do_grid_decimation)
        grid_decimation_destroy(&grid_decimation);

    return EXIT_SUCCESS;
}

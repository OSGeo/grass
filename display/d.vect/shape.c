#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/raster.h>
#include <grass/glocale.h>

#include "local_proto.h"

static int ncolor_rules_skipped = 0;

int get_num_color_rules_skipped(void)
{
    return ncolor_rules_skipped;
}

int display_shape(struct Map_info *Map, int type, struct cat_list *Clist,
                  const struct Cell_head *window,
                  const struct color_rgb *bcolor,
                  const struct color_rgb *fcolor, int chcat, const char *icon,
                  double size, const char *size_column, int sqrt_flag,
                  const char *rot_column, /* lines only */
                  int id_flag, int cats_colors_flag, char *rgb_column,
                  int default_width, char *width_column, double width_scale,
                  char *z_style)
{
    int open_db, field, i, stat;
    dbCatValArray cvarr_rgb, cvarr_width, cvarr_size, cvarr_rot;
    struct field_info *fi = NULL;
    dbDriver *driver;
    int nrec_rgb, nrec_width, nrec_size, nrec_rot, have_colors;
    struct Colors colors, zcolors;
    struct bound_box box;

    stat = 0;
    nrec_rgb = nrec_width = nrec_size = nrec_rot = 0;

    open_db = rgb_column || width_column || size_column || rot_column;
    if (open_db) {
        field = Clist->field > 0 ? Clist->field : 1;
        fi = Vect_get_field(Map, field);
        if (!fi) {
            G_fatal_error(_("Database connection not defined for layer %d"),
                          field);
        }

        driver = db_start_driver_open_database(fi->driver, fi->database);
        if (!driver)
            G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
                          fi->database, fi->driver);
        db_set_error_handler_driver(driver);
    }

    /* first search for color table */
    have_colors =
        Vect_read_colors(Vect_get_name(Map), Vect_get_mapset(Map), &colors);
    if (have_colors && rgb_column) {
        G_warning(_("Both color table and <%s> option detected. "
                    "Color table will ignored."),
                  "rgb_column");
        have_colors = FALSE;
    }

    if (rgb_column) {
        /* read RRR:GGG:BBB color strings from table */
        db_CatValArray_init(&cvarr_rgb);

        nrec_rgb = db_select_CatValArray(driver, fi->table, fi->key, rgb_column,
                                         NULL, &cvarr_rgb);

        G_debug(3, "nrec_rgb (%s) = %d", rgb_column, nrec_rgb);

        if (cvarr_rgb.ctype != DB_C_TYPE_STRING) {
            G_warning(_("Color definition column ('%s') not a string. "
                        "Column must be of form 'RRR:GGG:BBB' where RGB values "
                        "range 0-255. "
                        "You can use '%s' module to define color rules. "
                        "Unable to colorize features."),
                      rgb_column, "v.colors");
            rgb_column = NULL;
        }
        else {
            if (nrec_rgb < 0)
                G_fatal_error(_("Unable to select data ('%s') from table"),
                              rgb_column);

            G_debug(2, "\n%d records selected from table", nrec_rgb);
        }
    }
    if (width_column) {
        if (*width_column == '\0')
            G_fatal_error(_("Line width column not specified"));

        db_CatValArray_init(&cvarr_width);

        nrec_width = db_select_CatValArray(driver, fi->table, fi->key,
                                           width_column, NULL, &cvarr_width);

        G_debug(3, "nrec_width (%s) = %d", width_column, nrec_width);

        if (cvarr_width.ctype != DB_C_TYPE_INT &&
            cvarr_width.ctype != DB_C_TYPE_DOUBLE)
            G_fatal_error(_("Line width column ('%s') not a number"),
                          width_column);

        if (nrec_width < 0)
            G_fatal_error(_("Unable to select data ('%s') from table"),
                          width_column);

        G_debug(2, "\n%d records selected from table", nrec_width);

        for (i = 0; i < cvarr_width.n_values; i++) {
            G_debug(4, "cat = %d  %s = %d", cvarr_width.value[i].cat,
                    width_column,
                    (cvarr_width.ctype == DB_C_TYPE_INT
                         ? cvarr_width.value[i].val.i
                         : (int)cvarr_width.value[i].val.d));
        }
    }

    if (size_column) {
        if (*size_column == '\0')
            G_fatal_error(_("Symbol size column not specified"));

        db_CatValArray_init(&cvarr_size);

        nrec_size = db_select_CatValArray(driver, fi->table, fi->key,
                                          size_column, NULL, &cvarr_size);

        G_debug(3, "nrec_size (%s) = %d", size_column, nrec_size);

        if (cvarr_size.ctype != DB_C_TYPE_INT &&
            cvarr_size.ctype != DB_C_TYPE_DOUBLE)
            G_fatal_error(_("Symbol size column ('%s') is not numeric"),
                          size_column);

        if (nrec_size < 0)
            G_fatal_error(_("Unable to select data ('%s') from table"),
                          size_column);

        G_debug(2, " %d records selected from table", nrec_size);

        for (i = 0; i < cvarr_size.n_values; i++) {
            G_debug(4, "(size) cat = %d  %s = %.2f", cvarr_size.value[i].cat,
                    size_column,
                    (cvarr_size.ctype == DB_C_TYPE_INT
                         ? (double)cvarr_size.value[i].val.i
                         : cvarr_size.value[i].val.d));
        }
    }

    if (rot_column) {
        if (*rot_column == '\0')
            G_fatal_error(_("Symbol rotation column not specified"));

        db_CatValArray_init(&cvarr_rot);

        nrec_rot = db_select_CatValArray(driver, fi->table, fi->key, rot_column,
                                         NULL, &cvarr_rot);

        G_debug(3, "nrec_rot (%s) = %d", rot_column, nrec_rot);

        if (cvarr_rot.ctype != DB_C_TYPE_INT &&
            cvarr_rot.ctype != DB_C_TYPE_DOUBLE)
            G_fatal_error(_("Symbol rotation column ('%s') is not numeric"),
                          rot_column);

        if (nrec_rot < 0)
            G_fatal_error(_("Unable to select data ('%s') from table"),
                          rot_column);

        G_debug(2, " %d records selected from table", nrec_rot);

        for (i = 0; i < cvarr_rot.n_values; i++) {
            G_debug(4, "(rot) cat = %d  %s = %.2f", cvarr_rot.value[i].cat,
                    rot_column,
                    (cvarr_rot.ctype == DB_C_TYPE_INT
                         ? (double)cvarr_rot.value[i].val.i
                         : cvarr_rot.value[i].val.d));
        }
    }

    if (z_style) {
        if (!Vect_is_3d(Map)) {
            G_warning(_("Vector map is not 3D. Unable to colorize features "
                        "based on z-coordinates."));
            z_style = NULL;
        }
        else if (rgb_column) {
            /* this should not happen, zcolor and rgb_columns are mutually
             * exclusive */
            z_style = NULL;
        }
        else {
            int ret;

            ret = 0;
            if (Vect_level(Map) > 1)
                ret = Vect_get_map_box(Map, &box);
            else
                ret = Vect_get_map_box1(Map, &box);

            if (ret == 1)
                Rast_make_fp_colors(&zcolors, z_style, box.B, box.T);
            else
                G_warning(
                    _("Unable to colorize features, unknown map bounding box"));
        }
    }

    stat = 0;
    if (type & GV_AREA &&
        Vect_get_num_primitives(Map, GV_CENTROID | GV_BOUNDARY) > 0)
        stat += display_area(
            Map, Clist, window, bcolor, fcolor, chcat, id_flag,
            cats_colors_flag, default_width, width_scale,
            z_style ? &zcolors : NULL, rgb_column ? &cvarr_rgb : NULL,
            have_colors ? &colors : NULL, &cvarr_width, nrec_width);

    stat += display_lines(
        Map, type, Clist, bcolor, fcolor, chcat, icon, size, sqrt_flag, id_flag,
        cats_colors_flag, default_width, width_scale, z_style ? &zcolors : NULL,
        rgb_column ? &cvarr_rgb : NULL, have_colors ? &colors : NULL,
        &cvarr_width, nrec_width, &cvarr_size, nrec_size, &cvarr_rot, nrec_rot);
    if (open_db) {
        db_close_database_shutdown_driver(driver);
        Vect_destroy_field_info(fi);
    }

    return stat;
}

int get_table_color(int cat, int line, struct Colors *colors,
                    dbCatValArray *cvarr, int *red, int *grn, int *blu)
{
    int custom_rgb;
    char colorstring[12]; /* RRR:GGG:BBB */

    dbCatVal *cv;

    custom_rgb = FALSE;
    cv = NULL;

    if (cat < 0)
        return custom_rgb;

    if (colors) {
        /* read color table */
        if (Rast_get_c_color(&cat, red, grn, blu, colors) == 1) {
            custom_rgb = TRUE;
            G_debug(3, "\tb: %d, g: %d, r: %d", *blu, *grn, *red);
        }
    }

    /* read RGB colors from db for current area # */
    if (cvarr && db_CatValArray_get_value(cvarr, cat, &cv) == DB_OK) {
        sprintf(colorstring, "%s", db_get_string(cv->val.s));
        if (*colorstring != '\0') {
            G_debug(3, "element %d: colorstring: %s", line, colorstring);

            if (G_str_to_color(colorstring, red, grn, blu) == 1) {
                custom_rgb = TRUE;
                G_debug(3, "element:%d  cat %d r:%d g:%d b:%d", line, cat, *red,
                        *grn, *blu);
            }
            else {
                G_debug(3, "Invalid color definition '%s' ignored",
                        colorstring);
                ncolor_rules_skipped++;
            }
        }
        else {
            G_debug(3, "Invalid color definition '%s' ignored", colorstring);
            ncolor_rules_skipped++;
        }
    }

    return custom_rgb;
}

int get_cat_color(int line, const struct line_cats *Cats,
                  const struct cat_list *Clist, int *red, int *grn, int *blu)
{
    int custom_rgb;
    unsigned char which;
    int cat;

    custom_rgb = FALSE;
    if (Clist->field > 0) {
        Vect_cat_get(Cats, Clist->field, &cat);
        if (cat >= 0) {
            G_debug(3, "display element %d, cat %d", line, cat);
            /* fetch color number from category */
            which = (cat % palette_ncolors);
            G_debug(3, "cat:%d which color:%d r:%d g:%d b:%d", cat, which,
                    palette[which].R, palette[which].G, palette[which].B);

            custom_rgb = TRUE;
            *red = palette[which].R;
            *grn = palette[which].G;
            *blu = palette[which].B;
        }
    }
    else if (Cats->n_cats > 0) {
        /* fetch color number from layer */
        which = (Cats->field[0] % palette_ncolors);
        G_debug(3, "layer:%d which color:%d r:%d g:%d b:%d", Cats->field[0],
                which, palette[which].R, palette[which].G, palette[which].B);

        custom_rgb = TRUE;
        *red = palette[which].R;
        *grn = palette[which].G;
        *blu = palette[which].B;
    }

    return custom_rgb;
}

double get_property(int cat, int line, dbCatValArray *cvarr, double scale,
                    double default_value)
{
    double value;
    dbCatVal *cv;

    cv = NULL;

    if (cat < 0)
        return default_value;

    /* Read line width from db for current area # */
    if (db_CatValArray_get_value(cvarr, cat, &cv) != DB_OK) {
        value = default_value;
    }
    else {
        value = scale *
                (cvarr->ctype == DB_C_TYPE_INT ? (double)cv->val.i : cv->val.d);
        if (value < 0) {
            G_important_message(
                _("Invalid negative value - feature %d with category %d"), line,
                cat);
            value = default_value;
        }
    }

    return value;
}

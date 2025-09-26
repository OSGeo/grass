#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/raster.h>
#include <grass/glocale.h>

#include "local_proto.h"

int scan_attr(struct Map_info *Map, int layer, const char *column_name,
              const char *style, const char *rules, const struct FPRange *range,
              struct Colors *colors, struct Colors *rcolors, int invert)
{
    int ctype, is_fp, nrec;
    double fmin, fmax;

    struct field_info *fi;
    struct Colors vcolors;
    dbDriver *driver;
    dbCatValArray cvarr;

    Rast_init_colors(colors);

    fi = Vect_get_field(Map, layer);
    if (!fi)
        G_fatal_error(_("Database connection not defined for layer %d"), layer);

    driver = db_start_driver_open_database(fi->driver, fi->database);
    if (!driver)
        G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
                      fi->database, fi->driver);
    db_set_error_handler_driver(driver);

    ctype = db_column_Ctype(driver, fi->table, column_name);
    if (ctype == -1)
        G_fatal_error(_("Column <%s> not found in table <%s>"), column_name,
                      fi->table);
    if (ctype != DB_C_TYPE_INT && ctype != DB_C_TYPE_DOUBLE)
        G_fatal_error(_("Column <%s> is not numeric"), column_name);

    is_fp = ctype == DB_C_TYPE_DOUBLE;

    nrec = db_select_CatValArray(driver, fi->table, fi->key, column_name, NULL,
                                 &cvarr);
    if (nrec < 1) {
        G_important_message(_("No data selected"));
        Vect_destroy_field_info(fi);
        db_close_database(driver);
        return 0;
    }

    /* color table for values */
    db_CatValArray_sort_by_value(&cvarr);
    if (is_fp) {
        fmin = cvarr.value[0].val.d;
        fmax = cvarr.value[cvarr.n_values - 1].val.d;

        if (range) {
            if (range->min >= fmin && range->min <= fmax)
                fmin = range->min;
            else
                G_warning(_("Min value (%f) is out of range %f,%f"), range->min,
                          fmin, fmax);

            if (range->max <= fmax && range->max >= fmin)
                fmax = range->max;
            else
                G_warning(_("Max value (%f) is out of range %f,%f"), range->max,
                          fmin, fmax);
        }
    }
    else {
        fmin = cvarr.value[0].val.i;
        fmax = cvarr.value[cvarr.n_values - 1].val.i;

        if (range) {
            if (range->min >= fmin && range->min <= fmax)
                fmin = range->min;
            else
                G_warning(_("Min value (%d) is out of range %d,%d"),
                          (int)range->min, (int)fmin, (int)fmax);

            if (range->max <= fmax && range->max >= fmin)
                fmax = range->max;
            else
                G_warning(_("Max value (%d) is out of range %d,%d"),
                          (int)range->max, (int)fmin, (int)fmax);
        }
    }

    if (rcolors)
        /* color table for categories */
        color_rules_to_cats(&cvarr, is_fp, rcolors, colors, invert, (DCELL)fmin,
                            (DCELL)fmax);
    else {
        if (style)
            make_colors(&vcolors, style, (DCELL)fmin, (DCELL)fmax, is_fp);
        else if (rules)
            load_colors(&vcolors, rules, (DCELL)fmin, (DCELL)fmax, is_fp);

        /* color table for categories */
        color_rules_to_cats(&cvarr, is_fp, &vcolors, colors, invert,
                            (DCELL)fmin, (DCELL)fmax);
    }

    db_close_database(driver);
    Vect_destroy_field_info(fi);

    return is_fp;
}

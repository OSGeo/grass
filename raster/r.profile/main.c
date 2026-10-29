/*
 * SPDX-FileCopyrightText: 2000 GRASS Development Team
 * SPDX-License-Identifier: GPL-2.0-or-later
 * Author: Bob Covill <bcovill@tekmap.ns.ca>
 *
 */

#include <stdlib.h>
#include <grass/gis.h>
#include <grass/gjson.h>
#include <grass/glocale.h>
#include <grass/raster.h>
#include <grass/vector.h>

#include "local_proto.h"

int clr;
struct Colors colors;

/* Running state of the profile being built: cumulative distance and the
 * last sampled position, carried over from one segment to the next. */
static double dist, e, n;
static int have_last;
char *fs;

static void profile_segment(double e1, double e2, double n1, double n2,
                            double res, const struct profile_output *out);
static void profile_vector_lines(const char *name, const char *layer_name,
                                 char *where, char *cats, double res,
                                 const struct profile_output *out);

int main(int argc, char *argv[])
{
    char *name, *outfile;
    double res;
    char ebuf[256], nbuf[256], label[512];
    char b1[100], b2[100];
    int n;
    int havefirst = FALSE;
    int i, k = -1;
    double e1, e2, n1, n2;
    FILE *coor_fp;
    struct {
        struct Option *opt1, *profile, *res, *output, *null_str, *coord_file,
            *lines, *layer, *cats, *where, *units, *format, *color_format, *fs;
        struct Flag *g, *c;
    } parm;
    struct GModule *module;
    struct profile_output out = {0};
    G_JSON_Value *array_value;

    G_gisinit(argv[0]);

    /* Set description */
    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("profile"));
    G_add_keyword(_("transect"));
    module->description =
        _("Outputs the raster map layer values lying on user-defined line(s).");

    parm.opt1 = G_define_standard_option(G_OPT_R_INPUT);

    parm.output = G_define_standard_option(G_OPT_F_OUTPUT);
    parm.output->required = NO;
    parm.output->answer = "-";
    parm.output->description =
        _("Name of file for output (use output=- for stdout)");

    parm.profile = G_define_standard_option(G_OPT_M_COORDS);
    parm.profile->required = NO;
    parm.profile->multiple = YES;
    parm.profile->description = _("Profile coordinate pairs");

    parm.coord_file = G_define_standard_option(G_OPT_F_INPUT);
    parm.coord_file->key = "file";
    parm.coord_file->required = NO;
    parm.coord_file->label =
        _("Name of input file containing coordinate pairs");
    parm.coord_file->description = _("Use instead of the 'coordinates' option. "
                                     "\"-\" reads from stdin.");

    parm.lines = G_define_standard_option(G_OPT_V_INPUT);
    parm.lines->key = "lines";
    parm.lines->required = NO;
    parm.lines->label = _("Name of input vector map with profile lines");
    parm.lines->description =
        _("Each line is profiled separately. Use instead of the "
          "'coordinates' option.");

    parm.layer = G_define_standard_option(G_OPT_V_FIELD);
    parm.layer->guisection = _("Selection");

    parm.cats = G_define_standard_option(G_OPT_V_CATS);
    parm.cats->guisection = _("Selection");

    parm.where = G_define_standard_option(G_OPT_DB_WHERE);
    parm.where->guisection = _("Selection");

    parm.res = G_define_option();
    parm.res->key = "resolution";
    parm.res->type = TYPE_DOUBLE;
    parm.res->required = NO;
    parm.res->description =
        _("Resolution along profile (default = current region resolution)");

    parm.null_str = G_define_standard_option(G_OPT_M_NULL_VALUE);
    parm.null_str->answer = "*";

    parm.g = G_define_flag();
    parm.g->key = 'g';
    parm.g->description = _("Output easting and northing in first two columns "
                            "of four column output");

    parm.c = G_define_flag();
    parm.c->key = 'c';
    parm.c->description =
        _("Output color values for each profile point (format controlled by "
          "color_format option; default is 'triplet' for plain output, 'hex' "
          "for JSON)");

    parm.units = G_define_standard_option(G_OPT_M_UNITS);
    parm.units->options = "meters,kilometers,feet,miles";
    parm.units->label = parm.units->description;
    parm.units->description =
        _("If units are not specified, current project units are used. "
          "Meters are used by default in geographic (latlon) projects.");

    parm.format = G_define_standard_option(G_OPT_F_FORMAT);
    parm.format->options = "plain,csv,json";
    parm.format->descriptions = ("plain;Human readable text output;"
                                 "csv;CSV (Comma Separated Values);"
                                 "json;JSON (JavaScript Object Notation);");
    parm.format->guisection = _("Print");

    parm.color_format = G_define_standard_option(G_OPT_C_FORMAT);
    parm.color_format->required = NO;
    parm.color_format->answer = NULL;
    parm.color_format->guisection = _("Color");

    parm.fs = G_define_standard_option(G_OPT_F_SEP);
    parm.fs->answer = "comma";
    parm.fs->guisection = _("Formatting");

    G_option_required(parm.profile, parm.coord_file, parm.lines, NULL);
    G_option_exclusive(parm.profile, parm.coord_file, parm.lines, NULL);
    G_option_requires(parm.cats, parm.lines, NULL);
    G_option_requires(parm.where, parm.lines, NULL);

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    clr = 0;
    if (parm.c->answer)
        clr = 1; /* color output */

    out.null_string = parm.null_str->answer;
    out.coords = parm.g->answer ? 1 : 0;
    out.with_cat = parm.lines->answer ? 1 : 0;

    /* get conversion factor and units name */
    if (parm.units->answer) {
        int unit_id = G_units(parm.units->answer);

        out.factor = 1. / G_meters_to_units_factor(unit_id);
        out.unit = G_get_units_name(unit_id, 1, 0);
    }
    /* keep meters in case of latlon */
    else if (G_projection() == PROJECTION_LL) {
        out.factor = 1;
        out.unit = "meters";
    }
    else {
        /* get conversion factor to current units */
        out.unit = G_database_unit_name(1);
        out.factor = G_database_units_to_meters_factor();
    }

    if (parm.res->answer) {
        res = atof(parm.res->answer);
        /* Catch bad resolution ? */
        if (res <= 0)
            G_fatal_error(_("Illegal resolution %g [%s]"), res / out.factor,
                          out.unit);
    }
    else {
        struct Cell_head window;

        /* Do average of EW and NS res */
        G_get_window(&window);
        res = (window.ew_res + window.ns_res) / 2;
    }

    if (strcmp(parm.format->answer, "json") == 0) {
        out.format = JSON;
        array_value = G_json_value_init_array();
        out.array = G_json_array(array_value);
    }
    else if (strcmp(parm.format->answer, "csv") == 0) {
        out.format = CSV;
    }
    else {
        out.format = PLAIN;

        G_message(_("Using resolution: %g [%s]"), res / out.factor, out.unit);
    }

    fs = G_option_to_separator(parm.fs);

    if (clr) {
        if (parm.color_format->answer == NULL ||
            parm.color_format->answer[0] == '\0') {
            if (out.format == PLAIN)
                parm.color_format->answer = "triplet";
            else
                parm.color_format->answer = "hex";
        }

        out.clr_frmt = G_option_to_color_format(parm.color_format);
    }

    G_begin_distance_calculations();

    /* Open Input File for reading */
    /* Get Input Name */
    name = parm.opt1->answer;

    /* Open Raster File */
    out.fd = Rast_open_old(name, "");

    /* initialize color structure */
    if (clr)
        Rast_read_colors(name, "", &colors);

    /* Open ASCII file for output or stdout */
    outfile = parm.output->answer;

    if ((strcmp("-", outfile)) == 0) {
        out.fp = stdout;
    }
    else if (NULL == (out.fp = fopen(outfile, "w")))
        G_fatal_error(_("Unable to open file <%s>"), outfile);

    /* Get Raster Type */
    out.data_type = Rast_get_map_type(out.fd);
    /* Done with file */

    if (out.format == PLAIN) {
        char *dist_column;

        /* Show message giving output format */
        G_message(_("Output columns:"));
        G_asprintf(&dist_column, _("Along track dist. [%s], Elevation"),
                   out.unit);
        G_message("%s%s%s%s", out.with_cat ? _("Category, ") : "",
                  out.coords ? _("Easting, Northing, ") : "", dist_column,
                  clr ? _(" RGB color") : "");
        G_free(dist_column);
    }
    else if (out.format == CSV) {
        /* CSV Header */
        if (out.with_cat)
            fprintf(out.fp, "%s%s", "cat", fs);
        if (out.coords)
            fprintf(out.fp, "%s%s%s%s", "easting", fs, "northing", fs);
        fprintf(out.fp, "%s%s%s", "distance", fs, "value");
        if (clr)
            fprintf(out.fp, "%s%s", fs, "color");
        fprintf(out.fp, "\n");
    }

    start_profile();

    /* Get Profile Start Coords */
    if (parm.lines->answer) {
        profile_vector_lines(parm.lines->answer, parm.layer->answer,
                             parm.where->answer, parm.cats->answer, res, &out);
    }
    else if (parm.coord_file->answer) {
        if (strcmp("-", parm.coord_file->answer) == 0)
            coor_fp = stdin;
        else
            coor_fp = fopen(parm.coord_file->answer, "r");

        if (coor_fp == NULL)
            G_fatal_error(_("Could not open <%s>"), parm.coord_file->answer);

        for (n = 1; input(b1, ebuf, b2, nbuf, label, coor_fp); n++) {
            G_debug(4, "stdin line %d: ebuf=[%s]  nbuf=[%s]", n, ebuf, nbuf);
            if (!G_scan_easting(ebuf, &e2, G_projection()) ||
                !G_scan_northing(nbuf, &n2, G_projection()))
                G_fatal_error(_("Invalid coordinates %s %s"), ebuf, nbuf);

            if (havefirst)
                profile_segment(e1, e2, n1, n2, res, &out);
            e1 = e2;
            n1 = n2;
            havefirst = TRUE;
        }

        if (coor_fp != stdin)
            fclose(coor_fp);
    }
    else {
        /* Coords given on the Command Line using the profile= option */
        for (i = 0; parm.profile->answers[i]; i += 2) {
            /* Test for number coordinate pairs */
            k = i;
        }

        if (k == 0) {
            /* Only one coordinate pair supplied */
            G_scan_easting(parm.profile->answers[0], &e1, G_projection());
            G_scan_northing(parm.profile->answers[1], &n1, G_projection());
            e2 = e1;
            n2 = n1;

            /* Get profile info */
            profile_segment(e1, e2, n1, n2, res, &out);
        }
        else {
            for (i = 0; i <= k - 2; i += 2) {
                G_scan_easting(parm.profile->answers[i], &e1, G_projection());
                G_scan_northing(parm.profile->answers[i + 1], &n1,
                                G_projection());
                G_scan_easting(parm.profile->answers[i + 2], &e2,
                               G_projection());
                G_scan_northing(parm.profile->answers[i + 3], &n2,
                                G_projection());

                /* Get profile info */
                profile_segment(e1, e2, n1, n2, res, &out);
            }
        }
    }

    if (out.format == JSON) {
        char *serialized_string =
            G_json_serialize_to_string_pretty(array_value);
        if (serialized_string == NULL) {
            G_fatal_error(_("Failed to initialize pretty JSON string."));
        }
        puts(serialized_string);
        G_json_free_serialized_string(serialized_string);
        G_json_value_free(array_value);
    }

    Rast_close(out.fd);
    fclose(out.fp);

    if (clr)
        Rast_free_colors(&colors);

    exit(EXIT_SUCCESS);
} /* Done with main */

/* Profile one segment given by the user as a pair of coordinates. */
static void profile_segment(double e1, double e2, double n1, double n2,
                            double res, const struct profile_output *out)
{
    if (out->format == PLAIN)
        G_message(_("Approx. transect length: %f [%s]"),
                  G_distance(e1, n1, e2, n2) / out->factor, out->unit);

    if (!G_point_in_region(e2, n2))
        G_warning(
            _("Endpoint coordinates are outside of current region settings"));

    do_profile(e1, e2, n1, n2, res, out);
}

/* Profile every line of a vector map, each as a separate profile
 * starting at distance zero. Only lines matching the category
 * selection (if any) are used; points, centroids and boundaries are
 * skipped. */
static void profile_vector_lines(const char *name, const char *layer_name,
                                 char *where, char *cats, double res,
                                 const struct profile_output *out)
{
    struct Map_info Map;
    struct line_pnts *Points = Vect_new_line_struct();
    struct line_cats *Cats = Vect_new_cats_struct();
    struct cat_list *cat_list = NULL;
    struct profile_output line_out = *out;
    struct Cell_head window;
    struct bound_box region_box, line_box;
    int select = where || cats;
    int layer, type, nlines = 0, nselected = 0, ncategorized = 0;
    char label[128];

    Vect_set_open_level(1); /* topology not required */
    if (Vect_open_old2(&Map, name, "", layer_name) < 0)
        G_fatal_error(_("Unable to open vector map <%s>"), name);
    Vect_set_constraint_type(&Map, GV_LINE);

    layer = Vect_get_field_number(&Map, layer_name);
    if (select) {
        if (layer < 1)
            G_fatal_error(_("Options <cats> and <where> require a valid "
                            "layer, not <%s>"),
                          layer_name);
        /* NULL means nothing usable was selected, e.g. a malformed
         * cats value, not the absence of a constraint. */
        cat_list = Vect_cats_set_constraint(&Map, layer, where, cats);
    }

    G_get_window(&window);
    Vect_region_box(&window, &region_box);

    while ((type = Vect_read_next_line(&Map, Points, Cats)) > 0) {
        nlines++;
        if (select &&
            (!cat_list || !Vect_cats_in_constraint(Cats, layer, cat_list)))
            continue;

        if (Vect_cat_get(Cats, layer, &line_out.cat))
            ncategorized++;
        /* Repeated vertices would be sampled twice */
        Vect_line_prune(Points);

        /* Categories need not be unique or present, so messages name
         * the line by its position in the map as well. */
        if (line_out.cat < 0)
            snprintf(label, sizeof(label), _("Line %d (no category)"), nlines);
        else
            snprintf(label, sizeof(label), _("Line %d (category %d)"), nlines,
                     line_out.cat);
        if (out->format == PLAIN)
            G_message(_("%s: approx. length %f [%s]"), label,
                      Vect_line_geodesic_length(Points) / out->factor,
                      out->unit);
        Vect_line_box(Points, &line_box);
        if (line_box.W < region_box.W || line_box.E > region_box.E ||
            line_box.S < region_box.S || line_box.N > region_box.N)
            G_warning(_("%s is not fully inside the current region"), label);

        start_profile();
        if (Points->n_points == 1)
            do_profile(Points->x[0], Points->x[0], Points->y[0], Points->y[0],
                       res, &line_out);
        for (int i = 0; i < Points->n_points - 1; i++)
            do_profile(Points->x[i], Points->x[i + 1], Points->y[i],
                       Points->y[i + 1], res, &line_out);
        nselected++;
    }
    if (type == -1)
        G_fatal_error(_("Unable to read vector map <%s>"),
                      Vect_get_full_name(&Map));
    if (nlines == 0)
        G_warning(_("No lines found in vector map <%s>"),
                  Vect_get_full_name(&Map));
    else if (nselected == 0)
        G_warning(_("No lines selected from vector map <%s>"),
                  Vect_get_full_name(&Map));
    else if (ncategorized == 0)
        G_warning(_("No selected line has a category in layer <%s>"),
                  layer_name);

    Vect_close(&Map);
    Vect_destroy_line_struct(Points);
    Vect_destroy_cats_struct(Cats);
    if (cat_list)
        Vect_destroy_cat_list(cat_list);
}

/* Start a new profile: the next segment begins at distance zero. */
void start_profile(void)
{
    dist = 0;
    have_last = FALSE;
}

/* Calculate the Profile Now */
/* Establish parameters */
int do_profile(double e1, double e2, double n1, double n2, double res,
               const struct profile_output *out)
{
    double rows, cols;
    double Y, X, k;

    cols = e1 - e2;
    rows = n1 - n2;

    /* Calculate Azimuth of Line */
    if (rows == 0 && cols == 0) {
        /* Special case for no movement */
        e = e1;
        n = n1;
        read_rast(e, n, dist / out->factor, out);
    }

    k = res / hypot(rows, cols);
    Y = k * rows;
    X = k * cols;
    if (Y < 0)
        Y = Y * -1.;
    if (X < 0)
        X = X * -1.;

    /* The previous segment stepped past its end point; take back the
     * distance between that last step and the start of this segment. */
    if (have_last && (e != e1 || n != n1)) {
        dist -= G_distance(e, n, e1, n1);
    }

    if (rows >= 0 && cols < 0) {
        /* SE Quad or due east */
        for (e = e1, n = n1; e < e2 || n > n2; e += X, n -= Y) {
            read_rast(e, n, dist / out->factor, out);
            /* d+=res; */
            dist += G_distance(e - X, n + Y, e, n);
        }
    }

    if (rows < 0 && cols <= 0) {
        /* NE Quad  or due north */
        for (e = e1, n = n1; e < e2 || n < n2; e += X, n += Y) {
            read_rast(e, n, dist / out->factor, out);
            /* d+=res; */
            dist += G_distance(e - X, n - Y, e, n);
        }
    }

    if (rows > 0 && cols >= 0) {
        /* SW Quad or due south */
        for (e = e1, n = n1; e > e2 || n > n2; e -= X, n -= Y) {
            read_rast(e, n, dist / out->factor, out);
            /* d+=res; */
            dist += G_distance(e + X, n + Y, e, n);
        }
    }

    if (rows <= 0 && cols > 0) {
        /* NW Quad  or due west */
        for (e = e1, n = n1; e > e2 || n < n2; e -= X, n += Y) {
            read_rast(e, n, dist / out->factor, out);
            /* d+=res; */
            dist += G_distance(e + X, n - Y, e, n);
        }
    }
    have_last = TRUE;
    /*
     * return dist;
     */
    return 0;
} /* done with do_profile */

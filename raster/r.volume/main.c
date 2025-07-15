/****************************************************************************
 *
 * MODULE:       r.volume
 * AUTHOR(S):    Dr. James Hinthorne, Central Washington University GIS
 *                 Laboratory December 1988, (revised April 1989)
 *                 (original contributor)
 *               Revised Jul 1995 to use new sites API (McCauley)
 *               GRASS 6 update: Hamish Bowman <hamish_b yahoo.com>
 *               Glynn Clements <glynn gclements.plus.com>,
 *               Soeren Gebbert <soeren.gebbert gmx.de>
 *               GRASS 7 update (to use Vlib): Martin Landa
 *                 <landa.martin gmail.com>
 * PURPOSE:      r.volume is a program to compute the total, and average of
 *               cell values within regions of a map defined by clumps or
 *               patches on a second map (or by raster mask). It also computes
 *               the "volume" by multiplying the total within a clump by the
 *               area of each cell. It also outputs the "centroid" location of
 *               each clump. Output is to standard out.
 *
 * COPYRIGHT:    (C) 1999-2006, 2013 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/gjson.h>
#include <grass/raster.h>
#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

#include "local_proto.h"

int main(int argc, char *argv[])
{
    /* variables */
    DCELL *data_buf;
    CELL *clump_buf;
    CELL i, max;

    int row, col, rows, cols;
    int print;
    bool use_mask;
    unsigned long *n, *e;
    long int *count;
    int fd_data, fd_clump;

    const char *datamap, *clumpmap, *centroidsmap;

    double avg, vol, total_vol, east, north, *sum;

    struct Cell_head window;

    struct Map_info *fd_centroids;
    struct line_pnts *Points;
    struct line_cats *Cats;
    struct field_info *Fi;

    char buf[DB_SQL_MAX];
    dbString sql;
    dbDriver *driver;

    struct GModule *module;
    struct {
        struct Option *input, *clump, *centroids, *output, *fs, *format;
    } opt;
    struct {
        struct Flag *report, *print;
    } flag;
    char *fs;
    enum OutputFormat format;
    JSON_Value *root_value = NULL, *cat_value = NULL;
    JSON_Array *root_array = NULL;
    JSON_Object *cat_object = NULL;

    /* define parameters and flags */
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("volume"));
    G_add_keyword(_("clumps"));
    module->label = _("Calculates the volume of data \"clumps\".");
    module->description =
        _("Optionally produces a GRASS vector points map "
          "containing the calculated centroids of these clumps.");

    opt.input = G_define_standard_option(G_OPT_R_INPUT);
    opt.input->description = _("Name of input raster map representing data "
                               "that will be summed within clumps");

    opt.clump = G_define_standard_option(G_OPT_R_INPUT);
    opt.clump->key = "clump";
    opt.clump->required = NO;
    opt.clump->label = _("Name of input clump raster map");
    opt.clump->description = _("Preferably the output of r.clump. "
                               "If no clump map is given, "
                               "raster mask is used instead.");

    opt.centroids = G_define_standard_option(G_OPT_V_OUTPUT);
    opt.centroids->key = "centroids";
    opt.centroids->required = NO;
    opt.centroids->description =
        _("Name for output vector points map to contain clump centroids");

    opt.output = G_define_standard_option(G_OPT_F_OUTPUT);
    opt.output->required = NO;
    opt.output->label = _("Name for output file to hold the report");
    opt.output->description =
        _("If no output file given report is printed to standard output");

    opt.fs = G_define_standard_option(G_OPT_F_SEP);
    opt.fs->answer = NULL;
    opt.fs->guisection = _("Formatting");

    opt.format = G_define_standard_option(G_OPT_F_FORMAT);
    opt.format->options = "plain,csv,json";
    opt.format->descriptions = ("plain;Human readable text output;"
                                "csv;CSV (Comma Separated Values);"
                                "json;JSON (JavaScript Object Notation);");

    flag.report = G_define_flag();
    flag.report->key = 'f';
    flag.report->label = _(
        "Generate unformatted report (items separated by colon) [deprecated]");
    flag.report->description = _(
        "This flag is deprecated and will be removed in a future release. Use "
        "format=csv instead.");

    flag.print = G_define_flag();
    flag.print->key = 'p';
    flag.print->description = _("Print report");

    // Uncomment for GRASS v9
    // G_option_required(flag.print, opt.centroids, NULL);

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    /* get arguments */
    datamap = opt.input->answer;

    clumpmap = NULL;
    if (opt.clump->answer)
        clumpmap = opt.clump->answer;

    centroidsmap = NULL;
    fd_centroids = NULL;
    Points = NULL;
    Cats = NULL;
    driver = NULL;
    if (opt.centroids->answer) {
        centroidsmap = opt.centroids->answer;
        fd_centroids = G_malloc(sizeof(struct Map_info));
    }

    /* For backward compatibility */
    if (!opt.fs->answer) {
        if (strcmp(opt.format->answer, "csv") == 0)
            opt.fs->answer = "comma";
        else
            opt.fs->answer = ":";
    }
    fs = G_option_to_separator(opt.fs);

    if (strcmp(opt.format->answer, "json") == 0) {
        format = JSON;
        root_value = G_json_value_init_array();
        if (root_value == NULL) {
            G_fatal_error(_("Failed to initialize JSON array. Out of memory?"));
        }
        root_array = G_json_array(root_value);
    }
    else if (strcmp(opt.format->answer, "csv") == 0) {
        format = CSV;
    }
    else {
        format = PLAIN;
    }

    if (flag.report->answer) {
        G_verbose_message(
            _("Flag 'f' is deprecated and will be removed in a future "
              "release. Please use format=csv instead."));
        if (format == JSON) {
            G_fatal_error(_("The -f flag cannot be used with format=json. "
                            "Please select only one output format."));
        }
        format = CSV;
    }
    // print = flag.print->answer;
    // currently prints always, change for GRASS v9
    print = 1;

    /*
     * see if raster mask or a separate "clumpmap" raster map is to be used
     * -- it must(!) be one of those two choices.
     */
    use_mask = false;
    char mask_name[GNAME_MAX];
    char mask_mapset[GMAPSET_MAX];
    if (!clumpmap) {
        bool present =
            Rast_mask_status(mask_name, mask_mapset, NULL, NULL, NULL);
        if (!present)
            G_fatal_error(_("No clump map <%s> given and no raster mask found. "
                            "You need to define a clump raster map or create "
                            "a raster mask using r.mask."),
                          opt.clump->key);
        clumpmap = mask_name;
        use_mask = true;
        G_important_message(_("No clump map given, using raster mask"));
    }

    /* open input and clump raster maps */
    fd_data = Rast_open_old(datamap, "");
    fd_clump = Rast_open_old(clumpmap, use_mask ? mask_mapset : "");

    /* initialize vector map (for centroids) if needed */
    if (centroidsmap) {
        if (Vect_open_new(fd_centroids, centroidsmap, WITHOUT_Z) < 0)
            G_fatal_error(_("Unable to create vector map <%s>"), centroidsmap);

        Points = Vect_new_line_struct();
        Cats = Vect_new_cats_struct();

        /* initialize data structures */
        Vect_append_point(Points, 0., 0., 0.);
        Vect_cat_set(Cats, 1, 1);
    }

    /* initialize output file */
    if (opt.output->answer && strcmp(opt.output->answer, "-") != 0) {
        if (freopen(opt.output->answer, "w", stdout) == NULL) {
            perror(opt.output->answer);
            exit(EXIT_FAILURE);
        }
    }

    /* initialize data accumulation arrays */
    max = Rast_get_max_c_cat(clumpmap, use_mask ? mask_mapset : "");

    sum = (double *)G_malloc((max + 1) * sizeof(double));
    count = (long int *)G_malloc((max + 1) * sizeof(long int));

    G_zero(sum, (max + 1) * sizeof(double));
    G_zero(count, (max + 1) * sizeof(long int));

    data_buf = Rast_allocate_d_buf();
    clump_buf = Rast_allocate_c_buf();

    /* get window size */
    G_get_window(&window);
    rows = window.rows;
    cols = window.cols;

    /* now get the data -- first pass */
    for (row = 0; row < rows; row++) {
        G_percent(row, rows, 2);
        Rast_get_d_row(fd_data, data_buf, row);
        Rast_get_c_row(fd_clump, clump_buf, row);
        for (col = 0; col < cols; col++) {
            i = clump_buf[col];
            if (i > max)
                G_fatal_error(
                    _("Invalid category value %d (max=%d): row=%d col=%d"), i,
                    max, row, col);
            if (i < 1) {
                G_debug(3, "row=%d col=%d: zero or negs ignored", row, col);
                continue; /* ignore zeros and negs */
            }
            if (Rast_is_d_null_value(&data_buf[col])) {
                G_debug(3, "row=%d col=%d: NULL ignored", row, col);
                continue;
            }

            sum[i] += data_buf[col];
            count[i]++;
        }
    }
    G_percent(1, 1, 1);

    /* free some buffer space */
    G_free(data_buf);
    G_free(clump_buf);

    /* data lists for centroids of clumps */
    e = (unsigned long *)G_malloc((max + 1) * sizeof(unsigned long));
    n = (unsigned long *)G_malloc((max + 1) * sizeof(unsigned long));

    i = centroids(fd_clump, e, n, 1, max);

    /* close raster maps */
    Rast_close(fd_data);
    Rast_close(fd_clump);

    /* got everything, now do output */
    if (centroidsmap) {
        G_message(_("Creating vector point map <%s>..."), centroidsmap);
        /* set comment */
        snprintf(buf, sizeof(buf),
                 _("From '%s' on raster map <%s> using clumps from <%s>"),
                 argv[0], datamap, clumpmap);
        Vect_set_comment(fd_centroids, buf);

        /* create attribute table */
        Fi = Vect_default_field_info(fd_centroids, 1, NULL, GV_1TABLE);

        driver = db_start_driver_open_database(
            Fi->driver, Vect_subst_var(Fi->database, fd_centroids));
        if (driver == NULL) {
            G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
                          Vect_subst_var(Fi->database, fd_centroids),
                          Fi->driver);
        }
        db_set_error_handler_driver(driver);

        db_begin_transaction(driver);

        db_init_string(&sql);
        snprintf(
            buf, sizeof(buf),
            "create table %s (cat integer, volume double precision, "
            "average double precision, sum double precision, count integer)",
            Fi->table);
        db_set_string(&sql, buf);
        Vect_map_add_dblink(fd_centroids, 1, NULL, Fi->table, GV_KEY_COLUMN,
                            Fi->database, Fi->driver);

        G_debug(3, "%s", db_get_string(&sql));
        if (db_execute_immediate(driver, &sql) != DB_OK) {
            G_fatal_error(_("Unable to create table: %s"), db_get_string(&sql));
        }
    }

    /* print header */
    if (print && format == PLAIN) {
        fprintf(stdout,
                _("\nVolume report on data from <%s> using clumps on <%s> "
                  "raster map"),
                datamap, clumpmap);
        fprintf(stdout, "\n\n");
        fprintf(stdout, _("Category   Average   Data   # Cells        Centroid "
                          "            Total\n"));
        fprintf(stdout, _("Number     in clump  Total  in clump   Easting     "
                          "Northing     Volume"));
        fprintf(stdout, "\n%s\n", SEP);
    }
    total_vol = 0.0;

    /* print output, write centroids */
    if (fd_centroids || print)
        for (i = 1; i <= max; i++) {
            if (count[i]) {
                avg = sum[i] / (double)count[i];
                vol = sum[i] * window.ew_res * window.ns_res;
                total_vol += vol;
                east = window.west + (e[i] + 0.5) * window.ew_res;
                north = window.north - (n[i] + 0.5) * window.ns_res;
                if (fd_centroids) { /* write centroids if requested */
                    Points->x[0] = east;
                    Points->y[0] = north;
                    Cats->cat[0] = i;
                    Vect_write_line(fd_centroids, GV_POINT, Points, Cats);

                    snprintf(buf, sizeof(buf),
                             "insert into %s values (%d, %f, %f, %f, %ld)",
                             Fi->table, i, vol, avg, sum[i], count[i]);
                    db_set_string(&sql, buf);

                    if (db_execute_immediate(driver, &sql) != DB_OK)
                        G_fatal_error(_("Cannot insert new row: %s"),
                                      db_get_string(&sql));
                }

                if (print) {
                    switch (format) {
                    case PLAIN:
                        fprintf(stdout,
                                "%8d%10.2f%10.0f %7ld  %10.2f  %10.2f %16.2f\n",
                                i, avg, sum[i], count[i], east, north, vol);
                        break;
                    case CSV:
                        fprintf(stdout,
                                "%d%s%.2f%s%.0f%s%ld%s%.2f%s%.2f%s%.2f\n", i,
                                fs, avg, fs, sum[i], fs, count[i], fs, east, fs,
                                north, fs, vol);
                        break;
                    case JSON:
                        cat_value = G_json_value_init_object();
                        if (cat_value == NULL) {
                            G_fatal_error(_("Failed to initialize JSON object. "
                                            "Out of memory?"));
                        }
                        cat_object = G_json_object(cat_value);

                        G_json_object_set_number(cat_object, "category", i);
                        G_json_object_set_number(cat_object, "average", avg);
                        G_json_object_set_number(cat_object, "sum", sum[i]);
                        G_json_object_set_number(cat_object, "cells", count[i]);
                        G_json_object_set_number(cat_object, "volume", vol);
                        G_json_object_set_number(cat_object, "easting", east);
                        G_json_object_set_number(cat_object, "northing", north);

                        G_json_array_append_value(root_array, cat_value);
                        break;
                    }
                }
            }
        }

    /* write centroid attributes and close the map */
    if (fd_centroids) {
        db_commit_transaction(driver);
        Vect_build(fd_centroids);
        Vect_close(fd_centroids);
    }

    /* print total value */
    if (total_vol > 0.0 && print && format == PLAIN) {
        fprintf(stdout, "%s\n", SEP);
        fprintf(stdout, "%60s = %14.2f", _("Total Volume"), total_vol);
        fprintf(stdout, "\n");
    }

    if (format == JSON) {
        char *json_string = G_json_serialize_to_string_pretty(root_value);
        if (!json_string) {
            G_json_value_free(root_value);
            G_fatal_error(_("Failed to serialize JSON to pretty format."));
        }

        puts(json_string);

        G_json_free_serialized_string(json_string);
        G_json_value_free(root_value);
    }

    exit(EXIT_SUCCESS);
}

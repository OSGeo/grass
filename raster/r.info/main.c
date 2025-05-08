/***************************************************************************
 *
 * MODULE:       r.info
 *
 * AUTHOR(S):    Michael O'Shea
 *
 * PURPOSE:      Outputs basic information about a user-specified raster map
 *layer.
 *
 * COPYRIGHT:    (C) 2005-2011 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <math.h>
#include <sys/types.h>
#include <grass/config.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <inttypes.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include <grass/parson.h>
#include "local_proto.h"

#define printline(x) fprintf(out, " | %-74.74s |\n", x)
#define divider(x)           \
    fprintf(out, " %c", x);  \
    for (i = 0; i < 76; i++) \
        fprintf(out, "-");   \
    fprintf(out, "%c\n", x)
#define TMPBUF_SZ 100

enum OutputFormat { PLAIN, JSON };

/* local prototypes */
static void format_double(const double, char[100]);
static void compose_line(FILE *, const char *, ...);
static char *history_as_string(struct History *hist);

int main(int argc, char **argv)
{
    const char *name, *mapset;
    const char *title;
    char tmp1[TMPBUF_SZ], tmp2[TMPBUF_SZ], tmp3[TMPBUF_SZ], tmp4[TMPBUF_SZ];
    char timebuff[256];
    char *units, *vdatum, *semantic_label;
    int i;
    CELL mincat = 0, maxcat = 0, cat;
    FILE *out;
    struct Range crange;
    struct FPRange range;
    struct R_stats rstats;
    struct Cell_head cellhd;
    struct Categories cats;
    struct History hist;
    struct TimeStamp ts;
    int time_ok = 0, first_time_ok = 0, second_time_ok = 0;
    int cats_ok, hist_ok;
    int is_reclass;
    RASTER_MAP_TYPE data_type;
    struct Reclass reclass;
    struct GModule *module;
    struct Option *opt1, *fopt;
    struct Flag *gflag, *rflag, *eflag, *hflag, *sflag;
    enum OutputFormat format;

    JSON_Value *root_value;
    JSON_Object *root_object;

    /* Initialize GIS Engine */
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("metadata"));
    G_add_keyword(_("extent"));
    G_add_keyword(_("history"));
    module->description = _("Outputs basic information about a raster map.");

    opt1 = G_define_standard_option(G_OPT_R_MAP);

    gflag = G_define_flag();
    gflag->key = 'g';
    gflag->description =
        _("Print raster array information in shell script style");

    rflag = G_define_flag();
    rflag->key = 'r';
    rflag->description = _("Print range in shell script style");

    sflag = G_define_flag();
    sflag->key = 's';
    sflag->description = _("Print stats in shell script style");

    eflag = G_define_flag();
    eflag->key = 'e';
    eflag->description =
        _("Print extended metadata information in shell script style");

    hflag = G_define_flag();
    hflag->key = 'h';
    hflag->description = _("Print raster history instead of info");

    fopt = G_define_standard_option(G_OPT_F_FORMAT);
    fopt->guisection = _("Print");

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    if (hflag->answer &&
        (gflag->answer || rflag->answer || sflag->answer || eflag->answer))
        G_fatal_error(_("Flags -%c and -%c/%c/%c are mutually exclusive"),
                      hflag->key, gflag->key, rflag->key, eflag->key);

    name = G_store(opt1->answer);
    if ((mapset = G_find_raster2(name, "")) == NULL)
        G_fatal_error(_("Raster map <%s> not found"), name);

    if (strcmp(fopt->answer, "json") == 0) {
        format = JSON;
        root_value = json_value_init_object();
        root_object = json_value_get_object(root_value);
    }
    else {
        format = PLAIN;
    }

    Rast_get_cellhd(name, "", &cellhd);
    cats_ok = Rast_read_cats(name, "", &cats) >= 0;
    title = Rast_get_cats_title(&cats);
    hist_ok = Rast_read_history(name, "", &hist) >= 0;
    is_reclass = Rast_get_reclass(name, "", &reclass);
    data_type = Rast_map_type(name, "");

    units = Rast_read_units(name, "");

    vdatum = Rast_read_vdatum(name, "");

    semantic_label = Rast_read_semantic_label(name, "");

    /*Check the Timestamp */
    time_ok = G_read_raster_timestamp(name, "", &ts) > 0;
    /*Check for valid entries, show none if no timestamp available */
    if (time_ok) {
        if (ts.count > 0)
            first_time_ok = 1;
        if (ts.count > 1)
            second_time_ok = 1;
    }

    out = stdout;

    if (eflag->answer || (!gflag->answer && !rflag->answer && !sflag->answer &&
                          !hflag->answer)) {
        title = "";
        /* empty title by default */
        /* use title from category file as the primary (and only) title */
        if (cats_ok)
            title = cats.title;
        /* only use hist file title if there is none in the category file */
        if ((!title || title[0] == '\0') && hist_ok) {
            title = Rast_get_history(&hist, HIST_TITLE);
            /* if the title is the same as name of the map, don't use it */
            if (strcmp(title, name) == 0)
                title = "";
        }
    }

    if (!gflag->answer && !rflag->answer && !sflag->answer && !eflag->answer &&
        !hflag->answer && format == PLAIN) {
        divider('+');

        compose_line(out, "Map:      %-29.29s  Date: %s", name,
                     hist_ok ? Rast_get_history(&hist, HIST_MAPID) : "??");
        compose_line(out, "Mapset:   %-29.29s  Login of Creator: %s", mapset,
                     hist_ok ? Rast_get_history(&hist, HIST_CREATOR) : "??");
        compose_line(out, "Project:  %s", G_location());
        compose_line(out, "DataBase: %s", G_gisdbase());
        compose_line(out, "Title:    %s", title);

        /* This shows the TimeStamp */
        if (time_ok && (first_time_ok || second_time_ok)) {
            G_format_timestamp(&ts, timebuff);
            compose_line(out, "Timestamp: %s", timebuff);
        }
        else {
            compose_line(out, "Timestamp: none");
        }

        divider('|');
        printline("");

        if (cats_ok)
            format_double((double)cats.num, tmp1);

        compose_line(out, "  Type of Map:  %-20.20s Number of Categories: %-9s",
                     hist_ok ? Rast_get_history(&hist, HIST_MAPTYPE) : "??",
                     cats_ok ? tmp1 : "??");

        compose_line(out, "  Data Type:    %-20.20s Semantic label: %s ",
                     (data_type == CELL_TYPE
                          ? "CELL"
                          : (data_type == DCELL_TYPE
                                 ? "DCELL"
                                 : (data_type == FCELL_TYPE ? "FCELL" : "??"))),
                     (semantic_label ? semantic_label : "(none)"));

        /* For now hide these unless they exist to keep the noise low. In
         *   future when the two are used more widely they can be printed
         *   along with the standard set. */
        if (units || vdatum)
            compose_line(out, "  Data Units:   %-20.20s Vertical datum: %s",
                         units ? units : "(none)", vdatum ? vdatum : "(none)");

        {
            compose_line(out, "  Rows:         %d", cellhd.rows);
            compose_line(out, "  Columns:      %d", cellhd.cols);
            compose_line(out, "  Total Cells:  %jd",
                         (grass_int64)cellhd.rows * cellhd.cols);

            /* This is printed as a guide to what the following eastings and
             * northings are printed in. This data is NOT from the values
             * stored in the map's Cell_head */
            if (G_projection() == PROJECTION_UTM) {
                compose_line(out, "       Projection: %s (zone %d)",
                             G_database_projection_name(), G_zone());
            }
            else {
                compose_line(out, "       Projection: %s",
                             G_database_projection_name());
            }

            G_format_northing(cellhd.north, tmp1, cellhd.proj);
            G_format_northing(cellhd.south, tmp2, cellhd.proj);
            G_format_resolution(cellhd.ns_res, tmp3, cellhd.proj);
            compose_line(out, "           N: %10s    S: %10s   Res: %5s", tmp1,
                         tmp2, tmp3);

            G_format_easting(cellhd.east, tmp1, cellhd.proj);
            G_format_easting(cellhd.west, tmp2, cellhd.proj);
            G_format_resolution(cellhd.ew_res, tmp3, cellhd.proj);
            compose_line(out, "           E: %10s    W: %10s   Res: %5s", tmp1,
                         tmp2, tmp3);

            if (data_type == CELL_TYPE) {
                int ret;
                CELL min, max;

                /* print range only if available */
                ret = Rast_read_range(name, "", &crange);
                if (ret == 2)
                    compose_line(out,
                                 "  Range of data:    min = NULL  max = NULL");
                else if (ret > 0) {
                    Rast_get_range_min_max(&crange, &min, &max);

                    if (Rast_is_c_null_value(&min)) {
                        compose_line(
                            out, "  Range of data:    min = NULL  max = NULL");
                    }
                    else {
                        compose_line(out,
                                     "  Range of data:    min = %i  max = %i",
                                     min, max);
                    }
                }
            }
            else {
                int ret;
                DCELL min, max;

                /* print range only if available */
                ret = Rast_read_fp_range(name, "", &range);
                if (ret == 2) {
                    compose_line(out,
                                 "  Range of data:    min = NULL  max = NULL");
                }
                else if (ret > 0) {
                    Rast_get_fp_range_min_max(&range, &min, &max);

                    if (Rast_is_d_null_value(&min)) {
                        compose_line(
                            out, "  Range of data:    min = NULL  max = NULL");
                    }
                    else {
                        if (data_type == FCELL_TYPE) {
                            compose_line(
                                out,
                                "  Range of data:    min = %.7g  max = %.7g",
                                min, max);
                        }
                        else {
                            compose_line(
                                out,
                                "  Range of data:    min = %.15g  max = %.15g",
                                min, max);
                        }
                    }
                }
            }
        }

        printline("");

        if (hist_ok) {
            if (Rast_get_history(&hist, HIST_DATSRC_1)[0] != '\0' ||
                Rast_get_history(&hist, HIST_DATSRC_2)[0] != '\0') {
                printline("  Data Source:");
                compose_line(out, "   %s",
                             Rast_get_history(&hist, HIST_DATSRC_1));
                compose_line(out, "   %s",
                             Rast_get_history(&hist, HIST_DATSRC_2));
                printline("");
            }

            printline("  Data Description:");
            compose_line(out, "   %s", Rast_get_history(&hist, HIST_KEYWRD));

            printline("");
            if (Rast_history_length(&hist)) {
                printline("  Comments:  ");

                for (i = 0; i < Rast_history_length(&hist); i++)
                    compose_line(out, "   %s", Rast_history_line(&hist, i));
            }

            printline("");
        }

        if (is_reclass > 0) {
            int first = 1;

            divider('|');

            compose_line(out, "  Reclassification of [%s] in mapset [%s]",
                         reclass.name, reclass.mapset);

            printline("");
            printline("        Category        Original categories");
            printline("");

            for (i = 0; i < reclass.num; i++) {
                CELL x = reclass.table[i];

                if (Rast_is_c_null_value(&x))
                    continue;
                if (first || x < mincat)
                    mincat = x;
                if (first || x > maxcat)
                    maxcat = x;
                first = 0;
            }

            if (!first)
                for (cat = mincat; cat <= maxcat; cat++) {
                    char text[80];
                    char *num;
                    int next;

                    if (cat == 0)
                        continue;
                    if (G_asprintf(&num, "%5ld", (long)cat) < 1)
                        G_fatal_error(_("Cannot allocate memory for string"));

                    next = 0;
                    do {
                        next = reclass_text(text, cat, &reclass, next);
                        compose_line(out, "     %5s              %s", num,
                                     text);
                        *num = 0;
                    } while (next >= 0);
                }
        }
        divider('+');

        fprintf(out, "\n");
    }
    else { /* g, r, s, e, or h flags */
        int need_range, have_range, need_stats, have_stats;

        need_range = rflag->answer || format == JSON;
        need_stats = sflag->answer;
        if (need_stats)
            need_range = 1;

        have_range = have_stats = 0;
        if (need_range) {
            if (data_type == CELL_TYPE) {
                if (Rast_read_range(name, "", &crange) > 0)
                    have_range = 1;
            }
            else {
                if (Rast_read_fp_range(name, "", &range) > 0)
                    have_range = 1;
            }
        }
        if (need_stats) {
            if (Rast_read_rstats(name, mapset, &rstats) > 0)
                have_stats = 1;
        }

        if ((need_stats && !have_stats) || (need_range && !have_range)) {
            DCELL *dbuf, val, min, max;
            int fd, r, c;
            int first = 1;

            Rast_set_input_window(&cellhd);
            dbuf = Rast_allocate_d_input_buf();
            fd = Rast_open_old(name, mapset);
            min = max = 0;

            for (r = 0; r < cellhd.rows; r++) {
                Rast_get_d_row_nomask(fd, dbuf, r);
                for (c = 0; c < cellhd.cols; c++) {
                    val = dbuf[c];
                    if (Rast_is_d_null_value(&val))
                        continue;
                    if (first) {
                        rstats.sum = val;
                        rstats.sumsq = (DCELL)val * val;

                        rstats.count = 1;
                        min = max = val;

                        first = 0;
                    }
                    else {
                        rstats.sum += val;
                        rstats.sumsq += (DCELL)val * val;

                        rstats.count += 1;
                        if (min > val)
                            min = val;
                        if (max < val)
                            max = val;
                    }
                }
            }
            Rast_close(fd);
            G_free(dbuf);

            if (data_type == CELL_TYPE) {
                Rast_init_range(&crange);
                if (rstats.count > 0) {
                    Rast_update_range((CELL)min, &crange);
                    Rast_update_range((CELL)max, &crange);
                }
            }
            else {
                Rast_init_fp_range(&range);
                if (rstats.count > 0) {
                    Rast_update_fp_range(min, &range);
                    Rast_update_fp_range(max, &range);
                }
            }
        }

        if (gflag->answer || format == JSON) {
            const char *data_type_f =
                (data_type == CELL_TYPE
                     ? "CELL"
                     : (data_type == DCELL_TYPE
                            ? "DCELL"
                            : (data_type == FCELL_TYPE ? "FCELL" : "??")));
            grass_int64 total_cells = (grass_int64)cellhd.rows * cellhd.cols;

            switch (format) {
            case PLAIN:
                G_format_northing(cellhd.north, tmp1, -1);
                G_format_northing(cellhd.south, tmp2, -1);
                fprintf(out, "north=%s\n", tmp1);
                fprintf(out, "south=%s\n", tmp2);

                G_format_easting(cellhd.east, tmp1, -1);
                G_format_easting(cellhd.west, tmp2, -1);
                fprintf(out, "east=%s\n", tmp1);
                fprintf(out, "west=%s\n", tmp2);

                G_format_resolution(cellhd.ns_res, tmp3, -1);
                fprintf(out, "nsres=%s\n", tmp3);

                G_format_resolution(cellhd.ew_res, tmp3, -1);
                fprintf(out, "ewres=%s\n", tmp3);

                fprintf(out, "rows=%d\n", cellhd.rows);
                fprintf(out, "cols=%d\n", cellhd.cols);

                fprintf(out, "cells=%" PRId64 "\n", total_cells);

                fprintf(out, "datatype=%s\n", data_type_f);

                if (cats_ok)
                    format_double((double)cats.num, tmp4);
                fprintf(out, "ncats=%s\n", cats_ok ? tmp4 : "??");
                break;
            case JSON:
                json_object_set_number(root_object, "north", cellhd.north);
                json_object_set_number(root_object, "south", cellhd.south);
                json_object_set_number(root_object, "nsres", cellhd.ns_res);

                json_object_set_number(root_object, "east", cellhd.east);
                json_object_set_number(root_object, "west", cellhd.west);
                json_object_set_number(root_object, "ewres", cellhd.ew_res);

                json_object_set_number(root_object, "rows", cellhd.rows);
                json_object_set_number(root_object, "cols", cellhd.cols);
                json_object_set_number(root_object, "cells", total_cells);

                json_object_set_string(root_object, "datatype", data_type_f);
                if (cats_ok) {
                    json_object_set_number(root_object, "ncats", cats.num);
                }
                else {
                    json_object_set_null(root_object, "ncats");
                }
                break;
            }
        }

        if (rflag->answer || sflag->answer || format == JSON) {
            if (data_type == CELL_TYPE) {
                CELL min, max;

                Rast_get_range_min_max(&crange, &min, &max);
                if (Rast_is_c_null_value(&min)) {
                    switch (format) {
                    case PLAIN:
                        fprintf(out, "min=NULL\n");
                        fprintf(out, "max=NULL\n");
                        break;
                    case JSON:
                        json_object_set_null(root_object, "min");
                        json_object_set_null(root_object, "max");
                        break;
                    }
                }
                else {
                    switch (format) {
                    case PLAIN:
                        fprintf(out, "min=%i\n", min);
                        fprintf(out, "max=%i\n", max);
                        break;
                    case JSON:
                        json_object_set_number(root_object, "min", min);
                        json_object_set_number(root_object, "max", max);
                        break;
                    }
                }
            }
            else {
                DCELL min, max;

                Rast_get_fp_range_min_max(&range, &min, &max);
                if (Rast_is_d_null_value(&min)) {
                    switch (format) {
                    case PLAIN:
                        fprintf(out, "min=NULL\n");
                        fprintf(out, "max=NULL\n");
                        break;
                    case JSON:
                        json_object_set_null(root_object, "min");
                        json_object_set_null(root_object, "max");
                        break;
                    }
                }
                else {
                    switch (format) {
                    case PLAIN:
                        if (data_type == FCELL_TYPE) {
                            fprintf(out, "min=%.7g\n", min);
                            fprintf(out, "max=%.7g\n", max);
                        }
                        else {
                            fprintf(out, "min=%.15g\n", min);
                            fprintf(out, "max=%.15g\n", max);
                        }
                        break;
                    case JSON:
                        json_object_set_number(root_object, "min", min);
                        json_object_set_number(root_object, "max", max);
                        break;
                    }
                }
            }
        }

        if (sflag->answer) {

            if (!gflag->answer) {
                grass_int64 total_cells =
                    (grass_int64)cellhd.rows * cellhd.cols;
                /* always report total number of cells */
                switch (format) {
                case PLAIN:
                    fprintf(out, "cells=%" PRId64 "\n", total_cells);
                    break;
                case JSON:
                    json_object_set_number(root_object, "cells", total_cells);
                    break;
                }
            }

            if (rstats.count > 0) {
                double mean, sd;

                mean = (double)(rstats.sum / rstats.count);
                sd = sqrt(rstats.sumsq / rstats.count - (mean * mean));

                if (data_type == CELL_TYPE) {
                    CELL min, max;

                    Rast_get_range_min_max(&crange, &min, &max);
                    if (min == max) {
                        mean = min;
                        sd = 0;
                    }
                }
                else {
                    DCELL min, max;

                    Rast_get_fp_range_min_max(&range, &min, &max);
                    if (min == max) {
                        mean = min;
                        sd = 0;
                    }
                }

                switch (format) {
                case PLAIN:
                    fprintf(out, "n=%" PRId64 "\n", rstats.count);
                    fprintf(out, "mean=%.15g\n", mean);
                    fprintf(out, "stddev=%.15g\n", sd);
                    fprintf(out, "sum=%.15g\n", rstats.sum);
                    break;
                case JSON:
                    json_object_set_number(root_object, "n", rstats.count);
                    json_object_set_number(root_object, "mean", mean);
                    json_object_set_number(root_object, "stddev", sd);
                    json_object_set_number(root_object, "sum", rstats.sum);
                    break;
                }
            }
            else {
                switch (format) {
                case PLAIN:
                    fprintf(out, "n=0\n");
                    fprintf(out, "mean=NULL\n");
                    fprintf(out, "stddev=NULL\n");
                    fprintf(out, "sum=NULL\n");
                    break;
                case JSON:
                    json_object_set_number(root_object, "n", 0);
                    json_object_set_null(root_object, "mean");
                    json_object_set_null(root_object, "stddev");
                    json_object_set_null(root_object, "sum");
                    break;
                }
            }
        }

        if (eflag->answer || format == JSON) {
            char xname[GNAME_MAX], xmapset[GMAPSET_MAX];
            const char *maptype, *date, *creator;

            G_unqualified_name(name, mapset, xname, xmapset);

            maptype = hist_ok ? Rast_get_history(&hist, HIST_MAPTYPE) : "??";
            date = hist_ok ? Rast_get_history(&hist, HIST_MAPID) : "??";
            creator = hist_ok ? Rast_get_history(&hist, HIST_CREATOR) : "??";

            switch (format) {
            case PLAIN:
                fprintf(out, "map=%s\n", xname);
                fprintf(out, "maptype=%s\n", maptype);
                fprintf(out, "mapset=%s\n", mapset);
                fprintf(out, "location=%s\n", G_location());
                fprintf(out, "project=%s\n", G_location());
                fprintf(out, "database=%s\n", G_gisdbase());
                fprintf(out, "date=\"%s\"\n", date);
                fprintf(out, "creator=\"%s\"\n", creator);
                fprintf(out, "title=\"%s\"\n", title);
                break;
            case JSON:
                json_object_set_string(root_object, "map", name);
                json_object_set_string(root_object, "maptype", maptype);
                json_object_set_string(root_object, "mapset", mapset);
                json_object_set_string(root_object, "location", G_location());
                json_object_set_string(root_object, "project", G_location());
                json_object_set_string(root_object, "database", G_gisdbase());
                json_object_set_string(root_object, "date", date);
                json_object_set_string(root_object, "creator", creator);
                json_object_set_string(root_object, "title", title);
                break;
            }
            if (time_ok && (first_time_ok || second_time_ok)) {

                G_format_timestamp(&ts, timebuff);
                switch (format) {
                case PLAIN:
                    /*Create the r.info timestamp string */
                    fprintf(out, "timestamp=\"%s\"\n", timebuff);
                    break;
                case JSON:
                    json_object_set_string(root_object, "timestamp", timebuff);
                    break;
                }
            }
            else {
                switch (format) {
                case PLAIN:
                    fprintf(out, "timestamp=\"none\"\n");
                    break;
                case JSON:
                    json_object_set_null(root_object, "timestamp");
                    break;
                }
            }

            switch (format) {
            case PLAIN:
                fprintf(out, "units=%s\n", units ? units : "\"none\"");
                fprintf(out, "vdatum=%s\n", vdatum ? vdatum : "\"none\"");
                fprintf(out, "semantic_label=%s\n",
                        semantic_label ? semantic_label : "\"none\"");
                fprintf(out, "source1=\"%s\"\n",
                        hist_ok ? Rast_get_history(&hist, HIST_DATSRC_1)
                                : "\"none\"");
                fprintf(out, "source2=\"%s\"\n",
                        hist_ok ? Rast_get_history(&hist, HIST_DATSRC_2)
                                : "\"none\"");
                fprintf(out, "description=\"%s\"\n",
                        hist_ok ? Rast_get_history(&hist, HIST_KEYWRD)
                                : "\"none\"");
                if (Rast_history_length(&hist)) {
                    fprintf(out, "comments=\"");
                    for (i = 0; i < Rast_history_length(&hist); i++)
                        fprintf(out, "%s", Rast_history_line(&hist, i));
                    fprintf(out, "\"\n");
                }
                break;
            case JSON:
                if (units) {
                    json_object_set_string(root_object, "units", units);
                }
                else {
                    json_object_set_null(root_object, "units");
                }
                if (vdatum) {
                    json_object_set_string(root_object, "vdatum", vdatum);
                }
                else {
                    json_object_set_null(root_object, "vdatum");
                }
                if (semantic_label) {
                    json_object_set_string(root_object, "semantic_label",
                                           semantic_label);
                }
                else {
                    json_object_set_null(root_object, "semantic_label");
                }

                if (hist_ok) {
                    json_object_set_string(
                        root_object, "source1",
                        Rast_get_history(&hist, HIST_DATSRC_1));
                    json_object_set_string(
                        root_object, "source2",
                        Rast_get_history(&hist, HIST_DATSRC_2));
                    json_object_set_string(
                        root_object, "description",
                        Rast_get_history(&hist, HIST_KEYWRD));
                    char *buffer = history_as_string(&hist);
                    if (buffer) {
                        json_object_set_string(root_object, "comments", buffer);
                        G_free(buffer);
                    }
                    else {
                        json_object_set_null(root_object, "comments");
                    }
                }
                else {
                    json_object_set_null(root_object, "source1");
                    json_object_set_null(root_object, "source2");
                    json_object_set_null(root_object, "description");
                    json_object_set_null(root_object, "comments");
                }
                break;
            }
        }

        if (hflag->answer) {
            if (hist_ok) {
                switch (format) {
                case PLAIN:
                    fprintf(out, "Data Source:\n");
                    fprintf(out, "   %s\n",
                            Rast_get_history(&hist, HIST_DATSRC_1));
                    fprintf(out, "   %s\n",
                            Rast_get_history(&hist, HIST_DATSRC_2));
                    fprintf(out, "Data Description:\n");
                    fprintf(out, "   %s\n",
                            Rast_get_history(&hist, HIST_KEYWRD));
                    if (Rast_history_length(&hist)) {
                        fprintf(out, "Comments:\n");
                        for (i = 0; i < Rast_history_length(&hist); i++)
                            fprintf(out, "   %s\n",
                                    Rast_history_line(&hist, i));
                    }
                    break;
                case JSON:
                    json_object_set_string(
                        root_object, "source1",
                        Rast_get_history(&hist, HIST_DATSRC_1));
                    json_object_set_string(
                        root_object, "source2",
                        Rast_get_history(&hist, HIST_DATSRC_2));
                    json_object_set_string(
                        root_object, "description",
                        Rast_get_history(&hist, HIST_KEYWRD));
                    char *buffer = history_as_string(&hist);
                    if (buffer) {
                        json_object_set_string(root_object, "comments", buffer);
                        G_free(buffer);
                    }
                    else {
                        json_object_set_null(root_object, "comments");
                    }

                    break;
                }
            }
        }
    } /* else rflag or sflag or tflag or gflag or hflag or mflag */

    if (format == JSON) {
        char *serialized_string = NULL;
        serialized_string = json_serialize_to_string_pretty(root_value);
        if (serialized_string == NULL) {
            G_fatal_error(_("Failed to initialize pretty JSON string."));
        }
        puts(serialized_string);
        json_free_serialized_string(serialized_string);
        json_value_free(root_value);
    }

    return EXIT_SUCCESS;
}

static void format_double(const double value, char buf[100])
{
    snprintf(buf, TMPBUF_SZ, "%.8f", value);
    G_trim_decimal(buf);
}

static void compose_line(FILE *out, const char *fmt, ...)
{
    char *line = NULL;
    va_list ap;

    va_start(ap, fmt);

    if (G_vasprintf(&line, fmt, ap) <= 0)
        G_fatal_error(_("Cannot allocate memory for string"));

    va_end(ap);

    printline(line);
    G_free(line);
}

static char *history_as_string(struct History *hist)
{
    int history_length = Rast_history_length(hist);
    char *buffer = NULL;
    if (history_length) {
        size_t buffer_size = 0;
        size_t total_length = 0;
        for (int i = 0; i < history_length; i++) {
            const char *line = Rast_history_line(hist, i);
            size_t line_length = strlen(line);

            // +1 for the null character
            size_t required_size = total_length + line_length + 1;
            if (required_size > buffer_size) {
                // This is heuristic for reallocation based on remaining
                // iterations and current size which is a good estimate for the
                // first iteration and possible overshoot later on reducing the
                // number of reallocations.
                buffer_size = required_size * (history_length - i);
                buffer = (char *)G_realloc(buffer, buffer_size);
                if (total_length == 0)
                    buffer[0] = '\0';
            }
            if (line_length >= 1 && line[line_length - 1] == '\\') {
                // Ending backslash is line continuation.
                strncat(buffer, line, line_length - 1);
                total_length += line_length - 1;
            }
            else {
                strncat(buffer, line, line_length);
                total_length += line_length;
                if (i < history_length - 1) {
                    // Add newline to separate lines, but don't and newline at
                    // the end of last (or only) line.
                    strcat(buffer, "\n");
                    ++total_length;
                }
            }
        }
    }
    return buffer;
}

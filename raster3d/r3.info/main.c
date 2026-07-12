/***************************************************************************
 *
 * MODULE:       r3.info
 *
 * AUTHOR(S):    Roman Waupotitsch, Michael Shapiro, Helena Mitasova,
 *               Bill Brown, Lubos Mitas, Jaro Hofierka
 *
 * PURPOSE:      Outputs basic information about a user-specified 3D raster map
 *               layer.
 *
 * COPYRIGHT:    (C) 2005 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

/* \todo
 *    History support still not full implemented.
 *    Only parts of the timestep functionality are implemented, the timezone is
 * missed ;).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/raster3d.h>
#include <grass/gjson.h>
#include <grass/glocale.h>

#define printline(x) fprintf(out, " | %-74.74s |\n", x)
#define divider(x)           \
    fprintf(out, " %c", x);  \
    for (i = 0; i < 76; i++) \
        fprintf(out, "-");   \
    fprintf(out, "%c\n", x)

#define TMP_LENGTH 100

/*local prototype */
static int format_double(double, char[100]);
static char *history_as_string(struct History *hist);

static char *name;

enum OutputFormat { PLAIN, SHELL, JSON };

/**************************************************************************/
int main(int argc, char *argv[])
{
    const char *mapset;
    char *line = NULL;
    char tmp1[TMP_LENGTH] = "", tmp2[TMP_LENGTH] = "", tmp3[TMP_LENGTH] = "";
    char timebuff[256];
    int i;
    FILE *out;
    RASTER3D_Region cellhd;
    RASTER3D_Map *g3map;
    struct Categories cats;
    struct History hist;
    struct TimeStamp ts;
    int head_ok;
    int cats_ok;
    int hist_ok;
    int time_ok = 0, first_time_ok = 0, second_time_ok = 0;
    struct Option *opt1;
    struct Option *frmt;
    struct Flag *rflag;
    struct Flag *gflag;
    struct Flag *hflag;
    int data_type;
    enum OutputFormat format;
    G_JSON_Value *root_value = NULL;
    G_JSON_Object *root_object = NULL;

    struct GModule *module;
    double dmin, dmax;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster3d"));
    G_add_keyword(_("metadata"));
    G_add_keyword(_("voxel"));
    G_add_keyword(_("extent"));
    module->description = _("Outputs basic information about a user-specified "
                            "3D raster map layer.");

    opt1 = G_define_standard_option(G_OPT_R3_MAP);

    frmt = G_define_standard_option(G_OPT_F_FORMAT);
    frmt->required = NO;
    frmt->answer = NULL;
    frmt->options = "plain,shell,json";
    frmt->descriptions = _("plain;Plain text output;"
                           "shell;shell script style output;"
                           "json;JSON (JavaScript Object Notation);");
    frmt->guisection = _("Print");

    gflag = G_define_flag();
    gflag->key = 'g';
    gflag->description = _("Print raster3d array information");

    rflag = G_define_flag();
    rflag->key = 'r';
    rflag->description = _("Print range");

    hflag = G_define_flag();
    hflag->key = 'h';
    hflag->description = _("Print raster3d history");

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    name = G_store(opt1->answer);
    mapset = G_find_raster3d(name, "");

    if (mapset == NULL)
        G_fatal_error(_("3D Raster map <%s> not found"), name);

    /*We need to open the map */
    g3map = Rast3d_open_cell_old(name, mapset, RASTER3D_DEFAULT_WINDOW,
                                 RASTER3D_TILE_SAME_AS_FILE, RASTER3D_NO_CACHE);
    if (NULL == g3map)
        G_fatal_error(_("Unable to open 3D raster map <%s>"), name);

    // If no format option is specified, preserve backward compatibility
    if (frmt->answer == NULL || frmt->answer[0] == '\0') {
        if (gflag->answer || rflag->answer) {
            // In the new behavior, "plain" will be the default format.
            // Warn when the user specifies -g, or -r without
            // format=shell option.
            G_verbose_message(
                _("The output format for flags -g, and -r currently "
                  "defaults to 'shell', but this will change to 'plain' in a "
                  "future release. To avoid unexpected behaviour, specify the "
                  "format explicitly."));
            frmt->answer = "shell";
        }
        else {
            frmt->answer = "plain";
        }
    }

    if (strcmp(frmt->answer, "json") == 0) {
        format = JSON;
        root_value = G_json_value_init_object();
        if (root_value == NULL) {
            G_fatal_error(
                _("Failed to initialize JSON object. Out of memory?"));
        }
        root_object = G_json_object(root_value);
    }
    else if (strcmp(frmt->answer, "shell") == 0) {
        format = SHELL;
    }
    else {
        format = PLAIN;
    }

    /*Get the maptype */
    data_type = Rast3d_file_type_map(g3map);
    head_ok = Rast3d_read_region_map(name, mapset, &cellhd) >= 0;
    hist_ok = Rast3d_read_history(name, mapset, &hist) >= 0;
    cats_ok = Rast3d_read_cats(name, mapset, &cats) >= 0;
    /*Check the Timestamp */
    time_ok = G_read_raster3d_timestamp(name, mapset, &ts) > 0;

    /*Check for valid entries, show none if no entire available! */
    if (time_ok) {
        if (ts.count > 0)
            first_time_ok = 1;
        if (ts.count > 1)
            second_time_ok = 1;
    }

    out = stdout;

    /*Show the info if no flag is set */
    if (!rflag->answer && !gflag->answer && !hflag->answer) {
        switch (format) {
        case PLAIN:
            divider('+');

            if (G_asprintf(&line, "Map:      %-29.29s  Date: %s", name,
                           hist_ok ? Rast_get_history(&hist, HIST_MAPID)
                                   : "??") > 0)
                printline(line);
            else
                G_fatal_error(_("Cannot allocate memory for string"));

            if (G_asprintf(
                    &line, "Mapset:   %-29.29s  Login of Creator: %s", mapset,
                    hist_ok ? Rast_get_history(&hist, HIST_CREATOR) : "??") > 0)
                printline(line);
            else
                G_fatal_error(_("Cannot allocate memory for string"));

            if (G_asprintf(&line, "Project: %s", G_location()) > 0)
                printline(line);
            else
                G_fatal_error(_("Cannot allocate memory for string"));

            if (G_asprintf(&line, "DataBase: %s", G_gisdbase()) > 0)
                printline(line);
            else
                G_fatal_error(_("Cannot allocate memory for string"));

            if (G_asprintf(&line, "Title:    %s",
                           hist_ok ? Rast_get_history(&hist, HIST_TITLE)
                                   : "??") > 0)
                printline(line);
            else
                G_fatal_error(_("Cannot allocate memory for string"));

            if (G_asprintf(&line, "Units:    %s", Rast3d_get_unit(g3map)))
                printline(line);
            else
                G_fatal_error(_("Cannot allocate memory for string"));

            if (G_asprintf(&line, "Vertical unit: %s",
                           Rast3d_get_vertical_unit(g3map)))
                printline(line);
            else
                G_fatal_error(_("Cannot allocate memory for string"));

            /*This shows the TimeStamp */
            if (time_ok && (first_time_ok || second_time_ok)) {

                G_format_timestamp(&ts, timebuff);

                /*Create the r.info timestamp string */
                if (G_asprintf(&line, "Timestamp: %s", timebuff) > 0)
                    printline(line);
                else
                    G_fatal_error(_("Cannot allocate memory for string"));
            }
            else {
                if (G_asprintf(&line, "Timestamp: none") > 0)
                    printline(line);
                else
                    G_fatal_error(_("Cannot allocate memory for string"));
            }

            divider('|');
            printline("");

            if (cats_ok) {
                format_double((double)cats.num, tmp1);
            }

            if (G_asprintf(
                    &line,
                    "  Type of Map:  %-20.20s Number of Categories: %-9s",
                    "raster_3d", cats_ok ? tmp1 : "??") > 0)
                printline(line);
            else
                G_fatal_error(_("Cannot allocate memory for string"));

            if (G_asprintf(&line, "  Data Type:    %s",
                           (data_type == FCELL_TYPE
                                ? "FCELL"
                                : (data_type == DCELL_TYPE ? "DCELL" : "??"))) >
                0)
                printline(line);
            else
                G_fatal_error(_("Cannot allocate memory for string"));

            if (head_ok) {
                if (G_asprintf(&line, "  Rows:         %d", cellhd.rows) > 0)
                    printline(line);
                else
                    G_fatal_error(_("Cannot allocate memory for string"));

                if (G_asprintf(&line, "  Columns:      %d", cellhd.cols) > 0)
                    printline(line);
                else
                    G_fatal_error(_("Cannot allocate memory for string"));

                if (G_asprintf(&line, "  Depths:       %d", cellhd.depths) > 0)
                    printline(line);
                else
                    G_fatal_error(_("Cannot allocate memory for string"));

                if (G_asprintf(&line, "  Total Cells:  %ld",
                               (long)cellhd.rows * cellhd.cols *
                                   cellhd.depths) > 0)
                    printline(line);
                else
                    G_fatal_error(_("Cannot allocate memory for string"));

                double totalSize = 0;

                for (i = 0; i < g3map->nTiles; i++)
                    totalSize += g3map->tileLength[i];

                if (G_asprintf(&line, "  Total size:           %ld Bytes",
                               (long)(totalSize)) > 0)
                    printline(line);
                else
                    G_fatal_error(_("Cannot allocate memory for string"));

                if (G_asprintf(&line, "  Number of tiles:      %d",
                               g3map->nTiles) > 0)
                    printline(line);
                else
                    G_fatal_error(_("Cannot allocate memory for string"));

                if (G_asprintf(&line, "  Mean tile size:       %ld Bytes",
                               (long)(totalSize / g3map->nTiles)) > 0)
                    printline(line);
                else
                    G_fatal_error(_("Cannot allocate memory for string"));

                int tileSize = 0;

                if (data_type == FCELL_TYPE)
                    tileSize = sizeof(FCELL) * g3map->tileX * g3map->tileY *
                               ((RASTER3D_Map *)g3map)->tileZ;

                if (data_type == DCELL_TYPE)
                    tileSize = sizeof(DCELL) * g3map->tileX * g3map->tileY *
                               g3map->tileZ;

                if (G_asprintf(&line, "  Tile size in memory:  %ld Bytes",
                               (long)(tileSize)) > 0)
                    printline(line);
                else
                    G_fatal_error(_("Cannot allocate memory for string"));

                if (G_asprintf(&line,
                               "  Number of tiles in x, y and  z:   %d, %d, %d",
                               g3map->nx, g3map->ny, g3map->nz) > 0)
                    printline(line);
                else
                    G_fatal_error(_("Cannot allocate memory for string"));

                if (G_asprintf(&line,
                               "  Dimension of a tile in x, y, z:   %d, %d, %d",
                               g3map->tileX, g3map->tileY, g3map->tileZ) > 0)
                    printline(line);
                else
                    G_fatal_error(_("Cannot allocate memory for string"));

                printline("");

                if (G_asprintf(&line, "       Projection: %s (zone %d)",
                               G_database_projection_name(), G_zone()) > 0)
                    printline(line);
                else
                    G_fatal_error(_("Cannot allocate memory for string"));

                G_format_northing(cellhd.north, tmp1, cellhd.proj);
                G_format_northing(cellhd.south, tmp2, cellhd.proj);
                G_format_resolution(cellhd.ns_res, tmp3, cellhd.proj);
                if (G_asprintf(&line,
                               "           N: %10s    S: %10s   Res: %5s", tmp1,
                               tmp2, tmp3) > 0)
                    printline(line);
                else
                    G_fatal_error(_("Cannot allocate memory for string"));

                G_format_easting(cellhd.east, tmp1, cellhd.proj);
                G_format_easting(cellhd.west, tmp2, cellhd.proj);
                G_format_resolution(cellhd.ew_res, tmp3, cellhd.proj);
                if (G_asprintf(&line,
                               "           E: %10s    W: %10s   Res: %5s", tmp1,
                               tmp2, tmp3) > 0)
                    printline(line);
                else
                    G_fatal_error(_("Cannot allocate memory for string"));

                format_double(cellhd.top, tmp1);
                format_double(cellhd.bottom, tmp2);
                format_double(cellhd.tb_res, tmp3);
                if (G_asprintf(&line,
                               "           T: %10s    B: %10s   Res: %5s", tmp1,
                               tmp2, tmp3) > 0)
                    printline(line);
                else
                    G_fatal_error(_("Cannot allocate memory for string"));
                if (0 == Rast3d_range_load(g3map))
                    G_fatal_error(
                        _("Unable to read range of 3D raster map <%s>"), name);

                Rast3d_range_min_max(g3map, &dmin, &dmax);

                if (dmin != dmin)
                    snprintf(tmp1, sizeof(tmp1), "%s", "NULL");
                else
                    format_double(dmin, tmp1);
                if (dmax != dmax)
                    snprintf(tmp2, sizeof(tmp2), "%s", "NULL");
                else
                    format_double(dmax, tmp2);

                if (G_asprintf(&line,
                               "  Range of data:   min = %10s max = %10s", tmp1,
                               tmp2) > 0)
                    printline(line);
                else
                    G_fatal_error(_("Cannot allocate memory for string"));
            }

            printline("");

            if (hist_ok) {
                printline("  Data Source:");
                if (G_asprintf(&line, "   %s",
                               Rast_get_history(&hist, HIST_DATSRC_1)) > 0)
                    printline(line);
                else
                    G_fatal_error(_("Cannot allocate memory for string"));

                if (G_asprintf(&line, "   %s",
                               Rast_get_history(&hist, HIST_DATSRC_2)) > 0)
                    printline(line);
                else
                    G_fatal_error(_("Cannot allocate memory for string"));

                printline("");

                printline("  Data Description:");
                if (G_asprintf(&line, "   %s",
                               Rast_get_history(&hist, HIST_KEYWRD)) > 0)
                    printline(line);
                else
                    G_fatal_error(_("Cannot allocate memory for string"));

                printline("");
                if (Rast_history_length(&hist)) {
                    printline("  Comments:  ");

                    for (i = 0; i < Rast_history_length(&hist); i++)

                    /**************************************/
                    {
                        if (G_asprintf(&line, "   %s",
                                       Rast_history_line(&hist, i)) > 0)
                            printline(line);
                        else
                            G_fatal_error(
                                _("Cannot allocate memory for string"));
                    }
                }

                printline("");
            }

            divider('+');

            fprintf(out, "\n");
            break;

        case SHELL:
            fprintf(out, "map=%s\n", name);
            fprintf(out, "date=\"%s\"\n",
                    hist_ok ? Rast_get_history(&hist, HIST_MAPID) : "??");
            fprintf(out, "mapset=%s\n", mapset);
            fprintf(out, "creator=\"%s\"\n",
                    hist_ok ? Rast_get_history(&hist, HIST_CREATOR) : "??");
            fprintf(out, "project=%s\n", G_location());
            fprintf(out, "database=%s\n", G_gisdbase());
            fprintf(out, "title=\"%s\"\n",
                    hist_ok ? Rast_get_history(&hist, HIST_TITLE) : "??");
            fprintf(out, "units=\"%s\"\n", Rast3d_get_unit(g3map));
            fprintf(out, "vertical_units=\"%s\"\n",
                    Rast3d_get_vertical_unit(g3map));

            /*This shows the TimeStamp */
            if (time_ok && (first_time_ok || second_time_ok)) {
                G_format_timestamp(&ts, timebuff);
                fprintf(out, "timestamp=\"%s\"\n", timebuff);
            }
            else {
                fprintf(out, "timestamp=\"none\"\n");
            }

            if (cats_ok) {
                format_double((double)cats.num, tmp1);
            }

            fprintf(out, "maptype=%s\n", "raster_3d");
            fprintf(out, "ncats=%s\n", cats_ok ? tmp1 : "??");
            fprintf(out, "datatype=\"%s\"\n",
                    (data_type == FCELL_TYPE
                         ? "FCELL"
                         : (data_type == DCELL_TYPE ? "DCELL" : "??")));

            if (head_ok) {
                fprintf(out, "rows=%d\n", cellhd.rows);
                fprintf(out, "cols=%d\n", cellhd.cols);
                fprintf(out, "depths=%d\n", cellhd.depths);
                fprintf(out, "cells=%ld\n",
                        (long)cellhd.rows * cellhd.cols * cellhd.depths);

                double totalSize = 0;
                for (i = 0; i < g3map->nTiles; i++)
                    totalSize += g3map->tileLength[i];

                fprintf(out, "size=%ld\n", (long)(totalSize));
                fprintf(out, "ntiles=%d\n", g3map->nTiles);
                fprintf(out, "meansize=%ld\n",
                        (long)(totalSize / g3map->nTiles));

                int tileSize = 0;
                if (data_type == FCELL_TYPE)
                    tileSize = sizeof(FCELL) * g3map->tileX * g3map->tileY *
                               ((RASTER3D_Map *)g3map)->tileZ;

                if (data_type == DCELL_TYPE)
                    tileSize = sizeof(DCELL) * g3map->tileX * g3map->tileY *
                               g3map->tileZ;

                fprintf(out, "tilesize=%ld\n", (long)(tileSize));

                fprintf(out, "tilenumx=%d\n", g3map->nx);
                fprintf(out, "tilenumy=%d\n", g3map->ny);
                fprintf(out, "tilenumz=%d\n", g3map->nz);

                fprintf(out, "tiledimx=%d\n", g3map->tileX);
                fprintf(out, "tiledimy=%d\n", g3map->tileY);
                fprintf(out, "tiledimz=%d\n", g3map->tileZ);

                G_format_northing(cellhd.north, tmp1, cellhd.proj);
                G_format_northing(cellhd.south, tmp2, cellhd.proj);
                G_format_resolution(cellhd.ns_res, tmp3, cellhd.proj);
                fprintf(out, "north=%s\n", tmp1);
                fprintf(out, "south=%s\n", tmp2);
                fprintf(out, "nsres=%s\n", tmp3);

                G_format_easting(cellhd.east, tmp1, cellhd.proj);
                G_format_easting(cellhd.west, tmp2, cellhd.proj);
                G_format_resolution(cellhd.ew_res, tmp3, cellhd.proj);
                fprintf(out, "east=%s\n", tmp1);
                fprintf(out, "west=%s\n", tmp2);
                fprintf(out, "ewres=%s\n", tmp3);

                format_double(cellhd.top, tmp1);
                format_double(cellhd.bottom, tmp2);
                format_double(cellhd.tb_res, tmp3);
                fprintf(out, "top=%s\n", tmp1);
                fprintf(out, "bottom=%s\n", tmp2);
                fprintf(out, "tbres=%s\n", tmp3);

                if (0 == Rast3d_range_load(g3map))
                    G_fatal_error(
                        _("Unable to read range of 3D raster map <%s>"), name);

                Rast3d_range_min_max(g3map, &dmin, &dmax);

                if (dmin != dmin)
                    snprintf(tmp1, sizeof(tmp1), "%s", "NULL");
                else
                    format_double(dmin, tmp1);

                if (dmax != dmax)
                    snprintf(tmp2, sizeof(tmp2), "%s", "NULL");
                else
                    format_double(dmax, tmp2);

                fprintf(out, "min=%s\n", tmp1);
                fprintf(out, "max=%s\n", tmp2);
            }

            if (hist_ok) {
                fprintf(out, "source1=\"%s\"\n",
                        Rast_get_history(&hist, HIST_DATSRC_1));
                fprintf(out, "source2=\"%s\"\n",
                        Rast_get_history(&hist, HIST_DATSRC_2));
                fprintf(out, "description=\"%s\"\n",
                        Rast_get_history(&hist, HIST_KEYWRD));

                if (Rast_history_length(&hist)) {
                    fprintf(out, "comments=\"");
                    for (i = 0; i < Rast_history_length(&hist); i++)
                        fprintf(out, "%s", Rast_history_line(&hist, i));
                    fprintf(out, "\"\n");
                }
            }
            break;

        case JSON:
            G_json_object_set_string(root_object, "map", name);
            G_json_object_set_string(
                root_object, "date",
                hist_ok ? Rast_get_history(&hist, HIST_MAPID) : "??");
            G_json_object_set_string(root_object, "mapset", mapset);
            G_json_object_set_string(
                root_object, "creator",
                hist_ok ? Rast_get_history(&hist, HIST_CREATOR) : "??");
            G_json_object_set_string(root_object, "project", G_location());
            G_json_object_set_string(root_object, "database", G_gisdbase());
            G_json_object_set_string(
                root_object, "title",
                hist_ok ? Rast_get_history(&hist, HIST_TITLE) : "??");
            G_json_object_set_string(root_object, "units",
                                     Rast3d_get_unit(g3map));
            G_json_object_set_string(root_object, "vertical_units",
                                     Rast3d_get_vertical_unit(g3map));

            /*This shows the TimeStamp */
            if (time_ok && (first_time_ok || second_time_ok)) {
                G_format_timestamp(&ts, timebuff);
                G_json_object_set_string(root_object, "timestamp", timebuff);
            }
            else {
                G_json_object_set_null(root_object, "timestamp");
            }

            G_json_object_set_string(root_object, "maptype", "raster_3d");

            if (cats_ok) {
                G_json_object_set_number(root_object, "ncats", cats.num);
            }
            else {
                G_json_object_set_null(root_object, "ncats");
            }

            G_json_object_set_string(
                root_object, "datatype",
                (data_type == FCELL_TYPE
                     ? "FCELL"
                     : (data_type == DCELL_TYPE ? "DCELL" : "??")));

            if (head_ok) {
                G_json_object_set_number(root_object, "rows", cellhd.rows);
                G_json_object_set_number(root_object, "cols", cellhd.cols);
                G_json_object_set_number(root_object, "depths", cellhd.depths);
                G_json_object_set_number(root_object, "cells",
                                         (long)cellhd.rows * cellhd.cols *
                                             cellhd.depths);

                double totalSize = 0;
                for (i = 0; i < g3map->nTiles; i++)
                    totalSize += g3map->tileLength[i];

                G_json_object_set_number(root_object, "size",
                                         (long)(totalSize));
                G_json_object_set_number(root_object, "ntiles", g3map->nTiles);
                G_json_object_set_number(root_object, "meansize",
                                         (long)(totalSize / g3map->nTiles));

                int tileSize = 0;
                if (data_type == FCELL_TYPE)
                    tileSize = sizeof(FCELL) * g3map->tileX * g3map->tileY *
                               ((RASTER3D_Map *)g3map)->tileZ;

                if (data_type == DCELL_TYPE)
                    tileSize = sizeof(DCELL) * g3map->tileX * g3map->tileY *
                               g3map->tileZ;

                G_json_object_set_number(root_object, "tilesize",
                                         (long)(tileSize));

                G_json_object_set_number(root_object, "tilenumx", g3map->nx);
                G_json_object_set_number(root_object, "tilenumy", g3map->ny);
                G_json_object_set_number(root_object, "tilenumz", g3map->nz);

                G_json_object_set_number(root_object, "tiledimx", g3map->tileX);
                G_json_object_set_number(root_object, "tiledimy", g3map->tileY);
                G_json_object_set_number(root_object, "tiledimz", g3map->tileZ);

                G_json_object_set_number(root_object, "north", cellhd.north);
                G_json_object_set_number(root_object, "south", cellhd.south);
                G_json_object_set_number(root_object, "nsres", cellhd.ns_res);

                G_json_object_set_number(root_object, "east", cellhd.east);
                G_json_object_set_number(root_object, "west", cellhd.west);
                G_json_object_set_number(root_object, "ewres", cellhd.ew_res);

                G_json_object_set_number(root_object, "top", cellhd.top);
                G_json_object_set_number(root_object, "bottom", cellhd.bottom);
                G_json_object_set_number(root_object, "tbres", cellhd.tb_res);

                if (0 == Rast3d_range_load(g3map))
                    G_fatal_error(
                        _("Unable to read range of 3D raster map <%s>"), name);

                Rast3d_range_min_max(g3map, &dmin, &dmax);

                if (dmin != dmin)
                    G_json_object_set_null(root_object, "min");
                else
                    G_json_object_set_number(root_object, "min", dmin);

                if (dmax != dmax)
                    G_json_object_set_null(root_object, "max");
                else
                    G_json_object_set_number(root_object, "max", dmax);
            }

            if (hist_ok) {
                G_json_object_set_string(
                    root_object, "source1",
                    Rast_get_history(&hist, HIST_DATSRC_1));
                G_json_object_set_string(
                    root_object, "source2",
                    Rast_get_history(&hist, HIST_DATSRC_2));
                G_json_object_set_string(root_object, "description",
                                         Rast_get_history(&hist, HIST_KEYWRD));
                char *buffer = history_as_string(&hist);
                if (buffer) {
                    G_json_object_set_string(root_object, "comments", buffer);
                    G_free(buffer);
                }
                else {
                    G_json_object_set_null(root_object, "comments");
                }
            }
            break;
        }
    }
    else { /* Print information */
        if (gflag->answer) {
            switch (format) {
            case PLAIN:
                snprintf(tmp1, sizeof(tmp1), "%f", cellhd.north);
                snprintf(tmp2, sizeof(tmp2), "%f", cellhd.south);
                G_trim_decimal(tmp1);
                G_trim_decimal(tmp2);
                fprintf(out, "North: %s\n", tmp1);
                fprintf(out, "South: %s\n", tmp2);

                snprintf(tmp1, sizeof(tmp1), "%f", cellhd.east);
                snprintf(tmp2, sizeof(tmp2), "%f", cellhd.west);
                G_trim_decimal(tmp1);
                G_trim_decimal(tmp2);
                fprintf(out, "East: %s\n", tmp1);
                fprintf(out, "West: %s\n", tmp2);

                fprintf(out, "Bottom: %g\n", cellhd.bottom);
                fprintf(out, "Top: %g\n", cellhd.top);

                G_format_resolution(cellhd.ns_res, tmp3, cellhd.proj);
                fprintf(out, "North-south resolution: %s\n", tmp3);

                G_format_resolution(cellhd.ew_res, tmp3, cellhd.proj);
                fprintf(out, "East-west resolution: %s\n", tmp3);

                fprintf(out, "Top-Bottom resolution: %g\n", cellhd.tb_res);

                fprintf(out, "Rows: %d\n", cellhd.rows);
                fprintf(out, "Columns: %d\n", cellhd.cols);
                fprintf(out, "Depths: %d\n", cellhd.depths);

                fprintf(out, "Data Type: %s\n",
                        data_type == FCELL_TYPE   ? "FCELL"
                        : data_type == DCELL_TYPE ? "DCELL"
                                                  : "??");

                if (time_ok && (first_time_ok || second_time_ok)) {
                    G_format_timestamp(&ts, timebuff);
                    fprintf(out, "Timestamp: %s\n", timebuff);
                }
                else {
                    fprintf(out, "Timestamp: none\n");
                }
                fprintf(out, "Units: %s\n", Rast3d_get_unit(g3map));
                fprintf(out, "Vertical unit: %s\n",
                        Rast3d_get_vertical_unit(g3map));
                fprintf(out, "Number of tiles in x: %d\n", g3map->nx);
                fprintf(out, "Number of tiles in y: %d\n", g3map->ny);
                fprintf(out, "Number of tiles in z: %d\n", g3map->nz);
                fprintf(out, "Dimension of a tile in x: %d\n", g3map->tileX);
                fprintf(out, "Dimension of a tile in y: %d\n", g3map->tileY);
                fprintf(out, "Dimension of a tile in z: %d\n", g3map->tileZ);
                break;

            case SHELL:
                snprintf(tmp1, sizeof(tmp1), "%f", cellhd.north);
                snprintf(tmp2, sizeof(tmp2), "%f", cellhd.south);
                G_trim_decimal(tmp1);
                G_trim_decimal(tmp2);
                fprintf(out, "north=%s\n", tmp1);
                fprintf(out, "south=%s\n", tmp2);

                snprintf(tmp1, sizeof(tmp1), "%f", cellhd.east);
                snprintf(tmp2, sizeof(tmp2), "%f", cellhd.west);
                G_trim_decimal(tmp1);
                G_trim_decimal(tmp2);
                fprintf(out, "east=%s\n", tmp1);
                fprintf(out, "west=%s\n", tmp2);

                fprintf(out, "bottom=%g\n", cellhd.bottom);
                fprintf(out, "top=%g\n", cellhd.top);

                G_format_resolution(cellhd.ns_res, tmp3, cellhd.proj);
                fprintf(out, "nsres=%s\n", tmp3);

                G_format_resolution(cellhd.ew_res, tmp3, cellhd.proj);
                fprintf(out, "ewres=%s\n", tmp3);

                fprintf(out, "tbres=%g\n", cellhd.tb_res);

                fprintf(out, "rows=%d\n", cellhd.rows);
                fprintf(out, "cols=%d\n", cellhd.cols);
                fprintf(out, "depths=%d\n", cellhd.depths);

                fprintf(out, "datatype=\"%s\"\n",
                        data_type == FCELL_TYPE   ? "FCELL"
                        : data_type == DCELL_TYPE ? "DCELL"
                                                  : "??");

                if (time_ok && (first_time_ok || second_time_ok)) {
                    G_format_timestamp(&ts, timebuff);
                    fprintf(out, "timestamp=\"%s\"\n", timebuff);
                }
                else {
                    fprintf(out, "timestamp=\"none\"\n");
                }
                fprintf(out, "units=\"%s\"\n", Rast3d_get_unit(g3map));
                fprintf(out, "vertical_units=\"%s\"\n",
                        Rast3d_get_vertical_unit(g3map));
                fprintf(out, "tilenumx=%d\n", g3map->nx);
                fprintf(out, "tilenumy=%d\n", g3map->ny);
                fprintf(out, "tilenumz=%d\n", g3map->nz);
                fprintf(out, "tiledimx=%d\n", g3map->tileX);
                fprintf(out, "tiledimy=%d\n", g3map->tileY);
                fprintf(out, "tiledimz=%d\n", g3map->tileZ);
                break;

            case JSON:
                G_json_object_set_number(root_object, "north", cellhd.north);
                G_json_object_set_number(root_object, "south", cellhd.south);
                G_json_object_set_number(root_object, "east", cellhd.east);
                G_json_object_set_number(root_object, "west", cellhd.west);
                G_json_object_set_number(root_object, "bottom", cellhd.bottom);
                G_json_object_set_number(root_object, "top", cellhd.top);

                G_json_object_set_number(root_object, "nsres", cellhd.ns_res);
                G_json_object_set_number(root_object, "ewres", cellhd.ew_res);
                G_json_object_set_number(root_object, "tbres", cellhd.tb_res);

                G_json_object_set_number(root_object, "rows", cellhd.rows);
                G_json_object_set_number(root_object, "cols", cellhd.cols);
                G_json_object_set_number(root_object, "depths", cellhd.depths);

                G_json_object_set_string(
                    root_object, "datatype",
                    (data_type == FCELL_TYPE
                         ? "FCELL"
                         : (data_type == DCELL_TYPE ? "DCELL" : "??")));

                if (time_ok && (first_time_ok || second_time_ok)) {
                    G_format_timestamp(&ts, timebuff);
                    G_json_object_set_string(root_object, "timestamp",
                                             timebuff);
                }
                else {
                    G_json_object_set_null(root_object, "timestamp");
                }

                G_json_object_set_string(root_object, "units",
                                         Rast3d_get_unit(g3map));
                G_json_object_set_string(root_object, "vertical_units",
                                         Rast3d_get_vertical_unit(g3map));

                G_json_object_set_number(root_object, "tilenumx", g3map->nx);
                G_json_object_set_number(root_object, "tilenumy", g3map->ny);
                G_json_object_set_number(root_object, "tilenumz", g3map->nz);

                G_json_object_set_number(root_object, "tiledimx", g3map->tileX);
                G_json_object_set_number(root_object, "tiledimy", g3map->tileY);
                G_json_object_set_number(root_object, "tiledimz", g3map->tileZ);
                break;
            }
        }
        if (rflag->answer) {
            if (0 == Rast3d_range_load(g3map))
                G_fatal_error(_("Unable to read range of 3D raster map <%s>"),
                              name);

            Rast3d_range_min_max(g3map, &dmin, &dmax);

            switch (format) {
            case PLAIN:
                if (dmin != dmin)
                    fprintf(out, "Minimum: NULL\n");
                else
                    fprintf(out, "Minimum: %f\n", dmin);
                if (dmax != dmax)
                    fprintf(out, "Maximum: NULL\n");
                else
                    fprintf(out, "Maximum: %f\n", dmax);
                break;

            case SHELL:
                if (dmin != dmin)
                    fprintf(out, "min=NULL\n");
                else
                    fprintf(out, "min=%f\n", dmin);
                if (dmax != dmax)
                    fprintf(out, "max=NULL\n");
                else
                    fprintf(out, "max=%f\n", dmax);
                break;

            case JSON:
                if (dmin != dmin)
                    G_json_object_set_null(root_object, "min");
                else
                    G_json_object_set_number(root_object, "min", dmin);
                if (dmax != dmax)
                    G_json_object_set_null(root_object, "max");
                else
                    G_json_object_set_number(root_object, "max", dmax);
                break;
            }
        }
        if (hflag->answer) {
            if (hist_ok) {
                switch (format) {
                case PLAIN:
                    fprintf(out, "Title:\n");
                    fprintf(out, "   %s\n",
                            Rast_get_history(&hist, HIST_TITLE));
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

                case SHELL:
                    fprintf(out, "title=");
                    fprintf(out, "\"%s\"\n",
                            Rast_get_history(&hist, HIST_TITLE));
                    fprintf(out, "source1=");
                    fprintf(out, "\"%s\"\n",
                            Rast_get_history(&hist, HIST_DATSRC_1));
                    fprintf(out, "source2=");
                    fprintf(out, "\"%s\"\n",
                            Rast_get_history(&hist, HIST_DATSRC_2));
                    fprintf(out, "description=");
                    fprintf(out, "\"%s\"\n",
                            Rast_get_history(&hist, HIST_KEYWRD));
                    if (Rast_history_length(&hist)) {
                        fprintf(out, "comments=\"");
                        for (i = 0; i < Rast_history_length(&hist); i++)
                            fprintf(out, "%s", Rast_history_line(&hist, i));
                        fprintf(out, "\"\n");
                    }
                    break;

                case JSON:
                    G_json_object_set_string(
                        root_object, "title",
                        Rast_get_history(&hist, HIST_TITLE));
                    G_json_object_set_string(
                        root_object, "source1",
                        Rast_get_history(&hist, HIST_DATSRC_1));
                    G_json_object_set_string(
                        root_object, "source2",
                        Rast_get_history(&hist, HIST_DATSRC_2));
                    G_json_object_set_string(
                        root_object, "description",
                        Rast_get_history(&hist, HIST_KEYWRD));
                    char *buffer = history_as_string(&hist);
                    if (buffer) {
                        G_json_object_set_string(root_object, "comments",
                                                 buffer);
                        G_free(buffer);
                    }
                    else {
                        G_json_object_set_null(root_object, "comments");
                    }
                    break;
                }
            }
            else {
                G_fatal_error(_("Error while reading history file"));
            }
        }
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

    /*Close the opened map */
    if (!Rast3d_close(g3map))
        G_fatal_error(_("Unable to close 3D raster map <%s>"), name);

    return 0;
}

/**************************************************************************/
static int format_double(double value, char buf[100])
{
    snprintf(buf, TMP_LENGTH, "%.8f", value);
    G_trim_decimal(buf);
    return 0;
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

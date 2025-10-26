#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "watershed.h"
#include "string.h"

/* make sure any useful info is transferred to the man page before ripping out
 * the interactive help messages */
/* in addition there seem to be some useful user options here which are not
 * currently available from the main parser */
int com_line_Gwater(INPUT *input, OUTPUT *output)
{
    struct Cell_head *window;
    char map_layer[48], buf[100], *prog_name, *mapset;
    double d;
    int i;

    window = &(output->window);
    if (0 == G_yes("Continue?", 1))
        exit(EXIT_SUCCESS);

    input->haf_name = (char *)G_calloc(40, sizeof(char));
    input->accum_name = (char *)G_calloc(40, sizeof(char));

    G_message(
        _("\nThis set of questions will organize the command line for the"));
    G_message(_("%s program to run properly for your application."), NON_NAME);
    G_message(_("The first question is whether you want %s to run"), NON_NAME);
    G_message(_("in its fast mode or its slow mode.  If you run %s"), NON_NAME);
    G_message(
        _("in the fast mode, the computer will finish about 10 times faster"));
    G_message(
        _("than in the slow mode, but will not allow other programs to run"));
    G_message(
        _("at the same time.  The fast mode also places all of the data into"));
    G_message(
        _("RAM, which limits the size of window that can be run.  The slow"));
    G_message(_("mode uses disk space in the same hard disk partition as where "
                "GRASS is"));
    G_message(_("stored.  Thus, if the program does not work in the slow mode, "
                "you will"));
    G_message(_("need to remove unnecessary files from that partition.  The "
                "slow mode"));
    G_message(_("will allow other processes to run concurrently with %s.\n"),
              NON_NAME);

    snprintf(buf, sizeof(buf), "Do you want to use the fast mode of %s?",
             NON_NAME);
    input->com_line_ram = input->com_line_seg = NULL;
    input->fast = 0;
    input->slow = 0;
    if (G_yes(buf, 1)) {
        input->fast = 1;
        input->com_line_ram = (char *)G_calloc(400, sizeof(char));
        prog_name = G_store(RAM_NAME);
        snprintf(input->com_line_ram, (400 * sizeof(char)),
                 "\"%s/etc/water/%s\"", G_gisbase(), RAM_NAME);
        fprintf(stderr,
                "\nIf there is not enough ram for the fast mode (%s) to run,\n",
                RAM_NAME);
        snprintf(buf, sizeof(buf), "should the slow mode (%s) be run instead?",
                 SEG_NAME);
        if (G_yes(buf, 1)) {
            input->slow = 1;
            input->com_line_seg = (char *)G_calloc(400, sizeof(char));
            snprintf(input->com_line_seg, (400, sizeof(char)),
                     "\"%s/etc/water/%s\"", G_gisbase(), SEG_NAME);
        }
    }
    else {
        input->slow = 1;
        prog_name = G_store(SEG_NAME);
        input->com_line_seg = (char *)G_calloc(400, sizeof(char));
        snprintf(input->com_line_seg, (400, sizeof(char)),
                 "\"%s/etc/water/%s\"", G_gisbase(), SEG_NAME);
    }

    G_message(_("\nIf you hit <return> by itself for the next question, this"));
    G_message(_("program will terminate."));

    mapset = G_ask_old("What is the name of the elevation map layer?",
                       map_layer, "cell", "cell");
    if (!mapset)
        exit(EXIT_FAILURE);
    if (input->fast)
        com_line_add(&(input->com_line_ram), " el=", map_layer, mapset);
    if (input->slow)
        com_line_add(&(input->com_line_seg), " el=", map_layer, mapset);

    G_message(_("\nOne of the options for %s is a `depression map'.  A"),
              prog_name);
    G_message(_("depression map indicates all the locations in the current map "
                "window where"));
    G_message(_("water accumulates and does not leave by the edge of the map. "
                "Lakes without"));
    G_message(_("outlet streams and sinkholes are examples of `depressions'.  "
                "If you wish to"));
    G_message(_("have a depression map, prepare a map where non-zero values "
                "indicate the"));
    G_message(_("locations where depressions occur.\n"));
    G_message(_("Hit <return> by itself for the next question if there is no "
                "depression map."));

    mapset = G_ask_old("What is the name of the depression map layer?",
                       map_layer, "cell", "cell");
    if (mapset) {
        if (input->fast)
            com_line_add(&(input->com_line_ram), " de=", map_layer, mapset);
        if (input->slow)
            com_line_add(&(input->com_line_seg), " de=", map_layer, mapset);
    }

    G_message(
        _("\nThe %s program will divide the elevation map into a number of"),
        prog_name);
    G_message(_("watershed basins.  The number of watershed basins is "
                "indirectly determined"));
    G_message(_("by the `basin threshold' value.  The basin threshold is the "
                "area necessary for"));
    G_message(
        _("%s to define a unique watershed basin.  This area only applies to"),
        prog_name);
    G_message(_("`exterior drainage basins'.  An exterior drainage basin does "
                "not have any"));
    G_message(_("drainage basins flowing into it.  Interior drainage basin "
                "size is determined"));
    G_message(_("by the surface flow going into stream segments between stream "
                "interceptions."));
    G_message(
        _("Thus interior drainage basins can be of any size.  The %s program"),
        prog_name);
    G_message(_("also allows the user to relate basin size to potential "
                "overland flow"));
    G_message(_("(i.e., areas with low infiltration capacities will need "
                "smaller areas to"));
    G_message(_("develop stream channels than neighboring areas with high "
                "infiltration rates)."));
    G_message(_("The user can create a map layer with potential overland flow "
                "values, and"));
    G_message(_("%s will accumulate those values instead of area.\n"),
              prog_name);
    G_message(_("What unit of measure will you use for the basin threshold:"));

    do {
        G_message(
            _(" 1) acres,          2) meters sq., 3) miles sq., 4) hectares,"));
        G_message(
            _(" 5) kilometers sq., 6) map cells,  7) overland flow units"));
        fprintf(stderr, _("Choose 1-7 or 0 to exit this program: "));
        G_gets(map_layer);
        sscanf(map_layer, "%d", &i);
    } while (i > 7 || i < 0);

    if (!i)
        exit(EXIT_SUCCESS);

    output->type_area = (char)i;

    G_message(_("\nHow large an area (or how many overland flow units) must a "
                "drainage basin"));
    fprintf(stderr, _("be for it to be an exterior drainage basin: "));
    G_gets(map_layer);
    sscanf(map_layer, "%lf", &d);

    switch (i) {
    case 1:
        if (input->fast)
            basin_com_add(&(input->com_line_ram), d, ACRE_TO_METERSQ, window);
        if (input->slow)
            basin_com_add(&(input->com_line_seg), d, ACRE_TO_METERSQ, window);
        break;
    case 2:
        if (input->fast)
            basin_com_add(&(input->com_line_ram), d, 1.0, window);
        if (input->slow)
            basin_com_add(&(input->com_line_seg), d, 1.0, window);
        break;
    case 3:
        if (input->fast)
            basin_com_add(&(input->com_line_ram), d, MILESQ_TO_METERSQ, window);
        if (input->slow)
            basin_com_add(&(input->com_line_seg), d, MILESQ_TO_METERSQ, window);
        break;
    case 4:
        if (input->fast)
            basin_com_add(&(input->com_line_ram), d, HECTACRE_TO_METERSQ,
                          window);
        if (input->slow)
            basin_com_add(&(input->com_line_seg), d, HECTACRE_TO_METERSQ,
                          window);
        break;
    case 5:
        if (input->fast)
            basin_com_add(&(input->com_line_ram), d, KILOSQ_TO_METERSQ, window);
        if (input->slow)
            basin_com_add(&(input->com_line_seg), d, KILOSQ_TO_METERSQ, window);
        break;
    case 6:
        if (input->fast)
            basin_com_add(&(input->com_line_ram), d,
                          (window->ns_res * window->ew_res), window);
        if (input->slow)
            basin_com_add(&(input->com_line_seg), d,
                          (window->ns_res * window->ew_res), window);
        break;
    case 7: /* needs an overland flow map */
        G_message(
            _("\nIf you hit <return> by itself for the next question, this"));
        G_message(_("program will terminate."));
        mapset = G_ask_old("What is the name of the overland flow map layer?",
                           map_layer, "cell", "cell");
        if (!mapset)
            exit(EXIT_FAILURE);
        if (input->fast) {
            com_line_add(&(input->com_line_ram), " ov=", map_layer, mapset);
            basin_com_add(&(input->com_line_ram), d,
                          (window->ns_res * window->ew_res), window);
        }
        if (input->slow) {
            com_line_add(&(input->com_line_seg), " ov=", map_layer, mapset);
            basin_com_add(&(input->com_line_seg), d,
                          (window->ns_res * window->ew_res), window);
        }
        break;
    }

    G_message(_("\n%s must create a map layer of watershed basins"), prog_name);
    G_message(_("before %s can run properly."), G_program_name());

    strcpy(buf, "Please name the output watershed basin map:");
    do {
        mapset = G_ask_new(buf, input->haf_name, "cell", "");
    } while (NULL == mapset);

    if (input->fast)
        com_line_add(&(input->com_line_ram), " ba=", input->haf_name, NULL);
    if (input->slow)
        com_line_add(&(input->com_line_seg), " ba=", input->haf_name, NULL);

        /*
           This section queries the user about the armsed file input. If
           you want to make this an option,  the code below "COMMENT2" needs to
           be modified.
         */

#ifdef ARMSED
    G_message(_("\n%s must create a file of watershed basin relationships"),
              prog_name);
    G_message(_("before %s can run properly."), G_program_name());

    input->ar_file_name = NULL;
    while (input->ar_file_name == NULL) {
        fprintf(stderr, _("\nPlease name this file:"));
        G_gets(char_input);
        if (1 != G_legal_filename(char_input)) {
            G_message(_("<%s> is an illegal file name"), char_input);
        }
        else
            input->ar_file_name = G_store(char_input);
    }

    if (input->fast)
        com_line_add(&(input->com_line_ram), " ar=", input->ar_file_name, NULL);
    if (input->slow)
        com_line_add(&(input->com_line_seg), " ar=", input->ar_file_name, NULL);

    /*
       end of ARMSED comment code
     */

    /*
       COMMENT2 This section of code tells the program where to place the
       statistics about the watershed basin. GRASS users don't need this (w/
       r.stats), but the format is supposed to be "user-friendly" to
       hydrologists. For the stats to be created, the armsed file output needs
       to exist. For the stats to be an option in this program: 1) it should be
       queried before the armsed file query, and 2) make the armsed file query
       mandatory if this option is invoked.
     */

    G_message(_("\n%s will generate a lot of output.  Indicate a file"),
              G_program_name());
    G_message(_("name for %s to send the output to."), G_program_name());

    output->file_name = NULL;
    while (output->file_name == NULL) {
        fprintf(stderr, _("\nPlease name this file:"));
        G_gets(char_input);
        if (1 != G_legal_filename(char_input)) {
            G_message(_("<%s> is an illegal file name"), char_input);
        }
        else
            output->file_name = G_store(char_input);
    }

    /*
       end of COMMENT2
     */
#endif

    G_message(_("\nThe accumulation map from %s must be present for"),
              prog_name);
    G_message(_("%s to work properly."), G_program_name());
    strcpy(buf, "Please name the accumulation map:");
    do {
        mapset = G_ask_new(buf, input->accum_name, "cell", "");
    } while (NULL == mapset);

    if (input->fast)
        com_line_add(&(input->com_line_ram), " ac=", input->accum_name, NULL);
    if (input->slow)
        com_line_add(&(input->com_line_seg), " ac=", input->accum_name, NULL);

    G_message(_("\n%s can produce several maps not necessary for"), prog_name);
    G_message(_("%s to function (stream channels, overland flow aspect, and"),
              G_program_name());
    G_message(_("a display version of the accumulation map).  %s also has the"),
              prog_name);
    G_message(_("ability to generate several variables in the Revised "
                "Universal Soil Loss"));
    G_message(
        _("Equation (Rusle): Slope Length (LS), and Slope Steepness (S).\n"));

    snprintf(buf, sizeof(buf),
             "Would you like any of these maps to be created?");
    if (G_yes(buf, 1)) {
        mapset = G_ask_new("", map_layer, "cell", "stream channel");
        if (mapset != NULL) {
            if (input->fast)
                com_line_add(&(input->com_line_ram), " se=", map_layer, NULL);
            if (input->slow)
                com_line_add(&(input->com_line_seg), " se=", map_layer, NULL);
        }
        mapset = G_ask_new("", map_layer, "cell", "half basin");
        if (mapset != NULL) {
            if (input->fast)
                com_line_add(&(input->com_line_ram), " ha=", map_layer, NULL);
            if (input->slow)
                com_line_add(&(input->com_line_seg), " ha=", map_layer, NULL);
        }
        mapset = G_ask_new("", map_layer, "cell", "overland aspect");
        if (mapset != NULL) {
            if (input->fast)
                com_line_add(&(input->com_line_ram), " dr=", map_layer, NULL);
            if (input->slow)
                com_line_add(&(input->com_line_seg), " dr=", map_layer, NULL);
        }
        mapset = G_ask_new("", map_layer, "cell", "display");
        if (mapset != NULL) {
            if (input->fast)
                com_line_add(&(input->com_line_ram), " di=", map_layer, NULL);
            if (input->slow)
                com_line_add(&(input->com_line_seg), " di=", map_layer, NULL);
        }
        i = 0;
        mapset = G_ask_new("", map_layer, "cell", "Slope Length");
        if (mapset != NULL) {
            i = 1;
            if (input->fast)
                com_line_add(&(input->com_line_ram), " LS=", map_layer, NULL);
            if (input->slow)
                com_line_add(&(input->com_line_seg), " LS=", map_layer, NULL);
        }
        mapset = G_ask_new("", map_layer, "cell", "Slope Steepness");
        if (mapset != NULL) {
            i = 1;
            if (input->fast)
                com_line_add(&(input->com_line_ram), " S=", map_layer, NULL);
            if (input->slow)
                com_line_add(&(input->com_line_seg), " S=", map_layer, NULL);
        }

        if (i) {
            G_message(_("\nThe Slope Length factor (LS) and Slope Steepness "
                        "(S) are influenced by"));
            G_message(_("disturbed land.  %s reflects this with an optional "
                        "map layer or value"),
                      prog_name);
            G_message(_("where the value indicates the percent of disturbed "
                        "(barren) land in that cell."));
            G_message(_("Type <return> if you do not have a disturbed land map "
                        "layer."));

            mapset = G_ask_old("", map_layer, "cell", "disturbed land");
            if (mapset != NULL) {
                if (input->fast)
                    com_line_add(&(input->com_line_ram), " r=", map_layer,
                                 NULL);
                if (input->slow)
                    com_line_add(&(input->com_line_seg), " r=", map_layer,
                                 NULL);
            }
            else {
                G_message(_("\nType the value indicating the percent of "
                            "disturbed land.  This value will"));
                G_message(_("be used for every cell in the current region."));
                i = -6;
                while (i < 0 || i > 100) {
                    fprintf(stderr, _("\nInput value here [0-100]: "));
                    fgets(buf, 80, stdin);
                    sscanf(buf, "%d", &i);
                }
                if (input->fast)
                    com_add(&(input->com_line_ram), " r=", i);
                if (input->slow)
                    com_add(&(input->com_line_seg), " r=", i);
            }

            /*       12345678901234567890123456789012345678901234567890123456789012345678901234567890
             */
            G_message(_("\nOverland surface flow only occurs for a set "
                        "distance before swales form."));
            G_message(_("Because of digital terrain model limitations, %s "
                        "cannot pick up"),
                      prog_name);
            G_message(_("these swales.  %s allows for an input (warning: "
                        "kludge factor)"),
                      prog_name);
            G_message(_("that prevents the surface flow distance from getting "
                        "too long.  Normally,"));
            G_message(_(
                "maximum slope length is around 600 feet (about 183 meters)."));

            i = -1;
            while (i < 0) {
                fprintf(stdout,
                        "\nInput maximum slope length here (in meters): ");
                fgets(buf, 80, stdin);
                sscanf(buf, "%d", &i);
            }
            if (input->fast)
                com_add(&(input->com_line_ram), " ms=", i);
            if (input->slow)
                com_add(&(input->com_line_seg), " ms=", i);

            /*       12345678901234567890123456789012345678901234567890123456789012345678901234567890
             */
            G_message(_("\nRoads, ditches, changes in ground cover, and other "
                        "factors will stop"));
            G_message(_("slope length.  You may input a raster map indicating "
                        "the locations of these"));
            G_message(_("blocking factors.\n"));
            G_message(_("Hit <return> by itself for the next question if there "
                        "is no blocking map."));

            mapset = G_ask_old("What is the name of the blocking map layer?",
                               map_layer, "cell", "cell");
            if (mapset) {
                if (input->fast)
                    com_line_add(&(input->com_line_ram), " ob=", map_layer,
                                 mapset);
                if (input->slow)
                    com_line_add(&(input->com_line_seg), " ob=", map_layer,
                                 mapset);
            }
        }
    }

    return 0;
}

int com_line_add(char **com_line, char *prompt, char *map_layer, char *mapset)
{
    strcat(*com_line, prompt);
    strcat(*com_line, "\"");
    strcat(*com_line, map_layer);
    if (mapset) {
        strcat(*com_line, "@");
        strcat(*com_line, mapset);
    }
    strcat(*com_line, "\"");

    return 0;
}

int basin_com_add(char **com_line, double d, double modifier,
                  struct Cell_head *window)
{
    int i;
    char buf[20];

    i = (int)(.5 + modifier * d / window->ns_res / window->ew_res);
    if (i < 1)
        i = 1;
    snprintf(buf, sizeof(buf), " t=%d", i);
    strcat(*com_line, buf);

    return 0;
}

int com_add(char **com_line, char *prompt, int ril_value)
{
    char buf[20];

    strcat(*com_line, prompt);
    snprintf(buf, sizeof(buf), "%d", ril_value);
    strcat(*com_line, buf);

    return 0;
}

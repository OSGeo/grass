/****************************************************************************
 *
 * MODULE:       d.redraw
 * AUTHOR(S):    Huidae Cho <grass4u gmail.com>
 *               Based on scripts/d.redraw/d.redraw.py by Martin Landa
 * PURPOSE:      Redraws the content of currently selected monitor
 * COPYRIGHT:    (C) 2024 by the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2). Read the file COPYING that
 *               comes with GRASS for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/spawn.h>
#include <grass/display.h>
#include <grass/glocale.h>

#define LINES_SIZE_INC 1024
#define LINE_LEN       1024

int main(int argc, char **argv)
{
    struct GModule *module;
    const char *mon;
    char element[GPATH_MAX], cmd_file[GPATH_MAX];
    FILE *fp;
    int lines_size, num_lines, num_comment_lines;
    char **lines, line[LINE_LEN];
    char **cmd_argv;
    int i;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("display"));
    G_add_keyword(_("graphics"));
    G_add_keyword(_("monitors"));
    module->description =
        _("Redraws the content of currently selected monitor.");

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    if (!(mon = G_getenv_nofatal("MONITOR")))
        G_fatal_error(_("No graphics device selected. Use d.mon to select "
                        "graphics device."));

    D_open_driver();
    D_close_driver();

    G_temp_element(element);
    strcat(element, "/MONITORS/");
    strcat(element, mon);
    G_file_name(cmd_file, element, "cmd", G_mapset());

    if (!(fp = fopen(cmd_file, "r")))
        G_fatal_error(_("Unable to open file '%s' for reading."), cmd_file);

    lines_size = num_lines = num_comment_lines = 0;
    lines = NULL;

    /* read and save cmd lines; run display commands now */
    while (G_getl2(line, LINE_LEN, fp)) {
        /* don't add d.redraw this time */
        if (strcmp(line, "d.redraw") == 0) {
            /* remove its comment lines above */
            num_lines -= num_comment_lines;
            continue;
        }
        if (lines_size == num_lines) {
            lines_size += LINES_SIZE_INC;
            lines = G_realloc(lines, sizeof(char *) * lines_size);
        }
        lines[num_lines++] = G_store(line);

        if (*line == '#') {
            num_comment_lines++;
            /* render next command into the same existing file */
            if (strstr(line, "# GRASS_RENDER_") == line)
                putenv(G_store(line + 2));
            continue;
        }
        num_comment_lines = 0;

        /* split line by space; double-quote delimiters protect spaces */
        cmd_argv = G_tokenize2(line, " ", "\"");
        /* run display command */
        G_vspawn_ex(cmd_argv[0], (const char **)cmd_argv);
        G_free_tokens(cmd_argv);
    }

    fclose(fp);

    /* write out cmd file without d.redraw */
    if (!(fp = fopen(cmd_file, "w")))
        G_fatal_error(_("Unable to open file '%s' for writing."), cmd_file);

    for (i = 0; i < num_lines; i++)
        fprintf(fp, "%s\n", lines[i]);

    fclose(fp);

    exit(EXIT_SUCCESS);
}

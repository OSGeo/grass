/****************************************************************************
 *
 * MODULE:       g.mapsets
 * AUTHOR(S):    Michael Shapiro (CERL),
 *               Greg Koerper (ManTech Environmental Technology) (original
 *                 contributors),
 *               Glynn Clements <glynn gclements.plus.com>
 *               Hamish Bowman <hamish_b yahoo.com>,
 *               Markus Neteler <neteler itc.it>,
 *               Moritz Lennert <mlennert club.worldonline.be>,
 *               Martin Landa <landa.martin gmail.com>,
 *               Huidae Cho <grass4u gmail.com>,
 *               Corey White <smortopahri gmail.com>
 * PURPOSE:      set current mapset path
 * COPYRIGHT:    (C) 1994-2009, 2012-2024 by the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2). Read the file COPYING that
 *               comes with GRASS for details.
 *
 *****************************************************************************/

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/spawn.h>
#include <grass/glocale.h>

#include "local_proto.h"

#define OP_UKN 0
#define OP_SET 1
#define OP_ADD 2
#define OP_REM 3

enum OutputFormat { PLAIN, JSON };

void fatal_error_option_value_excludes_flag(struct Option *option,
                                            struct Flag *excluded,
                                            const char *because)
{
    if (!excluded->answer)
        return;
    G_fatal_error(_("The flag -%c is not allowed with %s=%s. %s"),
                  excluded->key, option->key, option->answer, because);
}

void fatal_error_option_value_excludes_option(struct Option *option,
                                              struct Option *excluded,
                                              const char *because)
{
    if (!excluded->answer)
        return;
    G_fatal_error(_("The option %s is not allowed with %s=%s. %s"),
                  excluded->key, option->key, option->answer, because);
}

static void append_mapset(char **, const char *);

int main(int argc, char *argv[])
{
    int n, i;
    int skip;
    const char *cur_mapset, *mapset;
    char **ptr;
    char **tokens;
    int no_tokens;
    FILE *fp;
    char path_buf[GPATH_MAX];
    char *path, *fsep;
    int operation, nchoices;
    enum OutputFormat format;
    char **mapset_name;
    int nmapsets;

    struct GModule *module;
    struct _opt {
        struct Option *mapset, *op, *format, *fsep;
        struct Flag *print, *list, *dialog;
    } opt;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("general"));
    G_add_keyword(_("settings"));
    G_add_keyword(_("search path"));
    module->label = _("Modifies/prints the user's current mapset search path.");
    module->description = _("Affects the user's access to data existing "
                            "under the other mapsets in the current location.");

    opt.mapset = G_define_standard_option(G_OPT_M_MAPSET);
    opt.mapset->required = YES;
    opt.mapset->multiple = YES;
    opt.mapset->description =
        _("Name(s) of existing mapset(s) to add/remove or set");

    opt.op = G_define_option();
    opt.op->key = "operation";
    opt.op->type = TYPE_STRING;
    opt.op->required = YES;
    opt.op->multiple = NO;
    opt.op->options = "set,add,remove";
    opt.op->description = _("Operation to be performed");
    opt.op->answer = "add";

    opt.format = G_define_option();
    opt.format->key = "format";
    opt.format->type = TYPE_STRING;
    opt.format->required = YES;
    opt.format->label = _("Output format for printing (-l and -p flags)");
    opt.format->options = "plain,json";
    opt.format->descriptions = "plain;Configurable plain text output;"
                               "json;JSON (JavaScript Object Notation);";
    opt.format->answer = "plain";
    opt.format->guisection = _("Print");

    opt.fsep = G_define_standard_option(G_OPT_F_SEP);
    opt.fsep->answer = NULL;
    opt.fsep->guisection = _("Print");

    opt.list = G_define_flag();
    opt.list->key = 'l';
    opt.list->description =
        _("List all available mapsets in alphabetical order");
    opt.list->guisection = _("Print");
    opt.list->suppress_required = YES;

    opt.print = G_define_flag();
    opt.print->key = 'p';
    opt.print->description = _("Print mapsets in current search path");
    opt.print->guisection = _("Print");
    opt.print->suppress_required = YES;

    opt.dialog = G_define_flag();
    opt.dialog->key = 's';
    opt.dialog->description = _("Launch mapset selection GUI dialog");
    opt.dialog->suppress_required = YES;

    path = NULL;
    mapset_name = NULL;
    nmapsets = nchoices = 0;

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    operation = OP_UKN;
    if (opt.mapset->answer && opt.op->answer) {
        switch (opt.op->answer[0]) {
        case 's':
            operation = OP_SET;
            break;
        case 'a':
            operation = OP_ADD;
            break;
        case 'r':
            operation = OP_REM;
            break;
        default:
            G_fatal_error(_("Unknown operation '%s'"), opt.op->answer);
            break;
        }
    }

    if (strcmp(opt.format->answer, "json") == 0)
        format = JSON;
    else
        format = PLAIN;
    if (format == JSON) {
        fatal_error_option_value_excludes_option(
            opt.format, opt.fsep, _("Separator is part of the format."));
    }

    /* the field separator */
    if (opt.fsep->answer) {
        fsep = G_option_to_separator(opt.fsep);
    }
    else {
        /* A different separator is needed to for each format and output. */
        if (format == PLAIN) {
            fsep = G_store(" ");
        }
        else
            fsep = NULL; /* Something like a separator is part of the format. */
    }

    /* list available mapsets */
    if (opt.list->answer) {
        if (opt.print->answer)
            G_warning(_("Flag -%c ignored"), opt.print->key);
        if (opt.dialog->answer)
            G_warning(_("Flag -%c ignored"), opt.dialog->key);
        if (opt.mapset->answer)
            G_warning(_("Option <%s> ignored"), opt.mapset->key);
        mapset_name = get_available_mapsets(&nmapsets);
        if (format == JSON) {
            list_avaliable_mapsets_json((const char **)mapset_name, nmapsets);
        }
        else {
            list_available_mapsets((const char **)mapset_name, nmapsets, fsep);
        }

        exit(EXIT_SUCCESS);
    }

    if (opt.print->answer) {
        if (opt.dialog->answer)
            G_warning(_("Flag -%c ignored"), opt.dialog->key);
        if (opt.mapset->answer)
            G_warning(_("Option <%s> ignored"), opt.mapset->key);
        if (format == JSON) {
            list_accessible_mapsets_json();
        }
        else {
            list_accessible_mapsets(fsep);
        }

        exit(EXIT_SUCCESS);
    }

    /* show GUI dialog */
    if (opt.dialog->answer) {
        if (opt.mapset->answer)
            G_warning(_("Option <%s> ignored"), opt.mapset->key);
        sprintf(path_buf, "%s/gui/wxpython/modules/mapsets_picker.py",
                G_gisbase());
        G_spawn(getenv("GRASS_PYTHON"), "mapsets_picker.py", path_buf, NULL);
        exit(EXIT_SUCCESS);
    }

    cur_mapset = G_mapset();

    /* modify search path */
    if (operation == OP_SET) {
        int cur_found;

        cur_found = FALSE;
        for (ptr = opt.mapset->answers; *ptr != NULL; ptr++) {
            mapset = substitute_mapset(*ptr);
            if (G_mapset_permissions(mapset) < 0)
                G_fatal_error(_("Mapset <%s> not found"), mapset);
            if (strcmp(mapset, cur_mapset) == 0)
                cur_found = TRUE;
            nchoices++;
            append_mapset(&path, mapset);
        }
        if (!cur_found)
            G_warning(_("Current mapset (<%s>) must always included in the "
                        "search path"),
                      cur_mapset);
    }
    else if (operation == OP_ADD) {
        /* add to existing search path */
        const char *oldname;

        if (path) {
            G_free(path);
            path = NULL;
        }

        /* read existing mapsets from SEARCH_PATH */
        for (n = 0; (oldname = G_get_mapset_name(n)); n++)
            append_mapset(&path, oldname);

        /* fetch and add new mapsets from param list */
        for (ptr = opt.mapset->answers; *ptr != NULL; ptr++) {

            mapset = substitute_mapset(*ptr);

            if (G_is_mapset_in_search_path(mapset)) {
                G_message(_("Mapset <%s> already in the path"), mapset);
                continue;
            }

            if (G_mapset_permissions(mapset) < 0)
                G_fatal_error(_("Mapset <%s> not found"), mapset);
            else
                G_verbose_message(_("Mapset <%s> added to search path"),
                                  mapset);

            nchoices++;
            append_mapset(&path, mapset);
        }
    }
    else if (operation == OP_REM) {
        /* remove from existing search path */
        const char *oldname;
        int found;

        if (path) {
            G_free(path);
            path = NULL;
        }

        /* read existing mapsets from SEARCH_PATH */
        for (n = 0; (oldname = G_get_mapset_name(n)); n++) {
            found = FALSE;

            for (ptr = opt.mapset->answers; *ptr && !found; ptr++)
                mapset = substitute_mapset(*ptr);

            if (strcmp(oldname, mapset) == 0)
                found = TRUE;

            if (found) {
                if (strcmp(oldname, cur_mapset) == 0)
                    G_warning(_("Current mapset (<%s>) must always included in "
                                "the search path"),
                              cur_mapset);
                else
                    G_verbose_message(_("Mapset <%s> removed from search path"),
                                      oldname);
                continue;
            }

            nchoices++;
            append_mapset(&path, oldname);
        }
    }
    /* stuffem sets nchoices */

    if (nchoices == 0) {
        G_important_message(_("Search path not modified"));
        if (path)
            G_free(path);

        if (nmapsets) {
            for (nmapsets--; nmapsets >= 0; nmapsets--)
                G_free(mapset_name[nmapsets]);
            G_free(mapset_name);
        }

        exit(EXIT_SUCCESS);
    }

    /* note I'm assuming that mapsets cannot have ' 's in them */
    tokens = G_tokenize(path, " ");

    fp = G_fopen_new("", "SEARCH_PATH");
    if (!fp)
        G_fatal_error(_("Unable to open SEARCH_PATH for write"));

    /*
     * make sure current mapset is specified in the list if not add it
     * to the head of the list
     */

    skip = 0;
    for (n = 0; n < nchoices; n++)
        if (strcmp(cur_mapset, tokens[n]) == 0) {
            skip = 1;
            break;
        }
    if (!skip) {
        fprintf(fp, "%s\n", cur_mapset);
    }

    /*
     * output the list, removing duplicates
     */

    no_tokens = G_number_of_tokens(tokens);
    for (n = 0; n < no_tokens; n++) {
        skip = 0;
        for (i = n; i < no_tokens; i++) {
            if (i != n) {
                if (strcmp(tokens[i], tokens[n]) == 0)
                    skip = 1;
            }
        }

        if (!skip)
            fprintf(fp, "%s\n", tokens[n]);
    }

    fclose(fp);
    G_free_tokens(tokens);

    if (path)
        G_free(path);

    if (nmapsets) {
        for (nmapsets--; nmapsets >= 0; nmapsets--)
            G_free(mapset_name[nmapsets]);
        G_free(mapset_name);
    }

    exit(EXIT_SUCCESS);
}

void append_mapset(char **path, const char *mapset)
{
    int len = (*path == NULL ? 0 : strlen(*path));

    *path = (char *)G_realloc(*path, len + strlen(mapset) + 2);
    if (!len) {
        *path[0] = '\0';
        strcat(*path, mapset);
    }
    else {
        strcat(*path, " ");
        strcat(*path, mapset);
    }
    return;
}

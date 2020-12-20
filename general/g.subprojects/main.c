
/****************************************************************************
 *
 * MODULE:       g.subprojects
 * AUTHOR(S):    Michael Shapiro (CERL),
 *               Greg Koerper (ManTech Environmental Technology) (original contributors), 
 *               Glynn Clements <glynn gclements.plus.com>
 *               Hamish Bowman <hamish_b yahoo.com>, 
 *               Markus Neteler <neteler itc.it>, 
 *               Moritz Lennert <mlennert club.worldonline.be>,
 *               Martin Landa <landa.martin gmail.com>,
 *               Huidae Cho <grass4u gmail.com>
 * PURPOSE:      set current subproject path
 * COPYRIGHT:    (C) 1994-2009, 2012 by the GRASS Development Team
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

static void append_subproject(char **, const char *);

int main(int argc, char *argv[])
{
    int n, i;
    int skip;
    const char *cur_subproject, *subproject;
    char **ptr;
    char **tokens;
    int no_tokens;
    FILE *fp;
    char path_buf[GPATH_MAX];
    char *path, *fs;
    int operation, nchoices;
    
    char **subproject_name;
    int nsubprojects;
    
    struct GModule *module;    
    struct _opt {
        struct Option *subproject, *op, *fs;
        struct Flag *print, *list, *dialog;
    } opt;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("general"));
    G_add_keyword(_("settings"));
    G_add_keyword(_("search path"));
    module->label = _("Modifies/prints the user's current subproject search path.");
    module->description = _("Affects the user's access to data existing "
                            "under the other subprojects in the current project.");

    opt.subproject = G_define_standard_option(G_OPT_M_MAPSET);
    opt.subproject->required = YES;
    opt.subproject->multiple = YES;
    opt.subproject->description = _("Name(s) of existing subproject(s) to add/remove or set");
    
    opt.op = G_define_option();
    opt.op->key = "operation";
    opt.op->type = TYPE_STRING;
    opt.op->required = YES;
    opt.op->multiple = NO;
    opt.op->options = "set,add,remove";
    opt.op->description = _("Operation to be performed");
    opt.op->answer = "add";
    
    opt.fs = G_define_standard_option(G_OPT_F_SEP);
    opt.fs->label = _("Field separator for printing (-l and -p flags)");
    opt.fs->answer = "space";
    opt.fs->guisection = _("Print");
    
    opt.list = G_define_flag();
    opt.list->key = 'l';
    opt.list->description = _("List all available subprojects in alphabetical order");
    opt.list->guisection = _("Print");
    opt.list->suppress_required = YES;

    opt.print = G_define_flag();
    opt.print->key = 'p';
    opt.print->description = _("Print subprojects in current search path");
    opt.print->guisection = _("Print");
    opt.print->suppress_required = YES;

    opt.dialog = G_define_flag();
    opt.dialog->key = 's';
    opt.dialog->description = _("Launch subproject selection GUI dialog");
    opt.dialog->suppress_required = YES;
    
    path = NULL;
    subproject_name = NULL;
    nsubprojects = nchoices = 0;

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    operation = OP_UKN;
    if (opt.subproject->answer && opt.op->answer) {
        switch(opt.op->answer[0]) {
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
    
    fs = G_option_to_separator(opt.fs);

    /* list available subprojects */
    if (opt.list->answer) {
        if (opt.print->answer)
            G_warning(_("Flag -%c ignored"), opt.print->key);
        if (opt.dialog->answer)
            G_warning(_("Flag -%c ignored"), opt.dialog->key);
        if (opt.subproject->answer)
            G_warning(_("Option <%s> ignored"), opt.subproject->key);
        subproject_name = get_available_subprojects(&nsubprojects);
        list_available_subprojects((const char **)subproject_name, nsubprojects, fs);
        exit(EXIT_SUCCESS);
    }

    if (opt.print->answer) {
        if (opt.dialog->answer)
            G_warning(_("Flag -%c ignored"), opt.dialog->key);
        if (opt.subproject->answer)
            G_warning(_("Option <%s> ignored"), opt.subproject->key);
        list_accessible_subprojects(fs);
        exit(EXIT_SUCCESS);
    }
    
    /* show GUI dialog */
    if (opt.dialog->answer) {
        if (opt.subproject->answer)
            G_warning(_("Option <%s> ignored"), opt.subproject->key);
        sprintf(path_buf, "%s/gui/wxpython/modules/subprojects_picker.py", G_gisbase());
        G_spawn(getenv("GRASS_PYTHON"), "subprojects_picker.py", path_buf, NULL);
        exit(EXIT_SUCCESS);
    }

    cur_subproject = G_subproject();
    
    /* modify search path */
    if (operation == OP_SET) {
        int cur_found;
        
        cur_found = FALSE;
        for (ptr = opt.subproject->answers; *ptr != NULL; ptr++) {
            subproject = substitute_subproject(*ptr);
            if (G_subproject_permissions(subproject) < 0)
                G_fatal_error(_("Subproject <%s> not found"), subproject);
            if (strcmp(subproject, cur_subproject) == 0)
                cur_found = TRUE;
            nchoices++;
            append_subproject(&path, subproject);
        }
        if (!cur_found)
            G_warning(_("Current subproject (<%s>) must always included in the search path"),
                      cur_subproject);
    }
    else if (operation == OP_ADD) {
        /* add to existing search path */
        const char *oldname;
        
        if (path) {
            G_free(path);
            path = NULL;
        }

        /* read existing subprojects from SEARCH_PATH */
        for (n = 0; (oldname = G_get_subproject_name(n)); n++)
            append_subproject(&path, oldname);

        /* fetch and add new subprojects from param list */
        for (ptr = opt.subproject->answers; *ptr != NULL; ptr++) {

            subproject = substitute_subproject(*ptr);

            if (G_is_subproject_in_search_path(subproject)) {
                G_message(_("Subproject <%s> already in the path"), subproject);
                continue;
            }
            
            if (G_subproject_permissions(subproject) < 0)
                G_fatal_error(_("Subproject <%s> not found"), subproject);
            else
                G_verbose_message(_("Subproject <%s> added to search path"),
                                  subproject);

            nchoices++;
            append_subproject(&path, subproject);
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
        
        /* read existing subprojects from SEARCH_PATH */
        for (n = 0; (oldname = G_get_subproject_name(n)); n++) {
            found = FALSE;
            
            for (ptr = opt.subproject->answers; *ptr && !found; ptr++)

                subproject = substitute_subproject(*ptr);
            
                if (strcmp(oldname, subproject) == 0)
                    found = TRUE;
            
                if (found) {
                    if (strcmp(oldname, cur_subproject) == 0)
                        G_warning(_("Current subproject (<%s>) must always included in the search path"),
                                  cur_subproject);
                    else
                        G_verbose_message(_("Subproject <%s> removed from search path"),
                                          oldname);
                    continue;
                }
                
                nchoices++;
                append_subproject(&path, oldname);
        }
    }
    /* stuffem sets nchoices */

    if (nchoices == 0) {
        G_important_message(_("Search path not modified"));
        if (path)
            G_free(path);
        
        if (nsubprojects) {
            for(nsubprojects--; nsubprojects >= 0; nsubprojects--)
                G_free(subproject_name[nsubprojects]);
            G_free(subproject_name);
        }
        
        exit(EXIT_SUCCESS);
    }
    
    /* note I'm assuming that subprojects cannot have ' 's in them */
    tokens = G_tokenize(path, " ");

    fp = G_fopen_new("", "SEARCH_PATH");
    if (!fp)
        G_fatal_error(_("Unable to open SEARCH_PATH for write"));

    /*
     * make sure current subproject is specified in the list if not add it
     * to the head of the list
     */
    
    skip = 0;
    for (n = 0; n < nchoices; n++)
        if (strcmp(cur_subproject, tokens[n]) == 0) {
            skip = 1;
            break;
        }
    if (!skip) {
        fprintf(fp, "%s\n", cur_subproject);
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

    if (nsubprojects) {
        for(nsubprojects--; nsubprojects >= 0; nsubprojects--)
            G_free(subproject_name[nsubprojects]);
        G_free(subproject_name);
    }

    exit(EXIT_SUCCESS);
}

void append_subproject(char **path, const char *subproject)
{
    int len = (*path == NULL ? 0 : strlen(*path));

    *path = (char *)G_realloc(*path, len + strlen(subproject) + 2);
    if (!len)
        *path[0] = '\0';
    strcat(*path, subproject);
    strcat(*path, " ");
    return;
}

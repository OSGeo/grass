
/****************************************************************************
 *
 * MODULE:       g.gui
 *
 * AUTHOR(S):    Martin Landa <landa.martin gmail.com>
 *		 Hamish Bowman <hamish_b yahoo com> (fine tuning)
 *
 * PURPOSE:      Start GRASS GUI from command line.
 *
 * COPYRIGHT:    (C) 2008-2015 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/spawn.h>

int main(int argc, char *argv[])
{
    struct Option *type, *rc_file;
    struct Flag *update_ui, *fglaunch, *nolaunch;
    struct GModule *module;
    const char *gui_type_env;
    char progname[GPATH_MAX];
    char *desc;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("general"));
    G_add_keyword(_("GUI"));
    G_add_keyword(_("user interface"));
        
    module->label =
	_("Launches a GRASS graphical user interface (GUI) session.");
    module->description = _("Optionally updates default user interface settings.");

    type = G_define_option();
    type->key = "ui";
    type->type = TYPE_STRING;
    type->description = _("User interface");
    desc = NULL;
    G_asprintf(&desc,
               "wxpython;%s;text;%s;gtext;%s;",
               _("wxPython based GUI (wxGUI)"),
               _("command line interface only"),
               _("command line interface with GUI startup screen"));
    type->descriptions = desc;
    type->options = "wxpython,text,gtext";
    type->answer = "wxpython";
    type->guisection = _("Type");
    
    rc_file = G_define_standard_option(G_OPT_F_INPUT);
    rc_file->key = "workspace";
    rc_file->required = NO;
    rc_file->key_desc = "name.gxw";
    rc_file->label = _("Name of workspace file to load on start-up");
    rc_file->description = _("This is valid only for wxGUI (wxpython)");

    fglaunch = G_define_flag();
    fglaunch->key = 'f';
    fglaunch->label = _("Start GUI in the foreground");
    fglaunch->description = _("By default the GUI starts in the background"
        " and control is immediately returned to the caller."
        " When GUI runs in foregreound, it blocks the command line");

    update_ui = G_define_flag();
    update_ui->key = 'd';
    update_ui->description = _("Update default user interface settings");
    update_ui->guisection = _("Default");

    nolaunch = G_define_flag();
    nolaunch->key = 'n';
    nolaunch->description =
	_("Do not launch GUI after updating the default user interface settings");
    nolaunch->guisection = _("Default");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    gui_type_env = G_getenv_nofatal("GUI");
    G_debug(1, "GUI: %s", gui_type_env ? gui_type_env : "unset");
    if (update_ui->answer) {
        if (!gui_type_env || strcmp(type->answer, gui_type_env)) {
            G_setenv("GUI", type->answer);
            G_message(_("<%s> is now the default GUI"), type->answer);
        }
    }

    if(strcmp(type->answer, "wxpython") != 0 || nolaunch->answer) {
        if (!update_ui->answer)
            G_warning(_("Nothing to do. For setting up <%s> as default UI use -%c flag."),
                      type->answer, update_ui->key);
	exit(EXIT_SUCCESS);
    }

    sprintf(progname, "%s/gui/wxpython/wxgui.py", G_gisbase());
    if (access(progname, F_OK) == -1)
        G_fatal_error(_("Your installation doesn't include GUI, exiting."));
                      
    if (fglaunch->answer) {
        G_message(_("Launching <%s> GUI, please wait..."), type->answer);
        if (rc_file->answer) {
            G_spawn_ex(getenv("GRASS_PYTHON"), getenv("GRASS_PYTHON"), progname,
                       "--workspace", rc_file->answer, NULL);
        }
        else {
            G_spawn_ex(getenv("GRASS_PYTHON"), getenv("GRASS_PYTHON"), progname,
                       NULL);
        }
    }
    else {
        G_message(_("Launching <%s> GUI in the background, please wait..."), type->answer);
        if (rc_file->answer) {
            G_spawn_ex(getenv("GRASS_PYTHON"), getenv("GRASS_PYTHON"), progname,
                       "--workspace", rc_file->answer, SF_BACKGROUND, NULL);
        }
        else {
            G_spawn_ex(getenv("GRASS_PYTHON"), getenv("GRASS_PYTHON"), progname,
                       SF_BACKGROUND, NULL);
        }
        /* stop the impatient from starting it again
           before the splash screen comes up */
        G_sleep(3);
    }

    exit(EXIT_SUCCESS);
}


/****************************************************************************
 *
 * MODULE:       g.gui
 *
 * AUTHOR(S):    Martin Landa <landa.martin gmail.com>
 *		 Hamish Bowman <hamish_b yahoo com> (fine tuning)
 *
 * PURPOSE:      Start GRASS GUI from command line.
 *
 * COPYRIGHT:    (C) 2008, 2010-2011 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/spawn.h>

int main(int argc, char *argv[])
{
    struct Option *type, *rc_file;
    struct Flag *update, *nolaunch;
    struct GModule *module;
    const char *gui_type_env;
    char progname[GPATH_MAX];
    char *desc;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("general"));
    G_add_keyword(_("gui"));
    module->description =
	_("Launches a GRASS graphical user interface (GUI) session.");

    type = G_define_option();
    type->key = "gui";
    type->type = TYPE_STRING;
    type->label = _("GUI type");
    type->description = _("Default value: GRASS_GUI if defined otherwise wxpython");
    desc = NULL;
    G_asprintf(&desc,
	        "wxpython;%s;text;%s",
	        _("wxPython based GUI (wxGUI)"),
	        _("command line interface only"));
    type->descriptions = desc;
    type->options = "wxpython,text";
    type->guisection = _("Type");
    
    rc_file = G_define_standard_option(G_OPT_F_INPUT);
    rc_file->key = "workspace";
    rc_file->required = NO;
    rc_file->key_desc = "name.gxw";
    rc_file->description = _("Name of workspace file to load on start-up (valid only for wxGUI)");

    update = G_define_flag();
    update->key = 'u';
    update->description = _("Update default GUI setting");
    update->guisection = _("Default");

    nolaunch = G_define_flag();
    nolaunch->key = 'n';
    nolaunch->description =
	_("Do not launch GUI after updating the default GUI setting");
    nolaunch->guisection = _("Default");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);


    if (type->answer && strcmp(type->answer, "text") == 0 &&
	!nolaunch->answer)
	nolaunch->answer = TRUE;
    
    if (nolaunch->answer && !update->answer)
	update->answer = TRUE;
    
    gui_type_env = G__getenv("GUI");

    if (!type->answer) {
	if (gui_type_env && strcmp(gui_type_env, "text")) {
	    type->answer = G_store(gui_type_env);
	}
	else {
	    type->answer = "wxpython";
	}
    }

    if (((gui_type_env && update->answer) &&
	 strcmp(gui_type_env, type->answer) != 0) || !gui_type_env) {
	G_setenv("GUI", type->answer);
	G_message(_("<%s> is now the default GUI"), type->answer);
    }
    else {
	if(update->answer)
	    if(gui_type_env) {
		G_debug(1, "No change: old gui_type_env=[%s], new type->ans=[%s]",
			gui_type_env, type->answer);
	    }
    }

    if(nolaunch->answer)
	exit(EXIT_SUCCESS);


    G_message(_("Launching <%s> GUI in the background, please wait..."), type->answer);

    if (strcmp(type->answer, "wxpython") == 0) {
	sprintf(progname, "%s/etc/gui/wxpython/wxgui.py", G_gisbase());
	if (rc_file->answer) {
	    G_spawn_ex(getenv("GRASS_PYTHON"), getenv("GRASS_PYTHON"), progname,
		    "--workspace", rc_file->answer, SF_BACKGROUND, NULL);
	}
	else {
	    G_spawn_ex(getenv("GRASS_PYTHON"), getenv("GRASS_PYTHON"), progname,
		    SF_BACKGROUND, NULL);
	}
    }

    /* stop the impatient from starting it again
        before the splash screen comes up */
    G_sleep(3);

    exit(EXIT_SUCCESS);
}

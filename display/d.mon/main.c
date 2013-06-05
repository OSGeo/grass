
/****************************************************************************
 *
 * MODULE:       d.mon
 * AUTHOR(S):    Martin Landa <landa.martin gmail.com>
 * PURPOSE:      Controls graphics monitors for CLI
 * COPYRIGHT:    (C) 2011-2012 by Martin Landa, and the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2). Read the file COPYING that
 *               comes with GRASS for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/glocale.h>

#include "proto.h"

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct Option *start_opt, *select_opt, *stop_opt, *output_opt,
      *width_opt, *height_opt, *bgcolor_opt;
    struct Flag *list_flag, *selected_flag, *select_flag, *release_flag, 
	*cmd_flag, *truecolor_flag;
    
    int nopts, ret;
    const char *mon;
    
    G_gisinit(argv[0]);
    
    module = G_define_module();
    G_add_keyword(_("display"));
    G_add_keyword(_("graphics"));
    G_add_keyword(_("monitors"));
    G_add_keyword(_("drivers"));
    module->description = _("Controls graphics display monitors from the command line.");
    
    start_opt = G_define_option();
    start_opt->key = "start";
    start_opt->type = TYPE_STRING;
    start_opt->description = _("Name of monitor to start");
    start_opt->options = "wx0,wx1,wx2,wx3,wx4,wx5,wx6,wx7,png,ps,html,cairo";
    start_opt->guisection = _("Manage");
    
    stop_opt = G_define_option();
    stop_opt->key = "stop";
    stop_opt->type = TYPE_STRING;
    stop_opt->description = _("Name of monitor to stop");
    stop_opt->options = "wx0,wx1,wx2,wx3,wx4,wx5,wx6,wx7,png,ps,html,cairo";
    stop_opt->guisection = _("Manage");

    select_opt = G_define_option();
    select_opt->key = "select";
    select_opt->type = TYPE_STRING;
    select_opt->description = _("Name of monitor to select");
    select_opt->options = "wx0,wx1,wx2,wx3,wx4,wx5,wx6,wx7,png,ps,html,cairo";
    select_opt->guisection = _("Manage");

    width_opt = G_define_option();
    width_opt->key = "width";
    width_opt->description = _("Width for display monitor if not set by GRASS_WIDTH");
    width_opt->type = TYPE_INTEGER;
    width_opt->key_desc = "value";
    width_opt->guisection = _("Settings");

    height_opt = G_define_option();
    height_opt->key = "height";
    height_opt->description = _("Height for display monitor if not set by GRASS_HEIGHT");
    height_opt->type = TYPE_INTEGER;
    height_opt->key_desc = "value";
    height_opt->guisection = _("Settings");

    bgcolor_opt = G_define_standard_option(G_OPT_C_BG);
    bgcolor_opt->guisection = _("Settings");

    output_opt = G_define_standard_option(G_OPT_F_OUTPUT);
    output_opt->required = NO;
    output_opt->label = _("Name for output file (when starting new monitor)");
    output_opt->description = _("Ignored for 'wx' monitors");
    output_opt->guisection = _("Settings");
    
    list_flag = G_define_flag();
    list_flag->key = 'l';
    list_flag->description = _("List running monitors and exit");
    list_flag->guisection = _("Print");

    selected_flag = G_define_flag();
    selected_flag->key = 'p';
    selected_flag->description = _("Print name of currently selected monitor and exit");
    selected_flag->guisection = _("Print");

    cmd_flag = G_define_flag();
    cmd_flag->key = 'c';
    cmd_flag->description = _("Print commands for currently selected monitor and exit");
    cmd_flag->guisection = _("Print");

    select_flag = G_define_flag();
    select_flag->key = 's';
    select_flag->description = _("Do not automatically select when starting");
    select_flag->guisection = _("Manage");

    release_flag = G_define_flag();
    release_flag->key = 'r';
    release_flag->description = _("Release currently selected monitor and exit");
    release_flag->guisection = _("Manage");

    truecolor_flag = G_define_flag();
    truecolor_flag->key = 't';
    truecolor_flag->description = _("Disable true colors");
    truecolor_flag->guisection = _("Manage");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);
    
    if (selected_flag->answer || release_flag->answer || cmd_flag->answer) {
	if (list_flag->answer)
	    G_warning(_("Flag -%c ignored"), list_flag->key);
	mon = G__getenv("MONITOR");
	if (mon) {
	    if (selected_flag->answer) {
		G_verbose_message(_("Currently selected monitor:"));
		fprintf(stdout, "%s\n", mon);
	    }
	    else if (cmd_flag->answer) {
		G_message(_("List of commands for monitor <%s>:"), mon);
		list_cmd(mon, stdout);
	    }
	    else if (mon) { /* release */
		G_unsetenv("MONITOR");
		G_verbose_message(_("Monitor <%s> released"), mon); 
	    }
	}
	else
	    G_important_message(_("No monitor selected"));
	
	exit(EXIT_SUCCESS);
    }

    if (list_flag->answer) {
	print_list(stdout);
	exit(EXIT_SUCCESS);
    }
	
    nopts = 0;
    if (start_opt->answer)
	nopts++;
    if (stop_opt->answer)
	nopts++;
    if (select_opt->answer)
	nopts++;

    if (nopts != 1)
	G_fatal_error(_("Either <%s>, <%s> or <%s> must be given"),
		      start_opt->key, stop_opt->key, select_opt->key);
    
    if (output_opt->answer &&
	(!start_opt->answer || strncmp(start_opt->answer, "wx", 2) == 0))
	G_warning(_("Option <%s> ignored"), output_opt->key);
    
    if (start_opt->answer)
	ret = start_mon(start_opt->answer, output_opt->answer, !select_flag->answer,
			width_opt->answer, height_opt->answer, bgcolor_opt->answer,
			!truecolor_flag->answer);
    
    if (stop_opt->answer)
	ret = stop_mon(stop_opt->answer);
    
    if (select_opt->answer)
	ret = select_mon(select_opt->answer);
    
    if (ret != 0)
	exit(EXIT_FAILURE);
    
    exit(EXIT_SUCCESS);
}

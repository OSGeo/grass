/*
 ****************************************************************************
 *
 * MODULE:       d.erase
 * AUTHOR(S):    James Westervelt - USA CERL
 * PURPOSE:      Erase the current display frame with user defined color.
 * COPYRIGHT:    (C) 2000, 2011 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <grass/gis.h>
#include <grass/display.h>
#include <grass/glocale.h>

int main(int argc, char *argv[])
{
    struct Option *color;
    struct Flag *eraseframe;
    struct GModule *module;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("display"));
    G_add_keyword(_("graphics"));
    G_add_keyword(_("monitors"));
    module->description =
	_("Erases the contents of the active graphics display frame with user defined color.");

    color = G_define_standard_option(G_OPT_C);
    color->key = "bgcolor";
    color->label = _("Background color");
    color->answer = DEFAULT_BG_COLOR;
    
    eraseframe = G_define_flag();
    eraseframe->key = 'f';
    eraseframe->description = _("Remove all frames and erase the screen");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    D_open_driver();
    
    D_setup_unity(0);

    D_erase(color->answer);

    if (eraseframe->answer)
	D__erase();

    D_save_command(NULL);
    D_close_driver();

    exit(EXIT_SUCCESS);
}

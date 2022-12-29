
/****************************************************************************
 *
 * MODULE:       d.fontlist
 * AUTHOR(S):    James Westervelt (CERL) (original contributor)
 *               Markus Neteler <neteler itc.it>,
 *               Bernhard Reiter <bernhard intevation.de>, 
 *               Huidae Cho <grass4u gmail.com>, 
 *               Eric G. Miller <egm2 jps.net>, 
 *               Glynn Clements <glynn gclements.plus.com>, 
 *               Jan-Oliver Wagner <jan intevation.de>
 * PURPOSE:      user selection of font for graphics monitor text
 * COPYRIGHT:    (C) 1999-2008 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 * Implementation:
 *  d.fontlist gets the list via D_font_list(), which calls COM_Font_list(),
 *  which first reads the fonts from the file specified by $GRASS_FONT_CAP
 *  (falling back to $GISBASE/etc/fontcap), then adds any fonts obtained by
 *  the driver's Font_list method if provided (currently, only the cairo
 *  driver implements this method).
 * 
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/display.h>
#include <grass/glocale.h>

int main(int argc, char **argv)
{
    struct GModule *module;
    struct Flag *flagl, *flagL;
    char **list;
    int count;
    int i;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("display"));
    G_add_keyword(_("settings"));
    module->description = _("Lists the available fonts.");

    flagl = G_define_flag();
    flagl->key = 'l';
    flagl->description = _("List fonts (default; provided for compatibility with d.font)");

    flagL = G_define_flag();
    flagL->key = 'v';
    flagL->description = _("List fonts verbosely");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    D_open_driver();
    
    if (flagL->answer)
	D_font_info(&list, &count);
    else
	D_font_list(&list, &count);

    for (i = 0; i < count; i++)
	fprintf(stdout, "%s\n", list[i]);
    
    D_close_driver();

    exit(EXIT_SUCCESS);
}


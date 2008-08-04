/*
 ****************************************************************************
 *
 * MODULE:       d.title
 *
 * AUTHOR(S):    James Westervelt, US Army CERL
 *
 * PURPOSE:      print out title for raster on stdout
 *
 * COPYRIGHT:    (C) 2001 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *              License (>=v2). Read the file COPYING that comes with GRASS
 *              for details.
 *
 *****************************************************************************/

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <grass/display.h>
#include <grass/raster.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#define MAIN
#include "options.h"
#include "local_proto.h"

int main(int argc, char **argv)
{
    char *mapset;
    struct Cell_head window;
    struct Categories cats;
    struct GModule *module;
    struct Option *opt1, *opt2, *opt3;
    struct Flag *fancy_mode, *simple_mode, *draw;
    char *tmpfile;
    char command[GPATH_MAX + 12];
    FILE *fp;

    /* Initialize the GIS calls */
    G_gisinit(argv[0]);

    module = G_define_module();
    module->keywords = _("display");
    module->description =
	_("Create a TITLE for a raster map in a form suitable "
	  "for display with d.text.");

    opt1 = G_define_standard_option(G_OPT_R_MAP);

    opt2 = G_define_option();
    opt2->key = "color";
    opt2->type = TYPE_STRING;
    opt2->answer = DEFAULT_FG_COLOR;
    opt2->required = NO;
    opt2->options = D_color_list();
    opt2->description = _("Sets the text color");

    opt3 = G_define_option();
    opt3->key = "size";
    opt3->type = TYPE_DOUBLE;
    opt3->answer = "4.0";
    opt3->options = "0-100";
    opt3->description =
	_("Sets the text size as percentage of the frame's height");

    draw = G_define_flag();
    draw->key = 'd';
    draw->description = _("Draw title on current display");

    fancy_mode = G_define_flag();
    fancy_mode->key = 'f';
    fancy_mode->description = _("Do a fancier title");

    /* currently just title, but it doesn't have to be /that/ simple */
    simple_mode = G_define_flag();
    simple_mode->key = 's';
    simple_mode->description = _("Do a simple title");


    /* Check command line */
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);


    strcpy(map_name, opt1->answer);

    strcpy(color, opt2->answer);

    if (opt3->answer != NULL)
	sscanf(opt3->answer, "%f", &size);

    type = fancy_mode->answer ? FANCY : NORMAL;

    if (fancy_mode->answer && simple_mode->answer)
	G_fatal_error(_("Title can be fancy or simple, not both"));

    if (!strlen(map_name))
	G_fatal_error(_("No map name given"));

    /* Make sure map is available */
    mapset = G_find_cell(map_name, "");
    if (mapset == NULL)
	G_fatal_error(_("Raster map <%s> not found"), map_name);

    if (G_get_cellhd(map_name, mapset, &window) == -1)
	G_fatal_error(_("Unable to read header of raster map <%s@%s>"),
		      map_name, mapset);

    if (G_read_cats(map_name, mapset, &cats) == -1)
	G_fatal_error(_("Unable to read category file of raster map <%s@%s>"),
		      map_name, mapset);


    if (draw->answer) {
	tmpfile = G_convert_dirseps_to_host(G_tempfile());
	if (!(fp = fopen(tmpfile, "w")))
	    G_fatal_error(_("Unable to open temporary file <%s>"), tmpfile);
    }
    else
	fp = stdout;


    if (type == NORMAL)
	normal(mapset, &window, &cats, simple_mode->answer, fp);
    else
	fancy(mapset, &window, &cats, fp);


    if (draw->answer) {
	fclose(fp);
	sprintf(command, "d.text < \"%s\"", tmpfile);
	G_debug(3, "cmd = [%s]", command);
	G_system(command);
	unlink(tmpfile);
	/* note a tmp file will remain, created by d.text so it can survive d.redraw */
    }

    exit(EXIT_SUCCESS);
}

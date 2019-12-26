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
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/spawn.h>
#include <grass/glocale.h>

#include "options.h"
#include "local_proto.h"

const char *map_name;
const char *color;
float size;
int type;

int main(int argc, char **argv)
{
    struct Cell_head window;
    struct Categories cats;
    struct GModule *module;
    struct Option *opt1, *opt2, *opt3;
    struct Flag *fancy_mode, *simple_mode, *draw;
    char *tmpfile;
    FILE *fp;

    /* Initialize the GIS calls */
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("display"));
    G_add_keyword(_("cartography"));
    module->description =
	_("Create a TITLE for a raster map in a form suitable "
	  "for display with d.text.");

    opt1 = G_define_standard_option(G_OPT_R_MAP);

    opt2 = G_define_option();
    opt2->key = "color";
    opt2->type = TYPE_STRING;
    opt2->answer = DEFAULT_FG_COLOR;
    opt2->required = NO;
    opt2->gisprompt = "old_color,color,color";
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


    map_name = opt1->answer;

    color = opt2->answer;

    if (opt3->answer != NULL)
	sscanf(opt3->answer, "%f", &size);

    type = fancy_mode->answer ? FANCY : NORMAL;

    if (fancy_mode->answer && simple_mode->answer)
	G_fatal_error(_("Title can be fancy or simple, not both"));

    if (!strlen(map_name))
	G_fatal_error(_("No map name given"));

    Rast_get_cellhd(map_name, "", &window);

    if (Rast_read_cats(map_name, "", &cats) == -1)
	G_fatal_error(_("Unable to read category file of raster map <%s>"),
		      map_name);


    if (draw->answer) {
	tmpfile = G_convert_dirseps_to_host(G_tempfile());
	if (!(fp = fopen(tmpfile, "w")))
	    G_fatal_error(_("Unable to open temporary file <%s>"), tmpfile);
    }
    else
	fp = stdout;


    if (type == NORMAL)
	normal(&window, &cats, simple_mode->answer, fp);
    else
	fancy(&window, &cats, fp);


    if (draw->answer) {
	char inarg[GPATH_MAX];
	fclose(fp);
	sprintf(inarg, "input=%s", tmpfile);
	/* note this tmp file will remain so it can survive d.redraw */
	G_spawn("d.text", "d.text", inarg, NULL);
    }

    exit(EXIT_SUCCESS);
}

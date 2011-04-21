
/****************************************************************************
 *
 * MODULE:       d.what.rast
 * AUTHOR(S):    Michael Shapiro (CERL) (original contributor)
 *               Markus Neteler <neteler itc.it>, 
 *               Andreas Lange <andreas.lange rhein-main.de>, 
 *               Bernhard Reiter <bernhard intevation.de>, 
 *               Huidae Cho <grass4u gmail.com>, 
 *               Eric G. Miller <egm2 jps.net>, 
 *               Glynn Clements <glynn gclements.plus.com>, 
 *               Hamish Bowman <hamish_b yahoo.com>
 * PURPOSE:      interactive query of cat/label of raster map in display
 * COPYRIGHT:    (C) 1999-2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include "what.h"
#include <grass/display.h>
#include <grass/glocale.h>
#include "local_proto.h"

struct Categories *cats;
int *fd;
char **rast;
int nrasts;
char **name;
char **mapset;

int main(int argc, char **argv)
{
    int i, j;
    int width, mwidth;
    struct Flag *once, *terse, *colrow;
    struct Option *opt1, *fs;
    struct GModule *module;

    /* Initialize the GIS calls */
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("display"));
    G_add_keyword(_("raster"));
    G_add_keyword(_("position"));
    G_add_keyword(_("querying"));
    module->description =
	_("Allows the user to interactively query the category contents "
	  "of multiple raster map layers at user specified locations "
	  "within the current geographic region.");


    rast = NULL;

    opt1 = G_define_option();
    opt1->key = "map";
    opt1->type = TYPE_STRING;
    opt1->required = YES;
    opt1->multiple = YES;
    opt1->gisprompt = "old,cell,raster";
    opt1->description = _("Name of existing raster map(s)");
    opt1->key_desc = "name";

    fs = G_define_option();
    fs->key = "fs";
    fs->type = TYPE_STRING;
    fs->required = NO;
    fs->answer = ":";
    fs->description = _("Field separator (terse mode only)");
    fs->key_desc = "character";

    once = G_define_flag();
    once->key = '1';
    once->description = _("Identify just one location");

    terse = G_define_flag();
    terse->key = 't';
    terse->description = _("Terse output. For parsing by programs");

    colrow = G_define_flag();
    colrow->key = 'c';
    colrow->description =
	_("Print out col/row for the entire map in grid resolution of the region");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);


    if (opt1->answers && opt1->answers[0])
	rast = opt1->answers;

    if (R_open_driver() != 0)
	G_fatal_error(_("No graphics device selected"));

    D_setup(0);

    if (rast) {
	for (i = 0; rast[i]; i++) ;
	nrasts = i;

	fd = (int *)G_malloc(nrasts * sizeof(int));
	name = (char **)G_malloc(nrasts * sizeof(char *));
	mapset = (char **)G_malloc(nrasts * sizeof(char *));
	cats =
	    (struct Categories *)G_malloc(nrasts * sizeof(struct Categories));

	width = mwidth = 0;
	for (i = 0; i < nrasts; i++) {
	    name[i] = (char *)G_malloc(GNAME_MAX);
	    mapset[i] = (char *)G_malloc(GMAPSET_MAX);

	    if ((fd[i] = opencell(rast[i], name[i], mapset[i])) < 0)
		G_fatal_error(_("Raster map <%s> not found"), rast[i]);

	    j = strlen(name[i]);
	    if (j > width)
		width = j;

	    j = strlen(mapset[i]);
	    if (j > mwidth)
		mwidth = j;

	    if (Rast_read_cats(name[i], mapset[i], &cats[i]) < 0)
		cats[i].ncats = -1;
	}
    }

    what(once->answer, terse->answer, colrow->answer, fs->answer, width,
	 mwidth);

    R_close_driver();
    exit(EXIT_SUCCESS);
}

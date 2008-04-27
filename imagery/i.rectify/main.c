/****************************************************************************
 *
 * MODULE:       i.rectify
 * AUTHOR(S):    William R. Enslin, Michigan State U. (original contributor)
 *               Luca Palmeri <palmeri ux1.unipd.it>
 *               Bill Hughes,
 *               Pierre de Mouveaux <pmx audiovu.com>,
 *               Bob Covill (original CMD version), 
 *               Markus Neteler <neteler itc.it>, 
 *               Bernhard Reiter <bernhard intevation.de>, 
 *               Glynn Clements <glynn gclements.plus.com>, 
 *               Hamish Bowman <hamish_nospam yahoo.com>
 * PURPOSE:      calculate a transformation matrix and then convert x,y cell 
 *               coordinates to standard map coordinates for each pixel in the 
 *               image (control points can come from i.points or i.vpoints)
 * COPYRIGHT:    (C) 2002-2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#define GLOBAL
#include <stdlib.h>
#include <string.h>
#include "global.h"
#include "crs.h"
#include <grass/glocale.h>

#define NFILES 15

void err_exit(char *, char *);

int main(int argc, char *argv[])
{
    char group[INAME_LEN], extension[INAME_LEN];
    char result[NFILES][15];
    int order;			/* ADDED WITH CRS MODIFICATIONS */
    int n, i, m, k;
    int got_file = 0;

    struct Option *grp, *val, *ifile, *ext;
    struct Flag *c, *a;
    struct GModule *module;

    struct Cell_head cellhd;

    setbuf(stdout, NULL);
    setbuf(stderr, NULL);

    G_gisinit(argv[0]);

    module = G_define_module();
    module->keywords = _("imagery");
    module->description =
	_("Rectifies an image by computing a coordinate "
	  "transformation for each pixel in the image based on the "
	  "control points");

    grp = G_define_option();
    grp->key = "group";
    grp->type = TYPE_STRING;
    grp->required = YES;
    grp->gisprompt = "old,group,group";
    grp->description = _("Name of imagery group");

    ifile = G_define_option();
    ifile->key = "input";
    ifile->type = TYPE_STRING;
    ifile->required = NO;
    ifile->multiple = YES;
    ifile->gisprompt = "old,cell,raster";
    ifile->description = _("Name of input raster map(s)");

    ext = G_define_option();
    ext->key = "extension";
    ext->type = TYPE_STRING;
    ext->required = YES;
    ext->multiple = NO;
    ext->description = _("Output file extension (inputfile(s) + extension)");

    val = G_define_option();
    val->key = "order";
    val->type = TYPE_INTEGER;
    val->required = YES;
    val->description = _("Rectification polynom order (1-3)");

    c = G_define_flag();
    c->key = 'c';
    c->description =
	_("Use curr. region settings in target location (def.=calculate smallest area)");

    a = G_define_flag();
    a->key = 'a';
    a->description = _("Rectify all images in group");



    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    G_strip(grp->answer);
    strcpy(group, grp->answer);
    strcpy(extension, ext->answer);
    order = atoi(val->answer);

    if (!ifile->answers)
	a->answer = 1;		/* force all */

    /* Find out how files on command line */
    if (!a->answer) {
	for (m = 0; ifile->answers[m]; m++) {
	    k = m;
	}
	k++;
    }

    if (order < 1 || order > MAXORDER)
	G_fatal_error(_("Invalid order (%d) please enter 1 to %d"), order,
		      MAXORDER);

    /* determine the number of files in this group */
    if (I_get_group_ref(group, &ref) <= 0)
	G_fatal_error(_("Group <%s> does not exist"), grp->answer);

    if (ref.nfiles <= 0) {
	G_important_message(_("Group <%s> contains no maps. Run i.group"), grp->answer);
	exit(EXIT_SUCCESS);
    }

    for (i = 0; i < NFILES; i++)
	result[i][0] = 0;

    ref_list = (int *)G_malloc(ref.nfiles * sizeof(int));
    new_name = (char **)G_malloc(ref.nfiles * sizeof(char *));

    if (a->answer) {
	for (n = 0; n < ref.nfiles; n++) {
	    ref_list[n] = -1;
	}
    }
    else {
	for (m = 0; m < k; m++) {
	    got_file = 0;
	    for (n = 0; n < ref.nfiles; n++) {
		if (strcmp(ifile->answers[m], ref.file[n].name) == 0) {
		    got_file = 1;
		    ref_list[n] = -1;
		    break;
		}
	    }
	    if (got_file == 0)
		err_exit(ifile->answers[m], group);
	}
    }

    /* read the control points for the group */
    get_control_points(group, order);

    /* get the target */
    get_target(group);


    if (c->answer) {
	/* Use current Region */
	G_get_window(&target_window);
    }
    else {
	/* Calculate smallest region */
	if (a->answer) {
	    if (G_get_cellhd(ref.file[0].name, ref.file[0].mapset, &cellhd) < 0)
		G_fatal_error(_("Unable to read header of raster map <%s>"), ref.file[0].name);
	}
	else {
	    if (G_get_cellhd(ifile->answers[0], ref.file[0].mapset, &cellhd) < 0)
		G_fatal_error(_("Unable to read header of raster map <%s>"), ifile->answers[0]);
	}
	georef_window(&cellhd, &target_window, order);
    }

    G_message( _("Using Region: N=%f S=%f, E=%f W=%f"), target_window.north,
	    target_window.south, target_window.east, target_window.west);

    exec_rectify(order, extension);

    exit(EXIT_SUCCESS);
}


void err_exit(char *file, char *grp)
{
    int n;

    fprintf(stderr, "Input file <%s> does not exist in group <%s>.\n Try:\n",
	    file, grp);

    for (n = 0; n < ref.nfiles; n++)
	fprintf(stderr, "%s\n", ref.file[n].name);

    G_fatal_error("Exit!");
}


/****************************************************************************
 *
 * MODULE:       d.paint.labels
 * AUTHOR(S):    Jim Westervelt (CERL) (original contributor)
 *               Radim Blazek <radim.blazek gmail.com>, 
 *               Stephan Holl <sholl gmx net>, 
 *               Glynn Clements <glynn gclements.plus.com>, 
 *               Hamish Bowman <hamish_b yahoo.com>, 
 *               Markus Neteler <neteler itc.it>
 * PURPOSE:      displays a paint label file in the active display frame
 * COPYRIGHT:    (C) 2003-2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#include <stdlib.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/display.h>
#include "local_proto.h"
#include <grass/glocale.h>

int main(int argc, char **argv)
{
    struct Cell_head window;
    char *label_name;
    const char *mapset;
    double minreg, maxreg, reg, dx, dy;
    FILE *infile;
    struct Option *opt1;
    struct Option *maxreg_opt, *minreg_opt;
    struct Flag *horiz_flag;
    struct GModule *module;

    /* Initialize the GIS calls */
    G_gisinit(argv[0]);

    /* Set description */
    module = G_define_module();
    G_add_keyword(_("display"));
    G_add_keyword(_("paint labels"));
    module->description =
	_("Displays text labels (created with v.label) "
	  "to the active frame on the graphics monitor.");

    horiz_flag = G_define_flag();
    horiz_flag->key = 'i';
    horiz_flag->description =
	_("Ignore rotation setting and draw horizontally");

    opt1 = G_define_option();
    opt1->key = "labels";
    opt1->type = TYPE_STRING;
    opt1->required = YES;
    opt1->gisprompt = "old,paint/labels,paint labels";
    opt1->description = _("Name of label file");

    minreg_opt = G_define_option();
    minreg_opt->key = "minreg";
    minreg_opt->type = TYPE_DOUBLE;
    minreg_opt->required = NO;
    minreg_opt->description =
	_("Minimum region size (diagonal) when labels are displayed");

    maxreg_opt = G_define_option();
    maxreg_opt->key = "maxreg";
    maxreg_opt->type = TYPE_DOUBLE;
    maxreg_opt->required = NO;
    maxreg_opt->description =
	_("Maximum region size (diagonal) when labels are displayed");

    /* Check command line */
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    /* Save map name */
    label_name = opt1->answer;

    /* Make sure map is available */
    mapset = G_find_file("paint/labels", label_name, "");
    if (mapset == NULL)
	G_fatal_error(_("Label file <%s> not found"), label_name);

    /* Read in the map window associated with window */
    G_get_window(&window);

    /* Check min/max region */
    dx = window.east - window.west;
    dy = window.north - window.south;
    reg = sqrt(dx * dx + dy * dy);
    if (minreg_opt->answer) {
	minreg = atof(minreg_opt->answer);
	if (reg < minreg) {
	    G_warning(_("Region size is lower than minreg, nothing displayed."));
	    D_close_driver();
	    exit(0);
	}
    }
    if (maxreg_opt->answer) {
	maxreg = atof(maxreg_opt->answer);
	if (reg > maxreg) {
	    G_warning(_("Region size is greater than maxreg, nothing displayed."));
	    D_close_driver();
	    exit(0);
	}
    }

    /* Open map is available */
    infile = G_fopen_old("paint/labels", label_name, mapset);
    if (infile == NULL)
	G_fatal_error(_("Unable to open label file <%s>"), label_name);

    D_open_driver();
    
    D_setup(0);

    /* Go draw the raster map */
    do_labels(infile, !horiz_flag->answer);

    D_save_command(G_recreate_command());
    D_close_driver();

    exit(EXIT_SUCCESS);
}

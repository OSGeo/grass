
/****************************************************************************
 *
 * MODULE:       i.segment
 * AUTHOR(S):    Markus Metz
 *               based on the the GSoC project by Eric Momsen <eric.momsen at gmail com>
 * PURPOSE:      Object recognition, segments an image group.
 * COPYRIGHT:    (C) 2012 by Eric Momsen, and the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2). Read the COPYING file that
 *               comes with GRASS for details.
 * 
 *
 *               NOTE: the word "segment" is already used by the
 *               Segmentation Library for the data files/tiling, so
 *               iseg (image segmentation) will be used to refer to
 *               the image segmentation.
 * 
 *****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "iseg.h"

int main(int argc, char *argv[])
{
    struct globals globals; /* input and output file descriptors, data structure, buffers */
    struct GModule *module;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("imagery"));
    G_add_keyword(_("segmentation"));
    G_add_keyword(_("classification"));
    G_add_keyword(_("object recognition"));
    module->description =
	_("Identifies segments (objects) from imagery data.");

    parse_args(argc, argv, &globals);

    G_debug(1, "Main: starting open_files()");
    if (open_files(&globals) != TRUE)
	G_fatal_error(_("Error in reading data"));

    G_debug(1, "Main: starting create_isegs()");
    if (create_isegs(&globals) != TRUE)
	G_fatal_error(_("Error in creating segments"));

    G_debug(1, "Main: starting write_output()");
    if (write_ids(&globals) != TRUE)
	G_fatal_error(_("Error in writing IDs"));

    if (globals.method == ORM_RG && globals.gof) {
	if (write_gof_rg(&globals) != TRUE)
	    G_fatal_error(_("Error in writing goodness of fit"));
    }

    if (globals.method == ORM_MS && globals.bsuf) {
	if (write_bands_ms(&globals) != TRUE)
	    G_fatal_error(_("Error in writing new band values"));
    }

    G_debug(1, "Main: starting close_files()");
    close_files(&globals);

    G_done_msg(_("Number of segments created: %d"), globals.max_rid);

    exit(EXIT_SUCCESS);
}


/****************************************************************************
 *
 * MODULE:       i.segment
 * AUTHOR(S):    Eric Momsen <eric.momsen at gmail com>
 * PURPOSE:      Segments an image group.
 * COPYRIGHT:    (C) 2012 by Eric Momsen, and the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the COPYING file that comes with GRASS
 *               for details.
 * 
 *
 *               NOTE: the word "segment" is already used by the Segmentation
 *               Library for the data files/tiling, so iseg (image segmentation)
 *               will be used to refer to the image segmentation.
 * 
 * 
 *****************************************************************************/

#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "iseg.h"

int main(int argc, char *argv[])
{
    struct globals globals;		/* input and output file descriptors, data structure, buffers */
    struct GModule *module;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("imagery"));
    G_add_keyword(_("segmentation"));
    module->description =
	_("Outputs a single segmented map (raster) based on input values in an image group.");

    if (parse_args(argc, argv, &globals) != TRUE)
	G_fatal_error(_("Error in parse_args()"));

    G_debug(1, "Main: starting open_files()");
    if (open_files(&globals) != TRUE)
	G_fatal_error(_("Error in open_files()"));

    G_debug(1, "Main: starting create_isegs()");
    if (create_isegs(&globals) != TRUE)
	G_fatal_error(_("Error in create_isegs()"));

    G_debug(1, "Main: starting write_output()");
    if (write_output(&globals) != TRUE)
	G_fatal_error(_("Error in write_output()"));

    G_debug(1, "Main: starting close_files()");
    close_files(&globals);

    G_done_msg(_("Number of segments created: %d"), globals.n_regions);

    exit(EXIT_SUCCESS);
}

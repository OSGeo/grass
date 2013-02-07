
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
 *               NOTE: the names "segment" and "SEG" are already used by the Segmentation
 *               Library for the data files/tiling, so "iseg" (image segmentation)
 *               will be used to refer to the image segmentation.
 * 
 * 				 First developed for GSoC 2012 with mentor: Markus Metz
 * 
 *****************************************************************************/

#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/imagery.h>
#include "iseg.h"

int main(int argc, char *argv[])
{
    struct files files;		/* input and output file descriptors, data structure, buffers */
    struct functions functions;	/* function pointers and parameters for the calculations */
    struct GModule *module;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("imagery"));
    G_add_keyword(_("segmentation"));
    module->description =
	_("Outputs a single segmented map (raster) based on input values in an image group.");

    if (parse_args(argc, argv, &files, &functions) != TRUE)
	G_fatal_error(_("Error in parse_args()"));

    /* check if we are doing normal processing or if the estimate threshold and exit flag has been selected */

    if (functions.estimate_threshold == FALSE) {

	G_debug(1, "Main: starting open_files()");
	if (open_files(&files, &functions) != TRUE)
	    G_fatal_error(_("Error in open_files()"));

	G_debug(1, "Main: starting create_isegs()");
	if (create_isegs(&files, &functions) != TRUE)
	    G_fatal_error(_("Error in create_isegs()"));

	G_debug(1, "Main: starting write_output()");
	if (write_output(&files) != TRUE)
	    G_fatal_error(_("Error in write_output()"));

	G_debug(1, "Main: starting close_files()");
	close_files(&files);

	G_done_msg(_("Number of segments created: <%d>"), files.nsegs);
    }
    else {
	estimate_threshold(files.image_group);
    }
    exit(EXIT_SUCCESS);
}

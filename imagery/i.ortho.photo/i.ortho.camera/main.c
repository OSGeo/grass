
/****************************************************************************
 *
 * MODULE:       i.photo.camera
 * AUTHOR(S):    Mike Baba,  DBA Systems, Inc. (original contributor)
 *               Markus Neteler <neteler itc.it>,
 *               Roberto Flor <flor itc.it>, 
 *               Bernhard Reiter <bernhard intevation.de>, 
 *               Glynn Clements <glynn gclements.plus.com>
 *               Hamish Bowman
 *
 * PURPOSE:      Creates or modifies entries in a camera reference file
 * COPYRIGHT:    (C) 1999-2008 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
/* select_camera */
/* select a camera reference file for a given imagery group */

#include <stdlib.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/glocale.h>
#include "orthophoto.h"

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct Option *group_opt, *camera_opt;
    /* options for 
     * camera name
     * camera definition
     * calibrated focal length
     * principal point of symmetry x,y
     * fiducial coordinates in mm from the principal point */

    const char *location;
    const char *mapset;
    char *group;
    char *camera;
    struct Ortho_Camera_File_Ref cam_info;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("imagery"));
    G_add_keyword(_("orthorectify"));
    module->description =
	_("Interactively select and modify the imagery group camera reference file.");

    group_opt = G_define_standard_option(G_OPT_I_GROUP);
    group_opt->description =
	_("Name of imagery group for ortho-rectification");

    camera_opt = G_define_standard_option(G_OPT_F_INPUT);
    camera_opt->key = "camera";
    camera_opt->required = NO;
    camera_opt->gisprompt = "old_file,camera,camera";
    camera_opt->description =
	_("Name of camera reference file to use");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);


    camera = (char *)G_malloc(GNAME_MAX * sizeof(char));

    location = G_location();
    mapset = G_mapset();

    group = group_opt->answer;

    /* group must be in current mapset */
    if (!I_find_group(group))
	G_fatal_error(_("No group '%s' in current mapset"), group);

    if (camera_opt->answer) {
	if (G_legal_filename (camera_opt->answer) < 0)
	    G_fatal_error(_("<%s> is an illegal file name"),
			  camera_opt->answer);
	else
	    strcpy(camera, camera_opt->answer);
    }
    else {
	/* use user-provided camera definition */

	I_put_cam_info(camera, &cam_info);
    }

    /* I_put_camera (camera); */
    I_put_group_camera(group, camera);

    G_message(
	_("Group [%s] in location [%s] mapset [%s] now has camera file [%s]"),
	  group, location, mapset, camera);


    /* print camera info */

    exit(EXIT_SUCCESS);
}

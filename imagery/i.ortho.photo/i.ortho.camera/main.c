
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

/* create/edit a camera reference file 
 * optionally set the camera for a given imagery group */

#include <stdlib.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/glocale.h>
#include "orthophoto.h"

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct Option *group_opt,     /* group */
                  *camera_opt,    /* name of camera file camera */
		  *cname_opt,     /* camera name */
		  *cid_opt,       /* camera id */
		  *cfl_opt,       /* calibrated focal length */
		  *pp_opt,        /* principal point of symmetry x,y */
		  *fid_opt;	  /* coordinates of fiducials */

    /* flag to print camera info */
    /* flag to print camera info in shell script style */

    const char *location;
    const char *mapset;
    char *group;
    char *camera, *cam_name, *cam_id;
    double ppx, ppy, cfl;
    struct Ortho_Camera_File_Ref cam_info;
    int put_cam_info = 0;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("imagery"));
    G_add_keyword(_("orthorectify"));
    module->description =
	_("Select and modify the imagery group camera reference file.");

    group_opt = G_define_standard_option(G_OPT_I_GROUP);
    group_opt->required = NO;
    group_opt->description =
	_("Name of imagery group for ortho-rectification");

    camera_opt = G_define_standard_option(G_OPT_F_INPUT);
    camera_opt->key = "camera";
    camera_opt->required = YES;
    camera_opt->gisprompt = "old_file,camera,camera";
    camera_opt->label =
	_("Name of camera reference file");

    cname_opt = G_define_option();
    cname_opt->type = TYPE_STRING; 
    cname_opt->key = "name";
    cname_opt->label =
	_("Camera name");

    cid_opt = G_define_option();
    cid_opt->type = TYPE_STRING; 
    cid_opt->key = "id";
    cid_opt->label =
	_("Camera id");

    cfl_opt = G_define_option();
    cfl_opt->type = TYPE_DOUBLE; 
    cfl_opt->key = "clf";
    cfl_opt->label =
	_("Calibrated focal length");

    pp_opt = G_define_standard_option(G_OPT_M_COORDS);
    pp_opt->key = "pp";
    pp_opt->label =
	_("Principal point coordinates");

    fid_opt = G_define_standard_option(G_OPT_M_COORDS);
    fid_opt->key = "fid";
    fid_opt->multiple = YES;
    fid_opt->label =
	_("Fiducial coordinates");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    location = G_location();
    mapset = G_mapset();

    group = group_opt->answer;
    camera = camera_opt->answer;
    cam_name = cname_opt->answer;
    cam_id = cid_opt->answer;

    if (G_legal_filename(camera) < 0)
	G_fatal_error(_("<%s> is an illegal file name"),
		      camera);

    if (cam_name || cam_id || cfl_opt->answer || pp_opt->answers || fid_opt->answers)
	put_cam_info = 1;

    ppx = ppy = .0;
    if (pp_opt->answers) {
	sscanf(pp_opt->answers[0], "%lf", &ppx);
	sscanf(pp_opt->answers[1], "%lf", &ppy);
    }

    if (G_find_file2("camera", camera, G_mapset())) {
	/* use existing camera file */
	if (I_get_cam_info(camera, &cam_info))
	    G_fatal_error(_("Can not read camera file '%s'"), camera);

	if (cam_name && strcmp(cam_name, cam_info.cam_name)) {
	    G_message(_("Replacing camera name '%s' with '%s'"),
	              cam_info.cam_name, cam_name);
	    strcpy(cam_info.cam_name, cam_name);
	}

	if (cam_id && strcmp(cam_id, cam_info,cam_id)) {
	    G_message(_("Replacing camera cam_id '%s' with '%s'"),
	              cam_info.cam_id, cam_id);
	    strcpy(cam_info.cam_id, cam_id);
	}
	
	if (clf_opt->answer) {
	    cfl = atof(clf_opt->answer);
	    if (cfl != cam_info.CFL) {
		G_message(_("Replacing calibrated focal length '%g' with '%g'"),
			  cam_info.CFL, cfl);
		cam_info.CFL = cfl;
	    }
	}

	if (pp_opt->answers) {
	    G_message(_("Replacing coordinates of principal point '%.17g, %.17g' with '%.17g, %.17g'"),
	              cam_info.Xp, cam_info.Yp, ppx, ppy);
	    cam_info.Xp = ppx;
	    cam_info.Yp = ppy;
	}

    }
    else {
	/* create new camera file */

	if (!cam_name)
	    G_fatal_error(_("Please provide a camera name for a new camera definition"));
	strcpy(cam_info.cam_name, cam_name);
	
	if (!cam_id)
	    G_fatal_error(_("Please provide a camera ID for a new camera definition"));
	strcpy(cam_info.cam_id, cam_id);

	if (!cfl_opt->answer)
	    G_fatal_error(_("Please provide calibrated focal length for a new camera definition"));
	cam_info.CFL = atof(cfl_opt->answer);

	if (!pp_opt->answers)
	    G_message(_("Using default coordinates 0.0, 0.0 for the principal point"));
	cam_info.Xp = ppx;
	cam_info.Yp = ppy;
    }

    /* fiducials */
    if (fid_opt->answers) {
	int i, fid_no;
	double Xf, Yf;
	
	for (i = 0, fid_no = 0; fid_opt->answers[i] != NULL; i += 2, fid_no++) {
	    /* cam_info can hold max 20 fiducials */
	    if (fid_no > 19) {
		G_warning(_("Too many fiducials!"));
		break;
	    }
	    sscanf(fid_opt->answers[i], "%lf", &Xf);
	    cam_info.fiducials[fid_no].Xf = Xf;
	    sscanf(fid_opt->answers[i + 1], "%lf", &Yf);
	    cam_info.fiducials[fid_no].Yf = Yf;
	    
	    sprintf(cam_info.fiducials[fid_no].fid_id, "%d", fid_no);
	}
	cam_info.num_fid = fid_no;
    }

    if (put_cam_info) {
	/* create/modify camera file */
	I_put_cam_info(camera, &cam_info);
    }

    /* set group camera */
    if (group) {
	/* group must be in current mapset */
	if (!I_find_group(group))
	    G_fatal_error(_("No group '%s' in current mapset"), group);

	I_put_group_camera(group, camera);

	G_message(
	    _("Group [%s] in location [%s] mapset [%s] now uses camera file [%s]"),
	      group, location, mapset, camera);
    }

    exit(EXIT_SUCCESS);
}

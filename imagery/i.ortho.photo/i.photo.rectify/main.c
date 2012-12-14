
/****************************************************************************
 *
 * MODULE:       i.photo.rectify
 * AUTHOR(S):    Mike Baba,  DBA Systems, Inc. (original contributor)
 *               Markus Neteler <neteler itc.it>, 
 *               Bernhard Reiter <bernhard intevation.de>, 
 *               Glynn Clements <glynn gclements.plus.com>, 
 *               Hamish Bowman <hamish_b yahoo.com>,
 *               Markus Metz
 *
 * PURPOSE:      Rectifies an image by using the image to photo coordinate 
 *               and photo to target transformation matrices
 * COPYRIGHT:    (C) 1999-2010 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include "global.h"

int seg_mb_img, seg_mb_elev;

int *ref_list;
struct Ortho_Image_Group group;

char *elev_name;
char *elev_mapset;

func interpolate;

struct Cell_head target_window;

void err_exit(char *, char *);

/* modify this table to add new methods */
struct menu menu[] = {
    {p_nearest, "nearest", "nearest neighbor"},
    {p_bilinear, "bilinear", "bilinear"},
    {p_cubic, "cubic", "cubic convolution"},
    {p_bilinear_f, "bilinear_f", "bilinear with fallback"},
    {p_cubic_f, "cubic_f", "cubic convolution with fallback"},
    {NULL, NULL, NULL}
};

static char *make_ipol_list(void);

int main(int argc, char *argv[])
{
    char extension[INAME_LEN];
    char *ipolname;		/* name of interpolation method */
    int method;
    char *seg_mb;
    int i, m, k = 0;
    int got_file = 0, target_overwrite = 0;
    char *overstr;

    char *camera;
    int n, nfiles;
    char tl[100];
    char math_exp[100];
    char units[100];
    char nd[100];

    struct Cell_head cellhd, elevhd;

    struct Option *grp,         /* imagery group */
     *ifile,			/* input files */
     *ext,			/* extension */
     *tres,			/* target resolution */
     *mem,			/* amount of memory for cache */
     *interpol,			/* interpolation method:
				   nearest neighbor, bilinear, cubic */
     *angle;			/* camera angle relative to ground surface */

    struct Flag *c, *a;
    struct GModule *module;

    G_gisinit(argv[0]);

    module = G_define_module();
    module->keywords = _("imagery, orthorectify");
    module->description =
	_("Orthorectifies an image by using the image to photo coordinate transformation matrix.");

    grp = G_define_standard_option(G_OPT_I_GROUP);

    ifile = G_define_standard_option(G_OPT_R_INPUTS);
    ifile->required = NO;

    ext = G_define_option();
    ext->key = "extension";
    ext->type = TYPE_STRING;
    ext->required = YES;
    ext->multiple = NO;
    ext->description = _("Output raster map(s) suffix");

    tres = G_define_option();
    tres->key = "resolution";
    tres->type = TYPE_DOUBLE;
    tres->required = NO;
    tres->description = _("Target resolution (ignored if -c flag used)");

    mem = G_define_option();
    mem->key = "memory";
    mem->type = TYPE_DOUBLE;
    mem->key_desc = "memory in MB";
    mem->required = NO;
    mem->answer = "300";
    mem->description = _("Amount of memory to use in MB");

    ipolname = make_ipol_list();

    interpol = G_define_option();
    interpol->key = "method";
    interpol->type = TYPE_STRING;
    interpol->required = NO;
    interpol->answer = "nearest";
    interpol->options = ipolname;
    interpol->description = _("Interpolation method to use");
    
    angle = G_define_standard_option(G_OPT_R_OUTPUT);
    angle->key = "angle";
    angle->required = NO;
    angle->description = _("Raster map with camera angle relative to ground surface");

    c = G_define_flag();
    c->key = 'c';
    c->description =
	_("Use current region settings in target location (def.=calculate smallest area)");

    a = G_define_flag();
    a->key = 'a';
    a->description = _("Rectify all raster maps in group");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    /* get the method */
    for (method = 0; (ipolname = menu[method].name); method++)
	if (strcmp(ipolname, interpol->answer) == 0)
	    break;

    if (!ipolname)
	G_fatal_error(_("<%s=%s> unknown %s"),
		      interpol->key, interpol->answer, interpol->key);
    interpolate = menu[method].method;

    G_strip(grp->answer);
    strcpy(group.name, grp->answer);
    strcpy(extension, ext->answer);

    seg_mb = NULL;
    if (mem->answer) {
	if (atoi(mem->answer) > 0)
	    seg_mb = mem->answer;
    }

    if (!ifile->answers)
	a->answer = 1;		/* force all */

    /* Find out how many files on command line */
    if (!a->answer) {
	for (k = 0; ifile->answers[k]; k++);
    }

    camera = (char *)G_malloc(GNAME_MAX * sizeof(char));
    elev_name = (char *)G_malloc(GNAME_MAX * sizeof(char));
    elev_mapset = (char *)G_malloc(GMAPSET_MAX * sizeof(char));

    /* find group */
    if (!I_find_group(group.name))
	G_fatal_error(_("Group <%s> not found"), group.name);

    /* get the group ref */
    if (!I_get_group_ref(group.name, (struct Ref *)&group.group_ref))
	G_fatal_error(_("Could not read REF file for group <%s>"), group.name);
    nfiles = group.group_ref.nfiles;
    if (nfiles <= 0) {
	G_important_message(_("Group <%s> contains no raster maps; run i.group"),
			    grp->answer);
	exit(EXIT_SUCCESS);
    }

    ref_list = (int *)G_malloc(nfiles * sizeof(int));

    if (a->answer) {
	for (n = 0; n < group.group_ref.nfiles; n++) {
	    ref_list[n] = 1;
	}
    }
    else {
	char xname[GNAME_MAX], xmapset[GMAPSET_MAX], *name, *mapset;

	for (n = 0; n < group.group_ref.nfiles; n++)
		ref_list[n] = 0;

	for (m = 0; m < k; m++) {
	    got_file = 0;
	    if (G__name_is_fully_qualified(ifile->answers[m], xname, xmapset)) {
		name = xname;
		mapset = xmapset;
	    }
	    else {
		name = ifile->answers[m];
		mapset = NULL;
	    }

	    got_file = 0;
	    for (n = 0; n < group.group_ref.nfiles; n++) {
		if (mapset) {
		    if (strcmp(name, group.group_ref.file[n].name) == 0 &&
		        strcmp(mapset, group.group_ref.file[n].mapset) == 0) {
			got_file = 1;
			ref_list[n] = 1;
			break;
		    }
		}
		else {
		    if (strcmp(name, group.group_ref.file[n].name) == 0) {
			got_file = 1;
			ref_list[n] = 1;
			break;
		    }
		}
	    }
	    if (got_file == 0)
		err_exit(ifile->answers[m], group.name);
	}
    }

    /** look for camera info for this block **/
    if (!I_get_group_camera(group.name, camera))
	G_fatal_error(_("No camera reference file selected for group <%s>"),
		      group.name);

    if (!I_get_cam_info(camera, &group.camera_ref))
	G_fatal_error(_("Bad format in camera file for group <%s>"),
		      group.name);

    /* get initial camera exposure station, if any */
    if (I_find_initial(group.name)) {
	if (!I_get_init_info(group.name, &group.camera_exp))
	    G_warning(_("Bad format in initial exposure station file for group <%s>"),
		      group.name);
    }

    /* read the reference points for the group, compute image-to-photo trans. */
    get_ref_points();

    /* read the control points for the group, convert to photo coords. */
    get_conz_points();

    /* get the target */
    get_target(group.name);
    
    /* Check the GRASS_OVERWRITE environment variable */
    if ((overstr = getenv("GRASS_OVERWRITE")))  /* OK ? */
	target_overwrite = atoi(overstr);

    if (!target_overwrite) {
	/* check if output exists in target location/mapset */
	char result[GNAME_MAX];
	
	select_target_env();
	for (i = 0; i < group.group_ref.nfiles; i++) {
	    if (!ref_list[i])
		continue;

	    strcpy(result, group.group_ref.file[i].name);
	    strcat(result, extension);
	    
	    if (G_legal_filename(result) < 0)
		G_fatal_error(_("Extension <%s> is illegal"), extension);
		
	    if (G_find_cell(result, G_mapset())) {
		G_warning(_("The following raster map already exists in"));
		G_warning(_("target LOCATION %s, MAPSET %s:"),
			  G_location(), G_mapset());
		G_warning("<%s>", result);
		G_fatal_error(_("Orthorectification cancelled."));
	    }
	}
	if (angle->answer) {
	    if (G_find_cell(angle->answer, G_mapset())) {
		G_warning(_("The following raster map already exists in"));
		G_warning(_("target LOCATION %s, MAPSET %s:"),
			  G_location(), G_mapset());
		G_warning("<%s>", angle->answer);
		G_fatal_error(_("Orthorectification cancelled."));
	    }
	}
	
	select_current_env();
    }
    else
	G_debug(1, "Overwriting OK");

    /* do not use current region in target location */
    if (!c->answer) {
	double res = -1;
	
	if (tres->answer) {
	    if (!((res = atof(tres->answer)) > 0))
		G_warning(_("Target resolution must be > 0, ignored"));
	}
	/* get reference window from imagery group */
	get_ref_window(&cellhd);
	georef_window(&cellhd, &target_window, res);
    }

    G_verbose_message(_("Using region: N=%f S=%f, E=%f W=%f"), target_window.north,
	      target_window.south, target_window.east, target_window.west);

    G_debug(1, "Looking for elevation file in group: <%s>", group.name);

    /* get the block elevation layer raster map in target location */
    if (!I_get_group_elev(group.name, elev_name, elev_mapset, tl,
			 math_exp, units, nd))
	G_fatal_error(_("No target elevation model selected for group <%s>"),
		      group.name);

    G_debug(1, "Block elevation: <%s> in <%s>", elev_name, elev_mapset);

    /* get the elevation layer header in target location */
    select_target_env();
    G_get_cellhd(elev_name, elev_mapset, &elevhd);
    select_current_env();
    
    /* determine memory for elevation and imagery */
    seg_mb_img = seg_mb_elev = -1;
    if (seg_mb) {
	int max_rows, max_cols;
	int nx, ny;
	double max_mb_img, max_mb_elev;
	int seg_mb_total;

	max_rows = max_cols = 0;
	for (i = 0; i < group.group_ref.nfiles; i++) {
	    if (ref_list[i]) {
		G_get_cellhd(group.group_ref.file[i].name,
			     group.group_ref.file[i].mapset, &cellhd);
		if (max_rows < cellhd.rows)
		    max_rows = cellhd.rows;
		if (max_cols < cellhd.cols)
		    max_cols = cellhd.cols;
	    }
	}

	ny = (max_rows + BDIM - 1) / BDIM;
	nx = (max_cols + BDIM - 1) / BDIM;

	max_mb_img = ((double)nx * ny * sizeof(block)) / (1<<20);

	ny = (target_window.rows + BDIM - 1) / BDIM;
	nx = (target_window.cols + BDIM - 1) / BDIM;

	max_mb_elev = ((double)nx * ny * sizeof(block)) / (1<<20);

	if ((seg_mb_total = atoi(seg_mb)) > 0) {
	    seg_mb_elev = max_mb_elev * (seg_mb_total / (max_mb_img + max_mb_elev)) + 0.5;
	    seg_mb_img = max_mb_img * (seg_mb_total / (max_mb_img + max_mb_elev)) + 0.5;
	}
    }

    /* go do it */
    exec_rectify(extension, interpol->answer, angle->answer);

    G_done_msg(" ");

    exit(EXIT_SUCCESS);
}


void err_exit(char *file, char *grp)
{
    int n;

    G_warning(_("Input raster map <%s> does not exist in group <%s>."),
	    file, grp);
    G_message(_("Try:"));

    for (n = 0; n < group.group_ref.nfiles; n++)
	G_message("%s@%s", group.group_ref.file[n].name, group.group_ref.file[n].mapset);

    G_fatal_error(_("Exit!"));
}

static char *make_ipol_list(void)
{
    int size = 0;
    int i;
    char *buf;

    for (i = 0; menu[i].name; i++)
	size += strlen(menu[i].name) + 1;

    buf = G_malloc(size);
    *buf = '\0';

    for (i = 0; menu[i].name; i++) {
	if (i)
	    strcat(buf, ",");
	strcat(buf, menu[i].name);
    }

    return buf;
}

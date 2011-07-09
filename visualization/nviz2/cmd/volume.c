/*!
  \file volume.c
  
  \brief Volume subroutines
  
  (C) 2008, 2010 by the GRASS Development Team

  This program is free software under the GNU General Public
  License (>=v2). Read the file COPYING that comes with GRASS
  for details.
  
  \author Martin Landa <landa.martin gmail.com> (Google SoC 2008/2010)
*/

#include <stdlib.h>
#include <string.h>

#include <grass/G3d.h>
#include <grass/glocale.h>

#include "local_proto.h"

/*!
  \brief Load 3d raster map layers -> volume
  
  \param params module parameters
  \param data nviz data

  \return number of loaded volumes
*/
int load_rasters3d(const struct GParams *params, nv_data *data)
{
    int i, nvol, id;
    float x, y, z;
    char *mapset;
    
    nvol = opt_get_num_answers(params->volume);

    for (i = 0; i < nvol; i++) {
	mapset = G_find_grid3(params->volume->answers[i], "");
	if (mapset == NULL) {
	    G_fatal_error(_("3d raster map <%s> not found"),
			  params->volume->answers[i]);
	}

	id = Nviz_new_map_obj(MAP_OBJ_VOL,
			      G_fully_qualified_name(params->volume->answers[i],
						     mapset),
			      0.0, data);

	/* set position */
	if (opt_get_num_answers(params->volume_pos) != 3 * nvol) {
	    x = atof(params->volume_pos->answers[0]);
	    y = atof(params->volume_pos->answers[1]);
	    z = atof(params->volume_pos->answers[2]);
	}
	else {
	    x = atof(params->volume_pos->answers[i*3+0]);
	    y = atof(params->volume_pos->answers[i*3+1]);
	    z = atof(params->volume_pos->answers[i*3+2]);
	}
    
	GVL_set_trans(id, x, y, z);
    }

    return 1;
}

/*!
  \brief Add isosurfaces and set their attributes
  
  \param params module parameters
  \param data nviz data
  
  \return number of defined isosurfaces
*/
int add_isosurfs(const struct GParams *params, nv_data *data)
{
    int i;
    float level;
    int num, nvols, *vol_list, id, nisosurfs;
    int ncolor_map, ncolor_const, ntransp_map, ntransp_const, nshine_map, nshine_const;
    int res, draw_mode;
    char **tokens;
    const char *mapset, *style;
    
    vol_list = GVL_get_vol_list(&nvols);

    for (i = 0; params->isosurf_level->answers[i]; i++) {
	tokens = G_tokenize(params->isosurf_level->answers[i], ":");
	if (G_number_of_tokens(tokens) != 2) 
	    G_fatal_error(_("Error tokenize '%s'"), 
			  params->isosurf_level->answers[i]);
	num = atoi(tokens[0]);
	level = atof(tokens[1]);
	G_free_tokens(tokens);

	if (num > nvols) {
	    G_fatal_error(_("Volume set number %d is not available"), 
			  num);
	}

	id = vol_list[num-1];
	if (GVL_isosurf_add(id) < 0) {
	    G_fatal_error(_("Unable to add isosurface (volume set %d)"),
			  id);
	}

	nisosurfs = GVL_isosurf_num_isosurfs(id);

	/* topography (level) */
	if (GVL_isosurf_set_att_const(id, nisosurfs-1, ATT_TOPO, level) < 0) {
	    G_fatal_error(_("Unable to set isosurface (%d) attribute (%d) of volume %d"),
			  nisosurfs-1, ATT_TOPO, id);
	}

	/* color */
	ncolor_map = opt_get_num_answers(params->isosurf_color_map);
	ncolor_const = opt_get_num_answers(params->isosurf_color_const);

	if (i < ncolor_map && strcmp(params->isosurf_color_map->answers[i], "")) {
	    mapset = G_find_grid3(params->isosurf_color_map->answers[i], "");

	    if (mapset == NULL) {
		G_fatal_error(_("3d raster map <%s> not found"),
			      params->isosurf_color_map->answers[i]);
	    }

	    if (GVL_isosurf_set_att_map(id, nisosurfs-1, ATT_COLOR, 
					params->isosurf_color_map->answers[i]) < 0)
		G_fatal_error(_("Unable to set isosurface (%d) attribute (%d) of volume %d"),
				nisosurfs-1, ATT_COLOR, id);
	}
	else if (i-ncolor_map < ncolor_const &&
		 strcmp(params->isosurf_color_const->answers[i-ncolor_map], "")) {

	    if (GVL_isosurf_set_att_const(id, nisosurfs-1, ATT_COLOR,
			    Nviz_color_from_str(params->isosurf_color_const->answers[i-ncolor_map])) < 0)
		G_fatal_error(_("Unable to set isosurface (%d) attribute (%d) of volume %d"),
				nisosurfs-1, ATT_COLOR, id);
	}
	else {			/* use by default 3d raster map for coloring */
	    GVL_isosurf_set_att_map(id, nisosurfs-1, ATT_COLOR, params->volume->answers[num-1]);
	    G_verbose_message(_("Color attribute not defined, using default <%s>"),
				params->volume->answers[num-1]);
	}

	/* transparency */
	ntransp_map = opt_get_num_answers(params->isosurf_transp_map);
	ntransp_const = opt_get_num_answers(params->isosurf_transp_const);

	if (i < ntransp_map && strcmp(params->isosurf_transp_map->answers[i], "")) {
	    if (GVL_isosurf_set_att_map(id, nisosurfs-1, ATT_TRANSP, 
					params->isosurf_transp_map->answers[i]) < 0)
		G_fatal_error(_("Unable to set isosurface (%d) attribute (%d) of volume %d"),
				nisosurfs-1, ATT_TRANSP, id);
	}
	else if (i-ntransp_map < ntransp_const &&
		 strcmp(params->isosurf_transp_const->answers[i-ntransp_map], "")) {
	    if (GVL_isosurf_set_att_const(id, nisosurfs-1, ATT_TRANSP,
					  atof(params->isosurf_transp_const->answers[i-ntransp_map])) < 0)
		G_fatal_error(_("Unable to set isosurface (%d) attribute (%d) of volume %d"),
				nisosurfs-1, ATT_TRANSP, id);
	}

	/* shine */
	nshine_map = opt_get_num_answers(params->isosurf_shine_map);
	nshine_const = opt_get_num_answers(params->isosurf_shine_const);

	if (i < nshine_map && strcmp(params->isosurf_shine_map->answers[i], "")) {
	    if (GVL_isosurf_set_att_map(id, nisosurfs-1, ATT_SHINE, 
					params->isosurf_shine_map->answers[i]) < 0)
		G_fatal_error(_("Unable to set isosurface (%d) attribute (%d) of volume %d"),
				nisosurfs-1, ATT_SHINE, id);
	}
	else if (i-nshine_map < nshine_const &&
		 strcmp(params->isosurf_shine_const->answers[i-nshine_map], "")) {
	    if (GVL_isosurf_set_att_const(id, nisosurfs-1, ATT_SHINE,
					  atof(params->isosurf_shine_const->answers[i-nshine_map])) < 0)
		G_fatal_error(_("Unable to set isosurface (%d) attribute (%d) of volume %d"),
				nisosurfs-1, ATT_SHINE, id);
	}
    }

    /* set draw resolution and shading after isosurfaces are added*/
    for (i = 0; i < nvols; i++) {

	id = vol_list[i];
	/* set resolution */
	if (opt_get_num_answers(params->volume_res) != nvols)
	    res = atof(params->volume_res->answers[0]);
	else
	    res = atof(params->volume_res->answers[i]);

	GVL_isosurf_set_drawres(id, res, res, res);

	/* set shading */
	if (opt_get_num_answers(params->volume_shade) != nvols)
	    style = params->volume_shade->answers[0];
	else
	    style = params->volume_shade->answers[i];

	draw_mode = 0;

	if (strcmp(style, "flat") == 0) {
	    draw_mode |= DM_FLAT;
	}
	else {
	    draw_mode |= DM_GOURAUD;
	}
	
	GVL_isosurf_set_drawmode(id, draw_mode);
    }

    return 1;
}

/*!
  \file surface.c
 
  \brief Surface procedures
  
  COPYRIGHT: (C) 2008 by the GRASS Development Team

  This program is free software under the GNU General Public
  License (>=v2). Read the file COPYING that comes with GRASS
  for details.

  \author Martin Landa <landa.martin gmail.com> (Google SoC 2008)

  \date 2008
*/

#include <stdlib.h>
#include <string.h>

#include <grass/glocale.h>

#include "local_proto.h"

/*!
  \brief Load raster maps/constants and set their attributes
  
  \param params module parameters
  \param data nviz data
*/
int load_rasters(const struct GParams *params,
		 nv_data *data)
{
    char *mapset;
    int i;
    int ncolors, nmask_map;
    int ntransp_map, ntransp_const, nshine_map, nshine_const;
    int nemit_map, nemit_const;
    int *surf_list, nsurfs;
    int id;

    /* topography */    
    if (params->elev_map->answer) {
	/* maps */
	for (i = 0; params->elev_map->answers[i]; i++) {
	    mapset = G_find_cell2 (params->elev_map->answers[i], "");
	    if (mapset == NULL) {
		G_fatal_error(_("Raster map <%s> not found"),
			      params->elev_map->answers[i]);
	    }
	    
	    id = Nviz_new_map_obj(MAP_OBJ_SURF,
				  G_fully_qualified_name(params->elev_map->answers[i], mapset), 0.0,
				  data);
	}
    }
    
    /* color */
    ncolors = opt_get_num_answers(params->color_map);
    if (params->elev_const->answer) {
	/* constants */
	float value;
	surf_list = GS_get_surf_list(&nsurfs);
	for (i = 0; i < nsurfs; i++) {
	    if (i < ncolors) { /* check map first */
		value = atof(params->elev_const->answers[i]);
	    }
	    /* check for color */
	    if (i + nsurfs >= ncolors) {
		G_fatal_error (_("Missing color settings for elevation value %f"),
			       value);
		/* topography */
		id = Nviz_new_map_obj(MAP_OBJ_SURF,
				      NULL, value,
				      data);
	    }
	}
    }

    /* set surface attributes */
    surf_list = GS_get_surf_list(&nsurfs);
    nmask_map = opt_get_num_answers(params->mask_map);
    ntransp_map = opt_get_num_answers(params->transp_map);
    ntransp_const = opt_get_num_answers(params->transp_const);
    nshine_map = opt_get_num_answers(params->shine_map);
    nshine_const = opt_get_num_answers(params->shine_const);
    nemit_map = opt_get_num_answers(params->emit_map);
    nemit_const = opt_get_num_answers(params->emit_const);
    for (i = 0; i < nsurfs; i++) {
	id = surf_list[i];
	/* color */
	if (i < ncolors) {
	    mapset = G_find_cell2 (params->color_map->answers[i], "");
	    if (mapset == NULL) {
		G_fatal_error(_("Raster map <%s> not found"),
			      params->color_map->answers[i]);
	    }
	    
	    Nviz_set_attr(id, MAP_OBJ_SURF, ATT_COLOR, MAP_ATT,
			  G_fully_qualified_name(params->color_map->answers[i], mapset), -1.0,
			  data);
	}
	else if (i < ncolors) { /* check for color value */
	    Nviz_set_attr(id, MAP_OBJ_SURF, ATT_COLOR, CONST_ATT,
			  NULL, Nviz_color_from_str(params->color_const->answers[i]),
			  data);
	}
	else { /* use by default elevation map for coloring */
	    Nviz_set_attr(id, MAP_OBJ_SURF, ATT_COLOR, MAP_ATT,
			  G_fully_qualified_name(params->elev_map->answers[i], mapset), -1.0,
			  data);
	}
	/* mask */
	if (i < nmask_map) {
	    Nviz_set_attr(id, MAP_OBJ_SURF, ATT_MASK, MAP_ATT,
			  G_fully_qualified_name(params->mask_map->answers[i], mapset), -1.0,
			  data);
	}

	/* transparency */
	if (i < ntransp_map) {
	    Nviz_set_attr(id, MAP_OBJ_SURF, ATT_TRANSP, MAP_ATT,
			  G_fully_qualified_name(params->transp_map->answers[i], mapset), -1.0,
			  data);
	}
	else if (i < ntransp_map + ntransp_const) {
	    Nviz_set_attr(id, MAP_OBJ_SURF, ATT_TRANSP, CONST_ATT,
			  NULL, atof(params->transp_const->answers[i-ntransp_map]),
			  data);
	}

	/* shininess */
	if (i < nshine_map) {
	    Nviz_set_attr(id, MAP_OBJ_SURF, ATT_SHINE, MAP_ATT,
			  G_fully_qualified_name(params->shine_map->answers[i], mapset), -1.0,
			  data);
	}
	else if (i < nshine_map + nshine_const) {
	    Nviz_set_attr(id, MAP_OBJ_SURF, ATT_SHINE, CONST_ATT,
			  NULL, atof(params->shine_const->answers[i-nshine_map]),
			  data);
	}

	/* emission */
	if (i < nemit_map) {
	    Nviz_set_attr(id, MAP_OBJ_SURF, ATT_EMIT, MAP_ATT,
			  G_fully_qualified_name(params->emit_map->answers[i], mapset), -1.0,
			  data);
	}
	else if (i < nemit_map + nemit_const) {
	    Nviz_set_attr(id, MAP_OBJ_SURF, ATT_EMIT, CONST_ATT,
			  NULL, atof(params->emit_const->answers[i-nemit_map]),
			  data);
	}

        /*
	  if (i > 1)
	  set_default_wirecolors(data, i);
	*/
    }

    return nsurfs;
}

/*!
  \brief Set draw mode for loaded surfaces

  \param params module parameters
*/
void set_draw_mode(const struct GParams *params)
{
    int *surf_list, nsurfs;
    int i, id, draw_mode;
    int resol_fine, resol_coarse;
    
    char *mode, *style, *shade, *res_fine, *res_coarse, *wire_color;

    surf_list = GS_get_surf_list(&nsurfs);

    for (i = 0; i < nsurfs; i++) {
	draw_mode = 0;
	id = surf_list[i];
	if (!GS_surf_exists(id))
	    G_fatal_error (_("Surface id %d doesn't exist"), id);
	
	if (params->mode_all->answer) { /* use one mode for all surfaces */
	    mode = params->mode->answers[0];
	    style = params->style->answers[0];
	    shade = params->shade->answers[0];
	    res_fine = params->res_fine->answers[0];
	    res_coarse = params->res_coarse->answers[0];
	    wire_color = params->wire_color->answers[0];
	}
	else {
	    mode = params->mode->answers[i];
	    style = params->style->answers[i];
	    shade = params->shade->answers[i];
	    res_fine = params->res_fine->answers[i];
	    res_coarse = params->res_coarse->answers[i];
	    wire_color = params->wire_color->answers[i];
	}

	/* mode */
	if (strcmp(mode, "coarse") == 0) {
	    draw_mode |= DM_WIRE;
	}
	else if (strcmp(mode, "fine") == 0) {
	    draw_mode |= DM_POLY;
	}
	else { /* both */
	    draw_mode |= DM_WIRE_POLY;
	}

	/* style */
	if (strcmp(params->style->answers[i], "wire") == 0) {
	    draw_mode |= DM_GRID_WIRE;
	}
	else { /* surface */
	    draw_mode |= DM_GRID_SURF;
	}

	/* shading */
	if (strcmp(params->shade->answers[i], "flat") == 0) {
	    draw_mode |= DM_FLAT;
	}
	else { /* gouraud */
	    draw_mode |= DM_GOURAUD;
	}

	if (GS_set_drawmode(id, draw_mode) < 0)
	    G_fatal_error (_("Unable to set draw mode for surface id %d"),
			   id);

	/* resolution */
	resol_fine = atoi(res_fine);
	resol_coarse = atoi(res_coarse);
	if (GS_set_drawres(id, resol_fine, resol_fine,
			   resol_coarse, resol_coarse) < 0)
	    G_fatal_error (_("Unable to set draw mode for surface id %d"),
			   id);
	
	/* wire color */
	GS_set_wire_color(id, Nviz_color_from_str(wire_color));
    }

    return;
}

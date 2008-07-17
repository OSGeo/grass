/*!
  \file vector.c
 
  \brief Vector subroutines
  
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
  \brief Load vector maps and set attributes
  
  \param params module parameters
  \param data nviz data

  \return number of loaded vectors
*/
int load_vectors(const struct GParams *params,
		 nv_data *data)
{
    int i;
    int nvects;

    char *mapset;

    if (!params->elev_map->answer && GS_num_surfs() == 0) { /* load base surface if no loaded */
	int *surf_list, nsurf;
	
	Nviz_new_map_obj(MAP_OBJ_SURF, NULL, 0.0, data);
	
	surf_list = GS_get_surf_list(&nsurf);
	GS_set_att_const(surf_list[0], ATT_TRANSP, 255);
    }

    nvects = 0;

    for (i = 0; params->vector->answers[i]; i++) {
	mapset = G_find_vector2 (params->vector->answers[i], "");
	if (mapset == NULL) {
	    G_fatal_error(_("Vector map <%s> not found"),
			  params->vector->answers[i]);
	}
	Nviz_new_map_obj(MAP_OBJ_VECT,
			 G_fully_qualified_name(params->vector->answers[i], mapset), 0.0,
			 data);

	nvects++;
    }

    return nvects;
}

/*!
  \brief Set vector mode

  \param params parameters

  \return 1 on success
  \return 0 on failure
*/
int set_lines_attrb(const struct GParams *params)
{
    int i, color, width, flat, height;
    int *vect_list, nvects;

    vect_list = GV_get_vect_list(&nvects);
    
    for(i = 0; i < nvects; i++) {
	/* mode -- use memory by default */
	color =  Nviz_color_from_str(params->line_color->answers[i]);
	width = atoi(params->line_width->answers[i]);
	if (strcmp(params->line_mode->answers[i], "flat") == 0)
	    flat = 1;
	else
	    flat = 0;
	if (GV_set_vectmode(vect_list[i], 1, color, width, flat) < 0)
	    return 0;

	/* height */
	height = atoi(params->line_height->answers[i]);
	if (height > 0)
	    GV_set_trans(vect_list[i], 0.0, 0.0, height);
    }

    return 1;
}

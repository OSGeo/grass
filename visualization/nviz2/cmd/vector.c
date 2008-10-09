/*!
   \file vector.c

   \brief Vector subroutines

   (C) 2008 by the GRASS Development Team

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

static int load_vectors(const struct Option *, const struct Option *,
			const struct Option *, const struct Option *, int, nv_data *);

/*!
   \brief Load vector maps (lines)

   \param params module parameters
   \param data nviz data

   \return number of loaded vectors
 */
int load_vlines(const struct GParams *params, nv_data * data)
{
    return load_vectors(params->elev_map, params->elev_const,
			params->vlines, params->vline_pos,
			MAP_OBJ_VECT, data);
}

/*!
   \brief Load vector maps (points)

   \param params module parameters
   \param data nviz data

   \return number of loaded vectors
 */
int load_vpoints(const struct GParams *params, nv_data * data)
{
    return load_vectors(params->elev_map, params->elev_const,
			params->vpoints, params->vpoint_pos,
			MAP_OBJ_SITE, data);
}

int load_vectors(const struct Option *elev_map,
		 const struct Option *elev_const, const struct Option *vect,
		 const struct Option *position,
		 int map_obj_type, nv_data * data)
{
    int i, id;
    int nvects;

    char *mapset;

    double x, y, z;
    
    if ((!elev_map->answer || elev_const->answer) && GS_num_surfs() == 0) {	/* load base surface if no loaded */
	int *surf_list, nsurf;

	Nviz_new_map_obj(MAP_OBJ_SURF, NULL, 0.0, data);

	surf_list = GS_get_surf_list(&nsurf);
	GS_set_att_const(surf_list[0], ATT_TRANSP, 255);
    }

    nvects = 0;

    for (i = 0; vect->answers[i]; i++) {
	mapset = G_find_vector2(vect->answers[i], "");
	if (mapset == NULL) {
	    G_fatal_error(_("Vector map <%s> not found"), vect->answers[i]);
	}
	id = Nviz_new_map_obj(map_obj_type,
			      G_fully_qualified_name(vect->answers[i], mapset),
			      0.0, data);

	/* set position */
	x = atof(position->answers[i]);
	y = atof(position->answers[i+1]);
	z = atof(position->answers[i+2]);

	if (map_obj_type == MAP_OBJ_VECT)
	    GV_set_trans(id, x, y, z);
	else
	    GP_set_trans(id, x, y, z);

	nvects++;
    }

    return nvects;
}

/*!
   \brief Set vector lines mode

   \param params parameters

   \return 1 on success
   \return 0 on failure
 */
int vlines_set_attrb(const struct GParams *params)
{
    int i, color, width, flat, height;
    int *vect_list, nvects;

    vect_list = GV_get_vect_list(&nvects);

    for (i = 0; i < nvects; i++) {
	/* mode -- use memory by default */
	color = Nviz_color_from_str(params->vline_color->answers[i]);
	width = atoi(params->vline_width->answers[i]);
	if (strcmp(params->vline_mode->answers[i], "flat") == 0)
	    flat = 1;
	else
	    flat = 0;
	if (GV_set_vectmode(vect_list[i], 1, color, width, flat) < 0)
	    return 0;

	/* height */
	height = atoi(params->vline_height->answers[i]);
	if (height > 0)
	    GV_set_trans(vect_list[i], 0.0, 0.0, height);
    }

    return 1;
}

/*!
   \brief Set vector points mode

   \param params parameters

   \return 1 on success
   \return 0 on failure
 */
int vpoints_set_attrb(const struct GParams *params)
{
    int i;
    int *site_list, nsites;
    int marker, color, width;
    float size;
    char *marker_str;

    site_list = GP_get_site_list(&nsites);

    for (i = 0; i < nsites; i++) {
	color = Nviz_color_from_str(params->vpoint_color->answers[i]);
	size = atof(params->vpoint_size->answers[i]);
	width = atoi(params->vpoint_width->answers[i]);
	marker_str = params->vpoint_marker->answers[i];

	if (strcmp(marker_str, "x") == 0)
	    marker = ST_X;
	else if (strcmp(marker_str, "sphere") == 0)
	    marker = ST_SPHERE;
	else if (strcmp(marker_str, "diamond") == 0)
	    marker = ST_DIAMOND;
	else if (strcmp(marker_str, "cube") == 0)
	    marker = ST_CUBE;
	else if (strcmp(marker_str, "box") == 0)
	    marker = ST_BOX;
	else if (strcmp(marker_str, "gyro") == 0)
	    marker = ST_GYRO;
	else if (strcmp(marker_str, "aster") == 0)
	    marker = ST_ASTER;
	else if (strcmp(marker_str, "histogram") == 0)
	    marker = ST_HISTOGRAM;
	else
	    G_fatal_error(_("Unknown icon marker"));

	GP_set_sitemode(site_list[i], ST_ATT_NONE,
			color, width, size, marker);
    }

    return 1;
}

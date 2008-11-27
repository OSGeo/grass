/**
   \file load.cpp
   
   \brief Experimental C++ wxWidgets Nviz prototype -- load data layers

   Used by wxGUI Nviz extension.

   Copyright: (C) by the GRASS Development Team

   This program is free software under the GNU General Public
   License (>=v2). Read the file COPYING that comes with GRASS
   for details.

   \author Martin Landa <landa.martin gmail.com> (Google SoC 2008)

   \date 2008
*/

#include "nviz.h"

extern "C" {
#include <grass/G3d.h>
#include <grass/glocale.h>
}

/*!
  \brief Load raster map (surface)

  \param name raster map name
  \param color_name raster map for color (NULL for color_value)
  \param color_value color string (named color or RGB triptet)

  \return object id
  \return -1 on failure
*/
int Nviz::LoadSurface(const char* name, const char *color_name, const char *color_value)
{
    const char *mapset;
    int id;

    mapset = G_find_cell2 (name, "");
    if (mapset == NULL) {
	G_warning(_("Raster map <%s> not found"),
		  name);
	return -1;
    }
	    
    /* topography */
    id = Nviz_new_map_obj(MAP_OBJ_SURF,
			  G_fully_qualified_name(name, mapset), 0.0,
			  data);

    if (color_name) { /* check for color map */
	mapset = G_find_cell2 (color_name, "");
	if (mapset == NULL) {
	    G_warning(_("Raster map <%s> not found"),
		      color_name);
	    GS_delete_surface(id);
	    return -1;
	}

	Nviz_set_attr(id, MAP_OBJ_SURF, ATT_COLOR, MAP_ATT,
		      G_fully_qualified_name(color_name, mapset), -1.0,
		      data);
    }
    else if (color_value) { /* check for color value */
	Nviz_set_attr(id, MAP_OBJ_SURF, ATT_COLOR, CONST_ATT,
		      NULL, Nviz_color_from_str(color_value),
		      data);
    }
    else { /* use by default elevation map for coloring */
	Nviz_set_attr(id, MAP_OBJ_SURF, ATT_COLOR, MAP_ATT,
		      G_fully_qualified_name(name, mapset), -1.0,
		      data);
    }
	    
    /*
      if (i > 1)
      set_default_wirecolors(data, i);
    */

    /* focus on loaded data */
    Nviz_set_focus_map(MAP_OBJ_UNDEFINED, -1);

    G_debug(1, "Nviz::LoadRaster(): name=%s -> id=%d", name, id);

    return id;
}

/*!
  \brief Unload surface

  \param id surface id

  \return 1 on success
  \return 0 on failure
*/
int Nviz::UnloadSurface(int id)
{
    if (!GS_surf_exists(id)) {
	return 0;
    }

    G_debug(1, "Nviz::UnloadSurface(): id=%d", id);

    if (GS_delete_surface(id) < 0)
      return 0;

    return 1;
}

/*!
  \brief Load vector map overlay

  \param name vector map name
  \param points if true load 2d points rather then 2d lines

  \return object id
  \return -1 on failure
*/
int Nviz::LoadVector(const char *name, bool points)
{
    int id;
    const char *mapset;

    if (GS_num_surfs() == 0) { /* load base surface if no loaded */
	int *surf_list, nsurf;
	
	Nviz_new_map_obj(MAP_OBJ_SURF, NULL, 0.0, data);

	surf_list = GS_get_surf_list(&nsurf);
	GS_set_att_const(surf_list[0], ATT_TRANSP, 255);
    }

    mapset = G_find_vector2 (name, "");
    if (mapset == NULL) {
	G_warning(_("Vector map <%s> not found"),
		  name);
    }

    if (points) {
      id = Nviz_new_map_obj(MAP_OBJ_SITE,
			    G_fully_qualified_name(name, mapset), 0.0,
			    data);
    }
    else {
      id = Nviz_new_map_obj(MAP_OBJ_VECT,
			    G_fully_qualified_name(name, mapset), 0.0,
			    data);
    }

    G_debug(1, "Nviz::LoadVector(): name=%s -> id=%d", name, id);

    return id;
}
  
/*!
  \brief Unload vector set

  \param id vector set id
  \param points vector points or lines set

  \return 1 on success
  \return 0 on failure
*/
int Nviz::UnloadVector(int id, bool points)
{
    G_debug(1, "Nviz::UnloadVector(): id=%d", id);

    if (points) {
	if (!GP_site_exists(id)) {
	    return 0;
	}
	
	if (GP_delete_site(id) < 0)
	    return 0;
    }
    else {
	if (!GV_vect_exists(id)) {
	    return 0;
	}
	
	if (GV_delete_vector(id) < 0)
	    return 0;
    }

    return 1;
}

/*!
  \brief Load 3d raster map (volume)

  \param name 3d raster map name
  \param color_name 3d raster map for color (NULL for color_value)
  \param color_value color string (named color or RGB triptet)

  \return object id
  \return -1 on failure
*/
int Nviz::LoadVolume(const char* name, const char *color_name, const char *color_value)
{
    char *mapset;
    int id;

    mapset = G_find_grid3(name, "");
    if (mapset == NULL) {
	G_warning(_("3d raster map <%s> not found"),
		  name);
	return -1;
    }
	    
    /* topography */
    id = Nviz_new_map_obj(MAP_OBJ_VOL,
			  G_fully_qualified_name(name, mapset), 0.0,
			  data);

    if (color_name) { /* check for color map */
	mapset = G_find_grid3(color_name, "");
	if (mapset == NULL) {
	    G_warning(_("3d raster map <%s> not found"),
		      color_name);
	    GVL_delete_vol(id);
	    return -1;
	}

	Nviz_set_attr(id, MAP_OBJ_VOL, ATT_COLOR, MAP_ATT,
		      G_fully_qualified_name(color_name, mapset), -1.0,
		      data);
    }
    else if (color_value) { /* check for color value */
	Nviz_set_attr(id, MAP_OBJ_VOL, ATT_COLOR, CONST_ATT,
		      NULL, Nviz_color_from_str(color_value),
		      data);
    }
    else { /* use by default elevation map for coloring */
	Nviz_set_attr(id, MAP_OBJ_VOL, ATT_COLOR, MAP_ATT,
		      G_fully_qualified_name(name, mapset), -1.0,
		      data);
    }
    
    G_debug(1, "Nviz::LoadVolume(): name=%s -> id=%d", name, id);

    return id;
}

/*!
  \brief Unload volume

  \param id volume id

  \return 1 on success
  \return 0 on failure
*/
int Nviz::UnloadVolume(int id)
{
    if (!GVL_vol_exists(id)) {
	return 0;
    }

    G_debug(1, "Nviz::UnloadVolume(): id=%d", id);

    if (GVL_delete_vol(id) < 0)
      return 0;

    return 1;
}


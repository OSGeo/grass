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
    char *mapset;
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

    G_debug(1, "Nviz::LoadRaster(): name=%s", name);

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

    if (GS_delete_surface(id) < 0)
      return 0;

    return 1;
}

/**
   \file vector.cpp
   
   \brief Experimental C++ wxWidgets Nviz prototype -- vector mode and attributes

   Used by wxGUI Nviz extension.

   Copyright: (C) by the GRASS Development Team

   This program is free software under the GNU General Public
   License (>=v2). Read the file COPYING that comes with GRASS
   for details.

   \author Martin Landa <landa.martin gmail.com> (Google SoC 2008)

   \date 2008
*/

#include "nviz.h"

/**
   \brief Set mode of vector overlay

   \param id vector id
   \param color_str color string
   \param width line width
   \param flat
*/
int Nviz::SetVectorLineMode(int id, const char *color_str,
			    int width, int flat)
{
    int color;

    if(!GV_vect_exists(id))
	return 0;

    G_debug(1, "Nviz::SetVectorMode(): id=%d, color=%s, width=%d, flat=%d",
	    id, color_str, width, flat);


    color = Nviz_color_from_str(color_str);

    /* use memory by default */
    if (GV_set_vectmode(id, 1, color, width, flat) < 0)
	return 0;
    
    return 1;
}

/**
  \brief Set vector height above surface

  \param id vector set id
  \param height

  \return 1 on success
  \return 0 on failure
*/
int Nviz::SetVectorHeight(int id, float height)
{
    if(!GV_vect_exists(id))
	return 0;

    G_debug(1, "Nviz::SetVectorHeight(): id=%d, height=%f",
	    id, height);

    GV_set_trans(id, 0.0, 0.0, height);

    return 1;
}

/**
   \brief Set reference surface of vector set

   \param id vector set id
   \param surf_id surface id

   \return 1 on success
   \return 0 on failure
*/
int Nviz::SetVectorSurface(int id, int surf_id)
{
    if (!GS_surf_exists(surf_id) || !GV_vect_exists(id))
	return 0;

    if (GV_select_surf(id, surf_id) < 0)
	return 0;

    return 1;
}

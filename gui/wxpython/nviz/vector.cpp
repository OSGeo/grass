/**
   \file nviz/vector.cpp

   \brief wxNviz extension (3D view mode) - vector attributes
   
   This program is free software under the GNU General Public
   License (>=v2). Read the file COPYING that comes with GRASS
   for details.
   
   (C) 2008-2009 by Martin Landa, and the GRASS development team
   
   \author Martin Landa <landa.martin gmail.com> (Google SoC 2008)
*/

#include "nviz.h"

/**
   \brief Set mode of vector line overlay

   \param id vector id
   \param color_str color string
   \param width line width
   \param flat display flat or on surface

   \return -1 vector set not found
   \return -2 on failure
   \return 1 on success
*/
int Nviz::SetVectorLineMode(int id, const char *color_str,
			    int width, int flat)
{
    int color;

    if(!GV_vect_exists(id))
	return -1;

    G_debug(1, "Nviz::SetVectorMode(): id=%d, color=%s, width=%d, flat=%d",
	    id, color_str, width, flat);


    color = Nviz_color_from_str(color_str);

    /* use memory by default */
    if (GV_set_vectmode(id, 1, color, width, flat) < 0)
	return -2;
    
    return 1;
}

/**
  \brief Set vector height above surface (lines)

  \param id vector set id
  \param height

  \return -1 vector set not found
  \return 1 on success
*/
int Nviz::SetVectorLineHeight(int id, float height)
{
    if(!GV_vect_exists(id))
	return -1;

    G_debug(1, "Nviz::SetVectorLineHeight(): id=%d, height=%f",
	    id, height);

    GV_set_trans(id, 0.0, 0.0, height);

    return 1;
}

/**
   \brief Set reference surface of vector set (lines)

   \param id vector set id
   \param surf_id surface id

   \return 1 on success
   \return -1 vector set not found
   \return -2 surface not found
   \return -3 on failure
*/
int Nviz::SetVectorLineSurface(int id, int surf_id)
{
    if (!GV_vect_exists(id))
	return -1;
    
    if (!GS_surf_exists(surf_id))
	return -2;

    if (GV_select_surf(id, surf_id) < 0)
	return -3;

    return 1;
}

/**
   \brief Set mode of vector point overlay

   \param id vector id
   \param color_str color string
   \param width line width
   \param flat

   \return -1 vector set not found
*/
int Nviz::SetVectorPointMode(int id, const char *color_str,
			     int width, float size, int marker)
{
    int color;

    if(!GP_site_exists(id))
	return -1;

    G_debug(1, "Nviz::SetVectorPointMode(): id=%d, color=%s, "
	    "width=%d, size=%f, marker=%d",
	    id, color_str, width, size, marker);


    color = Nviz_color_from_str(color_str);

    if (GP_set_sitemode(id, ST_ATT_NONE,
			color, width, size, marker) < 0)
	return -2;
    
    return 1;
}

/**
  \brief Set vector height above surface (points)

  \param id vector set id
  \param height

  \return -1 vector set not found
  \return 1 on success
*/
int Nviz::SetVectorPointHeight(int id, float height)
{
    if(!GP_site_exists(id))
	return -1;

    G_debug(1, "Nviz::SetVectorPointHeight(): id=%d, height=%f",
	    id, height);

    GP_set_trans(id, 0.0, 0.0, height);

    return 1;
}

/**
   \brief Set reference surface of vector set (points)

   \param id vector set id
   \param surf_id surface id

   \return 1 on success
   \return -1 vector set not found
   \return -2 surface not found
   \return -3 on failure
*/
int Nviz::SetVectorPointSurface(int id, int surf_id)
{
    if (!GP_site_exists(id))
	return -1;
    
    if (!GS_surf_exists(surf_id))
	return -2;

    if (GP_select_surf(id, surf_id) < 0)
	return -3;

    return 1;
}

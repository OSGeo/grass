/**
   \file volume.cpp
   
   \brief Experimental C++ wxWidgets Nviz prototype -- volume attributes

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
   \brief Add new isosurface

   \param id volume id
   \param level isosurface level (topography)

   \return -1 on failure
   \return 1 on success
*/

int Nviz::AddIsosurface(int id, int level)
{
    int nisosurfs;
    
    if (!GVL_vol_exists(id))
	return -1;

    if (GVL_isosurf_add(id) < 0)
	return -1;

    /* set topography level */
    nisosurfs = GVL_isosurf_num_isosurfs(id);
    
    return GVL_isosurf_set_att_const(id, nisosurfs - 1,
				     ATT_TOPO, level);
}

/**
   \brief Delete isosurface

   \param id volume id
   \param isosurf_id isosurface id

   \return 1 on success
   \return -1 volume not found
   \return -2 isosurface not found
   \return -3 on failure
*/

int Nviz::DeleteIsosurface(int id, int isosurf_id)
{
    int ret;
    
    if (!GVL_vol_exists(id))
	return -1;

    if (isosurf_id > GVL_isosurf_num_isosurfs(id))
	return -2;
    
    ret = GVL_isosurf_del(id, isosurf_id);

    return ret < 0 ? -3 : 1;
}

/*!
  \brief Move isosurface up/down in the list

  \param id volume id
  \param isosurf_id isosurface id
  \param up if true move up otherwise down

  \return 1 on success
  \return -1 volume not found
  \return -2 isosurface not found
  \return -3 on failure
*/
int Nviz::MoveIsosurface(int id, int isosurf_id, bool up)
{
    int ret;
    
    if (!GVL_vol_exists(id))
	return -1;

    if (isosurf_id > GVL_isosurf_num_isosurfs(id))
	return -2;
    
    if (up)
	ret = GVL_isosurf_move_up(id, isosurf_id);
    else
	ret = GVL_isosurf_move_down(id, isosurf_id);

    return ret < 0 ? -3 : 1;
}

/*!
  \brief Set surface color

  \param id surface id
  \param isosurf_id isosurface id (0 - MAX_ISOSURFS)
  \param map if true use map otherwise constant
  \param value map name of value

  \return 1 on success
  \return -1 volume not found
  \return -2 isosurface not found
  \return -3 setting attributes failed
*/
int Nviz::SetIsosurfaceColor(int id, int isosurf_id,
			     bool map, const char *value)
{
    return SetIsosurfaceAttr(id, isosurf_id, ATT_COLOR, map, value);
}

/*!
  \brief Set isosurface attribute

  \param id volume id
  \param isosurf_id isosurface id (0 - MAX_ISOSURFS)
  \param attr attribute desc
  \param map if true use map otherwise constant
  \param value map name of value

  \return 1 on success
  \return -1 volume not found
  \return -2 isosurface not found
  \return -3 setting attributes failed
*/
int Nviz::SetIsosurfaceAttr(int id, int isosurf_id,
			    int attr, bool map, const char *value)
{
    int ret;

    if (!GVL_vol_exists(id)) {
	return -1;
    }
    
    if (isosurf_id > GVL_isosurf_num_isosurfs(id) - 1)
	return -2;
    
    if (map) {
	ret = GVL_isosurf_set_att_map(id, isosurf_id, attr,
				      value);
    }
    else {
	float val;
	if (attr == ATT_COLOR) {
	    val = Nviz_color_from_str(value);
	}
	else {
	    val = atof(value);
	}
	ret = GVL_isosurf_set_att_const(id, isosurf_id, attr,
					val);
    }
	
    G_debug(1, "Nviz::SetIsosurfaceAttr(): id=%d, isosurf=%d, "
	    "attr=%d, map=%d, value=%s",
	    id, isosurf_id, attr, map, value);

    return ret > 0 ? 1 : -2;
}

/*!
  \brief Unset surface attribute

  \param id surface id
  \param isosurf_id isosurface id (0 - MAX_ISOSURFS)
  \param attr attribute descriptor

  \return 1 on success
  \return -1 volume not found
  \return -2 isosurface not found
  \return -2 on failure
*/
int Nviz::UnsetIsosurfaceAttr(int id, int isosurf_id,
			      int attr)
{
    int ret;
    
    if (!GVL_vol_exists(id)) {
	return -1;
    }

    if (isosurf_id > GVL_isosurf_num_isosurfs(id) - 1)
	return -2;
    
    G_debug(1, "Nviz::UnsetSurfaceAttr(): id=%d, isosurf_id=%d, attr=%d",
	    id, isosurf_id, attr);
    
    ret = GVL_isosurf_unset_att(id, isosurf_id, attr);
    
    return ret > 0 ? 1 : -2;
}

/*!
  \brief Set draw mode for isosurfaces

  \param mode
  
  \return 1 on success
  \return -1 volume set not found
  \return -2 on failure
*/
int Nviz::SetIsosurfaceMode(int id, int mode)
{
    int ret;
    
    if (!GVL_vol_exists(id)) {
	return -1;
    }

    ret = GVL_isosurf_set_drawmode(id, mode);
    
    return ret < 0 ? -2 : 1;
}

/*!
  \brief Set draw resolution for isosurfaces

  \param res resolution value
  
  \return 1 on success
  \return -1 volume set not found
  \return -2 on failure
*/
int Nviz::SetIsosurfaceRes(int id, int res)
{
    int ret;

    if (!GVL_vol_exists(id)) {
	return -1;
    }

    ret = GVL_isosurf_set_drawres(id, res, res, res);

    return ret < 0 ? -2 : 1;
}

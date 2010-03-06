/**
   \file nviz/surface.cpp
   
   \brief wxNviz extension (3D view mode) - surface attributes
   
   This program is free software under the GNU General Public
   License (>=v2). Read the file COPYING that comes with GRASS
   for details.
   
   (C) 2008-2009 by Martin Landa, and the GRASS development team
   
   \author Martin Landa <landa.martin gmail.com> (Google SoC 2008)
*/

#include "nviz.h"

/*!
  \brief Set surface topography

  \param id surface id
  \param map if true use map otherwise constant
  \param value map name of value

  \return 1 on success
  \return -1 surface not found
  \return -2 setting attributes failed
*/
int Nviz::SetSurfaceTopo(int id, bool map, const char *value)
{
    return SetSurfaceAttr(id, ATT_TOPO, map, value);
}

/*!
  \brief Set surface color

  \param id surface id
  \param map if true use map otherwise constant
  \param value map name of value

  \return 1 on success
  \return -1 surface not found
  \return -2 setting attributes failed
*/
int Nviz::SetSurfaceColor(int id, bool map, const char *value)
{
    return SetSurfaceAttr(id, ATT_COLOR, map, value);
}

/*!
  \brief Set surface mask

  @todo invert

  \param id surface id
  \param invert if true invert mask 
  \param value map name of value

  \return 1 on success
  \return -1 surface not found
  \return -2 setting attributes failed
*/
int Nviz::SetSurfaceMask(int id, bool invert, const char *value)
{
    return SetSurfaceAttr(id, ATT_MASK, true, value);
}

/*!
  \brief Set surface mask

  @todo invert

  \param id surface id
  \param map if true use map otherwise constant
  \param value map name of value

  \return 1 on success
  \return -1 surface not found
  \return -2 setting attributes failed
*/
int Nviz::SetSurfaceTransp(int id, bool map, const char *value)
{
    return SetSurfaceAttr(id, ATT_TRANSP, map, value);
}

/*!
  \brief Set surface shininess

  \param id surface id
  \param map if true use map otherwise constant
  \param value map name of value

  \return 1 on success
  \return -1 surface not found
  \return -2 setting attributes failed
*/
int Nviz::SetSurfaceShine(int id, bool map, const char *value)
{
    return SetSurfaceAttr(id, ATT_SHINE, map, value);
}

/*!
  \brief Set surface emission

  \param id surface id
  \param map if true use map otherwise constant
  \param value map name of value

  \return 1 on success
  \return -1 surface not found
  \return -2 setting attributes failed
*/
int Nviz::SetSurfaceEmit(int id, bool map, const char *value)
{
    return SetSurfaceAttr(id, ATT_EMIT, map, value);
}

/*!
  \brief Set surface attribute

  \param id surface id
  \param attr attribute desc
  \param map if true use map otherwise constant
  \param value map name of value

  \return 1 on success
  \return -1 surface not found
  \return -2 setting attributes failed
*/
int Nviz::SetSurfaceAttr(int id, int attr, bool map, const char *value)
{
    int ret;

    if (!GS_surf_exists(id)) {
	return -1;
    }

    if (map) {
	ret = Nviz_set_attr(id, MAP_OBJ_SURF, attr, MAP_ATT,
			    value, -1.0,
			    data);
    }
    else {
	float val;
	if (attr == ATT_COLOR) {
	    val = Nviz_color_from_str(value);
	}
	else {
	    val = atof(value);
	}
	ret = Nviz_set_attr(id, MAP_OBJ_SURF, attr, CONST_ATT,
			    NULL, val,
			    data);
    }
	
    G_debug(1, "Nviz::SetSurfaceAttr(): id=%d, attr=%d, map=%d, value=%s",
	    id, attr, map, value);

    return ret ? 1 : -2;
}

/*!
  \brief Unset surface mask

  \param id surface id

  \return 1 on success
  \return -1 surface not found
  \return -2 setting attributes failed
  \return -1 on failure
*/

int Nviz::UnsetSurfaceMask(int id)
{
    return UnsetSurfaceAttr(id, ATT_MASK);
}

/*!
  \brief Unset surface transparency

  \param id surface id

  \return 1 on success
  \return -1 surface not found
  \return -2 setting attributes failed
*/

int Nviz::UnsetSurfaceTransp(int id)
{
    return UnsetSurfaceAttr(id, ATT_TRANSP);
}

/*!
  \brief Unset surface emission

  \param id surface id

  \return 1 on success
  \return -1 surface not found
  \return -2 setting attributes failed
*/

int Nviz::UnsetSurfaceEmit(int id)
{
    return UnsetSurfaceAttr(id, ATT_EMIT);
}

/*!
  \brief Unset surface attribute

  \param id surface id
  \param attr attribute descriptor

  \return 1 on success
  \return -1 surface not found
  \return -2 setting attributes failed
*/
int Nviz::UnsetSurfaceAttr(int id, int attr)
{
    int ret;
    
    if (!GS_surf_exists(id)) {
	return -1;
    }

    G_debug(1, "Nviz::UnsetSurfaceAttr(): id=%d, attr=%d",
	    id, attr);
    
    ret = Nviz_unset_attr(id, MAP_OBJ_SURF, attr);
    
    return ret ? 1 : -2;
}

/*!
  \brief Set surface resolution

  \param id surface id
  \param fine x/y fine resolution
  \param coarse x/y coarse resolution

  \return 1 on success
  \return -1 surface not found
  \return -2 setting attributes failed
*/
int Nviz::SetSurfaceRes(int id, int fine, int coarse)
{
    G_debug(1, "Nviz::SetSurfaceRes(): id=%d, fine=%d, coarse=%d",
	    id, fine, coarse);


    if (id > 0) {
	if (!GS_surf_exists(id)) {
	    return -1;
	}

	if (GS_set_drawres(id, fine, fine, coarse, coarse) < 0) {
	    return -2;
	}
    }
    else {
	GS_setall_drawres(fine, fine, coarse, coarse);
    }

    return 1;
}

/*!
  \brief Set draw style

  Draw styles:
   - DM_GOURAUD
   - DM_FLAT
   - DM_FRINGE
   - DM_WIRE
   - DM_COL_WIRE
   - DM_POLY
   - DM_WIRE_POLY
   - DM_GRID_WIRE
   - DM_GRID_SURF

  \param id surface id (<= 0 for all)
  \param style draw style

  \return 1 on success
  \return -1 surface not found
  \return -2 setting attributes failed
*/
int Nviz::SetSurfaceStyle(int id, int style)
{
    G_debug(1, "Nviz::SetSurfaceStyle(): id=%d, style=%d",
	    id, style);

    if (id > 0) {
	if (!GS_surf_exists(id)) {
	    return -1;
	}
	
	if (GS_set_drawmode(id, style) < 0) {
	    return -2;
	}
	return 1;
    }

    if (GS_setall_drawmode(style) < 0) {
	return -2;
    }

    return 1;
}

/*!
  \brief Set color of wire

  \todo all

  \param surface id (< 0 for all)
  \param color color string (R:G:B)

  \return 1 on success
  \return -1 surface not found
  \return -2 setting attributes failed
  \return 1 on success
  \return 0 on failure
*/
int Nviz::SetWireColor(int id, const char* color_str)
{
    int color;

    G_debug(1, "Nviz::SetWireColor(): id=%d, color=%s",
	    id, color_str);

    color = Nviz_color_from_str(color_str);

    if (id > 0) {
	if (!GS_surf_exists(id)) {
	    return -1;
	}
	GS_set_wire_color(id, color);
    }
    else {
	int *surf_list, nsurfs, id;

	surf_list = GS_get_surf_list(&nsurfs);
	for (int i = 0; i < nsurfs; i++) {
	    id = surf_list[i];
	    GS_set_wire_color(id, color);
	}

	G_free(surf_list);
	surf_list = NULL;
    }

    return 1;
}

/*!
  \brief Get surface position

  \param id surface id

  \return x,y,z
  \return zero-length vector on error
*/
std::vector<double> Nviz::GetSurfacePosition(int id)
{
    std::vector<double> vals;
    float x, y, z;
    if (!GS_surf_exists(id)) {
	return vals;
    }
    
    GS_get_trans(id, &x, &y, &z);
    
    G_debug(1, "Nviz::GetSurfacePosition(): id=%d, x=%f, y=%f, z=%f",
	    id, x, y, z);


    vals.push_back(double (x));
    vals.push_back(double (y));
    vals.push_back(double (z));

    return vals;
}

/*!
  \brief Set surface position

  \param id surface id
  \param x,y,z translation values

  \return 1 on success
  \return -1 surface not found
  \return -2 setting position failed
*/
int Nviz::SetSurfacePosition(int id, float x, float y, float z)
{
    if (!GS_surf_exists(id)) {
	return -1;
    }
    
    G_debug(1, "Nviz::SetSurfacePosition(): id=%d, x=%f, y=%f, z=%f",
	    id, x, y, z);

    GS_set_trans(id, x, y, z);

    return 1;
}


from ctypes import *
from grass.lib.grass import *
from grass.lib.ogsf import *
from grass.lib.nviz import *

def G_debug(level, str, *args):
    print str, args

class Nviz(object):
#!
# \brief Initialize Nviz class instance

    def __init__(self, log):
        G_gisinit("")       # GRASS functions

        # logStream = log
        # G_set_error_routine(&print_error)
        # G_set_percent_routine(&print_percent)

        GS_libinit()
        GVL_libinit()

        # GS_set_swap_func(swap_gl)

        self.data_obj = nv_data()
        self.data = pointer(self.data_obj)

        G_debug(1, "Nviz::Nviz()")

#!
# \brief Destroy Nviz class instance

    def __del__(self):
        # G_unset_error_routine()
        # G_unset_percent_routine()
        del self.data
        del self.data_obj
        # logStream = None

#!
# \brief GL canvas resized
# 
# \param width window width
# \param height window height
# 
# \return 1 on success
# \return 0 on failure (window resized by default to 20x20 px)

    def ResizeWindow(self, width, height):
        G_debug(1, "Nviz::ResizeWindow(): width=%d height=%d",
                width, height)
        return Nviz_resize_window(width, height)

#!
# \brief Set default view (based on loaded data)
#
# \return z-exag value, default, min and max height

    def SetViewDefault(self):
        # determine z-exag
        z_exag = Nviz_get_exag()
        Nviz_change_exag(self.data, z_exag)

        # determine height
        hdef = c_float()
        hmin = c_float()
        hmax = c_float()
        Nviz_get_exag_height(byref(hdef), byref(hmin), byref(hmax))

        G_debug(1, "Nviz::SetViewDefault(): hdef=%f, hmin=%f, hmax=%f",
                hdef, hmin, hmax)

        return (z_exag, hdef.value, hmin.value, hmax.value)

#!
# \brief Change view settings
#
# \param x,y position
# \param height
# \param persp perpective
# \param twist
#
# \return 1 on success

    def SetView(self, x, y, height, persp, twist):
        Nviz_set_viewpoint_height(self.data, height)
        Nviz_set_viewpoint_position(self.data, x, y)
        Nviz_set_viewpoint_twist(self.data, twist)
        Nviz_set_viewpoint_persp(self.data, persp)

        G_debug(1, "Nviz::SetView(): x=%f, y=%f, height=%f, persp=%f, twist=%f",
                x, y, height, persp, twist)

        return 1

#!
# \brief Set z-exag value
# 
# \param z_exag value
# 
# \return 1

    def SetZExag(self, z_exag):
        G_debug(1, "Nviz::SetZExag(): z_exag=%f", z_exag)
        return Nviz_change_exag(self.data, z_exag)

#!
# \brief Draw map
# 
# Draw quick mode:
#  - DRAW_QUICK_SURFACE
#  - DRAW_QUICK_VLINES
#  - DRAW_QUICK_VPOINTS
#  - DRAW_QUICK_VOLUME
#  
# \param quick if true draw in wiremode
# \param quick_mode quick mode

    def Draw(self, quick, quick_mode):
        Nviz_draw_cplane(self.data, -1, -1) # ?

        if quick:
            Nviz_draw_quick(self.data, quick_mode)
        else:
            Nviz_draw_all(self.data)

        G_debug(1, "Nviz::Draw(): quick=%d", quick)

#!
# \brief Erase map display (with background color)

    def EraseMap(self):
        GS_clear(self.data.bgcolor)
        G_debug(1, "Nviz::EraseMap()")

    def InitView(self):
        # initialize nviz data
        Nviz_init_data(self.data)

        # define default attributes for map objects
        Nviz_set_surface_attr_default()
        # set background color
        Nviz_set_bgcolor(self.data, Nviz_color_from_str("white")) # TODO

        # initialize view
        Nviz_init_view()

        # set default lighting model
        self.SetLightsDefault()

        # clear window
        GS_clear(self.data.bgcolor)

        G_debug(1, "Nviz::InitView()")

#!
# \brief Set background color
# 
# \param color_str color string

    def SetBgColor(self, color_str):
        self.data.bgcolor = Nviz_color_from_str(color_str)

#!
# \brief Set default lighting model

    def SetLightsDefault(self):
        # first
        Nviz_set_light_position(self.data, 0, 0.68, -0.68, 0.80, 0.0)
        Nviz_set_light_bright(self.data, 0, 0.8)
        Nviz_set_light_color(self.data, 0, 1.0, 1.0, 1.0)
        Nviz_set_light_ambient(self.data, 0, 0.2, 0.2, 0.2)

        # second
        Nviz_set_light_position(self.data, 1, 0.0, 0.0, 1.0, 0.0)
        Nviz_set_light_bright(self.data, 1, 0.5)
        Nviz_set_light_color(self.data, 1, 1.0, 1.0, 1.0)
        Nviz_set_light_ambient(self.data, 1, 0.3, 0.3, 0.3)

        G_debug(1, "Nviz::SetLightsDefault()")

#!
# \brief Load raster map (surface)
# 
# \param name raster map name
# \param color_name raster map for color (NULL for color_value)
# \param color_value color string (named color or RGB triptet)
# 
# \return object id
# \return -1 on failure

    def LoadSurface(self, name, color_name, color_value):
        mapset = G_find_raster2(name, "")
        if mapset is None:
            G_warning(_("Raster map <%s> not found"), name)
            return -1

        # topography
        id = Nviz_new_map_obj(MAP_OBJ_SURF,
                              G_fully_qualified_name(name, mapset), 0.0,
                              self.data)

        if color_name:      # check for color map
            mapset = G_find_raster2(color_name, "")
            if mapset is None:
                G_warning(_("Raster map <%s> not found"), color_name)
                GS_delete_surface(id)
                return -1


            Nviz_set_attr(id, MAP_OBJ_SURF, ATT_COLOR, MAP_ATT,
                          G_fully_qualified_name(color_name, mapset), -1.0,
                          self.data)

        elif color_value:   # check for color value
            Nviz_set_attr(id, MAP_OBJ_SURF, ATT_COLOR, CONST_ATT,
                          None, Nviz_color_from_str(color_value),
                          self.data)

        else:               # use by default elevation map for coloring
            Nviz_set_attr(id, MAP_OBJ_SURF, ATT_COLOR, MAP_ATT,
                          G_fully_qualified_name(name, mapset), -1.0,
                          self.data)

        # if (i > 1)
        #     set_default_wirecolors(self.data, i)


        # focus on loaded self.data
        Nviz_set_focus_map(MAP_OBJ_UNDEFINED, -1)

        G_debug(1, "Nviz::LoadRaster(): name=%s -> id=%d", name, id)

        return id


#!
# \brief Unload surface
# 
# \param id surface id
# 
# \return 1 on success
# \return 0 on failure

    def UnloadSurface(self, id):
        if not GS_surf_exists(id):
            return 0

        G_debug(1, "Nviz::UnloadSurface(): id=%d", id)

        if GS_delete_surface(id) < 0:
            return 0

        return 1

#!
# \brief Load vector map overlay
# 
# \param name vector map name
# \param points if true load 2d points rather then 2d lines
# 
# \return object id
# \return -1 on failure

    def LoadVector(self, name, points):
        if GS_num_surfs() == 0:     # load base surface if no loaded
            Nviz_new_map_obj(MAP_OBJ_SURF, NULL, 0.0, self.data)

            nsurf = c_int()
            surf_list = GS_get_surf_list(byref(nsurf))
            GS_set_att_const(surf_list[0], ATT_TRANSP, 255)

        mapset = G_find_vector2 (name, "")
        if mapset is None:
            G_warning(_("Vector map <%s> not found"),
                      name)

        if points:
            id = Nviz_new_map_obj(MAP_OBJ_SITE,
                                  G_fully_qualified_name(name, mapset), 0.0,
                                  self.data)
        else:
            id = Nviz_new_map_obj(MAP_OBJ_VECT,
                                  G_fully_qualified_name(name, mapset), 0.0,
                                  self.data)

        G_debug(1, "Nviz::LoadVector(): name=%s -> id=%d", name, id)

        return id

#!
# \brief Unload vector set
# 
# \param id vector set id
# \param points vector points or lines set
# 
# \return 1 on success
# \return 0 on failure

    def UnloadVector(self, id, points):
        G_debug(1, "Nviz::UnloadVector(): id=%d", id)

        if points:
            if not GP_site_exists(id):
                return 0
            if GP_delete_site(id) < 0:
                return 0
        else:
            if not GV_vect_exists(id):
                return 0
            if GV_delete_vector(id) < 0:
                return 0

        return 1

#!
# \brief Load 3d raster map (volume)
# 
# \param name 3d raster map name
# \param color_name 3d raster map for color (NULL for color_value)
# \param color_value color string (named color or RGB triptet)
# 
# \return object id
# \return -1 on failure

    def LoadVolume(self, name, color_name, color_value):
        mapset = G_find_grid3(name, "")
        if mapset is None:
            G_warning(_("3d raster map <%s> not found"),
                      name)
            return -1

        # topography
        id = Nviz_new_map_obj(MAP_OBJ_VOL,
                              G_fully_qualified_name(name, mapset), 0.0,
                              self.data)

        if color_name:      # check for color map
            mapset = G_find_grid3(color_name, "")
            if mapset is None:
                G_warning(_("3d raster map <%s> not found"),
                          color_name)
                GVL_delete_vol(id)
                return -1

            Nviz_set_attr(id, MAP_OBJ_VOL, ATT_COLOR, MAP_ATT,
                          G_fully_qualified_name(color_name, mapset), -1.0,
                          self.data)
        elif color_value:   # check for color value
            Nviz_set_attr(id, MAP_OBJ_VOL, ATT_COLOR, CONST_ATT,
                          NULL, Nviz_color_from_str(color_value),
                          self.data)
        else:               # use by default elevation map for coloring
            Nviz_set_attr(id, MAP_OBJ_VOL, ATT_COLOR, MAP_ATT,
                          G_fully_qualified_name(name, mapset), -1.0,
                          self.data)

        G_debug(1, "Nviz::LoadVolume(): name=%s -> id=%d", name, id)

        return id

#!
# \brief Unload volume
# 
# \param id volume id
# 
# \return 1 on success
# \return 0 on failure

    def UnloadVolume(self, id):
        if not GVL_vol_exists(id):
            return 0

        G_debug(1, "Nviz::UnloadVolume(): id=%d", id)

        if GVL_delete_vol(id) < 0:
          return 0

        return 1

#!
# \brief Set surface topography
# 
# \param id surface id
# \param map if true use map otherwise constant
# \param value map name of value
# 
# \return 1 on success
# \return -1 surface not found
# \return -2 setting attributes failed

    def SetSurfaceTopo(self, id, map, value):
        return self.SetSurfaceAttr(id, ATT_TOPO, map, value)

#!
# \brief Set surface color
# 
# \param id surface id
# \param map if true use map otherwise constant
# \param value map name of value
# 
# \return 1 on success
# \return -1 surface not found
# \return -2 setting attributes failed

    def SetSurfaceColor(self, id, map, value):
        return self.SetSurfaceAttr(id, ATT_COLOR, map, value)

#!
# \brief Set surface mask
# 
# @todo invert
# 
# \param id surface id
# \param invert if true invert mask 
# \param value map name of value
# 
# \return 1 on success
# \return -1 surface not found
# \return -2 setting attributes failed

    def SetSurfaceMask(self, id, invert, value):
        return self.SetSurfaceAttr(id, ATT_MASK, true, value)

#!
# \brief Set surface mask
# 
# @todo invert
# 
# \param id surface id
# \param map if true use map otherwise constant
# \param value map name of value
# 
# \return 1 on success
# \return -1 surface not found
# \return -2 setting attributes failed

    def SetSurfaceTransp(self, id, map, value):
        return self.SetSurfaceAttr(id, ATT_TRANSP, map, value)

#!
# \brief Set surface shininess
# 
# \param id surface id
# \param map if true use map otherwise constant
# \param value map name of value
# 
# \return 1 on success
# \return -1 surface not found
# \return -2 setting attributes failed

    def SetSurfaceShine(self, id, map, value):
        return self.SetSurfaceAttr(id, ATT_SHINE, map, value)

#!
# \brief Set surface emission
# 
# \param id surface id
# \param map if true use map otherwise constant
# \param value map name of value
# 
# \return 1 on success
# \return -1 surface not found
# \return -2 setting attributes failed

    def SetSurfaceEmit(self, id, map, value):
        return self.SetSurfaceAttr(id, ATT_EMIT, map, value)

#!
# \brief Set surface attribute
# 
# \param id surface id
# \param attr attribute desc
# \param map if true use map otherwise constant
# \param value map name of value
# 
# \return 1 on success
# \return -1 surface not found
# \return -2 setting attributes failed

    def SetSurfaceAttr(self, id, attr, map, value):
        if not GS_surf_exists(id):
            return -1

        if map:
            ret = Nviz_set_attr(id, MAP_OBJ_SURF, attr, MAP_ATT,
                                value, -1.0, self.data)
        else:
            if attr == ATT_COLOR:
                val = Nviz_color_from_str(value)
            else:
                val = atof(value)

            ret = Nviz_set_attr(id, MAP_OBJ_SURF, attr, CONST_ATT,
                                NULL, val, self.data)

        G_debug(1, "Nviz::SetSurfaceAttr(): id=%d, attr=%d, map=%d, value=%s",
                id, attr, map, value)

        return 1 if ret else -2

#!
# \brief Unset surface mask
# 
# \param id surface id
# 
# \return 1 on success
# \return -1 surface not found
# \return -2 setting attributes failed
# \return -1 on failure

    def UnsetSurfaceMask(self, id):
        return self.UnsetSurfaceAttr(id, ATT_MASK)

#!
# \brief Unset surface transparency
# 
# \param id surface id
# 
# \return 1 on success
# \return -1 surface not found
# \return -2 setting attributes failed

    def UnsetSurfaceTransp(self, id):
        return self.UnsetSurfaceAttr(id, ATT_TRANSP)

#!
# \brief Unset surface emission
# 
# \param id surface id
# 
# \return 1 on success
# \return -1 surface not found
# \return -2 setting attributes failed

    def UnsetSurfaceEmit(self, id):
        return self.UnsetSurfaceAttr(id, ATT_EMIT)

#!
# \brief Unset surface attribute
# 
# \param id surface id
# \param attr attribute descriptor
# 
# \return 1 on success
# \return -1 surface not found
# \return -2 setting attributes failed

    def UnsetSurfaceAttr(self, id, attr):
        if not GS_surf_exists(id):
            return -1

        G_debug(1, "Nviz::UnsetSurfaceAttr(): id=%d, attr=%d",
                id, attr)

        ret = Nviz_unset_attr(id, MAP_OBJ_SURF, attr)

        return 1 if ret else -2

#!
# \brief Set surface resolution
# 
# \param id surface id
# \param fine x/y fine resolution
# \param coarse x/y coarse resolution
# 
# \return 1 on success
# \return -1 surface not found
# \return -2 setting attributes failed

    def SetSurfaceRes(self, id, fine, coarse):
        G_debug(1, "Nviz::SetSurfaceRes(): id=%d, fine=%d, coarse=%d",
                id, fine, coarse)

        if id > 0:
            if not GS_surf_exists(id):
                return -1

            if GS_set_drawres(id, fine, fine, coarse, coarse) < 0:
                return -2
        else:
            GS_setall_drawres(fine, fine, coarse, coarse)

        return 1

#!
# \brief Set draw style
# 
# Draw styles:
#  - DM_GOURAUD
#  - DM_FLAT
#  - DM_FRINGE
#  - DM_WIRE
#  - DM_COL_WIRE
#  - DM_POLY
#  - DM_WIRE_POLY
#  - DM_GRID_WIRE
#  - DM_GRID_SURF
# 
# \param id surface id (<= 0 for all)
# \param style draw style
# 
# \return 1 on success
# \return -1 surface not found
# \return -2 setting attributes failed

    def SetSurfaceStyle(self, id, style):
        G_debug(1, "Nviz::SetSurfaceStyle(): id=%d, style=%d",
                id, style)

        if id > 0:
            if not GS_surf_exists(id):
                return -1

            if GS_set_drawmode(id, style) < 0:
                return -2

            return 1

        if GS_setall_drawmode(style) < 0:
            return -2

        return 1

#!
# \brief Set color of wire
# 
# \todo all
# 
# \param surface id (< 0 for all)
# \param color color string (R:G:B)
# 
# \return 1 on success
# \return -1 surface not found
# \return -2 setting attributes failed
# \return 1 on success
# \return 0 on failure

    def SetWireColor(self, id, color_str):
        G_debug(1, "Nviz::SetWireColor(): id=%d, color=%s",
                id, color_str)

        color = Nviz_color_from_str(color_str)

        if id > 0:
            if not GS_surf_exists(id):
                return -1

            GS_set_wire_color(id, color)
        else:
            nsurfs = c_int()
            surf_list = GS_get_surf_list(byref(nsurfs))
            for i in xrange(nsurfs.value):
                id = surf_list[i]
                GS_set_wire_color(id, color)

            G_free(surf_list)
            surf_list = None

        return 1

#!
# \brief Get surface position
# 
# \param id surface id
# 
# \return x,y,z
# \return zero-length vector on error

    def GetSurfacePosition(self, id):
        if not GS_surf_exists(id):
            return []

        x, y, z = c_float(), c_float(), c_float()
        GS_get_trans(id, byref(x), byref(y), byref(z))

        G_debug(1, "Nviz::GetSurfacePosition(): id=%d, x=%f, y=%f, z=%f",
                id, x, y, z)

        return [x.value, y.value, z.value]

#!
# \brief Set surface position
# 
# \param id surface id
# \param x,y,z translation values
# 
# \return 1 on success
# \return -1 surface not found
# \return -2 setting position failed

    def SetSurfacePosition(self, id, x, y, z):
        if not GS_surf_exists(id):
            return -1

        G_debug(1, "Nviz::SetSurfacePosition(): id=%d, x=%f, y=%f, z=%f",
                id, x, y, z)

        GS_set_trans(id, x, y, z)

        return 1

#!
# \brief Set mode of vector line overlay
#
# \param id vector id
# \param color_str color string
# \param width line width
# \param flat display flat or on surface
#
# \return -1 vector set not found
# \return -2 on failure
# \return 1 on success

    def SetVectorLineMode(self, id, color_str, width, flat):
        if not GV_vect_exists(id):
            return -1

        G_debug(1, "Nviz::SetVectorMode(): id=%d, color=%s, width=%d, flat=%d",
                id, color_str, width, flat)


        color = Nviz_color_from_str(color_str)

        # use memory by default
        if GV_set_vectmode(id, 1, color, width, flat) < 0:
            return -2

        return 1

#!
# \brief Set vector height above surface (lines)
# 
# \param id vector set id
# \param height
# 
# \return -1 vector set not found
# \return 1 on success

    def SetVectorLineHeight(self, id, height):
        if not GV_vect_exists(id):
            return -1

        G_debug(1, "Nviz::SetVectorLineHeight(): id=%d, height=%f",
                id, height)

        GV_set_trans(id, 0.0, 0.0, height)

        return 1

#!
# \brief Set reference surface of vector set (lines)
#
# \param id vector set id
# \param surf_id surface id
#
# \return 1 on success
# \return -1 vector set not found
# \return -2 surface not found
# \return -3 on failure

    def SetVectorLineSurface(self, id, surf_id):
        if not GV_vect_exists(id):
            return -1

        if not GS_surf_exists(surf_id):
            return -2

        if GV_select_surf(id, surf_id) < 0:
            return -3

        return 1

#!
# \brief Set mode of vector point overlay
#
# \param id vector id
# \param color_str color string
# \param width line width
# \param flat
#
# \return -1 vector set not found

    def SetVectorPointMode(self, id, color_str, width, size, marker):
        if not GP_site_exists(id):
            return -1

        G_debug(1, "Nviz::SetVectorPointMode(): id=%d, color=%s, "
                "width=%d, size=%f, marker=%d",
                id, color_str, width, size, marker)

        color = Nviz_color_from_str(color_str)

        if GP_set_style(id, color, width, size, marker) < 0:
            return -2

        return 1

#!
# \brief Set vector height above surface (points)
# 
# \param id vector set id
# \param height
# 
# \return -1 vector set not found
# \return 1 on success

    def SetVectorPointHeight(self, id, height):
        if not GP_site_exists(id):
            return -1

        G_debug(1, "Nviz::SetVectorPointHeight(): id=%d, height=%f",
                id, height)

        GP_set_trans(id, 0.0, 0.0, height)

        return 1

#!
# \brief Set reference surface of vector set (points)
#
# \param id vector set id
# \param surf_id surface id
#
# \return 1 on success
# \return -1 vector set not found
# \return -2 surface not found
# \return -3 on failure

    def SetVectorPointSurface(self, id, surf_id):
        if not GP_site_exists(id):
            return -1

        if not GS_surf_exists(surf_id):
            return -2

        if GP_select_surf(id, surf_id) < 0:
            return -3

        return 1

#!
# \brief Add new isosurface
#
# \param id volume id
# \param level isosurface level (topography)
#
# \return -1 on failure
# \return 1 on success

    def AddIsosurface(self, id, level):
        if not GVL_vol_exists(id):
            return -1

        if GVL_isosurf_add(id) < 0:
            return -1

        # set topography level
        nisosurfs = GVL_isosurf_num_isosurfs(id)

        return GVL_isosurf_set_att_const(id, nisosurfs - 1, ATT_TOPO, level)

#!
# \brief Delete isosurface
#
# \param id volume id
# \param isosurf_id isosurface id
#
# \return 1 on success
# \return -1 volume not found
# \return -2 isosurface not found
# \return -3 on failure


    def DeleteIsosurface(self, id, isosurf_id):
        if not GVL_vol_exists(id):
            return -1

        if isosurf_id > GVL_isosurf_num_isosurfs(id):
            return -2

        ret = GVL_isosurf_del(id, isosurf_id)

        return -3 if ret < 0 else 1

#!
# \brief Move isosurface up/down in the list
# 
# \param id volume id
# \param isosurf_id isosurface id
# \param up if true move up otherwise down
# 
# \return 1 on success
# \return -1 volume not found
# \return -2 isosurface not found
# \return -3 on failure

    def MoveIsosurface(self, id, isosurf_id, up):
        if not GVL_vol_exists(id):
            return -1

        if isosurf_id > GVL_isosurf_num_isosurfs(id):
            return -2

        if up:
            ret = GVL_isosurf_move_up(id, isosurf_id)
        else:
            ret = GVL_isosurf_move_down(id, isosurf_id)

        return -3 if ret < 0 else 1

#!
# \brief Set isosurface color
# 
# \param id volume id
# \param isosurf_id isosurface id (0 - MAX_ISOSURFS)
# \param map if true use map otherwise constant
# \param value map name of value
# 
# \return 1 on success
# \return -1 volume not found
# \return -2 isosurface not found
# \return -3 on failure

    def SetIsosurfaceColor(self, id, isosurf_id, map, value):
        return self.SetIsosurfaceAttr(id, isosurf_id, ATT_COLOR, map, value)

#!
# \brief Set isosurface mask
# 
# @todo invert
# 
# \param id volume id
# \param isosurf_id isosurface id (0 - MAX_ISOSURFS)
# \param invert true for invert mask
# \param value map name to be used for mask
# 
# \return 1 on success
# \return -1 volume not found
# \return -2 isosurface not found
# \return -3 on failure

    def SetIsosurfaceMask(self, id, isosurf_id, invert, value):
        return self.SetIsosurfaceAttr(id, isosurf_id, ATT_MASK, true, value)

#!
# \brief Set isosurface transparency
# 
# \param id volume id
# \param isosurf_id isosurface id (0 - MAX_ISOSURFS)
# \param map if true use map otherwise constant
# \param value map name of value
# 
# \return 1 on success
# \return -1 volume not found
# \return -2 isosurface not found
# \return -3 on failure

    def SetIsosurfaceTransp(self, id, isosurf_id, map, value):
        return self.SetIsosurfaceAttr(id, isosurf_id, ATT_TRANSP, map, value)

#!
# \brief Set isosurface shininess
# 
# \param id volume id
# \param isosurf_id isosurface id (0 - MAX_ISOSURFS)
# \param map if true use map otherwise constant
# \param value map name of value
# 
# \return 1 on success
# \return -1 volume not found
# \return -2 isosurface not found
# \return -3 on failure

    def SetIsosurfaceShine(self, id, isosurf_id, map, value):
        return self.SetIsosurfaceAttr(id, isosurf_id, ATT_SHINE, map, value)

#!
# \brief Set isosurface emission
# 
# \param id volume id
# \param isosurf_id isosurface id (0 - MAX_ISOSURFS)
# \param map if true use map otherwise constant
# \param value map name of value
# 
# \return 1 on success
# \return -1 volume not found
# \return -2 isosurface not found
# \return -3 on failure

    def SetIsosurfaceEmit(self, id, isosurf_id, map, value):
        return self.SetIsosurfaceAttr(id, isosurf_id, ATT_EMIT, map, value)

#!
# \brief Set isosurface attribute
# 
# \param id volume id
# \param isosurf_id isosurface id (0 - MAX_ISOSURFS)
# \param attr attribute desc
# \param map if true use map otherwise constant
# \param value map name of value
# 
# \return 1 on success
# \return -1 volume not found
# \return -2 isosurface not found
# \return -3 setting attributes failed

    def SetIsosurfaceAttr(self, id, isosurf_id, attr, map, value):
        if not GVL_vol_exists(id):
            return -1

        if isosurf_id > GVL_isosurf_num_isosurfs(id) - 1:
            return -2

        if map:
            ret = GVL_isosurf_set_att_map(id, isosurf_id, attr, value)
        else:
            if attr == ATT_COLOR:
                val = Nviz_color_from_str(value)
            else:
                val = float(value)

            ret = GVL_isosurf_set_att_const(id, isosurf_id, attr, val)

        G_debug(1, "Nviz::SetIsosurfaceAttr(): id=%d, isosurf=%d, "
                "attr=%d, map=%d, value=%s",
                id, isosurf_id, attr, map, value)

        return 1 if ret > 0 else -2

#!
# \brief Unset isosurface mask
# 
# \param id volume id
# \param isosurf_id isosurface id (0 - MAX_ISOSURFS)
# 
# \return 1 on success
# \return -1 volume not found
# \return -2 isosurface not found
# \return -3 setting attributes failed

    def UnsetIsosurfaceMask(self, id, isosurf_id):
        return self.UnsetIsosurfaceAttr(id, isosurf_id, ATT_MASK)

#!
# \brief Unset isosurface transparency
# 
# \param id volume id
# \param isosurf_id isosurface id (0 - MAX_ISOSURFS)
# 
# \return 1 on success
# \return -1 volume not found
# \return -2 isosurface not found
# \return -3 setting attributes failed

    def UnsetIsosurfaceTransp(self, id, isosurf_id):
        return self.UnsetIsosurfaceAttr(id, isosurf_id, ATT_TRANSP)

#!
# \brief Unset isosurface emission
# 
# \param id volume id
# \param isosurf_id isosurface id (0 - MAX_ISOSURFS)
# 
# \return 1 on success
# \return -1 volume not found
# \return -2 isosurface not found
# \return -3 setting attributes failed

    def UnsetIsosurfaceEmit(self, id, isosurf_id):
        return self.UnsetIsosurfaceAttr(id, isosurf_id, ATT_EMIT)

#!
# \brief Unset surface attribute
# 
# \param id surface id
# \param isosurf_id isosurface id (0 - MAX_ISOSURFS)
# \param attr attribute descriptor
# 
# \return 1 on success
# \return -1 volume not found
# \return -2 isosurface not found
# \return -2 on failure

    def UnsetIsosurfaceAttr(self, id, isosurf_id, attr):
        if not GVL_vol_exists(id):
            return -1

        if isosurf_id > GVL_isosurf_num_isosurfs(id) - 1:
            return -2

        G_debug(1, "Nviz::UnsetSurfaceAttr(): id=%d, isosurf_id=%d, attr=%d",
                id, isosurf_id, attr)

        ret = GVL_isosurf_unset_att(id, isosurf_id, attr)

        return 1 if ret > 0 else -2

#!
# \brief Set draw mode for isosurfaces
# 
# \param mode
# 
# \return 1 on success
# \return -1 volume set not found
# \return -2 on failure

    def SetIsosurfaceMode(self, id, mode):
        if not GVL_vol_exists(id):
            return -1

        ret = GVL_isosurf_set_drawmode(id, mode)

        return -2 if ret < 0 else 1

#!
# \brief Set draw resolution for isosurfaces
# 
# \param res resolution value
# 
# \return 1 on success
# \return -1 volume set not found
# \return -2 on failure

    def SetIsosurfaceRes(self, id, res):
        if not GVL_vol_exists(id):
            return -1

        ret = GVL_isosurf_set_drawres(id, res, res, res)

        return -2 if ret < 0 else 1

"""!
@package nviz.wxnviz

@brief wxGUI 3D view mode (ctypes-based classes)

This module implements 3D visualization mode for map display (ctypes
required).

List of classes:
 - wxnviz::Nviz
 - wxnviz::Texture
 - wxnviz::ImageTexture
 - wxnviz::TextTexture

(C) 2008-2011 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com> (Google SoC 2008/2010)
@author Pythonized by Glynn Clements
@author Anna Kratochvilova <KratochAnna seznam.cz> (Google SoC 2011)
"""

import sys
import locale
import struct
from math  import sqrt
try:
    from numpy import matrix
except ImportError:
    msg = _("This module requires the NumPy module, which could not be "
            "imported. It probably is not installed (it's not part of the "
            "standard Python distribution). See the Numeric Python site "
            "(http://numpy.scipy.org) for information on downloading source or "
            "binaries.")
    print >> sys.stderr, "wxnviz.py: " + msg

import wx

from ctypes import *

from grass.lib.gis      import *
from grass.lib.raster3d import *
from grass.lib.vector   import *
from grass.lib.ogsf     import *
from grass.lib.nviz     import *
from grass.lib.raster   import *

from core.debug import Debug
import grass.script as grass

log      = None
progress = None

def print_error(msg, type):
    """!Redirect stderr"""
    global log
    if log:
        log.write(msg)
    else:
        print msg
    
    return 0

def print_progress(value):
    """!Redirect progress info"""
    global progress
    if progress:
        progress.SetValue(value)
    else:
        print value
    
    return 0

try:
    errtype = CFUNCTYPE(UNCHECKED(c_int), String, c_int)
    errfunc = errtype(print_error)
    pertype = CFUNCTYPE(UNCHECKED(c_int), c_int)
    perfunc = pertype(print_progress)
except NameError:
    pass

class Nviz(object):
    def __init__(self, glog, gprogress):
        """!Initialize Nviz class instance
        
        @param glog logging area
        @param gprogress progressbar
        """
        global errfunc, perfunc, log, progress
        log = glog
        progress = gprogress
        
        G_gisinit("wxnviz")
        # gislib is already initialized (where?)
        G_set_error_routine(errfunc) 
        G_set_percent_routine(perfunc)
        
        self.Init()
        
        self.data_obj = nv_data()
        self.data = pointer(self.data_obj)
        self.color_obj = Colors()
        self.color = pointer(self.color_obj)
        
        self.width = self.height = -1
        self.showLight = False
        
        Debug.msg(1, "Nviz::Nviz()")
        
    def __del__(self):
        """!Destroy Nviz class instance"""
        G_unset_error_routine()
        G_unset_percent_routine()
        del self.data
        del self.data_obj
        self.log = None

    def Init(self):
        """!Initialize window"""
        locale.setlocale(locale.LC_NUMERIC, 'C')
        G_unset_window()
        Rast_unset_window()
        Rast__init_window()
        GS_libinit()
        GVL_libinit()
        GVL_init_region()
    
    def ResizeWindow(self, width, height):
        """!GL canvas resized
        
        @param width window width
        @param height window height
        
        @return 1 on success
        @return 0 on failure (window resized by default to 20x20 px)
        """
        self.width  = width
        self.height = height
        Debug.msg(3, "Nviz::ResizeWindow(): width=%d height=%d",
                  width, height)
        return Nviz_resize_window(width, height)
    
    def GetLongDim(self):
        """!Get longest dimension, used for initial size of north arrow"""
        return Nviz_get_longdim(self.data)
    
    def SetViewDefault(self):
        """!Set default view (based on loaded data)
        
        @return z-exag value, default, min and max height
        """
        # determine z-exag
        z_exag = Nviz_get_exag()
        Nviz_change_exag(self.data, z_exag)
        
        # determine height
        hdef = c_double()
        hmin = c_double()
        hmax = c_double()
        Nviz_get_exag_height(byref(hdef), byref(hmin), byref(hmax))
        
        Debug.msg(1, "Nviz::SetViewDefault(): hdef=%f, hmin=%f, hmax=%f",
                  hdef.value, hmin.value, hmax.value)
        
        return (z_exag, hdef.value, hmin.value, hmax.value)
    
    def SetView(self, x, y, height, persp, twist):
        """!Change view settings
        @param x,y position
        @param height
        @param persp perpective
        @param twist
        """
        Nviz_set_viewpoint_height(height)
        Nviz_set_viewpoint_position(x, y)
        Nviz_set_viewpoint_twist(twist)
        Nviz_set_viewpoint_persp(persp)
        
        Debug.msg(3, "Nviz::SetView(): x=%f, y=%f, height=%f, persp=%f, twist=%f",
                  x, y, height, persp, twist)
                
    def GetViewpointPosition(self):
        x = c_double()
        y = c_double()
        h = c_double()
        Nviz_get_viewpoint_height(byref(h))
        Nviz_get_viewpoint_position(byref(x), byref(y))
        
        return (x.value, y.value, h.value)
        
    def LookHere(self, x, y):
        """!Look here feature 
        @param x,y screen coordinates
        """
        
        Nviz_look_here(x, y)
        Debug.msg(3, "Nviz::LookHere(): x=%f, y=%f", x, y)
    
    def LookAtCenter(self):
        """!Center view at center of displayed surface"""
        Nviz_set_focus_map(MAP_OBJ_UNDEFINED, -1)
        Debug.msg(3, "Nviz::LookAtCenter()")
    
    def GetFocus(self):
        """!Get focus"""
        Debug.msg(3, "Nviz::GetFocus()")
        if Nviz_has_focus(self.data):
            x = c_float()
            y = c_float()
            z = c_float()
            Nviz_get_focus(self.data, byref(x), byref(y), byref(z))
            return x.value, y.value, z.value
        else:
            return -1, -1, -1
        
    def SetFocus(self, x, y, z):
        """!Set focus"""
        Debug.msg(3, "Nviz::SetFocus()")
        Nviz_set_focus(self.data, x, y, z)
        
    def GetViewdir(self):
        """!Get viewdir"""
        Debug.msg(3, "Nviz::GetViewdir()")
        dir = (c_float * 3)()
        GS_get_viewdir(byref(dir))
        
        return dir[0], dir[1], dir[2]
        
    def SetViewdir(self, x, y, z):
        """!Set viewdir"""
        Debug.msg(3, "Nviz::SetViewdir(): x=%f, y=%f, z=%f" % (x, y, z))
        dir = (c_float * 3)()
        for i, coord in enumerate((x, y, z)):
            dir[i] = coord
        GS_set_viewdir(byref(dir))
                
    def SetZExag(self, z_exag):
        """!Set z-exag value
        
        @param z_exag value
        
        @return 1
        """
        Debug.msg(3, "Nviz::SetZExag(): z_exag=%f", z_exag)
        return Nviz_change_exag(self.data, z_exag)
    
    def Draw(self, quick, quick_mode):
        """!Draw canvas
        
        Draw quick mode:
         - DRAW_QUICK_SURFACE
         - DRAW_QUICK_VLINES
         - DRAW_QUICK_VPOINTS
         - DRAW_QUICK_VOLUME
        
        @param quick if true draw in wiremode
        @param quick_mode quick mode
        """
        Debug.msg(3, "Nviz::Draw(): quick=%d", quick)
        
        Nviz_draw_cplane(self.data, -1, -1) # ?
        
        if quick:
            Nviz_draw_quick(self.data, quick_mode)
        else:
            Nviz_draw_all(self.data)
        
    def EraseMap(self):
        """!Erase map display (with background color)
        """
        Debug.msg(1, "Nviz::EraseMap()")
        GS_clear(Nviz_get_bgcolor(self.data))
        
    def InitView(self):
        """!Initialize view"""
        # initialize nviz data
        Nviz_init_data(self.data)
        
        # define default attributes for map objects
        Nviz_set_surface_attr_default()
        # set background color
        Nviz_set_bgcolor(self.data, Nviz_color_from_str("white"))
        
        GS_clear(Nviz_get_bgcolor(self.data))        
        # initialize view, lights
        Nviz_init_view(self.data)
        
        Debug.msg(1, "Nviz::InitView()")
        
    def SetBgColor(self, color_str):
        """!Set background color
        
        @param color_str color string
        """
        Nviz_set_bgcolor(self.data, Nviz_color_from_str(color_str))
        
    def SetLight(self, x, y, z, color, bright, ambient, w = 0, lid = 1):
        """!Change lighting settings
        
        @param x,y,z position
        @param color light color (as string)
        @param bright light brightness
        @param ambient light ambient
        @param w local coordinate (default to 0)
        @param lid light id
        """
        Nviz_set_light_position(self.data, lid, x, y, z, w)
        Nviz_set_light_bright(self.data, lid, bright)
        Nviz_set_light_color(self.data, lid, int(color[0]), int(color[1]), int(color[2]))
        Nviz_set_light_ambient(self.data, lid, ambient)
                             
    def LoadSurface(self, name, color_name, color_value):
        """!Load raster map (surface)
        
        @param name raster map name
        @param color_name raster map for color (None for color_value)
        @param color_value color string (named color or RGB triptet)
        
        @return object id
        @return -1 on failure
        """
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
        
        Debug.msg(1, "Nviz::LoadRaster(): name=%s -> id=%d", name, id)
        
        return id
    
    def AddConstant(self, value, color):
        """!Add new constant surface"""
        id = Nviz_new_map_obj(MAP_OBJ_SURF, None, value, self.data)
        
        Nviz_set_attr(id, MAP_OBJ_SURF, ATT_COLOR, CONST_ATT,
                        None, Nviz_color_from_str(color),
                        self.data)
        Nviz_set_focus_map(MAP_OBJ_UNDEFINED, -1)
        
        Debug.msg(1, "Nviz::AddConstant(): id=%d", id)
        return id
        
    def UnloadSurface(self, id):
        """!Unload surface
        
        @param id surface id
        
        @return 1 on success
        @return 0 on failure
        """
        if not GS_surf_exists(id):
            return 0
        
        Debug.msg(1, "Nviz::UnloadSurface(): id=%d", id)
        
        if GS_delete_surface(id) < 0:
            return 0
        
        return 1
    
    def LoadVector(self, name, points):
        """!Load vector map overlay
        
        @param name vector map name
        @param points if true load 2d points rather then 2d lines
        
        @return object id, id of base surface (or -1 if it is not loaded)
        @return -1 on failure
        """
        baseId = -1
        if GS_num_surfs() == 0:     # load base surface if no loaded
            baseId = Nviz_new_map_obj(MAP_OBJ_SURF, None, 0.0, self.data)
            
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
        
        Debug.msg(1, "Nviz::LoadVector(): name=%s -> id=%d", name, id)
        
        return id, baseId
    
    def UnloadVector(self, id, points):
        """!Unload vector set
        
        @param id vector set id
        @param points vector points or lines set
        
        @return 1 on success
        @return 0 on failure
        """
        Debug.msg(1, "Nviz::UnloadVector(): id=%d", id)
        
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

    def VectorSurfaceSelected(self, vid, sid):
        """!Check if surface is selected (currently unused)
        
        @param vid vector id
        @param sid surface id
        
        @return True if selected
        @return False if not selected
        """
        selected = GV_surf_is_selected(vid, sid)
        Debug.msg(1, "Nviz::VectorSurfaceSelected(): vid=%s, sid=%d -> selected=%d", vid, sid, selected)
        return selected
    
    def LoadVolume(self, name, color_name, color_value):
        """!Load 3d raster map (volume)
        
        @param name 3d raster map name
        @param color_name 3d raster map for color (None for color_value)
        @param color_value color string (named color or RGB triptet)
        
        @return object id
        @return -1 on failure
        """
        mapset = G_find_raster3d(name, "")
        if mapset is None:
            G_warning(_("3d raster map <%s> not found"),
                      name)
            return -1
        
        # topography
        id = Nviz_new_map_obj(MAP_OBJ_VOL,
                              G_fully_qualified_name(name, mapset), 0.0,
                              self.data)
        
        if color_name:      # check for color map
            mapset = G_find_raster3d(color_name, "")
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
                          None, Nviz_color_from_str(color_value),
                          self.data)
        else:               # use by default elevation map for coloring
            Nviz_set_attr(id, MAP_OBJ_VOL, ATT_COLOR, MAP_ATT,
                          G_fully_qualified_name(name, mapset), -1.0,
                          self.data)
        
        Debug.msg(1, "Nviz::LoadVolume(): name=%s -> id=%d", name, id)
        
        return id

    def UnloadVolume(self, id):
        """!Unload volume
        
        @param id volume id
        
        @return 1 on success
        @return 0 on failure
        """
        if not GVL_vol_exists(id):
            return 0
        
        Debug.msg(1, "Nviz::UnloadVolume(): id=%d", id)
        
        if GVL_delete_vol(id) < 0:
          return 0
        
        return 1
    
    def SetSurfaceTopo(self, id, map, value):
        """!Set surface topography
        
        @param id surface id
        @param map if true use map otherwise constant
        @param value map name of value
        
        @return 1 on success
        @return -1 surface not found
        @return -2 setting attributes failed
        """
        return self.SetSurfaceAttr(id, ATT_TOPO, map, value)
    
    def SetSurfaceColor(self, id, map, value):
        """!Set surface color
        
        @param id surface id
        @param map if true use map otherwise constant
        @param value map name or value
        
        @return 1 on success
        @return -1 surface not found
        @return -2 setting attributes failed
        """
        return self.SetSurfaceAttr(id, ATT_COLOR, map, value)
    
    def SetSurfaceMask(self, id, invert, value):
        """!Set surface mask
        
        @todo invert
        
        @param id surface id
        @param invert if true invert mask 
        @param value map name of value
        
        @return 1 on success
        @return -1 surface not found
        @return -2 setting attributes failed
        """
        return self.SetSurfaceAttr(id, ATT_MASK, True, value)
    
    def SetSurfaceTransp(self, id, map, value):
        """!Set surface mask
        
        @todo invert
        
        @param id surface id
        @param map if true use map otherwise constant
        @param value map name of value
        
        @return 1 on success
        @return -1 surface not found
        @return -2 setting attributes failed
        """
        return self.SetSurfaceAttr(id, ATT_TRANSP, map, value)
    
    def SetSurfaceShine(self, id, map, value):
        """!Set surface shininess
        
        @param id surface id
        @param map if true use map otherwise constant
        @param value map name of value
        
        @return 1 on success
        @return -1 surface not found
        @return -2 setting attributes failed
        """
        return self.SetSurfaceAttr(id, ATT_SHINE, map, value)
    
    def SetSurfaceEmit(self, id, map, value):
        """!Set surface emission (currently unused)
        
        @param id surface id
        @param map if true use map otherwise constant
        @param value map name of value
        
        @return 1 on success
        @return -1 surface not found
        @return -2 setting attributes failed
        """
        return self.SetSurfaceAttr(id, ATT_EMIT, map, value)
    
    def SetSurfaceAttr(self, id, attr, map, value):
        """!Set surface attribute
        
        @param id surface id
        @param attr attribute desc
        @param map if true use map otherwise constant
        @param value map name of value
        
        @return 1 on success
        @return -1 surface not found
        @return -2 setting attributes failed
        """
        if not GS_surf_exists(id):
            return -1
        
        if map:
            ret = Nviz_set_attr(id, MAP_OBJ_SURF, attr, MAP_ATT,
                                value, -1.0, self.data)
        else:
            if attr == ATT_COLOR:
                val = Nviz_color_from_str(value)
            else:
                val = float(value)
            
            ret = Nviz_set_attr(id, MAP_OBJ_SURF, attr, CONST_ATT,
                                None, val, self.data)
        
        Debug.msg(3, "Nviz::SetSurfaceAttr(): id=%d, attr=%d, map=%d, value=%s",
                  id, attr, map, value)
        
        if ret < 0:
            return -2
        
        return 1
    
    def UnsetSurfaceMask(self, id):
        """!Unset surface mask
        
        @param id surface id
        
        @return 1 on success
        @return -1 surface not found
        @return -2 setting attributes failed
        @return -1 on failure
        """
        return self.UnsetSurfaceAttr(id, ATT_MASK)
    
    def UnsetSurfaceTransp(self, id):
        """!Unset surface transparency
        
        @param id surface id
        
        @return 1 on success
        @return -1 surface not found
        @return -2 setting attributes failed
        """
        return self.UnsetSurfaceAttr(id, ATT_TRANSP)
    
    def UnsetSurfaceEmit(self, id):
        """!Unset surface emission (currently unused)
        
        @param id surface id
        
        @return 1 on success
        @return -1 surface not found
        @return -2 setting attributes failed
        """
        return self.UnsetSurfaceAttr(id, ATT_EMIT)
    
    def UnsetSurfaceAttr(self, id, attr):
        """!Unset surface attribute
        
        @param id surface id
        @param attr attribute descriptor
        
        @return 1 on success
        @return -1 surface not found
        @return -2 setting attributes failed
        """
        if not GS_surf_exists(id):
            return -1
        
        Debug.msg(3, "Nviz::UnsetSurfaceAttr(): id=%d, attr=%d",
                  id, attr)
        
        ret = Nviz_unset_attr(id, MAP_OBJ_SURF, attr)
        
        if ret < 0:
            return -2
        
        return 1

    def SetSurfaceRes(self, id, fine, coarse):
        """!Set surface resolution
        
        @param id surface id
        @param fine x/y fine resolution
        @param coarse x/y coarse resolution
        
        @return 1 on success
        @return -1 surface not found
        @return -2 setting attributes failed
        """
        Debug.msg(3, "Nviz::SetSurfaceRes(): id=%d, fine=%d, coarse=%d",
                  id, fine, coarse)
        
        if id > 0:
            if not GS_surf_exists(id):
                return -1
            
            if GS_set_drawres(id, fine, fine, coarse, coarse) < 0:
                return -2
        else:
            GS_setall_drawres(fine, fine, coarse, coarse)
        
        return 1

    def SetSurfaceStyle(self, id, style):
        """!Set draw style
        
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
         
        @param id surface id (<= 0 for all)
        @param style draw style
        
        @return 1 on success
        @return -1 surface not found
        @return -2 setting attributes failed
        """
        Debug.msg(3, "Nviz::SetSurfaceStyle(): id=%d, style=%d",
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
    
    def SetWireColor(self, id, color_str):
        """!Set color of wire
        
        @todo all
         
        @param id surface id (< 0 for all)
        @param color_str color string (R:G:B)
        
        @return 1 on success
        @return -1 surface not found
        @return -2 setting attributes failed
        @return 1 on success
        @return 0 on failure
        """
        Debug.msg(3, "Nviz::SetWireColor(): id=%d, color=%s",
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
    
    def GetSurfacePosition(self, id):
        """!Get surface position
        
        @param id surface id
        
        @return x,y,z
        @return zero-length vector on error
        """
        if not GS_surf_exists(id):
            return []
        
        x, y, z = c_float(), c_float(), c_float()
        GS_get_trans(id, byref(x), byref(y), byref(z))
        
        Debug.msg(3, "Nviz::GetSurfacePosition(): id=%d, x=%f, y=%f, z=%f",
                  id, x.value, y.value, z.value)
        
        return [x.value, y.value, z.value]

    def SetSurfacePosition(self, id, x, y, z):
        """!Set surface position
        
        @param id surface id
        @param x,y,z translation values
        
        @return 1 on success
        @return -1 surface not found
        @return -2 setting position failed
        """
        if not GS_surf_exists(id):
            return -1
        
        Debug.msg(3, "Nviz::SetSurfacePosition(): id=%d, x=%f, y=%f, z=%f",
                  id, x, y, z)
        
        GS_set_trans(id, x, y, z)
        
        return 1

    def SetVectorLineMode(self, id, color_str, width, flat):
        """!Set mode of vector line overlay
        
        @param id vector id
        @param color_str color string
        @param width line width
        @param flat display flat or on surface
        
        @return -1 vector set not found
        @return -2 on failure
        @return 1 on success
        """
        if not GV_vect_exists(id):
            return -1
        
        Debug.msg(3, "Nviz::SetVectorMode(): id=%d, color=%s, width=%d, flat=%d",
                  id, color_str, width, flat)
        
        color = Nviz_color_from_str(color_str)
        
        # use memory by default
        if GV_set_style(id, 1, color, width, flat) < 0:
            return -2
        
        return 1

    def SetVectorLineHeight(self, id, height):
        """!Set vector height above surface (lines)
        
        @param id vector set id
        @param height
        
        @return -1 vector set not found
        @return 1 on success
        """
        if not GV_vect_exists(id):
            return -1
        
        Debug.msg(3, "Nviz::SetVectorLineHeight(): id=%d, height=%f",
                  id, height)
        
        GV_set_trans(id, 0.0, 0.0, height)
        
        return 1

    def SetVectorLineSurface(self, id, surf_id):
        """!Set reference surface of vector set (lines)
        
        @param id vector set id
        @param surf_id surface id
        
        @return 1 on success
        @return -1 vector set not found
        @return -2 surface not found
        @return -3 on failure
        """
        if not GV_vect_exists(id):
            return -1
        
        if not GS_surf_exists(surf_id):
            return -2
        
        if GV_select_surf(id, surf_id) < 0:
            return -3
        
        return 1

    def UnsetVectorLineSurface(self, id, surf_id):
        """!Unset reference surface of vector set (lines)
        
        @param id vector set id
        @param surf_id surface id
        
        @return 1 on success
        @return -1 vector set not found
        @return -2 surface not found
        @return -3 on failure
        """
        if not GV_vect_exists(id):
            return -1
        
        if not GS_surf_exists(surf_id):
            return -2
        
        if GV_unselect_surf(id, surf_id) < 0:
            return -3
        
        return 1
        
    def SetVectorPointMode(self, id, color_str, width, size, marker):
        """!Set mode of vector point overlay
        
        @param id vector id
        @param color_str color string
        @param width line width
        @param size size of the symbol
        @param marker type of the symbol
        
        @return -1 vector set not found
        """
        if not GP_site_exists(id):
            return -1
        
        # dtree and ctree defined but not used
        if marker > 5:
            marker += 2
        
        Debug.msg(3, "Nviz::SetVectorPointMode(): id=%d, color=%s, "
                  "width=%d, size=%f, marker=%d",
                  id, color_str, width, size, marker)
        
        color = Nviz_color_from_str(color_str)
        
        if GP_set_style(id, color, width, size, marker) < 0:
            return -2
        
        return 1

    def SetVectorPointHeight(self, id, height):
        """!Set vector height above surface (points)
        
        @param id vector set id
        @param height
        
        @return -1 vector set not found
        @return 1 on success
        """
        if not GP_site_exists(id):
            return -1
        
        Debug.msg(3, "Nviz::SetVectorPointHeight(): id=%d, height=%f",
                  id, height)
        
        GP_set_trans(id, 0.0, 0.0, height)
        
        return 1

    def SetVectorPointSurface(self, id, surf_id):
        """!Set reference surface of vector set (points)
        
        @param id vector set id
        @param surf_id surface id
        
        @return 1 on success
        @return -1 vector set not found
        @return -2 surface not found
        @return -3 on failure
        """
        if not GP_site_exists(id):
            return -1
        
        if not GS_surf_exists(surf_id):
            return -2
        
        if GP_select_surf(id, surf_id) < 0:
            return -3
        
        return 1

    def ReadVectorColors(self, name, mapset):
        """!Read vector colors
        
        @param name vector map name
        @param mapset mapset name (empty string (\c "") for search path)
        
        @return -1 on error 
        @return 0 if color table missing 
        @return 1 on success (color table found) 
        """
        return Vect_read_colors(name, mapset, self.color)
        
    def CheckColorTable(self, id, type):
        """!Check if color table exists.
        
        @param id vector set id
        @param type vector set type (lines/points)
        
        @return 1 color table exists
        @return 0 no color table found
        @return -1 on error
        @return -2 vector set not found
        """
        file = c_char_p()
        
        if type == 'points':
            ret = GP_get_sitename(id, byref(file))
        elif type == 'lines':
            ret = GV_get_vectname(id, byref(file))
            
        if ret < 0:
            return -2
        
        return self.ReadVectorColors(file, "")
        
    def SetPointsStyleThematic(self, id, layer, color = None, colorTable = False, 
                               width = None, size = None, symbol = None):
        """!Set thematic style for vector points
        
        @param id vector set id
        @param layer layer number for thematic mapping
        @param colorTable use color table 
        @param color color column name 
        @param width width column name 
        @param size size column name 
        @param symbol symbol column name 
        """
        file = c_char_p()
        ret = GP_get_sitename(id, byref(file))
        if ret < 0:
            return -1
        
        ret = self.ReadVectorColors(file, "")
        if ret < 0:
            return -1
        
        if colorTable:
            GP_set_style_thematic(id, layer, color, width, size, symbol, self.color)
        else:
            GP_set_style_thematic(id, layer, color, width, size, symbol, None)

    def SetLinesStyleThematic(self, id, layer, color = None, colorTable = False, width = None):
        """!Set thematic style for vector lines
        
        @param id vector set id
        @param layer layer number for thematic mapping
        @param color color column name 
        @param colorTable use color table 
        @param width width column name 
        """
        file = c_char_p()
        ret = GV_get_vectname(id, byref(file))
        if ret < 0:
            return -1
        
        ret = self.ReadVectorColors(file, "")
        if ret < 0:
            return -1
        
        if colorTable:
            GV_set_style_thematic(id, layer, color, width, self.color)
        else:
            GV_set_style_thematic(id, layer, color, width, None)
        
    def UnsetLinesStyleThematic(self, id):
        """!Unset thematic style for vector points"""
        GV_unset_style_thematic(id)      
         
    def UnsetPointsStyleThematic(self, id):
        """!Unset thematic style for vector lines"""
        GP_unset_style_thematic(id)
        
    def UnsetVectorPointSurface(self, id, surf_id):
        """!Unset reference surface of vector set (points)
        
        @param id vector set id
        @param surf_id surface id
        
        @return 1 on success
        @return -1 vector set not found
        @return -2 surface not found
        @return -3 on failure
        """
        if not GP_site_exists(id):
            return -1
        
        if not GS_surf_exists(surf_id):
            return -2
        
        if GP_unselect_surf(id, surf_id) < 0:
            return -3
        
        return 1
        
    def AddIsosurface(self, id, level, isosurf_id = None):
        """!Add new isosurface
        
        @param id volume id
        @param level isosurface level (topography)
        @param isosurf_id isosurface id
        
        @return -1 on failure
        @return 1 on success
        """
        if not GVL_vol_exists(id):
            return -1
        
        if isosurf_id is not None:
            num = GVL_isosurf_num_isosurfs(id)
            if num < 0 or isosurf_id != num:
                return -1
                
        if GVL_isosurf_add(id) < 0:
            return -1
        
        # set topography level
        nisosurfs = GVL_isosurf_num_isosurfs(id)
        
        return GVL_isosurf_set_att_const(id, nisosurfs - 1, ATT_TOPO, level)
    
    def AddSlice(self, id, slice_id = None):
        """!Add new slice
        
        @param id volume id
        @param slice_id slice id
        
        @return -1 on failure
        @return number of slices
        """
        if not GVL_vol_exists(id):
            return -1
        
        if slice_id is not None:
            num = GVL_slice_num_slices(id)
            if num < 0 or slice_id != num:
                return -1
                
        if GVL_slice_add(id) < 0:
            return -1
        
        return GVL_slice_num_slices(id)
    
    def DeleteIsosurface(self, id, isosurf_id):
        """!Delete isosurface
        
        @param id volume id
        @param isosurf_id isosurface id
        
        @return 1 on success
        @return -1 volume not found
        @return -2 isosurface not found
        @return -3 on failure
        """
        if not GVL_vol_exists(id):
            return -1
        
        if isosurf_id > GVL_isosurf_num_isosurfs(id):
            return -2
        
        ret = GVL_isosurf_del(id, isosurf_id)
        
        if ret < 0:
            return -3

        return 1
    
    def DeleteSlice(self, id, slice_id):
        """!Delete slice
        
        @param id volume id
        @param slice_id slice id
        
        @return 1 on success
        @return -1 volume not found
        @return -2 slice not found
        @return -3 on failure
        """
        if not GVL_vol_exists(id):
            return -1
        
        if slice_id > GVL_slice_num_slices(id):
            return -2
        
        ret = GVL_slice_del(id, slice_id)
        
        if ret < 0:
            return -3

        return 1
    
    def MoveIsosurface(self, id, isosurf_id, up):
        """!Move isosurface up/down in the list
        
        @param id volume id
        @param isosurf_id isosurface id
        @param up if true move up otherwise down
        
        @return 1 on success
        @return -1 volume not found
        @return -2 isosurface not found
        @return -3 on failure
        """
        if not GVL_vol_exists(id):
            return -1
        
        if isosurf_id > GVL_isosurf_num_isosurfs(id):
            return -2
        
        if up:
            ret = GVL_isosurf_move_up(id, isosurf_id)
        else:
            ret = GVL_isosurf_move_down(id, isosurf_id)
        
        if ret < 0:
            return -3

        return 1

    def MoveSlice(self, id, slice_id, up):
        """!Move slice up/down in the list
        
        @param id volume id
        @param slice_id slice id
        @param up if true move up otherwise down
        
        @return 1 on success
        @return -1 volume not found
        @return -2 slice not found
        @return -3 on failure
        """
        if not GVL_vol_exists(id):
            return -1
        
        if slice_id > GVL_slice_num_slices(id):
            return -2
        
        if up:
            ret = GVL_slice_move_up(id, slice_id)
        else:
            ret = GVL_slice_move_down(id, slice_id)
        
        if ret < 0:
            return -3

        return 1
    
    def SetIsosurfaceTopo(self, id, isosurf_id, map, value):
        """!Set isosurface level
        
        @param id volume id
        @param isosurf_id isosurface id (0 - MAX_ISOSURFS)
        @param map if true use map otherwise constant
        @param value map name of value
        
        @return 1 on success
        @return -1 volume not found
        @return -2 isosurface not found
        @return -3 on failure
        """
        return self.SetIsosurfaceAttr(id, isosurf_id, ATT_TOPO, map, value)
    
    def SetIsosurfaceColor(self, id, isosurf_id, map, value):
        """!Set isosurface color
        
        @param id volume id
        @param isosurf_id isosurface id (0 - MAX_ISOSURFS)
        @param map if true use map otherwise constant
        @param value map name of value
        
        @return 1 on success
        @return -1 volume not found
        @return -2 isosurface not found
        @return -3 on failure
        """
        return self.SetIsosurfaceAttr(id, isosurf_id, ATT_COLOR, map, value)
    
    def SetIsosurfaceMask(self, id, isosurf_id, invert, value):
        """!Set isosurface mask
        
        @todo invert
        
        @param id volume id
        @param isosurf_id isosurface id (0 - MAX_ISOSURFS)
        @param invert true for invert mask
        @param value map name to be used for mask
        
        @return 1 on success
        @return -1 volume not found
        @return -2 isosurface not found
        @return -3 on failure
        """
        return self.SetIsosurfaceAttr(id, isosurf_id, ATT_MASK, True, value)
    
    def SetIsosurfaceTransp(self, id, isosurf_id, map, value):
        """!Set isosurface transparency
        
        @param id volume id
        @param isosurf_id isosurface id (0 - MAX_ISOSURFS)
        @param map if true use map otherwise constant
        @param value map name of value
        
        @return 1 on success
        @return -1 volume not found
        @return -2 isosurface not found
        @return -3 on failure
        """
        return self.SetIsosurfaceAttr(id, isosurf_id, ATT_TRANSP, map, value)
    
    def SetIsosurfaceShine(self, id, isosurf_id, map, value):
        """!Set isosurface shininess
        
        @param id volume id
        @param isosurf_id isosurface id (0 - MAX_ISOSURFS)
        @param map if true use map otherwise constant
        @param value map name of value
        
        @return 1 on success
        @return -1 volume not found
        @return -2 isosurface not found
        @return -3 on failure
        """
        return self.SetIsosurfaceAttr(id, isosurf_id, ATT_SHINE, map, value)
    
    def SetIsosurfaceEmit(self, id, isosurf_id, map, value):
        """!Set isosurface emission (currently unused)
        
        @param id volume id
        @param isosurf_id isosurface id (0 - MAX_ISOSURFS)
        @param map if true use map otherwise constant
        @param value map name of value
        
        @return 1 on success
        @return -1 volume not found
        @return -2 isosurface not found
        @return -3 on failure
        """
        return self.SetIsosurfaceAttr(id, isosurf_id, ATT_EMIT, map, value)
    
    def SetIsosurfaceAttr(self, id, isosurf_id, attr, map, value):
        """!Set isosurface attribute
        
        @param id volume id
        @param isosurf_id isosurface id (0 - MAX_ISOSURFS)
        @param attr attribute desc
        @param map if true use map otherwise constant
        @param value map name of value
        
        @return 1 on success
        @return -1 volume not found
        @return -2 isosurface not found
        @return -3 setting attributes failed
        """
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
        
        Debug.msg(3, "Nviz::SetIsosurfaceAttr(): id=%d, isosurf=%d, "
                  "attr=%d, map=%s, value=%s",
                  id, isosurf_id, attr, map, value)
        
        if ret < 0:
            return -2
        
        return 1
    
    def UnsetIsosurfaceMask(self, id, isosurf_id):
        """!Unset isosurface mask
        
        @param id volume id
        @param isosurf_id isosurface id (0 - MAX_ISOSURFS)
        
        @return 1 on success
        @return -1 volume not found
        @return -2 isosurface not found
        @return -3 setting attributes failed
        """
        return self.UnsetIsosurfaceAttr(id, isosurf_id, ATT_MASK)
    
    def UnsetIsosurfaceTransp(self, id, isosurf_id):
        """!Unset isosurface transparency
        
        @param id volume id
        @param isosurf_id isosurface id (0 - MAX_ISOSURFS)
        
        @return 1 on success
        @return -1 volume not found
        @return -2 isosurface not found
        @return -3 setting attributes failed
        """
        return self.UnsetIsosurfaceAttr(id, isosurf_id, ATT_TRANSP)
    
    def UnsetIsosurfaceEmit(self, id, isosurf_id):
        """!Unset isosurface emission (currently unused)
        
        @param id volume id
        @param isosurf_id isosurface id (0 - MAX_ISOSURFS)
        
        @return 1 on success
        @return -1 volume not found
        @return -2 isosurface not found
        @return -3 setting attributes failed
        """
        return self.UnsetIsosurfaceAttr(id, isosurf_id, ATT_EMIT)
    
    def UnsetIsosurfaceAttr(self, id, isosurf_id, attr):
        """!Unset surface attribute
        
        @param id surface id
        @param isosurf_id isosurface id (0 - MAX_ISOSURFS)
        @param attr attribute descriptor
        
        @return 1 on success
        @return -1 volume not found
        @return -2 isosurface not found
        @return -2 on failure
        """
        if not GVL_vol_exists(id):
            return -1
        
        if isosurf_id > GVL_isosurf_num_isosurfs(id) - 1:
            return -2
        
        Debug.msg(3, "Nviz::UnsetSurfaceAttr(): id=%d, isosurf_id=%d, attr=%d",
                  id, isosurf_id, attr)
        
        ret = GVL_isosurf_unset_att(id, isosurf_id, attr)
        
        if ret < 0:
            return -2
        
        return 1

    def SetIsosurfaceMode(self, id, mode):
        """!Set draw mode for isosurfaces
        
        @param id isosurface id
        @param mode isosurface draw mode
        
        @return 1 on success
        @return -1 volume set not found
        @return -2 on failure
        """
        if not GVL_vol_exists(id):
            return -1
        
        ret = GVL_isosurf_set_drawmode(id, mode)
        
        if ret < 0:
            return -2
        
        return 1
    
    def SetSliceMode(self, id, mode):
        """!Set draw mode for slices
        
        @param id slice id
        @param mode slice draw mode
        
        @return 1 on success
        @return -1 volume set not found
        @return -2 on failure
        """
        if not GVL_vol_exists(id):
            return -1
        
        ret = GVL_slice_set_drawmode(id, mode)
        
        if ret < 0:
            return -2
        
        return 1
    
    def SetIsosurfaceRes(self, id, res):
        """!Set draw resolution for isosurfaces
        
        @param id isosurface id
        @param res resolution value
        
        @return 1 on success
        @return -1 volume set not found
        @return -2 on failure
        """
        if not GVL_vol_exists(id):
            return -1
        
        ret = GVL_isosurf_set_drawres(id, res, res, res)
        
        if ret < 0:
            return -2
        
        return 1
    
    def SetSliceRes(self, id, res):
        """!Set draw resolution for slices
        
        @param id slice id
        @param res resolution value
        
        @return 1 on success
        @return -1 volume set not found
        @return -2 on failure
        """
        if not GVL_vol_exists(id):
            return -1
        
        ret = GVL_slice_set_drawres(id, res, res, res)
        
        if ret < 0:
            return -2
        
        return 1
    
    def SetSlicePosition(self, id, slice_id, x1, x2, y1, y2, z1, z2, dir):
        """!Set slice position
        
        @param id volume id
        @param slice_id slice id
        @param x1,x2,y1,y2,z1,z2 slice coordinates
        @param dir axis
        
        @return 1 on success
        @return -1 volume not found
        @return -2 slice not found
        @return -3 on failure
        """
        if not GVL_vol_exists(id):
            return -1
        
        if slice_id > GVL_slice_num_slices(id):
            return -2
        
        ret = GVL_slice_set_pos(id, slice_id, x1, x2, y1, y2, z1, z2, dir)
        
        if ret < 0:
            return -2
        
        return 1
    
    def SetSliceTransp(self, id, slice_id, value):
        """!Set slice transparency
        
        @param id volume id
        @param slice_id slice id
        @param value transparency value (0 - 255)
        
        @return 1 on success
        @return -1 volume not found
        @return -2 slice not found
        @return -3 on failure
        """
        
        if not GVL_vol_exists(id):
            return -1
        
        if slice_id > GVL_slice_num_slices(id):
            return -2
        
        ret = GVL_slice_set_transp(id, slice_id, value)
        
        if ret < 0:
            return -2
        
        return 1
    
    def SetIsosurfaceInOut(self, id, isosurf_id, inout):
        """!Set inout mode
        
        @param id volume id
        @param isosurf_id isosurface id
        @param inout mode true/false
        
        @return 1 on success
        @return -1 volume set not found
        @return -2 isosurface not found
        @return -3 on failure
        """
        if not GVL_vol_exists(id):
            return -1
        
        if isosurf_id > GVL_isosurf_num_isosurfs(id) - 1:
            return -2
        
        ret = GVL_isosurf_set_flags(id, isosurf_id, inout)
        
        if ret < 0:
            return -3
        
        return 1
    
    def GetVolumePosition(self, id):
        """!Get volume position
        
        @param id volume id
        
        @return x,y,z
        @return zero-length vector on error
        """
        if not GVL_vol_exists(id):
            return []
        
        x, y, z = c_float(), c_float(), c_float()
        GVL_get_trans(id, byref(x), byref(y), byref(z))
        
        Debug.msg(3, "Nviz::GetVolumePosition(): id=%d, x=%f, y=%f, z=%f",
                  id, x.value, y.value, z.value)
        
        return [x.value, y.value, z.value]
    
    def SetVolumePosition(self, id, x, y, z):
        """!Set volume position
        
        @param id volume id
        @param x,y,z translation values
        
        @return 1 on success
        @return -1 volume not found
        @return -2 setting position failed
        """
        if not GVL_vol_exists(id):
            return -1
        
        Debug.msg(3, "Nviz::SetVolumePosition(): id=%d, x=%f, y=%f, z=%f",
                  id, x, y, z)
        
        GVL_set_trans(id, x, y, z)
        
        return 1

    def SetVolumeDrawBox(self, id, ifBox):
        """!Display volume wire box
        
        @param id volume id
        @param ifBox True to draw wire box, False otherwise
        
        @return 1 on success
        @return -1 volume not found
        """
        if not GVL_vol_exists(id):
            return -1

        Debug.msg(3, "Nviz::SetVolumeDrawBox(): id=%d, ifBox=%d", id, ifBox)
        
        GVL_set_draw_wire(id, int(ifBox))

        return 1

    def GetCPlaneCurrent(self):
        return Nviz_get_current_cplane(self.data)
    
    def GetCPlanesCount(self):
        """!Returns number of cutting planes"""
        return Nviz_num_cplanes(self.data) 
    
    def GetCPlaneRotation(self):
        """!Returns rotation parameters of current cutting plane"""
        x, y, z = c_float(), c_float(), c_float()
        
        current = Nviz_get_current_cplane(self.data)
        Nviz_get_cplane_rotation(self.data, current, byref(x), byref(y), byref(z))
        
        return x.value, y.value, z.value
    
    def GetCPlaneTranslation(self):
        """!Returns translation parameters of current cutting plane"""
        x, y, z = c_float(), c_float(), c_float()
        
        current = Nviz_get_current_cplane(self.data)
        Nviz_get_cplane_translation(self.data, current, byref(x), byref(y), byref(z))
        
        return x.value, y.value, z.value
    
    def SetCPlaneRotation(self, x, y, z):
        """!Set current clip plane rotation
        
        @param x,y,z rotation parameters
        """
        current = Nviz_get_current_cplane(self.data)
        Nviz_set_cplane_rotation(self.data, current, x, y, z)
        Nviz_draw_cplane(self.data, -1, -1)
    
    def SetCPlaneTranslation(self, x, y, z):
        """!Set current clip plane translation
        
        @param x,y,z translation parameters
        """
        current = Nviz_get_current_cplane(self.data)
        Nviz_set_cplane_translation(self.data, current, x, y, z)
        Nviz_draw_cplane(self.data, -1, -1) 
        Debug.msg(3, "Nviz::SetCPlaneTranslation(): id=%d, x=%f, y=%f, z=%f",
                  current, x, y, z)
                
    def SetCPlaneInteractively(self, x, y):
        current = Nviz_get_current_cplane(self.data)
        ret = Nviz_set_cplane_here(self.data, current, x, y)
        if ret:
            Nviz_draw_cplane(self.data, -1, -1)
            x, y, z = self.GetCPlaneTranslation()
            return x, y, z
        else:
            return None, None, None
        
        
    def SelectCPlane(self, index):
        """!Select cutting plane
        
        @param index index of cutting plane
        """
        Nviz_on_cplane(self.data, index)
    
    def UnselectCPlane(self, index):
        """!Unselect cutting plane
        
        @param index index of cutting plane
        """
        Nviz_off_cplane(self.data, index)
        
    def SetFenceColor(self, index):
        """!Select current cutting plane
        
        @param index type of fence - from 0 (off) to 4
        """    
        Nviz_set_fence_color(self.data, index)
            
    def GetXYRange(self):
        """!Get xy range"""
        return Nviz_get_xyrange(self.data)
    
    def GetZRange(self):
        """!Get z range"""
        min, max = c_float(), c_float()
        Nviz_get_zrange(self.data, byref(min), byref(max))
        return min.value, max.value
    
    def SaveToFile(self, filename, width = 20, height = 20, itype = 'ppm'):
        """!Save current GL screen to ppm/tif file

        @param filename file name
        @param width image width
        @param height image height
        @param itype image type ('ppm' or 'tif')
        """
        widthOrig  = self.width
        heightOrig = self.height
        
        self.ResizeWindow(width, height)
        GS_clear(Nviz_get_bgcolor(self.data))
        self.Draw(False, -1)
        if itype == 'ppm':
            GS_write_ppm(filename)
        else:
            GS_write_tif(filename)
        
        self.ResizeWindow(widthOrig, heightOrig)

    def DrawLightingModel(self):
        """!Draw lighting model"""
        if self.showLight:
            Nviz_draw_model(self.data)

    def DrawFringe(self):
        """!Draw fringe"""
        Nviz_draw_fringe(self.data)
        
    def SetFringe(self, sid, color, elev, nw = False, ne = False, sw = False, se = False):
        """!Set fringe

        @param sid surface id
        @param color color
        @param elev elevation (height)
        @param nw,ne,sw,se fringe edges (turn on/off)
        """
        scolor = str(color[0]) + ':' + str(color[1]) + ':' + str(color[2])
        Nviz_set_fringe(self.data,
                        sid, Nviz_color_from_str(scolor),
                        elev, int(nw), int(ne), int(sw), int(se))
    
    def DrawArrow(self):
        """!Draw north arrow
        """
        return Nviz_draw_arrow(self.data)
        
    def SetArrow(self, sx, sy, size, color):
        """!Set north arrow from canvas coordinates
        
        @param sx,sy canvas coordinates
        @param size arrow length
        @param color arrow color
        """
        return Nviz_set_arrow(self.data, sx, sy, size, Nviz_color_from_str(color))       
        
    def DeleteArrow(self):
        """!Delete north arrow
        """
        Nviz_delete_arrow(self.data)
    
    def SetScalebar(self, id, sx, sy, size, color):
        """!Set scale bar from canvas coordinates
        
        @param sx,sy canvas coordinates
        @param id scale bar id
        @param size scale bar length
        @param color scale bar color
        """
        return Nviz_set_scalebar(self.data, id, sx, sy, size, Nviz_color_from_str(color))
    
    def DrawScalebar(self):
        """!Draw scale bar
        """
        return Nviz_draw_scalebar(self.data)
    
    def DeleteScalebar(self, id):
        """!Delete scalebar
        """
        Nviz_delete_scalebar(self.data, id)
        
    def GetPointOnSurface(self, sx, sy):
        """!Get point on surface

        @param sx,sy canvas coordinates (LL)
        """
        sid = c_int()
        x   = c_float()
        y   = c_float()
        z   = c_float()
        Debug.msg(5, "Nviz::GetPointOnSurface(): sx=%d sy=%d" % (sx, sy))
        num = GS_get_selected_point_on_surface(sx, sy, byref(sid), byref(x), byref(y), byref(z))
        if num == 0:
            return (None, None, None, None)
        
        return (sid.value, x.value, y.value, z.value)

    def QueryMap(self, sx, sy):
        """!Query surface map

        @param sx,sy canvas coordinates (LL)
        """
        sid, x, y, z = self.GetPointOnSurface(sx, sy)
        if not sid:
            return None
        
        catstr = create_string_buffer(256)
        valstr = create_string_buffer(256)
        GS_get_cat_at_xy(sid, ATT_TOPO, catstr, x, y)
        GS_get_val_at_xy(sid, ATT_COLOR, valstr, x, y)
        
        return { 'id' : sid,
                 'x'  : x,
                 'y'  : y,
                 'z'  : z,
                 'elevation' : catstr.value.replace('(', '').replace(')', ''),
                 'color'     : valstr.value }
    
    def GetDistanceAlongSurface(self, sid, p1, p2, useExag = True):
        """!Get distance measured along surface"""
        d = c_float()
        
        GS_get_distance_alongsurf(sid, p1[0], p1[1], p2[0], p2[1],
                                  byref(d), int(useExag))
        
        return d.value

    def GetRotationParameters(self, dx, dy):
        """!Get rotation parameters (angle, x, y, z axes)
        
        @param dx,dy difference from previous mouse drag event
        """
        modelview = (c_double * 16)()
        Nviz_get_modelview(byref(modelview))
        
        angle = sqrt(dx*dx+dy*dy)/float(self.width+1)*180.0
        m = []
        row = []
        for i, item in enumerate(modelview):
            row.append(item)
            if (i+1) % 4 == 0:
                m.append(row)
                row = []
        inv = matrix(m).I
        ax, ay, az = dy, dx, 0.
        x = inv[0,0]*ax + inv[1,0]*ay + inv[2,0]*az
        y = inv[0,1]*ax + inv[1,1]*ay + inv[2,1]*az
        z = inv[0,2]*ax + inv[1,2]*ay + inv[2,2]*az
        
        return angle, x, y, z 
       
    def Rotate(self, angle, x, y, z):
        """!Set rotation parameters
        Rotate scene (difference from current state).

        @param angle angle
        @param x,y,z axis coordinate
        """
        Nviz_set_rotation(angle, x, y, z)
        
    def UnsetRotation(self):
        """!Stop rotating the scene"""
        Nviz_unset_rotation()
        
    def ResetRotation(self):
        """!Reset scene rotation"""
        Nviz_init_rotation()
        
    def GetRotationMatrix(self):
        """!Get rotation matrix"""
        matrix = (c_double * 16)()
        GS_get_rotation_matrix(byref(matrix))
        returnMatrix = []
        for item in matrix:
            returnMatrix.append(item)
        return returnMatrix
        
    def SetRotationMatrix(self, matrix):
        """!Set rotation matrix"""
        mtrx = (c_double * 16)()
        for i in range(len(matrix)):
            mtrx[i] = matrix[i]
        GS_set_rotation_matrix(byref(mtrx))
    
    def Start2D(self):
        Nviz_set_2D(self.width, self.height)
        
    def FlyThrough(self, flyInfo, mode, exagInfo):
        """!Fly through the scene
        
        @param flyInfo fly parameters
        @param mode 0 or 1 for different fly behaviour
        @param exagInfo parameters changing fly speed
        """
        fly = (c_float * 3)()
        for i, item in enumerate(flyInfo):
            fly[i] = item
        exag = (c_int * 2)()
        exag[0] = int(exagInfo['move'])
        exag[1] = int(exagInfo['turn'])
        Nviz_flythrough(self.data, fly, exag, mode)
        
class Texture(object):
    """!Class representing OpenGL texture"""
    def __init__(self, filepath, overlayId, coords):
        """!Load image to texture

        @param filepath path to image file
        @param overlayId id of overlay (1 for legend, 101 and more for text)
        @param coords image coordinates
        """
        self.path = filepath
        self.image = wx.Image(filepath, wx.BITMAP_TYPE_ANY)
        self.width = self.image.GetWidth()
        self.height = self.image.GetHeight()
        self.id = overlayId
        self.coords = list(coords)
        self.bounds = wx.Rect()
        self.active = True
        
        # alpha needs to be initialized
        if not self.image.HasAlpha():
            self.image.InitAlpha()
    
        # resize image to match 2^n
        self.Resize()
        
        # check max texture size
        maxSize = c_int()
        Nviz_get_max_texture(byref(maxSize))
        self.maxSize = maxSize.value
        if self.maxSize < self.width or self.maxSize < self.height:
            # TODO: split up image 
            self.textureId = None
        else:
            self.textureId = self.Load()
            
    def __del__(self):
        """!Delete texture"""
        if self.textureId:
            Nviz_del_texture(self.textureId)
        grass.try_remove(self.path)
            
    def Resize(self):    
        """!Resize image to match 2^n"""
        n = m = 1
        while self.width > pow(2,n):
            n += 1
        while self.height > pow(2,m):
            m += 1
        self.image.Resize(size = (pow(2,n), pow(2,m)), pos = (0, 0))
        self.width = self.image.GetWidth()
        self.height = self.image.GetHeight()
        
    def Load(self):
        """!Load image to texture"""  
        if self.image.HasAlpha():
            bytesPerPixel = 4
        else:
            bytesPerPixel = 3
        bytes = bytesPerPixel * self.width * self.height
        rev_val = self.height - 1
        im = (c_ubyte * bytes)()
        bytes3 = 3 * self.width * self.height
        bytes1 = self.width * self.height
        imageData = struct.unpack(str(bytes3) + 'B', self.image.GetData())
        if self.image.HasAlpha():
            alphaData = struct.unpack(str(bytes1) + 'B', self.image.GetAlphaData())
        
        # this takes too much time
        wx.BeginBusyCursor()
        for i in range(self.height):
            for j in range(self.width):
                im[(j + i * self.width) * bytesPerPixel + 0] = imageData[( j + (rev_val - i) * self.width) * 3 + 0]
                im[(j + i * self.width) * bytesPerPixel + 1] = imageData[( j + (rev_val - i) * self.width) * 3 + 1]
                im[(j + i * self.width) * bytesPerPixel + 2] = imageData[( j + (rev_val - i) * self.width) * 3 + 2]
                if self.image.HasAlpha():
                    im[(j + i * self.width) * bytesPerPixel + 3] = alphaData[( j + (rev_val - i) * self.width)]
        wx.EndBusyCursor()
        
        id = Nviz_load_image(im, self.width, self.height, self.image.HasAlpha())
        
        return id
        
    def Draw(self):
        """!Draw texture as an image"""
        Nviz_draw_image(self.coords[0], self.coords[1], self.width, self.height, self.textureId)
    
        
    def SetBounds(self, rect):
        """!Set Bounding Rectangle"""
        self.bounds = rect
        
    def HitTest(self, x, y, radius):
        copy = wx.Rect(*self.bounds)
        copy.Inflate(radius, radius)
        return copy.ContainsXY(x, y)
    
    def MoveTexture(self, dx, dy):
        """!Move texture on the screen"""
        self.coords[0] += dx
        self.coords[1] += dy
        self.bounds.OffsetXY(dx, dy)
    
    def SetCoords(self, coords):
        """!Set coordinates"""
        dx = coords[0] - self.coords[0]
        dy = coords[1] - self.coords[1]
        self.MoveTexture(dx, dy)
        
    def GetId(self):
        """!Returns image id."""
        return self.id
    
    def SetActive(self, active = True):
        self.active = active
        
    def IsActive(self):
        return self.active
        
class ImageTexture(Texture):
    """!Class representing OpenGL texture as an overlay image"""
    def __init__(self, filepath, overlayId, coords, cmd):
        """!Load image to texture

        @param filepath path to image file
        @param overlayId id of overlay (1 for legend)
        @param coords image coordinates
        @param cmd d.legend command      
        """
        Texture.__init__(self, filepath = filepath, overlayId = overlayId, coords = coords)
        
        self.cmd = cmd
        
    def GetCmd(self):
        """!Returns overlay command."""
        return self.cmd
        
    def Corresponds(self, item):
        return sorted(self.GetCmd()) == sorted(item.GetCmd())
        
class TextTexture(Texture):
    """!Class representing OpenGL texture as a text label"""
    def __init__(self, filepath, overlayId, coords, textDict):
        """!Load image to texture

        @param filepath path to image file
        @param overlayId id of overlay (101 and more for text)
        @param coords text coordinates
        @param textDict text properties      
        """
        Texture.__init__(self, filepath = filepath, overlayId = overlayId, coords = coords)
        
        self.textDict = textDict
        
    def GetTextDict(self):
        """!Returns text properties."""
        return self.textDict
        
        
    def Corresponds(self, item):
        t = self.GetTextDict()
        for prop in t.keys():
            if prop in ('coords','bbox'): continue
            if t[prop] != item[prop]:
                return False
                
        return True
    

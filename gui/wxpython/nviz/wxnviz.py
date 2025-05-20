"""
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

from __future__ import annotations

import locale
import struct
import sys
from math import sqrt
from typing import TYPE_CHECKING, Literal, TypedDict, overload

import wx
from core.debug import Debug
from core.gcmd import DecodeString
from core.globalvar import wxPythonPhoenix
from core.utils import autoCropImageFromFile
from gui_core.wrap import Rect

import grass.script as gs

try:
    from ctypes import (
        CFUNCTYPE,
        byref,
        c_char_p,
        c_double,
        c_float,
        c_int,
        c_ubyte,
        create_string_buffer,
        pointer,
    )
except KeyError as e:
    print("wxnviz.py: {}".format(e), file=sys.stderr)

try:
    from grass.lib.ctypes_preamble import UNCHECKED, String
    from grass.lib.gis import (
        Colors,
        G_find_raster2,
        G_find_raster3d,
        G_find_vector2,
        G_free,
        G_fully_qualified_name,
        G_gisinit,
        G_set_error_routine,
        G_set_percent_routine,
        G_unset_error_routine,
        G_unset_percent_routine,
        G_unset_window,
        G_warning,
    )
    from grass.lib.nviz import (
        DRAW_QUICK_SURFACE,
        DRAW_QUICK_VLINES,
        DRAW_QUICK_VOLUME,
        DRAW_QUICK_VPOINTS,
        MAP_OBJ_SITE,
        MAP_OBJ_SURF,
        MAP_OBJ_UNDEFINED,
        MAP_OBJ_VECT,
        MAP_OBJ_VOL,
        Nviz_change_exag,
        Nviz_color_from_str,
        Nviz_del_texture,
        Nviz_delete_arrow,
        Nviz_delete_scalebar,
        Nviz_draw_all,
        Nviz_draw_arrow,
        Nviz_draw_cplane,
        Nviz_draw_fringe,
        Nviz_draw_image,
        Nviz_draw_model,
        Nviz_draw_quick,
        Nviz_draw_scalebar,
        Nviz_flythrough,
        Nviz_get_bgcolor,
        Nviz_get_cplane_rotation,
        Nviz_get_cplane_translation,
        Nviz_get_current_cplane,
        Nviz_get_exag,
        Nviz_get_exag_height,
        Nviz_get_focus,
        Nviz_get_longdim,
        Nviz_get_max_texture,
        Nviz_get_modelview,
        Nviz_get_viewpoint_height,
        Nviz_get_viewpoint_position,
        Nviz_get_xyrange,
        Nviz_get_zrange,
        Nviz_has_focus,
        Nviz_init_data,
        Nviz_init_rotation,
        Nviz_init_view,
        Nviz_load_image,
        Nviz_look_here,
        Nviz_new_map_obj,
        Nviz_num_cplanes,
        Nviz_off_cplane,
        Nviz_on_cplane,
        Nviz_resize_window,
        Nviz_set_2D,
        Nviz_set_arrow,
        Nviz_set_attr,
        Nviz_set_bgcolor,
        Nviz_set_cplane_here,
        Nviz_set_cplane_rotation,
        Nviz_set_cplane_translation,
        Nviz_set_fence_color,
        Nviz_set_focus,
        Nviz_set_focus_map,
        Nviz_set_fringe,
        Nviz_set_light_ambient,
        Nviz_set_light_bright,
        Nviz_set_light_color,
        Nviz_set_light_position,
        Nviz_set_rotation,
        Nviz_set_scalebar,
        Nviz_set_surface_attr_default,
        Nviz_set_viewpoint_height,
        Nviz_set_viewpoint_persp,
        Nviz_set_viewpoint_position,
        Nviz_set_viewpoint_twist,
        Nviz_unset_attr,
        Nviz_unset_rotation,
        nv_data,
    )
    from grass.lib.ogsf import (
        ATT_COLOR,
        ATT_EMIT,
        ATT_MASK,
        ATT_SHINE,
        ATT_TOPO,
        ATT_TRANSP,
        CONST_ATT,
        DM_FLAT,
        DM_GOURAUD,
        DM_GRID_SURF,
        DM_GRID_WIRE,
        DM_POLY,
        DM_WIRE,
        DM_WIRE_POLY,
        MAP_ATT,
        MAX_ISOSURFS,
        GP_delete_site,
        GP_get_sitename,
        GP_select_surf,
        GP_set_style,
        GP_set_style_thematic,
        GP_set_trans,
        GP_set_zmode,
        GP_site_exists,
        GP_unselect_surf,
        GP_unset_style_thematic,
        GS_clear,
        GS_delete_surface,
        GS_get_cat_at_xy,
        GS_get_distance_alongsurf,
        GS_get_rotation_matrix,
        GS_get_selected_point_on_surface,
        GS_get_surf_list,
        GS_get_trans,
        GS_get_val_at_xy,
        GS_get_viewdir,
        GS_libinit,
        GS_num_surfs,
        GS_set_att_const,
        GS_set_drawmode,
        GS_set_drawres,
        GS_set_rotation_matrix,
        GS_set_trans,
        GS_set_viewdir,
        GS_set_wire_color,
        GS_setall_drawmode,
        GS_setall_drawres,
        GS_surf_exists,
        GS_write_ppm,
        GS_write_tif,
        GV_delete_vector,
        GV_get_vectname,
        GV_select_surf,
        GV_set_style,
        GV_set_style_thematic,
        GV_set_trans,
        GV_surf_is_selected,
        GV_unselect_surf,
        GV_unset_style_thematic,
        GV_vect_exists,
        GVL_delete_vol,
        GVL_get_trans,
        GVL_init_region,
        GVL_isosurf_add,
        GVL_isosurf_del,
        GVL_isosurf_move_down,
        GVL_isosurf_move_up,
        GVL_isosurf_num_isosurfs,
        GVL_isosurf_set_att_const,
        GVL_isosurf_set_att_map,
        GVL_isosurf_set_drawmode,
        GVL_isosurf_set_drawres,
        GVL_isosurf_set_flags,
        GVL_isosurf_unset_att,
        GVL_libinit,
        GVL_set_draw_wire,
        GVL_set_trans,
        GVL_slice_add,
        GVL_slice_del,
        GVL_slice_move_down,
        GVL_slice_move_up,
        GVL_slice_num_slices,
        GVL_slice_set_drawmode,
        GVL_slice_set_drawres,
        GVL_slice_set_pos,
        GVL_slice_set_transp,
        GVL_vol_exists,
    )
    from grass.lib.raster import Rast__init_window, Rast_unset_window
    from grass.lib.vector import Vect_read_colors
except (ImportError, OSError, TypeError) as e:
    print("wxnviz.py: {}".format(e), file=sys.stderr)
try:
    from numpy import matrix
except ImportError:
    msg = _(
        "This module requires the NumPy module, which could not be "
        "imported. It probably is not installed (it's not part of the "
        "standard Python distribution). See the Numeric Python site "
        "(https://numpy.org) for information on downloading source or "
        "binaries."
    )
    print("wxnviz.py: " + msg, file=sys.stderr)

if TYPE_CHECKING:
    from collections.abc import Iterable, Mapping

    from _typeshed import StrPath


log = None
progress = None

PointId = int
"""Point set id, as used with GP_site_exists(id)"""

VectorId = int
"""Vector set id, as used with GV_vect_exists(id)"""

VolumeId = int
"""Volume set id, as used with GVL_vol_exists(id)"""

SurfaceId = int
"""Surface id, as used with GS_surf_exists(id)"""

IsosurfaceId = int
"""Isosurface id (0 - MAX_ISOSURFS), as used with GVL_isosurf_get_att, for isosurf_id"""

SliceId = int
"""Slice id, as used with volume sets in GVL_slice_del(id, slice_id)"""

ClipPlaneId = int
"""Clip plane id (cplane), as returned by Nviz_get_current_cplane()"""


class QueryMapResult(TypedDict):
    id: SurfaceId
    x: int
    y: int
    z: int
    elevation: str
    color: str


def print_error(msg, type):
    """Redirect stderr"""
    global log
    if log:
        msg = DecodeString(msg.data)
        log.write(msg)
    else:
        print(msg)

    return 0


def print_progress(value):
    """Redirect progress info"""
    global progress
    if progress:
        if progress.GetRange() != 100:
            progress.SetRange(100)
        progress.SetValue(value)
    else:
        print(value)

    return 0


try:
    errtype = CFUNCTYPE(UNCHECKED(c_int), String, c_int)
    errfunc = errtype(print_error)
    pertype = CFUNCTYPE(UNCHECKED(c_int), c_int)
    perfunc = pertype(print_progress)
except NameError:
    pass


class Nviz:
    def __init__(self, glog, gprogress) -> None:
        """Initialize Nviz class instance

        :param glog: logging area
        :param gprogress: progressbar
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
        self.width: int
        self.height: int
        self.width = self.height = -1
        self.showLight = False

        Debug.msg(1, "Nviz::Nviz()")

    def __del__(self):
        """Destroy Nviz class instance"""
        G_unset_error_routine()
        G_unset_percent_routine()
        del self.data
        del self.data_obj
        self.log = None

    def Init(self):
        """Initialize window"""
        if sys.platform != "win32":
            locale.setlocale(locale.LC_NUMERIC, "C")
        G_unset_window()
        Rast_unset_window()
        Rast__init_window()
        GS_libinit()
        GVL_libinit()
        GVL_init_region()

    def ResizeWindow(self, width: int, height: int, scale: float = 1) -> Literal[1, 0]:
        """GL canvas resized

        :param width: window width
        :param height: window height

        :return: 1 on success
        :return: 0 on failure (window resized by default to 20x20 px)
        """
        self.width = int(width * scale)
        self.height = int(height * scale)
        Debug.msg(
            3, "Nviz::ResizeWindow(): width=%d height=%d", self.width, self.height
        )
        return Nviz_resize_window(self.width, self.height)

    def GetLongDim(self):
        """Get longest dimension, used for initial size of north arrow"""
        return Nviz_get_longdim(self.data)

    def SetViewDefault(self) -> tuple[float, float, float, float]:
        """Set default view (based on loaded data)

        :return: z-exag value, default, min and max height
        """
        # determine z-exag
        z_exag: float = Nviz_get_exag()
        Nviz_change_exag(self.data, z_exag)

        # determine height
        hdef = c_double()
        hmin = c_double()
        hmax = c_double()
        Nviz_get_exag_height(byref(hdef), byref(hmin), byref(hmax))

        Debug.msg(
            1,
            "Nviz::SetViewDefault(): hdef=%f, hmin=%f, hmax=%f",
            hdef.value,
            hmin.value,
            hmax.value,
        )

        return (z_exag, hdef.value, hmin.value, hmax.value)

    def SetView(self, x, y, height, persp, twist):
        """Change view settings
        :param x,y: position
        :param height:
        :param persp: perspective
        :param twist:
        """
        Nviz_set_viewpoint_height(height)
        Nviz_set_viewpoint_position(x, y)
        Nviz_set_viewpoint_twist(twist)
        Nviz_set_viewpoint_persp(persp)

        Debug.msg(
            3,
            "Nviz::SetView(): x=%f, y=%f, height=%f, persp=%f, twist=%f",
            x,
            y,
            height,
            persp,
            twist,
        )

    def GetViewpointPosition(self) -> tuple[float, float, float]:
        x = c_double()
        y = c_double()
        h = c_double()
        Nviz_get_viewpoint_height(byref(h))
        Nviz_get_viewpoint_position(byref(x), byref(y))

        return (x.value, y.value, h.value)

    def LookHere(self, x, y, scale: float = 1) -> None:
        """Look here feature
        :param x,y: screen coordinates
        """

        Nviz_look_here(int(x * scale), int(y * scale))
        Debug.msg(3, "Nviz::LookHere(): x=%f, y=%f", x * scale, y * scale)

    def LookAtCenter(self):
        """Center view at center of displayed surface"""
        Nviz_set_focus_map(MAP_OBJ_UNDEFINED, -1)
        Debug.msg(3, "Nviz::LookAtCenter()")

    def GetFocus(
        self,
    ) -> tuple[float, float, float] | tuple[Literal[-1], Literal[-1], Literal[-1]]:
        """Get focus"""
        Debug.msg(3, "Nviz::GetFocus()")
        if not Nviz_has_focus(self.data):
            return (-1, -1, -1)
        x = c_float()
        y = c_float()
        z = c_float()
        Nviz_get_focus(self.data, byref(x), byref(y), byref(z))
        return (x.value, y.value, z.value)

    def SetFocus(self, x: float, y: float, z: float) -> None:
        """Set focus"""
        Debug.msg(3, "Nviz::SetFocus()")
        Nviz_set_focus(self.data, x, y, z)

    def GetViewdir(self) -> tuple[float, float, float]:
        """Get viewdir"""
        Debug.msg(3, "Nviz::GetViewdir()")
        dir = (c_float * 3)()
        GS_get_viewdir(byref(dir))

        return dir[0], dir[1], dir[2]

    def SetViewdir(self, x: float, y: float, z: float) -> None:
        """Set viewdir"""
        Debug.msg(3, "Nviz::SetViewdir(): x=%f, y=%f, z=%f" % (x, y, z))
        dir = (c_float * 3)()
        for i, coord in enumerate((x, y, z)):
            dir[i] = coord
        GS_set_viewdir(byref(dir))

    def SetZExag(self, z_exag: float) -> Literal[1]:
        """Set z-exag value

        :param z_exag: value

        :return: 1
        """
        Debug.msg(3, "Nviz::SetZExag(): z_exag=%f", z_exag)
        return Nviz_change_exag(self.data, z_exag)

    def Draw(self, quick: bool, quick_mode: int) -> None:
        """Draw canvas

        Draw quick mode:
         - DRAW_QUICK_SURFACE
         - DRAW_QUICK_VLINES
         - DRAW_QUICK_VPOINTS
         - DRAW_QUICK_VOLUME

        :param quick: if true draw in wiremode
        :param quick_mode: quick mode
        """
        Debug.msg(3, "Nviz::Draw(): quick=%d", quick)

        Nviz_draw_cplane(self.data, -1, -1)  # ?

        if quick:
            Nviz_draw_quick(self.data, quick_mode)
        else:
            Nviz_draw_all(self.data)

    def EraseMap(self) -> None:
        """Erase map display (with background color)"""
        Debug.msg(1, "Nviz::EraseMap()")
        GS_clear(Nviz_get_bgcolor(self.data))

    def InitView(self) -> None:
        """Initialize view"""
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

    def SetBgColor(self, color_str: str) -> None:
        """Set background color

        :param str color_str: color string
        """
        Nviz_set_bgcolor(self.data, Nviz_color_from_str(color_str))

    def SetLight(
        self,
        x: float,
        y: float,
        z: float,
        color,
        bright: float,
        ambient: float,
        w: float = 0,
        lid: int = 1,
    ) -> None:
        """Change lighting settings

        :param x,y,z: position
        :param color: light color (as string)
        :param bright: light brightness
        :param ambient: light ambient
        :param w: local coordinate (default to 0)
        :param lid: light id
        """
        Nviz_set_light_position(self.data, lid, x, y, z, w)
        Nviz_set_light_bright(self.data, lid, bright)
        Nviz_set_light_color(
            self.data, lid, int(color[0]), int(color[1]), int(color[2])
        )
        Nviz_set_light_ambient(self.data, lid, ambient)

    def LoadSurface(self, name, color_name, color_value):
        """Load raster map (surface)

        :param name: raster map name
        :param color_name: raster map for color (None for color_value)
        :param color_value: color string (named color or RGB triptet)

        :return: object id
        :return: -1 on failure
        """
        mapset = G_find_raster2(name, "")
        if mapset is None:
            G_warning(_("Raster map <%s> not found"), name)
            return -1

        # topography
        id = Nviz_new_map_obj(
            MAP_OBJ_SURF, G_fully_qualified_name(name, mapset), 0.0, self.data
        )

        if color_name:  # check for color map
            mapset = G_find_raster2(color_name, "")
            if mapset is None:
                G_warning(_("Raster map <%s> not found"), color_name)
                GS_delete_surface(id)
                return -1

            Nviz_set_attr(
                id,
                MAP_OBJ_SURF,
                ATT_COLOR,
                MAP_ATT,
                G_fully_qualified_name(color_name, mapset),
                -1.0,
                self.data,
            )

        elif color_value:  # check for color value
            Nviz_set_attr(
                id,
                MAP_OBJ_SURF,
                ATT_COLOR,
                CONST_ATT,
                None,
                Nviz_color_from_str(color_value),
                self.data,
            )

        else:  # use by default elevation map for coloring
            Nviz_set_attr(
                id,
                MAP_OBJ_SURF,
                ATT_COLOR,
                MAP_ATT,
                G_fully_qualified_name(name, mapset),
                -1.0,
                self.data,
            )

        # if (i > 1)
        #     set_default_wirecolors(self.data, i)

        # focus on loaded self.data
        Nviz_set_focus_map(MAP_OBJ_UNDEFINED, -1)

        Debug.msg(1, "Nviz::LoadRaster(): name=%s -> id=%d", name, id)

        return id

    def AddConstant(self, value, color):
        """Add new constant surface"""
        id = Nviz_new_map_obj(MAP_OBJ_SURF, None, value, self.data)

        Nviz_set_attr(
            id,
            MAP_OBJ_SURF,
            ATT_COLOR,
            CONST_ATT,
            None,
            Nviz_color_from_str(color),
            self.data,
        )
        Nviz_set_focus_map(MAP_OBJ_UNDEFINED, -1)

        Debug.msg(1, "Nviz::AddConstant(): id=%d", id)
        return id

    def UnloadSurface(self, id: SurfaceId) -> Literal[1, 0]:
        """Unload surface

        :param id: surface id

        :return: 1 on success
        :return: 0 on failure
        """
        if not GS_surf_exists(id):
            return 0

        Debug.msg(1, "Nviz::UnloadSurface(): id=%d", id)

        if GS_delete_surface(id) < 0:
            return 0

        return 1

    def LoadVector(self, name, points):
        """Load vector map overlay

        :param name: vector map name
        :param points: if true load 2d points rather then 2d lines

        :return: object id, id of base surface (or -1 if it is not loaded)
        :return: -1 on failure
        """
        baseId = -1
        if GS_num_surfs() == 0:  # load base surface if no loaded
            baseId = Nviz_new_map_obj(MAP_OBJ_SURF, None, 0.0, self.data)

            nsurf = c_int()
            surf_list = GS_get_surf_list(byref(nsurf))
            GS_set_att_const(surf_list[0], ATT_TRANSP, 255)

        mapset = G_find_vector2(name, "")
        if mapset is None:
            G_warning(_("Vector map <%s> not found"), name)

        if points:
            id = Nviz_new_map_obj(
                MAP_OBJ_SITE, G_fully_qualified_name(name, mapset), 0.0, self.data
            )
        else:
            id = Nviz_new_map_obj(
                MAP_OBJ_VECT, G_fully_qualified_name(name, mapset), 0.0, self.data
            )

        Debug.msg(1, "Nviz::LoadVector(): name=%s -> id=%d", name, id)

        return id, baseId

    @overload
    def UnloadVector(self, id: PointId, points: Literal[True]) -> Literal[1, 0]:
        pass

    @overload
    def UnloadVector(self, id: VectorId, points: Literal[False]) -> Literal[1, 0]:
        pass

    def UnloadVector(self, id: PointId | VectorId, points: bool) -> Literal[1, 0]:
        """Unload vector set

        :param id: vector set id
        :param points: vector points or lines set

        :return: 1 on success
        :return: 0 on failure
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
        """Check if surface is selected (currently unused)

        :param vid: vector id
        :param sid: surface id

        :return: True if selected
        :return: False if not selected
        """
        selected = GV_surf_is_selected(vid, sid)
        Debug.msg(
            1,
            "Nviz::VectorSurfaceSelected(): vid=%s, sid=%d -> selected=%d",
            vid,
            sid,
            selected,
        )
        return selected

    def LoadVolume(self, name, color_name, color_value):
        """Load 3d raster map (volume)

        :param name: 3d raster map name
        :param color_name: 3d raster map for color (None for color_value)
        :param color_value: color string (named color or RGB triptet)

        :return: object id
        :return: -1 on failure
        """
        mapset = G_find_raster3d(name, "")
        if mapset is None:
            G_warning(_("3d raster map <%s> not found"), name)
            return -1

        # topography
        id = Nviz_new_map_obj(
            MAP_OBJ_VOL, G_fully_qualified_name(name, mapset), 0.0, self.data
        )

        if color_name:  # check for color map
            mapset = G_find_raster3d(color_name, "")
            if mapset is None:
                G_warning(_("3d raster map <%s> not found"), color_name)
                GVL_delete_vol(id)
                return -1

            Nviz_set_attr(
                id,
                MAP_OBJ_VOL,
                ATT_COLOR,
                MAP_ATT,
                G_fully_qualified_name(color_name, mapset),
                -1.0,
                self.data,
            )
        elif color_value:  # check for color value
            Nviz_set_attr(
                id,
                MAP_OBJ_VOL,
                ATT_COLOR,
                CONST_ATT,
                None,
                Nviz_color_from_str(color_value),
                self.data,
            )
        else:  # use by default elevation map for coloring
            Nviz_set_attr(
                id,
                MAP_OBJ_VOL,
                ATT_COLOR,
                MAP_ATT,
                G_fully_qualified_name(name, mapset),
                -1.0,
                self.data,
            )

        Debug.msg(1, "Nviz::LoadVolume(): name=%s -> id=%d", name, id)

        return id

    def UnloadVolume(self, id: VolumeId) -> Literal[0, 1]:
        """Unload volume

        :param id: volume id

        :return: 1 on success
        :return: 0 on failure
        """
        if not GVL_vol_exists(id):
            return 0

        Debug.msg(1, "Nviz::UnloadVolume(): id=%d", id)

        if GVL_delete_vol(id) < 0:
            return 0

        return 1

    @overload
    def SetSurfaceTopo(
        self, id: SurfaceId, map: Literal[True], value: str
    ) -> Literal[1, -1, -2]:
        pass

    @overload
    def SetSurfaceTopo(
        self, id: SurfaceId, map: Literal[False], value: float
    ) -> Literal[1, -1, -2]:
        pass

    def SetSurfaceTopo(
        self, id: SurfaceId, map: bool, value: str | float
    ) -> Literal[1, -1, -2]:
        """Set surface topography

        :param id: surface id
        :param map: if true use map otherwise constant
        :param value: map name of value

        :return: 1 on success
        :return: -1 surface not found
        :return: -2 setting attributes failed
        """
        return self.SetSurfaceAttr(id, ATT_TOPO, map, value)

    @overload
    def SetSurfaceColor(
        self, id: SurfaceId, map: Literal[True], value: str
    ) -> Literal[1, -1, -2]:
        pass

    @overload
    def SetSurfaceColor(
        self, id: SurfaceId, map: Literal[False], value: float
    ) -> Literal[1, -1, -2]:
        pass

    def SetSurfaceColor(
        self, id: SurfaceId, map: bool, value: str | float
    ) -> Literal[1, -1, -2]:
        """Set surface color

        :param id: surface id
        :param map: if true use map otherwise constant
        :param value: map name or value

        :return: 1 on success
        :return: -1 surface not found
        :return: -2 setting attributes failed
        """
        return self.SetSurfaceAttr(id, ATT_COLOR, map, value)

    def SetSurfaceMask(
        self, id: SurfaceId, invert: bool, value: str
    ) -> Literal[1, -1, -2]:
        """Set surface mask

        .. todo::
            invert

        :param id: surface id
        :param invert: if true invert mask (unimplemented, always true)
        :param value: map name of value

        :return: 1 on success
        :return: -1 surface not found
        :return: -2 setting attributes failed
        """
        return self.SetSurfaceAttr(id, ATT_MASK, True, value)

    @overload
    def SetSurfaceTransp(
        self, id: SurfaceId, map: Literal[True], value: str
    ) -> Literal[1, -1, -2]:
        pass

    @overload
    def SetSurfaceTransp(
        self, id: SurfaceId, map: Literal[False], value: float
    ) -> Literal[1, -1, -2]:
        pass

    def SetSurfaceTransp(
        self, id: SurfaceId, map: bool, value: str | float
    ) -> Literal[1, -1, -2]:
        """Set surface mask

        ..todo::
            invert

        :param id: surface id
        :param map: if true use map otherwise constant
        :param value: map name of value

        :return: 1 on success
        :return: -1 surface not found
        :return: -2 setting attributes failed
        """
        return self.SetSurfaceAttr(id, ATT_TRANSP, map, value)

    @overload
    def SetSurfaceShine(
        self, id: SurfaceId, map: Literal[True], value: str
    ) -> Literal[1, -1, -2]:
        pass

    @overload
    def SetSurfaceShine(
        self, id: SurfaceId, map: Literal[False], value: float
    ) -> Literal[1, -1, -2]:
        pass

    def SetSurfaceShine(
        self, id: SurfaceId, map: bool, value: str | float
    ) -> Literal[1, -1, -2]:
        """Set surface shininess

        :param id: surface id
        :param map: if true use map otherwise constant
        :param value: map name of value

        :return: 1 on success
        :return: -1 surface not found
        :return: -2 setting attributes failed
        """
        return self.SetSurfaceAttr(id, ATT_SHINE, map, value)

    @overload
    def SetSurfaceEmit(
        self, id: SurfaceId, map: Literal[True], value: str
    ) -> Literal[1, -1, -2]:
        pass

    @overload
    def SetSurfaceEmit(
        self, id: SurfaceId, map: Literal[False], value: float
    ) -> Literal[1, -1, -2]:
        pass

    def SetSurfaceEmit(
        self, id: SurfaceId, map: bool, value: str | float
    ) -> Literal[1, -1, -2]:
        """Set surface emission (currently unused)

        :param id: surface id
        :param map: if true use map otherwise constant
        :param value: map name of value

        :return: 1 on success
        :return: -1 surface not found
        :return: -2 setting attributes failed
        """
        return self.SetSurfaceAttr(id, ATT_EMIT, map, value)

    @overload
    def SetSurfaceAttr(
        self, id: SurfaceId, attr: int, map: Literal[True], value: str
    ) -> Literal[1, -1, -2]:
        pass

    @overload
    def SetSurfaceAttr(
        self, id: SurfaceId, attr: int, map: Literal[False], value: float
    ) -> Literal[1, -1, -2]:
        pass

    @overload
    def SetSurfaceAttr(
        self, id: SurfaceId, attr: Literal[2], map: Literal[False], value: str
    ) -> Literal[1, -1, -2]:
        pass

    @overload
    def SetSurfaceAttr(
        self, id: SurfaceId, attr: int, map: bool, value: str | float
    ) -> Literal[1, -1, -2]:
        pass

    def SetSurfaceAttr(
        self, id: SurfaceId, attr: int, map: bool, value: str | float
    ) -> Literal[1, -1, -2]:
        """Set surface attribute

        :param id: surface id
        :param attr: attribute desc
        :param map: if true use map otherwise constant
        :param value: map name of value

        :return: 1 on success
        :return: -1 surface not found
        :return: -2 setting attributes failed
        """
        if not GS_surf_exists(id):
            return -1

        if map:
            ret = Nviz_set_attr(id, MAP_OBJ_SURF, attr, MAP_ATT, value, -1.0, self.data)
        else:
            val: int | float = (
                Nviz_color_from_str(value) if attr == ATT_COLOR else float(value)
            )
            ret = Nviz_set_attr(id, MAP_OBJ_SURF, attr, CONST_ATT, None, val, self.data)

        Debug.msg(
            3,
            "Nviz::SetSurfaceAttr(): id=%d, attr=%d, map=%d, value=%s",
            id,
            attr,
            map,
            value,
        )

        if ret < 0:
            return -2

        return 1

    def UnsetSurfaceMask(self, id: SurfaceId) -> Literal[1, -1, -2]:
        """Unset surface mask

        :param id: surface id

        :return: 1 on success
        :return: -1 surface not found
        :return: -2 setting attributes failed
        :return: -1 on failure
        """
        return self.UnsetSurfaceAttr(id, ATT_MASK)

    def UnsetSurfaceTransp(self, id: SurfaceId) -> Literal[1, -1, -2]:
        """Unset surface transparency

        :param id: surface id

        :return: 1 on success
        :return: -1 surface not found
        :return: -2 setting attributes failed
        """
        return self.UnsetSurfaceAttr(id, ATT_TRANSP)

    def UnsetSurfaceEmit(self, id: SurfaceId) -> Literal[1, -1, -2]:
        """Unset surface emission (currently unused)

        :param id: surface id

        :return: 1 on success
        :return: -1 surface not found
        :return: -2 setting attributes failed
        """
        return self.UnsetSurfaceAttr(id, ATT_EMIT)

    def UnsetSurfaceAttr(self, id: SurfaceId, attr: int) -> Literal[1, -1, -2]:
        """Unset surface attribute

        :param id: surface id
        :param attr: attribute descriptor

        :return: 1 on success
        :return: -1 surface not found
        :return: -2 setting attributes failed
        """
        if not GS_surf_exists(id):
            return -1

        Debug.msg(3, "Nviz::UnsetSurfaceAttr(): id=%d, attr=%d", id, attr)

        ret = Nviz_unset_attr(id, MAP_OBJ_SURF, attr)

        if ret < 0:
            return -2

        return 1

    def SetSurfaceRes(
        self, id: SurfaceId, fine: int, coarse: int
    ) -> Literal[1, -1, -2]:
        """Set surface resolution

        :param id: surface id
        :param fine: x/y fine resolution
        :param coarse: x/y coarse resolution

        :return: 1 on success
        :return: -1 surface not found
        :return: -2 setting attributes failed
        """
        Debug.msg(
            3, "Nviz::SetSurfaceRes(): id=%d, fine=%d, coarse=%d", id, fine, coarse
        )

        if id > 0:
            if not GS_surf_exists(id):
                return -1

            if GS_set_drawres(id, fine, fine, coarse, coarse) < 0:
                return -2
        else:
            GS_setall_drawres(fine, fine, coarse, coarse)

        return 1

    def SetSurfaceStyle(self, id: SurfaceId, style: int) -> Literal[1, -1, -2]:
        """Set draw style

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

        :param id: surface id (<= 0 for all)
        :param style: draw style

        :return: 1 on success
        :return: -1 surface not found
        :return: -2 setting attributes failed
        """
        Debug.msg(3, "Nviz::SetSurfaceStyle(): id=%d, style=%d", id, style)

        if id > 0:
            if not GS_surf_exists(id):
                return -1

            if GS_set_drawmode(id, style) < 0:
                return -2

            return 1

        if GS_setall_drawmode(style) < 0:
            return -2

        return 1

    def SetWireColor(self, id: SurfaceId, color_str: str) -> Literal[1, -1]:
        """Set color of wire

        .. todo::
            all

        :param id: surface id (< 0 for all)
        :param color_str: color string (R:G:B)

        :return: 1 on success
        :return: -1 surface not found
        :return: -2 setting attributes failed
        :return: 0 on failure
        """
        Debug.msg(3, "Nviz::SetWireColor(): id=%d, color=%s", id, color_str)

        color: int = Nviz_color_from_str(color_str)

        if id > 0:
            if not GS_surf_exists(id):
                return -1

            GS_set_wire_color(id, color)
        else:
            nsurfs = c_int()
            surf_list = GS_get_surf_list(byref(nsurfs))
            for i in range(nsurfs.value):
                id = surf_list[i]
                GS_set_wire_color(id, color)

            G_free(surf_list)
            surf_list = None

        return 1

    def GetSurfacePosition(
        self, id: SurfaceId
    ) -> tuple[()] | tuple[float, float, float]:
        """Get surface position

        :param id: surface id

        :return: x,y,z
        :return: zero-length vector on error
        """
        if not GS_surf_exists(id):
            return ()

        x, y, z = c_float(), c_float(), c_float()
        GS_get_trans(id, byref(x), byref(y), byref(z))

        Debug.msg(
            3,
            "Nviz::GetSurfacePosition(): id=%d, x=%f, y=%f, z=%f",
            id,
            x.value,
            y.value,
            z.value,
        )

        return (x.value, y.value, z.value)

    def SetSurfacePosition(
        self, id: SurfaceId, x: float, y: float, z: float
    ) -> Literal[1, -1]:
        """Set surface position

        :param id: surface id
        :param x,y,z: translation values

        :return: 1 on success
        :return: -1 surface not found
        :return: -2 setting position failed
        """
        if not GS_surf_exists(id):
            return -1

        Debug.msg(3, "Nviz::SetSurfacePosition(): id=%d, x=%f, y=%f, z=%f", id, x, y, z)

        GS_set_trans(id, x, y, z)

        return 1

    def SetVectorLineMode(
        self, id: VectorId, color_str: str, width: int, use_z: bool | int
    ) -> Literal[1, -1, -2]:
        """Set mode of vector line overlay

        :param id: vector id
        :param color_str: color string
        :param width: line width
        :param use_z: display 3d or on surface, true or non-zero to use z

        :return: -1 vector set not found
        :return: -2 on failure
        :return: 1 on success
        """
        if not GV_vect_exists(id):
            return -1

        Debug.msg(
            3,
            "Nviz::SetVectorMode(): id=%d, color=%s, width=%d, use_z=%d",
            id,
            color_str,
            width,
            use_z,
        )

        color: int = Nviz_color_from_str(color_str)

        # use memory by default
        if GV_set_style(id, 1, color, width, use_z) < 0:
            return -2

        return 1

    def SetVectorLineHeight(self, id: VectorId, height: float) -> Literal[1, -1]:
        """Set vector height above surface (lines)

        :param id: vector set id
        :param height:

        :return: -1 vector set not found
        :return: 1 on success
        """
        if not GV_vect_exists(id):
            return -1

        Debug.msg(3, "Nviz::SetVectorLineHeight(): id=%d, height=%f", id, height)

        GV_set_trans(id, 0.0, 0.0, height)

        return 1

    def SetVectorLineSurface(
        self, id: VectorId, surf_id: SurfaceId
    ) -> Literal[1, -1, -2, -3]:
        """Set reference surface of vector set (lines)

        :param id: vector set id
        :param surf_id: surface id

        :return: 1 on success
        :return: -1 vector set not found
        :return: -2 surface not found
        :return: -3 on failure
        """
        if not GV_vect_exists(id):
            return -1

        if not GS_surf_exists(surf_id):
            return -2

        if GV_select_surf(id, surf_id) < 0:
            return -3

        return 1

    def UnsetVectorLineSurface(
        self, id: VectorId, surf_id: SurfaceId
    ) -> Literal[1, -1, -2, -3]:
        """Unset reference surface of vector set (lines)

        :param id: vector set id
        :param surf_id: surface id

        :return: 1 on success
        :return: -1 vector set not found
        :return: -2 surface not found
        :return: -3 on failure
        """
        if not GV_vect_exists(id):
            return -1

        if not GS_surf_exists(surf_id):
            return -2

        if GV_unselect_surf(id, surf_id) < 0:
            return -3

        return 1

    def SetVectorPointMode(
        self, id: PointId, color_str: str, width: int, size: float, marker: int
    ) -> Literal[1, -1, -2]:
        """Set mode of vector point overlay

        :param id: vector id
        :param color_str: color string
        :param width: line width
        :param size: size of the symbol
        :param marker: type of the symbol

        :return: -1 vector set not found
        """
        if not GP_site_exists(id):
            return -1

        # dtree and ctree defined but not used
        if marker > 5:
            marker += 2

        Debug.msg(
            3,
            "Nviz::SetVectorPointMode(): id=%d, color=%s, width=%d, size=%f, marker=%d",
            id,
            color_str,
            width,
            size,
            marker,
        )

        color: int = Nviz_color_from_str(color_str)

        if GP_set_style(id, color, width, size, marker) < 0:
            return -2

        return 1

    def SetVectorPointHeight(self, id: PointId, height: float) -> Literal[1, -1]:
        """Set vector height above surface (points)

        :param id: point set id
        :param height:

        :return: -1 vector set not found
        :return: 1 on success
        """
        if not GP_site_exists(id):
            return -1

        Debug.msg(3, "Nviz::SetVectorPointHeight(): id=%d, height=%f", id, height)

        GP_set_trans(id, 0.0, 0.0, height)

        return 1

    def SetVectorPointSurface(
        self, id: PointId, surf_id: SurfaceId
    ) -> Literal[1, -1, -2, -3]:
        """Set reference surface of vector set (points)

        :param id: vector set id
        :param surf_id: surface id

        :return: 1 on success
        :return: -1 vector set not found
        :return: -2 surface not found
        :return: -3 on failure
        """
        if not GP_site_exists(id):
            return -1

        if not GS_surf_exists(surf_id):
            return -2

        if GP_select_surf(id, surf_id) < 0:
            return -3

        return 1

    def ReadVectorColors(self, name: str, mapset: str) -> Literal[1, 0, -1]:
        r"""Read vector colors

        :param name: vector map name
        :param mapset: mapset name (empty string (\c "") for search path)

        :return: -1 on error
        :return: 0 if color table missing
        :return: 1 on success (color table found)
        """
        return Vect_read_colors(name, mapset, self.color)

    def CheckColorTable(
        self, id: PointId | VectorId, type: Literal["points", "lines"]
    ) -> Literal[1, 0, -1, -2]:
        """Check if color table exists.

        :param id: vector set id
        :param type: vector set type (lines/points)

        :return: 1 color table exists
        :return: 0 no color table found
        :return: -1 on error
        :return: -2 vector set not found
        """
        file = c_char_p()

        if type == "points":
            ret = GP_get_sitename(id, byref(file))
        elif type == "lines":
            ret = GV_get_vectname(id, byref(file))

        if ret < 0:
            return -2

        return self.ReadVectorColors(file, "")

    def SetPointsStyleThematic(
        self,
        id: PointId,
        layer: int,
        color: str | None = None,
        colorTable: bool = False,
        width: str | None = None,
        size: str | None = None,
        symbol: str | None = None,
    ) -> Literal[-1] | None:
        """Set thematic style for vector points

        :param id: vector set id
        :param layer: layer number for thematic mapping
        :param colorTable: use color table
        :param color: color column name
        :param width: width column name
        :param size: size column name
        :param symbol: symbol column name
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

    def SetLinesStyleThematic(
        self,
        id: VectorId,
        layer: int,
        color: str | None = None,
        colorTable: bool = False,
        width: str | None = None,
    ) -> Literal[-1] | None:
        """Set thematic style for vector lines

        :param id: vector set id
        :param layer: layer number for thematic mapping
        :param color: color column name
        :param colorTable: use color table
        :param width: width column name
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

    def UnsetLinesStyleThematic(self, id: VectorId) -> None:
        """Unset thematic style for vector lines"""
        GV_unset_style_thematic(id)

    def UnsetPointsStyleThematic(self, id: PointId) -> None:
        """Unset thematic style for vector points"""
        GP_unset_style_thematic(id)

    def UnsetVectorPointSurface(
        self, id: PointId, surf_id: SurfaceId
    ) -> Literal[1, -1, -2, -3]:
        """Unset reference surface of vector set (points)

        :param id: vector point set id
        :param surf_id: surface id

        :return: 1 on success
        :return: -1 vector set not found
        :return: -2 surface not found
        :return: -3 on failure
        """
        if not GP_site_exists(id):
            return -1

        if not GS_surf_exists(surf_id):
            return -2

        if GP_unselect_surf(id, surf_id) < 0:
            return -3

        return 1

    def SetVectorPointZMode(self, id: PointId, zMode: bool) -> Literal[1, 0, -1]:
        """Set z mode (use z coordinate or not)

        :param id: vector point set id
        :param zMode: bool

        :return: -1 on failure
        :return: 0 when no 3d
        :return: 1 on success
        """
        if not GP_site_exists(id):
            return -1

        return GP_set_zmode(id, int(zMode))

    def AddIsosurface(
        self, id: VolumeId, level: float, isosurf_id: IsosurfaceId | None = None
    ) -> Literal[1, -1]:
        """Add new isosurface

        :param id: volume id
        :param level: isosurface level (topography)
        :param isosurf_id: isosurface id

        :return: -1 on failure
        :return: 1 on success
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

    def AddSlice(
        self, id: VolumeId, slice_id: SliceId | None = None
    ) -> int | Literal[-1]:
        """Add new slice

        :param id: volume id
        :param slice_id: slice id

        :return: -1 on failure
        :return: number of slices
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

    def DeleteIsosurface(
        self, id: VolumeId, isosurf_id: IsosurfaceId
    ) -> Literal[1, -1, -2, -3]:
        """Delete isosurface

        :param id: volume id
        :param isosurf_id: isosurface id

        :return: 1 on success
        :return: -1 volume not found
        :return: -2 isosurface not found
        :return: -3 on failure
        """
        if not GVL_vol_exists(id):
            return -1

        if isosurf_id > GVL_isosurf_num_isosurfs(id):
            return -2

        ret = GVL_isosurf_del(id, isosurf_id)

        if ret < 0:
            return -3

        return 1

    def DeleteSlice(self, id: VolumeId, slice_id: SliceId) -> Literal[1, -1, -2, -3]:
        """Delete slice

        :param id: volume id
        :param slice_id: slice id

        :return: 1 on success
        :return: -1 volume not found
        :return: -2 slice not found
        :return: -3 on failure
        """
        if not GVL_vol_exists(id):
            return -1

        if slice_id > GVL_slice_num_slices(id):
            return -2

        ret = GVL_slice_del(id, slice_id)

        if ret < 0:
            return -3

        return 1

    def MoveIsosurface(
        self, id: VolumeId, isosurf_id: IsosurfaceId, up: bool
    ) -> Literal[1, -1, -2, -3]:
        """Move isosurface up/down in the list

        :param id: volume id
        :param isosurf_id: isosurface id
        :param up: if true move up otherwise down

        :return: 1 on success
        :return: -1 volume not found
        :return: -2 isosurface not found
        :return: -3 on failure
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

    def MoveSlice(
        self, id: VolumeId, slice_id: SliceId, up: bool
    ) -> Literal[1, -1, -2, -3]:
        """Move slice up/down in the list

        :param id: volume id
        :param slice_id: slice id
        :param up: if true move up otherwise down

        :return: 1 on success
        :return: -1 volume not found
        :return: -2 slice not found
        :return: -3 on failure
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

    @overload
    def SetIsosurfaceTopo(
        self, id: VolumeId, isosurf_id: IsosurfaceId, map: Literal[True], value: str
    ) -> Literal[1, -1, -2, -3]:
        pass

    @overload
    def SetIsosurfaceTopo(
        self, id: VolumeId, isosurf_id: IsosurfaceId, map: Literal[False], value: float
    ) -> Literal[1, -1, -2, -3]:
        pass

    def SetIsosurfaceTopo(
        self, id: VolumeId, isosurf_id: IsosurfaceId, map: bool, value: str | float
    ) -> Literal[1, -1, -2, -3]:
        """Set isosurface level

        :param id: volume id
        :param isosurf_id: isosurface id (0 - MAX_ISOSURFS)
        :param map: if true use map otherwise constant
        :param value: map name of value

        :return: 1 on success
        :return: -1 volume not found
        :return: -2 isosurface not found
        :return: -3 on failure
        """
        return self.SetIsosurfaceAttr(id, isosurf_id, ATT_TOPO, map, value)

    def SetIsosurfaceColor(
        self, id: VolumeId, isosurf_id: IsosurfaceId, map: bool, value: str
    ) -> Literal[1, -1, -2, -3]:
        """Set isosurface color

        :param id: volume id
        :param isosurf_id: isosurface id (0 - MAX_ISOSURFS)
        :param map: if true use map otherwise constant
        :param value: map name of value

        :return: 1 on success
        :return: -1 volume not found
        :return: -2 isosurface not found
        :return: -3 on failure
        """
        return self.SetIsosurfaceAttr(id, isosurf_id, ATT_COLOR, map, value)

    def SetIsosurfaceMask(
        self, id: VolumeId, isosurf_id: IsosurfaceId, invert: bool, value: str
    ) -> Literal[1, -1, -2, -3]:
        """Set isosurface mask

        .. todo::
            invert

        :param id: volume id
        :param isosurf_id: isosurface id (0 - MAX_ISOSURFS)
        :param invert: true for invert mask
        :param value: map name to be used for mask

        :return: 1 on success
        :return: -1 volume not found
        :return: -2 isosurface not found
        :return: -3 on failure
        """
        return self.SetIsosurfaceAttr(id, isosurf_id, ATT_MASK, True, value)

    @overload
    def SetIsosurfaceTransp(
        self, id: VolumeId, isosurf_id: IsosurfaceId, map: Literal[True], value: str
    ) -> Literal[1, -1, -2]:
        pass

    @overload
    def SetIsosurfaceTransp(
        self, id: VolumeId, isosurf_id: IsosurfaceId, map: Literal[False], value: float
    ) -> Literal[1, -1, -2]:
        pass

    def SetIsosurfaceTransp(
        self, id: VolumeId, isosurf_id: IsosurfaceId, map: bool, value: str | float
    ) -> Literal[1, -1, -2]:
        """Set isosurface transparency

        :param id: volume id
        :param isosurf_id: isosurface id (0 - MAX_ISOSURFS)
        :param map: if true use map otherwise constant
        :param value: map name of value

        :return: 1 on success
        :return: -1 volume not found
        :return: -2 isosurface not found
        :return: -3 on failure
        """
        return self.SetIsosurfaceAttr(id, isosurf_id, ATT_TRANSP, map, value)

    @overload
    def SetIsosurfaceShine(
        self, id: VolumeId, isosurf_id: IsosurfaceId, map: Literal[True], value: str
    ) -> Literal[1, -1, -2]:
        pass

    @overload
    def SetIsosurfaceShine(
        self, id: VolumeId, isosurf_id: IsosurfaceId, map: Literal[False], value: float
    ) -> Literal[1, -1, -2]:
        pass

    def SetIsosurfaceShine(
        self, id: VolumeId, isosurf_id: IsosurfaceId, map: bool, value: str | float
    ) -> Literal[1, -1, -2]:
        """Set isosurface shininess

        :param id: volume id
        :param isosurf_id: isosurface id (0 - MAX_ISOSURFS)
        :param map: if true use map otherwise constant
        :param value: map name of value

        :return: 1 on success
        :return: -1 volume not found
        :return: -2 isosurface not found
        :return: -3 on failure
        """
        return self.SetIsosurfaceAttr(id, isosurf_id, ATT_SHINE, map, value)

    @overload
    def SetIsosurfaceEmit(
        self, id: VolumeId, isosurf_id: IsosurfaceId, map: Literal[True], value: str
    ) -> Literal[1, -1, -2]:
        pass

    @overload
    def SetIsosurfaceEmit(
        self, id: VolumeId, isosurf_id: IsosurfaceId, map: Literal[False], value: float
    ) -> Literal[1, -1, -2]:
        pass

    def SetIsosurfaceEmit(
        self, id: VolumeId, isosurf_id: IsosurfaceId, map: bool, value: str | float
    ) -> Literal[1, -1, -2]:
        """Set isosurface emission (currently unused)

        :param id: volume id
        :param isosurf_id: isosurface id (0 - MAX_ISOSURFS)
        :param map: if true use map otherwise constant
        :param value: map name of value

        :return: 1 on success
        :return: -1 volume not found
        :return: -2 isosurface not found
        :return: -3 on failure
        """
        return self.SetIsosurfaceAttr(id, isosurf_id, ATT_EMIT, map, value)

    @overload
    def SetIsosurfaceAttr(
        self,
        id: VolumeId,
        isosurf_id: IsosurfaceId,
        attr: int,
        map: Literal[True],
        value: str,
    ) -> Literal[1, -1, -2]:
        pass

    @overload
    def SetIsosurfaceAttr(
        self,
        id: VolumeId,
        isosurf_id: IsosurfaceId,
        attr: int,
        map: Literal[False],
        value: float,
    ) -> Literal[1, -1, -2]:
        pass

    @overload
    def SetIsosurfaceAttr(
        self,
        id: VolumeId,
        isosurf_id: IsosurfaceId,
        attr: Literal[2],
        map: Literal[False],
        value: str,
    ) -> Literal[1, -1, -2]:
        pass

    @overload
    def SetIsosurfaceAttr(
        self,
        id: VolumeId,
        isosurf_id: IsosurfaceId,
        attr: int,
        map: bool,
        value: str | float,
    ) -> Literal[1, -1, -2]:
        pass

    def SetIsosurfaceAttr(
        self,
        id: VolumeId,
        isosurf_id: IsosurfaceId,
        attr: int,
        map: bool,
        value: str | float,
    ) -> Literal[1, -1, -2]:
        """Set isosurface attribute

        :param id: volume id
        :param isosurf_id: isosurface id (0 - MAX_ISOSURFS)
        :param attr: attribute desc
        :param map: if true use map otherwise constant
        :param value: map name of value

        :return: 1 on success
        :return: -1 volume not found
        :return: -2 isosurface not found
        :return: -3 setting attributes failed
        """
        if not GVL_vol_exists(id):
            return -1

        if isosurf_id > GVL_isosurf_num_isosurfs(id) - 1:
            return -2

        if map:
            ret = GVL_isosurf_set_att_map(id, isosurf_id, attr, value)
        else:
            val: int | float = (
                Nviz_color_from_str(value) if attr == ATT_COLOR else float(value)
            )
            ret: int = GVL_isosurf_set_att_const(id, isosurf_id, attr, val)

        Debug.msg(
            3,
            "Nviz::SetIsosurfaceAttr(): id=%d, isosurf=%d, attr=%d, map=%s, value=%s",
            id,
            isosurf_id,
            attr,
            map,
            value,
        )

        if ret < 0:
            return -2

        return 1

    def UnsetIsosurfaceMask(
        self, id: VolumeId, isosurf_id: IsosurfaceId
    ) -> Literal[1, -1, -2]:
        """Unset isosurface mask

        :param id: volume id
        :param isosurf_id: isosurface id (0 - MAX_ISOSURFS)

        :return: 1 on success
        :return: -1 volume not found
        :return: -2 isosurface not found
        :return: -3 setting attributes failed
        """
        return self.UnsetIsosurfaceAttr(id, isosurf_id, ATT_MASK)

    def UnsetIsosurfaceTransp(
        self, id: VolumeId, isosurf_id: IsosurfaceId
    ) -> Literal[1, -1, -2]:
        """Unset isosurface transparency

        :param id: volume id
        :param isosurf_id: isosurface id (0 - MAX_ISOSURFS)

        :return: 1 on success
        :return: -1 volume not found
        :return: -2 isosurface not found
        :return: -3 setting attributes failed
        """
        return self.UnsetIsosurfaceAttr(id, isosurf_id, ATT_TRANSP)

    def UnsetIsosurfaceEmit(
        self, id: VolumeId, isosurf_id: IsosurfaceId
    ) -> Literal[1, -1, -2]:
        """Unset isosurface emission (currently unused)

        :param id: volume id
        :param isosurf_id: isosurface id (0 - MAX_ISOSURFS)

        :return: 1 on success
        :return: -1 volume not found
        :return: -2 isosurface not found
        :return: -3 setting attributes failed
        """
        return self.UnsetIsosurfaceAttr(id, isosurf_id, ATT_EMIT)

    def UnsetIsosurfaceAttr(
        self, id: VolumeId, isosurf_id: IsosurfaceId, attr: int
    ) -> Literal[1, -1, -2]:
        """Unset surface attribute

        :param id: volume id
        :param isosurf_id: isosurface id (0 - MAX_ISOSURFS)
        :param attr: attribute descriptor

        :return: 1 on success
        :return: -1 volume not found
        :return: -2 isosurface not found
        :return: -2 on failure
        """
        if not GVL_vol_exists(id):
            return -1

        if isosurf_id > GVL_isosurf_num_isosurfs(id) - 1:
            return -2

        Debug.msg(
            3,
            "Nviz::UnsetSurfaceAttr(): id=%d, isosurf_id=%d, attr=%d",
            id,
            isosurf_id,
            attr,
        )

        ret = GVL_isosurf_unset_att(id, isosurf_id, attr)

        if ret < 0:
            return -2

        return 1

    def SetIsosurfaceMode(self, id: VolumeId, mode: int) -> Literal[1, -1, -2]:
        """Set draw mode for isosurfaces

        :param id: volume set id
        :param mode: isosurface draw mode

        :return: 1 on success
        :return: -1 volume set not found
        :return: -2 on failure
        """
        if not GVL_vol_exists(id):
            return -1

        ret = GVL_isosurf_set_drawmode(id, mode)

        if ret < 0:
            return -2

        return 1

    def SetSliceMode(self, id: VolumeId, mode: int) -> Literal[1, -1, -2]:
        """Set draw mode for slices

        :param id: volume set id
        :param mode: slice draw mode

        :return: 1 on success
        :return: -1 volume set not found
        :return: -2 on failure
        """
        if not GVL_vol_exists(id):
            return -1

        ret = GVL_slice_set_drawmode(id, mode)

        if ret < 0:
            return -2

        return 1

    def SetIsosurfaceRes(self, id: VolumeId, res: int) -> Literal[1, -1, -2]:
        """Set draw resolution for isosurfaces

        :param id: volume set id
        :param res: resolution value

        :return: 1 on success
        :return: -1 volume set not found
        :return: -2 on failure
        """
        if not GVL_vol_exists(id):
            return -1

        ret = GVL_isosurf_set_drawres(id, res, res, res)

        if ret < 0:
            return -2

        return 1

    def SetSliceRes(self, id: VolumeId, res: int) -> Literal[1, -1, -2]:
        """Set draw resolution for slices

        :param id: volume set id
        :param res: resolution value

        :return: 1 on success
        :return: -1 volume set not found
        :return: -2 on failure
        """
        if not GVL_vol_exists(id):
            return -1

        ret = GVL_slice_set_drawres(id, res, res, res)

        if ret < 0:
            return -2

        return 1

    def SetSlicePosition(
        self,
        id: VolumeId,
        slice_id: SliceId,
        x1: float,
        x2: float,
        y1: float,
        y2: float,
        z1: float,
        z2: float,
        dir: int,
    ) -> Literal[1, -1, -2, -3]:
        """Set slice position

        :param id: volume id
        :param slice_id: slice id
        :param x1,x2,y1,y2,z1,z2: slice coordinates
        :param dir: axis

        :return: 1 on success
        :return: -1 volume not found
        :return: -2 slice not found
        :return: -3 on failure
        """
        if not GVL_vol_exists(id):
            return -1

        if slice_id > GVL_slice_num_slices(id):
            return -2

        ret = GVL_slice_set_pos(id, slice_id, x1, x2, y1, y2, z1, z2, dir)

        if ret < 0:
            return -3

        return 1

    def SetSliceTransp(
        self, id: VolumeId, slice_id: SliceId, value: int
    ) -> Literal[1, -1, -2, -3]:
        """Set slice transparency

        :param id: volume id
        :param slice_id: slice id
        :param value: transparency value (0 - 255)

        :return: 1 on success
        :return: -1 volume not found
        :return: -2 slice not found
        :return: -3 on failure
        """

        if not GVL_vol_exists(id):
            return -1

        if slice_id > GVL_slice_num_slices(id):
            return -2

        ret = GVL_slice_set_transp(id, slice_id, value)

        if ret < 0:
            return -3

        return 1

    def SetIsosurfaceInOut(
        self, id: VolumeId, isosurf_id: IsosurfaceId, inout: bool
    ) -> Literal[1, -1, -2, -3]:
        """Set inout mode

        :param id: volume id
        :param isosurf_id: isosurface id
        :param inout: mode true/false

        :return: 1 on success
        :return: -1 volume set not found
        :return: -2 isosurface not found
        :return: -3 on failure
        """
        if not GVL_vol_exists(id):
            return -1

        if isosurf_id > GVL_isosurf_num_isosurfs(id) - 1:
            return -2

        ret: int = GVL_isosurf_set_flags(id, isosurf_id, int(inout))

        if ret < 0:
            return -3

        return 1

    def GetVolumePosition(self, id: VolumeId) -> tuple[()] | tuple[float, float, float]:
        """Get volume position

        :param id: volume id

        :return: x,y,z
        :return: zero-length vector on error
        """
        if not GVL_vol_exists(id):
            return ()

        x, y, z = c_float(), c_float(), c_float()
        GVL_get_trans(id, byref(x), byref(y), byref(z))

        Debug.msg(
            3,
            "Nviz::GetVolumePosition(): id=%d, x=%f, y=%f, z=%f",
            id,
            x.value,
            y.value,
            z.value,
        )

        return (x.value, y.value, z.value)

    def SetVolumePosition(
        self, id: VolumeId, x: float, y: float, z: float
    ) -> Literal[1, -1]:
        """Set volume position

        :param id: volume id
        :param x,y,z: translation values

        :return: 1 on success
        :return: -1 volume not found
        """
        if not GVL_vol_exists(id):
            return -1

        Debug.msg(3, "Nviz::SetVolumePosition(): id=%d, x=%f, y=%f, z=%f", id, x, y, z)

        GVL_set_trans(id, x, y, z)

        return 1

    def SetVolumeDrawBox(self, id: VolumeId, ifBox: bool) -> Literal[1, -1]:
        """Display volume wire box

        :param id: volume id
        :param ifBox: True to draw wire box, False otherwise
        :return: 1 on success
        :return: -1 volume not found
        """
        if not GVL_vol_exists(id):
            return -1

        Debug.msg(3, "Nviz::SetVolumeDrawBox(): id=%d, ifBox=%d", id, int(ifBox))

        GVL_set_draw_wire(id, int(ifBox))

        return 1

    def GetCPlaneCurrent(self) -> ClipPlaneId:
        return Nviz_get_current_cplane(self.data)

    def GetCPlanesCount(self) -> int:
        """Returns number of cutting planes"""
        return Nviz_num_cplanes(self.data)

    def GetCPlaneRotation(self) -> tuple[float, float, float]:
        """Returns rotation parameters of current cutting plane"""
        x, y, z = c_float(), c_float(), c_float()

        current: ClipPlaneId = Nviz_get_current_cplane(self.data)
        Nviz_get_cplane_rotation(self.data, current, byref(x), byref(y), byref(z))

        return x.value, y.value, z.value

    def GetCPlaneTranslation(self) -> tuple[float, float, float]:
        """Returns translation parameters of current cutting plane"""
        x, y, z = c_float(), c_float(), c_float()

        current: ClipPlaneId = Nviz_get_current_cplane(self.data)
        Nviz_get_cplane_translation(self.data, current, byref(x), byref(y), byref(z))

        return x.value, y.value, z.value

    def SetCPlaneRotation(self, x: float, y: float, z: float) -> None:
        """Set current clip plane rotation

        :param x,y,z: rotation parameters
        """
        current: ClipPlaneId = Nviz_get_current_cplane(self.data)
        Nviz_set_cplane_rotation(self.data, current, x, y, z)
        Nviz_draw_cplane(self.data, -1, -1)

    def SetCPlaneTranslation(self, x: float, y: float, z: float) -> None:
        """Set current clip plane translation

        :param x,y,z: translation parameters
        """
        current: ClipPlaneId = Nviz_get_current_cplane(self.data)
        Nviz_set_cplane_translation(self.data, current, x, y, z)
        Nviz_draw_cplane(self.data, -1, -1)
        Debug.msg(
            3, "Nviz::SetCPlaneTranslation(): id=%d, x=%f, y=%f, z=%f", current, x, y, z
        )

    def SetCPlaneInteractively(
        self, x: float, y: float
    ) -> tuple[float, float, float] | tuple[None, None, None]:
        current: ClipPlaneId = Nviz_get_current_cplane(self.data)
        ret = Nviz_set_cplane_here(self.data, current, x, y)
        if ret:
            Nviz_draw_cplane(self.data, -1, -1)
            x, y, z = self.GetCPlaneTranslation()
            return x, y, z
        return None, None, None

    def SelectCPlane(self, index: ClipPlaneId) -> None:
        """Select cutting plane

        :param index: index of cutting plane
        """
        Nviz_on_cplane(self.data, index)

    def UnselectCPlane(self, index: ClipPlaneId) -> None:
        """Unselect cutting plane

        :param index: index of cutting plane
        """
        Nviz_off_cplane(self.data, index)

    def SetFenceColor(self, type: int) -> None:
        """Set appropriate fence color

        :param type: type of fence - from 0 (off) to 4
        """
        Nviz_set_fence_color(self.data, type)

    def GetXYRange(self) -> float:
        """Get xy range"""
        return Nviz_get_xyrange(self.data)

    def GetZRange(self) -> tuple[float, float]:
        """Get z range"""
        min, max = c_float(), c_float()
        Nviz_get_zrange(self.data, byref(min), byref(max))
        return min.value, max.value

    def SaveToFile(
        self,
        filename: str,
        width: int = 20,
        height: int = 20,
        itype: Literal["ppm", "tif"] = "ppm",
    ) -> None:
        """Save current GL screen to ppm/tif file

        :param filename: file name
        :param width: image width
        :param height: image height
        :param itype: image type ('ppm' or 'tif')
        """
        widthOrig = self.width
        heightOrig = self.height

        self.ResizeWindow(width, height)
        GS_clear(Nviz_get_bgcolor(self.data))
        self.Draw(False, -1)
        if itype == "ppm":
            GS_write_ppm(filename)
        else:
            GS_write_tif(filename)

        self.ResizeWindow(widthOrig, heightOrig)

    def DrawLightingModel(self) -> None:
        """Draw lighting model"""
        if self.showLight:
            Nviz_draw_model(self.data)

    def DrawFringe(self) -> None:
        """Draw fringe"""
        Nviz_draw_fringe(self.data)

    def SetFringe(
        self,
        sid: SurfaceId,
        color: tuple[int, int, int] | tuple[int, int, int, int],
        elev: float,
        nw: bool = False,
        ne: bool = False,
        sw: bool = False,
        se: bool = False,
    ) -> None:
        """Set fringe

        :param sid: surface id
        :param color: color
        :param elev: elevation (height)
        :param nw,ne,sw,se: fringe edges (turn on/off)
        """
        scolor = str(color[0]) + ":" + str(color[1]) + ":" + str(color[2])
        Nviz_set_fringe(
            self.data,
            sid,
            Nviz_color_from_str(scolor),
            elev,
            int(nw),
            int(ne),
            int(sw),
            int(se),
        )

    def DrawArrow(self) -> Literal[1]:
        """Draw north arrow"""
        return Nviz_draw_arrow(self.data)

    def SetArrow(self, sx: int, sy: int, size: float, color: str) -> Literal[1, 0]:
        """Set north arrow from canvas coordinates

        :param sx,sy: canvas coordinates
        :param size: arrow length
        :param color: arrow color
        :return: 1 on success
        :return: 0 on failure (no surfaces found)
        """
        return Nviz_set_arrow(self.data, sx, sy, size, Nviz_color_from_str(color))

    def DeleteArrow(self) -> None:
        """Delete north arrow"""
        Nviz_delete_arrow(self.data)

    def SetScalebar(
        self, id: int, sx: int, sy: int, size: float, color: str
    ):  # -> struct_scalebar_data | None:
        """Set scale bar from canvas coordinates

        :param sx,sy: canvas coordinates
        :param id: scale bar id
        :param size: scale bar length
        :param color: scale bar color
        """
        return Nviz_set_scalebar(
            self.data, id, sx, sy, size, Nviz_color_from_str(color)
        )

    def DrawScalebar(self) -> None:
        """Draw scale bar"""
        Nviz_draw_scalebar(self.data)

    def DeleteScalebar(self, id: int) -> None:
        """Delete scalebar"""
        Nviz_delete_scalebar(self.data, id)

    def GetPointOnSurface(
        self, sx, sy, scale: float = 1
    ) -> tuple[SurfaceId, float, float, float] | tuple[None, None, None, None]:
        """Get point on surface

        :param sx,sy: canvas coordinates (LL)
        """
        sid = c_int()
        x = c_float()
        y = c_float()
        z = c_float()
        Debug.msg(
            5, "Nviz::GetPointOnSurface(): sx=%d sy=%d" % (sx * scale, sy * scale)
        )
        num = GS_get_selected_point_on_surface(
            int(sx * scale), int(sy * scale), byref(sid), byref(x), byref(y), byref(z)
        )
        if num == 0:
            return (None, None, None, None)

        return (sid.value, x.value, y.value, z.value)

    def QueryMap(self, sx, sy, scale: float = 1) -> QueryMapResult | None:
        """Query surface map

        :param sx,sy: canvas coordinates (LL)
        """
        sid, x, y, z = self.GetPointOnSurface(sx, sy, scale)
        if not sid or (x is None or y is None or z is None):
            return None
        catstr = create_string_buffer(256)
        valstr = create_string_buffer(256)
        GS_get_cat_at_xy(sid, ATT_TOPO, catstr, x, y)
        GS_get_val_at_xy(sid, ATT_COLOR, valstr, x, y)

        return {
            "id": sid,
            "x": x,
            "y": y,
            "z": z,
            "elevation": DecodeString(catstr.value).replace("(", "").replace(")", ""),
            "color": DecodeString(valstr.value),
        }

    def GetDistanceAlongSurface(
        self,
        sid: SurfaceId,
        p1: tuple[float, float],
        p2: tuple[float, float],
        useExag: bool = True,
    ) -> float:
        """Get distance measured along surface"""
        d = c_float()

        GS_get_distance_alongsurf(
            sid, p1[0], p1[1], p2[0], p2[1], byref(d), int(useExag)
        )

        return d.value

    def GetRotationParameters(
        self, dx: float, dy: float
    ) -> tuple[float, float, float, float]:
        """Get rotation parameters (angle, x, y, z axes)

        :param dx,dy: difference from previous mouse drag event
        """
        modelview = (c_double * 16)()
        Nviz_get_modelview(byref(modelview))

        angle = sqrt(dx * dx + dy * dy) / float(self.width + 1) * 180.0
        m = []
        row = []
        for i, item in enumerate(modelview):
            row.append(item)
            if (i + 1) % 4 == 0:
                m.append(row)
                row = []
        inv = matrix(m).I
        ax, ay, az = dy, dx, 0.0
        x = inv[0, 0] * ax + inv[1, 0] * ay + inv[2, 0] * az
        y = inv[0, 1] * ax + inv[1, 1] * ay + inv[2, 1] * az
        z = inv[0, 2] * ax + inv[1, 2] * ay + inv[2, 2] * az

        return angle, x, y, z

    def Rotate(self, angle: float, x: float, y: float, z: float) -> None:
        """Set rotation parameters
        Rotate scene (difference from current state).

        :param angle: angle
        :param x,y,z: axis coordinate
        """
        Nviz_set_rotation(angle, x, y, z)

    def UnsetRotation(self) -> None:
        """Stop rotating the scene"""
        Nviz_unset_rotation()

    def ResetRotation(self) -> None:
        """Reset scene rotation"""
        Nviz_init_rotation()

    def GetRotationMatrix(self):
        """Get rotation matrix"""
        matrix = (c_double * 16)()
        GS_get_rotation_matrix(byref(matrix))
        returnMatrix = []
        for item in matrix:
            returnMatrix.append(item)
        return returnMatrix

    def SetRotationMatrix(self, matrix):
        """Set rotation matrix"""
        mtrx = (c_double * 16)()
        for i in range(len(matrix)):
            mtrx[i] = matrix[i]
        GS_set_rotation_matrix(byref(mtrx))

    def Start2D(self):
        Nviz_set_2D(self.width, self.height)

    def FlyThrough(
        self, flyInfo: Iterable[float], mode: int, exagInfo: Mapping[str, float | int]
    ):
        """Fly through the scene

        :param flyInfo: fly parameters
        :param mode: 0 or 1 for different fly behavior
        :param exagInfo: parameters changing fly speed
        """
        fly = (c_float * 3)()
        for i, item in enumerate(flyInfo):
            fly[i] = item
        exag = (c_int * 2)()
        exag[0] = int(exagInfo["move"])
        exag[1] = int(exagInfo["turn"])
        Nviz_flythrough(self.data, fly, exag, mode)


class Texture:
    """Class representing OpenGL texture"""

    def __init__(
        self, filepath: StrPath, overlayId: int, coords: tuple[int, int]
    ) -> None:
        """Load image to texture

        :param filepath: path to image file
        :param overlayId: id of overlay (1 for legend, 101 and more for text)
        :param coords: image coordinates
        """
        self.path = filepath
        self.image: wx.Image = autoCropImageFromFile(filepath)
        self.width: int
        self.orig_width: int
        self.height: int
        self.orig_height: int
        self.width = self.orig_width = self.image.GetWidth()
        self.height = self.orig_height = self.image.GetHeight()
        self.id: int = overlayId
        self.coords: tuple[int, int] = coords
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

    def __del__(self) -> None:
        """Delete texture"""
        if self.textureId:
            Nviz_del_texture(self.textureId)
        gs.try_remove(self.path)

    def Resize(self) -> None:
        """Resize image to match 2^n"""
        n = m = 1
        while self.width > pow(2, n):
            n += 1
        while self.height > pow(2, m):
            m += 1
        self.image.Resize(size=(pow(2, n), pow(2, m)), pos=(0, 0))
        self.width: int = self.image.GetWidth()
        self.height: int = self.image.GetHeight()

    def Load(self) -> int:
        """Load image to texture

        :return: The texture id
        """
        bytesPerPixel = 4 if self.image.HasAlpha() else 3
        bytes = bytesPerPixel * self.width * self.height
        rev_val = self.height - 1
        im = (c_ubyte * bytes)()
        bytes3 = 3 * self.width * self.height
        bytes1 = self.width * self.height
        imageData = struct.unpack(str(bytes3) + "B", self.image.GetData())
        if self.image.HasAlpha():
            if wxPythonPhoenix:
                alphaData = struct.unpack(str(bytes1) + "B", self.image.GetAlpha())
            else:
                alphaData = struct.unpack(str(bytes1) + "B", self.image.GetAlphaData())

        # this takes too much time
        wx.BeginBusyCursor()
        for i in range(self.height):
            for j in range(self.width):
                im[(j + i * self.width) * bytesPerPixel + 0] = imageData[
                    (j + (rev_val - i) * self.width) * 3 + 0
                ]
                im[(j + i * self.width) * bytesPerPixel + 1] = imageData[
                    (j + (rev_val - i) * self.width) * 3 + 1
                ]
                im[(j + i * self.width) * bytesPerPixel + 2] = imageData[
                    (j + (rev_val - i) * self.width) * 3 + 2
                ]
                if self.image.HasAlpha():
                    im[(j + i * self.width) * bytesPerPixel + 3] = alphaData[
                        (j + (rev_val - i) * self.width)
                    ]
        wx.EndBusyCursor()

        return Nviz_load_image(im, self.width, self.height, self.image.HasAlpha())

    def Draw(self) -> None:
        """Draw texture as an image"""
        Nviz_draw_image(
            self.coords[0], self.coords[1], self.width, self.height, self.textureId
        )

    def HitTest(self, x: int, y: int, radius: int) -> bool:
        copy = Rect(self.coords[0], self.coords[1], self.orig_width, self.orig_height)
        copy.Inflate(radius, radius)
        return copy.ContainsXY(x, y)

    def MoveTexture(self, dx: int, dy: int) -> None:
        """Move texture on the screen"""
        self.coords[0] += dx
        self.coords[1] += dy

    def SetCoords(self, coords: tuple[int, int]) -> None:
        """Set coordinates"""
        dx = coords[0] - self.coords[0]
        dy = coords[1] - self.coords[1]
        self.MoveTexture(dx, dy)

    def GetId(self) -> int:
        """Returns image id."""
        return self.id

    def SetActive(self, active: bool = True) -> None:
        self.active: bool = active

    def IsActive(self) -> bool:
        return self.active


class ImageTexture(Texture):
    """Class representing OpenGL texture as an overlay image"""

    def __init__(
        self, filepath: StrPath, overlayId, coords: tuple[int, int], cmd
    ) -> None:
        """Load image to texture

        :param filepath: path to image file
        :param overlayId: id of overlay (1 for legend)
        :param coords: image coordinates
        :param cmd: d.legend command
        """
        Texture.__init__(self, filepath=filepath, overlayId=overlayId, coords=coords)

        self.cmd = cmd

    def GetCmd(self):
        """Returns overlay command."""
        return self.cmd

    def Corresponds(self, item) -> bool:
        return sorted(self.GetCmd()) == sorted(item.GetCmd())


__all__: list[str] = [
    "DM_FLAT",
    "DM_GOURAUD",
    "DM_GRID_SURF",
    "DM_GRID_WIRE",
    "DM_POLY",
    "DM_WIRE",
    "DM_WIRE_POLY",
    "DRAW_QUICK_SURFACE",
    "DRAW_QUICK_VLINES",
    "DRAW_QUICK_VOLUME",
    "DRAW_QUICK_VPOINTS",
    "MAX_ISOSURFS",
    "ImageTexture",
    "Nviz",
]

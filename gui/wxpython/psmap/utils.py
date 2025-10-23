"""
@package psmap.utils

@brief utilities for wxpsmap (classes, functions)

Classes:
 - utils::Rect2D
 - utils::Rect2DPP
 - utils::Rect2DPS
 - utils::UnitConversion

(C) 2012 by Anna Kratochvilova, and the GRASS Development Team
This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Anna Kratochvilova <kratochanna gmail.com>
"""

from __future__ import annotations

from math import ceil, cos, floor, fmod, radians, sin
from typing import overload

import wx
from core.gcmd import GError, RunCommand

import grass.script as gs
from grass.exceptions import ScriptError

try:
    from PIL import Image as PILImage  # noqa: F401

    havePILImage = True
except ImportError:
    havePILImage = False


class Rect2D(wx.Rect2D):
    """Class representing rectangle with floating point values.

    Overrides wx.Rect2D to unify Rect access methods, which are
    different (e.g. wx.Rect.GetTopLeft() x wx.Rect2D.GetLeftTop()).
    More methods can be added depending on needs.
    """

    def __init__(self, x=0, y=0, width=0, height=0):
        wx.Rect2D.__init__(self, x=x, y=y, w=width, h=height)

    def GetX(self):
        return self.x

    def GetY(self):
        return self.y

    def GetWidth(self):
        return self.width

    def SetWidth(self, width):
        self.width = width

    def GetHeight(self):
        return self.height

    def SetHeight(self, height):
        self.height = height


class Rect2DPP(Rect2D):
    """Rectangle specified by 2 points (with floating point values).

    :class:`Rect2D`, :class:`Rect2DPS`
    """

    def __init__(self, topLeft=wx.Point2D(), bottomRight=wx.Point2D()):
        Rect2D.__init__(self, x=0, y=0, width=0, height=0)

        x1, y1 = topLeft[0], topLeft[1]
        x2, y2 = bottomRight[0], bottomRight[1]

        self.SetLeft(min(x1, x2))
        self.SetTop(min(y1, y2))
        self.SetRight(max(x1, x2))
        self.SetBottom(max(y1, y2))


class Rect2DPS(Rect2D):
    """Rectangle specified by point and size (with floating point values).

    :class:`Rect2D`, :class:`Rect2DPP`
    """

    def __init__(self, pos=wx.Point2D(), size=(0, 0)):
        Rect2D.__init__(self, x=pos[0], y=pos[1], width=size[0], height=size[1])


class UnitConversion:
    """Class for converting units"""

    def __init__(self, parent=None):
        self.parent = parent
        ppi = wx.ClientDC(self.parent).GetPPI() if self.parent else (72, 72)
        self._unitsPage = {
            "inch": {"val": 1.0, "tr": _("inch")},
            "point": {"val": 72.0, "tr": _("point")},
            "centimeter": {"val": 2.54, "tr": _("centimeter")},
            "millimeter": {"val": 25.4, "tr": _("millimeter")},
        }
        self._unitsMap = {
            "meters": {"val": 0.0254, "tr": _("meters")},
            "kilometers": {"val": 2.54e-5, "tr": _("kilometers")},
            "feet": {"val": 1.0 / 12, "tr": _("feet")},
            "miles": {"val": 1.0 / 63360, "tr": _("miles")},
            "nautical miles": {"val": 1 / 72913.386, "tr": _("nautical miles")},
        }

        self._units = {
            "pixel": {"val": ppi[0], "tr": _("pixel")},
            "meter": {"val": 0.0254, "tr": _("meter")},
            "nautmiles": {"val": 1 / 72913.386, "tr": _("nautical miles")},
            # like 1 meter, incorrect
            "degrees": {"val": 0.0254, "tr": _("degree")},
        }
        self._units.update(self._unitsPage)
        self._units.update(self._unitsMap)

    def getPageUnitsNames(self):
        return sorted(self._unitsPage[unit]["tr"] for unit in self._unitsPage.keys())

    def getMapUnitsNames(self):
        return sorted(self._unitsMap[unit]["tr"] for unit in self._unitsMap.keys())

    def getAllUnits(self):
        return sorted(self._units.keys())

    def findUnit(self, name):
        """Returns unit by its tr. string"""
        for unit in self._units.keys():
            if self._units[unit]["tr"] == name:
                return unit
        return None

    def findName(self, unit):
        """Returns tr. string of a unit"""
        try:
            return self._units[unit]["tr"]
        except KeyError:
            return None

    def convert(self, value, fromUnit=None, toUnit=None):
        return float(value) / self._units[fromUnit]["val"] * self._units[toUnit]["val"]


@overload
def convertRGB(rgb: wx.Colour) -> str:
    pass


@overload
def convertRGB(rgb: str) -> wx.Colour | None:
    pass


def convertRGB(rgb: wx.Colour | str) -> str | wx.Colour | None:
    """Converts wx.Colour(r,g,b,a) to string 'r:g:b' or named color,
    or named color/r:g:b string to wx.Colour, depending on input"""
    # transform a wx.Colour tuple into an r:g:b string
    if isinstance(rgb, wx.Colour):
        for name, color in gs.named_colors.items():
            if (
                rgb.Red() == int(color[0] * 255)
                and rgb.Green() == int(color[1] * 255)
                and rgb.Blue() == int(color[2] * 255)
            ):
                return name
        return str(rgb.Red()) + ":" + str(rgb.Green()) + ":" + str(rgb.Blue())
    # transform a GRASS named color or an r:g:b string into a wx.Colour tuple
    parsed_color = gs.parse_color(rgb)
    if parsed_color is None:
        return None
    color = wx.Colour(*tuple(int(x * 255) for x in parsed_color))
    if color.IsOk():
        return color
    return None


def PaperMapCoordinates(mapInstr, x, y, paperToMap=True, env=None):
    """Converts paper (inch) coordinates <-> map coordinates.

    :param mapInstr: map frame instruction
    :param x,y: paper coords in inches or mapcoords in map units
    :param paperToMap: specify conversion direction
    """
    region = gs.region(env=env)
    mapWidthPaper = mapInstr["rect"].GetWidth()
    mapHeightPaper = mapInstr["rect"].GetHeight()
    mapWidthEN = region["e"] - region["w"]
    mapHeightEN = region["n"] - region["s"]

    if not paperToMap:
        diffEW = x - region["w"]
        diffNS = region["n"] - y
        diffX = mapWidthPaper * diffEW / mapWidthEN
        diffY = mapHeightPaper * diffNS / mapHeightEN
        xPaper = mapInstr["rect"].GetX() + diffX
        yPaper = mapInstr["rect"].GetY() + diffY
        return (xPaper, yPaper)

    diffX = x - mapInstr["rect"].GetX()
    diffY = y - mapInstr["rect"].GetY()
    diffEW = diffX * mapWidthEN / mapWidthPaper
    diffNS = diffY * mapHeightEN / mapHeightPaper
    e = region["w"] + diffEW
    n = region["n"] - diffNS
    if projInfo()["proj"] == "ll":
        return (e, n)
    return (int(e), int(n))


def AutoAdjust(self, scaleType, rect, env, map=None, mapType=None, region=None):
    """Computes map scale, center and map frame rectangle to fit region
    (scale is not fixed)
    """
    currRegionDict = {}
    try:
        if scaleType == 0 and map:  # automatic, region from raster or vector
            res = ""
            if mapType == "raster":
                try:
                    res = gs.read_command("g.region", flags="gu", raster=map, env=env)
                except ScriptError:
                    pass
            elif mapType == "vector":
                res = gs.read_command("g.region", flags="gu", vector=map, env=env)
            currRegionDict = gs.parse_key_val(res, val_type=float)
        elif scaleType == 1 and region:  # saved region
            res = gs.read_command("g.region", flags="gu", region=region, env=env)
            currRegionDict = gs.parse_key_val(res, val_type=float)
        elif scaleType == 2:  # current region
            currRegionDict = gs.region(env=None)

        else:
            return None, None, None
    # fails after switching location
    except (ScriptError, gs.CalledModuleError):
        pass

    if not currRegionDict:
        return None, None, None
    rX = rect.x
    rY = rect.y
    rW = rect.width
    rH = rect.height
    if not hasattr(self, "unitConv"):
        self.unitConv = UnitConversion(self)
    toM = 1
    if projInfo()["proj"] != "xy":
        toM = float(projInfo()["meters"])

    mW = self.unitConv.convert(
        value=(currRegionDict["e"] - currRegionDict["w"]) * toM,
        fromUnit="meter",
        toUnit="inch",
    )
    mH = self.unitConv.convert(
        value=(currRegionDict["n"] - currRegionDict["s"]) * toM,
        fromUnit="meter",
        toUnit="inch",
    )
    scale = min(rW / mW, rH / mH)

    if rW / rH > mW / mH:
        x = rX - (rH * (mW / mH) - rW) / 2
        y = rY
        rWNew = rH * (mW / mH)
        rHNew = rH
    else:
        x = rX
        y = rY - (rW * (mH / mW) - rH) / 2
        rHNew = rW * (mH / mW)
        rWNew = rW

    # center
    cE = (currRegionDict["w"] + currRegionDict["e"]) / 2
    cN = (currRegionDict["n"] + currRegionDict["s"]) / 2
    return scale, (cE, cN), Rect2D(x, y, rWNew, rHNew)  # inch


def SetResolution(dpi, width, height, env):
    """If resolution is too high, lower it

    :param dpi: max DPI
    :param width: map frame width
    :param height: map frame height
    """
    region = gs.region(env=env)
    if region["cols"] > width * dpi or region["rows"] > height * dpi:
        rows = height * dpi
        cols = width * dpi
        env["GRASS_REGION"] = gs.region_env(rows=rows, cols=cols, env=env)


def ComputeSetRegion(self, mapDict, env):
    """Computes and sets region from current scale, map center
    coordinates and map rectangle
    """

    if mapDict["scaleType"] == 3:  # fixed scale
        scale = mapDict["scale"]

        if not hasattr(self, "unitConv"):
            self.unitConv = UnitConversion(self)

        fromM = 1
        if projInfo()["proj"] != "xy":
            fromM = float(projInfo()["meters"])
        rectHalfInch = (mapDict["rect"].width / 2, mapDict["rect"].height / 2)
        rectHalfMeter = (
            self.unitConv.convert(
                value=rectHalfInch[0], fromUnit="inch", toUnit="meter"
            )
            / fromM
            / scale,
            self.unitConv.convert(
                value=rectHalfInch[1], fromUnit="inch", toUnit="meter"
            )
            / fromM
            / scale,
        )

        centerE = mapDict["center"][0]
        centerN = mapDict["center"][1]

        raster = self.instruction.FindInstructionByType("raster")
        rasterId = raster.id if raster else None

        if rasterId:
            env["GRASS_REGION"] = gs.region_env(
                n=ceil(centerN + rectHalfMeter[1]),
                s=floor(centerN - rectHalfMeter[1]),
                e=ceil(centerE + rectHalfMeter[0]),
                w=floor(centerE - rectHalfMeter[0]),
                rast=self.instruction[rasterId]["raster"],
                env=env,
            )
        else:
            env["GRASS_REGION"] = gs.region_env(
                n=ceil(centerN + rectHalfMeter[1]),
                s=floor(centerN - rectHalfMeter[1]),
                e=ceil(centerE + rectHalfMeter[0]),
                w=floor(centerE - rectHalfMeter[0]),
                env=env,
            )


def projInfo():
    """Return region projection and map units information,
    taken from render.py
    """
    proj_info = RunCommand(
        "g.proj",
        flags="g",
        read=True,
        parse=gs.parse_key_val,
    )

    return (
        proj_info
        if proj_info.get("name") != "xy_location_unprojected"
        else {"proj": "xy", "units": ""}
    )


def GetMapBounds(filename, env, portrait=True):
    """Run ps.map -b to get information about map bounding box

    :param filename: psmap input file
    :param env: environment with GRASS_REGION defined
    :param portrait: page orientation"""
    orient = ""
    if not portrait:
        orient = "r"
    try:
        bb = list(
            map(
                float,
                gs.read_command(
                    "ps.map", flags="b" + orient, quiet=True, input=filename, env=env
                )
                .strip()
                .split("=")[1]
                .split(","),
            )
        )
    except (ScriptError, IndexError):
        GError(message=_("Unable to run `ps.map -b`"))
        return None
    return Rect2D(bb[0], bb[3], bb[2] - bb[0], bb[1] - bb[3])


def getRasterType(map):
    """Returns type of raster map (CELL, FCELL, DCELL)"""
    if map is None:
        map = ""
    file = gs.find_file(name=map, element="cell")
    if file.get("file"):
        return gs.raster_info(map)["datatype"]
    return None


def BBoxAfterRotation(w: float, h: float, angle: float) -> tuple[int, int]:
    """Compute the bounding box of a rotated rectangle

    :param w: rectangle width
    :param h: rectangle height
    :param angle: angle (0, 360) in degrees
    """

    angle = fmod(angle, 360)
    angleRad: float = radians(angle)
    ct: float = cos(angleRad)
    st: float = sin(angleRad)

    hct: float = h * ct
    wct: float = w * ct
    hst: float = h * st
    wst: float = w * st
    y = x = 0

    if angle == 0:
        return (ceil(w), ceil(h))
    if 0 < angle <= 90:
        y_min = y
        y_max = y + hct + wst
        x_min = x - hst
        x_max = x + wct
    elif 90 < angle <= 180:
        y_min = y + hct
        y_max = y + wst
        x_min = x - hst + wct
        x_max = x
    elif 180 < angle <= 270:
        y_min = y + wst + hct
        y_max = y
        x_min = x + wct
        x_max = x - hst
    elif 270 < angle <= 360:
        y_min = y + wst
        y_max = y + hct
        x_min = x
        x_max = x + wct - hst
    else:
        msg = "The angle argument should be between 0 and 360 degrees"
        raise ValueError(msg)

    width: int = ceil(abs(x_max) + abs(x_min))
    height: int = ceil(abs(y_max) + abs(y_min))
    return (width, height)

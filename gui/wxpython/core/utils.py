"""
@package core.utils

@brief Misc utilities for wxGUI

(C) 2007-2024 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
@author Jachym Cepicky
"""

from __future__ import annotations

import os
import sys
import platform
import glob
import shlex
import re
import inspect
import operator
from string import digits
from typing import TYPE_CHECKING


from grass.script import core as grass
from grass.script import task as gtask
from grass.app.runtime import get_grass_config_dir

from core.gcmd import RunCommand
from core.debug import Debug
from core.globalvar import wxPythonPhoenix


if TYPE_CHECKING:
    import wx
    import PIL.Image


def cmp(a, b):
    """cmp function"""
    return (a > b) - (a < b)


def normalize_whitespace(text):
    """Remove redundant whitespace from a string"""
    return (" ").join(text.split())


def split(s):
    """Platform specific shlex.split"""
    try:
        if sys.platform == "win32":
            return shlex.split(s.replace("\\", r"\\"))
        return shlex.split(s)
    except ValueError as e:
        sys.stderr.write(_("Syntax error: %s") % e)

    return []


def GetTempfile(pref=None):
    """Creates GRASS temporary file using defined prefix.

    .. todo::
        Fix path on MS Windows/MSYS

    :param pref: prefer the given path

    :return: Path to file name (string) or None
    """
    ret = RunCommand("g.tempfile", read=True, pid=os.getpid())

    tempfile = ret.splitlines()[0].strip()

    # FIXME
    # ugly hack for MSYS (MS Windows)
    if platform.system() == "Windows":
        tempfile = tempfile.replace("/", "\\")
    try:
        path, file = os.path.split(tempfile)
        if pref:
            return os.path.join(pref, file)
        return tempfile
    except Exception:
        return None


def GetLayerNameFromCmd(dcmd, fullyQualified=False, param=None, layerType=None):
    """Get map name from GRASS command

    Parameter dcmd can be modified when first parameter is not
    defined.

    :param dcmd: GRASS command (given as list)
    :param fullyQualified: change map name to be fully qualified
    :param param: params directory
    :param str layerType: check also layer type ('raster', 'vector',
                          'raster_3d', ...)

    :return: tuple (name, found)
    """
    mapname = ""
    found = True

    if len(dcmd) < 1:
        return mapname, False

    if dcmd[0] == "d.grid":
        mapname = "grid"
    elif "d.geodesic" in dcmd[0]:
        mapname = "geodesic"
    elif "d.rhumbline" in dcmd[0]:
        mapname = "rhumb"
    elif "d.graph" in dcmd[0]:
        mapname = "graph"
    else:
        params = []
        for idx in range(len(dcmd)):
            try:
                p, v = dcmd[idx].split("=", 1)
            except ValueError:
                continue

            if p == param:
                params = [(idx, p, v)]
                break

            # this does not use types, just some (incomplete subset of?) names
            if p in {
                "map",
                "input",
                "layer",
                "red",
                "blue",
                "green",
                "hue",
                "saturation",
                "intensity",
                "shade",
                "labels",
            }:
                params.append((idx, p, v))

        if len(params) < 1:
            if len(dcmd) <= 1:
                return (mapname, False)

            i = 1
            while i < len(dcmd):
                if "=" not in dcmd[i] and (not dcmd[i].startswith("-")):
                    task = gtask.parse_interface(dcmd[0])
                    # this expects the first parameter to be the right one
                    p = task.get_options()["params"][0].get("name", "")
                    params.append((i, p, dcmd[i]))
                    break
                i += 1

        if len(params) < 1:
            return mapname, False

        # need to add mapset for all maps
        mapsets = {}
        for i, p, v in params:
            if p == "layer":
                continue
            mapname = v
            mapset = ""
            if fullyQualified and "@" not in mapname:
                if layerType in {"raster", "vector", "raster_3d", "rgb", "his"}:
                    try:
                        if layerType in {"raster", "rgb", "his"}:
                            findType = "cell"
                        elif layerType == "raster_3d":
                            findType = "grid3"
                        else:
                            findType = layerType
                        mapset = grass.find_file(mapname, element=findType)["mapset"]
                    except AttributeError:  # not found
                        return "", False
                    if not mapset:
                        found = False
                else:
                    mapset = ""  # grass.gisenv()['MAPSET']
            mapsets[i] = mapset

        # update dcmd
        for i, p, v in params:
            if p == "layer":
                continue
            dcmd[i] = p + "=" + v
            if i in mapsets and mapsets[i]:
                dcmd[i] += "@" + mapsets[i]

        maps = []
        ogr = False
        for i, p, v in params:
            if v.lower().rfind("@ogr") > -1:
                ogr = True
            if p == "layer" and not ogr:
                continue
            maps.append(dcmd[i].split("=", 1)[1])

        mapname = "\n".join(maps)

    return mapname, found


def GetValidLayerName(name):
    """Make layer name SQL compliant, based on G_str_to_sql()

    .. todo::
        Better use directly Ctypes to reuse venerable libgis C fns...
    """
    retName = name.strip()

    # check if name is fully qualified
    if "@" in retName:
        retName, mapset = retName.split("@")
    else:
        mapset = None

    cIdx = 0
    retNameList = list(retName)
    for c in retNameList:
        if not ("A" <= c <= "Z") and not ("a" <= c <= "z") and not ("0" <= c <= "9"):
            retNameList[cIdx] = "_"
        cIdx += 1
    retName = "".join(retNameList)

    if (
        retName
        and not (retName[0] >= "A" and retName[0] <= "Z")
        and not (retName[0] >= "a" and retName[0] <= "z")
    ):
        retName = "x" + retName[1:]

    if mapset:
        retName = retName + "@" + mapset

    return retName


def ListOfCatsToRange(cats):
    """Convert list of category number to range(s)

    Used for example for d.vect cats=[range]

    :param cats: category list

    :return: category range string
    :return: '' on error
    """

    catstr = ""

    try:
        cats = list(map(int, cats))
    except ValueError:
        return catstr

    i = 0
    while i < len(cats):
        next = 0
        j = i + 1
        while j < len(cats):
            if cats[i + next] != cats[j] - 1:
                break
            next += 1
            j += 1

        if next > 1:
            catstr += "%d-%d," % (cats[i], cats[i + next])
            i += next + 1
        else:
            catstr += "%d," % (cats[i])
            i += 1

    return catstr.strip(",")


def ListOfMapsets(get="ordered"):
    """Get list of available/accessible mapsets.
    Option 'ordered' returns list of all mapsets, first accessible
    then not accessible. Raises ValueError for wrong parameter values.

    :param str get: method ('all', 'accessible', 'ordered')

    :return: list of mapsets
    :return: [] on error
    """
    if get in {"all", "ordered"}:
        ret = RunCommand("g.mapsets", read=True, quiet=True, flags="l", sep="newline")
        if not ret:
            return []
        mapsets_all = ret.splitlines()
        ListSortLower(mapsets_all)
        if get == "all":
            return mapsets_all

    if get not in {"accessible", "ordered"}:
        msg = "Invalid value for 'get' parameter of ListOfMapsets()"
        raise ValueError(msg)
    ret = RunCommand("g.mapsets", read=True, quiet=True, flags="p", sep="newline")
    if not ret:
        return []
    mapsets_accessible = ret.splitlines()
    if get == "accessible":
        return mapsets_accessible

    mapsets_ordered = mapsets_accessible.copy()
    for mapset in mapsets_all:
        if mapset not in mapsets_accessible:
            mapsets_ordered.append(mapset)
    return mapsets_ordered


def ListSortLower(list):
    """Sort list items (not case-sensitive)"""
    list.sort(key=lambda x: x.lower())


def GetVectorNumberOfLayers(vector):
    """Get list of all vector layers"""
    layers = []
    if not vector:
        return layers

    fullname = grass.find_file(name=vector, element="vector")["fullname"]
    if not fullname:
        Debug.msg(
            5, "utils.GetVectorNumberOfLayers(): vector map '%s' not found" % vector
        )
        return layers

    ret, out, msg = RunCommand(
        "v.category", getErrorMsg=True, read=True, input=fullname, option="layers"
    )
    if ret != 0:
        sys.stderr.write(
            _("Vector map <%(map)s>: %(msg)s\n") % {"map": fullname, "msg": msg}
        )
        return layers
    Debug.msg(1, "GetVectorNumberOfLayers(): ret %s" % ret)

    for layer in out.splitlines():
        layers.append(layer)

    Debug.msg(
        3,
        "utils.GetVectorNumberOfLayers(): vector=%s -> %s"
        % (fullname, ",".join(layers)),
    )

    return layers


def Deg2DMS(lon, lat, string=True, hemisphere=True, precision=3):
    """Convert deg value to dms string

    :param lon: longitude (x)
    :param lat: latitude (y)
    :param string: True to return string otherwise tuple
    :param hemisphere: print hemisphere
    :param precision: seconds precision

    :return: DMS string or tuple of values
    :return: empty string on error
    """
    try:
        flat = float(lat)
        flon = float(lon)
    except ValueError:
        if string:
            return ""
        return None

    # fix longitude
    while flon > 180.0:
        flon -= 360.0
    while flon < -180.0:
        flon += 360.0

    # hemisphere
    if hemisphere:
        if flat < 0.0:
            flat = abs(flat)
            hlat = "S"
        else:
            hlat = "N"

        if flon < 0.0:
            hlon = "W"
            flon = abs(flon)
        else:
            hlon = "E"
    else:
        flat = abs(flat)
        flon = abs(flon)
        hlon = ""
        hlat = ""

    slat = __ll_parts(flat, precision=precision)
    slon = __ll_parts(flon, precision=precision)

    if string:
        return slon + hlon + "; " + slat + hlat

    return (slon + hlon, slat + hlat)


def DMS2Deg(lon, lat):
    """Convert dms value to deg

    :param lon: longitude (x)
    :param lat: latitude (y)

    :return: tuple of converted values
    :return: ValueError on error
    """
    x = __ll_parts(lon, reverse=True)
    y = __ll_parts(lat, reverse=True)

    return (x, y)


def __ll_parts(value, reverse=False, precision=3):
    """Converts deg to d:m:s string

    :param value: value to be converted
    :param reverse: True to convert from d:m:s to deg
    :param precision: seconds precision (ignored if reverse is True)

    :return: converted value (string/float)
    :return: ValueError on error (reverse == True)
    """
    if not reverse:
        if value == 0.0:
            return "%s%.*f" % ("00:00:0", precision, 0.0)

        d = int(value)
        m = int((value - d) * 60)
        s = ((value - d) * 60 - m) * 60
        if m < 0:
            m = "00"
        elif m < 10:
            m = "0" + str(m)
        else:
            m = str(m)
        if s < 0:
            s = "00.0000"
        elif s < 10.0:
            s = "0%.*f" % (precision, s)
        else:
            s = "%.*f" % (precision, s)

        return str(d) + ":" + m + ":" + s
    # -> reverse
    try:
        d, m, s = value.split(":")
        hs = s[-1]
        s = s[:-1]
    except ValueError:
        try:
            d, m = value.split(":")
            hs = m[-1]
            m = m[:-1]
            s = "0.0"
        except ValueError:
            try:
                d = value
                hs = d[-1]
                d = d[:-1]
                m = "0"
                s = "0.0"
            except ValueError:
                raise ValueError

    if hs not in {"N", "S", "E", "W"}:
        raise ValueError

    coef = 1.0
    if hs in {"S", "W"}:
        coef = -1.0

    fm = int(m) / 60.0
    fs = float(s) / (60 * 60)

    return coef * (float(d) + fm + fs)


def GetCmdString(cmd):
    """Get GRASS command as string.

    :param cmd: GRASS command given as tuple

    :return: command string
    """
    return " ".join(gtask.cmdtuple_to_list(cmd))


def PathJoin(*args):
    """Check path created by os.path.join"""
    path = os.path.join(*args)
    if platform.system() == "Windows" and "/" in path:
        return path[1].upper() + ":\\" + path[3:].replace("/", "\\")

    return path


def ReadEpsgCodes():
    """Read EPSG codes with g.proj

    :return: dictionary of EPSG code
    """
    epsgCodeDict = {}

    ret = RunCommand("g.proj", read=True, list_codes="EPSG")

    for line in ret.splitlines():
        code, descr, params = line.split("|")
        epsgCodeDict[int(code)] = (descr, params)

    return epsgCodeDict


def ReprojectCoordinates(coord, projOut, projIn=None, flags=""):
    """Reproject coordinates

    :param coord: coordinates given as tuple
    :param projOut: output projection
    :param projIn: input projection (use location projection settings)

    :return: reprojected coordinates (returned as tuple)
    """
    coors = RunCommand(
        "m.proj",
        flags=flags,
        input="-",
        proj_in=projIn,
        proj_out=projOut,
        sep=";",
        stdin="%f;%f" % (coord[0], coord[1]),
        read=True,
    )
    if coors:
        coors = coors.split(";")
        e = coors[0]
        n = coors[1]
        try:
            proj = projOut.split(" ")[0].split("=")[1]
        except IndexError:
            proj = ""
        if proj in {"ll", "latlong", "longlat"} and "d" not in flags:
            return (proj, (e, n))
        try:
            return (proj, (float(e), float(n)))
        except ValueError:
            return (None, None)

    return (None, None)


def GetListOfLocations(dbase):
    """Get list of GRASS locations in given dbase

    :param dbase: GRASS database path

    :return: list of locations (sorted)
    """
    listOfLocations = []

    for location in glob.glob(os.path.join(dbase, "*")):
        try:
            if os.path.join(location, "PERMANENT") in glob.glob(
                os.path.join(location, "*")
            ):
                listOfLocations.append(os.path.basename(location))
        except OSError:
            pass

    ListSortLower(listOfLocations)

    return listOfLocations


def GetListOfMapsets(dbase, location, selectable=False):
    """Get list of mapsets in given GRASS location

    :param dbase: GRASS database path
    :param location: GRASS location
    :param selectable: True to get list of selectable mapsets, otherwise all

    :return: list of mapsets - sorted (PERMANENT first)
    """
    listOfMapsets = []

    if selectable:
        ret = RunCommand(
            "g.mapset", read=True, flags="l", project=location, dbase=dbase
        )

        if not ret:
            return listOfMapsets

        for line in ret.rstrip().splitlines():
            listOfMapsets += line.split(" ")
    else:
        for mapset in glob.glob(os.path.join(dbase, location, "*")):
            if os.path.isdir(mapset) and os.path.isfile(
                os.path.join(dbase, location, mapset, "WIND")
            ):
                listOfMapsets.append(os.path.basename(mapset))

    ListSortLower(listOfMapsets)
    return listOfMapsets


def GetColorTables():
    """Get list of color tables"""
    ret = RunCommand("r.colors", read=True, flags="l")
    if not ret:
        return []

    return ret.splitlines()


def _getGDALFormats():
    """Get dictionary of available GDAL drivers"""
    try:
        ret = grass.read_command("r.in.gdal", quiet=True, flags="f")
    except grass.CalledModuleError:
        ret = None

    return _parseFormats(ret), _parseFormats(ret, writableOnly=True)


def _getOGRFormats():
    """Get dictionary of available OGR drivers"""
    try:
        ret = grass.read_command("v.in.ogr", quiet=True, flags="f")
    except grass.CalledModuleError:
        ret = None

    return _parseFormats(ret), _parseFormats(ret, writableOnly=True)


def _parseFormats(output, writableOnly=False):
    """Parse r.in.gdal/v.in.ogr -f output"""
    formats = {"file": {}, "database": {}, "protocol": {}}

    if not output:
        return formats

    patt = None
    if writableOnly:
        patt = re.compile(r"\(rw\+?\)$", re.IGNORECASE)

    for line in output.splitlines():
        key, name = (x.strip() for x in line.strip().split(":", 1))
        if writableOnly and not patt.search(key):
            continue

        if name in {"Memory", "Virtual Raster", "In Memory Raster"}:
            continue
        if name in {
            "PostgreSQL",
            "PostgreSQL/PostGIS",
            "SQLite",
            "SQLite / Spatialite",
            "ODBC",
            "ESRI Personal GeoDatabase",
            "Rasterlite",
            "PostGIS WKT Raster driver",
            "PostGIS Raster driver",
            "CouchDB",
            "MSSQLSpatial",
            "FileGDB",
        }:
            formats["database"][key.split(" ")[0]] = name
        elif name in {
            "GeoJSON",
            "OGC Web Coverage Service",
            "OGC Web Map Service",
            "WFS",
            "GeoRSS",
            "HTTP Fetching Wrapper",
        }:
            formats["protocol"][key.split(" ")[0]] = name
        else:
            formats["file"][key.split(" ")[0]] = name

    for k, v in formats.items():
        formats[k] = dict(sorted(v.items(), key=operator.itemgetter(1)))

    return formats


formats = None


def GetFormats(writableOnly=False):
    """Get GDAL/OGR formats"""
    global formats
    if not formats:
        gdalAll, gdalWritable = _getGDALFormats()
        ogrAll, ogrWritable = _getOGRFormats()
        formats = {
            "all": {
                "gdal": gdalAll,
                "ogr": ogrAll,
            },
            "writable": {
                "gdal": gdalWritable,
                "ogr": ogrWritable,
            },
        }

    if writableOnly:
        return formats["writable"]

    return formats["all"]


rasterFormatExtension = {
    "GeoTIFF": "tif",
    "Erdas Imagine Images (.img)": "img",
    "Ground-based SAR Applications Testbed File Format (.gff)": "gff",
    "Arc/Info Binary Grid": "adf",
    "Portable Network Graphics": "png",
    "JPEG JFIF": "jpg",
    "Japanese DEM (.mem)": "mem",
    "Graphics Interchange Format (.gif)": "gif",
    "X11 PixMap Format": "xpm",
    "MS Windows Device Independent Bitmap": "bmp",
    "SPOT DIMAP": "dim",
    "RadarSat 2 XML Product": "xml",
    "EarthWatch .TIL": "til",
    "ERMapper .ers Labelled": "ers",
    "ERMapper Compressed Wavelets": "ecw",
    "GRIdded Binary (.grb)": "grb",
    "EUMETSAT Archive native (.nat)": "nat",
    "Idrisi Raster A.1": "rst",
    "Golden Software ASCII Grid (.grd)": "grd",
    "Golden Software Binary Grid (.grd)": "grd",
    "Golden Software 7 Binary Grid (.grd)": "grd",
    "R Object Data Store": "r",
    "USGS DOQ (Old Style)": "doq",
    "USGS DOQ (New Style)": "doq",
    "ENVI .hdr Labelled": "hdr",
    "ESRI .hdr Labelled": "hdr",
    "Generic Binary (.hdr Labelled)": "hdr",
    "PCI .aux Labelled": "aux",
    "EOSAT FAST Format": "fst",
    "VTP .bt (Binary Terrain) 1.3 Format": "bt",
    "FARSITE v.4 Landscape File (.lcp)": "lcp",
    "Swedish Grid RIK (.rik)": "rik",
    "USGS Optional ASCII DEM (and CDED)": "dem",
    "Northwood Numeric Grid Format .grd/.tab": "",
    "Northwood Classified Grid Format .grc/.tab": "",
    "ARC Digitized Raster Graphics": "arc",
    "Magellan topo (.blx)": "blx",
    "SAGA GIS Binary Grid (.sdat)": "sdat",
    "GeoPackage (.gpkg)": "gpkg",
}


vectorFormatExtension = {
    "ESRI Shapefile": "shp",
    "GeoPackage": "gpkg",
    "UK .NTF": "ntf",
    "SDTS": "ddf",
    "DGN": "dgn",
    "VRT": "vrt",
    "REC": "rec",
    "BNA": "bna",
    "CSV": "csv",
    "GML": "gml",
    "GPX": "gpx",
    "KML": "kml",
    "GMT": "gmt",
    "PGeo": "mdb",
    "XPlane": "dat",
    "AVCBin": "adf",
    "AVCE00": "e00",
    "DXF": "dxf",
    "Geoconcept": "gxt",
    "GeoRSS": "xml",
    "GPSTrackMaker": "gtm",
    "VFK": "vfk",
    "SVG": "svg",
}


def GetSettingsPath():
    """Get full path to the settings directory"""
    version_major, version_minor, _ = grass.version()["version"].split(".")
    return get_grass_config_dir(version_major, version_minor, os.environ)


def StoreEnvVariable(key, value=None, envFile=None):
    """Store environmental variable

    If value is not given (is None) then environmental variable is
    unset.

    :param key: env key
    :param value: env value
    :param envFile: path to the environmental file (None for default location)
    """
    windows = sys.platform == "win32"
    if not envFile:
        gVersion = grass.version()["version"].split(".", 1)[0]
        if not windows:
            envFile = os.path.join(os.getenv("HOME"), ".grass%s" % gVersion, "bashrc")
        else:
            envFile = os.path.join(
                os.getenv("APPDATA"), "GRASS%s" % gVersion, "env.bat"
            )

    # read env file
    environ = {}
    lineSkipped = []
    if os.path.exists(envFile):
        try:
            with open(envFile) as fd:
                for line in fd:
                    line = line.rstrip(os.linesep)
                    try:
                        k, v = (x.strip() for x in line.split(" ", 1)[1].split("=", 1))
                    except Exception as e:
                        sys.stderr.write(
                            _("%s: line skipped - unable to parse '%s'\nReason: %s\n")
                            % (envFile, line, e)
                        )
                        lineSkipped.append(line)
                        continue
                    if k in environ:
                        sys.stderr.write(_("Duplicated key: %s\n") % k)
                    environ[k] = v
        except OSError as error:
            sys.stderr.write(
                _("Unable to open file '{name}': {error}\n").format(
                    name=envFile, error=error
                )
            )
            return

    # update environmental variables
    if value is None:
        environ.pop(key, None)
    else:
        environ[key] = value

    # write update env file
    try:
        with open(envFile, "w") as fd:
            expCmd = "set" if windows else "export"

            fd.writelines(
                "%s %s=%s\n" % (expCmd, key, value) for key, value in environ.items()
            )

            # write also skipped lines
            fd.writelines(line + os.linesep for line in lineSkipped)
    except OSError as error:
        sys.stderr.write(
            _("Unable to create file '{name}': {error}\n").format(
                name=envFile, error=error
            )
        )
        return


def SetAddOnPath(addonPath=None, key="PATH"):
    """Set default AddOn path

    :param addonPath: path to addons (None for default)
    :param key: env key - 'PATH' or 'BASE'
    """
    gVersion = grass.version()["version"].split(".", 1)[0]
    # update env file
    if not addonPath:
        if sys.platform != "win32":
            addonPath = os.path.join(
                os.path.join(os.getenv("HOME"), ".grass%s" % gVersion, "addons")
            )
        else:
            addonPath = os.path.join(
                os.path.join(os.getenv("APPDATA"), "GRASS%s" % gVersion, "addons")
            )

    StoreEnvVariable(key="GRASS_ADDON_" + key, value=addonPath)
    os.environ["GRASS_ADDON_" + key] = addonPath

    # update path
    if addonPath not in os.environ["PATH"]:
        os.environ["PATH"] = addonPath + os.pathsep + os.environ["PATH"]


# predefined colors and their names
# must be in sync with lib/gis/color_str.c
str2rgb = {
    "aqua": (100, 128, 255),
    "black": (0, 0, 0),
    "blue": (0, 0, 255),
    "brown": (180, 77, 25),
    "cyan": (0, 255, 255),
    "gray": (128, 128, 128),
    "grey": (128, 128, 128),
    "green": (0, 255, 0),
    "indigo": (0, 128, 255),
    "magenta": (255, 0, 255),
    "orange": (255, 128, 0),
    "red": (255, 0, 0),
    "violet": (128, 0, 255),
    "purple": (128, 0, 255),
    "white": (255, 255, 255),
    "yellow": (255, 255, 0),
}

rgb2str = {r: s for s, r in str2rgb.items()}
# ensure that gray value has 'gray' string and not 'grey'
rgb2str[str2rgb["gray"]] = "gray"
# purple is defined as nickname for violet in lib/gis
# (although Wikipedia says that purple is (128, 0, 128))
# we will prefer the defined color, not nickname
rgb2str[str2rgb["violet"]] = "violet"


def color_resolve(color):
    if len(color) > 0 and color[0] in digits:
        rgb = tuple(map(int, color.split(":")))
        label = color
    else:
        # Convert color names to RGB
        try:
            rgb = str2rgb[color]
            label = color
        except KeyError:
            rgb = (200, 200, 200)
            label = _("Select Color")
    return (rgb, label)


command2ltype = {
    "d.rast": "raster",
    "d.rast3d": "raster_3d",
    "d.rgb": "rgb",
    "d.his": "his",
    "d.shade": "shaded",
    "d.legend": "rastleg",
    "d.rast.arrow": "rastarrow",
    "d.rast.num": "rastnum",
    "d.rast.leg": "maplegend",
    "d.vect": "vector",
    "d.vect.thematic": "thememap",
    "d.vect.chart": "themechart",
    "d.grid": "grid",
    "d.geodesic": "geodesic",
    "d.rhumbline": "rhumb",
    "d.labels": "labels",
    "d.barscale": "barscale",
    "d.redraw": "redraw",
    "d.wms": "wms",
    "d.histogram": "histogram",
    "d.colortable": "colortable",
    "d.graph": "graph",
    "d.out.file": "export",
    "d.to.rast": "torast",
    "d.text": "text",
    "d.northarrow": "northarrow",
    "d.polar": "polar",
    "d.legend.vect": "vectleg",
}
ltype2command = {ltype: cmd for cmd, ltype in command2ltype.items()}


def GetGEventAttribsForHandler(method, event):
    """Get attributes from event, which can be used by handler method.

    Be aware of event class attributes.

    :param method: handler method (including self arg)
    :param event: event

    :return: (valid kwargs for method,
             list of method's args without default value
             which were not found among event attributes)
    """
    args_spec = inspect.getargspec(method)

    args = args_spec[0]

    defaults = []
    if args_spec[3]:
        defaults = args_spec[3]

    # number of arguments without def value
    req_args = len(args) - 1 - len(defaults)

    kwargs = {}
    missing_args = []

    for i, a in enumerate(args):
        if hasattr(event, a):
            kwargs[a] = getattr(event, a)
        elif i < req_args:
            missing_args.append(a)

    return kwargs, missing_args


def PilImageToWxImage(pilImage: PIL.Image.Image, copyAlpha: bool = True) -> wx.Image:
    """Convert PIL image to wx.Image

    Based on http://wiki.wxpython.org/WorkingWithImages
    """
    from gui_core.wrap import EmptyImage

    hasAlpha = pilImage.mode[-1] == "A"
    if copyAlpha and hasAlpha:  # Make sure there is an alpha layer copy.
        wxImage = EmptyImage(*pilImage.size)
        pilImageCopyRGBA = pilImage.copy()
        pilImageCopyRGB = pilImageCopyRGBA.convert("RGB")  # RGBA --> RGB
        wxImage.SetData(pilImageCopyRGB.tobytes())
        # Create layer and insert alpha values.
        if wxPythonPhoenix:
            wxImage.SetAlpha(pilImageCopyRGBA.tobytes()[3::4])
        else:
            wxImage.SetAlphaData(pilImageCopyRGBA.tobytes()[3::4])

    else:  # The resulting image will not have alpha.
        wxImage = EmptyImage(*pilImage.size)
        pilImageCopy = pilImage.copy()
        # Discard any alpha from the PIL image.
        pilImageCopyRGB = pilImageCopy.convert("RGB")
        wxImage.SetData(pilImageCopyRGB.tobytes())

    return wxImage


def autoCropImageFromFile(filename) -> wx.Image:
    """Loads image from file and crops it automatically.

    If PIL is not installed, it does not crop it.

    :param filename: path to file
    :return: wx.Image instance
    """
    try:
        from PIL import Image

        pilImage = Image.open(filename)
        imageBox = pilImage.getbbox()
        cropped = pilImage.crop(imageBox)
        return PilImageToWxImage(cropped, copyAlpha=True)
    except ImportError:
        import wx

        return wx.Image(filename)


def isInRegion(regionA, regionB) -> bool:
    """Tests if 'regionA' is inside of 'regionB'.

    For example, region A is a display region and region B is some reference
    region, e.g., a computational region.

    >>> displayRegion = {'n': 223900, 's': 217190, 'w': 630780, 'e': 640690}
    >>> compRegion = {'n': 228500, 's': 215000, 'w': 630000, 'e': 645000}
    >>> isInRegion(displayRegion, compRegion)
    True
    >>> displayRegion = {'n':226020, 's': 212610, 'w': 626510, 'e': 646330}
    >>> isInRegion(displayRegion, compRegion)
    False

    :param regionA: input region A as dictionary
    :param regionB: input region B as dictionary

    :return: True if region A is inside of region B
    :return: False otherwise
    """
    return bool(
        regionA["s"] >= regionB["s"]
        and regionA["n"] <= regionB["n"]
        and regionA["w"] >= regionB["w"]
        and regionA["e"] <= regionB["e"]
    )


def do_doctest_gettext_workaround():
    """Setups environment for doing a doctest with gettext usage.

    When using gettext with dynamically defined underscore function
    (`_("For translation")`), doctest does not work properly. One option is to
    use `import as` instead of dynamically defined underscore function but this
    would require change all modules which are used by tested module. This
    should be considered for the future. The second option is to define dummy
    underscore function and one other function which creates the right
    environment to satisfy all. This is done by this function.
    """

    def new_displayhook(string):
        """A replacement for default `sys.displayhook`"""
        if string is not None:
            sys.stdout.write("%r\n" % (string,))

    def new_translator(string):
        """A fake gettext underscore function."""
        return string

    sys.displayhook = new_displayhook

    import builtins

    builtins.__dict__["_"] = new_translator


def doc_test():
    """Tests the module using doctest

    :return: a number of failed tests
    """
    import doctest

    do_doctest_gettext_workaround()
    return doctest.testmod().failed


def registerPid(pid):
    """Register process id as GUI_PID GRASS variable

    :param: pid process id
    """
    env = grass.gisenv()
    guiPid = []
    if "GUI_PID" in env:
        guiPid = env["GUI_PID"].split(",")
    guiPid.append(str(pid))
    grass.run_command("g.gisenv", set="GUI_PID={0}".format(",".join(guiPid)))


def unregisterPid(pid):
    """Unregister process id from GUI_PID GRASS variable

    :param: pid process id
    """
    env = grass.gisenv()
    if "GUI_PID" not in env:
        return

    guiPid = env["GUI_PID"].split(",")
    pid = str(os.getpid())
    if pid in guiPid:
        guiPid.remove(pid)
        grass.run_command("g.gisenv", set="GUI_PID={0}".format(",".join(guiPid)))


def get_shell_pid(env=None):
    """Get shell PID from the GIS environment or None"""
    try:
        return int(grass.gisenv(env=env)["PID"])
    except (KeyError, ValueError) as error:
        Debug.msg(
            1, "No PID for GRASS shell (assuming no shell running): {}".format(error)
        )
        return None


def is_shell_running() -> bool:
    """Return True if a separate shell is registered in the GIS environment"""
    return get_shell_pid() is not None


def parse_mapcalc_cmd(command):
    """Parse r.mapcalc/r3.mapcalc module command

    >>> parse_mapcalc_cmd(command="r.mapcalc map = 1")
    "r.mapcalc expression='map = 1'"

    >>> parse_mapcalc_cmd(command="r.mapcalc map =    1")
    "r.mapcalc expression='map = 1'"

    >>> parse_mapcalc_cmd(command="r.mapcalc map=1")
    "r.mapcalc expression='map=1'"

    >>> parse_mapcalc_cmd(command="r.mapcalc map = a - b")
    "r.mapcalc expression='map = a - b'"

    >>> parse_mapcalc_cmd(command="r.mapcalc expression=map = a - b")
    "r.mapcalc expression='map = a - b'"

    >>> parse_mapcalc_cmd(command="r.mapcalc expression=map = a -     b")
    "r.mapcalc expression='map = a - b'"

    >>> cmd = "r.mapcalc expr='map = a - b' region=clip --overwrite"
    >>> parse_mapcalc_cmd(command=cmd)
    "r.mapcalc --overwrite expr='map = a - b' region=clip"

    >>> cmd = 'r.mapcalc expr="map = a - b" region=clip --overwrite'
    >>> parse_mapcalc_cmd(command=cmd)
    "r.mapcalc --overwrite expr='map = a - b' region=clip"

    >>> cmd = "r.mapcalc -s map = (a - b) / c region=clip --overwrite --verbose"
    >>> parse_mapcalc_cmd(command=cmd)
    "r.mapcalc -s --overwrite --verbose expression='map = (a - b) / c' region=clip"

    >>> cmd = 'r.mapcalc e="map = 1" region=clip --overwrite'
    >>> parse_mapcalc_cmd(command=cmd)
    "r.mapcalc --overwrite e='map = 1' region=clip"

    >>> cmd = 'r.mapcalc ex="map = 1" region=clip --overwrite'
    >>> parse_mapcalc_cmd(command=cmd)
    "r.mapcalc --overwrite ex='map = 1' region=clip"

    >>> cmd = 'r.mapcalc exp="map = 1" region=clip --overwrite'
    >>> parse_mapcalc_cmd(command=cmd)
    "r.mapcalc --overwrite exp='map = 1' region=clip"

    >>> cmd = 'r.mapcalc expr="map = 1" region=clip --overwrite'
    >>> parse_mapcalc_cmd(command=cmd)
    "r.mapcalc --overwrite expr='map = 1' region=clip"

    >>> cmd = 'r.mapcalc expre="map = 1" region=clip --overwrite'
    >>> parse_mapcalc_cmd(command=cmd)
    "r.mapcalc --overwrite expre='map = 1' region=clip"

    >>> cmd = 'r.mapcalc expres="map = 1" region=clip --overwrite'
    >>> parse_mapcalc_cmd(command=cmd)
    "r.mapcalc --overwrite expres='map = 1' region=clip"

    >>> cmd = 'r.mapcalc express="map = 1" region=clip --overwrite'
    >>> parse_mapcalc_cmd(command=cmd)
    "r.mapcalc --overwrite express='map = 1' region=clip"

    >>> cmd = 'r.mapcalc expressi="map = 1" region=clip --overwrite'
    >>> parse_mapcalc_cmd(command=cmd)
    "r.mapcalc --overwrite expressi='map = 1' region=clip"

    >>> cmd = 'r.mapcalc expressio="map = 1" region=clip --overwrite'
    >>> parse_mapcalc_cmd(command=cmd)
    "r.mapcalc --overwrite expressio='map = 1' region=clip"

    >>> cmd = 'r.mapcalc expression="map = 1" region=clip --overwrite'
    >>> parse_mapcalc_cmd(command=cmd)
    "r.mapcalc --overwrite expression='map = 1' region=clip"

    >>> cmd = 'r.mapcalc exp="map = a + e" region=clip --overwrite'
    >>> parse_mapcalc_cmd(command=cmd)
    "r.mapcalc --overwrite exp='map = a + e' region=clip"

    >>> cmd = 'r.mapcalc exp="map = a + exp(5)" region=clip --overwrite'
    >>> parse_mapcalc_cmd(command=cmd)
    "r.mapcalc --overwrite exp='map = a + exp(5)' region=clip"

    :param str command: r.mapcalc command string

    :return str: parsed r.mapcalc command string
    """
    flags = []
    others_params_args = []
    expr_param_regex = re.compile(r"e.*?=")
    flag_regex = re.compile(
        r"^-[a-z]|^--overwrite|^--quiet|^--verbose|^--help|^--o|^--q|^--v|^--h",
    )

    command = split(command)
    module = command.pop(0)

    for arg in command[:]:
        flag = flag_regex.search(arg)
        if flag:
            flags.append(command.pop(command.index(flag.group())))
        elif "region=" in arg or "file=" in arg or "seed=" in arg:
            others_params_args.append(command.pop(command.index(arg)))

    cmd = " ".join(command)
    expr_param = expr_param_regex.search(cmd)
    if not expr_param:
        expr_param_name = "expression="
    else:
        # Remove expression param
        command = split(cmd.replace(expr_param.group(), ""))
        expr_param_name = expr_param.group()
    # Add quotes
    if "'" not in cmd or '"' not in cmd:
        cmd = f"'{' '.join(command)}'"
    expression_param_arg = f"{expr_param_name}{cmd}"

    return " ".join(
        [
            module,
            *flags,
            expression_param_arg,
            *others_params_args,
        ]
    )


def replace_module_cmd_special_flags(command):
    """Replace module command special flags short version with
    full version

    Flags:

    --o -> --overwrite
    --q -> --quiet
    --v -> --verbose
    --h -> --help

    >>> cmd = "r.mapcalc -s --o --v expression='map = 1' region=clip"
    >>> replace_module_cmd_special_flags(command=cmd)
    "r.mapcalc -s --overwrite --verbose expression='map = 1' region=clip"

    >>> cmd = "r.mapcalc -s --o --q expression='map = 1' region=clip"
    >>> replace_module_cmd_special_flags(command=cmd)
    "r.mapcalc -s --overwrite --quiet expression='map = 1' region=clip"

    :param str command: module command string

    :return str: module command string with replaced flags
    """
    flags_regex = re.compile(
        r"(--o(\s+|$))|(--q(\s+|$))|(--v(\s+|$))|(--h(\s+|$))",
    )
    replace = {
        "--o": "--overwrite ",
        "--q": "--quiet ",
        "--v": "--verbose ",
        "--h": "--help ",
    }
    return flags_regex.sub(
        lambda flag: replace[flag.group().strip()],
        command,
    )


if __name__ == "__main__":
    sys.exit(doc_test())

#!/usr/bin/env python3
###############################################################################
# Project:  ProjPicker (Projection Picker)
#           <https://github.com/HuidaeCho/projpicker>
# Authors:  Huidae Cho, Owen Smith
#           Institute for Environmental and Spatial Analysis
#           University of North Georgia
# Since:    May 27, 2021
#
# Copyright (C) 2021 Huidae Cho <https://faculty.ung.edu/hcho/> and
#                    Owen Smith <https://www.gaderian.io/>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
###############################################################################
"""
This module implements the CLI and API of ProjPicker.
"""

import collections
import os
import sys
import argparse
import sqlite3
import re
import math
import json
import pprint

from grass.script.setup import set_gui_path
set_gui_path()
from projpicker_gui import gui

from .common import (BBox, coor_sep, pos_float_pat, bbox_schema,
                     bbox_columns, is_verbose, get_float)
from . import coor_latlon
from . import coor_xy

# module path
module_path = os.path.dirname(__file__)

# environment variables for default paths
projpicker_db_env = "PROJPICKER_DB"
proj_db_env = "PROJ_DB"
# https://proj.org/usage/environmentvars.html
proj_lib_env = "PROJ_LIB"

# Earth parameters from https://en.wikipedia.org/wiki/Earth_radius#Global_radii
# equatorial radius in km
rx = 6378.1370
# polar radius in km
ry = 6356.7523

geom_var_chars = "([a-zA-Z0-9_]+)"
geom_var_re = re.compile(f"^(?:{geom_var_chars}:|:{geom_var_chars}:|"
                         f":{geom_var_chars})$")

# geometry-bbox namedtuple class
GeomBBox = collections.namedtuple("GeomBBox", "is_latlon type geom bbox")
# geometry namedtuple class
Geom = collections.namedtuple("Geom", "is_latlon type geom")


###############################################################################
# generic

def message(*args, end=None):
    """
    Print args to stderr immediately.

    Args:
        *args (arguments): Arguments to print. Passed to print().
        end (str): Passed to print(). Defaults to None.
    """
    print(*args, end=end, file=sys.stderr, flush=True)


def read_file(infile="-"):
    """
    Read a file (stdin by default) and return a list of str lines.

    Args:
        infile (str): Input filename. Defaults to "-" for stdin.

    Returns:
        list: List of str lines read from infile.

    Raises:
        Exception: If infile does not exist.
    """
    if infile in (None, ""):
        infile = "-"

    if infile == "-":
        f = sys.stdin
    elif not os.path.isfile(infile):
        raise Exception(f"{infile}: No such file found")
    else:
        f = open(infile)

    lines = f.readlines()

    if infile != "-":
        f.close()
    return lines


def tidy_lines(lines):
    """
    Tidy a list of str lines in place by removing leading and trailing
    whitespaces including newlines. Comments start with a hash and comment-only
    lines are deleted as if they did not even exist. A line starting with
    whitespaces immediately followed by a comment is considered a comment-only
    line and deleted. This function directly modifies the input list and does
    not return anything.

    Args:
        lines (list): List of str lines.
    """
    for i in reversed(range(len(lines))):
        if lines[i].startswith("#"):
            del lines[i]
        elif i > 0 and lines[i].strip() == lines[i-1].strip() == "":
            del lines[i]
        else:
            commented = False
            if "#" in lines[i]:
                lines[i] = lines[i].split("#")[0]
                commented = True
            lines[i] = lines[i].strip()
            if commented and lines[i] == "":
                del lines[i]
            elif " " in lines[i] or "\t" in lines[i]:
                words = lines[i].split()
                all_nums = True
                for word in words:
                    if not re.match(f"^[+-]?{pos_float_pat}", word):
                        all_nums = False
                        break
                n = len(words)
                if (all_nums and n in (2, 4) and coor_sep not in lines[i] and
                    "=" not in lines[i]):
                    # normalize lat lon to lat,lon for multiple geometries per
                    # line; avoid any constraining directives using =
                    lines[i] = coor_sep.join(words)
                elif (words[0].startswith("unit=") and '"' not in words[0] and
                      "'" not in words[0]):
                    # protect whitespaces in constraining directives
                    m = re.match("""^([^ =]+=)([^"'].*)$""", lines[i])
                    if m:
                        quote = "'" if '"' in m[2] else '"'
                        lines[i] = f"{m[1]}{quote}{m[2]}{quote}"
    if lines and lines[0] == "":
        del lines[0]

    # protect empty lines as geometry separators
    text = " ".join("\0" if line == "" else line for line in lines)
    lines.clear()
    # revert geometry separators back to empty lines
    lines.extend("" if line == "\0" else line for line in text.split())
    normalize_lines(lines)


def normalize_lines(lines):
    """
    Normalize a list of str lines in place by splitting combined lines into
    individual lines.

    Args:
        lines (list): List of str lines.
    """
    idx = []
    n = len(lines)
    i = 0
    while i < n:
        m = re.match("""^(|[a-z_]+=)(["'])(.*)$""", lines[i])
        if m:
            lines[i] = m[1] + m[3]
            quote = m[2]
            if lines[i].endswith(quote):
                lines[i] = lines[i][:-len(quote)]
            else:
                for j in range(i+1, n):
                    idx.append(j)
                    m = re.match(f"^(.*){quote}$", lines[j])
                    if m:
                        lines[i] += f" {m[1]}"
                        break
                    else:
                        lines[i] += f" {lines[j]}"
                i = j
        i += 1
    for i in reversed(idx):
        del lines[i]


def get_separator(separator):
    r"""
    Convert a separator name to its corresponding character. If an unsupported
    name is given, return it as is.

    Args:
        separator (str): Separator name. It supports special names including
            pipe (|), comma (,), space ( ), tab (\t), and newline (\n).

    Returns:
        str: Separator character.
    """
    sep_dic = {
            "pipe": "|",
            "comma": ",",
            "space": " ",
            "tab": "\t",
            "newline": "\n"}
    if separator in sep_dic:
        separator = sep_dic[separator]
    return separator


###############################################################################
# Earth parameters

def calc_xy_at_lat_scaling(lat):
    """
    Calculate x and y at a given latitude on Earth's cross section that passes
    through the South and North Poles. The x- and y-axes are from the center to
    the right equatorial point and North Pole, respectively. The x-y space is
    first scaled to [-1, 1]**2, which is then rescaled back to x-y later.

    Args:
        lat (float): Latitude in decimal degrees.

    Returns:
        float, float: x and y.

    Raises:
        Exception: If lat is outside [-90, 90].
    """
    if not -90 <= lat <= 90:
        raise Exception(f"{lat}: Invalid latitude")

    # (x/rx)**2 + (y/ry)**2 = 1
    # x = rx*cos(theta2)
    # y = ry*sin(theta2)
    # theta2 = atan2(rx*tan(theta), ry)
    theta = lat/180*math.pi
    theta2 = math.atan2(rx*math.tan(theta), ry)
    x = rx*math.cos(theta2)
    y = ry*math.sin(theta2)
    return x, y


def calc_xy_at_lat_noscaling(lat):
    """
    Calculate x and y at a given latitude on Earth's cross section that passes
    through the South and North Poles. The x- and y-axes are from the center to
    the right equatorial point and North Pole, respectively. The radius at the
    latitude is first determined, and x and y are calculated using trigonometry
    functions.

    Args:
        lat (float): Latitude in decimal degrees.

    Returns:
        float, float: x and y.
    """
    # (x/rx)**2 + (y/ry)**2 = (r*cos(theta)/rx)**2 + (r*sin(theta)/ry)**2 = 1
    r = calc_radius_at_lat(lat)
    theta = lat/180*math.pi
    x = r*math.cos(theta)
    y = r*math.sin(theta)
    return x, y


# use the shorter version of calc_xy_at_lat_*scaling()
calc_xy_at_lat = calc_xy_at_lat_scaling


def calc_horiz_radius_at_lat(lat):
    """
    Calculate the horizontal distance from the y-axis to the latitude line.

    Args:
        lat (float): Latitude in decimal degrees.

    Returns:
        float: Radius.
    """
    return calc_xy_at_lat(lat)[0]


def calc_radius_at_lat(lat):
    """
    Calculate the distance from the center to the latitude line.

    Args:
        lat (float): Latitude in decimal degrees.

    Returns:
        float: Radius.

    Raises:
        Exception: If lat is outside [-90, 90].
    """
    if not -90 <= lat <= 90:
        raise Exception(f"{lat}: Invalid latitude")

    # (x/rx)**2 + (y/ry)**2 = (r*cos(theta)/rx)**2 + (r*sin(theta)/ry)**2 = 1
    theta = lat/180*math.pi
    r = math.sqrt((rx*ry)**2/((math.cos(theta)*ry)**2+(math.sin(theta)*rx)**2))
    return r


def calc_area(bbox):
    """
    Calculate the surface area of the segment defined by south, north, west,
    and east floats in decimal degrees. North latitude must be greater than or
    equal to south latitude, but east longitude can be less than west longitude
    wieh the segment crosses the antimeridian.

    Args:
        bbox (list): List of south, north, west, and east floats in decimal
            degrees.

    Returns:
        float: Area in square kilometers.

    Raises:
        Exception: If s or n is outside [-90, 90], or s is greater than n.
    """
    s, n, w, e = bbox

    if not -90 <= s <= 90:
        raise Exception(f"{s}: Invalid south latitude")
    if not -90 <= n <= 90:
        raise Exception(f"{n}: Invalid south latitude")
    if s > n:
        raise Exception(f"South ({s}) greater than north ({n})")

    lats = []
    nlats = math.ceil(n-s)+1
    for i in range(nlats-1):
        lats.append(s+i)
    lats.append(n)

    if w == e or (w == -180 and e == 180):
        dlon = 360
    elif w < e:
        dlon = e-w
    else:
        dlon = 360-w+e
    dlon *= math.pi/180

    area = 0
    for i in range(nlats-1):
        b = lats[i]
        t = lats[i+1]
        r = calc_horiz_radius_at_lat((b+t)/2)
        width = r*dlon
        xb, yb = calc_xy_at_lat(b)
        xt, yt = calc_xy_at_lat(t)
        height = math.sqrt((xt-xb)**2+(yt-yb)**2)
        area += width*height
    return area


###############################################################################
# version and default paths

def get_version():
    """
    Return the ProjPicker version str from the VERSION file.

    Returns:
        str: ProjPicker version.
    """
    with open(os.path.join(module_path, "VERSION")) as f:
        version = f.read().strip()
    return version


def get_projpicker_db(projpicker_db=None):
    """
    Return the projpicker.db path. If one is given as an argument, return it as
    is. Otherwise (None), check the PROJPICKER_DB environment variable. If this
    variable is not available, return the default "projpicker.db".

    Args:
        projpicker_db (str): User-provided projpicker.db path. Defaults to
            None.

    Returns:
        str: projpicker.db path.
    """
    if projpicker_db is None:
        if projpicker_db_env in os.environ:
            projpicker_db = os.environ[projpicker_db_env]
        else:
            projpicker_db = os.path.join(module_path, "projpicker.db")
    return projpicker_db


def get_proj_db(proj_db=None):
    """
    Return the proj.db path. If one is given as an argument, return it as is.
    Otherwise (None), check the PROJ_DB environment variable. If this variable
    is not available, check the PROJ_LIB environment variable as it is likely
    to be set by PROJ. If none works, return the default
    "/usr/share/proj/proj.db".

    Args:
        proj_db (str): User-provided proj.db path. Defaults to None.

    Returns:
        str: proj.db path.
    """
    if proj_db is None:
        if proj_db_env in os.environ:
            proj_db = os.environ[proj_db_env]
        else:
            proj_lib = os.environ.get(proj_lib_env, "/usr/share/proj")
            proj_db = os.path.join(proj_lib, "proj.db")
    return proj_db


###############################################################################
# projpicker.db creation

def find_unit(proj_table, crs_auth, crs_code, proj_cur):
    """
    Find and return the unit of a given coordinate reference system (CRS) using
    the cursor.

    Args:
        proj_table (str): Name of a CRS table in proj.db.
        crs_auth: CRS authority name
        crs_code: CRS code
        proj_cur (sqlite3.Cursor): proj.db cursor.

    Returns:
        str: Unit name.

    Raises:
        Exception: If no or multiple units of measure are found.
    """
    if proj_table == "compound_crs":
        sql = f"""SELECT table_name, horiz_crs_auth_name, horiz_crs_code
                  FROM compound_crs cc
                  JOIN crs_view c
                    ON horiz_crs_auth_name=c.auth_name AND
                       horiz_crs_code=c.code
                  WHERE cc.auth_name='{crs_auth}' AND cc.code='{crs_code}'
                  ORDER BY horiz_crs_auth_name, horiz_crs_code"""
        proj_cur.execute(sql)
        (table, auth, code) = proj_cur.fetchone()
    else:
        table = proj_table
        auth = crs_auth
        code = crs_code
    sql = f"""SELECT orientation,
                     uom.auth_name, uom.code,
                     uom.name
              FROM {table} c
              JOIN axis a
                ON c.coordinate_system_auth_name=a.coordinate_system_auth_name
                   AND
                   c.coordinate_system_code=a.coordinate_system_code
              JOIN unit_of_measure uom
                ON a.uom_auth_name=uom.auth_name AND
                   a.uom_code=uom.code
              WHERE c.auth_name='{auth}' AND c.code='{code}'
              ORDER BY uom.auth_name, uom.code"""
    proj_cur.execute(sql)
    nuoms = 0
    uom_auth = uom_code = unit = None
    for uom_row in proj_cur.fetchall():
        (orien,
         um_auth, um_code,
         um_name) = uom_row
        if table != "vertical_crs" and orien in ("up", "down"):
            continue
        if um_auth != uom_auth or um_code != uom_code:
            uom_auth = um_auth
            uom_code = um_code
            unit = um_name
            nuoms += 1
    if nuoms == 0:
        sql = f"""SELECT text_definition
                  FROM {table}
                  WHERE auth_name='{auth}' AND code='{code}'"""
        proj_cur.execute(sql)
        unit = re.sub("^.*\"([^\"]+)\".*$", r"\1",
               re.sub("[A-Z]*\[.*\[.*\],?", "",
               re.sub("UNIT\[([^]]+)\]", r"\1",
               re.sub("^PROJCS\[[^,]*,|\]$", "",
                      proj_cur.fetchone()[0]))))
        if unit == "":
            raise Exception(f"{crs_auth}:{crs_code}: No units?")
    elif nuoms > 1:
        raise Exception(f"{crs_auth}:{crs_code}: Multiple units?")

    # use GRASS unit names
    unit = unit.replace(
        "Meter", "meter").replace(
        "metre", "meter").replace(
        "Foot_US", "US foot").replace(
        "US survey foot", "US foot").replace(
        "_Kilo", " kilo").replace(
        " (supplier to define representation)", "")

    return unit


def transform_xy_point(point, from_crs):
    """
    Transform a point defined by x and y floats in the from_crs CRS to latitude
    and longitude floats in decimal degrees in EPSG:4326. It requires the
    pyproj module.

    Args:
        point (list): List of x and y floats.
        from_crs (str): Source CRS.

    Returns:
        float, float: Latitude and longitude in decimal degrees.
    """
    import pyproj

    x, y = point
    trans = pyproj.Transformer.from_crs(from_crs, "EPSG:4326", always_xy=True)
    lon, lat = trans.transform(x, y)
    return lat, lon


def transform_latlon_point(point, to_crs):
    """
    Transform a point defined by latitude and longitude floats in decimal
    degrees in EPSG:4326 to the projected x and y floats in the to_crs CRS. It
    requires the pyproj module.

    Args:
        point (list): List of latitude and longitude floats in decimal degrees.
        to_crs (str): Target CRS.

    Returns:
        float, float: x and y floats.
    """
    import pyproj

    lat, lon = point
    trans = pyproj.Transformer.from_crs("EPSG:4326", to_crs, always_xy=True)
    x, y = trans.transform(lon, lat)
    return x, y


def transform_latlon_bbox(bbox, to_crs):
    """
    Transform a bbox defined by south, north, west, and east floats in decimal
    degrees in EPSG:4326 to the projected bbox in the to_crs CRS defined by
    bottom, top, left, and right floats in to_crs units. It requires the pyproj
    module. If, for any reason, the transformed bbox is not finite, a tuple of
    four Nones is returned.

    Args:
        bbox (list): List of south, north, west, and east floats in decimal
            degrees.
        to_crs (str): Target CRS.

    Returns:
        float, float, float, float: Bottom, top, left, and right in to_crs
        units or all Nones on failed transformation.
    """
    import pyproj

    s, n, w, e = bbox
    try:
        trans = pyproj.Transformer.from_crs("EPSG:4326", to_crs,
                                            always_xy=True)
        x = [w, w, e, e]
        y = [s, n, s, n]
        if s*n < 0:
            x.extend([w, e])
            y.extend([0, 0])
            inc_zero = True
        else:
            inc_zero = False
        x, y = trans.transform(x, y)
        b = min(y[0], y[2])
        t = max(y[1], y[3])
        l = min(x[0], x[1])
        r = max(x[2], x[3])
        if inc_zero:
            l = min(l, x[4])
            r = max(r, x[5])
        if math.isinf(b) or math.isinf(t) or math.isinf(l) or math.isinf(r):
            b = t = l = r = None
    except:
        b = t = l = r = None
    return b, t, l, r


def create_projpicker_db(
        overwrite=False,
        projpicker_db=None,
        proj_db=None):
    """
    Create a projpicker.db sqlite database. If projpicker_db or proj_db is None
    (default), get_projpicker_db() or get_proj_db() is used, respectively.

    Args:
        overwrite (bool): Whether or not to overwrite projpicker.db. Defaults
            to False.
        projpicker_db (str): projpicker.db path. Defaults to None.
        proj_db (str): proj.db path. Defaults to None.

    Raises:
        Exception: If projpicker_db already exists.
    """
    projpicker_db = get_projpicker_db(projpicker_db)
    proj_db = get_proj_db(proj_db)

    if os.path.isfile(projpicker_db):
        if overwrite:
            os.remove(projpicker_db)
        else:
            raise Exception(f"{projpicker_db}: File already exists")

    with sqlite3.connect(projpicker_db) as projpicker_con:
        projpicker_con.execute(bbox_schema)
        projpicker_con.commit()
        with sqlite3.connect(proj_db) as proj_con:
            proj_cur = proj_con.cursor()
            sql_tpl = """SELECT {columns}
                         FROM crs_view c
                         JOIN usage u
                            ON c.auth_name=u.object_auth_name AND
                               c.code=u.object_code
                         JOIN extent e
                            ON u.extent_auth_name=e.auth_name AND
                               u.extent_code=e.code
                         WHERE c.table_name=u.object_table_name AND
                               south_lat IS NOT NULL AND
                               north_lat IS NOT NULL AND
                               west_lon IS NOT NULL AND
                               east_lon IS NOT NULL
                         ORDER BY c.table_name,
                                  c.auth_name, c.code,
                                  u.auth_name, u.code,
                                  e.auth_name, e.code,
                                  south_lat, north_lat,
                                  west_lon, east_lon"""
            sql = sql_tpl.replace("{columns}", "count(c.table_name)")
            proj_cur.execute(sql)
            nrows = proj_cur.fetchone()[0]
            sql = sql_tpl.replace("{columns}", """c.table_name, c.name,
                                                  c.auth_name, c.code,
                                                  u.auth_name, u.code,
                                                  e.auth_name, e.code,
                                                  south_lat, north_lat,
                                                  west_lon, east_lon""")
            proj_cur.execute(sql)
            nrow = 1
            for row in proj_cur.fetchall():
                message("\b"*80+f"{nrow}/{nrows}", end="")
                (proj_table, crs_name,
                 crs_auth, crs_code,
                 usg_auth, usg_code,
                 ext_auth, ext_code,
                 s, n, w, e) = row
                bbox = s, n, w, e
                area = calc_area(bbox)
                unit = find_unit(proj_table, crs_auth, crs_code, proj_cur)
                if unit == "degree":
                    # XXX: might be incorrect!
                    b, t, l, r = s, n, w, e
                else:
                    b, t, l, r = transform_latlon_bbox(
                                                bbox, f"{crs_auth}:{crs_code}")

                sql = """INSERT INTO bbox
                         VALUES (?, ?,
                                 ?, ?, ?, ?, ?, ?,
                                 ?, ?, ?, ?,
                                 ?, ?, ?, ?,
                                 ?, ?)"""
                projpicker_con.execute(sql, (proj_table, crs_name,
                                             crs_auth, crs_code,
                                             usg_auth, usg_code,
                                             ext_auth, ext_code,
                                             s, n, w, e,
                                             b, t, l, r,
                                             unit, area))
                projpicker_con.commit()
                nrow += 1
            message()


def write_bbox_db(
        bbox,
        bbox_db,
        overwrite=False):
    """
    Write a list of BBox instances to a bbox database.

    Args:
        bbox (list): List of BBox instances.
        bbox_db (str): Path for output bbox_db.
        overwrite (bool): Whether or not to overwrite output file. Defaults to
            False.

    Raises:
        Exception: If bbox_db file already exists when overwriting is not
        requested.
    """
    if os.path.isfile(bbox_db):
        if overwrite:
            os.remove(bbox_db)
        else:
            raise Exception(f"{bbox_db}: File already exists")

    with sqlite3.connect(bbox_db) as bbox_con:
        bbox_con.execute(bbox_schema)
        bbox_con.commit()

        nrows = len(bbox)
        nrow = 1
        for row in bbox:
            message("\b"*80+f"{nrow}/{nrows}", end="")
            sql = """INSERT INTO bbox
                     VALUES (?, ?,
                             ?, ?, ?, ?, ?, ?,
                             ?, ?, ?, ?,
                             ?, ?, ?, ?,
                             ?, ?)"""
            bbox_con.execute(sql, (row.proj_table, row.crs_name,
                                   row.crs_auth_name, row.crs_code,
                                   row.usage_auth_name, row.usage_code,
                                   row.extent_auth_name, row.extent_code,
                                   row.south_lat, row.north_lat,
                                   row.west_lon, row.east_lon,
                                   row.bottom, row.top,
                                   row.left, row.right,
                                   row.unit, row.area_sqkm))
            bbox_con.commit()
            nrow += 1
        message()


def read_bbox_db(
        bbox_db,
        unit="any",
        proj_table="any"):
    """
    Return a list of all BBox instances in unit in proj_table in a bbox
    database. Each BBox instance is a named tuple with all the columns from the
    bbox table in projpicker.db. Results are sorted by area.

    Args:
        bbox_db (str): Path for the input bbox database.
        unit (str): Unit values from the input bbox database. Defaults to
            "any".
        proj_table (str): Proj table values from the input bbox database.
            Defaults to "any".

    Returns:
        list: List of all BBox instances sorted by area.
    """
    outbbox = []
    with sqlite3.connect(bbox_db) as bbox_con:
        bbox_cur = bbox_con.cursor()
        sql = f"""SELECT *
                  FROM bbox
                  WHERE_UNIT_AND_PROJ_TABLE
                  ORDER BY area_sqkm,
                           proj_table,
                           crs_auth_name, crs_code,
                           usage_auth_name, usage_code,
                           extent_auth_name, extent_code"""
        params = []
        if unit == "any" and proj_table == "any":
            sql = sql.replace("WHERE_UNIT_AND_PROJ_TABLE", "")
        elif unit == "any":
            sql = sql.replace("WHERE_UNIT_AND_PROJ_TABLE",
                              "WHERE proj_table = ?")
            params.append(proj_table)
        elif proj_table == "any":
            sql = sql.replace("WHERE_UNIT_AND_PROJ_TABLE",
                              "WHERE unit = ?")
            params.append(unit)
        else:
            sql = sql.replace("WHERE_UNIT_AND_PROJ_TABLE",
                              "WHERE unit = ? and proj_table = ?")
            params.extend([unit, proj_table])
        bbox_cur.execute(sql, params)
        for row in map(BBox._make, bbox_cur.fetchall()):
            outbbox.append(row)
    return outbbox


###############################################################################
# coordinate systems

def set_coordinate_system(coor_sys="latlon"):
    """
    Set the coordinate system to either latitude-longitude or x-y by globally
    exposing coordinate-system-specific functions from the corresponding
    module.

    Args:
        coor_sys (str): Coordinate system (latlon, xy). Defaults to "latlon".

    Raises:
        Exception: If coor_sys is not one of "latlon" or "xy".
    """
    if coor_sys not in ("latlon", "xy"):
        raise Exception(f"{coor_sys}: Invalid coordinate system")

    if coor_sys == "latlon":
        coor_mod = coor_latlon
        point_re = coor_mod.latlon_re
    else:
        coor_mod = coor_xy
        point_re = coor_mod.xy_re

    parse_point = coor_mod.parse_point
    parse_bbox = coor_mod.parse_bbox

    calc_poly_bbox = coor_mod.calc_poly_bbox

    is_point_within_bbox = coor_mod.is_point_within_bbox
    is_bbox_within_bbox = coor_mod.is_bbox_within_bbox

    query_point_using_cursor = coor_mod.query_point_using_cursor
    query_bbox_using_cursor = coor_mod.query_bbox_using_cursor

    globals().update(locals())


def set_latlon():
    """
    Set the coordinate system to latitude-longitude by calling
    set_coordinate_system().
    """
    set_coordinate_system()


def set_xy():
    """
    Set the coordinate system to x-y by calling set_coordinate_system().
    """
    set_coordinate_system("xy")


def is_latlon():
    """
    Return True if the coordinate system is latitude-longitude. Otherwise,
    return False.
    """
    return coor_mod == coor_latlon


###############################################################################
# parsing

def parse_points(points):
    """
    Parse a list of strs of latitude and longitude or x and y, and return a
    list of lists of two floats. A list of two floats can be used in place of a
    str of latitude and longitude. Any unparsable str is ignored with a
    warning. If an output from this function is passed, the same output is
    returned.

    For example,
    ["1,2", "3,4", ",", "5,6", "7,8"] or
    [[1,2], "3,4", ",", "5,6", [7,8]] returns the same
    [[1.0, 2.0], [3.0, 4.0], [5.0, 6.0], [7.0, 8.0]] with a warning about the
    unparsable comma.

    Args:
        points (list): List of parsable point geometries.

    Returns:
        list: List of lists of parsed point geometries in float.
    """
    outpoints = []
    for point in points:
        c1 = c2 = None
        typ = type(point)
        if typ == str:
            # "lat,lon" or "x,y"
            c1, c2 = parse_point(point)
        elif typ in (list, tuple):
            if len(point) == 2:
                # [ lat, lon ] or [ x, y ]
                c1, c2 = point
                c1 = get_float(c1)
                c2 = get_float(c2)
        if c1 is not None and c2 is not None:
            outpoints.append([c1, c2])
    return outpoints


parse_poly = parse_points


def parse_polys(polys):
    """
    Parse a list of strs of latitude and longitude or x and y, and return a
    list of lists of lists of two floats. A list of two floats can be used in
    place of a str of coordinates. Any unparsable str starts a new poly. If an
    output from this function is passed, the same output is returned.

    For example,
    ["1,2", "3,4", ",", "5,6", "7,8"] or
    [[1,2], "3,4", ",", "5,6", [7,8]] returns the same
    [[[1.0, 2.0], [3.0, 4.0]], [[5.0, 6.0], [7.0, 8.0]]].

    Args:
        points (list): List of parsable point geometries with an unparsable str
            as a poly separator.

    Returns:
        list: List of lists of lists of parsed point geometries in float.
    """
    outpolys = []
    poly = []

    for point in polys:
        c1 = c2 = None
        typ = type(point)
        if typ == str:
            # "lat,lon" or "x,y"
            c1, c2 = parse_point(point)
        elif typ in (list, tuple):
            if len(point) == 2:
                typ0 = type(point[0])
                typ1 = type(point[1])
            else:
                typ0 = typ1 = None
            if ((typ0 in (int, float) and typ1 in (int, float)) or
                (typ0 == str and not point_re.match(point[0]) and
                 typ1 == str and not point_re.match(point[1]))):
                # [ lat, lon ] or [ x, y ]
                c1, c2 = point
                c1 = get_float(c1)
                c2 = get_float(c2)
            else:
                # [ "lat,lon", ... ] or [ "x,y", ... ]
                # [ [ lat, lon ], ... ] or [ [ x, y ], ... ]
                p = parse_points(point)
                if p:
                    outpolys.append(p)
        if c1 is not None and c2 is not None:
            poly.append([c1, c2])
        elif poly:
            # use invalid coordinates as a flag for a new poly
            outpolys.append(poly)
            poly = []

    if poly:
        outpolys.append(poly)

    return outpolys


def parse_bboxes(bboxes):
    """
    Parse a list of strs of four floats, and return them as a list. A list of
    four floats can be used in place of a str of four floats. Any unparsable
    str is ignored. If an output from this function is passed, the same output
    is returned.

    For example, ["10,20,30,40", [50,60,70,80]] returns
    [[10.0, 20.0, 30.0, 40.0], [50.0, 60.0, 70.0, 80.0]]

    Args:
        bboxes (list): List of parsable strs of four floats.

    Returns:
        list: List of lists of four floats.
    """
    outbboxes = []
    for bbox in bboxes:
        s = n = w = e = None
        typ = type(bbox)
        if typ == str:
            s, n, w, e = parse_bbox(bbox)
        elif typ in (list, tuple):
            if len(bbox) == 4:
                s, n, w, e = bbox
                s = get_float(s)
                n = get_float(n)
                w = get_float(w)
                e = get_float(e)
        if s is not None and n is not None and w is not None and e is not None:
            outbboxes.append([s, n, w, e])
    return outbboxes


def parse_geom(geom, geom_type="point"):
    """
    Parse a geometry and return it as a list.

    Args:
        geom (list): List or a str of a parsable geometry. See parse_point(),
            parse_poly(), and parse_bbox().
        geom_type (str): Geometry type (point, poly, bbox). Defaults to
            "point".

    Returns:
        list: List of a parsed geometry.

    Raises:
        Exception: If geom_type is not one of "point", "poly", or "bbox".
    """
    if geom_type not in ("point", "poly", "bbox"):
        raise Exception(f"{geom_type}: Invalid geometry type")

    if geom_type == "point":
        geom = parse_point(geom)
    elif geom_type == "poly":
        geom = parse_poly(geom)
    else:
        geom = parse_bbox(geom)
    return geom


def parse_geoms(geoms, geom_type="point"):
    """
    Parse geometries and return them as a list.

    Args:
        geom (list): List of parsable geometries. See parse_points(),
            parse_polys(), and parse_bboxes().
        geom_type (str): Geometry type (point, poly, bbox). Defaults to
            "point".

    Returns:
        list: List of parsed geometries.

    Raises:
        Exception: If geom_type is not one of "point", "poly", or "bbox".
    """
    if geom_type not in ("point", "poly", "bbox"):
        raise Exception(f"{geom_type}: Invalid geometry type")

    if geom_type == "point":
        geoms = parse_points(geoms)
    elif geom_type == "poly":
        geoms = parse_polys(geoms)
    else:
        geoms = parse_bboxes(geoms)

    return geoms


def parse_mixed_geoms(geoms):
    """
    Parse mixed input geometries and return them as a list. The first non-empty
    element in geoms can optionally be "all", "and", "or", "xor", or "not" to
    set the query operator. The "all" query operator ignores the rest of input
    geometries and returns all bbox rows from the database. The "and" query
    operator performs the intersection of bbox rows while the "or" operator the
    union. Geometry types can be specified using words "point" (default),
    "poly", and "bbox". Words "latlon" (default) and "xy" start the
    latitude-longitude and x-y coordinate systems, respectively. This function
    ignores the current coordinate system set by set_coordinate_system(),
    set_latlon(), or set_xy(), and always starts in the latitude-longitude
    coordinate system by default.

    Args:
        geoms (list or str): List of "point", "poly", "bbox", "none", "all",
            "latlon", "xy", "and", "or", "xor", "not", "match", "unit=",
            "proj_table=", "match_tol=", "match_max=", and parsable geometries.
            The first word can be either "and", "or", "xor", or "postfix". See
            parse_points(), parse_polys(), and parse_bboxes().

    Returns:
        list: List of parsed geometries.

    Raises:
        Exception: If the geometry stack size is not 1 after postfix parsing.
    """
    def parse_next_geom(g):
        if geom_type == "poly":
            i = g
            while (i < ngeoms and geoms[i] not in keywords and
                   not (type(geoms[i]) == str and
                        (geoms[i] == "" or None in parse_point(geoms[i]) or
                        (("=" in geoms[i] and
                          geoms[i].split("=")[0] in constraints) or
                         geom_var_re.match(geoms[i]))))):
                i += 1
            geom = parse_geom(geoms[g:i], geom_type)
            g = i - 1
        else:
            geom = parse_geom(geoms[g], geom_type)
        return geom, g

    def parse_next_geoms(g):
        i = g
        while (i < ngeoms and geoms[i] not in keywords and
               not (type(geoms[i]) == str and
                    (("=" in geoms[i] and
                      geoms[i].split("=")[0] in constraints) or
                     geom_var_re.match(geoms[i])))):
            i += 1
        ogeoms = parse_geoms(geoms[g:i], geom_type)
        g = i
        return ogeoms, g

    if type(geoms) == str:
        geoms = geoms.split("\n")
        tidy_lines(geoms)

    outgeoms = []

    ngeoms = len(geoms)
    if ngeoms == 0:
        return outgeoms

    if geoms[0] in ("and", "or", "xor", "postfix"):
        query_op = geoms[0]
        first_index = 1
        outgeoms.append(query_op)
    else:
        query_op = "and"
        first_index = 0

    query_ops = ("and", "or", "xor", "not", "match")
    spec_geoms = ("none", "all")
    geom_types = ("point", "poly", "bbox")
    coor_sys = ("latlon", "xy")
    keywords = query_ops + spec_geoms + geom_types + coor_sys

    constraints = ("unit", "proj_table", "match_tol", "match_max")

    geom_type = "point"

    was_latlon = is_latlon()
    try:
        set_latlon()

        geom_vars = []
        stack_size = 0
        g = first_index

        while g < ngeoms:
            geom = geoms[g]
            typ = type(geom)
            if geom in query_ops:
                if query_op == "postfix":
                    if geom == "not" and stack_size >= 1:
                        pass
                    elif stack_size >= 2:
                        stack_size -= 1
                    else:
                        raise Exception(f"Not enough operands for {geom}")
                else:
                    raise Exception(f"{geom}: Not in postfix query")
            elif geom in geom_types:
                geom_type = geom
            elif geom in coor_sys:
                if geom == "latlon":
                    set_latlon()
                else:
                    set_xy()
            elif (typ == str and "=" in geom and
                  geom.split("=")[0] in constraints):
                pass
            elif geom in ("none", "all"):
                stack_size += 1
            else:
                m = geom_var_re.match(geom) if typ == str else None
                if m:
                    sav = m[1] is not None or m[2] is not None
                    use = m[2] is not None or m[3] is not None
                    name = m[1] or m[2] or m[3]
                    if sav:
                        if name not in geom_vars:
                            geom_vars.append(name)
                        outgeoms.append(geom)
                        g += 1
                        geom = geoms[g]
                        typ = type(geom)
                        if (typ == str and not geom.startswith(":") and
                            geom not in spec_geoms):
                            geom, g = parse_next_geom(g)
                        outgeoms.append(geom)
                    if use:
                        if name not in geom_vars:
                            raise Exception(f"{name}: Undefined geometry "
                                            "variable")
                        stack_size += 1
                        if not sav:
                            outgeoms.append(geom)
                    g += 1
                else:
                    ogeoms, g = parse_next_geoms(g)
                    if ogeoms and None not in ogeoms:
                        stack_size += len(ogeoms)
                        outgeoms.extend(ogeoms)
                continue
            if typ == str:
                outgeoms.append(geom)
            g += 1

        if query_op == "postfix":
            if stack_size == 0:
                raise Exception("Nothing to return from postfix stack")
            elif stack_size > 1:
                raise Exception(f"{stack_size}: Excessive stack size for "
                                "postfix operations")
    finally:
        if was_latlon and not is_latlon():
            set_latlon()
        elif not was_latlon and is_latlon():
            set_xy()

    return outgeoms


###############################################################################
# bbox operators

def bbox_not(bbox, bbox_all):
    """
    Return the set-theoretic complement of bbox.

    Args:
        bbox (list): List of BBox instances.
        bbox_all (list): List of BBox instances in the universe.

    Returns:
        list: List of BBox instances from bbox_all that are not in the input
        bbox.
    """
    return [b for b in bbox_all if b not in bbox]


def bbox_and(bbox1, bbox2):
    """
    Return the set-theoretic result of the AND operation on bbox1 and bbox2.

    Args:
        bbox1 (list): List of BBox instances.
        bbox2 (list): List of BBox instances.

    Returns:
        list: List of BBox instances resulting from the AND operation between
        bbox1 and bbox2.
    """
    return [b for b in bbox1 if b in bbox2]


def bbox_or(bbox1, bbox2):
    """
    Return the set-theoretic result of the OR operation on bbox1 and bbox2.

    Args:
        bbox1 (list): List of BBox instances.
        bbox2 (list): List of BBox instances.

    Returns:
        list: List of BBox instances resulting from the OR operation between
        bbox1 and bbox2.
    """
    outbbox = bbox1.copy()
    for b in bbox2:
        if b not in bbox1:
            outbbox.append(b)
    return outbbox


def bbox_xor(bbox1, bbox2):
    """
    Return the set-theoretic result of the XOR operation on bbox1 and bbox2.

    Args:
        bbox1 (list): List of BBox instances.
        bbox2 (list): List of BBox instances.

    Returns:
        list: List of BBox instances resulting from the XOR operation between
        bbox1 and bbox2.
    """
    outbbox = []
    for b in bbox1 + bbox2:
        if (b in bbox1) + (b in bbox2) == 1:
            outbbox.append(b)
    return outbbox


def bbox_binary_operator(bbox1, bbox2, bbox_op):
    """
    Return the set-theoretic result of the bbox_op binary operator on bbox1 and
    bbox2. This function invokes invidial binary operator functions.

    Args:
        bbox1 (list): List of BBox instances.
        bbox2 (list): List of BBox instances.
        bbox_op (str): Binary operator (and, or, xor).

    Returns:
        list: List of BBox instances resulting from the op binary operation
        between bbox1 and bbox2.

    Raises:
        Exception: If bbox_op is not one of "and", "or", or "xor".
    """
    if bbox_op not in ("and", "or", "xor"):
        raise Exception(f"{bbox_op}: Invalid bbox operator")

    if bbox_op == "and":
        outbbox = bbox_and(bbox1, bbox2)
    elif bbox_op == "or":
        outbbox = bbox_or(bbox1, bbox2)
    else:
        outbbox = bbox_xor(bbox1, bbox2)

    return outbbox


def sort_bbox(bbox):
    """
    Sort a list of BBox instances by area_sqkm in place after deduplicating
    data.

    Args:
        bbox (list): List of BBox instances.
    """
    bbox.sort(key=lambda x: x.crs_auth_name+":"+x.crs_code)
    for i in reversed(range(len(bbox))):
        if i > 0 and bbox[i] == bbox[i-1]:
            del bbox[i]
    bbox.sort(key=lambda x: (x.area_sqkm,
                             x.proj_table,
                             x.crs_auth_name, x.crs_code,
                             x.usage_auth_name, x.usage_code,
                             x.extent_auth_name, x.extent_code))


###############################################################################
# geometry operators

def match_geoms(gbbox1, gbbox2, match_max=0, match_tol=1):
    """
    Match two geometries in different coordinate systems within a given
    distance match_tol in xy and return a subset list of the BBox instances of
    the xy geometry that can be transformed to the other geometry in latlon.
    Only the first match_max number of matches are returned. If match_max is 0,
    all matches are returned. This operation is useful when the coordinates of
    a geometry in both latlon and xy are known. This operation requires the
    pyproj module and is very slow.

    Args:
        gbbox1 (GeomBBox): Geometry-bbox 1.
        gbbox2 (GeomBBox): Geometry-bbox 2.
        match_max (int): Maximum number of matches to return. 0 to return all.
        match_tol (float): Positive distance tolerance.

    Returns:
        list: List of matched BBox instances from the BBox instances of the xy
        geometry.

    Raises:
        Exception: If matching cannot be done for any reason.
    """
    def find_matching_bbox(geom_latlon, geom, bbox):
        obbox = []
        if len(geom) == 2:
            x1, y1 = geom
        else:
            b1, t1, l1, r1 = geom
        nrows = len(bbox)
        nrow = 1
        for b in bbox:
            message("\b"*80+f"Matching... {nrow}/{nrows}", end="")
            crs = f"{b.crs_auth_name}:{b.crs_code}"
            if len(geom) == 2:
                x2, y2 = transform_latlon_point(geom_latlon, crs)
                dist = math.sqrt((x1-x2)**2+(y1-y2)**2)
                if dist <= match_tol:
                    obbox.append(b)
            else:
                b2, t2, l2, r2 = transform_latlon_bbox(geom_latlon, crs)
                dist1 = math.sqrt((b1-b2)**2+(l1-l2)**2)
                dist2 = math.sqrt((b1-b2)**2+(r1-r2)**2)
                dist3 = math.sqrt((t1-t2)**2+(l1-l2)**2)
                dist4 = math.sqrt((t1-t2)**2+(r1-r2)**2)
                if (dist1 <= match_tol and dist2 <= match_tol and
                    dist3 <= match_tol and dist4 <= match_tol):
                    obbox.append(b)
            if len(obbox) >= match_max > 0:
                break
            nrow += 1
        message()
        return obbox

    if None in (gbbox1.type, gbbox2.type):
        raise Exception("Non-raw geometries cannot be matched")

    if gbbox1.type != gbbox2.type:
        raise Exception("Geometries in different types cannot be matched")

    if gbbox1.is_latlon == gbbox2.is_latlon:
        raise Exception("Geometries in the same coordinate system cannot be "
                        "matched")

    geom1 = gbbox1.geom
    geom2 = gbbox2.geom

    if len(geom1) != len(geom2):
        raise Exception("Geometries in different lengths cannot be matched")

    outbbox = gbbox1.bbox if gbbox2.is_latlon else gbbox2.bbox

    if gbbox1.type == "poly":
        for i in range(len(geom1)):
            g1 = geom1[i]
            g2 = geom2[i]
            if gbbox1.is_latlon:
                outbbox = find_matching_bbox(g1, g2, outbbox)
            else:
                outbbox = find_matching_bbox(g2, g1, outbbox)
            if len(outbbox) >= match_max > 0:
                break
    else:
        if gbbox1.is_latlon:
            outbbox = find_matching_bbox(geom1, geom2, outbbox)
        else:
            outbbox = find_matching_bbox(geom2, geom1, outbbox)

    return outbbox


###############################################################################
# queries

def query_point(
        point,
        unit="any",
        proj_table="any",
        projpicker_db=None):
    """
    Return a list of BBox instances in unit in proj_table that completely
    contain an input point geometry. Each BBox instance is a named tuple with
    all the columns from the bbox table in projpicker.db. Results are sorted by
    area from the smallest to largest. If projpicker_db is None (default),
    get_projpicker_db() is used.

    Args:
        point (list or str): List of two floats or a parsable point geometry.
            See parse_point().
        unit (str): Unit values from projpicker.db. Defaults to "any".
        proj_table (str): Proj table values from projpicker.db. Defaults to
            "any".
        projpicker_db (str): projpicker.db path. Defaults to None.

    Returns:
        list: List of queried BBox instances sorted by area.
    """
    projpicker_db = get_projpicker_db(projpicker_db)

    with sqlite3.connect(projpicker_db) as projpicker_con:
        projpicker_cur = projpicker_con.cursor()
        outbbox = query_point_using_cursor(projpicker_cur, point, unit,
                                           proj_table)
    return outbbox


def query_point_using_bbox(
        prevbbox,
        point,
        unit="any",
        proj_table="any"):
    """
    Return a subset list of input BBox instances in unit in proj_table that
    completely contain an input point geometry. Each BBox instance is a named
    tuple with all the columns from the bbox table in projpicker.db. This
    function is used to perform an intersection operation on BBox instances
    consecutively.

    Args:
        prevbbox (list): List of BBox instances from a previous query.
        point (list or str): List of two floats or a parsable str of a point.
            See parse_point().
        unit (str): Unit values from projpicker.db. Defaults to "any".
        proj_table (str): Proj table values from projpicker.db. Defaults to
            "any".

    Returns:
        list: List of queried BBox instances sorted by area.
    """
    point = parse_point(point)

    idx = []
    for i in range(len(prevbbox)):
        if is_point_within_bbox(point, prevbbox[i]) and (
            unit == "any" or prevbbox[i].unit == unit) and (
            proj_table == "any" or prevbbox[i].proj_table == proj_table):
            idx.append(i)
    outbbox = [prevbbox[i] for i in idx]
    return outbbox


def query_points(
        points,
        query_op="and",
        unit="any",
        proj_table="any",
        projpicker_db=None):
    """
    Return a list of BBox instances in unit in proj_table that completely
    contain input point geometries. Each BBox instance is a named tuple with
    all the columns from the bbox table in projpicker.db. The "and" query
    operator performs the intersection of bbox rows while the "or" operator the
    union and the "xor" operator the exclusive OR. Results are sorted by area
    from the smallest to largest. If projpicker_db is None (default),
    get_projpicker_db() is used.

    Args:
        points (list): List of parsable point geometries. See parse_points().
        query_op (str): Query operator (and, or, xor). Defaults to "and".
        unit (str): Unit values from projpicker.db. Defaults to "any".
        proj_table (str): Proj table values from projpicker.db. Defaults to
            "any".
        projpicker_db (str): projpicker.db path. Defaults to None.

    Returns:
        list: List of queried BBox instances sorted by area.

    Raises:
        Exception: If query_op is not one of "and", "or", or "xor".
    """
    if query_op not in ("and", "or", "xor"):
        raise Exception(f"{query_op}: Invalid query operator")

    points = parse_points(points)
    projpicker_db = get_projpicker_db(projpicker_db)

    outbbox = []

    first = True
    sort = False

    with sqlite3.connect(projpicker_db) as projpicker_con:
        projpicker_cur = projpicker_con.cursor()
        for point in points:
            if query_op in ("or", "xor") or first:
                obbox = query_point_using_cursor(projpicker_cur, point, unit,
                                                 proj_table)
                if obbox:
                    n = len(outbbox)
                    if query_op in ("or", "xor") and not sort and n > 0:
                        sort = True
                    if query_op == "xor" and n > 0:
                        idx = []
                        for i in range(n):
                            if outbbox[i] in obbox:
                                idx.append(i)
                        for b in obbox:
                            if b not in outbbox:
                                outbbox.append(b)
                        for i in reversed(idx):
                            del outbbox[i]
                    else:
                        outbbox.extend(obbox)
                first = False
            else:
                outbbox = query_point_using_bbox(outbbox, point, unit,
                                                 proj_table)

    if sort:
        sort_bbox(outbbox)

    return outbbox


def query_points_using_bbox(
        prevbbox,
        points,
        query_op="and",
        unit="any",
        proj_table="any"):
    """
    Return a subset list of input BBox instances in unit in proj_table that
    completely contain input point geometres. Each BBox instance is a named
    tuple with all the columns from the bbox table in projpicker.db. This
    function is used to perform an intersection operation on BBox instances
    consecutively.

    Args:
        prevbbox (list): List of BBox instances from a previous query.
        points (list): List of parsable point geometries. See parse_points().
        query_op (str): Query operator (and, or, xor). Defaults to "and".
        unit (str): Unit values from projpicker.db. Defaults to "any".
        proj_table (str): Proj table values from projpicker.db. Defaults to
            "any".

    Returns:
        list: List of queried BBox instances sorted by area.

    Raises:
        Exception: If query_op is not one of "and", "or", or "xor".
    """
    if query_op not in ("and", "or", "xor"):
        raise Exception(f"{query_op}: Invalid query operator")

    points = parse_points(points)

    idx = []

    for point in points:
        for i in range(len(prevbbox)):
            if is_point_within_bbox(point, prevbbox[i]) and (
                unit == "any" or prevbbox[i].unit == unit) and (
                proj_table == "any" or prevbbox[i].proj_table == proj_table):
                if query_op != "xor" or i not in idx:
                    idx.append(i)
        if query_op == "and":
            prevbbox = [prevbbox[i] for i in idx]
            idx.clear()
    if query_op != "and":
        prevbbox = sort_bbox([prevbbox[i] for i in idx])

    return prevbbox


def query_poly(
        poly,
        unit="any",
        proj_table="any",
        projpicker_db=None):
    """
    Return a list of BBox instances in unit in proj_table that completely
    contain an input poly geometry. Each BBox instance is a named tuple with
    all the columns from the bbox table in projpicker.db. Results are sorted by
    area from the smallest to largest. If projpicker_db is None (default),
    get_projpicker_db() is used.

    Args:
        poly (list): List of parsable point geometries. See parse_poly().
        unit (str): Unit values from projpicker.db. Defaults to "any".
        proj_table (str): Proj table values from projpicker.db. Defaults to
            "any".
        projpicker_db (str): projpicker.db path. Defaults to None.

    Returns:
        list: List of queried BBox instances sorted by area.
    """
    return query_polys([poly], "and", unit, proj_table, projpicker_db)


def query_poly_using_bbox(
        prevbbox,
        poly,
        unit="any",
        proj_table="any"):
    """
    Return a subset list of input BBox instances in unit in proj_table that
    completely contain an input poly geometres. Each BBox instance is a named
    tuple with all the columns from the bbox table in projpicker.db. This
    function is used to perform an intersection operation on BBox instances
    consecutively.

    Args:
        prevbbox (list): List of BBox instances from a previous query.
        poly (list): List of parsable point geometries. See parse_poly().
        unit (str): Unit values from projpicker.db. Defaults to "any".
        proj_table (str): Proj table values from projpicker.db. Defaults to
            "any".

    Returns:
        list: List of queried BBox instances sorted by area.
    """
    return query_polys_using_bbox(prevbbox, [poly], "and", unit, proj_table)


def query_polys(
        polys,
        query_op="and",
        unit="any",
        proj_table="any",
        projpicker_db=None):
    """
    Return a list of BBox instances in unit in proj_table that completely
    contain input poly geometries. Each BBox instance is a named tuple with all
    the columns from the bbox table in projpicker.db. The "and" query operator
    performs the intersection of bbox rows while the "or" operator the union
    and the "xor" operator the exclusive OR. Results are sorted by area from
    the smallest to largest. If projpicker_db is None (default),
    get_projpicker_db() is used.

    Args:
        polys (list): List of parsable poly geometries. See parse_polys().
        query_op (str): Query operator (and, or, xor). Defaults to "and".
        unit (str): Unit values from projpicker.db. Defaults to "any".
        proj_table (str): Proj table values from projpicker.db. Defaults to
            "any".
        projpicker_db (str): projpicker.db path. Defaults to None.

    Returns:
        list: List of queried BBox instances sorted by area.
    """
    polys = parse_polys(polys)

    bboxes = [calc_poly_bbox(poly) for poly in polys]
    return query_bboxes(bboxes, query_op, unit, proj_table, projpicker_db)


def query_polys_using_bbox(
        prevbbox,
        polys,
        query_op="and",
        unit="any",
        proj_table="any"):
    """
    Return a subset list of input BBox instances in unit in proj_table that
    completely contain input poly geometres. Each BBox instance is a named
    tuple with all the columns from the bbox table in projpicker.db. This
    function is used to perform an intersection operation on BBox instances
    consecutively.

    Args:
        prevbbox (list): List of BBox instances from a previous query.
        polys (list): List of parsable poly geometries. See parse_polys().
        query_op (str): Query operator (and, or, xor). Defaults to "and".
        unit (str): Unit values from projpicker.db. Defaults to "any".
        proj_table (str): Proj table values from projpicker.db. Defaults to
            "any".

    Returns:
        list: List of queried BBox instances sorted by area.
    """
    polys = parse_polys(polys)

    bboxes = [calc_poly_bbox(poly) for poly in polys]
    return query_bboxes_using_bbox(prevbbox, bboxes, query_op, unit,
                                   proj_table)


def query_bbox(
        bbox,
        unit="any",
        proj_table="any",
        projpicker_db=None):
    """
    Return a list of BBox instances in unit in proj_table that completely
    contain an input bbox geometry. Each BBox instance is a named tuple with
    all the columns from the bbox table in projpicker.db. Results are sorted by
    area from the smallest to largest. If projpicker_db is None (default),
    get_projpicker_db() is used.

    Args:
        bbox (list or str): List of four floats or a parsable str of a bbox
            geometry. See parse_bbox().
        unit (str): Unit values from projpicker.db. Defaults to "any".
        proj_table (str): Proj table values from projpicker.db. Defaults to
            "any".
        projpicker_db (str): projpicker.db path. Defaults to None.

    Returns:
        list: List of queried BBox instances sorted by area.
    """
    projpicker_db = get_projpicker_db()

    with sqlite3.connect(projpicker_db) as projpicker_con:
        projpicker_cur = projpicker_con.cursor()
        outbbox = query_bbox_using_cursor(projpicker_cur, bbox, unit,
                                          proj_table)
    return outbbox


def query_bbox_using_bbox(
        prevbbox,
        bbox,
        unit="any",
        proj_table="any"):
    """
    Return a subset list of input BBox instances in unit in proj_table that
    completely contain an input bbox geometry defined by sout, north, west, and
    east. Each BBox instance is a named tuple with all the columns from the
    bbox table in projpicker.db. This function is used to perform an
    intersection operation on bbox rows consecutively.

    Args:
        prevbbox (list): List of BBox instances from a previous query.
        bbox (list or str): List of south, north, west, and east floats in
            decimal degrees or a parsable str of south, north, west, and east.
            See parse_bbox().
        unit (str): Unit values from projpicker.db. Defaults to "any".
        proj_table (str): Proj table values from projpicker.db. Defaults to
            "any".

    Returns:
        list: List of queried BBox instances sorted by area.
    """
    bbox = parse_bbox(bbox)

    idx = []

    for i in range(len(prevbbox)):
        if is_bbox_within_bbox(bbox, prevbbox[i]) and (
            unit == "any" or prevbbox[i].unit == unit) and (
            proj_table == "any" or prevbbox[i].proj_table == proj_table):
            idx.append(i)
    return [prevbbox[i] for i in idx]


def query_bboxes(
        bboxes,
        query_op="and",
        unit="any",
        proj_table="any",
        projpicker_db=None):
    """
    Return a list of BBox instances in unit in proj_table that completely
    contain input bbox geometries. Each BBox instance is a named tuple with all
    the columns from the bbox table in projpicker.db. The "and" query operator
    performs the intersection of bbox rows while the "or" operator the union
    and the "xor" operator the exclusive OR. Results are sorted by area from
    the smallest to largest. If projpicker_db is None (default),
    get_projpicker_db() is used.

    Args:
        bboxes (list): List of parsable bbox geometries. See parse_bboxes().
        query_op (str): Query operator (and, or, xor). Defaults to "and".
        unit (str): Unit values from projpicker.db. Defaults to "any".
        proj_table (str): Proj table values from projpicker.db. Defaults to
            "any".
        projpicker_db (str): projpicker.db path. Defaults to None.

    Returns:
        list: List of queried BBox instances sorted by area.

    Raises:
        Exception: If query_op is not one of "and", "or", or "xor".
    """
    if query_op not in ("and", "or", "xor"):
        raise Exception(f"{query_op}: Invalid query operator")

    bboxes = parse_bboxes(bboxes)
    projpicker_db = get_projpicker_db()

    outbbox = []

    first = True
    sort = False

    with sqlite3.connect(projpicker_db) as projpicker_con:
        projpicker_cur = projpicker_con.cursor()
        for bbox in bboxes:
            if query_op in ("or", "xor") or first:
                obbox = query_bbox_using_cursor(projpicker_cur, bbox, unit,
                                                proj_table)
                if obbox:
                    n = len(outbbox)
                    if query_op in ("or", "xor") and not sort and n > 0:
                        sort = True
                    if query_op == "xor" and n > 0:
                        idx = []
                        for i in range(n):
                            if outbbox[i] in obbox:
                                idx.append(i)
                        for b in obbox:
                            if b not in outbbox:
                                outbbox.append(b)
                        for i in reversed(idx):
                            del outbbox[i]
                    else:
                        outbbox.extend(obbox)
                first = False
            else:
                outbbox = query_bbox_using_bbox(outbbox, bbox, unit,
                                                proj_table)

    if sort:
        sort_bbox(outbbox)

    return outbbox


def query_bboxes_using_bbox(
        prevbbox,
        bboxes,
        query_op="and",
        unit="any",
        proj_table="any"):
    """
    Return a subset list of input BBox instances in unit in proj_table that
    completely contain input bbox geometres. Each BBox instance is a named
    tuple with all the columns from the bbox table in projpicker.db. This
    function is used to perform an intersection operation on bbox rows
    consecutively.

    Args:
        prevbbox (list): List of BBox instances from a previous query.
        bboxes (list): List of parsable bbox geometries. See parse_bboxes().
        query_op (str): Query operator (and, or, xor). Defaults to "and".
        unit (str): Unit values from projpicker.db. Defaults to "any".
        proj_table (str): Proj table values from projpicker.db. Defaults to
            "any".

    Returns:
        list: List of queried BBox instances sorted by area.

    Raises:
        Exception: If query_op is not one of "and", "or", "xor".
    """
    if query_op not in ("and", "or", "xor"):
        raise Exception(f"{query_op}: Invalid query operator")

    bboxes = parse_bboxes(bboxes)

    idx = []

    for bbox in bboxes:
        for i in range(len(prevbbox)):
            if is_bbox_within_bbox(bbox, prevbbox[i]) and (
                unit == "any" or prevbbox[i].unit == unit) and (
                proj_table == "any" or prevbbox[i].proj_table == proj_table):
                if query_op != "xor" or i not in idx:
                    idx.append(i)
        if query_op == "and":
            prevbbox = [prevbbox[i] for i in idx]
            idx.clear()
    if query_op != "and":
        prevbbox = sort_bbox([prevbbox[i] for i in idx])

    return prevbbox


def query_geom(
        geom,
        geom_type="point",
        unit="any",
        proj_table="any",
        projpicker_db=None):
    """
    Return a list of BBox instances in unit in proj_table that completely
    contain an input geometry. Each BBox instance is a named tuple with all the
    columns from the bbox table in projpicker.db. Results are sorted by area
    from the smallest to largest. If projpicker_db is None (default),
    get_projpicker_db() is used.

    Args:
        geom (list or str): List or str of a parsable geometry. See
            parse_points(), parse_polys(), and parse_bboxes().
        geom_type (str): Geometry type (point, poly, bbox). Defaults to
            "point".
        unit (str): Unit values from projpicker.db. Defaults to "any".
        proj_table (str): Proj table values from projpicker.db. Defaults to
            "any".
        projpicker_db (str): projpicker.db path. Defaults to None.

    Returns:
        list: List of queried BBox instances sorted by area.

    Raises:
        Exception: If geom_type is not one of "point", "poly", or "bbox".
    """
    if geom_type not in ("point", "poly", "bbox"):
        raise Exception(f"{geom_type}: Invalid geometry type")

    if geom_type == "point":
        outbbox = query_point(geom, unit, proj_table, projpicker_db)
    elif geom_type == "poly":
        outbbox = query_poly(geom, unit, proj_table, projpicker_db)
    else:
        outbbox = query_bbox(geom, unit, proj_table, projpicker_db)
    return outbbox


def query_geom_using_bbox(
        prevbbox,
        geom,
        geom_type="point",
        unit="any",
        proj_table="any"):
    """
    Return a subset list of input BBox instances in unit in proj_table that
    completely contain an input geometry. Each BBox instance is a named tuple
    with all the columns from the bbox table in projpicker.db. This function is
    used to perform an intersection operation on bbox rows consecutively.

    Args:
        prevbbox (list): List of BBox instances from a previous query.
        geom (list or str): List or str of a parsable geometry. See
            parse_points(), parse_polys(), and parse_bboxes().
        geom_type (str): Geometry type (point, poly, bbox). Defaults to
            "point".
        unit (str): Unit values from projpicker.db. Defaults to "any".
        proj_table (str): Proj table values from projpicker.db. Defaults to
            "any".

    Returns:
        list: List of queried BBox instances sorted by area.

    Raises:
        Exception: If geom_type is not one of "point", "poly", or "bbox".
    """
    if geom_type not in ("point", "poly", "bbox"):
        raise Exception(f"{geom_type}: Invalid geometry type")

    if geom_type == "point":
        outbbox = query_point_using_bbox(prevbbox, geom, unit, proj_table)
    elif geom_type == "poly":
        outbbox = query_poly_using_bbox(prevbbox, geom, unit, proj_table)
    else:
        outbbox = query_bbox_using_bbox(prevbbox, geom, unit, proj_table)

    return outbbox


def query_geoms(
        geoms,
        geom_type="point",
        query_op="and",
        unit="any",
        proj_table="any",
        projpicker_db=None):
    """
    Return a list of BBox instances in unit in proj_table that completely
    contain input geometries. Each BBox instance is a named tuple with all the
    columns from the bbox table in projpicker.db. The "and" query operator
    performs the intersection of bbox rows while the "or" operator the union
    and the "xor" operator the exclusive OR. Results are sorted by area from
    the smallest to largest. If projpicker_db is None (default),
    get_projpicker_db() is used.

    Args:
        geoms (list): List of parsable geometries. See parse_points(),
            parse_polys(), and parse_bboxes().
        geom_type (str): Geometry type (point, poly, bbox). Defaults to
            "point".
        query_op (str): Query operator (and, or, xor). Defaults to "and".
        unit (str): Unit values from projpicker.db. Defaults to "any".
        proj_table (str): Proj table values from projpicker.db. Defaults to
            "any".
        projpicker_db (str): projpicker.db path. Defaults to None.

    Returns:
        list: List of queried BBox instances sorted by area.

    Raises:
        Exception: If geom_type is not one of "point", "poly", or "bbox", or
            query_op is not one of "and", "or", or "xor".
    """
    if geom_type not in ("point", "poly", "bbox"):
        raise Exception(f"{geom_type}: Invalid geometry type")

    if query_op not in ("and", "or", "xor"):
        raise Exception(f"{query_op}: Invalid query operator")

    if geom_type == "point":
        outbbox = query_points(geoms, query_op, unit, proj_table,
                               projpicker_db)
    elif geom_type == "poly":
        outbbox = query_polys(geoms, query_op, unit, proj_table, projpicker_db)
    else:
        outbbox = query_bboxes(geoms, query_op, unit, proj_table,
                               projpicker_db)
    return outbbox


def query_geoms_using_bbox(
        prevbbox,
        geoms,
        geom_type="point",
        query_op="and",
        unit="any",
        proj_table="any"):
    """
    Return a subset list of input BBox instances in unit in proj_table that
    completely contain input geometries. Each BBox instance is a named tuple
    with all the columns from the bbox table in projpicker.db. This function is
    used to perform an intersection operation on bbox rows consecutively.

    Args:
        prevbbox (list): List of BBox instances from a previous query.
        geoms (list): List of parsable geometries. See parse_points(),
            parse_polys(), and parse_bboxes().
        geom_type (str): Geometry type (point, poly, bbox). Defaults to
            "point".
        query_op (str): Query operator (and, or, xor). Defaults to "and".
        unit (str): Unit values from projpicker.db. Defaults to "any".
        proj_table (str): Proj table values from projpicker.db. Defaults to
            "any".

    Returns:
        list: List of queried BBox instances sorted by area.

    Raises:
        Exception: If geom_type is not one of "point", "poly", or "bbox".
    """
    if geom_type not in ("point", "poly", "bbox"):
        raise Exception(f"{geom_type}: Invalid geometry type")

    if geom_type == "point":
        outbbox = query_points_using_bbox(prevbbox, geom, query_op, unit,
                                          proj_table)
    elif geom_type == "poly":
        outbbox = query_polys_using_bbox(prevbbox, geom, query_op, unit,
                                         proj_table)
    else:
        outbbox = query_bboxes_using_bbox(prevbbox, geom, query_op, unit,
                                          proj_table)
    return outbbox


def query_all(
        unit="any",
        proj_table="any",
        projpicker_db=None):
    """
    Return a list of all BBox instances in unit in proj_table. Each BBox
    instance is a named tuple with all the columns from the bbox table in
    projpicker.db. Results are sorted by area. If projpicker_db is None
    (default), get_projpicker_db() is used.

    Args:
        unit (str): Unit values from projpicker.db. Defaults to "any".
        proj_table (str): Proj table values from projpicker.db. Defaults to
            "any".
        projpicker_db (str): projpicker.db path. Defaults to None.

    Returns:
        list: List of all BBox instances sorted by area.
    """
    projpicker_db = get_projpicker_db(projpicker_db)
    return read_bbox_db(projpicker_db, unit, proj_table)


def query_all_using_bbox(
        prevbbox,
        unit="any",
        proj_table="any"):
    """
    Return a subset list of input BBox instances in unit in proj_table. Each
    BBox instance is a named tuple with all the columns from the bbox table in
    projpicker.db. This function is used to perform an intersection operation
    on BBox instances consecutively.

    Args:
        prevbbox (list): List of BBox instances from a previous query.
        unit (str): Unit values from projpicker.db. Defaults to "any".
        proj_table (str): Proj table values from projpicker.db. Defaults to
            "any".

    Returns:
        list: List of queried BBox instances sorted by area.
    """
    idx = []
    for i in range(len(prevbbox)):
        if (unit == "any" or prevbbox[i].unit == unit) and (
            proj_table == "any" or prevbbox[i].proj_table == proj_table):
            idx.append(i)
    outbbox = [prevbbox[i] for i in idx]
    return outbbox


def query_mixed_geoms(
        geoms,
        projpicker_db=None):
    """
    Return a list of BBox instances that completely contain mixed input
    geometries. Each BBox instance is a named tuple with all the columns from
    the bbox table in projpicker.db. The first non-empty element in geoms can
    optionally be "all", "and" (default), "or", "xor", or "postfix" to set the
    query operator. The "all" query operator ignores the rest of input
    geometries and returns all bbox rows from the database. The "and" query
    operator performs the intersection of bbox rows while the "or" operator the
    union and the "xor" operator the exclusive OR. The "postfix" operator
    supports the "and", "or", "xor", and "not" operators in a postfix
    arithmetic manner. Geometry types can be specified using words "point"
    (default), "poly", and "bbox". Words "latlon" (default) and "xy" start the
    latitude-longitude and x-y coordinate systems, respectively. This function
    ignores the current coordinate system set by set_coordinate_system(),
    set_latlon(), or set_xy(), and always starts in the latitude-longitude
    coordinate system by default. Results are sorted by area from the smallest
    to largest. If projpicker_db is None (default), get_projpicker_db() is
    used.

    Args:
        geoms (list or str): List of "point", "poly", "bbox", "none", "all",
            "latlon", "xy", "and", "or", "xor", "not", "match", "unit=",
            "proj_table=", "match_tole=", "match_max=", and parsable
            geometries. The first word can be either "and", "or", "xor", or
            "postfix". See parse_points(), parse_polys(), and parse_bboxes().
        projpicker_db (str): projpicker.db path. Defaults to None.

    Returns:
        list: List of queried BBox instances sorted by area.

    Raises:
        Exception: If postfix operations failed.
    """
    geoms = parse_mixed_geoms(geoms)

    outbbox = []

    ngeoms = len(geoms)
    if ngeoms == 0:
        return outbbox

    if geoms[0] in ("and", "or", "xor", "postfix"):
        query_op = geoms[0]
        first_index = 1
    else:
        query_op = "and"
        first_index = 0

    if query_op == "postfix":
        geombbox_stack = []

    geom_type = "point"

    was_latlon = is_latlon()
    try:
        set_latlon()

        first = True
        sort = False
        unit = "any"
        proj_table = "any"
        match_tol = 1
        match_max = 0
        bbox_all = {}
        geom_vars = {}
        sav_is_latlon = sav_geom_type = None

        g = first_index
        while g < ngeoms:
            geom = geoms[g]
            typ = type(geom)

            m = geom_var_re.match(geom) if typ == str else None
            if m:
                sav = m[1] is not None or m[2] is not None
                use = m[2] is not None or m[3] is not None
                name = m[1] or m[2] or m[3]
                if sav:
                    g += 1
                    geom_vars[name] = Geom(is_latlon(), geom_type, geoms[g])
                if use:
                    if name not in geom_vars:
                        raise Exception(f"{name}: Undefined geometry variable")
                    nam = name
                    while True:
                        if nam not in geom_vars:
                            raise Exception(f"{nam}: Undefined geometry "
                                            "variable")
                        geom = geom_vars[nam]
                        typ = type(geom.geom)
                        if not (typ == str and geom.geom.startswith(":")):
                            break
                        nam = geom.geom[1:]
                        if nam == name:
                            raise Exception(f"{name}: Recursive geometry "
                                            "variable")
                    if geom.is_latlon != is_latlon():
                        sav_is_latlon = is_latlon()
                        if geom.is_latlon:
                            set_latlon()
                        else:
                            set_xy()
                    else:
                        sav_is_latlon = None
                    if geom.type != geom_type:
                        sav_geom_type = geom_type
                        geom_type = geom.type
                    else:
                        save_geom_type = None
                    geom = geom.geom
                else:
                    g += 1
                    continue

            if typ == str and geom.startswith("unit="):
                unit = geom.split("=")[1]
            elif typ == str and geom.startswith("proj_table="):
                proj_table = geom.split("=")[1]
            elif typ == str and geom.startswith("match_tol="):
                match_tol = float(geom.split("=")[1])
            elif typ == str and geom.startswith("match_max="):
                match_max = int(geom.split("=")[1])
            elif geom in ("point", "poly", "bbox"):
                geom_type = geom
            elif geom == "latlon":
                set_latlon()
            elif geom == "xy":
                set_xy()
            elif query_op == "postfix":
                all_key = unit + proj_table
                n = len(geombbox_stack)
                if geom == "not" and n >= 1:
                    if all_key not in bbox_all:
                        bbox_all[all_key] = query_all(unit, proj_table,
                                                      projpicker_db)
                    gbbox = geombbox_stack.pop()
                    obbox = bbox_not(gbbox.bbox, bbox_all[all_key])
                    geombbox_stack.append(GeomBBox(is_latlon(), None, geom,
                                                   obbox))
                elif geom in ("and", "or", "xor", "match") and n >= 2:
                    gbbox2 = geombbox_stack.pop()
                    gbbox1 = geombbox_stack.pop()
                    if geom == "match":
                        if None in (gbbox1.type, gbbox2.type):
                            raise Exception("Non-raw geometries cannot be "
                                            "matched")
                        obbox = bbox_binary_operator(gbbox1.bbox, gbbox2.bbox,
                                                     "and")
                        gbbox1 = GeomBBox(gbbox1.is_latlon, gbbox1.type,
                                          gbbox1.geom, obbox)
                        gbbox2 = GeomBBox(gbbox2.is_latlon, gbbox2.type,
                                          gbbox2.geom, obbox)
                        obbox = match_geoms(gbbox1, gbbox2, match_max,
                                            match_tol)
                    else:
                        obbox = bbox_binary_operator(gbbox1.bbox, gbbox2.bbox,
                                                     geom)
                    geombbox_stack.append(GeomBBox(is_latlon(), None, geom,
                                                   obbox))
                elif geom in ("and", "or", "xor", "not", "match"):
                    raise Exception(f"Not enough operands for {geom}")
                else:
                    if geom == "none":
                        obbox = []
                    elif geom == "all":
                        if all_key not in bbox_all:
                            bbox_all[all_key] = query_all(unit, proj_table,
                                                          projpicker_db)
                        obbox = bbox_all[all_key]
                    else:
                        obbox = query_geom(geom, geom_type, unit, proj_table,
                                           projpicker_db)
                    geombbox_stack.append(GeomBBox(is_latlon(), geom_type,
                                                   geom, obbox))
                if geom in ("or", "xor", "not") and not sort:
                    sort = True
            elif geom in ("and", "or", "xor", "not"):
                raise Exception(f"{geom}: Not in postfix query")
            elif query_op in ("or", "xor") or first:
                all_key = unit + proj_table
                if geom == "none":
                    obbox = []
                elif geom == "all":
                    if all_key not in bbox_all:
                        bbox_all[all_key] = query_all(unit, proj_table,
                                                      projpicker_db)
                    obbox = bbox_all[all_key]
                else:
                    obbox = query_geom(geom, geom_type, unit, proj_table,
                                       projpicker_db)
                if obbox:
                    n = len(outbbox)
                    if query_op in ("or", "xor") and not sort and n > 0:
                        sort = True
                    if query_op == "xor" and n > 0:
                        idx = []
                        for i in range(len(outbbox)):
                            if outbbox[i] in obbox:
                                idx.append(i)
                        for b in obbox:
                            if b not in outbbox:
                                outbbox.append(b)
                        for i in reversed(idx):
                            del outbbox[i]
                    else:
                        for b in obbox:
                            if b not in outbbox:
                                outbbox.append(b)
                first = False
            elif geom == "none":
                outbbox.clear()
            elif geom == "all":
                all_key = unit + proj_table
                if all_key not in bbox_all:
                    bbox_all[all_key] = query_all(unit, proj_table,
                                                  projpicker_db)
                outbbox = bbox_all[all_key]
            else:
                outbbox = query_geom_using_bbox(outbbox, geom, geom_type, unit,
                                                proj_table)

            if sav_is_latlon is not None:
                if sav_is_latlon:
                    set_latlon()
                else:
                    set_xy()
                sav_is_latlon = None

            if sav_geom_type is not None:
                geom_type = sav_geom_type
                sav_geom_type = None

            g += 1
    finally:
        if was_latlon and not is_latlon():
            set_latlon()
        elif not was_latlon and is_latlon():
            set_xy()

    if query_op == "postfix":
        if len(geombbox_stack) > 1:
            raise Exception("Postfix operations failed")
        outbbox = geombbox_stack[0].bbox

    if sort:
        sort_bbox(outbbox)

    return outbbox


###############################################################################
# search

def search_bbox(bbox, text, ignore_case=True, search_op="and"):
    """
    Search a list of BBox instances for text in any string fields. Text can be
    a CRS ID in crs_auth_name:crs_code.

    Args:
        bbox (list): List of BBox instances.
        text (list or str): List of strings or a string to search for.
        ignore_case (bool): Whether or not to ignore case. Defaults to True.
        search_op (str): Search operator (and, or). Defaults to "and".

    Returns:
        list: List of BBox instances any of whose string field values contain
        text.
    """
    def compare_crs_id(crs_id, b):
        auth, code = crs_id.split(":")
        b_auth, b_code = b.crs_auth_name, b.crs_code
        if ignore_case:
            auth = auth.lower()
            code = code.lower()
            b_auth = b_auth.lower()
            b_code = b_code.lower()
        return b_auth.endswith(auth) and b_code.startswith(code)

    def compare(word, item):
        if type(item) == str:
            if ignore_case:
                word = word.lower()
                item = item.lower()
            return word in item
        else:
            return False

    outbbox = []
    if type(text) == str:
        text = [text]

    words = []
    for txt in text:
        txt = txt.strip()
        if txt and txt not in words:
            words.append(txt)

    n = len(words)
    for b in bbox:
        found = False
        c = 0
        for word in words:
            item_found = False
            if ":" in word:
                if compare_crs_id(word, b):
                    item_found = True
            if not item_found:
                for item in b:
                    if compare(word, item):
                        item_found = True
                        break
            if item_found:
                c += 1
                if search_op == "or":
                    found = True
                    break
        if search_op == "and" and c == n:
            found = True
        if found:
            outbbox.append(b)
    return outbbox


###############################################################################
# conversions

def stringify_bbox(bbox, header=True, separator="|"):
    r"""
    Convert a list of BBox instances to a str. If the input bbox list is empty,
    an empty string is returned.

    Args:
        bbox (list or BBox): List of BBox instances or a BBox instance.
        header (bool): Whether or not to print header. Defaults to True.
        separator (str): Column separator. Some CRS names contain commas. It
            supports special names including pipe (|), comma (,), space ( ),
            tab (\t), and newline (\n). Defaults to "|".

    Returns:
        str: Stringified bbox rows.
    """
    separator = get_separator(separator)

    if type(bbox) == BBox:
        bbox = [bbox]

    if header and bbox:
        outstr = separator.join(bbox_columns) + "\n"
    else:
        outstr = ""
    for row in bbox:
        outstr += separator.join(map(str, row)) + "\n"
    return outstr


def dictify_bbox(bbox):
    """
    Convert a list of BBox instances to a list of bbox dicts.

    Args:
        bbox (list or BBox): List of BBox instances or a BBox instance.

    Returns:
        list: List of bbox dicts.
    """
    if type(bbox) == BBox:
        bbox = [bbox]

    outdicts = []
    for row in bbox:
        outdicts.append(dict(row._asdict()))
    return outdicts


def jsonify_bbox(bbox):
    """
    Convert a list of BBox instances to a JSON object.

    Args:
        bbox (list or BBox): List of BBox instances or a BBox instance.

    Returns:
        str: JSONified bbox rows.
    """
    return json.dumps(dictify_bbox(bbox))


def extract_srids(bbox):
    """
    Extract spatial reference identifiers (SRIDs) from a list of BBox
    instances.

    Args:
        bbox (list or BBox): List of BBox instances or a BBox instance.

    Returns:
        list: List of SRID strs.
    """
    if type(bbox) == BBox:
        is_single = True
        bbox = [bbox]
    else:
        is_single = False

    srids = []
    for b in bbox:
        srids.append(f"{b.crs_auth_name}:{b.crs_code}")

    if is_single:
        srids = srids[0]

    return srids


###############################################################################
# plain printing

def print_bbox(bbox, outfile=sys.stdout, header=True, separator="|"):
    r"""
    Print a list of BBox instances in a plain format.

    Args:
        bbox (list): List of BBox instances.
        outfile (str): Output file object. Defaults to sys.stdout.
        header (bool): Whether or not to print header. Defaults to True.
        separator (str): Column separator. It supports special names including
            pipe (|), comma (,), space ( ), tab (\t), and newline (\n).
            Defaults to "|".
    """
    separator = get_separator(separator)
    print(stringify_bbox(bbox, header, separator), end="", file=outfile)


def print_srids(bbox, outfile=sys.stdout, separator="\n"):
    r"""
    Print a list of spatial reference identifiers (SRIDs) in a plain format.

    Args:
        bbox (list): List of BBox instances.
        outfile (str): Output file object. Defaults to sys.stdout.
        separator (str): SRID separator. It supports special names including
            pipe (|), comma (,), space ( ), tab (\t), and newline (\n).
            Defaults to "\n".
    """
    separator = get_separator(separator)

    if type(bbox) == BBox:
        bbox = [bbox]

    first = True
    for srid in extract_srids(bbox):
        if first:
            first = False
        else:
            srid = separator + srid
        print(srid, end="", file=outfile)
    print(file=outfile)


###############################################################################
# main

def start(
        geoms=[],
        infile="-",
        outfile="-",
        fmt="plain",
        no_header=False,
        separator=None,
        max_bbox=0,
        print_geoms=False,
        overwrite=False,
        append=False,
        start_gui=None,
        single=False,
        projpicker_db=None,
        proj_db=None,
        create=False):
    r"""
    Process options and perform requested tasks. This is the main API function.
    If geometries and an input file are specified at the same time, both
    sources are used except when the default stdin input file is specified and
    the function is run from a termal. In the latter case, only geometries are
    used and stdin is ignored. The first non-empty element in geoms can
    optionally be "and" or "or" to set the query operator. The "and" query
    operator performs the set-theoretic intersection of bbox rows while the
    "or" operator the union. Geometry types can be specified using words
    "point" for points (default), "poly" for polylines and polygons, and "bbox"
    for bounding boxes. Words "latlon" (default) and "xy" start the
    latitude-longitude and x-y coordinate systems, respectively. This function
    ignores the current coordinate system set by set_coordinate_system(),
    set_latlon(), or set_xy(), and always starts in the latitude-longitude
    coordinate system by default. The "plain", "json", "pretty", "sqlite", and
    "srid" formats are supported. No header and separator options only apply to
    the plain output format. The max_bbox option is used to limit the maximum
    number of returned BBox instances. Use 0 for all. The overwrite option
    applies to both projpicker.db and the output file, but the append option
    only appends to the output file. Only one of the overwrite or append
    options must be given. For selecting a subset of queried BBox instances, a
    GUI can be launched by setting gui to True. Results are sorted by area from
    the smallest to largest. The single argument is used to allow only one
    selection in the GUI. If projpicker_db or proj_db is None (default),
    get_projpicker_db() or get_proj_db() is used, respectively.

    Args:
        geoms (list): List of parsable geometries. Defaults to [].
        infile (str): Input geometry file. Defaults to "-" for sys.stdin.
        outfile (str): Output file. None for no output file. Defaults to "-"
            for sys.stdout.
        fmt (str): Output format (plain, json, pretty, sqlite, srid). Defaults
            to "plain".
        no_header (bool): Whether or not to print header for plain. Defaults to
            False.
        separator (str): Column separator for plain and srid output formats. It
            supports special names including pipe (|), comma (,), space ( ),
            tab (\t), and newline (\n). Defaults to None, meaning "|" for plain
            and "\n" for srid.
        max_bbox (int): Maximum number of returned BBox instances. Defaults to
            0, meaning all.
        print_geoms (bool): Whether or not to print parsed geometries and exit.
            Defaults to False.
        overwrite (bool): Whether or not to overwrite output file. Defaults to
            False.
        append (bool): Whether or not to append output to file. Defaults to
            False.
        start_gui (str): "select" to start the GUI for selecting queried BBox
            instances. "gui" to ignore input geometries and start the GUI
            immediatenly. None to not start the GUI. Defaults to None.
        single (bool): Whether or not to allow only one selection in the GUI.
            Defaults to False.
        projpicker_db (str): projpicker.db path. Defaults to None.
        proj_db (str): proj.db path. Defaults to None.
        create (bool): Whether or not to create a new projpicker.db. Defaults
            to False.

    Returns:
        list: List of queried BBox instances sorted by area.

    Raises:
        Exception: If format is invalid, both overwrite and append are True,
            either projpicker_db or outfile already exists when overwrite is
            False, proj_db does not exist when create is True, projpicker_db
            does not exist when create is False, output is None or "-" when
            append is True, or sqlite format is written to stdout.
    """
    projpicker_db = get_projpicker_db(projpicker_db)
    proj_db = get_proj_db(proj_db)

    if not has_gui:
        start_gui = None

    if fmt not in ("plain", "json", "pretty", "sqlite", "srid"):
        raise Exception(f"{fmt}: Unsupported output format")

    if overwrite and append:
        raise Exception("Both overwrite and append requested")

    if create:
        if not overwrite and os.path.isfile(projpicker_db):
            raise Exception(f"{projpicker_db}: File already exists")
        if not os.path.isfile(proj_db):
            raise Exception(f"{proj_db}: No such file found")
        create_projpicker_db(overwrite, projpicker_db, proj_db)
    elif not os.path.isfile(projpicker_db):
        raise Exception(f"{projpicker_db}: No such file found")

    if not overwrite and not append and outfile and os.path.isfile(outfile):
        raise Exception(f"{outfile}: File already exists")

    if append and (not outfile or outfile == "-"):
        raise Exception("Cannot append output to None or stdout")

    if start_gui == "gui":
        bbox, *_ = gui.start(single=single)
    else:
        if ((create and (infile != "-" or not sys.stdin.isatty())) or
            (not create and (len(geoms) == 0 or infile != "-" or
                             not sys.stdin.isatty()))):
            geoms.extend(read_file(infile))

        tidy_lines(geoms)

        if print_geoms:
            pprint.pprint(parse_mixed_geoms(geoms))
            return []

        if start_gui == "select":
            bbox, *_ = gui.start(geoms, bbox_or_quit=True, single=single)
        else:
            bbox = query_mixed_geoms(geoms, projpicker_db)

    if max_bbox > 0:
        bbox = bbox[:max_bbox]

    if not outfile:
        return bbox

    mode = "w"
    header = not no_header
    if append and outfile != "-" and os.path.isfile(outfile):
        if fmt == "plain":
            mode = "a"
            header = False
        elif fmt == "json":
            with open(outfile) as f:
                bbox_dict = json.load(f)
            bbox_dict.extend(dictify_bbox(bbox))
            bbox_json = json.dumps(bbox_dict)
        elif fmt == "pretty":
            with open(outfile) as f:
                # https://stackoverflow.com/a/65647108
                lcls = locals()
                exec("bbox_dict = " + f.read(), globals(), lcls)
                bbox_dict = lcls["bbox_dict"]
            bbox_dict.extend(dictify_bbox(bbox))
        elif fmt == "sqlite":
            bbox_merged = read_bbox_db(outfile)
            for b in bbox:
                if b not in bbox_merged:
                    bbox_merged.append(b)
            sort_bbox(bbox_merged)
            write_bbox_db(bbox_merged, outfile, True)
            return bbox_merged
        else:
            mode = "a"
    elif fmt == "json":
        bbox_json = jsonify_bbox(bbox)
    elif fmt == "pretty":
        bbox_dict = dictify_bbox(bbox)
    elif fmt == "sqlite":
        if outfile == "-":
            raise Exception("Cannot write sqlite output to stdout")
        write_bbox_db(bbox, outfile, True)
        return bbox

    if separator is None:
        if fmt == "plain":
            separator = "|"
        else:
            separator = "\n"

    f = sys.stdout if outfile == "-" else open(outfile, mode)
    if fmt == "plain":
        print_bbox(bbox, f, header, separator)
    elif fmt == "json":
        print(bbox_json, file=f)
    elif fmt == "pretty":
        # sort_dicts was added in Python 3.8, but I'm stuck with 3.7
        # https://docs.python.org/3/library/pprint.html
        if sys.version_info.major == 3 and sys.version_info.minor >= 8:
            pprint.pprint(bbox_dict, f, sort_dicts=False)
        else:
            pprint.pprint(bbox_dict, f)
    else:
        print_srids(bbox, f, separator)
    if outfile != "-":
        f.close()

    return bbox


###############################################################################
# go!

set_latlon()

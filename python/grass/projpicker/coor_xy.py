"""
This module provides parsing functions for the x-y coordinate system for
the ProjPicker API.
"""

import re
import sqlite3

from .common import (_pos_float_pat, _coor_sep_pat, get_float,
                     query_using_cursor)

# x,y
_xy_pat = f"([+-]?{_pos_float_pat}){_coor_sep_pat}([+-]?{_pos_float_pat})"

# x,y
_xy_re = re.compile(f"^{_xy_pat}$")
# xy bbox
_xy_bbox_re = re.compile(f"^{_xy_pat}{_coor_sep_pat}{_xy_pat}$")


###############################################################################
# parsing

def parse_point(point):
    """
    Parse a str of x and y. Return x and y floats. A list of two floats can be
    used in place of a str of x and y. Any missing or invalid coordinate is
    returned as None. If an output from this function is passed, the same
    output is returned.

    For example, "10,20" returns (10.0, 20.0).

    Args:
        point (str): Parsable str of x and y.

    Returns:
        float, float: Parsed x and y floats.
    """
    x = y = None
    typ = type(point)
    if typ == str:
        m = _xy_re.match(point)
        if m:
            x = float(m[1])
            y = float(m[2])
    elif typ in (list, tuple) and len(point) == 2:
        x = get_float(point[0])
        y = get_float(point[1])
    return [x, y]


def parse_bbox(bbox):
    """
    Parse a str of bottom, top, left, and right, and return bottom, top, left,
    and right floats. A list of four floats can be used in place of a str of
    left, right, bottom, and top. Any Any missing or invalid coordinate is
    returned as None. If an output from this function is passed, the same
    output is returned.

    For example, "10,20,30,40" returns (10.0, 20.0, 30.0, 40.0).

    Args:
        bbox (str): Parsable str of bottom, top, left, and right.

    Returns:
        float, float, float, float: Bottom, top, left, and right floats.
    """
    b = t = l = r = None
    typ = type(bbox)
    if typ == str:
        m = _xy_bbox_re.match(bbox)
        if m:
            s = float(m[1])
            n = float(m[2])
            l = float(m[3])
            r = float(m[4])
            if s <= n:
                b = s
                t = n
    elif typ in (list, tuple) and len(bbox) == 4:
        b = get_float(bbox[0])
        t = get_float(bbox[1])
        l = get_float(bbox[2])
        r = get_float(bbox[3])
    return [b, t, l, r]


###############################################################################
# relations

def calc_poly_bbox(poly):
    """
    Calculate the bounding box of a poly geometry and return bottom, top, left,
    and right floats.

    Args:
        poly (list): List of parsable point geometries. See parse_poly().

    Returns:
        float, float, float, float: Bottom, top, left, and right.
    """
    b = t = l = r = None

    for point in poly:
        x, y = point

        if b is None:
            b = t = y
            l = r = x
        else:
            if y < b:
                b = y
            elif y > t:
                t = y
            if x < l:
                l = x
            elif x > r:
                r = x

    return b, t, l, r


def is_point_within_bbox(point, bbox):
    """
    Return True if point is within bbox. Otherwise, return False.

    Args:
        point (list): List of x and y floats.
        bbox (BBox): BBox instance.

    Returns:
        bool: True if point is within bbox. Otherwise, False.
    """
    x, y = point
    b = bbox.bottom
    t = bbox.top
    l = bbox.left
    r = bbox.right
    if None in (b, t, l, r):
        # XXX: might be incorrect, but we cannot do better
        return False
    return l <= x <= r and b <= y <= t


def is_bbox_within_bbox(bbox1, bbox2):
    """
    Return True if bbox1 is within bbox2. Otherwise, return False.

    Args:
        bbox1 (list): List of bottom, top, left, and right floats.
        bbox2 (BBox): BBox instance.

    Returns:
        bool: True if bbox1 is within bbox2. Otherwise, False.
    """
    b, t, l, r = bbox1
    s = bbox2.bottom
    n = bbox2.top
    w = bbox2.left
    e = bbox2.right
    if None in (s, n, w, e):
        # XXX: might be incorrect, but we cannot do better
        return False
    return w <= l <= e and w <= r <= e and s <= b <= n and s <= t <= n


###############################################################################
# queries

def query_point_using_cursor(
        projpicker_cur,
        point,
        unit="any",
        proj_table="any",
        negate=False):
    """
    Return a list of BBox instances in unit in proj_table that completely
    contain an input point geometry defined by x and y. Use the negate argument
    to return non-containing BBox instances. Each BBox instance is a named
    tuple with all the columns from the bbox table in projpicker.db. This
    function is used to perform a union operation on BBox instances
    consecutively. Results are sorted by area from the smallest to largest.

    Args:
        projpicker_cur (sqlite3.Cursor): projpicker.db cursor.
        point (list or str): List of x and y floats or parsable str of x and y.
            See parse_point().
        unit (str): "any", unit values from projpicker.db.
        proj_table (str): Proj table values from projpicker.db. Defaults to
            "any".
        negate (bool): Whether or not to negate query. Defaults to False.

    Returns:
        list: List of queried BBox instances sorted by area.
    """
    x, y = parse_point(point)
    # if west_lon >= east_lon, bbox crosses the antimeridian
    sql = f"""SELECT *
              FROM bbox
              WHERE {"NOT" if negate else ""}
                    ({x} BETWEEN left AND right AND
                     {y} BETWEEN bottom AND top AND_UNIT)
              ORDER BY area_sqkm,
                       proj_table,
                       crs_auth_name, crs_code,
                       usage_auth_name, usage_code,
                       extent_auth_name, extent_code"""
    return query_using_cursor(projpicker_cur, sql, unit, proj_table)


def query_bbox_using_cursor(
        projpicker_cur,
        bbox,
        unit="any",
        proj_table="any",
        negate=False):
    """
    Return a list of BBox instances in unit in proj_table that completely
    contain an input bbox geometry defined by bottom, top, left, and right
    floats using a database cursor. Use the negate argument to return
    non-containing BBox instances. Each BBox instance is a named tuple with all
    the columns from the bbox table in projpicker.db. This function is used to
    perform a union operation on bbox rows consecutively. Results are sorted by
    area from the smallest to largest.

    Args:
        projpicker_cur (sqlite3.Cursor): projpicker.db cursor.
        bbox (list or str): List of bottom, top, left, and right floats or
            parsable str of bottom, top, left, and right. See parse_bbox().
        unit (str): "any", unit values from projpicker.db.
        proj_table (str): Proj table values from projpicker.db. Defaults to
            "any".
        negate (bool): Whether or not to negate query. Defaults to False.

    Returns:
        list: List of queried BBox instances sorted by area.
    """
    b, t, l, r = parse_bbox(bbox)
    sql = f"""SELECT *
              FROM bbox
              WHERE {"NOT" if negate else ""}
                    ({l} BETWEEN left AND right AND
                     {r} BETWEEN left AND right AND
                     {b} BETWEEN bottom AND top AND
                     {t} BETWEEN bottom AND top AND_UNIT)
              ORDER BY area_sqkm,
                       proj_table,
                       crs_auth_name, crs_code,
                       usage_auth_name, usage_code,
                       extent_auth_name, extent_code"""
    return query_using_cursor(projpicker_cur, sql, unit, proj_table)

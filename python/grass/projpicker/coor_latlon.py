"""
This module provides parsing functions for the latitude-longitude coordinate
system for the ProjPicker API.
"""
# pylint: disable=too-many-boolean-expressions

import re

from .common import _COOR_SEP_PAT, _POS_FLOAT_PAT, get_float, query_using_cursor

# symbols for degrees, minutes, and seconds (DMS)
# degree: [°od] (alt+0 in xterm for °)
# minute: ['′m]
# second: ["″s]|''
# decimal degrees
_DD_PAT = f"([+-]?{_POS_FLOAT_PAT})[°od]?"
# DMS without [SNWE]
_DMS_PAT = (
    f"([0-9]+)(?:[°od](?:[ \t]*(?:({_POS_FLOAT_PAT})['′m]?|"
    f"""([0-9]+)['′m](?:[ \t]*({_POS_FLOAT_PAT})(?:["″s]|'')?)?))?|"""
    f":(?:({_POS_FLOAT_PAT})|([0-9]+):({_POS_FLOAT_PAT})))"
)
# coordinate without [SNWE]
_COOR_PAT = (
    f"{_DD_PAT}|([+-])?{_DMS_PAT}|" f"(?:({_POS_FLOAT_PAT})[°od]?|{_DMS_PAT})[ \t]*"
)
# latitude
_LAT_PAT = f"(?:{_COOR_PAT}([SN])?)"
# longitude
_LON_PAT = f"(?:{_COOR_PAT}([WE])?)"
# latitude,longitude
_LATLON_PAT = f"{_LAT_PAT}{_COOR_SEP_PAT}{_LON_PAT}"
# matching groups for latitude:
#   1:              (-1.2)°
#   2,3,4:          (-)(1)°(2.3)'
#   2,3,5,6:        (-)(1)°(2)'(3.4)"
#   10,18:          (1.2)°(S)
#   11,12,18:       (1)°(2.3)'(S)
#   11,13,14,18:    (1)°(2)'(3.4)"(S)
#   2,3,7:          (-)(1):(2.3)
#   2,3,8,9:        (-)(1):(2):(3.4)
#   11,15,18:       (1):(2.3)(S)
#   11,16,17,18:    (1):(2):(3.4)(S)

# compiled regular expressions
# latitude,longitude
_latlon_re = re.compile(f"^{_LATLON_PAT}$")
# bounding box (south,north,west,east)
_latlon_bbox_re = re.compile(
    f"^{_LAT_PAT}{_COOR_SEP_PAT}{_LAT_PAT}{_COOR_SEP_PAT}"
    f"{_LON_PAT}{_COOR_SEP_PAT}{_LON_PAT}$"
)


###############################################################################
# parsing


def parse_coor(mat, ith, lat):
    """
    Parse the zero-based ith coordinate from a matched mat. If the format is
    degrees, minutes, and seconds (DMS), lat is used to determine its
    negativity.

    Args:
        mat (re.Match): re.compile() output.
        ith (int): Zero-based index for a coordinate group to parse from m.
        lat (bool): True if parsing latitude, False otherwise.

    Returns:
        float: Parsed coordinate in decimal degrees.
    """
    # pylint: disable=invalid-name
    i = 18 * ith
    if mat[i + 1] is not None:
        # 1: (-1.2)°
        x = float(mat[i + 1])
    elif mat[i + 4] is not None:
        # 2,3,4: (-)(1)°(2.3)'
        x = float(mat[i + 3]) + float(mat[i + 4]) / 60
    elif mat[i + 5] is not None:
        # 2,3,5,6: (-)(1)°(2)'(3.4)"
        x = float(mat[i + 3]) + float(mat[i + 5]) / 60 + float(mat[i + 6]) / 3600
    elif mat[i + 10] is not None:
        # 10,18: (1.2)°(S)
        x = float(mat[i + 10])
    elif mat[i + 12] is not None:
        # 11,12,18: (1)°(2.3)'(S)
        x = float(mat[i + 11]) + float(mat[i + 12]) / 60
    elif mat[i + 13] is not None:
        # 11,13,14,18: (1)°(2)'(3.4)"(S)
        x = float(mat[i + 11]) + float(mat[i + 13]) / 60 + float(mat[i + 14]) / 3600
    elif mat[i + 7] is not None:
        # 2,3,7: (-)(1):(2.3)
        x = float(mat[i + 3]) + float(mat[i + 7]) / 60
    elif mat[i + 8] is not None:
        # 2,3,8,9: (-)(1):(2):(3.4)
        x = float(mat[i + 3]) + float(mat[i + 8]) / 60 + float(mat[i + 9]) / 3600
    elif mat[i + 15] is not None:
        # 11,15,18: (1):(2.3)(S)
        x = float(mat[i + 11]) + float(mat[i + 15]) / 60
    elif mat[i + 16] is not None:
        # 11,16,17,18: (1):(2):(3.4)(S)
        x = float(mat[i + 11]) + float(mat[i + 16]) / 60 + float(mat[i + 17]) / 3600
    if x is not None and (
        mat[i + 2] == "-"
        or (lat and mat[i + 18] == "S")
        or (not lat and mat[i + 18] == "W")
    ):
        x *= -1
    return x


def parse_lat(mat, ith):
    """
    Parse the ith coordinate from a matched mat as a latitude.

    Args:
        mat (re.Match): re.compile() output.
        ith (int): Zero-based index for a coordinate group to parse from m as
            latitude.

    Returns:
        float: Parsed coordinate in decimal degrees.
    """
    return parse_coor(mat, ith, True)


def parse_lon(mat, ith):
    """
    Parse the ith coordinate from a matched mat as a longitude.

    Args:
        mat (re.Match): re.compile() output.
        ith (int): Zero-based index for a coordinate group to parse from m as
            longitude.

    Returns:
        float: Parsed coordinate in decimal degrees.
    """
    return parse_coor(mat, ith, False)


def parse_point(point):
    """
    Parse a str of latitude and longitude. Return latitude and longitude floats
    in decimal degrees. A list of two floats can be used in place of a str of
    latitude and longitude. Any missing or invalid coordinate is returned as
    None. If an output from this function is passed, the same output is
    returned.

    For example, "10,20" returns (10.0, 20.0).

    Args:
        point (str): Parsable str of latitude and longitude.

    Returns:
        float, float: Parsed latitude and longitude in decimal degrees.
    """
    # pylint: disable=invalid-name
    lat = lon = None
    typ = type(point)
    if typ == str:
        m = _latlon_re.match(point)
        if m:
            y = parse_lat(m, 0)
            x = parse_lon(m, 1)
            if y is not None and -90 <= y <= 90:
                lat = y
            if x is not None:  # don't check if -180 <= x <= 180 to support
                # antimeridian crossing
                lon = x
    elif typ in (list, tuple) and len(point) == 2:
        lat = get_float(point[0])
        lon = get_float(point[1])
    return [lat, lon]


def parse_bbox(bbox):
    """
    Parse a str of south, north, west, and east, and return south, north, west,
    and east floats in decimal degrees. A list of four floats can be used in
    place of a str of south, north, west, and east. Any Any missing or invalid
    coordinate is returned as None. If an output from this function is passed,
    the same output is returned.

    For example, "10,20,30,40" returns (10.0, 20.0, 30.0, 40.0).

    Args:
        bbox (str): Parsable str of south, north, west, and east.

    Returns:
        float, float, float, float: South, north, west, and east in decimal
        degrees.
    """
    # pylint: disable=invalid-name
    s = n = w = e = None
    typ = type(bbox)
    if typ == str:
        m = _latlon_bbox_re.match(bbox)
        if m:
            b = parse_lat(m, 0)
            t = parse_lat(m, 1)
            l = parse_lon(m, 2)  # noqa: E741
            r = parse_lon(m, 3)

            if -90 <= b <= 90 and -90 <= t <= 90 and b <= t:
                s = b
                n = t
            if -180 <= l <= 180:  # noqa: E741
                w = l
            if -180 <= r <= 180:
                e = r
    elif typ in (list, tuple) and len(bbox) == 4:
        s = get_float(bbox[0])
        n = get_float(bbox[1])
        w = get_float(bbox[2])
        e = get_float(bbox[3])
    return [s, n, w, e]


###############################################################################
# relations


def calc_poly_bbox(poly):
    """
    Calculate the bounding box of a poly geometry and return south, north,
    west, and east floats in decimal degrees.

    Args:
        poly (list): List of parsable point geometries. See parse_poly().

    Returns:
        float, float, float, float: South, north, west, and east in decimal
        degrees.
    """
    # pylint: disable=invalid-name
    s = n = w = e = None

    for point in poly:
        lat, lon = point

        if s is None:
            s = n = lat
            w = e = lon
        else:
            if lat < s:
                s = lat
            elif lat > n:
                n = lat
            if lon < w:
                w = lon
            elif lon > e:
                e = lon

    # if crossing the antimeridian
    while w < -180:
        w += 360
    while e > 180:
        e -= 360

    return s, n, w, e


def is_point_within_bbox(point, bbox):
    """
    Return True if point is within bbox. Otherwise, return False.

    Args:
        point (list): List of latitude and longitude floats in decimal degrees.
        bbox (BBox): BBox instance.

    Returns:
        bool: True if point is within bbox. Otherwise, False.
    """
    # pylint: disable=invalid-name
    lat, lon = point
    s = bbox.south_lat
    n = bbox.north_lat
    w = bbox.west_lon
    e = bbox.east_lon
    return s <= lat <= n and (
        w == e
        or (w == -180 and e == 180)
        or (w < e and w <= lon <= e)
        or (w > e and (-180 <= lon <= e or w <= lon <= 180))
    )


def is_bbox_within_bbox(bbox1, bbox2):
    """
    Return True if bbox1 is within bbox2. Otherwise, return False.

    Args:
        bbox1 (list): List of south, north, west, and east floats in decimal
            degrees.
        bbox2 (BBox): BBox instance.

    Returns:
        bool: True if bbox1 is within bbox2. Otherwise, False.
    """
    # pylint: disable=invalid-name
    s, n, w, e = bbox1
    b = bbox2.south_lat
    t = bbox2.north_lat
    l = bbox2.west_lon  # noqa: E741
    r = bbox2.east_lon
    return (
        b <= s <= t
        and b <= n <= t
        and (
            l == r
            or (l == -180 and r == 180)
            or (l < r and w <= e and l <= w <= r and l <= e <= r)
            or (
                l > r
                and (
                    (
                        w <= e
                        and (
                            (-180 <= w <= r and -180 <= e <= r)
                            or l <= w <= 180
                            and l <= e <= 180
                        )
                    )
                    or (w > e and -180 <= e <= r and l <= w <= 180)
                )
            )
        )
    )


###############################################################################
# queries


def query_point_using_cursor(
    projpicker_cur, point, unit="any", proj_table="any", negate=False
):
    """
    Return a list of BBox instances in unit in proj_table that completely
    contain an input point geometry defined by latitude and longitude in
    decimal degrees. Use the negate argument to return non-containing BBox
    instances. Each BBox instance is a named tuple with all the columns from
    the bbox table in projpicker.db. This function is used to perform a union
    operation on BBox instances consecutively. Results are sorted by area from
    the smallest to largest.

    Args:
        projpicker_cur (sqlite3.Cursor): projpicker.db cursor.
        point (list or str): List of latitude and longitude floats in decimal
            degrees or parsable str of latitude and longitude. See
            parse_point().
        unit (str): Unit values from projpicker.db. Defaults to "any".
        proj_table (str): Proj table values from projpicker.db. Defaults to
            "any".
        negate (bool): Whether or not to negate query. Defaults to False.

    Returns:
        list: List of queried BBox instances sorted by area.
    """
    lat, lon = parse_point(point)
    # if west_lon >= east_lon, bbox crosses the antimeridian
    sql = f"""SELECT *
              FROM bbox
              WHERE {"NOT" if negate else ""}
                    ({lat} BETWEEN south_lat AND north_lat AND
                     (west_lon = east_lon OR
                      (west_lon = -180 AND east_lon = 180) OR
                      (west_lon < east_lon AND
                       {lon} BETWEEN west_lon AND east_lon) OR
                      (west_lon > east_lon AND
                       ({lon} BETWEEN -180 AND east_lon OR
                        {lon} BETWEEN west_lon AND 180)))
                     AND_UNIT AND_PROJ_TABLE)
              ORDER BY area_sqkm,
                       proj_table,
                       crs_auth_name, crs_code,
                       usage_auth_name, usage_code,
                       extent_auth_name, extent_code"""
    return query_using_cursor(projpicker_cur, sql, unit, proj_table)


def query_bbox_using_cursor(
    projpicker_cur, bbox, unit="any", proj_table="any", negate=False
):
    """
    Return a list of BBox instances in unit in proj_table that completely
    contain an input bbox geometry defined by sout, north, west, and east using
    a database cursor. Use the negate argument to return non-containing BBox
    instances. Each BBox instance is a named tuple with all the columns from
    the bbox table in projpicker.db. This function is used to perform a union
    operation on bbox rows consecutively. Results are sorted by area from the
    smallest to largest.

    Args:
        projpicker_cur (sqlite3.Cursor): projpicker.db cursor.
        bbox (list or str): List of south, north, west, and east floats in
            decimal degrees or parsable str of south, north, west, and east.
            See parse_bbox().
        unit (str): Unit values from projpicker.db. Defaults to "any".
        proj_table (str): Proj table values from projpicker.db. Defaults to
            "any".
        negate (bool): Whether or not to negate query. Defaults to False.

    Returns:
        list: List of queried BBox instances sorted by area.
    """
    # pylint: disable=invalid-name
    s, n, w, e = parse_bbox(bbox)
    # if west_lon >= east_lon, bbox crosses the antimeridian
    sql = f"""SELECT *
              FROM bbox
              WHERE {"NOT" if negate else ""}
                    ({s} BETWEEN south_lat AND north_lat AND
                     {n} BETWEEN south_lat AND north_lat AND
                     (west_lon = east_lon OR
                      (west_lon = -180 AND east_lon = 180) OR
                      (west_lon < east_lon AND
                       {w} <= {e} AND
                       {w} BETWEEN west_lon AND east_lon AND
                       {e} BETWEEN west_lon AND east_lon) OR
                      (west_lon > east_lon AND
                       (({w} <= {e} AND
                         (({w} BETWEEN -180 AND east_lon AND
                           {e} BETWEEN -180 AND east_lon) OR
                          ({w} BETWEEN west_lon AND 180 AND
                           {e} BETWEEN west_lon AND 180))) OR
                        ({w} > {e} AND
                         {e} BETWEEN -180 AND east_lon AND
                         {w} BETWEEN west_lon AND 180))))
                     AND_UNIT AND_PROJ_TABLE)
              ORDER BY area_sqkm,
                       proj_table,
                       crs_auth_name, crs_code,
                       usage_auth_name, usage_code,
                       extent_auth_name, extent_code"""
    return query_using_cursor(projpicker_cur, sql, unit, proj_table)

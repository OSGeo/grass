"""
This module provides parsing functions for the latitude-longitude coordinate
system for the ProjPicker API.
"""

import re

from .common import _coor_sep_pat, _pos_float_pat, get_float, query_using_cursor

# symbols for degrees, minutes, and seconds (DMS)
# degree: [°od] (alt+0 in xterm for °)
# minute: ['′m]
# second: ["″s]|''
# decimal degrees
_dd_pat = f"([+-]?{_pos_float_pat})[°od]?"
# DMS without [SNWE]
_dms_pat = (
    f"([0-9]+)(?:[°od](?:[ \t]*(?:({_pos_float_pat})['′m]?|"
    f"""([0-9]+)['′m](?:[ \t]*({_pos_float_pat})(?:["″s]|'')?)?))?|"""
    f":(?:({_pos_float_pat})|([0-9]+):({_pos_float_pat})))"
)
# coordinate without [SNWE]
_coor_pat = (
    f"{_dd_pat}|([+-])?{_dms_pat}|" f"(?:({_pos_float_pat})[°od]?|{_dms_pat})[ \t]*"
)
# latitude
_lat_pat = f"(?:{_coor_pat}([SN])?)"
# longitude
_lon_pat = f"(?:{_coor_pat}([WE])?)"
# latitude,longitude
_latlon_pat = f"{_lat_pat}{_coor_sep_pat}{_lon_pat}"
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
_latlon_re = re.compile(f"^{_latlon_pat}$")
# bounding box (south,north,west,east)
_latlon_bbox_re = re.compile(
    f"^{_lat_pat}{_coor_sep_pat}{_lat_pat}{_coor_sep_pat}"
    f"{_lon_pat}{_coor_sep_pat}{_lon_pat}$"
)


###############################################################################
# parsing


def parse_coor(m, ith, lat):
    """
    Parse the zero-based ith coordinate from a matched m. If the format is
    degrees, minutes, and seconds (DMS), lat is used to determine its
    negativity.

    Args:
        m (re.Match): re.compile() output.
        ith (int): Zero-based index for a coordinate group to parse from m.
        lat (bool): True if parsing latitude, False otherwise.

    Returns:
        float: Parsed coordinate in decimal degrees.
    """
    i = 18 * ith
    if m[i + 1] is not None:
        # 1: (-1.2)°
        x = float(m[i + 1])
    elif m[i + 4] is not None:
        # 2,3,4: (-)(1)°(2.3)'
        x = float(m[i + 3]) + float(m[i + 4]) / 60
    elif m[i + 5] is not None:
        # 2,3,5,6: (-)(1)°(2)'(3.4)"
        x = float(m[i + 3]) + float(m[i + 5]) / 60 + float(m[i + 6]) / 3600
    elif m[i + 10] is not None:
        # 10,18: (1.2)°(S)
        x = float(m[i + 10])
    elif m[i + 12] is not None:
        # 11,12,18: (1)°(2.3)'(S)
        x = float(m[i + 11]) + float(m[i + 12]) / 60
    elif m[i + 13] is not None:
        # 11,13,14,18: (1)°(2)'(3.4)"(S)
        x = float(m[i + 11]) + float(m[i + 13]) / 60 + float(m[i + 14]) / 3600
    elif m[i + 7] is not None:
        # 2,3,7: (-)(1):(2.3)
        x = float(m[i + 3]) + float(m[i + 7]) / 60
    elif m[i + 8] is not None:
        # 2,3,8,9: (-)(1):(2):(3.4)
        x = float(m[i + 3]) + float(m[i + 8]) / 60 + float(m[i + 9]) / 3600
    elif m[i + 15] is not None:
        # 11,15,18: (1):(2.3)(S)
        x = float(m[i + 11]) + float(m[i + 15]) / 60
    elif m[i + 16] is not None:
        # 11,16,17,18: (1):(2):(3.4)(S)
        x = float(m[i + 11]) + float(m[i + 16]) / 60 + float(m[i + 17]) / 3600
    if x is not None and (
        m[i + 2] == "-" or (lat and m[i + 18] == "S") or (not lat and m[i + 18] == "W")
    ):
        x *= -1
    return x


def parse_lat(m, ith):
    """
    Parse the ith coordinate from a matched m as a latitude.

    Args:
        m (re.Match): re.compile() output.
        ith (int): Zero-based index for a coordinate group to parse from m as
            latitude.

    Returns:
        float: Parsed coordinate in decimal degrees.
    """
    return parse_coor(m, ith, True)


def parse_lon(m, ith):
    """
    Parse the ith coordinate from a matched m as a longitude.

    Args:
        m (re.Match): re.compile() output.
        ith (int): Zero-based index for a coordinate group to parse from m as
            longitude.

    Returns:
        float: Parsed coordinate in decimal degrees.
    """
    return parse_coor(m, ith, False)


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
    south = north = west = east = None
    typ = type(bbox)
    if typ == str:
        m = _latlon_bbox_re.match(bbox)
        if m:
            bottom = parse_lat(m, 0)
            top = parse_lat(m, 1)
            left = parse_lon(m, 2)
            right = parse_lon(m, 3)

            if -90 <= bottom <= 90 and -90 <= top <= 90 and bottom <= top:
                south = bottom
                north = top
            if -180 <= left <= 180:
                west = left
            if -180 <= right <= 180:
                east = right
    elif typ in (list, tuple) and len(bbox) == 4:
        south = get_float(bbox[0])
        north = get_float(bbox[1])
        west = get_float(bbox[2])
        east = get_float(bbox[3])
    return [south, north, west, east]


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
    south = north = west = east = None

    for point in poly:
        lat, lon = point

        if south is None:
            south = north = lat
            west = east = lon
        else:
            if lat < south:
                south = lat
            elif lat > north:
                north = lat
            if lon < west:
                west = lon
            elif lon > east:
                east = lon

    # if crossing the antimeridian
    while west < -180:
        west += 360
    while east > 180:
        east -= 360

    return south, north, west, east


def is_point_within_bbox(point, bbox):
    """
    Return True if point is within bbox. Otherwise, return False.

    Args:
        point (list): List of latitude and longitude floats in decimal degrees.
        bbox (BBox): BBox instance.

    Returns:
        bool: True if point is within bbox. Otherwise, False.
    """
    lat, lon = point
    south = bbox.south_lat
    north = bbox.north_lat
    west = bbox.west_lon
    east = bbox.east_lon
    return south <= lat <= north and (
        west == east
        or (west == -180 and east == 180)
        or (west < east and west <= lon <= east)
        or (west > east and (-180 <= lon <= east or west <= lon <= 180))
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
    south, north, west, east = bbox1
    bottom = bbox2.south_lat
    top = bbox2.north_lat
    left = bbox2.west_lon
    right = bbox2.east_lon
    return (
        bottom <= south <= top
        and bottom <= north <= top
        and (
            left == right
            or (left == -180 and right == 180)
            or (
                left < right
                and west <= east
                and left <= west <= right
                and left <= east <= right
            )
            or (
                left > right
                and (
                    (
                        west <= east
                        and (
                            (-180 <= west <= right and -180 <= east <= right)
                            or left <= west <= 180
                            and left <= east <= 180
                        )
                    )
                    or (west > east and -180 <= east <= right and left <= west <= 180)
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
    south, north, west, east = parse_bbox(bbox)
    # if west_lon >= east_lon, bbox crosses the antimeridian
    sql = f"""SELECT *
              FROM bbox
              WHERE {"NOT" if negate else ""}
                    ({south} BETWEEN south_lat AND north_lat AND
                     {north} BETWEEN south_lat AND north_lat AND
                     (west_lon = east_lon OR
                      (west_lon = -180 AND east_lon = 180) OR
                      (west_lon < east_lon AND
                       {west} <= {east} AND
                       {west} BETWEEN west_lon AND east_lon AND
                       {east} BETWEEN west_lon AND east_lon) OR
                      (west_lon > east_lon AND
                       (({west} <= {east} AND
                         (({west} BETWEEN -180 AND east_lon AND
                           {east} BETWEEN -180 AND east_lon) OR
                          ({west} BETWEEN west_lon AND 180 AND
                           {east} BETWEEN west_lon AND 180))) OR
                        ({west} > {east} AND
                         {east} BETWEEN -180 AND east_lon AND
                         {west} BETWEEN west_lon AND 180))))
                     AND_UNIT AND_PROJ_TABLE)
              ORDER BY area_sqkm,
                       proj_table,
                       crs_auth_name, crs_code,
                       usage_auth_name, usage_code,
                       extent_auth_name, extent_code"""
    return query_using_cursor(projpicker_cur, sql, unit, proj_table)

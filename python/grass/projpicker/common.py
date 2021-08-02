"""
This module provides common variables and functions for other ProjPicker
modules.
"""

import os
import re
import collections

projpicker_verbose_env = "PROJPICKER_VERBOSE"

# regular expression patterns
# coordinate separator
coor_sep = ","
coor_sep_pat = f"[ \t]*[{coor_sep} \t][ \t]*"
# positive float
pos_float_pat = "(?:[0-9]+(?:\.[0-9]*)?|\.[0-9]+)"

# bbox table schema
bbox_schema = """
CREATE TABLE bbox (
    proj_table TEXT NOT NULL CHECK (length(proj_table) >= 1),
    crs_name TEXT NOT NULL CHECK (length(crs_name) >= 2),
    crs_auth_name TEXT NOT NULL CHECK (length(crs_auth_name) >= 1),
    crs_code TEXT NOT NULL CHECK (length(crs_code) >= 1),
    usage_auth_name TEXT NOT NULL CHECK (length(usage_auth_name) >= 1),
    usage_code TEXT NOT NULL CHECK (length(usage_code) >= 1),
    extent_auth_name TEXT NOT NULL CHECK (length(extent_auth_name) >= 1),
    extent_code TEXT NOT NULL CHECK (length(extent_code) >= 1),
    south_lat FLOAT CHECK (south_lat BETWEEN -90 AND 90),
    north_lat FLOAT CHECK (north_lat BETWEEN -90 AND 90),
    west_lon FLOAT CHECK (west_lon BETWEEN -180 AND 180),
    east_lon FLOAT CHECK (east_lon BETWEEN -180 AND 180),
    bottom FLOAT,
    top FLOAT,
    left FLOAT,
    right FLOAT,
    unit TEXT NOT NULL CHECK (length(unit) >= 2),
    area_sqkm FLOAT CHECK (area_sqkm > 0),
    CONSTRAINT pk_bbox PRIMARY KEY (
        crs_auth_name, crs_code,
        usage_auth_name, usage_code
    ),
    CONSTRAINT check_bbox_lat CHECK (south_lat <= north_lat)
)
"""

# all column names in the bbox table
bbox_columns = re.sub("^ +| +$", "",
               re.sub("\n", " ",
               re.sub("(?:^[A-Z]| ).*", "",
               re.sub("\([^(]*\)", "",
               re.sub("^(?:CREATE TABLE.*|\))$|^ *", "",
                      bbox_schema, flags=re.MULTILINE),
                      flags=re.DOTALL), flags=re.MULTILINE))).split()

# BBox namedtuple class
BBox = collections.namedtuple("BBox", bbox_columns)


def is_verbose():
    return os.environ.get(projpicker_verbose_env, "NO") == "YES"


def get_float(x):
    """
    Typecast x into float; return None on failure.

    Args:
        x (str or float): Float in str or float.

    Returns:
        float or None: Typecasted x in float if successful, None otherwise.
    """
    if type(x) != float:
        try:
            x = float(x)
        except:
            x = None
    return x


def query_using_cursor(
        projpicker_cur,
        sql,
        unit="any",
        proj_table="any"):
    """
    Return a list of BBox instances in unit in proj_table using a SQL
    statement.

    Args:
        projpicker_cur (sqlite3.Cursor): projpicker.db cursor.
        sql (str): SQL statement with optional AND_UNIT and AND_PROJ_TABLE.
        unit (str): Unit values from projpicker.db. Defaults to "any".
        proj_table (str): Proj table values from projpicker.db. Defaults to
            "any".

    Returns:
        list: List of queried BBox instances sorted by area.
    """
    outbbox = []
    params = []
    if unit == "any" and proj_table == "any":
        sql = sql.replace(
                "AND_UNIT", "").replace(
                "AND_PROJ_TABLE", "")
    elif unit == "any":
        sql = sql.replace(
                "AND_UNIT", "").replace(
                "AND_PROJ_TABLE", "AND proj_table = ?")
        params.append(proj_table)
    elif proj_table == "any":
        sql = sql.replace(
                "AND_PROJ_TABLE", "").replace(
                "AND_UNIT", "AND unit = ?")
        params.append(unit)
    else:
        sql = sql.replace(
                "AND_UNIT", "AND unit = ?").replace(
                "AND_PROJ_TABLE", "AND proj_table = ?")
        params.extend([unit, proj_table])
    projpicker_cur.execute(sql, params)
    for row in map(BBox._make, projpicker_cur.fetchall()):
        outbbox.append(row)
    return outbbox

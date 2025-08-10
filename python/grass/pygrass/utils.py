import fnmatch
import itertools
import os
from sqlite3 import OperationalError

import grass.lib.gis as libgis
from grass.script import core as grasscore
from grass.script import utils as grassutils

# flake8: noqa: E402
libgis.G_gisinit("")

import grass.lib.raster as libraster
from grass.lib.ctypes_preamble import String
from grass.pygrass.errors import GrassError

# flake8: qa


test_vector_name = "Utils_test_vector"
test_raster_name = "Utils_test_raster"


def looking(obj, filter_string):
    """
    >>> import grass.lib.vector as libvect
    >>> sorted(looking(libvect, "*by_box*"))  # doctest: +NORMALIZE_WHITESPACE
    ['Vect_select_areas_by_box', 'Vect_select_isles_by_box',
     'Vect_select_lines_by_box', 'Vect_select_nodes_by_box']

    """
    word_list = dir(obj)
    word_list.sort()
    return fnmatch.filter(word_list, filter_string)


def findfiles(dirpath, match=None):
    """Return a list of the files"""
    res = []
    for f in sorted(os.listdir(dirpath)):
        abspath = os.path.join(dirpath, f)
        if os.path.isdir(abspath):
            res.extend(findfiles(abspath, match))

        if match:
            if fnmatch.fnmatch(abspath, match):
                res.append(abspath)
        else:
            res.append(abspath)
    return res


def findmaps(type, pattern=None, mapset="", location="", gisdbase=""):
    """Return a list of tuples containing the names of the:

    * map
    * mapset,
    * location,
    * gisdbase

    """
    from grass.pygrass.gis import Gisdbase, Location, Mapset

    def find_in_location(type, pattern, location):
        res = []
        for msetname in location.mapsets():
            mset = Mapset(msetname, location.name, location.gisdbase)
            res.extend(
                [
                    (m, mset.name, mset.location, mset.gisdbase)
                    for m in mset.glist(type, pattern)
                ]
            )
        return res

    def find_in_gisdbase(type, pattern, gisdbase):
        return [
            a
            for loc in gisdbase.locations()
            for a in find_in_location(type, pattern, Location(loc, gisdbase.name))
        ]

    if gisdbase and location and mapset:
        mset = Mapset(mapset, location, gisdbase)
        return [
            (m, mset.name, mset.location, mset.gisdbase)
            for m in mset.glist(type, pattern)
        ]
    if gisdbase and location:
        loc = Location(location, gisdbase)
        return find_in_location(type, pattern, loc)
    if gisdbase:
        gis = Gisdbase(gisdbase)
        return find_in_gisdbase(type, pattern, gis)
    if location:
        loc = Location(location)
        return find_in_location(type, pattern, loc)
    if mapset:
        mset = Mapset(mapset)
        return [
            (m, mset.name, mset.location, mset.gisdbase)
            for m in mset.glist(type, pattern)
        ]
    gis = Gisdbase()
    return find_in_gisdbase(type, pattern, gis)


def remove(oldname, maptype):
    """Remove a map"""
    grasscore.run_command("g.remove", quiet=True, flags="f", type=maptype, name=oldname)


def rename(oldname, newname, maptype, **kwargs):
    """Rename a map"""
    kwargs.update(
        {
            maptype: "{old},{new}".format(old=oldname, new=newname),
        }
    )
    grasscore.run_command("g.rename", quiet=True, **kwargs)


def copy(existingmap, newmap, maptype, **kwargs):
    """Copy a map

    >>> copy(test_vector_name, "mycensus", "vector")
    >>> rename("mycensus", "mynewcensus", "vector")
    >>> remove("mynewcensus", "vector")

    """
    kwargs.update({maptype: "{old},{new}".format(old=existingmap, new=newmap)})
    grasscore.run_command("g.copy", quiet=True, **kwargs)


def decode(obj, encoding=None):
    """Decode string coming from c functions,
    can be ctypes class String, bytes, or None
    """
    if isinstance(obj, String):
        return grassutils.decode(obj.data, encoding=encoding)
    if isinstance(obj, bytes):
        return grassutils.decode(obj)
    # eg None
    return obj


def getenv(env):
    """Return the current grass environment variables

    >>> from grass.script.core import gisenv
    >>> getenv("MAPSET") == gisenv()["MAPSET"]
    True

    """
    return decode(libgis.G_getenv_nofatal(env))


def get_mapset_raster(mapname, mapset=""):
    """Return the mapset of the raster map

    >>> get_mapset_raster(test_raster_name) == getenv("MAPSET")
    True

    """
    return decode(libgis.G_find_raster2(mapname, mapset))


def get_mapset_vector(mapname, mapset=""):
    """Return the mapset of the vector map

    >>> get_mapset_vector(test_vector_name) == getenv("MAPSET")
    True

    """
    return decode(libgis.G_find_vector2(mapname, mapset))


def is_clean_name(name) -> bool:
    """Return if the name is valid

    >>> is_clean_name("census")
    True
    >>> is_clean_name("0census")
    True
    >>> is_clean_name("census?")
    True
    >>> is_clean_name("cénsus")
    False

    """
    return libgis.G_legal_filename(name) >= 0


def coor2pixel(coord, region):
    """Convert coordinates into a pixel row and col

    >>> from grass.pygrass.gis.region import Region
    >>> reg = Region()
    >>> coor2pixel((reg.west, reg.north), reg)
    (0.0, 0.0)
    >>> coor2pixel((reg.east, reg.south), reg) == (reg.rows, reg.cols)
    True

    """
    (east, north) = coord
    return (
        libraster.Rast_northing_to_row(north, region.byref()),
        libraster.Rast_easting_to_col(east, region.byref()),
    )


def pixel2coor(pixel, region):
    """Convert row and col of a pixel into a coordinates

    >>> from grass.pygrass.gis.region import Region
    >>> reg = Region()
    >>> pixel2coor((0, 0), reg) == (reg.north, reg.west)
    True
    >>> pixel2coor((reg.cols, reg.rows), reg) == (reg.south, reg.east)
    True

    """
    (col, row) = pixel
    return (
        libraster.Rast_row_to_northing(row, region.byref()),
        libraster.Rast_col_to_easting(col, region.byref()),
    )


def get_raster_for_points(poi_vector, raster, column=None, region=None):
    """Query a raster map for each point feature of a vector

    Example

    >>> from grass.pygrass.raster import RasterRow
    >>> from grass.pygrass.gis.region import Region
    >>> from grass.pygrass.vector import VectorTopo
    >>> from grass.pygrass.vector.geometry import Point

    Create a vector map

    >>> cols = [("cat", "INTEGER PRIMARY KEY"), ("value", "double precision")]
    >>> vect = VectorTopo("test_vect_2")
    >>> vect.open("w", tab_name="test_vect_2", tab_cols=cols)
    >>> vect.write(
    ...     Point(10, 6),
    ...     cat=1,
    ...     attrs=[
    ...         10,
    ...     ],
    ... )
    >>> vect.write(
    ...     Point(12, 6),
    ...     cat=2,
    ...     attrs=[
    ...         12,
    ...     ],
    ... )
    >>> vect.write(
    ...     Point(14, 6),
    ...     cat=3,
    ...     attrs=[
    ...         14,
    ...     ],
    ... )
    >>> vect.table.conn.commit()
    >>> vect.close()

    Setup the raster sampling

    >>> region = Region()
    >>> region.from_rast(test_raster_name)
    >>> region.set_raster_region()
    >>> ele = RasterRow(test_raster_name)

    Sample the raster layer at the given points, return a list of values

    >>> l = get_raster_for_points(vect, ele, region=region)
    >>> l[0]  # doctest: +ELLIPSIS
    (1, 10.0, 6.0, 1)
    >>> l[1]  # doctest: +ELLIPSIS
    (2, 12.0, 6.0, 1)

    Add a new column and sample again

    >>> vect.open("r")
    >>> vect.table.columns.add(test_raster_name, "double precision")
    >>> vect.table.conn.commit()
    >>> test_raster_name in vect.table.columns
    True
    >>> get_raster_for_points(vect, ele, column=test_raster_name, region=region)
    True
    >>> vect.table.filters.select("value", test_raster_name)
    Filters('SELECT value, Utils_test_raster FROM test_vect_2;')
    >>> cur = vect.table.execute()
    >>> r = cur.fetchall()
    >>> r[0]  # doctest: +ELLIPSIS
    (10.0, 1.0)
    >>> r[1]  # doctest: +ELLIPSIS
    (12.0, 1.0)
    >>> remove("test_vect_2", "vect")

    :param poi_vector: A VectorTopo object that contains points
    :param raster: raster object
    :param str column: column name to update in the attrinute table,
                       if set to None a list of sampled values will be returned
    :param region: The region to work with, if not set the current computational region
                   will be used

    :return: True in case of success and a specified column for update,
             if column name for update was not set a list of (id, x, y, value) is
             returned
    """
    from math import isnan

    if not column:
        result = []
    if region is None:
        from grass.pygrass.gis.region import Region

        region = Region()
    if not poi_vector.is_open():
        poi_vector.open("r")
    if not raster.is_open():
        raster.open("r")
    if poi_vector.num_primitive_of("point") == 0:
        raise GrassError(_("Vector doesn't contain points"))

    for poi in poi_vector.viter("points"):
        val = raster.get_value(poi, region)
        if column:
            if val is not None and not isnan(val):
                poi.attrs[column] = val
        else:  # noqa: PLR5501
            if val is not None and not isnan(val):
                result.append((poi.id, poi.x, poi.y, val))
            else:
                result.append((poi.id, poi.x, poi.y, None))
    if not column:
        return result
    poi.attrs.commit()
    return True


def r_export(rast, output="", fmt="png", **kargs):
    from grass.pygrass.modules import Module

    if rast.exist():
        output = output or "%s_%s.%s" % (rast.name, rast.mapset, fmt)
        Module(
            "r.out.%s" % fmt,
            input=rast.fullname(),
            output=output,
            overwrite=True,
            **kargs,
        )
        return output
    msg = "Raster map does not exist."
    raise ValueError(msg)


def get_lib_path(modname, libname=None):
    """Return the path of the libname contained in the module.

    .. deprecated:: 7.1
        Use :func:`grass.script.utils.get_lib_path` instead.
    """
    from grass.script.utils import get_lib_path

    return get_lib_path(modname=modname, libname=libname)


def set_path(modulename, dirname=None, path="."):
    """Set sys.path looking in the the local directory GRASS directories.

    :param modulename: string with the name of the GRASS module
    :param dirname: string with the directory name containing the python
                    libraries, default None
    :param path: string with the path to reach the dirname locally.

    .. deprecated:: 7.1
        Use :func:`grass.script.utils.set_path` instead.
    """
    from grass.script.utils import set_path

    return set_path(modulename=modulename, dirname=dirname, path=path)


def split_in_chunk(iterable, length=10):
    """Split a list in chunk.

    >>> for chunk in split_in_chunk(range(25)):
    ...     print(chunk)
    (0, 1, 2, 3, 4, 5, 6, 7, 8, 9)
    (10, 11, 12, 13, 14, 15, 16, 17, 18, 19)
    (20, 21, 22, 23, 24)
    >>> for chunk in split_in_chunk(range(25), 3):
    ...     print(chunk)
    (0, 1, 2)
    (3, 4, 5)
    (6, 7, 8)
    (9, 10, 11)
    (12, 13, 14)
    (15, 16, 17)
    (18, 19, 20)
    (21, 22, 23)
    (24,)
    """
    it = iter(iterable)
    while True:
        chunk = tuple(itertools.islice(it, length))
        if not chunk:
            return
        yield chunk


def table_exist(cursor, table_name):
    """Return True if the table exist False otherwise"""
    try:
        # sqlite
        cursor.execute(
            "SELECT name FROM sqlite_master"
            " WHERE type='table' AND name='%s';" % table_name
        )
    except OperationalError:
        try:
            # pg
            cursor.execute(
                "SELECT EXISTS(SELECT * FROM "
                "information_schema.tables "
                "WHERE table_name=%s)" % table_name
            )
        except OperationalError:
            return False
    one = cursor.fetchone() if cursor else None
    return bool(one and one[0])


def txt2numpy(
    tablestring,
    sep=",",
    names=None,
    null_value=None,
    fill_value=None,
    comments="#",
    usecols=None,
    encoding=None,
    structured=True,
):
    """Read table-like output from grass modules as Numpy array;
    format instructions are handed down to Numpys genfromtxt function

    :param stdout: tabular stdout from GRASS GIS module call
    :type stdout: str|byte

    :param sep: Separator delimiting columns
    :type sep: str

    :param names: List of strings with names for columns
    :type names: list

    :param null_value: Characters representing the no-data value
    :type null_value: str

    :param fill_value: Value to fill no-data with
    :type fill_value: str

    :param comments: Character that identifies comments in the input string
    :type comments: str

    :param usecols: List of columns to import
    :type usecols: list

    :param structured: return structured array if True, un-structured otherwise
    :type structured: bool

    :return: numpy.ndarray

        >>> import grass.script.core as grasscore
        >>> import numpy as np
        >>> txt = grasscore.read_command(
        ...     "r.stats", flags="cn", input="basin_50K,geology_30m", separator="|"
        ... )
        >>> np_array = txt2numpy(txt, sep="|", names=None)
        >>> print(np_array)

    """

    from io import BytesIO
    import numpy as np

    if not encoding:
        encoding = grassutils._get_encoding()

    if type(tablestring).__name__ == "str":
        tablestring = grasscore.encode(tablestring, encoding=encoding)
    elif type(tablestring).__name__ != "bytes":
        raise GrassError(_("Unsupported data type"))

    kwargs = {
        "missing_values": null_value,
        "filling_values": fill_value,
        "usecols": usecols,
        "names": names,
        "encoding": encoding,
        "delimiter": sep,
    }

    if structured:
        kwargs["dtype"] = None

    return np.genfromtxt(BytesIO(tablestring), **kwargs)


def numpy2table(
    np_array,
    table,
    connection,
    formats=None,
    names=False,
    column_prefix="column",
    update_formats=True,
    overwrite=True,
):
    """Write numpy array to database table. Most suitable SQL data type is
    extracted from the numpy dtype, as well as column names (if possible),
    if not given by the user

    :param np_array: structured or unstructured 2d numpy array
    :type np_array: numpy.ndarray

    :param connection: A database (PostgreSQL or SQLite) connection object
    :type connection: connection

    :param formats: A list of strings that describe the dtype of the numpy array
    :type formats: list

    :param names: List of strings with names for columns
    :type names: list

        :param column_prefix: A sring with the prefix to be used for column names
        :type column_prefix: str

    :param update_formats: Flag whether to overwrite existing format definitions in structured numpy arrays
    :type update_formats: bool

    :param overwrite: Whether to overwrite existing tables with the same name
    :type overwrite: bool

        >>> import numpy as np
        >>> from io import BytesIO
        >>> import sqlite3
        >>> conn = sqlite3.connect("file::memory:?cache=shared")
        >>> np_array = np.array(
        ...     [
        ...         ["112", "abc", "2005-01-01", "13.543", "True", "1"],
        ...         [
        ...             "9223372036854775806",
        ...             "test",
        ...             "2005-02-01",
        ...             "29.543",
        ...             "False",
        ...             "0",
        ...         ],
        ...     ]
        ... )
        >>> table = "test"
        >>> numpy2table(
        ...     np_array, table, conn, names=None, formats=None, update_formats=True
        ... )
        >>> conn.close()
    """

    import sys
    from io import BytesIO
    import numpy as np
    from numpy.lib import recfunctions as rfn

    connection_info = str(type(connection)).split("'")[1].lower()
    if not "pg" and "sqlite" not in connection_info:
        raise GrassError(_("DB backend not supported, please check connection!"))

    dbdriver = (
        "sqlite" if "sqlite" in str(type(connection)).split("'")[1].lower() else "pg"
    )

    # Check DB connection
    if dbdriver == "sqlite":
        import sqlite3
    elif dbdriver == "pg":
        import psycopg2
        from psycopg2.extras import execute_values

    """
    # Get all numpy dtypes
    for char_code in np.typecodes["All"]:
        print(np.dtype(char_code).num, ": \"\" # ", np.dtype(char_code), char_code)
    # Compare to:
      - SQLite: https://www.sqlite.org/datatype3.html
      - PostgreSQL: https://www.postgresql.org/docs/11/datatype.html
    """
    sql_to_dtype = {
        "sqlite": {
            "INTEGER": [0, 2, 3, 4, 5, 6, 7, 8, 9, 10],
            "REAL": [11, 12, 23],
            "TEXT": [1, 18, 19, 21, 22],
            "BLOB": [17],
        },
        "pg": {
            "boolean": [0],
            "smallint": [2, 3, 4],
            "integer": [5, 6],
            "bigint": [7, 8, 9, 10],
            "text": [1, 18, 19],
            "real": [11, 23],
            "double precision": [12],
            "bytea": [17],
            "timestamp": [21],
            "intervall": [22],
        },
    }

    # Dictionary to translate numpy dtypes to backend-specific SQL data types
    dtype_to_sql = {
        "sqlite": {
            0: "INTEGER",  # numpy: bool ; short form: ?
            1: "TEXT",  # numpy: int8 ; short form: b
            2: "INTEGER",  # numpy: uint8 ; short form: B
            3: "INTEGER",  # numpy: int16 ; short form: h
            4: "INTEGER",  # numpy: uint16 ; short form: H
            5: "INTEGER",  # numpy: int32 ; short form: i
            6: "INTEGER",  # numpy: uint32 ; short form: I
            7: "INTEGER",  # numpy: int32 ; short form: l
            8: "INTEGER",  # numpy: uint32 ; short form: L
            9: "INTEGER",  # numpy: int64 ; short form: q, p
            10: "INTEGER",  # numpy: uint64 ; short form: Q, P
            11: "REAL",  # numpy: float32 ; short form: f
            12: "REAL",  # numpy: float64 ; short form: d
            13: "UNSUPPORTED",  # numpy: float64 ; short form: g
            14: "UNSUPPORTED",  # numpy: complex64 ; short form: F
            15: "UNSUPPORTED",  # numpy: complex128 ; short form: D
            16: "UNSUPPORTED",  # numpy: complex128 ; short form: G
            17: "BLOB",  # numpy: object ; short form: O
            18: "TEXT",  # numpy: |S0 ; short form: S
            19: "TEXT",  # numpy: <U0 ; short form: U
            20: "UNSUPPORTED",  # numpy: |V0 ; short form: V
            21: "TEXT",  # numpy: datetime64 ; short form: M
            22: "TEXT",  # numpy: timedelta64 ; short form: m
            23: "REAL",  # numpy: float16 ; short form: e
        },
        "pg": {
            0: "boolean",  # numpy: bool ; short form: ?
            1: "text",  # numpy: int8 ; short form: b
            2: "smallint",  # numpy: uint8 ; short form: B
            3: "smallint",  # numpy: int16 ; short form: h
            4: "smallint",  # numpy: uint16 ; short form: H
            5: "integer",  # numpy: int32 ; short form: i
            6: "integer",  # numpy: uint32 ; short form: I
            7: "integer",  # numpy: int32 ; short form: l
            8: "bigint",  # numpy: uint32 ; short form: L
            9: "bigint",  # numpy: int64 ; short form: q, p
            10: "bigint",  # numpy: uint64 ; short form: Q, P
            11: "real",  # numpy: float32 ; short form: f
            12: "double precision",  # numpy: float64 ; short form: d
            13: "UNSUPPORTED",  # numpy: float64 ; short form: g
            14: "UNSUPPORTED",  # numpy: complex64 ; short form: F
            15: "UNSUPPORTED",  # numpy: complex128 ; short form: D
            16: "UNSUPPORTED",  # numpy: complex128 ; short form: G
            17: "bytea",  # numpy: object ; short form: O
            18: "text",  # numpy: |S0 ; short form: S
            19: "text",  # numpy: <U0 ; short form: U
            20: "UNSUPPORTED",  # numpy: |V0 ; short form: V
            21: "timestamp",  # numpy: datetime64 ; short form: M
            22: "interval",  # numpy: timedelta64 ; short form: m
            23: "real",  # numpy: float16 ; short form: e
        },
    }

    # Check input data validity
    if names and formats:
        if len(formats) != len(names):
            raise GrassError(
                _(
                    "Length of parameter <names> differs from length of parameter <formats>!"
                )
            )

    length_names = (
        len(np_array.dtype.names) if np_array.dtype.names else np_array.shape[1]
    )
    if names:
        if len(names) != length_names:
            raise GrassError(
                _(
                    "Length of parameter <names> does not match number of columns in array!"
                )
            )

    # Check if user-given formats can be assigned to data
    if formats:
        if len(formats) != length_names:
            raise GrassError(
                _(
                    "Length of parameter <formats> does not match number of columns in array!"
                )
            )

        for idx, np_format in enumerate(formats):
            try:
                if np_format == "str":
                    size = (
                        np_array[:, idx].dtype.itemsize
                        if not np_array.dtype.names
                        else np_array[np_array.dtype.names[idx]].dtype.itemsize
                    )
                    np_format = np.dtype((np_format, size,)).str
                    formats[idx] = np_format
                if not np_array.dtype.names:
                    np_array[:, idx].astype(np_format).astype("str") == np_array[
                        :, idx
                    ].astype(np_format).astype("str")
                else:
                    np_array[np_array.dtype.names[idx]].astype(np_format)
            except:
                if not np_array.dtype.names:
                    raise GrassError(
                        _(
                            "Cannot represent column number {} as {}".format(
                                idx, np_format
                            )
                        )
                    )
                raise GrassError(
                    _(
                        "Cannot represent column {} as {}".format(
                            np_array.dtype.names[idx], np_format
                        )
                    )
                )

    # Start with unstructured array
    if not np_array.dtype.names:
        np_array_view = np_array
    elif update_formats or formats:
        np_array_view = rfn.structured_to_unstructured(np_array)

    # Generate a list of minimal formats to represent data in array columns
    if not formats and (update_formats or not np_array.dtype.names):
        formats = []
        for col_idx in range(np_array_view.shape[1]):
            dtype = None
            types = [
                np.uint8,
                np.int8,
                np.uint16,
                np.int16,
                np.uint32,
                np.int32,
                np.uint64,
                np.int64,
                np.single,
                np.double,
                np.longlong,
                np.ulonglong,
                np.datetime64,
            ]
            for np_dtype in types:  # np.typecodes["All"]:
                try:
                    # Check if data can be casted and still match original after type-cast
                    if not all(
                        np_array_view[:, col_idx].astype(np_dtype).astype("str")
                        == np_array_view[:, col_idx].astype("str")
                    ):
                        continue

                    # print(np_array_view[:,col_idx].astype(np_dtype))
                    # Bool types represented as integer
                    if np_dtype in (np.uint8, np.int8):
                        dtype = (
                            np.dtype(np.bool)
                            if np.max(np_array_view[:, col_idx].astype(np_dtype)) == 1
                            and np.min(np_array_view[:, col_idx].astype(np_dtype)) == 0
                            else np.dtype(np_dtype)
                        )
                    else:
                        # get character code of dtype
                        dtype = np_array_view[:, col_idx].astype(np_dtype).dtype
                    break
                except:
                    continue
            if not dtype:
                dtype = np_array_view[:, col_idx].dtype
            formats.append(dtype)

    # Generate a list of tuples with column names and formats for the array
    if not names and formats:
        dtype = np.dtype(
            [
                (
                    "{}{}".format(column_prefix, idx)
                    if not np_array.dtype.names
                    else np_array.dtype.names[idx],
                    np_format,
                )
                for idx, np_format in enumerate(formats)
            ]
        )
    elif names and formats:
        dtype = np.dtype(
            [(names[idx], np_format) for idx, np_format in enumerate(formats)]
        )
    elif names and not formats:
        dtype = np.dtype([(np_name, np_name) for idx, np_name in enumerate(names)])
    else:
        dtype = None

    # Start with unstructured array
    if not np_array.dtype.names or update_formats or formats:
        structured_array = rfn.unstructured_to_structured(np_array_view, dtype)
    else:
        structured_array = np_array

    # Generate a list of SQL data types for columns
    columns = []
    for col in structured_array.dtype.names:
        type_code = structured_array[col].dtype.num
        columns.append("{} {}".format(col, dtype_to_sql[dbdriver][type_code]))

    # Define initial SQL strings
    create_sql = "CREATE TABLE {} ({});".format(table, ", ".join(columns))

    # Execute SQL code
    with connection:
        cur = connection.cursor()
        if overwrite:
            drop_sql = "DROP TABLE IF EXISTS {};".format(table)
            cur.execute(drop_sql)
        # Create table
        cur.execute(create_sql)

        # Insert data
        if dbdriver == "sqlite":
            insert_sql = "INSERT INTO {}({}) VALUES({});".format(
                table,
                ", ".join(structured_array.dtype.names),
                ",".join(["?"] * len(structured_array.dtype.names)),
            )
            cur.executemany(insert_sql, structured_array.tolist())
        elif dbdriver == "pg":
            # For arrays that do not contain objects or binary data, they could be loaded using the copy statement
            if {
                np.dtype(descr[1]).num for descr in structured_array.dtype.descr
            }.isdisjoint({13, 14, 15, 16, 17, 20}):
                np_array_txt = BytesIO()
                np.savetxt(np_array_txt, structured_array, delimiter="\t", fmt="%s")
                np_array_txt.seek(0)
                cur.copy_from(np_array_txt, table)
            else:
                insert_sql = "INSERT INTO {}({}) VALUES %s;".format(
                    table,
                    ", ".join(structured_array.dtype.names),
                    ",".join(["?"] * len(structured_array.dtype.names)),
                )
                execute_values(cur, insert_sql, structured_array.tolist())
        connection.commit()


def create_test_vector_map(map_name="test_vector"):
    """This functions creates a vector map layer with points, lines, boundaries,
    centroids, areas, isles and attributes for testing purposes

    This should be used in doc and unit tests to create location/mapset
    independent vector map layer. This map includes 3 points, 3 lines,
    11 boundaries and 4 centroids. The attribute table contains cat, name
    and value columns.

    :param map_name: The vector map name that should be used

    .. code-block:: none

                                   P1 P2 P3
            6                       *  *  *
            5
            4    _______ ___ ___   L1 L2 L3
         Y  3   |A1___ *|  *|  *|   |  |  |
            2   | |A2*| |   |   |   |  |  |
            1   | |___| |A3 |A4 |   |  |  |
            0   |_______|___|___|   |  |  |
           -1
             -1 0 1 2 3 4 5 6 7 8 9 10 12 14
                            X
    """

    from grass.pygrass.vector import VectorTopo
    from grass.pygrass.vector.geometry import Boundary, Centroid, Line, Point

    cols = [
        ("cat", "INTEGER PRIMARY KEY"),
        ("name", "varchar(50)"),
        ("value", "double precision"),
    ]
    with VectorTopo(map_name, mode="w", tab_name=map_name, tab_cols=cols) as vect:
        # Write 3 points
        vect.write(Point(10, 6), cat=1, attrs=("point", 1))
        vect.write(Point(12, 6), cat=1)
        vect.write(Point(14, 6), cat=1)
        # Write 3 lines
        vect.write(Line([(10, 4), (10, 2), (10, 0)]), cat=2, attrs=("line", 2))
        vect.write(Line([(12, 4), (12, 2), (12, 0)]), cat=2)
        vect.write(Line([(14, 4), (14, 2), (14, 0)]), cat=2)
        # boundaries 1 - 4
        vect.write(Boundary(points=[(0, 0), (0, 4)]))
        vect.write(Boundary(points=[(0, 4), (4, 4)]))
        vect.write(Boundary(points=[(4, 4), (4, 0)]))
        vect.write(Boundary(points=[(4, 0), (0, 0)]))
        # 5. boundary (Isle)
        vect.write(Boundary(points=[(1, 1), (1, 3), (3, 3), (3, 1), (1, 1)]))
        # boundaries 6 - 8
        vect.write(Boundary(points=[(4, 4), (6, 4)]))
        vect.write(Boundary(points=[(6, 4), (6, 0)]))
        vect.write(Boundary(points=[(6, 0), (4, 0)]))
        # boundaries 9 - 11
        vect.write(Boundary(points=[(6, 4), (8, 4)]))
        vect.write(Boundary(points=[(8, 4), (8, 0)]))
        vect.write(Boundary(points=[(8, 0), (6, 0)]))
        # Centroids, all have the same cat and attribute
        vect.write(Centroid(x=3.5, y=3.5), cat=3, attrs=("centroid", 3))
        vect.write(Centroid(x=2.5, y=2.5), cat=3)
        vect.write(Centroid(x=5.5, y=3.5), cat=3)
        vect.write(Centroid(x=7.5, y=3.5), cat=3)

        vect.organization = "Thuenen Institut"
        vect.person = "Soeren Gebbert"
        vect.title = "Test dataset"
        vect.comment = "This is a comment"

        vect.table.conn.commit()

        vect.organization = "Thuenen Institut"
        vect.person = "Soeren Gebbert"
        vect.title = "Test dataset"
        vect.comment = "This is a comment"
        vect.close()


def create_test_stream_network_map(map_name="streams"):
    R"""Create test data

    This functions creates a vector map layer with lines that represent
    a stream network with two different graphs. The first graph
    contains a loop, the second can be used as directed graph.

    This should be used in doc and unit tests to create location/mapset
    independent vector map layer.

    :param map_name: The vector map name that should be used

    .. code-block:: none

           1(0,2)  3(2,2)
            \     /
           1 \   / 2
              \ /
               2(1,1)
        6(0,1) ||  5(2,1)
           5 \ || / 4
              \||/
               4(1,0)
               |
               | 6
               |7(1,-1)

           7(0,-1) 8(2,-1)
            \     /
           8 \   / 9
              \ /
               9(1, -2)
               |
               | 10
               |
              10(1,-3)
    """

    from grass.pygrass.vector import VectorTopo
    from grass.pygrass.vector.geometry import Line

    cols = [("cat", "INTEGER PRIMARY KEY"), ("id", "INTEGER")]
    with VectorTopo(map_name, mode="w", tab_name=map_name, tab_cols=cols) as streams:
        # First flow graph
        line = Line([(0, 2), (0.22, 1.75), (0.55, 1.5), (1, 1)])
        streams.write(line, cat=1, attrs=(1,))
        line = Line([(2, 2), (1, 1)])
        streams.write(line, cat=2, attrs=(2,))
        line = Line([(1, 1), (0.85, 0.5), (1, 0)])
        streams.write(line, cat=3, attrs=(3,))
        line = Line([(2, 1), (1, 0)])
        streams.write(line, cat=4, attrs=(4,))
        line = Line([(0, 1), (1, 0)])
        streams.write(line, cat=5, attrs=(5,))
        line = Line([(1, 0), (1, -1)])
        streams.write(line, cat=6, attrs=(6,))
        # Reverse line 3
        line = Line([(1, 0), (1.15, 0.5), (1, 1)])
        streams.write(line, cat=7, attrs=(7,))

        # second flow graph
        line = Line([(0, -1), (1, -2)])
        streams.write(line, cat=8, attrs=(8,))
        line = Line([(2, -1), (1, -2)])
        streams.write(line, cat=9, attrs=(9,))
        line = Line([(1, -2), (1, -3)])
        streams.write(line, cat=10, attrs=(10,))

        streams.organization = "Thuenen Institut"
        streams.person = "Soeren Gebbert"
        streams.title = "Test dataset for stream networks"
        streams.comment = "This is a comment"

        streams.table.conn.commit()
        streams.close()


if __name__ == "__main__":
    import doctest

    from grass.script.core import run_command

    create_test_vector_map(test_vector_name)
    run_command("g.region", n=50, s=0, e=60, w=0, res=1)
    run_command("r.mapcalc", expression="%s = 1" % (test_raster_name), overwrite=True)

    doctest.testmod()

    mset = get_mapset_vector(test_vector_name, mapset="")
    if mset:
        # Remove the generated vector map, if exists
        run_command("g.remove", flags="f", type="vector", name=test_vector_name)
    mset = get_mapset_raster(test_raster_name, mapset="")
    if mset:
        # Remove the generated raster map, if exists
        run_command("g.remove", flags="f", type="raster", name=test_raster_name)

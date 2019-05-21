# -*- coding: utf-8 -*-
import itertools
import fnmatch
import os
from sqlite3 import OperationalError

import grass.lib.gis as libgis
libgis.G_gisinit('')
import grass.lib.raster as libraster
from grass.lib.ctypes_preamble import String
from grass.script import core as grasscore
from grass.script import utils as grassutils

from grass.pygrass.errors import GrassError


test_vector_name="Utils_test_vector"
test_raster_name="Utils_test_raster"

def looking(obj, filter_string):
    """
    >>> import grass.lib.vector as libvect
    >>> sorted(looking(libvect, '*by_box*'))  # doctest: +NORMALIZE_WHITESPACE
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


def findmaps(type, pattern=None, mapset='', location='', gisdbase=''):
    """Return a list of tuple contining the names of the:

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
            res.extend([(m, mset.name, mset.location, mset.gisdbase)
                        for m in mset.glist(type, pattern)])
        return res

    def find_in_gisdbase(type, pattern, gisdbase):
        res = []
        for loc in gisdbase.locations():
            res.extend(find_in_location(type, pattern,
                                        Location(loc, gisdbase.name)))
        return res

    if gisdbase and location and mapset:
        mset = Mapset(mapset, location, gisdbase)
        return [(m, mset.name, mset.location, mset.gisdbase)
                for m in mset.glist(type, pattern)]
    elif gisdbase and location:
        loc = Location(location, gisdbase)
        return find_in_location(type, pattern, loc)
    elif gisdbase:
        gis = Gisdbase(gisdbase)
        return find_in_gisdbase(type, pattern, gis)
    elif location:
        loc = Location(location)
        return find_in_location(type, pattern, loc)
    elif mapset:
        mset = Mapset(mapset)
        return [(m, mset.name, mset.location, mset.gisdbase)
                for m in mset.glist(type, pattern)]
    else:
        gis = Gisdbase()
        return find_in_gisdbase(type, pattern, gis)


def remove(oldname, maptype):
    """Remove a map"""
    grasscore.run_command('g.remove', quiet=True, flags='f',
                          type=maptype, name=oldname)


def rename(oldname, newname, maptype, **kwargs):
    """Rename a map"""
    kwargs.update({maptype: '{old},{new}'.format(old=oldname, new=newname), })
    grasscore.run_command('g.rename', quiet=True, **kwargs)


def copy(existingmap, newmap, maptype, **kwargs):
    """Copy a map

    >>> copy(test_vector_name, 'mycensus', 'vector')
    >>> rename('mycensus', 'mynewcensus', 'vector')
    >>> remove('mynewcensus', 'vector')

    """
    kwargs.update({maptype: '{old},{new}'.format(old=existingmap, new=newmap)})
    grasscore.run_command('g.copy', quiet=True, **kwargs)


def decode(obj, encoding=None):
    """Decode string coming from c functions,
    can be ctypes class String, bytes, or None
    """
    if isinstance(obj, String):
        return grassutils.decode(obj.data, encoding=encoding)
    elif isinstance(obj, bytes):
        return grassutils.decode(obj)
    else:
        # eg None
        return obj


def getenv(env):
    """Return the current grass environment variables

    >>> from grass.script.core import gisenv
    >>> getenv("MAPSET") == gisenv()["MAPSET"]
    True

    """
    return decode(libgis.G_getenv_nofatal(env))


def get_mapset_raster(mapname, mapset=''):
    """Return the mapset of the raster map

    >>> get_mapset_raster(test_raster_name) == getenv("MAPSET")
    True

    """
    return decode(libgis.G_find_raster2(mapname, mapset))


def get_mapset_vector(mapname, mapset=''):
    """Return the mapset of the vector map

    >>> get_mapset_vector(test_vector_name) == getenv("MAPSET")
    True

    """
    return decode(libgis.G_find_vector2(mapname, mapset))


def is_clean_name(name):
    """Return if the name is valid

    >>> is_clean_name('census')
    True
    >>> is_clean_name('0census')
    True
    >>> is_clean_name('census?')
    True
    >>> is_clean_name('cénsus')
    False

    """
    if libgis.G_legal_filename(name) < 0:
        return False
    return True


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
    return (libraster.Rast_northing_to_row(north, region.byref()),
            libraster.Rast_easting_to_col(east, region.byref()))


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
    return (libraster.Rast_row_to_northing(row, region.byref()),
            libraster.Rast_col_to_easting(col, region.byref()))


def get_raster_for_points(poi_vector, raster, column=None, region=None):
    """Query a raster map for each point feature of a vector

    Example

    >>> from grass.pygrass.raster import RasterRow
    >>> from grass.pygrass.gis.region import Region
    >>> from grass.pygrass.vector import VectorTopo
    >>> from grass.pygrass.vector.geometry import Point

    Create a vector map

    >>> cols = [(u'cat', 'INTEGER PRIMARY KEY'),
    ...         (u'value', 'double precision')]
    >>> vect = VectorTopo("test_vect_2")
    >>> vect.open("w",tab_name="test_vect_2",
    ...           tab_cols=cols)
    >>> vect.write(Point(10, 6), cat=1, attrs=[10, ])
    >>> vect.write(Point(12, 6), cat=2, attrs=[12, ])
    >>> vect.write(Point(14, 6), cat=3, attrs=[14, ])
    >>> vect.table.conn.commit()
    >>> vect.close()

    Setup the raster sampling

    >>> region = Region()
    >>> region.from_rast(test_raster_name)
    >>> region.set_raster_region()
    >>> ele = RasterRow(test_raster_name)

    Sample the raster layer at the given points, return a list of values

    >>> l = get_raster_for_points(vect, ele, region=region)
    >>> l[0]                                        # doctest: +ELLIPSIS
    (1, 10.0, 6.0, 1)
    >>> l[1]                                        # doctest: +ELLIPSIS
    (2, 12.0, 6.0, 1)

    Add a new column and sample again

    >>> vect.open("r")
    >>> vect.table.columns.add(test_raster_name,'double precision')
    >>> vect.table.conn.commit()
    >>> test_raster_name in vect.table.columns
    True
    >>> get_raster_for_points(vect, ele, column=test_raster_name, region=region)
    True
    >>> vect.table.filters.select('value', test_raster_name)
    Filters('SELECT value, Utils_test_raster FROM test_vect_2;')
    >>> cur = vect.table.execute()
    >>> r = cur.fetchall()
    >>> r[0]                                        # doctest: +ELLIPSIS
    (10.0, 1.0)
    >>> r[1]                                        # doctest: +ELLIPSIS
    (12.0, 1.0)
    >>> remove('test_vect_2','vect')

    :param poi_vector: A VectorTopo object that contains points
    :param raster: raster object
    :param str column: column name to update in the attrinute table,
                       if set to None a list of sampled values will be returned
    :param region: The region to work with, if not set the current computational region will be used

    :return: True in case of success and a specified column for update,
             if column name for update was not set a list of (id, x, y, value) is returned
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
    if poi_vector.num_primitive_of('point') == 0:
        raise GrassError(_("Vector doesn't contain points"))

    for poi in poi_vector.viter('points'):
        val = raster.get_value(poi, region)
        if column:
            if val is not None and not isnan(val):
                poi.attrs[column] = val
        else:
            if val is not None and not isnan(val):
                result.append((poi.id, poi.x, poi.y, val))
            else:
                result.append((poi.id, poi.x, poi.y, None))
    if not column:
        return result
    else:
        poi.attrs.commit()
        return True


def r_export(rast, output='', fmt='png', **kargs):
    from grass.pygrass.modules import Module
    if rast.exist():
        output = output if output else "%s_%s.%s" % (rast.name, rast.mapset,
                                                     fmt)
        Module('r.out.%s' % fmt, input=rast.fullname(), output=output,
               overwrite=True, **kargs)
        return output
    else:
        raise ValueError('Raster map does not exist.')


def get_lib_path(modname, libname=None):
    """Return the path of the libname contained in the module.

    .. deprecated:: 7.1
        Use :func:`grass.script.utils.get_lib_path` instead.
    """
    from grass.script.utils import get_lib_path
    return get_lib_path(modname=modname, libname=libname)


def set_path(modulename, dirname=None, path='.'):
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

    >>> for chunk in split_in_chunk(range(25)): print (chunk)
    (0, 1, 2, 3, 4, 5, 6, 7, 8, 9)
    (10, 11, 12, 13, 14, 15, 16, 17, 18, 19)
    (20, 21, 22, 23, 24)
    >>> for chunk in split_in_chunk(range(25), 3): print (chunk)
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
        cursor.execute("SELECT name FROM sqlite_master"
                       " WHERE type='table' AND name='%s';" % table_name)
    except OperationalError:
        try:
            # pg
            cursor.execute("SELECT EXISTS(SELECT * FROM "
                           "information_schema.tables "
                           "WHERE table_name=%s)" % table_name)
        except OperationalError:
            return False
    one = cursor.fetchone() if cursor else None
    return True if one and one[0] else False


def create_test_vector_map(map_name="test_vector"):
    """This functions creates a vector map layer with points, lines, boundaries,
       centroids, areas, isles and attributes for testing purposes

       This should be used in doc and unit tests to create location/mapset
       independent vector map layer. This map includes 3 points, 3 lines,
       11 boundaries and 4 centroids. The attribute table contains cat, name
       and value columns.

        param map_name: The vector map name that should be used



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
    from grass.pygrass.vector.geometry import Point, Line, Centroid, Boundary

    cols = [(u'cat', 'INTEGER PRIMARY KEY'),
            (u'name','varchar(50)'),
            (u'value', 'double precision')]
    with VectorTopo(map_name, mode='w', tab_name=map_name,
                    tab_cols=cols) as vect:

        # Write 3 points
        vect.write(Point(10, 6), cat=1, attrs=("point", 1))
        vect.write(Point(12, 6), cat=1)
        vect.write(Point(14, 6), cat=1)
        # Write 3 lines
        vect.write(Line([(10, 4), (10, 2), (10,0)]), cat=2, attrs=("line", 2))
        vect.write(Line([(12, 4), (12, 2), (12,0)]), cat=2)
        vect.write(Line([(14, 4), (14, 2), (14,0)]), cat=2)
        # boundaries 1 - 4
        vect.write(Boundary(points=[(0, 0), (0,4)]))
        vect.write(Boundary(points=[(0, 4), (4,4)]))
        vect.write(Boundary(points=[(4, 4), (4,0)]))
        vect.write(Boundary(points=[(4, 0), (0,0)]))
        # 5. boundary (Isle)
        vect.write(Boundary(points=[(1, 1), (1,3), (3, 3), (3,1), (1,1)]))
        # boundaries 6 - 8
        vect.write(Boundary(points=[(4, 4), (6,4)]))
        vect.write(Boundary(points=[(6, 4), (6,0)]))
        vect.write(Boundary(points=[(6, 0), (4,0)]))
        # boundaries 9 - 11
        vect.write(Boundary(points=[(6, 4), (8,4)]))
        vect.write(Boundary(points=[(8, 4), (8,0)]))
        vect.write(Boundary(points=[(8, 0), (6,0)]))
        # Centroids, all have the same cat and attribute
        vect.write(Centroid(x=3.5, y=3.5), cat=3, attrs=("centroid", 3))
        vect.write(Centroid(x=2.5, y=2.5), cat=3)
        vect.write(Centroid(x=5.5, y=3.5), cat=3)
        vect.write(Centroid(x=7.5, y=3.5), cat=3)

        vect.organization = 'Thuenen Institut'
        vect.person = 'Soeren Gebbert'
        vect.title = 'Test dataset'
        vect.comment = 'This is a comment'

        vect.table.conn.commit()

        vect.organization = "Thuenen Institut"
        vect.person = "Soeren Gebbert"
        vect.title = "Test dataset"
        vect.comment = "This is a comment"
        vect.close()

def create_test_stream_network_map(map_name="streams"):
    """
       This functions creates a vector map layer with lines that represent
       a stream network with two different graphs. The first graph
       contains a loop, the second can be used as directed graph.

       This should be used in doc and unit tests to create location/mapset
       independent vector map layer.

        param map_name: The vector map name that should be used

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

    cols = [(u'cat', 'INTEGER PRIMARY KEY'), (u'id', 'INTEGER')]
    with VectorTopo(map_name, mode='w', tab_name=map_name,
                    tab_cols=cols) as streams:

        # First flow graph
        l = Line([(0,2), (0.22, 1.75), (0.55, 1.5), (1,1)])
        streams.write(l, cat=1, attrs=(1,))
        l = Line([(2,2),(1,1)])
        streams.write(l, cat=2, attrs=(2,))
        l = Line([(1,1), (0.85, 0.5), (1,0)])
        streams.write(l, cat=3, attrs=(3,))
        l = Line([(2,1),(1,0)])
        streams.write(l, cat=4, attrs=(4,))
        l = Line([(0,1),(1,0)])
        streams.write(l, cat=5, attrs=(5,))
        l = Line([(1,0),(1,-1)])
        streams.write(l, cat=6, attrs=(6,))
        # Reverse line 3
        l = Line([(1,0), (1.15, 0.5),(1,1)])
        streams.write(l, cat=7, attrs=(7,))

        # second flow graph
        l = Line([(0,-1),(1,-2)])
        streams.write(l, cat=8, attrs=(8,))
        l = Line([(2,-1),(1,-2)])
        streams.write(l, cat=9, attrs=(9,))
        l = Line([(1,-2),(1,-3)])
        streams.write(l, cat=10, attrs=(10,))

        streams.organization = 'Thuenen Institut'
        streams.person = 'Soeren Gebbert'
        streams.title = 'Test dataset for stream networks'
        streams.comment = 'This is a comment'

        streams.table.conn.commit()
        streams.close()

if __name__ == "__main__":

    import doctest
    from grass.pygrass import utils
    from grass.script.core import run_command

    utils.create_test_vector_map(test_vector_name)
    run_command("g.region", n=50, s=0, e=60, w=0, res=1)
    run_command("r.mapcalc", expression="%s = 1"%(test_raster_name),
                             overwrite=True)

    doctest.testmod()

    """Remove the generated vector map, if exist"""
    mset = utils.get_mapset_vector(test_vector_name, mapset='')
    if mset:
        run_command("g.remove", flags='f', type='vector', name=test_vector_name)
    mset = utils.get_mapset_raster(test_raster_name, mapset='')
    if mset:
        run_command("g.remove", flags='f', type='raster', name=test_raster_name)

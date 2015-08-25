# -*- coding: utf-8 -*-
import itertools
import fnmatch
import os
from sqlite3 import OperationalError

import grass.lib.gis as libgis
libgis.G_gisinit('')
import grass.lib.raster as libraster
from grass.script import core as grasscore

from grass.pygrass.errors import GrassError


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

    >>> copy('census', 'mycensus', 'vector')
    >>> rename('mycensus', 'mynewcensus', 'vector')
    >>> remove('mynewcensus', 'vector')

    """
    kwargs.update({maptype: '{old},{new}'.format(old=existingmap, new=newmap)})
    grasscore.run_command('g.copy', quiet=True, **kwargs)


def getenv(env):
    """Return the current grass environment variables

    >>> from grass.script.core import gisenv
    >>> getenv("MAPSET") == gisenv()["MAPSET"]
    True

    """
    return libgis.G_getenv_nofatal(env)


def get_mapset_raster(mapname, mapset=''):
    """Return the mapset of the raster map

    >>> get_mapset_raster('elevation')
    'PERMANENT'

    """
    return libgis.G_find_raster2(mapname, mapset)


def get_mapset_vector(mapname, mapset=''):
    """Return the mapset of the vector map

    >>> get_mapset_vector('census')
    'PERMANENT'

    """
    return libgis.G_find_vector2(mapname, mapset)


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
    return (libraster.Rast_northing_to_row(north, region.c_region),
            libraster.Rast_easting_to_col(east, region.c_region))


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
    return (libraster.Rast_row_to_northing(row, region.c_region),
            libraster.Rast_col_to_easting(col, region.c_region))


def get_raster_for_points(poi_vector, raster, column=None, region=None):
    """Query a raster map for each point feature of a vector

    Example

    >>> from grass.pygrass.vector import VectorTopo
    >>> from grass.pygrass.raster import RasterRow
    >>> ele = RasterRow('elevation')
    >>> copy('schools','myschools','vector')
    >>> sch = VectorTopo('myschools')
    >>> sch.open(mode='r')
    >>> get_raster_for_points(sch, ele)               # doctest: +ELLIPSIS
    [(1, 633649.2856743174, 221412.94434781274, 145.06602), ...]
    >>> sch.table.columns.add('elevation','double precision')
    >>> 'elevation' in sch.table.columns
    True
    >>> get_raster_for_points(sch, ele, column='elevation')
    True
    >>> sch.table.filters.select('NAMESHORT','elevation')
    Filters(u'SELECT NAMESHORT, elevation FROM myschools;')
    >>> cur = sch.table.execute()
    >>> cur.fetchall()                                # doctest: +ELLIPSIS
    [(u'SWIFT CREEK', 145.06602), ... (u'9TH GRADE CTR', None)]
    >>> remove('myschools','vect')


    :param point: point vector object
    :param raster: raster object
    :param str column: column name to update

    """
    from math import isnan
    if not column:
        result = []
    if region is None:
        from grass.pygrass.gis.region import Region
        region = Region()
    if not poi_vector.is_open():
        poi_vector.open()
    if not raster.is_open():
        raster.open()
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


def get_lib_path(modname, libname):
    """Return the path of the libname contained in the module.

    >>> get_lib_path(modname='r.modis', libname='libmodis')
    """
    from os.path import isdir, join
    from os import getenv

    if isdir(join(getenv('GISBASE'), 'etc', modname)):
        path = join(os.getenv('GISBASE'), 'etc', modname)
    elif getenv('GRASS_ADDON_BASE') and \
            isdir(join(getenv('GRASS_ADDON_BASE'), 'etc', modname)):
        path = join(getenv('GRASS_ADDON_BASE'), 'etc', modname)
    elif getenv('GRASS_ADDON_BASE') and \
            isdir(join(getenv('GRASS_ADDON_BASE'), modname, modname)):
        path = join(os.getenv('GRASS_ADDON_BASE'), modname, modname)
    elif isdir(join('..', libname)):
        path = join('..', libname)
    else:
        path = None
    return path


def set_path(modulename, dirname, path='.'):
    import sys
    """Set sys.path looking in the the local directory GRASS directories."""
    pathlib = os.path.join(path, dirname)
    if os.path.exists(pathlib):
        # we are running the script from the script directory
        sys.path.append(os.path.abspath(path))
    else:
        # running from GRASS GIS session
        path = get_lib_path(modulename, dirname)
        if path is None:
            raise ImportError("Not able to find the path %s directory." % path)
        sys.path.append(path)


def split_in_chunk(iterable, length=10):
    """Split a list in chunk.

    >>> for chunk in split_in_chunk(range(25)): print chunk
    (0, 1, 2, 3, 4, 5, 6, 7, 8, 9)
    (10, 11, 12, 13, 14, 15, 16, 17, 18, 19)
    (20, 21, 22, 23, 24)
    >>> for chunk in split_in_chunk(range(25), 3): print chunk
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
       11 boundaries and 4 centroids. The attribute table contains cat and name
       columns.

        param map_name: The vector map name that should be used
    """

    from grass.pygrass.vector import VectorTopo
    from grass.pygrass.vector.geometry import Point, Line, Centroid, Boundary, Area

    cols = [(u'cat', 'INTEGER PRIMARY KEY'),
            (u'name','varchar(50)'),
            (u'value', 'double precision')]
    with VectorTopo(map_name, mode='w', tab_name=map_name,
                    tab_cols=cols) as vect:

        # Write 3 points, 3 lines and 11 boundaries with one nested isle and 4 centroids
        #
        #
        #  ______ ___ ___   *  *  *
        # |1 __ *|3 *|4 *|  |  |  |
        # | |2*| |   |   |  |  |  |
        # | |__| |   |   |  |  |  |
        # |______|___|___|  |  |  |
        #
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

        vect.table.conn.commit()
        vect.close()

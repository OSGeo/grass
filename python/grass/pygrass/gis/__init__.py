#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from __future__ import (nested_scopes, generators, division, absolute_import,
                        with_statement, print_function, unicode_literals)
from os import listdir
from os.path import join, isdir
import shutil
import ctypes as ct
import fnmatch


import grass.lib.gis as libgis
from grass.pygrass.errors import GrassError
from grass.script.utils import encode, decode
from grass.pygrass.utils import getenv
from grass.pygrass.gis.region import Region

test_vector_name = "Gis_test_vector"
test_raster_name = "Gis_test_raster"

libgis.G_gisinit('')


ETYPE = {'raster': libgis.G_ELEMENT_RASTER,
         'raster_3d': libgis.G_ELEMENT_RASTER3D,
         'vector': libgis.G_ELEMENT_VECTOR,
         'label': libgis.G_ELEMENT_LABEL,
         'region': libgis.G_ELEMENT_REGION,
         'group': libgis.G_ELEMENT_GROUP}


CHECK_IS = {"GISBASE": libgis.G_is_gisbase,
            "GISDBASE": lambda x: True,
            "LOCATION_NAME": libgis.G_is_location,
            "MAPSET": libgis.G_is_mapset}


def is_valid(value, path, type):
    """Private function to check the correctness of a value.

    :param value: Name of the directory
    :type value: str

    :param path: Path where the directory is located
    :type path: path

    :param type: it is a string defining the type that will e checked,
                 valid types are: GISBASE, GISDBASE, LOCATION_NAME, MAPSET
    :type type: str

    :return: True if valid else False
    :rtype: str
    """
    return bool(CHECK_IS[type](join(path, value)))


def _check_raise(value, path, type):
    """Private function to check the correctness of a value.

    :param value: Name of the directory
    :type value: str

    :param path: Path where the directory is located
    :type path: path

    :param type: it is a string defining the type that will e checked,
                 valid types are: GISBASE, GISDBASE, LOCATION_NAME, MAPSET
    :type type: str

    :return: the value if verify else None and
             if value is empty return environmental variable
    :rtype: str
    """
    if value == '':
        from grass.pygrass.utils import getenv
        return getenv(type)
    if is_valid(value, path, type):
        return value
    raise GrassError("%s <%s> not found" % (type.title(), join(path, value)))


def set_current_mapset(mapset, location=None, gisdbase=None):
    """Set the current mapset as working area

    :param mapset: Name of the mapset
    :type value: str

    :param location: Name of the location
    :type location: str

    :param gisdbase: Name of the gisdbase
    :type gisdbase: str
    """
    libgis.G_setenv('MAPSET', mapset)
    if location:
        libgis.G_setenv('LOCATION_NAME', location)
    if gisdbase:
        libgis.G_setenv('GISDBASE', gisdbase)


def make_mapset(mapset, location=None, gisdbase=None):
    """Create a new mapset

    :param mapset: Name of the mapset
    :type value: str

    :param location: Name of the location
    :type location: str

    :param gisdbase: Name of the gisdbase
    :type gisdbase: str"""
    res = libgis.G_make_mapset(gisdbase, location, mapset)
    if res == -1:
        raise GrassError("Cannot create new mapset")
    elif res == -2:
        raise GrassError("Illegal name")


class Gisdbase(object):
    """Return Gisdbase object. ::

        >>> from grass.script.core import gisenv
        >>> gisdbase = Gisdbase()
        >>> gisdbase.name == gisenv()['GISDBASE']
        True

    ..
    """

    def __init__(self, gisdbase=''):
        self.name = gisdbase

    def _get_name(self):
        return self._name

    def _set_name(self, name):
        self._name = _check_raise(name, '', "GISDBASE")

    name = property(fget=_get_name, fset=_set_name,
                    doc="Set or obtain the name of GISDBASE")

    def __str__(self):
        return self.name

    def __repr__(self):
        return 'Gisdbase(%s)' % self.name

    def __getitem__(self, location):
        """Return a Location object. ::

            >>> from grass.script.core import gisenv
            >>> loc_env = gisenv()['LOCATION_NAME']
            >>> gisdbase = Gisdbase()
            >>> loc_py = gisdbase[loc_env]
            >>> loc_env == loc_py.name
            True

        ..
        """
        if location in self.locations():
            return Location(location, self.name)
        else:
            raise KeyError('Location: %s does not exist' % location)

    def __iter__(self):
        for loc in self.locations():
            yield Location(loc, self.name)

    # TODO remove or complete this function
    def new_location(self):
        if libgis.G_make_location() != 0:
            raise GrassError("Cannot create new location")

    def locations(self):
        """Return a list of locations that are available in the gisdbase: ::

            >>> gisdbase = Gisdbase()
            >>> gisdbase.locations()                     # doctest: +ELLIPSIS
            [...]

        ..
        """
        return sorted([loc for loc in listdir(self.name)
                       if libgis.G_is_location(encode(join(self.name, loc)))])


class Location(object):
    """Location object ::

        >>> from grass.script.core import gisenv
        >>> location = Location()
        >>> location                                      # doctest: +ELLIPSIS
        Location(...)
        >>> location.gisdbase == gisenv()['GISDBASE']
        True
        >>> location.name == gisenv()['LOCATION_NAME']
        True

    ..
    """

    def __init__(self, location='', gisdbase=''):
        self.gisdbase = gisdbase
        self.name = location

    def _get_gisdb(self):
        return self._gisdb

    def _set_gisdb(self, gisdb):
        self._gisdb = _check_raise(gisdb, '', "GISDBASE")

    gisdbase = property(fget=_get_gisdb, fset=_set_gisdb,
                        doc="Set or obtain the name of GISDBASE")

    def _get_name(self):
        return self._name

    def _set_name(self, name):
        self._name = _check_raise(name, self._gisdb, "LOCATION_NAME")

    name = property(fget=_get_name, fset=_set_name,
                    doc="Set or obtain the name of LOCATION")

    def __getitem__(self, mapset):
        if mapset in self.mapsets():
            return Mapset(mapset)
        else:
            raise KeyError('Mapset: %s does not exist' % mapset)

    def __iter__(self):
        lpath = self.path()
        return (m for m in listdir(lpath)
                if (isdir(join(lpath, m)) and is_valid(m, lpath, "MAPSET")))

    def __len__(self):
        return len(self.mapsets())

    def __str__(self):
        return self.name

    def __repr__(self):
        return 'Location(%r)' % self.name

    def mapsets(self, pattern=None, permissions=True):
        """Return a list of the available mapsets.

        :param pattern: the pattern to filter the result
        :type pattern: str
        :param permissions: check the permission of mapset
        :type permissions: bool
        :return: a list of mapset's names
        :rtype: list of strings

        ::

            >>> location = Location()
            >>> sorted(location.mapsets())                # doctest: +ELLIPSIS
            [...]

        """
        mapsets = [mapset for mapset in self]
        if permissions:
            mapsets = [mapset for mapset in mapsets
                       if libgis.G_mapset_permissions(encode(mapset))]
        if pattern:
            return fnmatch.filter(mapsets, pattern)
        return mapsets

    def path(self):
        """Return the complete path of the location"""
        return join(self.gisdbase, self.name)


class Mapset(object):
    """Mapset ::

        >>> from grass.script.core import gisenv
        >>> genv = gisenv()
        >>> mapset = Mapset()
        >>> mapset                                        # doctest: +ELLIPSIS
        Mapset(...)
        >>> mapset.gisdbase == genv['GISDBASE']
        True
        >>> mapset.location == genv['LOCATION_NAME']
        True
        >>> mapset.name == genv['MAPSET']
        True

    ..
    """

    def __init__(self, mapset='', location='', gisdbase=''):
        self.gisdbase = gisdbase
        self.location = location
        self.name = mapset
        self.visible = VisibleMapset(self.name, self.location, self.gisdbase)

    def _get_gisdb(self):
        return self._gisdb

    def _set_gisdb(self, gisdb):
        self._gisdb = _check_raise(gisdb, '', "GISDBASE")

    gisdbase = property(fget=_get_gisdb, fset=_set_gisdb,
                        doc="Set or obtain the name of GISDBASE")

    def _get_loc(self):
        return self._loc

    def _set_loc(self, loc):
        self._loc = _check_raise(loc, self._gisdb, "LOCATION_NAME")

    location = property(fget=_get_loc, fset=_set_loc,
                        doc="Set or obtain the name of LOCATION")

    def _get_name(self):
        return self._name

    def _set_name(self, name):
        self._name = _check_raise(name, join(self._gisdb, self._loc), "MAPSET")

    name = property(fget=_get_name, fset=_set_name,
                    doc="Set or obtain the name of MAPSET")

    def __str__(self):
        return self.name

    def __repr__(self):
        return 'Mapset(%r)' % self.name

    def glist(self, type, pattern=None):
        """Return a list of grass types like:

            * 'group',
            * 'label',
            * 'raster',
            * 'raster_3d',
            * 'region',
            * 'vector',

        :param type: the type of element to query
        :type type: str
        :param pattern: the pattern to filter the result
        :type pattern: str

        ::

            >>> mapset = Mapset()
            >>> mapset.current()
            >>> rast = mapset.glist('raster')
            >>> test_raster_name in rast
            True
            >>> vect = mapset.glist('vector')
            >>> test_vector_name in vect
            True

        ..
        """
        if type not in ETYPE:
            str_err = "Type %s is not valid, valid types are: %s."
            raise TypeError(str_err % (type, ', '.join(ETYPE.keys())))
        clist = libgis.G_list(ETYPE[type], self.gisdbase,
                              self.location, self.name)
        elist = []
        for el in clist:
            el_name = ct.cast(el, ct.c_char_p).value
            if el_name:
                elist.append(decode(el_name))
            else:
                if pattern:
                    return fnmatch.filter(elist, pattern)
                return elist

    def is_current(self):
        """Check if the MAPSET is the working MAPSET"""
        return (self.name == getenv('MAPSET') and
                self.location == getenv('LOCATION_NAME') and
                self.gisdbase == getenv('GISDBASE'))

    def current(self):
        """Set the mapset as current"""
        set_current_mapset(self.name, self.location, self.gisdbase)

    def delete(self):
        """Delete the mapset"""
        if self.is_current():
            raise GrassError('The mapset is in use.')
        shutil.rmtree(self.path())

    def path(self):
        """Return the complete path of the mapset"""
        return join(self.gisdbase, self.location, self.name)


class VisibleMapset(object):
    """VisibleMapset object
    """

    def __init__(self, mapset, location='', gisdbase=''):
        self.mapset = mapset
        self.location = Location(location, gisdbase)
        self._list = []
        self.spath = join(self.location.path(), self.mapset, 'SEARCH_PATH')

    def __repr__(self):
        return repr(self.read())

    def __iter__(self):
        for mapset in self.read():
            yield mapset

    def read(self):
        """Return the mapsets in the search path"""
        with open(self.spath, "ab+") as f:
            lines = f.readlines()
            if lines:
                return [decode(l.strip()) for l in lines]
        lns = [u'PERMANENT', ]
        self._write(lns)
        return lns

    def _write(self, mapsets):
        """Write to SEARCH_PATH file the changes in the search path

        :param mapsets: a list of mapset's names
        :type mapsets: list
        """
        with open(self.spath, "wb+") as f:
            ms = [decode(m) for m in self.location.mapsets()]
            f.write(b'\n'.join([encode(m) for m in mapsets if m in ms]))

    def add(self, mapset):
        """Add a mapset to the search path

        :param mapset: a mapset's name
        :type mapset: str
        """
        if mapset not in self.read() and mapset in self.location:
            with open(self.spath, "a+") as f:
                f.write('\n%s' % mapset)
        else:
            raise TypeError('Mapset not found')

    def remove(self, mapset):
        """Remove mapset to the search path

        :param mapset: a mapset's name
        :type mapset: str
        """
        mapsets = self.read()
        mapsets.remove(mapset)
        self._write(mapsets)

    def extend(self, mapsets):
        """Add more mapsets to the search path

        :param mapsets: a list of mapset's names
        :type mapsets: list
        """
        ms = [decode(m) for m in self.location.mapsets()]
        final = [decode(m) for m in self.read()]
        mapsets = [decode(m) for m in mapsets]
        final.extend([m for m in mapsets if m in ms and m not in final])
        self._write(final)

    def reset(self):
        """Reset to the original search path"""
        final = [self.mapset, 'PERMANENT']
        self._write(final)


if __name__ == "__main__":
    import doctest
    from grass.pygrass import utils
    from grass.script.core import run_command

    utils.create_test_vector_map(test_vector_name)
    run_command("g.region", n=50, s=0, e=60, w=0, res=1)
    run_command("r.mapcalc", expression="%s = 1" % (test_raster_name),
                overwrite=True)
    run_command("g.region", n=40, s=0, e=40, w=0, res=2)

    doctest.testmod()

    # Remove the generated vector map, if exist
    mset = utils.get_mapset_vector(test_vector_name, mapset='')
    if mset:
        run_command("g.remove", flags='f', type='vector',
                    name=test_vector_name)
    mset = utils.get_mapset_raster(test_raster_name, mapset='')
    if mset:
        run_command("g.remove", flags='f', type='raster',
                    name=test_raster_name)

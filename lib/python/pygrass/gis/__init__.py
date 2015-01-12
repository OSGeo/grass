# -*- coding: utf-8 -*-
#!/usr/bin/env python2.7

from __future__ import (nested_scopes, generators, division, absolute_import,
                        with_statement, print_function, unicode_literals)
from os import listdir
from os.path import join, isdir
import shutil
import ctypes as ct
import fnmatch


import grass.lib.gis as libgis
libgis.G_gisinit('')
from grass.pygrass.errors import GrassError


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


def _check(value, path, type):
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
    if value and CHECK_IS[type](join(path, value)):
        return value
    elif value is '':
        from grass.pygrass.utils import getenv
        return getenv(type)
    else:
        raise GrassError("%s <%s> not found" % (type.title(),
                                                join(path, value)))


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
        self._name = _check(name, '', "GISDBASE")

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
                       if libgis.G_is_location(join(self.name, loc))])


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
        self._gisdb = _check(gisdb, '', "GISDBASE")

    gisdbase = property(fget=_get_gisdb, fset=_set_gisdb,
                        doc="Set or obtain the name of GISDBASE")

    def _get_name(self):
        return self._name

    def _set_name(self, name):
        self._name = _check(name, self._gisdb, "LOCATION_NAME")

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
                if (isdir(join(lpath, m)) and _check(m, lpath, "MAPSET")))

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
            >>> sorted(location.mapsets())
            ['PERMANENT', 'user1']

        """
        mapsets = [mapset for mapset in self]
        if permissions:
            mapsets = [mapset for mapset in mapsets
                       if libgis.G_mapset_permissions(mapset)]
        if pattern:
            return fnmatch.filter(mapsets, pattern)
        return mapsets

    def path(self):
        """Return the complete path of the location"""
        return join(self.gisdbase, self.name)


class Mapset(object):
    """Mapset ::

        >>> mapset = Mapset()
        >>> mapset
        Mapset('user1')
        >>> mapset.gisdbase                               # doctest: +ELLIPSIS
        '/home/...'
        >>> mapset.location
        'nc_basic_spm_grass7'
        >>> mapset.name
        'user1'

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
        self._gisdb = _check(gisdb, '', "GISDBASE")

    gisdbase = property(fget=_get_gisdb, fset=_set_gisdb,
                        doc="Set or obtain the name of GISDBASE")

    def _get_loc(self):
        return self._loc

    def _set_loc(self, loc):
        self._loc = _check(loc, self._gisdb, "LOCATION_NAME")

    location = property(fget=_get_loc, fset=_set_loc,
                        doc="Set or obtain the name of LOCATION")

    def _get_name(self):
        return self._name

    def _set_name(self, name):
        self._name = _check(name, join(self._gisdb, self._loc), "MAPSET")

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

            >>> mapset = Mapset('PERMANENT')
            >>> rast = mapset.glist('rast')
            >>> rast.sort()
            >>> rast                                      # doctest: +ELLIPSIS
            ['basins', 'elevation', ...]
            >>> sorted(mapset.glist('rast', pattern='el*'))
            ['elevation', 'elevation_shade']

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
                elist.append(el_name)
            else:
                if pattern:
                    return fnmatch.filter(elist, pattern)
                return elist

    def is_current(self):
        """Check if the MAPSET is the working MAPSET"""
        return (self.name == libgis.G_getenv('MAPSET') and
                self.location == libgis.G_getenv('LOCATION_NAME') and
                self.gisdbase == libgis.G_getenv('GISDBASE'))

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
    """VisibleMapset object::

        >>> mapset = VisibleMapset('user1')
        >>> mapset
        ['user1', 'PERMANENT']

    ..
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
        with open(self.spath, "a+") as f:
            lines = f.readlines()
            if lines:
                return [l.strip() for l in lines]
        lns = ['PERMANENT', ]
        self.write(lns)
        return lns

    def _write(self, mapsets):
        """Write to SEARCH_PATH file the changes in the search path

        :param mapsets: a list of mapset's names
        :type mapsets: list
        """
        with open(self.spath, "w+") as f:
            ms = self.location.mapsets()
            f.write('%s' % '\n'.join([m for m in mapsets if m in ms]))

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
        ms = self.location.mapsets()
        final = self.read()
        final.extend([m for m in mapsets if m in ms and m not in final])
        self._write(final)

    def reset(self):
        """Reset to the original search path"""
        final = [self.mapset, 'PERMANENT']
        self._write(final)

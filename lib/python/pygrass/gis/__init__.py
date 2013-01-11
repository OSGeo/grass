# -*- coding: utf-8 -*-
#!/usr/bin/env python2.7
from __future__ import print_function
#import os
from os import listdir
from os.path import join
import ctypes as ct
import fnmatch

from grass import script
#from grass.script import setup
#
#
#GISBASE = "/home/pietro/docdat/src/gis/grass/grass70/dist.x86_64-unknown-linux-gnu"
#LOCATION = "nc_basic_spm_grass7'"
#GISDBASE = "/home/pietro/docdat/gis"
#MAPSET = "sqlite"
#GUI = "wxpython"
#
#setup.init(GISBASE, GISDBASE, LOCATION, MAPSET)
script.gisenv()

import grass.lib.gis as libgis
from pygrass.functions import getenv
from pygrass.errors import GrassError


#write dec to check if user have permissions or not

ETYPE = {'rast': libgis.G_ELEMENT_RASTER,
         'rast3d': libgis.G_ELEMENT_RASTER3D,
         'vect': libgis.G_ELEMENT_VECTOR,
         'oldvect': libgis.G_ELEMENT_OLDVECTOR,
         'asciivect': libgis.G_ELEMENT_ASCIIVECTOR,
         'icon': libgis.G_ELEMENT_ICON,
         'labels': libgis.G_ELEMENT_LABEL,
         'sites': libgis.G_ELEMENT_SITE,
         'region': libgis.G_ELEMENT_REGION,
         'region3d': libgis.G_ELEMENT_REGION3D,
         'group': libgis.G_ELEMENT_GROUP,
         'view3d': libgis.G_ELEMENT_3DVIEW}


CHECK_IS = {"GISBASE": libgis.G_is_gisbase,
            "GISDBASE": lambda x: True,
            "LOCATION_NAME": libgis.G_is_location,
            "MAPSET": libgis.G_is_mapset}


def _check(value, path, type):
    #import pdb; pdb.set_trace()
    if value and CHECK_IS[type](join(path, value)):
        return value
    elif value is '':
        return getenv(type)
    else:
        raise GrassError("%s <%s> not found." % (type.title(),
                                                 join(path, value)))


class Gisdbase(object):
    """Return Gisdbase object. ::

        >>> gisdbase = Gisdbase()
        >>> gisdbase.name          # doctest: +ELLIPSIS, +NORMALIZE_WHITESPACE
        '/home/...'


    """
    def __init__(self, gisdbase=''):
        self.name = gisdbase

    def _get_name(self):
        return self._name

    def _set_name(self, name):
        self._name = _check(name, '', "GISDBASE")

    name = property(fget=_get_name, fset=_set_name)

    def __str__(self):
        return self.name

    def __repr__(self):
        return 'Gisdbase(%s)' % self.name

    def __getitem__(self, location):
        """Return a Location object. ::

            >>> gisdbase = Gisdbase()
            >>> gisdbase['nc_basic_spm_grass7']
            Location('nc_basic_spm_grass7')

        ..
        """
        if location in self.locations():
            return Location(location, self.name)
        else:
            raise KeyError('Location: %s does not exist' % location)

    def __iter__(self):
        for loc in self.locations():
            yield Location(loc, self.name)

    def new_location(self):
        if libgis.G__make_location() != 0:
            raise GrassError("I cannot create a new mapset.")

    def locations(self):
        """Return a list of locations that are available in the gisdbase: ::

            >>> gisdbase = Gisdbase()
            >>> gisdbase.locations()                     # doctest: +ELLIPSIS
            [...]

        ..
        """
        locations = []
        for loc in listdir(self.name):
            if libgis.G_is_location(join(self.name, loc)):
                locations.append(loc)
        locations.sort()
        return locations


class Location(object):
    """Location object ::

        >>> location = Location()
        >>> location                                      # doctest: +ELLIPSIS
        Location(...)
        >>> location.gisdbase                              # doctest: +ELLIPSIS
        '/home/...'
        >>> location.name
        'nc_basic_spm_grass7'
    """
    def __init__(self, location='', gisdbase=''):
        self.gisdbase = gisdbase
        self.name = location

    def _get_gisdb(self):
        return self._gisdb

    def _set_gisdb(self, gisdb):
        self._gisdb = _check(gisdb, '', "GISDBASE")

    gisdbase = property(fget=_get_gisdb, fset=_set_gisdb)

    def _get_name(self):
        return self._name

    def _set_name(self, name):
        self._name = _check(name, self._gisdb, "LOCATION_NAME")

    name = property(fget=_get_name, fset=_set_name)

    def __getitem__(self, mapset):
        if mapset in self.mapsets():
            return Mapset(mapset)
        else:
            raise KeyError('Mapset: %s does not exist' % mapset)

    def __iter__(self):
        for mapset in libgis.G_available_mapsets():
            mapset_name = ct.cast(mapset, ct.c_char_p).value
            if mapset_name and libgis.G__mapset_permissions(mapset):
                yield mapset_name
            else:
                break

    def __len__(self):
        return len(self.mapsets())

    def __str__(self):
        return self.name

    def __repr__(self):
        return 'Location(%r)' % self.name

    def mapsets(self):
        """Return a list of the available mapsets. ::

            >>> location = Location()
            >>> location.mapsets()
            ['PERMANENT', 'user1']
        """
        return [mapset for mapset in self]

    def new_mapset(self, mapset):
        if libgis.G__make_mapset(self.gisdbase, self.location, mapset) != 0:
            raise GrassError("I cannot create a new mapset.")


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
    """
    def __init__(self, mapset='', location='', gisdbase=''):
        self.gisdbase = gisdbase
        self.location = location
        self.name = mapset

    def _get_gisdb(self):
        return self._gisdb

    def _set_gisdb(self, gisdb):
        self._gisdb = _check(gisdb, '', "GISDBASE")

    gisdbase = property(fget=_get_gisdb, fset=_set_gisdb)

    def _get_loc(self):
        return self._loc

    def _set_loc(self, loc):
        self._loc = _check(loc, self._gisdb, "LOCATION_NAME")

    location = property(fget=_get_loc, fset=_set_loc)

    def _get_name(self):
        return self._name

    def _set_name(self, name):
        self._name = _check(name, join(self._gisdb, self._loc), "MAPSET")

    name = property(fget=_get_name, fset=_set_name)

    def __str__(self):
        return self.name

    def __repr__(self):
        return 'Mapset(%r)' % self.name

    def glist(self, type, pattern=None):
        """Return a list of grass types like:

            * 'asciivect',
            * 'group',
            * 'icon',
            * 'labels',
            * 'oldvect',
            * 'rast',
            * 'rast3d',
            * 'region',
            * 'region3d',
            * 'sites',
            * 'vect',
            * 'view3d'

        ::

            >>> mapset = Mapset('PERMANENT')
            >>> rast = mapset.glist('rast')
            >>> rast.sort()
            >>> rast                                      # doctest: +ELLIPSIS
            ['basins', 'elevation', ...]
            >>> mapset.glist('rast', pattern='el*')
            ['elevation_shade', 'elevation']
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


if __name__ == "__main__":
    import doctest
    doctest.testmod(verbose=False)
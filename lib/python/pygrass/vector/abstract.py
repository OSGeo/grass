# -*- coding: utf-8 -*-
"""
Created on Fri Aug 17 17:24:03 2012

@author: pietro
"""
import ctypes
import datetime
import grass.lib.vector as libvect
from vector_type import MAPTYPE

from pygrass import functions
from pygrass.errors import GrassError, OpenError, must_be_open
from table import DBlinks

#=============================================
# VECTOR ABSTRACT CLASS
#=============================================


class Info(object):
    """Basic vector info.
    To get access to the vector info the map must be opened. ::

        >>> municip = Info('boundary_municp', 'PERMANENT')
        >>> municip.full_name
        You must open the map.

        >>> municip.open()

    Then it is possible to read and write the following map attributes: ::

        >>> municip.organization
        'NC OneMap'
        >>> municip.person
        'helena'
        >>> municip.title
        'North Carolina municipality boundaries (polygon map)'
        >>> municip.map_date
        datetime.datetime(2006, 11, 7, 0, 1, 27)
        >>> municip.date
        ''
        >>> municip.scale
        1
        >>> municip.comment
        ''
        >>> municip.comment = "One useful comment!"
        >>> municip.comment
        'One useful comment!'
        >>> municip.zone
        0
        >>> municip.proj
        99

    There are some read only attributes: ::

        >>> municip.full_name
        'boundary_municp@PERMANENT'
        >>> municip.proj_name
        'Lambert Conformal Conic'
        >>> municip.maptype
        'native'

    And some basic methods: ::

        >>> municip.is_3D()
        False
        >>> municip.exist()
        True
        >>> municip.is_open()
        True
        >>> municip.close()

    """
    def __init__(self, name, mapset='', link_id=None):
        # Set map name and mapset
        self._name = name
        self.mapset = mapset
        self.c_mapinfo = ctypes.pointer(libvect.Map_info())
        self._topo_level = 1
        self._class_name = 'Vector'
        self.overwrite = False
        self.date_fmt = '%a %b  %d %H:%M:%S %Y'
        self.link_id = link_id

    def _get_name(self):
        if self.exist() and self.is_open():
            return libvect.Vect_get_name(self.c_mapinfo)
        else:
            return self._name

    def _set_name(self, newname):
        self.rename(newname)

    name = property(fget=_get_name, fset=_set_name)

#    @property
#    def mapset(self):
#        return libvect.Vect_get_mapset(self.c_mapinfo)

    def _get_organization(self):
        return libvect.Vect_get_organization(self.c_mapinfo)

    def _set_organization(self, org):
        libvect.Vect_get_organization(self.c_mapinfo, ctypes.c_char_p(org))

    organization = property(fget=_get_organization, fset=_set_organization)

    def _get_date(self):
        return libvect.Vect_get_date(self.c_mapinfo)

    def _set_date(self, date):
        return libvect.Vect_set_date(self.c_mapinfo, ctypes.c_char_p(date))

    date = property(fget=_get_date, fset=_set_date)

    def _get_person(self):
        return libvect.Vect_get_person(self.c_mapinfo)

    def _set_person(self, person):
        libvect.Vect_set_person(self.c_mapinfo, ctypes.c_char_p(person))

    person = property(fget=_get_person, fset=_set_person)

    def _get_title(self):
        return libvect.Vect_get_map_name(self.c_mapinfo)

    def _set_title(self, title):
        libvect.Vect_set_map_name(self.c_mapinfo, ctypes.c_char_p(title))

    title = property(fget=_get_title, fset=_set_title)

    def _get_map_date(self):
        date_str = libvect.Vect_get_map_date(self.c_mapinfo)
        return datetime.datetime.strptime(date_str, self.date_fmt)

    def _set_map_date(self, datetimeobj):
        date_str = datetimeobj.strftime(self.date_fmt)
        libvect.Vect_set_map_date(self.c_mapinfo, ctypes.c_char_p(date_str))

    map_date = property(fget=_get_map_date, fset=_set_map_date)

    def _get_scale(self):
        return libvect.Vect_get_scale(self.c_mapinfo)

    def _set_scale(self, scale):
        return libvect.Vect_set_scale(self.c_mapinfo, ctypes.c_int(scale))

    scale = property(fget=_get_scale, fset=_set_scale)

    def _get_comment(self):
        return libvect.Vect_get_comment(self.c_mapinfo)

    def _set_comment(self, comm):
        return libvect.Vect_set_comment(self.c_mapinfo, ctypes.c_char_p(comm))

    comment = property(fget=_get_comment, fset=_set_comment)

    def _get_zone(self):
        return libvect.Vect_get_zone(self.c_mapinfo)

    def _set_zone(self, zone):
        return libvect.Vect_set_zone(self.c_mapinfo, ctypes.c_int(zone))

    zone = property(fget=_get_zone, fset=_set_zone)

    def _get_proj(self):
        return libvect.Vect_get_proj(self.c_mapinfo)

    def _set_proj(self, proj):
        libvect.Vect_set_proj(self.c_mapinfo, ctypes.c_int(proj))

    proj = property(fget=_get_proj, fset=_set_proj)

    def _get_thresh(self):
        return libvect.Vect_get_thresh(self.c_mapinfo)

    def _set_thresh(self, thresh):
        return libvect.Vect_set_thresh(self.c_mapinfo, ctypes.c_double(thresh))

    thresh = property(fget=_get_thresh, fset=_set_thresh)

    @property
    @must_be_open
    def full_name(self):
        return libvect.Vect_get_full_name(self.c_mapinfo)

    @property
    @must_be_open
    def maptype(self):
        return MAPTYPE[libvect.Vect_maptype(self.c_mapinfo)]

    @property
    @must_be_open
    def proj_name(self):
        return libvect.Vect_get_proj_name(self.c_mapinfo)

    def _write_header(self):
        libvect.Vect_write_header(self.c_mapinfo)

    def rename(self, newname):
        """Rename the map"""
        if self.exist():
            functions.rename(self.name, newname, 'vect')
        self._name = newname

    def is_3D(self):
        return bool(libvect.Vect_is_3d(self.c_mapinfo))

    def exist(self):
        if self._name:
            self.mapset = functions.get_mapset_vector(self._name, self.mapset)
        else:
            return False
        if self.mapset:
            return True
        else:
            return False

    def is_open(self):
        return (self.c_mapinfo.contents.open != 0 and
                self.c_mapinfo.contents.open != libvect.VECT_CLOSED_CODE)

    def open(self, mode='r', layer='0', overwrite=None):
        """::

            >>> mun = Info('boundary_municp_sqlite')
            >>> mun.open()
            >>> mun.is_open()
            True
            >>> mun.close()

        ..
        """
        # check if map exists or not
        if not self.exist() and mode != 'w':
            raise OpenError("Map <%s> not found." % self._name)
        if libvect.Vect_set_open_level(self._topo_level) != 0:
            raise OpenError("Invalid access level.")
        # update the overwrite attribute
        self.overwrite = overwrite if overwrite is not None else self.overwrite
        # check if the mode is valid
        if mode not in ('r', 'rw', 'w'):
            raise ValueError("Mode not supported. Use one of: 'r', 'rw', 'w'.")
        # check if the map exist
        if self.exist() and mode == 'r':
            openvect = libvect.Vect_open_old2(self.c_mapinfo, self.name,
                                              self.mapset, layer)
        # If it is opened in write mode
        if mode == 'w':
            #TODO: build topo if new
            openvect = libvect.Vect_open_new(self.c_mapinfo, self.name,
                                             libvect.WITHOUT_Z)
        elif mode == 'rw':
            openvect = libvect.Vect_open_update2(self.c_mapinfo, self.name,
                                                 self.mapset, layer)
        # initialize the dblinks object
        self.dblinks = DBlinks(self.c_mapinfo)
        # check the C function result.
        if openvect == -1:
            str_err = "Not able to open the map, C function return %d."
            raise OpenError(str_err % openvect)
        # istantiate the table
        self.table = self.get_table(link_id=self.link_id)
        self.writable = self.mapset == functions.getenv("MAPSET")

    def get_table(self, link_id=None, link_name=None,):
        if link_id is None and link_name is None and len(self.dblinks) == 0:
            return None
        if link_id is not None:
            return self.dblinks.by_number(link_id).table()
        elif link_name is not None:
            return self.dblinks.by_name(link_name).table()
        else:
            return self.dblinks.by_number(1).table()

    def close(self):
        if self.is_open():
            if libvect.Vect_close(self.c_mapinfo) != 0:
                str_err = 'Error when trying to close the map with Vect_close'
                raise GrassError(str_err)

    def remove(self):
        """Remove vector map"""
        if self.is_open():
            self.close()
        functions.remove(vect=self.name)

    def build(self):
        """Close the vector map and build vector Topology"""
        self.close()
        libvect.Vect_set_open_level(1)
        if libvect.Vect_open_old2(self.c_mapinfo, self.name,
                                  self.mapset, '0') != 1:
            str_err = 'Error when trying to open the vector map.'
            raise GrassError(str_err)
        # Vect_build returns 1 on success and 0 on error (bool approach)
        if libvect.Vect_build(self.c_mapinfo) != 1:
            str_err = 'Error when trying build topology with Vect_build'
            raise GrassError(str_err)
        libvect.Vect_close(self.c_mapinfo)
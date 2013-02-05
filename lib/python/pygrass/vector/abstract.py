# -*- coding: utf-8 -*-
"""
Created on Fri Aug 17 17:24:03 2012

@author: pietro
"""
import ctypes
import datetime
import grass.lib.vector as libvect
from vector_type import MAPTYPE

from grass.pygrass import functions
from grass.pygrass.errors import GrassError, OpenError, must_be_open
from table import DBlinks, Link

#=============================================
# VECTOR ABSTRACT CLASS
#=============================================


class Info(object):
    """Basic vector info.
    To get access to the vector info the map must be opened. ::

        >>> cens = Info('census')
        >>> cens.open()

    Then it is possible to read and write the following map attributes: ::

        >>> cens.organization
        'NC OneMap'
        >>> cens.person
        'hmitaso'
        >>> cens.title
        'Wake County census blocks with attributes, clipped (polygon map)'
        >>> cens.map_date
        datetime.datetime(2007, 3, 19, 22, 1, 37)
        >>> cens.date
        ''
        >>> cens.scale
        1
        >>> cens.comment
        ''
        >>> cens.comment = "One useful comment!"
        >>> cens.comment
        'One useful comment!'
        >>> cens.zone
        0
        >>> cens.proj
        99

    There are some read only attributes: ::

        >>> cens.full_name
        'census@PERMANENT'
        >>> cens.proj_name
        'Lambert Conformal Conic'
        >>> cens.maptype
        'native'

    And some basic methods: ::

        >>> cens.is_3D()
        False
        >>> cens.exist()
        True
        >>> cens.is_open()
        True
        >>> cens.close()

    """
    def __init__(self, name, mapset='', layer=None):
        # Set map name and mapset
        self._name = name
        self.mapset = mapset
        self.c_mapinfo = ctypes.pointer(libvect.Map_info())
        self._topo_level = 1
        self._class_name = 'Vector'
        self.overwrite = False
        self.date_fmt = '%a %b  %d %H:%M:%S %Y'
        self.layer = layer

    def _get_name(self):
        """Private method to obtain the Vector name"""
        if self.exist() and self.is_open():
            return libvect.Vect_get_name(self.c_mapinfo)
        else:
            return self._name

    def _set_name(self, newname):
        """Private method to change the Vector name"""
        if not functions.is_clean_name(newname):
            str_err = _("Map name {0} not valid")
            raise ValueError(str_err.format(newname))
        if self.exist():
            self.rename(newname)
        self._name = newname

    name = property(fget=_get_name, fset=_set_name)

#    @property
#    def mapset(self):
#        return libvect.Vect_get_mapset(self.c_mapinfo)

    def _get_organization(self):
        """Private method to obtain the Vector organization"""
        return libvect.Vect_get_organization(self.c_mapinfo)

    def _set_organization(self, org):
        """Private method to change the Vector organization"""
        libvect.Vect_get_organization(self.c_mapinfo, ctypes.c_char_p(org))

    organization = property(fget=_get_organization, fset=_set_organization)

    def _get_date(self):
        """Private method to obtain the Vector date"""
        return libvect.Vect_get_date(self.c_mapinfo)

    def _set_date(self, date):
        """Private method to change the Vector date"""
        return libvect.Vect_set_date(self.c_mapinfo, ctypes.c_char_p(date))

    date = property(fget=_get_date, fset=_set_date)

    def _get_person(self):
        """Private method to obtain the Vector person"""
        return libvect.Vect_get_person(self.c_mapinfo)

    def _set_person(self, person):
        """Private method to change the Vector person"""
        libvect.Vect_set_person(self.c_mapinfo, ctypes.c_char_p(person))

    person = property(fget=_get_person, fset=_set_person)

    def _get_title(self):
        """Private method to obtain the Vector title"""
        return libvect.Vect_get_map_name(self.c_mapinfo)

    def _set_title(self, title):
        """Private method to change the Vector title"""
        libvect.Vect_set_map_name(self.c_mapinfo, ctypes.c_char_p(title))

    title = property(fget=_get_title, fset=_set_title)

    def _get_map_date(self):
        """Private method to obtain the Vector map date"""
        date_str = libvect.Vect_get_map_date(self.c_mapinfo)
        return datetime.datetime.strptime(date_str, self.date_fmt)

    def _set_map_date(self, datetimeobj):
        """Private method to change the Vector map date"""
        date_str = datetimeobj.strftime(self.date_fmt)
        libvect.Vect_set_map_date(self.c_mapinfo, ctypes.c_char_p(date_str))

    map_date = property(fget=_get_map_date, fset=_set_map_date)

    def _get_scale(self):
        """Private method to obtain the Vector scale"""
        return libvect.Vect_get_scale(self.c_mapinfo)

    def _set_scale(self, scale):
        """Private method to set the Vector scale"""
        return libvect.Vect_set_scale(self.c_mapinfo, ctypes.c_int(scale))

    scale = property(fget=_get_scale, fset=_set_scale)

    def _get_comment(self):
        """Private method to obtain the Vector comment"""
        return libvect.Vect_get_comment(self.c_mapinfo)

    def _set_comment(self, comm):
        """Private method to set the Vector comment"""
        return libvect.Vect_set_comment(self.c_mapinfo, ctypes.c_char_p(comm))

    comment = property(fget=_get_comment, fset=_set_comment)

    def _get_zone(self):
        """Private method to obtain the Vector projection zone"""
        return libvect.Vect_get_zone(self.c_mapinfo)

    def _set_zone(self, zone):
        """Private method to set the Vector projection zone"""
        return libvect.Vect_set_zone(self.c_mapinfo, ctypes.c_int(zone))

    zone = property(fget=_get_zone, fset=_set_zone)

    def _get_proj(self):
        """Private method to obtain the Vector projection code"""
        return libvect.Vect_get_proj(self.c_mapinfo)

    def _set_proj(self, proj):
        """Private method to set the Vector projection code"""
        libvect.Vect_set_proj(self.c_mapinfo, ctypes.c_int(proj))

    proj = property(fget=_get_proj, fset=_set_proj)

    def _get_thresh(self):
        """Private method to obtain the Vector threshold"""
        return libvect.Vect_get_thresh(self.c_mapinfo)

    def _set_thresh(self, thresh):
        """Private method to set the Vector threshold"""
        return libvect.Vect_set_thresh(self.c_mapinfo, ctypes.c_double(thresh))

    thresh = property(fget=_get_thresh, fset=_set_thresh)

    @property
    @must_be_open
    def full_name(self):
        """Return the full name of Vector"""
        return libvect.Vect_get_full_name(self.c_mapinfo)

    @property
    @must_be_open
    def maptype(self):
        """Return the map type of Vector"""
        return MAPTYPE[libvect.Vect_maptype(self.c_mapinfo)]

    @property
    @must_be_open
    def proj_name(self):
        """Return the project name of Vector"""
        return libvect.Vect_get_proj_name(self.c_mapinfo)

    def _write_header(self):
        libvect.Vect_write_header(self.c_mapinfo)

    def rename(self, newname):
        """Method to rename the Vector map"""
        if self.exist():
            functions.rename(self.name, newname, 'vect')
        self._name = newname

    def is_3D(self):
        """Return if the Vector is 3D"""
        return bool(libvect.Vect_is_3d(self.c_mapinfo))

    def exist(self):
        """Return if the Vector exists or not"""
        if self._name:
            self.mapset = functions.get_mapset_vector(self._name, self.mapset)
        else:
            return False
        if self.mapset:
            return True
        else:
            return False

    def is_open(self):
        """Return if the Vector is open"""
        return (self.c_mapinfo.contents.open != 0 and
                self.c_mapinfo.contents.open != libvect.VECT_CLOSED_CODE)

    def open(self, mode='r', layer=1, overwrite=None,
             # parameters valid only if mode == 'w'
             tab_name='', tab_cols=None, link_name=None, link_key='cat',
             link_db='$GISDBASE/$LOCATION_NAME/$MAPSET/sqlite/sqlite.db',
             link_driver='sqlite'):
        """Open a Vector map.

         Parameters
        ----------
        mode : string
            Open a vector map in ``r`` in reading, ``w`` in writing and
            in ``rw`` read and write mode
        layer: int, optional
            Specify the layer that you want to use

        Some parameters are valid only if we open use the writing mode (``w``)

        overwrite: bool, optional
            valid only for ``w`` mode
        tab_name: string, optional
            Define the name of the table that will be generate
        tab_cols: list of pairs, optional
            Define the name and type of the columns of the attribute table
            of the vecto map
        link_name: string, optional
            Define the name of the link connecttion with the database
        link_key: string, optional
            Define the nema of the column that will be use as vector category
        link_db: string, optional
            Define the database connection parameters
        link_driver: string, optional
            Define witch database driver will be used

        See more examples in the documentation of the ``read`` and ``write``
        methods.
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
        if self.exist() and mode in ('r', 'rw'):
            # open in READ mode
            if mode == 'r':
                openvect = libvect.Vect_open_old2(self.c_mapinfo, self.name,
                                                  self.mapset, str(layer))
            # open in READ and WRITE mode
            elif mode == 'rw':
                openvect = libvect.Vect_open_update2(self.c_mapinfo, self.name,
                                                     self.mapset, str(layer))

            # instantiate class attributes
            self.dblinks = DBlinks(self.c_mapinfo)

        # If it is opened in write mode
        if mode == 'w':
            openvect = libvect.Vect_open_new(self.c_mapinfo, self.name,
                                             libvect.WITHOUT_Z)
            self.dblinks = DBlinks(self.c_mapinfo)
            if tab_cols:
                # create a link
                link = Link(layer,
                            link_name if link_name else self.name,
                            tab_name if tab_name else self.name,
                            link_key, link_db, link_driver)
                # add the new link
                self.dblinks.add(link)
                # create the table
                table = self.get_table()
                table.columns.create(tab_cols)
                table.conn.commit()

        # check the C function result.
        if openvect == -1:
            str_err = "Not able to open the map, C function return %d."
            raise OpenError(str_err % openvect)

        if len(self.dblinks) == 0:
            self.layer = layer
            self.table = None
            self.n_lines = 0
        else:
            self.layer = self.dblinks.by_layer(layer)
            self.table = self.dblinks.by_layer(layer).table()
            self.n_lines = self.table.n_rows()
        self.writable = self.mapset == functions.getenv("MAPSET")

    def close(self):
        """Method to close the Vector"""
        if hasattr(self, 'table') and self.table is not None:
            self.table.conn.close()
        if self.is_open():
            if libvect.Vect_close(self.c_mapinfo) != 0:
                str_err = 'Error when trying to close the map with Vect_close'
                raise GrassError(str_err)
            if (self.c_mapinfo.contents.mode == libvect.GV_MODE_RW or
                    self.c_mapinfo.contents.mode == libvect.GV_MODE_WRITE):
                self.build()

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

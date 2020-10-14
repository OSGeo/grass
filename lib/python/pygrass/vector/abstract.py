# -*- coding: utf-8 -*-
"""
Created on Fri Aug 17 17:24:03 2012

@author: pietro
"""
import ctypes
import datetime
import grass.lib.vector as libvect
from grass.pygrass.vector.vector_type import MAPTYPE

from grass.pygrass import utils
from grass.pygrass.errors import GrassError, OpenError, must_be_open
from grass.pygrass.vector.table import DBlinks, Link
from grass.pygrass.vector.find import PointFinder, BboxFinder, PolygonFinder

test_vector_name = "abstract_doctest_map"

def is_open(c_mapinfo):
    """Return if the Vector is open"""
    return (c_mapinfo.contents.open != 0 and
            c_mapinfo.contents.open != libvect.VECT_CLOSED_CODE)


#=============================================
# VECTOR ABSTRACT CLASS
#=============================================


class Info(object):
    """Basic vector info.
    To get access to the vector info the map must be opened. ::

        >>> test_vect = Info(test_vector_name)
        >>> test_vect.open(mode='r')

    Then it is possible to read and write the following map attributes: ::

        >>> test_vect.organization
        'Thuenen Institut'
        >>> test_vect.person
        'Soeren Gebbert'
        >>> test_vect.title
        'Test dataset'
        >>> test_vect.scale
        1
        >>> test_vect.comment
        'This is a comment'
        >>> test_vect.comment = "One useful comment!"
        >>> test_vect.comment
        'One useful comment!'

    There are some read only attributes: ::

        >>> test_vect.maptype
        'native'

    And some basic methods: ::

        >>> test_vect.is_3D()
        False
        >>> test_vect.exist()
        True
        >>> test_vect.is_open()
        True
        >>> test_vect.close()

    """

    def __init__(self, name, mapset='', *aopen, **kwopen):
        self._name = ''
        self._mapset = ''
        # Set map name and mapset
        self.name = name
        self.mapset = mapset
        self._aopen = aopen
        self._kwopen = kwopen
        self.c_mapinfo = ctypes.pointer(libvect.Map_info())
        self._topo_level = 1
        self._class_name = 'Vector'
        self._mode = 'r'
        self.overwrite = False
        self.date_fmt = '%a %b  %d %H:%M:%S %Y'

    def __enter__(self):
        self.open(*self._aopen, **self._kwopen)
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.close()

    def _get_mode(self):
        return self._mode

    def _set_mode(self, mode):
        if mode.upper() not in 'RW':
            str_err = _("Mode type: {0} not supported ('r', 'w')")
            raise ValueError(str_err.format(mode))
        self._mode = mode

    mode = property(fget=_get_mode, fset=_set_mode)

    def _get_name(self):
        """Private method to obtain the Vector name"""
        return self._name

    def _set_name(self, newname):
        """Private method to change the Vector name"""
        if not utils.is_clean_name(newname):
            str_err = _("Map name {0} not valid")
            raise ValueError(str_err.format(newname))
        self._name = newname

    name = property(fget=_get_name, fset=_set_name,
                    doc="Set or obtain the Vector name")

    def _get_mapset(self):
        """Private method to obtain the Vector mapset"""
        return self._mapset

    def _set_mapset(self, mapset):
        """Private method to change the Vector mapset"""
        if mapset:
            self._mapset = mapset

    mapset = property(fget=_get_mapset, fset=_set_mapset,
                      doc="Set or obtain the Vector mapset")

    def _get_organization(self):
        """Private method to obtain the Vector organization"""
        return utils.decode(libvect.Vect_get_organization(self.c_mapinfo))

    def _set_organization(self, org):
        """Private method to change the Vector organization"""
        libvect.Vect_set_organization(self.c_mapinfo, org)

    organization = property(fget=_get_organization, fset=_set_organization,
                            doc="Set or obtain the Vector organization")

    def _get_date(self):
        """Private method to obtain the Vector date"""
        return utils.decode(libvect.Vect_get_date(self.c_mapinfo))

    def _set_date(self, date):
        """Private method to change the Vector date"""
        return libvect.Vect_set_date(self.c_mapinfo, date)

    date = property(fget=_get_date, fset=_set_date,
                    doc="Set or obtain the Vector date")

    def _get_person(self):
        """Private method to obtain the Vector person"""
        return utils.decode(libvect.Vect_get_person(self.c_mapinfo))

    def _set_person(self, person):
        """Private method to change the Vector person"""
        libvect.Vect_set_person(self.c_mapinfo, person)

    person = property(fget=_get_person, fset=_set_person,
                      doc="Set or obtain the Vector author")

    def _get_title(self):
        """Private method to obtain the Vector title"""
        return utils.decode(libvect.Vect_get_map_name(self.c_mapinfo))

    def _set_title(self, title):
        """Private method to change the Vector title"""
        libvect.Vect_set_map_name(self.c_mapinfo, title)

    title = property(fget=_get_title, fset=_set_title,
                     doc="Set or obtain the Vector title")

    def _get_map_date(self):
        """Private method to obtain the Vector map date"""
        date_str = utils.decode(libvect.Vect_get_map_date(self.c_mapinfo))
        try:
            return datetime.datetime.strptime(date_str, self.date_fmt)
        except:
            return date_str

    def _set_map_date(self, datetimeobj):
        """Private method to change the Vector map date"""
        date_str = datetimeobj.strftime(self.date_fmt)
        libvect.Vect_set_map_date(self.c_mapinfo, date_str)

    map_date = property(fget=_get_map_date, fset=_set_map_date,
                        doc="Set or obtain the Vector map date")

    def _get_scale(self):
        """Private method to obtain the Vector scale"""
        return libvect.Vect_get_scale(self.c_mapinfo)

    def _set_scale(self, scale):
        """Private method to set the Vector scale"""
        return libvect.Vect_set_scale(self.c_mapinfo, ctypes.c_int(scale))

    scale = property(fget=_get_scale, fset=_set_scale,
                     doc="Set or obtain the Vector scale")

    def _get_comment(self):
        """Private method to obtain the Vector comment"""
        return utils.decode(libvect.Vect_get_comment(self.c_mapinfo))

    def _set_comment(self, comm):
        """Private method to set the Vector comment"""
        return libvect.Vect_set_comment(self.c_mapinfo, comm)

    comment = property(fget=_get_comment, fset=_set_comment,
                       doc="Set or obtain the Vector comment")

    def _get_zone(self):
        """Private method to obtain the Vector projection zone"""
        return libvect.Vect_get_zone(self.c_mapinfo)

    def _set_zone(self, zone):
        """Private method to set the Vector projection zone"""
        return libvect.Vect_set_zone(self.c_mapinfo, ctypes.c_int(zone))

    zone = property(fget=_get_zone, fset=_set_zone,
                    doc="Set or obtain the Vector projection zone")

    def _get_proj(self):
        """Private method to obtain the Vector projection code"""
        return libvect.Vect_get_proj(self.c_mapinfo)

    def _set_proj(self, proj):
        """Private method to set the Vector projection code"""
        libvect.Vect_set_proj(self.c_mapinfo, ctypes.c_int(proj))

    proj = property(fget=_get_proj, fset=_set_proj,
                    doc="Set or obtain the Vector projection code")

    def _get_thresh(self):
        """Private method to obtain the Vector threshold"""
        return libvect.Vect_get_thresh(self.c_mapinfo)

    def _set_thresh(self, thresh):
        """Private method to set the Vector threshold"""
        return libvect.Vect_set_thresh(self.c_mapinfo, ctypes.c_double(thresh))

    thresh = property(fget=_get_thresh, fset=_set_thresh,
                      doc="Set or obtain the Vector threshold")

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

    def write_header(self):
        """Save the change in the C struct permanently to disk."""
        libvect.Vect_write_header(self.c_mapinfo)

    def rename(self, newname):
        """Method to rename the Vector map

        :param newname: the new name for the Vector map
        :type newname: str
        """
        if self.exist():
            if not self.is_open():
                utils.rename(self.name, newname, 'vect')
            else:
                raise GrassError("The map is open, not able to renamed it.")
        self._name = newname

    def is_3D(self):
        """Return if the Vector is 3D"""
        return bool(libvect.Vect_is_3d(self.c_mapinfo))

    def exist(self):
        """Return if the Vector exists or not"""
        if self.name:
            if self.mapset == '':
                mapset = utils.get_mapset_vector(self.name, self.mapset)
                self.mapset = mapset if mapset else ''
                return True if mapset else False
            return bool(utils.get_mapset_vector(self.name, self.mapset))
        else:
            return False

    def is_open(self):
        """Return if the Vector is open"""
        return is_open(self.c_mapinfo)

    def open(self, mode=None, layer=1, overwrite=None, with_z=None,
             # parameters valid only if mode == 'w'
             tab_name='', tab_cols=None, link_name=None, link_key='cat',
             link_db='$GISDBASE/$LOCATION_NAME/$MAPSET/sqlite/sqlite.db',
             link_driver='sqlite'):
        """Open a Vector map.


        :param mode: open a vector map in ``r`` in reading, ``w`` in writing
                     and in ``rw`` read and write mode
        :type mode: str
        :param layer: specify the layer that you want to use
        :type layer: int
        :param overwrite: valid only for ``w`` mode
        :type overwrite: bool
        :param with_z: specify if vector map must be open with third dimension
                       enabled or not. Valid only for ``w`` mode,
                       default: False
        :type with_z: bool
        :param tab_name: define the name of the table that will be generate
        :type tab_name: str
        :param tab_cols: define the name and type of the columns of the
                         attribute table of the vecto map
        :type tab_cols: list of pairs
        :param link_name: define the name of the link connecttion with the
                          database
        :type link_name: str
        :param link_key: define the nema of the column that will be use as
                         vector category
        :type link_key: str
        :param link_db: define the database connection parameters
        :type link_db: str
        :param link_driver: define witch database driver will be used
        :param link_driver: str

        Some of the parameters are valid only with mode ``w`` or ``rw``

        See more examples in the documentation of the ``read`` and ``write``
        methods
        """
        self.mode = mode if mode else self.mode
        with_z = libvect.WITH_Z if with_z else libvect.WITHOUT_Z
        # check if map exists or not
        if not self.exist() and self.mode != 'w':
            raise OpenError("Map <%s> not found." % self._name)
        if libvect.Vect_set_open_level(self._topo_level) != 0:
            raise OpenError("Invalid access level.")
        # update the overwrite attribute
        self.overwrite = overwrite if overwrite is not None else self.overwrite
        # check if the mode is valid
        if self.mode not in ('r', 'rw', 'w'):
            raise ValueError("Mode not supported. Use one of: 'r', 'rw', 'w'.")

        # check if the map exist
        if self.exist() and self.mode in ('r', 'rw'):
            # open in READ mode
            if self.mode == 'r':
                openvect = libvect.Vect_open_old2(self.c_mapinfo, self.name,
                                                  self.mapset, str(layer))
            # open in READ and WRITE mode
            elif self.mode == 'rw':
                openvect = libvect.Vect_open_update2(self.c_mapinfo, self.name,
                                                     self.mapset, str(layer))

            # instantiate class attributes
            self.dblinks = DBlinks(self.c_mapinfo)

        # If it is opened in write mode
        if self.mode == 'w':
            openvect = libvect.Vect_open_new(self.c_mapinfo, self.name, with_z)
            self.dblinks = DBlinks(self.c_mapinfo)

        if self.mode in ('w', 'rw') and tab_cols:
            # create a link
            link = Link(layer,
                        link_name if link_name else self.name,
                        tab_name if tab_name else self.name,
                        link_key, link_db, link_driver)
            # add the new link
            self.dblinks.add(link)
            # create the table
            table = link.table()
            table.create(tab_cols, overwrite=overwrite)
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
            self.layer = self.dblinks.by_layer(layer).layer
            self.table = self.dblinks.by_layer(layer).table()
            self.n_lines = self.table.n_rows()
        self.writeable = self.mapset == utils.getenv("MAPSET")
        # Initialize the finder
        self.find = {'by_point': PointFinder(self.c_mapinfo, self.table,
                                             self.writeable),
                     'by_bbox': BboxFinder(self.c_mapinfo, self.table,
                                          self.writeable),
                     'by_polygon': PolygonFinder(self.c_mapinfo, self.table,
                                                 self.writeable), }
        self.find_by_point = self.find["by_point"]
        self.find_by_bbox  = self.find["by_bbox"]
        self.find_by_polygon = self.find["by_polygon"]

    def close(self, build=False):
        """Method to close the Vector

        :param build: True if the vector map should be build before close it
        :type build: bool
        """
        if hasattr(self, 'table') and self.table is not None:
            self.table.conn.close()
        if self.is_open():
            if libvect.Vect_close(self.c_mapinfo) != 0:
                str_err = 'Error when trying to close the map with Vect_close'
                raise GrassError(str_err)
            if ((self.c_mapinfo.contents.mode == libvect.GV_MODE_RW or
                    self.c_mapinfo.contents.mode == libvect.GV_MODE_WRITE) and
                    build):
                self.build()

    def remove(self):
        """Remove vector map"""
        if self.is_open():
            self.close()
        utils.remove(self.name, 'vect')

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

if __name__ == "__main__":
    import doctest
    utils.create_test_vector_map(test_vector_name)
    doctest.testmod()

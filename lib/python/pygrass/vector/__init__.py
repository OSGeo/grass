# -*- coding: utf-8 -*-
"""
Created on Tue Jul 17 08:51:53 2012

@author: pietro
"""
import ctypes

import grass.lib.vector as libvect
from vector_type import VTYPE, GV_TYPE

#
# import pygrass modules
#
from pygrass.errors import GrassError, must_be_open
from pygrass.functions import getenv

import geometry
from abstract import Info
from basic import Bbox

import grass.script.core as core
_GRASSENV = core.gisenv()

_NUMOF = {"areas": libvect.Vect_get_num_areas,
          "dblinks": libvect.Vect_get_num_dblinks,
          "faces": libvect.Vect_get_num_faces,
          "holes": libvect.Vect_get_num_holes,
          "islands": libvect.Vect_get_num_islands,
          "kernels": libvect.Vect_get_num_kernels,
          "lines": libvect.Vect_get_num_line_points,
          "points": libvect.Vect_get_num_lines,
          "nodes": libvect.Vect_get_num_nodes,
          "updated_lines": libvect.Vect_get_num_updated_lines,
          "updated_nodes": libvect.Vect_get_num_updated_nodes,
          "volumes": libvect.Vect_get_num_volumes}

_GEOOBJ = {"areas": geometry.Area,
           "dblinks": None,
           "faces": None,
           "holes": None,
           "islands": geometry.Isle,
           "kernels": None,
           "line_points": None,
           "points": geometry.Point,
           "lines": geometry.Line,
           "nodes": geometry.Node,
           "volumes": None}


#=============================================
# VECTOR
#=============================================

class Vector(Info):
    """ ::

        >>> from pygrass.vector import Vector
        >>> municip = Vector('boundary_municp_sqlite')
        >>> municip.is_open()
        False
        >>> municip.mapset
        ''
        >>> municip.exist()
        True
        >>> municip.mapset
        '%s'
        >>> municip.overwrite
        False

    ..
    """ % _GRASSENV['MAPSET']
    def __init__(self, name, mapset=''):
        # Set map name and mapset
        super(Vector, self).__init__(name, mapset)
        self._topo_level = 1
        self._class_name = 'Vector'
        self.overwrite = False

    def __repr__(self):
        if self.exist():
            return "%s(%r, %r)" % (self._class_name, self.name, self.mapset)
        else:
            return "%s(%r)" % (self._class_name, self.name)

    def __iter__(self):
        """::

            >>> mun = Vector('boundary_municp_sqlite')
            >>> mun.open()
            >>> features = [feature for feature in mun]
            >>> features[:3]
            [Boundary(v_id=None), Boundary(v_id=None), Boundary(v_id=None)]
            >>> mun.close()

        ..
        """
        #return (self.read(f_id) for f_id in xrange(self.num_of_features()))
        return self

    @must_be_open
    def next(self):
        """::

            >>> mun = Vector('boundary_municp_sqlite')
            >>> mun.open()
            >>> mun.next()
            Boundary(v_id=None)
            >>> mun.next()
            Boundary(v_id=None)
            >>> mun.close()

        ..
        """
        v_id = self.c_mapinfo.contents.next_line
        v_id = v_id if v_id != 0 else None
        c_points = ctypes.pointer(libvect.line_pnts())
        c_cats = ctypes.pointer(libvect.line_cats())
        ftype = libvect.Vect_read_next_line(self.c_mapinfo, c_points, c_cats)
        if ftype == -2:
            raise StopIteration()
        if ftype == -1:
            raise
        #if  GV_TYPE[ftype]['obj'] is not None:
        return GV_TYPE[ftype]['obj'](v_id=v_id,
                                     c_mapinfo=self.c_mapinfo,
                                     c_points=c_points,
                                     c_cats=c_cats)

    @must_be_open
    def rewind(self):
        if libvect.Vect_rewind(self.c_mapinfo) == -1:
            raise GrassError("Vect_rewind raise an error.")

    @must_be_open
    def write(self, geo_obj):
        """::

            >>> mun = Vector('boundary_municp_sqlite')         #doctest: +SKIP
            >>> mun.open(mode='rw')            #doctest: +SKIP
            >>> feature1 = mun.read(1)                         #doctest: +SKIP
            >>> feature1                                       #doctest: +SKIP
            Boundary(v_id=1)
            >>> feature1[:3]             #doctest: +SKIP +NORMALIZE_WHITESPACE
            [Point(463718.874987, 310970.844494),
             Point(463707.405987, 310989.499494),
             Point(463714.593986, 311084.281494)]
            >>> from geometry import Point                     #doctest: +SKIP
            >>> feature1.insert(1, Point(463713.000000, 310980.000000))
            ...                                                #doctest: +SKIP
            >>> feature1[:4]             #doctest: +SKIP +NORMALIZE_WHITESPACE
            [Point(463718.874987, 310970.844494),
             Point(463713.000000, 310980.000000),
             Point(463707.405987, 310989.499494),
             Point(463714.593986, 311084.281494)]
            >>> mun.write(feature1)                            #doctest: +SKIP
            >>> feature1                                       #doctest: +SKIP
            Boundary(v_id=8708)
            >>> mun.close()                                    #doctest: +SKIP

        ..
        """
        result = libvect.Vect_write_line(self.c_mapinfo, geo_obj.gtype,
                                         geo_obj.c_points, geo_obj.c_cats)
        if result == -1:
            raise GrassError("Not able to write the vector feature.")
        if self._topo_level == 2:
            # return new feature id (on level 2)
            geo_obj.id = result
        else:
            # return offset into file where the feature starts (on level 1)
            geo_obj.offset = result


#=============================================
# VECTOR WITH TOPOLOGY
#=============================================

class VectorTopo(Vector):
    def __init__(self, name, mapset=''):
        super(VectorTopo, self).__init__(name, mapset)
        self._topo_level = 2
        self._class_name = 'VectorTopo'

    def __len__(self):
        return libvect.Vect_get_num_lines(self.c_mapinfo)

    def __getitem__(self, key):
        """::

            >>> mun = VectorTopo('boundary_municp_sqlite')
            >>> mun.open()
            >>> mun[:3]
            [Boundary(v_id=1), Boundary(v_id=2), Boundary(v_id=3)]
            >>> mun.close()

        ..
        """
        if isinstance(key, slice):
            #import pdb; pdb.set_trace()
            #Get the start, stop, and step from the slice
            return [self.read(indx + 1)
                    for indx in xrange(*key.indices(len(self)))]
        elif isinstance(key, int):
            self.read(key)
        else:
            raise ValueError("Invalid argument type: %r." % key)

    @must_be_open
    def num_primitive_of(self, primitive):
        """Primitive are:

            * "boundary",
            * "centroid",
            * "face",
            * "kernel",
            * "line",
            * "point"
            * "area"
            * "volume"

        ::

            >>> municip = VectorTopo('boundary_municp_sqlite')
            >>> municip.open()
            >>> municip.num_primitive_of('point')
            0
            >>> municip.num_primitive_of('line')
            0
            >>> municip.num_primitive_of('centroid')
            3579
            >>> municip.num_primitive_of('boundary')
            5128
            >>> municip.close()

        ..
        """
        return libvect.Vect_get_num_primitives(self.c_mapinfo,
                                               VTYPE[primitive])

    @must_be_open
    def number_of(self, vtype):
        """
        vtype in ["areas", "dblinks", "faces", "holes", "islands", "kernels",
                  "line_points", "lines", "nodes", "update_lines",
                  "update_nodes", "volumes"]

            >>> municip = VectorTopo('boundary_municp_sqlite')
            >>> municip.open()
            >>> municip.number_of("areas")
            3579
            >>> municip.number_of("islands")
            2629
            >>> municip.number_of("holes")
            0
            >>> municip.number_of("lines")
            8707
            >>> municip.number_of("nodes")
            4178
            >>> municip.number_of("pizza")
            ...                     # doctest: +ELLIPSIS +NORMALIZE_WHITESPACE
            Traceback (most recent call last):
                ...
            ValueError: vtype not supported, use one of:
            'areas', 'dblinks', 'faces', 'holes', 'islands', 'kernels',
            'line_points', 'lines', 'nodes', 'updated_lines', 'updated_nodes',
            'volumes'
            >>> municip.close()


        ..
        """
        if vtype in _NUMOF.keys():
            return _NUMOF[vtype](self.c_mapinfo)
        else:
            keys = "', '".join(sorted(_NUMOF.keys()))
            raise ValueError("vtype not supported, use one of: '%s'" % keys)

    @must_be_open
    def num_primitives(self):
        """Return dictionary with the number of all primitives
        """
        output = {}
        for prim in VTYPE.keys():
            output[prim] = self.num_primitive_of(prim)
        return output

    @must_be_open
    def viter(self, vtype):
        """Return an iterator of vector features

        ::
            >>> municip = VectorTopo('boundary_municp_sqlite')
            >>> municip.open()
            >>> big = [area for area in municip.viter('areas')
            ...        if area.alive() and area.area >= 10000]
            >>> big[:3]
            [Area(1), Area(2), Area(3)]


        to sort the result in a efficient way, use: ::

            >>> from operator import methodcaller as method
            >>> big.sort(key = method('area'), reverse = True)  # sort the list
            >>> for area in big[:3]:
            ...     print area, area.area()
            Area(3102) 697521857.848
            Area(2682) 320224369.66
            Area(2552) 298356117.948
            >>> municip.close()

        ..
        """
        if vtype in _GEOOBJ.keys():
            if _GEOOBJ[vtype] is not None:
                return (_GEOOBJ[vtype](v_id=indx, c_mapinfo=self.c_mapinfo,
                                       table=self.table,
                                       writable=self.write)
                        for indx in xrange(1, self.number_of(vtype) + 1))
        else:
            keys = "', '".join(sorted(_GEOOBJ.keys()))
            raise ValueError("vtype not supported, use one of: '%s'" % keys)

    @must_be_open
    def rewind(self):
        """Rewind vector map to cause reads to start at beginning. ::

            >>> mun = VectorTopo('boundary_municp_sqlite')
            >>> mun.open()
            >>> mun.next()
            Boundary(v_id=1)
            >>> mun.next()
            Boundary(v_id=2)
            >>> mun.next()
            Boundary(v_id=3)
            >>> mun.rewind()
            >>> mun.next()
            Boundary(v_id=1)
            >>> mun.close()

        ..
        """
        libvect.Vect_rewind(self.c_mapinfo)

    @must_be_open
    def read(self, feature_id):
        """Return a geometry object given the feature id. ::

            >>> mun = VectorTopo('boundary_municp_sqlite')
            >>> mun.open()
            >>> feature1 = mun.read(0)                     #doctest: +ELLIPSIS
            Traceback (most recent call last):
                ...
            ValueError: The index must be >0, 0 given.
            >>> feature1 = mun.read(1)
            >>> feature1
            Boundary(v_id=1)
            >>> feature1.length()
            1415.3348048582038
            >>> mun.read(-1)
            Centoid(649102.382010, 15945.714502)
            >>> len(mun)
            8707
            >>> mun.read(8707)
            Centoid(649102.382010, 15945.714502)
            >>> mun.read(8708)                             #doctest: +ELLIPSIS
            Traceback (most recent call last):
              ...
            IndexError: Index out of range
            >>> mun.close()

        ..
        """
        if feature_id < 0:  # Handle negative indices
                feature_id += self.__len__() + 1
        if feature_id >= (self.__len__() + 1):
            raise IndexError('Index out of range')
        if feature_id > 0:
            c_points = ctypes.pointer(libvect.line_pnts())
            c_cats = ctypes.pointer(libvect.line_cats())
            ftype = libvect.Vect_read_line(self.c_mapinfo, c_points,
                                           c_cats, feature_id)
            if  GV_TYPE[ftype]['obj'] is not None:
                return GV_TYPE[ftype]['obj'](v_id=feature_id,
                                             c_mapinfo=self.c_mapinfo,
                                             c_points=c_points,
                                             c_cats=c_cats,
                                             table=self.table,
                                             write=self.write)
        else:
            raise ValueError('The index must be >0, %r given.' % feature_id)

    @must_be_open
    def is_empty(self):
        """Return if a vector map is empty or not
        """
        primitives = self.num_primitives()
        output = True
        for v in primitives.values():
            if v != 0:
                output = False
                break
        return output

    @must_be_open
    def rewrite(self, geo_obj):
        result = libvect.Vect_rewrite_line(self.c_mapinfo,
                                           geo_obj.id, geo_obj.gtype,
                                           geo_obj.c_points,
                                           geo_obj.c_cats)
        # return offset into file where the feature starts
        geo_obj.offset = result

    @must_be_open
    def delete(self, feature_id):
        if libvect.Vect_rewrite_line(self.c_mapinfo, feature_id) == -1:
            raise GrassError("C funtion: Vect_rewrite_line.")

    @must_be_open
    def restore(self, geo_obj):
        if hasattr(geo_obj, 'offset'):
            if libvect.Vect_restore_line(self.c_mapinfo, geo_obj.id,
                                         geo_obj.offset) == -1:
                raise GrassError("C funtion: Vect_restore_line.")
        else:
            raise ValueError("The value have not an offset attribute.")

    @must_be_open
    def bbox(self):
        """Return the BBox of the vecor map
        """
        bbox = Bbox()
        if libvect.Vect_get_map_box(self.c_mapinfo, bbox.c_bbox) == 0:
            raise GrassError("I can not find the Bbox.")
        return bbox

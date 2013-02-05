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
from grass.pygrass.errors import GrassError, must_be_open

import geometry
from abstract import Info
from basic import Bbox


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

        >>> from grass.pygrass.vector import Vector
        >>> cens = Vector('census')
        >>> cens.is_open()
        False
        >>> cens.mapset
        ''
        >>> cens.exist()
        True
        >>> cens.mapset
        'PERMANENT'
        >>> cens.overwrite
        False

    ..
    """
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

            >>> mun = Vector('census')
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

            >>> mun = Vector('census')
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
                                     c_cats=c_cats,
                                     table=self.table,
                                     writable=self.writable)

    @must_be_open
    def rewind(self):
        if libvect.Vect_rewind(self.c_mapinfo) == -1:
            raise GrassError("Vect_rewind raise an error.")

    @must_be_open
    def write(self, geo_obj, attrs=None):
        """Write geometry features and attributes

        Open a new vector map ::

            >>> new = VectorTopo('newvect')
            >>> new.exist()
            False

        define the new columns of the attribute table ::

            >>> cols = [(u'cat',       'INTEGER PRIMARY KEY'),
            ...         (u'name',      'TEXT')]

        open the vector map in write mode

            >>> new.open('w', tab_name='newvect', tab_cols=cols)

        import a geometry feature ::
            >>> from grass.pygrass.vector.geometry import Point

        create two points ::

            >>> point0 = Point(636981.336043, 256517.602235)
            >>> point1 = Point(637209.083058, 257970.129540)

        then write the two points on the map, with ::

            >>> new.write2(point0, ('pub', ))
            >>> new.write2(point1, ('resturnat', ))

        close the vector map ::

            >>> new.close()
            >>> new.exist()
            True

        then play with the map ::

            >>> new.open()
            >>> new.read(1)
            Point(636981.336043, 256517.602235)
            >>> new.read(2)
            Point(637209.083058, 257970.129540)
            >>> new.read(1).attrs['name']
            u'pub'
            >>> new.read(2).attrs['cat', 'name']
            (2, u'resturnat')
            >>> new.close()
            >>> new.remove()

        ..
        """
        self.n_lines += 1
        if self.table is not None and attrs:
            attr = [self.n_lines, ]
            attr.extend(attrs)
            cur = self.table.conn.cursor()
            cur.execute(self.table.columns.insert_str, attr)
            cur.close()

        libvect.Vect_cat_set(geo_obj.c_cats, self.layer, self.n_lines)
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

            >>> mun = VectorTopo('census')
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
            return self.read(key)
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

            >>> cens = VectorTopo('boundary_municp_sqlite')
            >>> cens.open()
            >>> cens.num_primitive_of('point')
            0
            >>> cens.num_primitive_of('line')
            0
            >>> cens.num_primitive_of('centroid')
            3579
            >>> cens.num_primitive_of('boundary')
            5128
            >>> cens.close()

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

            >>> cens = VectorTopo('boundary_municp_sqlite')
            >>> cens.open()
            >>> cens.number_of("areas")
            3579
            >>> cens.number_of("islands")
            2629
            >>> cens.number_of("holes")
            0
            >>> cens.number_of("lines")
            8707
            >>> cens.number_of("nodes")
            4178
            >>> cens.number_of("pizza")
            ...                     # doctest: +ELLIPSIS +NORMALIZE_WHITESPACE
            Traceback (most recent call last):
                ...
            ValueError: vtype not supported, use one of:
            'areas', 'dblinks', 'faces', 'holes', 'islands', 'kernels',
            'line_points', 'lines', 'nodes', 'updated_lines', 'updated_nodes',
            'volumes'
            >>> cens.close()


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
            >>> cens = VectorTopo('census')
            >>> cens.open()
            >>> big = [area for area in cens.viter('areas')
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
            >>> cens.close()

        ..
        """
        if vtype in _GEOOBJ.keys():
            if _GEOOBJ[vtype] is not None:
                return (_GEOOBJ[vtype](v_id=indx, c_mapinfo=self.c_mapinfo,
                                       table=self.table,
                                       writable=self.writable)
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
        if feature_id > (self.__len__()):
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
                                             writable=self.writable)
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
    def rewrite(self, line, geo_obj, attrs=None, **kargs):
        """Rewrite a geometry features
        """
        if self.table is not None and attrs:
            attr = [line, ]
            attr.extend(attrs)
            self.table.update(key=line, values=attr)

        libvect.Vect_cat_set(geo_obj.c_cats, self.layer, line)
        result = libvect.Vect_rewrite_line(self.c_mapinfo,
                                           line, geo_obj.gtype,
                                           geo_obj.c_points,
                                           geo_obj.c_cats)
        if result == -1:
            raise GrassError("Not able to write the vector feature.")

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

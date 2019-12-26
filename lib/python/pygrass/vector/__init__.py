# -*- coding: utf-8 -*-
from __future__ import print_function

from os.path import join, exists
import grass.lib.gis as libgis
libgis.G_gisinit('')
import grass.lib.vector as libvect
import ctypes

#
# import pygrass modules
#
from grass.pygrass.vector.vector_type import VTYPE
from grass.pygrass.errors import GrassError, must_be_open
from grass.pygrass.gis import Location

from grass.pygrass.vector.geometry import GEOOBJ as _GEOOBJ
from grass.pygrass.vector.geometry import read_line, read_next_line
from grass.pygrass.vector.geometry import Area as _Area
from grass.pygrass.vector.abstract import Info
from grass.pygrass.vector.basic import Bbox, Cats, Ilist


_NUMOF = {"areas": libvect.Vect_get_num_areas,
          "dblinks": libvect.Vect_get_num_dblinks,
          "faces": libvect.Vect_get_num_faces,
          "holes": libvect.Vect_get_num_holes,
          "islands": libvect.Vect_get_num_islands,
          "kernels": libvect.Vect_get_num_kernels,
          "points": (libvect.Vect_get_num_primitives, libvect.GV_POINT),
          "lines": (libvect.Vect_get_num_primitives, libvect.GV_LINE),
          "centroids": (libvect.Vect_get_num_primitives, libvect.GV_CENTROID),
          "boundaries": (libvect.Vect_get_num_primitives, libvect.GV_BOUNDARY),
          "nodes": libvect.Vect_get_num_nodes,
          "updated_lines": libvect.Vect_get_num_updated_lines,
          "updated_nodes": libvect.Vect_get_num_updated_nodes,
          "volumes": libvect.Vect_get_num_volumes}

# For test purposes
test_vector_name = "vector_doctest_map"

#=============================================
# VECTOR
#=============================================

class Vector(Info):
    """Vector class is the grass vector format without topology

        >>> from grass.pygrass.vector import Vector
        >>> test_vect = Vector(test_vector_name)
        >>> test_vect.is_open()
        False
        >>> test_vect.mapset
        ''
        >>> test_vect.exist()
        True
        >>> test_vect.overwrite
        False

    """
    def __init__(self, name, mapset='', *args, **kwargs):
        # Set map name and mapset
        super(Vector, self).__init__(name, mapset, *args, **kwargs)
        self._topo_level = 1
        self._class_name = 'Vector'
        self.overwrite = False
        self._cats = []

    def __repr__(self):
        if self.exist():
            return "%s(%r, %r)" % (self._class_name, self.name, self.mapset)
        else:
            return "%s(%r)" % (self._class_name, self.name)

    def __iter__(self):
        """::

            >>> test_vect = Vector(test_vector_name)
            >>> test_vect.open(mode='r')
            >>> features = [feature for feature in test_vect]
            >>> features[:3]
            [Point(10.000000, 6.000000), Point(12.000000, 6.000000), Point(14.000000, 6.000000)]
            >>> test_vect.close()

        ..
        """
        #return (self.read(f_id) for f_id in xrange(self.num_of_features()))
        return self

    @must_be_open
    def __next__(self):
        """::

            >>> test_vect = Vector(test_vector_name)
            >>> test_vect.open(mode='r')
            >>> test_vect.next()
            Point(10.000000, 6.000000)
            >>> test_vect.next()
            Point(12.000000, 6.000000)
            >>> test_vect.close()

        ..
        """
        return read_next_line(self.c_mapinfo, self.table, self.writeable,
                              is2D=not self.is_3D())

    @must_be_open
    def next(self):
        return self.__next__()

    @must_be_open
    def rewind(self):
        """Rewind vector map to cause reads to start at beginning."""
        if libvect.Vect_rewind(self.c_mapinfo) == -1:
            raise GrassError("Vect_rewind raise an error.")

    @must_be_open
    def write(self, geo_obj, cat=None, attrs=None):
        """Write geometry features and attributes.

        :param geo_obj: a geometry grass object define in
                        grass.pygrass.vector.geometry
        :type geo_obj: geometry GRASS object
        :param attrs: a list with the values that will be insert in the
                      attribute table.
        :type attrs: list
        :param cat: The category of the geometry feature, otherwise the
                         c_cats attribute of the geometry object will be used.
        :type cat: integer

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

            >>> point0 = Point(0, 0)
            >>> point1 = Point(1, 1)

        then write the two points on the map, with ::

            >>> new.write(point0, cat=1, attrs=('pub',))
            >>> new.write(point1, cat=2, attrs=('resturant',))

        commit the db changes ::

            >>> new.table.conn.commit()
            >>> new.table.execute().fetchall()
            [(1, 'pub'), (2, 'resturant')]

        close the vector map ::

            >>> new.close()
            >>> new.exist()
            True

        then play with the map ::

            >>> new.open(mode='r')
            >>> new.read(1)
            Point(0.000000, 0.000000)
            >>> new.read(2)
            Point(1.000000, 1.000000)
            >>> new.read(1).attrs['name']
            'pub'
            >>> new.read(2).attrs['name']
            'resturant'
            >>> new.close()
            >>> new.remove()

        """
        self.n_lines += 1
        if not isinstance(cat, int) and not isinstance(cat, str):
            # likely the case of using 7.0 API
            import warnings
            warnings.warn("Vector.write(geo_obj, attrs=(...)) is"
                          " depreciated, specify cat explicitly",
                          DeprecationWarning)
            # try to accommodate
            attrs = cat
            cat = None
        if attrs and cat is None:
            # TODO: this does not work as expected when there are
            # already features in the map when we opened it
            cat = (self._cats[-1] if self._cats else 0) + 1

        if cat is not None and cat not in self._cats:
            self._cats.append(cat)
            if self.table is not None and attrs is not None:
                attr = [cat, ]
                attr.extend(attrs)
                cur = self.table.conn.cursor()
                cur.execute(self.table.columns.insert_str, attr)
                cur.close()

        if cat is not None:
            cats = Cats(geo_obj.c_cats)
            cats.reset()
            cats.set(cat, self.layer)

        if geo_obj.gtype == _Area.gtype:
            result = self._write_area(geo_obj)
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

    @must_be_open
    def has_color_table(self):
        """Return if vector has color table associated in file system;
        Color table stored in the vector's attribute table well be not checked

        >>> test_vect = Vector(test_vector_name)
        >>> test_vect.open(mode='r')
        >>> test_vect.has_color_table()
        False

        >>> test_vect.close()
        >>> from grass.pygrass.utils import copy, remove
        >>> copy(test_vector_name,'mytest_vect','vect')
        >>> from grass.pygrass.modules.shortcuts import vector as v
        >>> v.colors(map='mytest_vect', color='population', column='value')
        Module('v.colors')
        >>> mytest_vect = Vector('mytest_vect')
        >>> mytest_vect.open(mode='r')
        >>> mytest_vect.has_color_table()
        True
        >>> mytest_vect.close()
        >>> remove('mytest_vect', 'vect')
        """
        loc = Location()
        path = join(loc.path(), self.mapset, 'vector', self.name, 'colr')
        return True if exists(path) else False


#=============================================
# VECTOR WITH TOPOLOGY
#=============================================

class VectorTopo(Vector):
    """Vector class with the support of the GRASS topology.

    Open a vector map using the *with statement*: ::

        >>> with VectorTopo(test_vector_name, mode='r') as test_vect:
        ...     for feature in test_vect[:7]:
        ...         print(feature.attrs['name'])
        ...
        point
        point
        point
        line
        line
        line
        >>> test_vect.is_open()
        False

    ..
    """
    def __init__(self, name, mapset='', *args, **kwargs):
        super(VectorTopo, self).__init__(name, mapset, *args, **kwargs)
        self._topo_level = 2
        self._class_name = 'VectorTopo'

    def __len__(self):
        return libvect.Vect_get_num_lines(self.c_mapinfo)

    def __getitem__(self, key):
        """::

            >>> test_vect = VectorTopo(test_vector_name)
            >>> test_vect.open(mode='r')
            >>> test_vect[:4]
            [Point(10.000000, 6.000000), Point(12.000000, 6.000000), Point(14.000000, 6.000000)]
            >>> test_vect.close()

        ..
        """
        if isinstance(key, slice):
            return [self.read(indx)
                    for indx in range(key.start if key.start else 1,
                                      key.stop if key.stop else len(self),
                                      key.step if key.step else 1)]
        elif isinstance(key, int):
            return self.read(key)
        else:
            raise ValueError("Invalid argument type: %r." % key)

    @must_be_open
    def num_primitive_of(self, primitive):
        """Return the number of primitive

        :param primitive: the name of primitive to query; the supported values are:

                            * *boundary*,
                            * *centroid*,
                            * *face*,
                            * *kernel*,
                            * *line*,
                            * *point*
                            * *area*
                            * *volume*

        :type primitive: str

        ::

            >>> test_vect = VectorTopo(test_vector_name)
            >>> test_vect.open(mode='r')
            >>> test_vect.num_primitive_of('point')
            3
            >>> test_vect.num_primitive_of('line')
            3
            >>> test_vect.num_primitive_of('centroid')
            4
            >>> test_vect.num_primitive_of('boundary')
            11
            >>> test_vect.close()

        ..
        """
        return libvect.Vect_get_num_primitives(self.c_mapinfo,
                                               VTYPE[primitive])

    @must_be_open
    def number_of(self, vtype):
        """Return the number of the chosen element type

        :param vtype: the name of type to query; the supported values are:
                      *areas*, *dblinks*, *faces*, *holes*, *islands*,
                      *kernels*, *points*, *lines*, *centroids*, *boundaries*,
                      *nodes*, *line_points*, *update_lines*, *update_nodes*,
                      *volumes*
        :type vtype: str

            >>> test_vect = VectorTopo(test_vector_name)
            >>> test_vect.open(mode='r')
            >>> test_vect.number_of("areas")
            4
            >>> test_vect.number_of("islands")
            2
            >>> test_vect.number_of("holes")
            0
            >>> test_vect.number_of("lines")
            3
            >>> test_vect.number_of("nodes")
            15
            >>> test_vect.number_of("pizza")
            ...                     # doctest: +ELLIPSIS +NORMALIZE_WHITESPACE
            Traceback (most recent call last):
                ...
            ValueError: vtype not supported, use one of: 'areas', ...
            >>> test_vect.close()


        ..
        """
        if vtype in _NUMOF.keys():
            if isinstance(_NUMOF[vtype], tuple):
                fn, ptype = _NUMOF[vtype]
                return fn(self.c_mapinfo, ptype)
            else:
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
    def viter(self, vtype, idonly=False):
        """Return an iterator of vector features

        :param vtype: the name of type to query; the supported values are:
                      *areas*, *dblinks*, *faces*, *holes*, *islands*,
                      *kernels*, *line_points*, *lines*, *nodes*, *points*,
                      *update_lines*, *update_nodes*, *volumes*
        :type vtype: str
        :param idonly: variable to return only the id of features instead of
                       full features
        :type idonly: bool

            >>> test_vect = VectorTopo(test_vector_name, mode='r')
            >>> test_vect.open(mode='r')
            >>> areas = [area for area in test_vect.viter('areas')]
            >>> areas[:3]
            [Area(1), Area(2), Area(3)]


        to sort the result in a efficient way, use: ::

            >>> from operator import methodcaller as method
            >>> areas.sort(key=method('area'), reverse=True)  # sort the list
            >>> for area in areas[:3]:
            ...     print(area, area.area())
            Area(1) 12.0
            Area(2) 8.0
            Area(4) 8.0

            >>> areas = [area for area in test_vect.viter('areas')]
            >>> for area in areas:
            ...     print(area.centroid().cat)
            3
            3
            3
            3

            >>> test_vect.close()
        """
        if vtype in _GEOOBJ.keys():
            if _GEOOBJ[vtype] is not None:
                ids = (indx for indx in range(1, self.number_of(vtype) + 1))
                if idonly:
                    return ids
                return (_GEOOBJ[vtype](v_id=indx, c_mapinfo=self.c_mapinfo,
                                       table=self.table,
                                       writeable=self.writeable)
                        for indx in ids)
        else:
            keys = "', '".join(sorted(_GEOOBJ.keys()))
            raise ValueError("vtype not supported, use one of: '%s'" % keys)

    @must_be_open
    def rewind(self):
        """Rewind vector map to cause reads to start at beginning. ::

            >>> test_vect = VectorTopo(test_vector_name)
            >>> test_vect.open(mode='r')
            >>> test_vect.next()
            Point(10.000000, 6.000000)
            >>> test_vect.next()
            Point(12.000000, 6.000000)
            >>> test_vect.next()
            Point(14.000000, 6.000000)
            >>> test_vect.rewind()
            >>> test_vect.next()
            Point(10.000000, 6.000000)
            >>> test_vect.close()

        ..
        """
        libvect.Vect_rewind(self.c_mapinfo)

    @must_be_open
    def cat(self, cat_id, vtype, layer=None, generator=False, geo=None):
        """Return the geometry features with category == cat_id.

        :param cat_id: the category number
        :type cat_id: int
        :param vtype: the type of geometry feature that we are looking for
        :type vtype: str
        :param layer: the layer number that will be used
        :type layer: int
        :param generator: if True return a generator otherwise it return a
                          list of features
        :type generator: bool
        """
        if geo is None and vtype not in _GEOOBJ:
            keys = "', '".join(sorted(_GEOOBJ.keys()))
            raise ValueError("vtype not supported, use one of: '%s'" % keys)
        Obj = _GEOOBJ[vtype] if geo is None else geo
        ilist = Ilist()
        libvect.Vect_cidx_find_all(self.c_mapinfo,
                                   layer if layer else self.layer,
                                   Obj.gtype, cat_id, ilist.c_ilist)
        is2D = not self.is_3D()
        if generator:
            return (read_line(feature_id=v_id, c_mapinfo=self.c_mapinfo,
                              table=self.table, writeable=self.writeable,
                              is2D=is2D)
                    for v_id in ilist)
        else:
            return [read_line(feature_id=v_id, c_mapinfo=self.c_mapinfo,
                              table=self.table, writeable=self.writeable,
                              is2D=is2D)
                    for v_id in ilist]

    @must_be_open
    def read(self, feature_id):
        """Return a geometry object given the feature id.

        :param int feature_id: the id of feature to obtain

        >>> test_vect = VectorTopo(test_vector_name)
        >>> test_vect.open(mode='r')
        >>> feature1 = test_vect.read(0)                     #doctest: +ELLIPSIS
        Traceback (most recent call last):
            ...
        ValueError: The index must be >0, 0 given.
        >>> feature1 = test_vect.read(5)
        >>> feature1
        Line([Point(12.000000, 4.000000), Point(12.000000, 2.000000), Point(12.000000, 0.000000)])
        >>> feature1.length()
        4.0
        >>> test_vect.read(-1)
        Centroid(7.500000, 3.500000)
        >>> len(test_vect)
        21
        >>> test_vect.read(21)
        Centroid(7.500000, 3.500000)
        >>> test_vect.read(22)                             #doctest: +ELLIPSIS
        Traceback (most recent call last):
          ...
        IndexError: Index out of range
        >>> test_vect.close()

        """
        return read_line(feature_id, self.c_mapinfo, self.table, self.writeable,
                         is2D=not self.is_3D())

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
    def rewrite(self, geo_obj, cat, attrs=None, **kargs):
        """Rewrite a geometry features

            >>> cols = [(u'cat',       'INTEGER PRIMARY KEY'),
            ...         (u'name',      'TEXT')]

        Generate a new vector map

            >>> test_vect = VectorTopo('newvect_2')
            >>> test_vect.open('w', tab_name='newvect_2', tab_cols=cols,
            ...                overwrite=True)

        import a geometry feature ::

            >>> from grass.pygrass.vector.geometry import Point

        create two points ::

            >>> point0 = Point(0, 0)
            >>> point1 = Point(1, 1)
            >>> point2 = Point(2, 2)

        then write the two points on the map, with ::

            >>> test_vect.write(point0, cat=1, attrs=('pub',))
            >>> test_vect.write(point1, cat=2, attrs=('resturant',))
            >>> test_vect.table.conn.commit()  # save changes in the DB
            >>> test_vect.table_to_dict()
            {1: [1, 'pub'], 2: [2, 'resturant']}
            >>> test_vect.close()

        Now rewrite one point of the vector map: ::

            >>> test_vect.open('rw')
            >>> test_vect.rewrite(point2, cat=1, attrs=('Irish Pub',))
            >>> test_vect.table.conn.commit()  # save changes in the DB
            >>> test_vect.close()

        Check the output:

            >>> test_vect.open('r')
            >>> test_vect[1] == point2
            True
            >>> test_vect[1].attrs['name'] == 'Irish Pub'
            True
            >>> test_vect.close()
            >>> test_vect.remove()
        """
        if self.table is not None and attrs:
            self.table.update(key=cat, values=attrs)
        elif self.table is None and attrs:
            print("Table for vector {name} does not exist, attributes not"
                  " loaded".format(name=self.name))
        libvect.Vect_cat_set(geo_obj.c_cats, self.layer, cat)
        result = libvect.Vect_rewrite_line(self.c_mapinfo,
                                           cat, geo_obj.gtype,
                                           geo_obj.c_points,
                                           geo_obj.c_cats)
        if result == -1:
            raise GrassError("Not able to write the vector feature.")

        # return offset into file where the feature starts
        geo_obj.offset = result

    @must_be_open
    def delete(self, feature_id):
        """Remove a feature by its id

        :param feature_id: the id of the feature
        :type feature_id: int
        """
        if libvect.Vect_rewrite_line(self.c_mapinfo, feature_id) == -1:
            raise GrassError("C function: Vect_rewrite_line.")

    @must_be_open
    def restore(self, geo_obj):
        if hasattr(geo_obj, 'offset'):
            if libvect.Vect_restore_line(self.c_mapinfo, geo_obj.offset,
                                         geo_obj.id) == -1:
                raise GrassError("C function: Vect_restore_line.")
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

    def close(self, build=True, release=True):
        """Close the VectorTopo map, if release is True, the memory
        occupied by spatial index is released"""
        if release:
            libvect.Vect_set_release_support(self.c_mapinfo)
        super(VectorTopo, self).close(build=build)

    @must_be_open
    def table_to_dict(self, where=None):
        """Return the attribute table as a dictionary with the category as keys

            The columns have the order of the self.table.columns.names() list.

            Examples

            >>> from grass.pygrass.vector import VectorTopo
            >>> from grass.pygrass.vector.basic import Bbox
            >>> test_vect = VectorTopo(test_vector_name)
            >>> test_vect.open('r')

            >>> test_vect.table_to_dict()
            {1: [1, 'point', 1.0], 2: [2, 'line', 2.0], 3: [3, 'centroid', 3.0]}

            >>> test_vect.table_to_dict(where="value > 2")
            {3: [3, 'centroid', 3.0]}

            >>> test_vect.table_to_dict(where="value > 0")
            {1: [1, 'point', 1.0], 2: [2, 'line', 2.0], 3: [3, 'centroid', 3.0]}

            >>> test_vect.table.filters.get_sql()
            'SELECT cat,name,value FROM vector_doctest_map WHERE value > 0 ORDER BY cat;'

        """

        if self.table is not None:
            table_dict = {}
            # Get the category index
            cat_index = self.table.columns.names().index("cat")
            # Prepare a filter
            if where is not None:
                self.table.filters.where(where)

            self.table.filters.order_by("cat")

            self.table.filters.select(",".join(self.table.columns.names()))
            # Execute the query and fetch the result
            cur = self.table.execute()
            l = cur.fetchall()
            # Generate the dictionary
            for entry in l:
                table_dict[entry[cat_index]] = list(entry)

            return(table_dict)

        return None

    @must_be_open
    def features_to_wkb_list(self, bbox=None, feature_type="point", field=1):
        """Return all features of type point, line, boundary or centroid
           as a list of Well Known Binary representations (WKB)
           (id, cat, wkb) triplets located in a specific
           bounding box.

           :param bbox: The boundingbox to search for features,
                       if bbox=None the boundingbox of the whole
                       vector map layer is used

           :type bbox: grass.pygrass.vector.basic.Bbox

           :param feature_type: The type of feature that should be converted to
                                the Well Known Binary (WKB) format. Supported are:
                               'point'    -> libvect.GV_POINT     1
                               'line'     -> libvect.GV_LINE      2
                               'boundary' -> libvect.GV_BOUNDARY  3
                               'centroid' -> libvect.GV_CENTROID  4
           :type type: string

           :param field: The category field
           :type field: integer

           :return: A list of triplets, or None if nothing was found

           The well known binary are stored in byte arrays.

            Examples:

            >>> from grass.pygrass.vector import VectorTopo
            >>> from grass.pygrass.vector.basic import Bbox
            >>> test_vect = VectorTopo(test_vector_name)
            >>> test_vect.open('r')

            >>> bbox = Bbox(north=20, south=-1, east=20, west=-1)
            >>> result = test_vect.features_to_wkb_list(bbox=bbox,
            ...                                         feature_type="point")
            >>> len(result)
            3
            >>> for entry in result:
            ...     f_id, cat, wkb = entry
            ...     print((f_id, cat, len(wkb)))
            (1, 1, 21)
            (2, 1, 21)
            (3, 1, 21)

            >>> result = test_vect.features_to_wkb_list(bbox=None,
            ...                                         feature_type="line")
            >>> len(result)
            3
            >>> for entry in result:
            ...     f_id, cat, wkb = entry
            ...     print((f_id, cat, len(wkb)))
            (4, 2, 57)
            (5, 2, 57)
            (6, 2, 57)

            >>> result = test_vect.features_to_wkb_list(bbox=bbox,
            ...                                         feature_type="boundary")
            >>> len(result)
            11

            >>> result = test_vect.features_to_wkb_list(bbox=None,
            ...                                         feature_type="centroid")
            >>> len(result)
            4

            >>> for entry in result:
            ...     f_id, cat, wkb = entry
            ...     print((f_id, cat, len(wkb)))
            (19, 3, 21)
            (18, 3, 21)
            (20, 3, 21)
            (21, 3, 21)

            >>> result = test_vect.features_to_wkb_list(bbox=bbox,
            ...                                         feature_type="blub")
            Traceback (most recent call last):
            ...
            grass.exceptions.GrassError: Unsupported feature type <blub>, supported are <point,line,boundary,centroid>

            >>> test_vect.close()

        """

        supported = ['point', 'line', 'boundary', 'centroid']

        if feature_type.lower() not in supported:
            raise GrassError("Unsupported feature type <%s>, "\
                             "supported are <%s>"%(feature_type,
                                                   ",".join(supported)))

        if bbox is None:
            bbox = self.bbox()

        bboxlist = self.find_by_bbox.geos(bbox, type=feature_type.lower(),
                                          bboxlist_only = True)

        if bboxlist is not None and len(bboxlist) > 0:

            l = []
            line_p = libvect.line_pnts()
            line_c = libvect.line_cats()
            size = ctypes.c_size_t()
            cat = ctypes.c_int()
            error = ctypes.c_int()

            for f_id in bboxlist.ids:
                barray = libvect.Vect_read_line_to_wkb(self.c_mapinfo,
                                                       ctypes.byref(line_p),
                                                       ctypes.byref(line_c),
                                                       f_id,
                                                       ctypes.byref(size),
                                                       ctypes.byref(error))
                if not barray:
                    if error == -1:
                        raise GrassError(_("Unable to read line of feature %i"%(f_id)))
                    if error == -2:
                        print("Empty feature %i"%(f_id))
                    continue

                ok = libvect.Vect_cat_get(ctypes.byref(line_c), field,
                                          ctypes.byref(cat))
                if ok < 1:
                    pcat = None
                else:
                    pcat = cat.value

                l.append((f_id, pcat, ctypes.string_at(barray, size.value)))
                libgis.G_free(barray)

            return l
        return None

    @must_be_open
    def areas_to_wkb_list(self, bbox=None, field=1):
        """Return all features of type point, line, boundary or centroid
           as a list of Well Known Binary representations (WKB)
           (id, cat, wkb) triplets located in a specific
           bounding box.

           :param bbox: The boundingbox to search for features,
                       if bbox=None the boundingbox of the whole
                       vector map layer is used

           :type bbox: grass.pygrass.vector.basic.Bbox

           :param field: The centroid category field
           :type field: integer

           :return: A list of triplets, or None if nothing was found

           The well known binary are stored in byte arrays.

            Examples:

            >>> from grass.pygrass.vector import VectorTopo
            >>> from grass.pygrass.vector.basic import Bbox
            >>> test_vect = VectorTopo(test_vector_name)
            >>> test_vect.open('r')

            >>> bbox = Bbox(north=20, south=-1, east=20, west=-1)
            >>> result = test_vect.areas_to_wkb_list(bbox=bbox)
            >>> len(result)
            4
            >>> for entry in result:
            ...     a_id, cat, wkb = entry
            ...     print((a_id, cat, len(wkb)))
            (1, 3, 225)
            (2, 3, 141)
            (3, 3, 93)
            (4, 3, 141)

            >>> result = test_vect.areas_to_wkb_list()
            >>> len(result)
            4
            >>> for entry in result:
            ...     a_id, cat, wkb = entry
            ...     print((a_id, cat, len(wkb)))
            (1, 3, 225)
            (2, 3, 141)
            (3, 3, 93)
            (4, 3, 141)

            >>> test_vect.close()


        """
        if bbox is None:
            bbox = self.bbox()

        bboxlist = self.find_by_bbox.areas(bbox, bboxlist_only = True)

        if bboxlist is not None and len(bboxlist) > 0:

            l = []
            line_c = libvect.line_cats()
            size = ctypes.c_size_t()
            cat = ctypes.c_int()

            for a_id in bboxlist.ids:
                barray = libvect.Vect_read_area_to_wkb(self.c_mapinfo,
                                                       a_id,
                                                       ctypes.byref(size))
                if not barray:
                    raise GrassError(_("Unable to read area with id %i"%(a_id)))

                pcat = None
                c_ok = libvect.Vect_get_area_cats(self.c_mapinfo, a_id,
                                                  ctypes.byref(line_c))
                if c_ok == 0: # Centroid found

                    ok = libvect.Vect_cat_get(ctypes.byref(line_c), field,
                                              ctypes.byref(cat))
                    if ok > 0:
                        pcat = cat.value

                l.append((a_id, pcat, ctypes.string_at(barray, size.value)))
                libgis.G_free(barray)

            return l
        return None

if __name__ == "__main__":
    import doctest
    from grass.pygrass import utils
    utils.create_test_vector_map(test_vector_name)
    doctest.testmod()

    """Remove the generated vector map, if exist"""
    from grass.pygrass.utils import get_mapset_vector
    from grass.script.core import run_command
    mset = get_mapset_vector(test_vector_name, mapset='')
    if mset:
        run_command("g.remove", flags='f', type='vector', name=test_vector_name)

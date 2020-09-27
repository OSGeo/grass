# -*- coding: utf-8 -*-
"""
Created on Tue Mar 19 11:09:30 2013

@author: pietro
"""
import grass.lib.vector as libvect

from grass.pygrass.errors import must_be_open

from grass.pygrass.vector.basic import Ilist, BoxList
from grass.pygrass.vector.geometry import read_line, Isle, Area, Point, Node

# For test purposes
test_vector_name = "find_doctest_map"

class AbstractFinder(object):
    def __init__(self, c_mapinfo, table=None, writeable=False):
        """Abstract finder
        -----------------

        Find geometry feature around a point.
        """
        self.c_mapinfo = c_mapinfo
        self.table = table
        self.writeable = writeable
        self.vtype = {'point': libvect.GV_POINT,  # 1
                      'line': libvect.GV_LINE,   # 2
                      'boundary': libvect.GV_BOUNDARY,  # 3
                      'centroid': libvect.GV_CENTROID,  # 4
                      'all': -1}

    def is_open(self):
        """Check if the vector map is open or not"""
        from . import abstract
        return abstract.is_open(self.c_mapinfo)

class PointFinder(AbstractFinder):
    """Point finder

        This class provides an interface to search geometry features
        of a vector map that are close to a point. The PointFinder class
        is part of a topological vector map object.
    """

    def __init__(self, c_mapinfo, table=None, writeable=False):
        """Find geometry feature(s) around a point.

            :param c_mapinfo: Pointer to the vector layer mapinfo structure
            :type c_mapinfo: ctypes pointer to mapinfo structure

            :param table: Attribute table of the vector layer
            :type table: Class Table from grass.pygrass.table

            :param writable: True or False
            :type writeable: boolean
        """
        super(PointFinder, self).__init__(c_mapinfo, table, writeable)

    @must_be_open
    def node(self, point, maxdist):
        """Find the nearest node around a specific point.

            :param point: The point to search
            :type point: grass.pygrass.vector.geometry.Point

            :param maxdist: The maximum search distance around the point
            :type maxdist: float

            :return: A grass.pygrass.vector.geometry.Node if found or None

            This methods uses libvect.Vect_find_node()()

            Examples:

            >>> from grass.pygrass.vector import VectorTopo
            >>> from grass.pygrass.vector.geometry import Point
            >>> test_vect = VectorTopo(test_vector_name)
            >>> test_vect.open('r')

            # Find nearest node
            >>> points = (Point(10,0), Point(10,4), Point(14,0))
            >>> result = []
            >>> for point in points:
            ...     f = test_vect.find_by_point.node(point=point, maxdist=1)
            ...     if f:
            ...         result.append(f)
            >>> result
            [Node(2), Node(1), Node(6)]

            >>> test_vect.find_by_point.node(point=Point(20,20), maxdist=0)

            >>> test_vect.close()
        """
        node_id = libvect.Vect_find_node(self.c_mapinfo, point.x,
                                         point.y,
                                         point.z if point.z else 0,
                                         float(maxdist),
                                         int(not point.is2D))
        if node_id:
            return Node(v_id=node_id, c_mapinfo=self.c_mapinfo,
                 table=self.table, writeable=self.writeable)

    @must_be_open
    def geo(self, point, maxdist, type='all', exclude=0):
        """Find the nearest vector feature around a specific point.

            :param point: The point to search
            :type point: grass.pygrass.vector.geometry.Point

            :param maxdist: The maximum search distance around the point
            :type maxdist: float

            :param type: The type of feature to search for
                         Valid type are all the keys in find.vtype dictionary
            :type type: string

            :param exclude: if > 0 number of lines which should be
                            excluded from selection

            :return: A grass.pygrass.vector.geometry.Node if found or None

            This methods uses libvect.Vect_find_line()()

            Examples:

            >>> from grass.pygrass.vector import VectorTopo
            >>> from grass.pygrass.vector.geometry import Point
            >>> test_vect = VectorTopo(test_vector_name)
            >>> test_vect.open('r')

            # Find single features
            >>> points = (Point(10,0), Point(10,6), Point(14,2))
            >>> result = []
            >>> for point in points:
            ...     f = test_vect.find_by_point.geo(point=point, maxdist=1)
            ...     if f:
            ...         result.append(f)
            >>> for f in result:
            ...     print(f.to_wkt_p())    #doctest: +NORMALIZE_WHITESPACE
            LINESTRING(10.000000 4.000000,
                       10.000000 2.000000,
                       10.000000 0.000000)
            POINT(10.000000 6.000000)
            LINESTRING(14.000000 4.000000,
                       14.000000 2.000000,
                       14.000000 0.000000)

            >>> test_vect.find_by_point.geo(point=Point(20,20), maxdist=0)

            >>> test_vect.close()
        """
        feature_id = libvect.Vect_find_line(self.c_mapinfo,
                                            point.x, point.y,
                                            point.z if point.z else 0,
                                            self.vtype[type], float(maxdist),
                                            int(not point.is2D), exclude)
        if feature_id:
            return read_line(feature_id, self.c_mapinfo,
                             self.table, self.writeable)

    @must_be_open
    def geos(self, point, maxdist, type='all', exclude=None):
        """Find the nearest vector features around a specific point.

            :param point: The point to search
            :type point: grass.pygrass.vector.geometry.Point

            :param maxdist: The maximum search distance around the point
            :type maxdist: float

            :param type: The type of feature to search for
                         Valid type are all the keys in find.vtype dictionary
            :type type: string

            :param exclude: if > 0 number of lines which should be
                            excluded from selection

            :return: A list of grass.pygrass.vector.geometry
                     (Line, Point, Boundary, Centroid) if found or None

            This methods uses libvect.Vect_find_line_list()()

            Examples:

            >>> from grass.pygrass.vector import VectorTopo
            >>> from grass.pygrass.vector.geometry import Point
            >>> test_vect = VectorTopo(test_vector_name)
            >>> test_vect.open('r')

            # Find multiple features
            >>> points = (Point(10,0), Point(10,5), Point(14,2))
            >>> result = []
            >>> for point in points:
            ...     f = test_vect.find_by_point.geos(point=point,
            ...                                      maxdist=1.5)
            ...     if f:
            ...         result.append(f)
            >>> for f in result:
            ...     print(f)             #doctest: +NORMALIZE_WHITESPACE
            [Line([Point(10.000000, 4.000000),
                   Point(10.000000, 2.000000),
                   Point(10.000000, 0.000000)])]
            [Line([Point(10.000000, 4.000000),
                   Point(10.000000, 2.000000),
                   Point(10.000000, 0.000000)]),
             Point(10.000000, 6.000000)]
            [Line([Point(14.000000, 4.000000),
                   Point(14.000000, 2.000000),
                   Point(14.000000, 0.000000)])]

            # Find multiple boundaries
            >>> point = Point(3,3)
            >>> result = test_vect.find_by_point.geos(point=Point(3,3),
            ...                                          type="boundary",
            ...                                          maxdist=1.5)
            >>> result                   #doctest: +NORMALIZE_WHITESPACE
            [Boundary([Point(0.000000, 4.000000), Point(4.000000, 4.000000)]),
             Boundary([Point(4.000000, 4.000000), Point(4.000000, 0.000000)]),
             Boundary([Point(1.000000, 1.000000), Point(1.000000, 3.000000),
                       Point(3.000000, 3.000000), Point(3.000000, 1.000000),
                       Point(1.000000, 1.000000)]),
             Boundary([Point(4.000000, 4.000000), Point(6.000000, 4.000000)])]

            # Find multiple centroids
            >>> point = Point(3,3)
            >>> result = test_vect.find_by_point.geos(point=Point(3,3),
            ...                                          type="centroid",
            ...                                          maxdist=1.5)
            >>> result                   #doctest: +NORMALIZE_WHITESPACE
            [Centroid(2.500000, 2.500000),
             Centroid(3.500000, 3.500000)]

            >>> test_vect.find_by_point.geos(point=Point(20,20), maxdist=0)

            >>> test_vect.close()
        """
        excl = Ilist(exclude) if exclude else Ilist([])
        found = Ilist()
        if libvect.Vect_find_line_list(self.c_mapinfo,
                                       point.x, point.y,
                                       point.z if point.z else 0,
                                       self.vtype[type], float(maxdist),
                                       int(not point.is2D),
                                       excl.c_ilist, found.c_ilist):
            return [read_line(f_id, self.c_mapinfo, self.table, self.writeable)
                    for f_id in found]


    @must_be_open
    def area(self, point):
        """Find the nearest area around a specific point.

            :param point: The point to search
            :type point: grass.pygrass.vector.geometry.Point

            :return: A grass.pygrass.vector.geometry.Area if found or None

            This methods uses libvect.Vect_find_area()

            Examples:

            >>> from grass.pygrass.vector import VectorTopo
            >>> from grass.pygrass.vector.geometry import Point
            >>> test_vect = VectorTopo(test_vector_name)
            >>> test_vect.open('r')

            # Find AREAS
            >>> points = (Point(0.5,0.5), Point(5,1), Point(7,1))
            >>> result = []
            >>> for point in points:
            ...     area = test_vect.find_by_point.area(point)
            ...     result.append(area)
            >>> result
            [Area(1), Area(2), Area(4)]
            >>> for area in result:
            ...     print(area.to_wkt())         #doctest: +NORMALIZE_WHITESPACE
            POLYGON ((0.0000000000000000 0.0000000000000000,
                      0.0000000000000000 4.0000000000000000,
                      0.0000000000000000 4.0000000000000000,
                      4.0000000000000000 4.0000000000000000,
                      4.0000000000000000 4.0000000000000000,
                      4.0000000000000000 0.0000000000000000,
                      4.0000000000000000 0.0000000000000000,
                      0.0000000000000000 0.0000000000000000),
                     (1.0000000000000000 1.0000000000000000,
                      3.0000000000000000 1.0000000000000000,
                      3.0000000000000000 3.0000000000000000,
                      1.0000000000000000 3.0000000000000000,
                      1.0000000000000000 1.0000000000000000))
            POLYGON ((4.0000000000000000 0.0000000000000000,
                      4.0000000000000000 4.0000000000000000,
                      4.0000000000000000 4.0000000000000000,
                      6.0000000000000000 4.0000000000000000,
                      6.0000000000000000 4.0000000000000000,
                      6.0000000000000000 0.0000000000000000,
                      6.0000000000000000 0.0000000000000000,
                      4.0000000000000000 0.0000000000000000))
            POLYGON ((6.0000000000000000 0.0000000000000000,
                      6.0000000000000000 4.0000000000000000,
                      6.0000000000000000 4.0000000000000000,
                      8.0000000000000000 4.0000000000000000,
                      8.0000000000000000 4.0000000000000000,
                      8.0000000000000000 0.0000000000000000,
                      8.0000000000000000 0.0000000000000000,
                      6.0000000000000000 0.0000000000000000))

            >>> test_vect.find_by_point.area(Point(20,20))

            >>> test_vect.close()
        """
        area_id = libvect.Vect_find_area(self.c_mapinfo, point.x, point.y)
        if area_id:
            return Area(v_id=area_id, c_mapinfo=self.c_mapinfo,
                        table=self.table, writeable=self.writeable)

    @must_be_open
    def island(self, point):
        """Find the nearest island around a specific point.

            :param point: The point to search
            :type point: grass.pygrass.vector.geometry.Point

            :return: A grass.pygrass.vector.geometry.Isle if found or None

            This methods uses Vect_find_island.Vect_find_area()

            Examples:

            >>> from grass.pygrass.vector import VectorTopo
            >>> from grass.pygrass.vector.geometry import Point
            >>> test_vect = VectorTopo(test_vector_name)
            >>> test_vect.open('r')

            # Find ISLANDS
            >>> points = (Point(2,2), Point(5,1))
            >>> result = []
            >>> for point in points:
            ...     area = test_vect.find_by_point.island(point)
            ...     result.append(area)
            >>> result
            [Isle(2), Isle(1)]
            >>> for isle in result:
            ...     print(isle.to_wkt())         #doctest: +NORMALIZE_WHITESPACE
            Polygon((1.000000 1.000000, 3.000000 1.000000,
                     3.000000 3.000000, 1.000000 3.000000, 1.000000 1.000000))
            Polygon((0.000000 4.000000, 0.000000 0.000000, 4.000000 0.000000,
                     6.000000 0.000000, 8.000000 0.000000, 8.000000 4.000000,
                     6.000000 4.000000, 4.000000 4.000000, 0.000000 4.000000))

            >>> test_vect.find_by_point.island(Point(20,20))

            >>> test_vect.close()
        """
        isle_id = libvect.Vect_find_island(self.c_mapinfo, point.x, point.y)
        if isle_id:
            return Isle(v_id=isle_id, c_mapinfo=self.c_mapinfo,
                        table=self.table, writeable=self.writeable)


class BboxFinder(AbstractFinder):
    """Bounding Box finder

    This class provides an interface to search geometry features
    of a vector map that are inside or intersect a boundingbox.
    The BboxFinder class is part of a topological vector map object.

    """

    def __init__(self, c_mapinfo, table=None, writeable=False):
        """Find geometry feature(s)that are insider or intersect
           with a boundingbox.

            :param c_mapinfo: Pointer to the vector layer mapinfo structure
            :type c_mapinfo: ctypes pointer to mapinfo structure

            :param table: Attribute table of the vector layer
            :type table: Class Table from grass.pygrass.table

            :param writable: True or False
            :type writeable: boolean
        """
        super(BboxFinder, self).__init__(c_mapinfo, table, writeable)

    @must_be_open
    def geos(self, bbox, type='all', bboxlist_only=False):
        """Find vector features inside a boundingbox.

            :param bbox: The boundingbox to search in
            :type bbox: grass.pygrass.vector.basic.Bbox

            :param type: The type of feature to search for
                         Valid type are all the keys in find.vtype dictionary
            :type type: string

            :param bboxlist_only: If true the BoxList will be returned,
                                  no features are generated
            :type bboxlist_only: boolean

            :return: A list of grass.pygrass.vector.geometry
                     (Line, Point, Boundary, Centroid) if found,
                     or None if nothing was found.
                     If bboxlist_only is True a BoxList
                     object will be returned, or None if nothing was found.

            This methods uses libvect.Vect_select_lines_by_box()

            Examples:

            >>> from grass.pygrass.vector import VectorTopo
            >>> from grass.pygrass.vector.basic import Bbox
            >>> test_vect = VectorTopo(test_vector_name)
            >>> test_vect.open('r')

            >>> bbox = Bbox(north=5, south=-1, east=3, west=-1)
            >>> result = test_vect.find_by_bbox.geos(bbox=bbox)
            >>> [bbox for bbox in result] #doctest: +NORMALIZE_WHITESPACE
            [Boundary([Point(4.000000, 0.000000), Point(0.000000, 0.000000)]),
             Boundary([Point(0.000000, 0.000000), Point(0.000000, 4.000000)]),
             Boundary([Point(0.000000, 4.000000), Point(4.000000, 4.000000)]),
             Boundary([Point(1.000000, 1.000000), Point(1.000000, 3.000000),
                       Point(3.000000, 3.000000), Point(3.000000, 1.000000),
                       Point(1.000000, 1.000000)]),
             Centroid(2.500000, 2.500000)]

            >>> bbox = Bbox(north=5, south=-1, east=3, west=-1)
            >>> result = test_vect.find_by_bbox.geos(bbox=bbox,
            ...                                      bboxlist_only=True)
            >>> result                   #doctest: +NORMALIZE_WHITESPACE
            Boxlist([Bbox(0.0, 0.0, 4.0, 0.0),
                     Bbox(4.0, 0.0, 0.0, 0.0),
                     Bbox(4.0, 4.0, 4.0, 0.0),
                     Bbox(3.0, 1.0, 3.0, 1.0),
                     Bbox(2.5, 2.5, 2.5, 2.5)])

            >>> bbox = Bbox(north=7, south=-1, east=15, west=9)
            >>> result = test_vect.find_by_bbox.geos(bbox=bbox)
            >>> [bbox for bbox in result] #doctest: +NORMALIZE_WHITESPACE
            [Line([Point(10.000000, 4.000000), Point(10.000000, 2.000000),
                   Point(10.000000, 0.000000)]),
             Point(10.000000, 6.000000),
             Line([Point(12.000000, 4.000000), Point(12.000000, 2.000000),
                   Point(12.000000, 0.000000)]),
             Point(12.000000, 6.000000),
             Line([Point(14.000000, 4.000000), Point(14.000000, 2.000000),
                   Point(14.000000, 0.000000)]),
             Point(14.000000, 6.000000)]

            >>> bbox = Bbox(north=20, south=18, east=20, west=18)
            >>> test_vect.find_by_bbox.geos(bbox=bbox)

            >>> bbox = Bbox(north=20, south=18, east=20, west=18)
            >>> test_vect.find_by_bbox.geos(bbox=bbox, bboxlist_only=True)

            >>> test_vect.close()
        """
        found = BoxList()
        if libvect.Vect_select_lines_by_box(self.c_mapinfo, bbox.c_bbox,
                                            self.vtype[type], found.c_boxlist):
            if bboxlist_only:
                return found
            else:
                return (read_line(f_id, self.c_mapinfo, self.table,
                                  self.writeable) for f_id in found.ids)

    @must_be_open
    def nodes(self, bbox):
        """Find nodes inside a boundingbox.

            :param bbox: The boundingbox to search in
            :type bbox: grass.pygrass.vector.basic.Bbox

            :return: A list of nodes or None if nothing was found

            This methods uses libvect.Vect_select_nodes_by_box()

            Examples:

            >>> from grass.pygrass.vector import VectorTopo
            >>> from grass.pygrass.vector.basic import Bbox
            >>> test_vect = VectorTopo(test_vector_name)
            >>> test_vect.open('r')

            # Find nodes in box
            >>> bbox = Bbox(north=5, south=-1, east=15, west=9)
            >>> result = test_vect.find_by_bbox.nodes(bbox=bbox)
            >>> [node for node in result]
            [Node(2), Node(1), Node(4), Node(3), Node(5), Node(6)]

            >>> bbox = Bbox(north=20, south=18, east=20, west=18)
            >>> test_vect.find_by_bbox.nodes(bbox=bbox)

            >>> test_vect.close()
        """
        found = Ilist()
        if libvect.Vect_select_nodes_by_box(self.c_mapinfo, bbox.c_bbox,
                                            found.c_ilist):
            if len(found) > 0:
                return (Node(v_id=n_id, c_mapinfo=self.c_mapinfo,
                             table=self.table, writeable=self.writeable)
                        for n_id in found)

    @must_be_open
    def areas(self, bbox, boxlist=None, bboxlist_only=False):
        """Find areas inside a boundingbox.

            :param bbox: The boundingbox to search in
            :type bbox: grass.pygrass.vector.basic.Bbox

            :param boxlist: An existing BoxList to be filled with
            :type_boxlist: grass.pygrass.vector.basic.BoxList

            :param bboxlist_only: If true the BoxList will be returned,
                                  no features are generated
            :type bboxlist_only: boolean

            :return: A list of areas or None if nothing was found

            This methods uses libvect.Vect_select_areas_by_box()

            Examples:

            >>> from grass.pygrass.vector import VectorTopo
            >>> from grass.pygrass.vector.basic import Bbox
            >>> test_vect = VectorTopo(test_vector_name)
            >>> test_vect.open('r')

            # Find areas in box
            >>> bbox = Bbox(north=5, south=-1, east=9, west=-1)
            >>> result = test_vect.find_by_bbox.areas(bbox=bbox)
            >>> [area for area in result]
            [Area(1), Area(2), Area(3), Area(4)]

            >>> bbox = Bbox(north=5, south=-1, east=9, west=-1)
            >>> result = test_vect.find_by_bbox.areas(bbox=bbox,
            ...                                       bboxlist_only=True)
            >>> result                   #doctest: +NORMALIZE_WHITESPACE
            Boxlist([Bbox(4.0, 0.0, 4.0, 0.0),
                     Bbox(4.0, 0.0, 6.0, 4.0),
                     Bbox(3.0, 1.0, 3.0, 1.0),
                     Bbox(4.0, 0.0, 8.0, 6.0)])

            >>> bbox = Bbox(north=20, south=18, east=20, west=18)
            >>> test_vect.find_by_bbox.areas(bbox=bbox)

            >>> test_vect.find_by_bbox.areas(bbox=bbox,
            ...                              bboxlist_only=True)

            >>> test_vect.close()
        """
        boxlist = boxlist if boxlist else BoxList()
        if libvect.Vect_select_areas_by_box(self.c_mapinfo, bbox.c_bbox,
                                            boxlist.c_boxlist):
            if bboxlist_only:
                return boxlist
            else:
                return (Area(v_id=a_id, c_mapinfo=self.c_mapinfo,
                             table=self.table, writeable=self.writeable)
                        for a_id in boxlist.ids)

    @must_be_open
    def islands(self, bbox, bboxlist_only=False):
        """Find isles inside a boundingbox.

            :param bbox: The boundingbox to search in
            :type bbox: grass.pygrass.vector.basic.Bbox

            :param bboxlist_only: If true the BoxList will be returned,
                                  no features are generated
            :type bboxlist_only: boolean

            :return: A list of isles or None if nothing was found

            This methods uses libvect.Vect_select_isles_by_box()

            Examples:

            >>> from grass.pygrass.vector import VectorTopo
            >>> from grass.pygrass.vector.basic import Bbox
            >>> test_vect = VectorTopo(test_vector_name)
            >>> test_vect.open('r')

            # Find isles in box
            >>> bbox = Bbox(north=5, south=-1, east=9, west=-1)
            >>> result = test_vect.find_by_bbox.islands(bbox=bbox)
            >>> [isle for isle in result]
            [Isle(1), Isle(2)]

            >>> bbox = Bbox(north=5, south=-1, east=9, west=-1)
            >>> result = test_vect.find_by_bbox.islands(bbox=bbox,
            ...                                       bboxlist_only=True)
            >>> result                   #doctest: +NORMALIZE_WHITESPACE
            Boxlist([Bbox(4.0, 0.0, 8.0, 0.0),
                     Bbox(3.0, 1.0, 3.0, 1.0)])

            >>> bbox = Bbox(north=20, south=18, east=20, west=18)
            >>> test_vect.find_by_bbox.islands(bbox=bbox)

            >>> test_vect.find_by_bbox.islands(bbox=bbox,
            ...                              bboxlist_only=True)

            >>> test_vect.close()
        """
        found = BoxList()
        if libvect.Vect_select_isles_by_box(self.c_mapinfo, bbox.c_bbox,
                                            found.c_boxlist):
            if bboxlist_only:
                return found
            else:
                return (Isle(v_id=i_id, c_mapinfo=self.c_mapinfo,
                             table=self.table, writeable=self.writeable)
                        for i_id in found.ids)


class PolygonFinder(AbstractFinder):
    def __init__(self, c_mapinfo, table=None, writeable=False):
        super(PolygonFinder, self).__init__(c_mapinfo, table, writeable)

    def lines(self, polygon, isles=None):
        pass

    def areas(self, polygon, isles=None):
        pass


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

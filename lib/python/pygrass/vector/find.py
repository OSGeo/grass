# -*- coding: utf-8 -*-
"""
Created on Tue Mar 19 11:09:30 2013

@author: pietro
"""
import grass.lib.vector as libvect

from grass.pygrass.errors import must_be_open

from basic import Ilist, BoxList
from geometry import read_line, Isle, Area, Point


class AbstractFinder(object):
    def __init__(self, c_mapinfo, table=None, writable=False):
        """Find geometry feature around a point.
        """
        self.c_mapinfo = c_mapinfo
        self.table = table
        self.writable = writable
        self.vtype = {'point':    libvect.GV_POINT,  # 1
                      'line':     libvect.GV_LINE,   # 2
                      'boundary': libvect.GV_BOUNDARY,  # 3
                      'centroid': libvect.GV_CENTROID,  # 4
                      'all': -1}

    def is_open(self):
        """Check if the vector map is open or not"""
        import abstract
        return abstract.is_open(self.c_mapinfo)


class PointFinder(AbstractFinder):
    """Find the geomtry features of a vector map that are close to a point. ::

        >>> from grass.pygrass.vector import VectorTopo
        >>> zipcodes = VectorTopo('zipcodes', 'PERMANENT')
        >>> schools = VectorTopo('schools', 'PERMANENT')
        >>> zipcodes.open('r')
        >>> schools.open('r')
        >>> result = []
        >>> for school in schools:
        ...         zipcode = zipcodes.find.area(school)
        ...         result.append((school.attrs['NAMESHORT'],
        ...                        zipcode.attrs['ZIPCODE']))
        ...
        >>> result[0]
        (u'SWIFT CREEK', u'RALEIGH 27606')
        >>> result[1]
        (u'BRIARCLIFF', u'CARY 27511')
        >>> result[2]
        (u'FARMINGTON WOODS', u'CARY 27511')
        >>> from grass.pygrass.vector.geometry import Point
        >>> pnt = Point(631213.349291, 224684.900084)
        >>> school = schools.find.geo(pnt, maxdist=300.)
        >>> school.attrs['NAMELONG']
        u'ADAMS ELEMENTARY'
        >>> for school in schools.find.geos(pnt, maxdist=1000.):
        ...     print school.attrs['NAMELONG']
        ...
        CARY HIGH
        EAST CARY MIDDLE SITE
        ADAMS ELEMENTARY
        >>> schools.close()
        >>> zipcodes.close()
    """
    def __init__(self, c_mapinfo, table=None, writable=False):
        """Find geometry feature around a point.
        """
        super(PointFinder, self).__init__(c_mapinfo, table, writable)

# TODO: add the Node class and enable this method
#    def node(self, point, maxdist):
#        """Find the nearest node. Vect_find_node"""
#        i = libvect.Vect_find_node(self.c_mapinfo, point.x, point.y, point.z,
#                                   float(maxdist), int(not point.is2D))
#        return geometry.Node(self.c_mapinfo.contents.plus.contents.Node[i])

    @must_be_open
    def geo(self, point, maxdist, type='all', exclude=0):
        """Find the nearest line. Vect_find_line

        Valid type are all the keys in find.vtype dictionary
        """
        feature_id = libvect.Vect_find_line(self.c_mapinfo,
                                            point.x, point.y,
                                            point.z if point.z else 0,
                                            self.vtype[type], float(maxdist),
                                            int(not point.is2D), exclude)
        if feature_id:
            return read_line(feature_id, self.c_mapinfo,
                             self.table, self.writable)

    @must_be_open
    def geos(self, point, maxdist, type='all', exclude=None):
        """Find the nearest line. Vect_find_line_list

        Valid type are all the keys in find.vtype dictionary
        """
        excl = Ilist(exclude) if exclude else Ilist([])
        found = Ilist()
        if libvect.Vect_find_line_list(self.c_mapinfo,
                                       point.x, point.y,
                                       point.z if point.z else 0,
                                       self.vtype[type], float(maxdist),
                                       int(not point.is2D),
                                       excl.c_ilist, found.c_ilist):
            return [read_line(f_id, self.c_mapinfo, self.table, self.writable)
                    for f_id in found]
        else:
            return []

    @must_be_open
    def area(self, point):
        """Find the nearest area. Vect_find_area"""
        area_id = libvect.Vect_find_area(self.c_mapinfo, point.x, point.y)
        if area_id:
            return Area(v_id=area_id, c_mapinfo=self.c_mapinfo,
                        table=self.table, writable=self.writable)

    @must_be_open
    def island(self, point):
        """Find the nearest island. Vect_find_island"""
        isle_id = libvect.Vect_find_island(self.c_mapinfo, point.x, point.y)
        if isle_id:
            return Isle(v_id=isle_id, c_mapinfo=self.c_mapinfo,
                        table=self.table, writable=self.writable)


class BboxFinder(AbstractFinder):
    def __init__(self, c_mapinfo, table=None, writable=False):
        super(BboxFinder, self).__init__(c_mapinfo, table, writable)

    @must_be_open
    def geos(self, bbox, type='all', bbox_list=False):
        """Find the geometry features contained in the bbox.
        Vect_select_lines_by_box

        Valid type are all the keys in find.vtype dictionary
        """
        found = BoxList()
        if libvect.Vect_select_lines_by_box(self.c_mapinfo, bbox.c_bbox,
                                            self.vtype[type], found.c_boxlist):
            if bbox_list:
                return found
            else:
                return (read_line(f_id, self.c_mapinfo, self.table,
                                  self.writable) for f_id in found.ids)

    @must_be_open
    def nodes(self, bbox):
        """Find the nearest area. Vect_find_area"""
        found = Ilist()
        if libvect.Vect_select_nodes_by_box(self.c_mapinfo, bbox.c_bbox,
                                            found.c_ilist):
            for n_id in found:
                yield Point(v_id=n_id, c_mapinfo=self.c_mapinfo,
                            table=self.table, writable=self.writable)

    @must_be_open
    def areas(self, bbox, bbox_list=False):
        """Find the nearest area. Vect_find_area"""
        found = BoxList()
        if libvect.Vect_select_areas_by_box(self.c_mapinfo, bbox.c_bbox,
                                            found.c_boxlist):
            if bbox_list:
                return found
            else:
                return (Area(v_id=a_id, c_mapinfo=self.c_mapinfo,
                             table=self.table, writable=self.writable)
                        for a_id in found.ids)
        return []

    @must_be_open
    def islands(self, bbox, bbox_list=False):
        """Find the nearest island. Vect_find_island"""
        found = BoxList()
        if libvect.Vect_select_isles_by_box(self.c_mapinfo, bbox.c_bbox,
                                            found.c_boxlist):
            if bbox_list:
                return found
            else:
                return (Isle(v_id=i_id, c_mapinfo=self.c_mapinfo,
                             table=self.table, writable=self.writable)
                        for i_id in found.ids)
        return []


class PolygonFinder(AbstractFinder):
    def __init__(self, c_mapinfo, table=None, writable=False):
        super(PolygonFinder, self).__init__(c_mapinfo, table, writable)

    def lines(self, polygon, isles=None):
        pass

    def areas(self, polygon, isles=None):
        pass

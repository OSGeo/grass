# -*- coding: utf-8 -*-
"""
Created on Tue Mar 19 11:09:30 2013

@author: pietro
"""
import grass.lib.vector as libvect

from grass.pygrass.errors import must_be_open

from basic import Ilist
from geometry import read_line, Isle, Area


class Finder(object):
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
        self.c_mapinfo = c_mapinfo
        self.table = table
        self.writable = writable
        self.vtype = {'point':    libvect.GV_POINT,  # 1
                      'line':     libvect.GV_LINE,   # 2
                      'boundary': libvect.GV_BOUNDARY,  # 3
                      'centroid': libvect.GV_CENTROID,  # 4
                      'all': -1}  # -1

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
    def geos(self, point, maxdist, type='all', exclude=[]):
        """Find the nearest line. Vect_find_line_list

        Valid type are all the keys in find.vtype dictionary
        """
        excl = Ilist(exclude)
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

    def is_open(self):
        """Check if the vector map is open or not"""
        import abstract
        return abstract.is_open(self.c_mapinfo)

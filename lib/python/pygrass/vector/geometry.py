# -*- coding: utf-8 -*-
"""
Created on Wed Jul 18 10:46:25 2012

@author: pietro

"""
import ctypes
import re
from collections import namedtuple

import numpy as np

import grass.lib.gis as libgis
import grass.lib.vector as libvect

from grass.pygrass.errors import GrassError

from grass.pygrass.vector.basic import Ilist, Bbox, Cats
from grass.pygrass.vector import sql


LineDist = namedtuple('LineDist', 'point dist spdist sldist')

WKT = {'POINT\((.*)\)': 'point',  # 'POINT\(\s*([+-]*\d+\.*\d*)+\s*\)'
       'LINESTRING\((.*)\)': 'line'}


def read_WKT(string):
    """Read the string and return a geometry object

    **WKT**:
    ::

        POINT(0 0)
        LINESTRING(0 0,1 1,1 2)
        POLYGON((0 0,4 0,4 4,0 4,0 0),(1 1, 2 1, 2 2, 1 2,1 1))
        MULTIPOINT(0 0,1 2)
        MULTILINESTRING((0 0,1 1,1 2),(2 3,3 2,5 4))
        MULTIPOLYGON(((0 0,4 0,4 4,0 4,0 0),(1 1,2 1,2 2,1 2,1 1)),
                     ((-1 -1,-1 -2,-2 -2,-2 -1,-1 -1)))
        GEOMETRYCOLLECTION(POINT(2 3),LINESTRING(2 3,3 4))

    **EWKT**:

    ::

        POINT(0 0 0) -- XYZ
        SRID=32632;POINT(0 0) -- XY with SRID
        POINTM(0 0 0) -- XYM
        POINT(0 0 0 0) -- XYZM
        SRID=4326;MULTIPOINTM(0 0 0,1 2 1) -- XYM with SRID
        MULTILINESTRING((0 0 0,1 1 0,1 2 1),(2 3 1,3 2 1,5 4 1))
        POLYGON((0 0 0,4 0 0,4 4 0,0 4 0,0 0 0),(1 1 0,2 1 0,2 2 0,1 2 0,1 1 0))
        MULTIPOLYGON(((0 0 0,4 0 0,4 4 0,0 4 0,0 0 0),
                      (1 1 0,2 1 0,2 2 0,1 2 0,1 1 0)),
                     ((-1 -1 0,-1 -2 0,-2 -2 0,-2 -1 0,-1 -1 0)))
        GEOMETRYCOLLECTIONM( POINTM(2 3 9), LINESTRINGM(2 3 4, 3 4 5) )
        MULTICURVE( (0 0, 5 5), CIRCULARSTRING(4 0, 4 4, 8 4) )
        POLYHEDRALSURFACE( ((0 0 0, 0 0 1, 0 1 1, 0 1 0, 0 0 0)),
                           ((0 0 0, 0 1 0, 1 1 0, 1 0 0, 0 0 0)),
                           ((0 0 0, 1 0 0, 1 0 1, 0 0 1, 0 0 0)),
                           ((1 1 0, 1 1 1, 1 0 1, 1 0 0, 1 1 0)),
                           ((0 1 0, 0 1 1, 1 1 1, 1 1 0, 0 1 0)),
                           ((0 0 1, 1 0 1, 1 1 1, 0 1 1, 0 0 1)) )
        TRIANGLE ((0 0, 0 9, 9 0, 0 0))
        TIN( ((0 0 0, 0 0 1, 0 1 0, 0 0 0)), ((0 0 0, 0 1 0, 1 1 0, 0 0 0)) )

    """
    for regexp, obj in WKT.items():
        if re.match(regexp, string):
            geo = 10
            return obj(geo)


def read_WKB(buff):
    """Read the binary buffer and return a geometry object"""
    pass


def intersects(lineA, lineB, with_z=False):
    """Return a list of points

    >>> lineA = Line([(0, 0), (4, 0)])
    >>> lineB = Line([(2, 2), (2, -2)])
    >>> intersects(lineA, lineB)
    Line([Point(2.000000, 0.000000)])
    """
    line = Line()
    if libvect.Vect_line_get_intersections(lineA.c_points, lineB.c_points,
                                           line.c_points, int(with_z)):
        return line
    else:
        return []

#=============================================
# GEOMETRY
#=============================================


def get_xyz(pnt):
    """Return a tuple with: x, y, z.

    >>> pnt = Point(0, 0)
    >>> get_xyz(pnt)
    (0.0, 0.0, 0.0)
    >>> get_xyz((1, 1))
    (1, 1, 0.0)
    >>> get_xyz((1, 1, 2))
    (1, 1, 2)
    >>> get_xyz((1, 1, 2, 2))                          #doctest: +ELLIPSIS
    Traceback (most recent call last):
        ...
    ValueError: The the format of the point is not supported: (1, 1, 2, 2)

    """
    if isinstance(pnt, Point):
        if pnt.is2D:
            x, y = pnt.x, pnt.y
            z = 0.
        else:
            x, y, z = pnt.x, pnt.y, pnt.z
    else:
        if len(pnt) == 2:
            x, y = pnt
            z = 0.
        elif len(pnt) == 3:
            x, y, z = pnt
        else:
            str_error = "The the format of the point is not supported: {0!r}"
            raise ValueError(str_error.format(pnt))
    return x, y, z


class Attrs(object):
    def __init__(self, cat, table, writable=False):
        self._cat = None
        self.cond = ''
        self.table = table
        self.cat = cat
        self.writable = writable

    def _get_cat(self):
        return self._cat

    def _set_cat(self, value):
        self._cat = value
        if value:
            # update condition
            self.cond = "%s=%d" % (self.table.key, value)

    cat = property(fget=_get_cat, fset=_set_cat,
                   doc="Set and obtain cat value")

    def __getitem__(self, key):
        """Return the value stored in the attribute table.

        >>> from grass.pygrass.vector import VectorTopo
        >>> schools = VectorTopo('schools')
        >>> schools.open('r')
        >>> school = schools[1]
        >>> attrs = Attrs(school.cat, schools.table)
        >>> attrs['TAG']
        u'568'

        """
        #SELECT {cols} FROM {tname} WHERE {condition};
        try:
            cur = self.table.execute(sql.SELECT_WHERE.format(cols=key,
                                                         tname=self.table.name,
                                                         condition=self.cond))
        except:
            import ipdb; ipdb.set_trace()
        results = cur.fetchone()
        if results is not None:
            return results[0] if len(results) == 1 else results

    def __setitem__(self, key, value):
        """Set value of a given column of a table attribute.

        >>> from grass.pygrass.vector import VectorTopo
        >>> from grass.pygrass.utils import copy, remove
        >>> copy('schools', 'myschools', 'vect')
        >>> schools = VectorTopo('myschools')
        >>> schools.open('r')
        >>> school = schools[1]
        >>> attrs = Attrs(school.cat, schools.table, True)
        >>> attrs['TAG'] = 'New Label'
        >>> attrs['TAG']
        u'New Label'
        >>> attrs.table.conn.close()
        >>> remove('myschools','vect')
        """
        if self.writable:
            #UPDATE {tname} SET {new_col} = {old_col} WHERE {condition}
            values = '%s=%r' % (key, value)
            self.table.execute(sql.UPDATE_WHERE.format(tname=self.table.name,
                                                       values=values,
                                                       condition=self.cond))
            #self.table.conn.commit()
        else:
            str_err = "You can only read the attributes if the map is in another mapset"
            raise GrassError(str_err)

    def __dict__(self):
        """Return a dict of the attribute table row."""
        dic = {}
        for key, val in zip(self.keys(), self.values()):
            dic[key] = val
        return dic

    def values(self):
        """Return the values of the attribute table row.
           >>> from grass.pygrass.vector import VectorTopo
           >>> schools = VectorTopo('schools')
           >>> schools.open('r')
           >>> school = schools[1]
           >>> attrs = Attrs(school.cat, schools.table)
           >>> attrs.values()       # doctest: +ELLIPSIS +NORMALIZE_WHITESPACE
           (1,
           ...
           None)

        """
        #SELECT {cols} FROM {tname} WHERE {condition}
        cur = self.table.execute(sql.SELECT_WHERE.format(cols='*',
                                                         tname=self.table.name,
                                                         condition=self.cond))
        return cur.fetchone()

    def keys(self):
        """Return the column name of the attribute table.
           >>> from grass.pygrass.vector import VectorTopo
           >>> schools = VectorTopo('schools')
           >>> schools.open('r')
           >>> school = schools[1]
           >>> attrs = Attrs(school.cat, schools.table)
           >>> attrs.keys()         # doctest: +ELLIPSIS +NORMALIZE_WHITESPACE
           [u'cat',
           ...
           u'NOTES']
        """
        return self.table.columns.names()

    def commit(self):
        """Save the changes"""
        self.table.conn.commit()


class Geo(object):
    """
    >>> geo0 = Geo()
    >>> points = ctypes.pointer(libvect.line_pnts())
    >>> cats = ctypes.pointer(libvect.line_cats())
    >>> geo1 = Geo(c_points=points, c_cats=cats)
    """
    gtype = None

    def __init__(self, v_id=0, c_mapinfo=None, c_points=None, c_cats=None,
                 table=None, writable=False, is2D=True):
        self.id = v_id  # vector id
        self.c_mapinfo = c_mapinfo
        self.is2D = (is2D if is2D is not None else
                     bool(libvect.Vect_is_3d(self.c_mapinfo) != 1))

        read = False
        # set c_points
        if c_points is None:
            self.c_points = ctypes.pointer(libvect.line_pnts())
            read = True
        else:
            self.c_points = c_points

        # set c_cats
        if c_cats is None:
            self.c_cats = ctypes.pointer(libvect.line_cats())
            read = True
        else:
            self.c_cats = c_cats

        # set the attributes
        self.attrs = None
        if table is not None:
            self.attrs = Attrs(self.cat, table, writable)

        if self.id and self.c_mapinfo is not None and read:
            self.read()

    @property
    def cat(self):
        if self.c_cats.contents.cat:
            return self.c_cats.contents.cat.contents.value

    def is_with_topology(self):
        if self.c_mapinfo is not None:
            return self.c_mapinfo.contents.level == 2
        else:
            return False

    def read(self):
        """Read and set the coordinates of the centroid from the vector map,
        using the centroid_id and calling the Vect_read_line C function"""
        self.id, ftype, c_points, c_cats = c_read_line(self.id, self.c_mapinfo,
                                                       self.c_points,
                                                       self.c_cats)


class Point(Geo):
    """Instantiate a Point object that could be 2 or 3D, default
    parameters are 0.

    ::

        >>> pnt = Point()
        >>> pnt.x
        0.0
        >>> pnt.y
        0.0
        >>> pnt.z
        >>> pnt.is2D
        True
        >>> pnt
        Point(0.000000, 0.000000)
        >>> pnt.z = 0
        >>> pnt.is2D
        False
        >>> pnt
        Point(0.000000, 0.000000, 0.000000)
        >>> print(pnt)
        POINT(0.000000 0.000000 0.000000)

    ..
    """
    # geometry type
    gtype = libvect.GV_POINT

    def __init__(self, x=0, y=0, z=None, **kargs):
        super(Point, self).__init__(**kargs)
        if self.id and self.c_mapinfo:
            self.read()
        else:
            self.is2D = True if z is None else False
            z = z if z is not None else 0
            libvect.Vect_append_point(self.c_points, x, y, z)

    def _get_x(self):
        return self.c_points.contents.x[0]

    def _set_x(self, value):
        self.c_points.contents.x[0] = value

    x = property(fget=_get_x, fset=_set_x,
                 doc="Set and obtain x coordinate")

    def _get_y(self):
        return self.c_points.contents.y[0]

    def _set_y(self, value):
        self.c_points.contents.y[0] = value

    y = property(fget=_get_y, fset=_set_y,
                 doc="Set and obtain y coordinate")

    def _get_z(self):
        if self.is2D:
            return None
        return self.c_points.contents.z[0]

    def _set_z(self, value):
        if value is None:
            self.is2D = True
            self.c_points.contents.z[0] = 0
        else:
            self.c_points.contents.z[0] = value
            self.is2D = False

    z = property(fget=_get_z, fset=_set_z,
                 doc="Set and obtain z coordinate")

    def __str__(self):
        return self.get_wkt()

    def __repr__(self):
        return "Point(%s)" % ', '.join(['%f' % coor for coor in self.coords()])

    def __eq__(self, pnt):
        """Return True if the coordinates are the same.

        >>> p0 = Point()
        >>> p1 = Point()
        >>> p2 = Point(1, 1)
        >>> p0 == p1
        True
        >>> p1 == p2
        False
        """
        if isinstance(pnt, Point):
            return pnt.coords() == self.coords()

        return Point(*pnt).coords() == self.coords()

    def __ne__(self, other):
        return not self == other

    # Restore Python 2 hashing beaviour on Python 3
    __hash__ = object.__hash__

    def coords(self):
        """Return a tuple with the point coordinates. ::

            >>> pnt = Point(10, 100)
            >>> pnt.coords()
            (10.0, 100.0)

        If the point is 2D return a x, y tuple. But if we change the ``z``
        the Point object become a 3D point, therefore the method return a
        x, y, z tuple. ::

            >>> pnt.z = 1000.
            >>> pnt.coords()
            (10.0, 100.0, 1000.0)

        ..
        """
        if self.is2D:
            return self.x, self.y
        else:
            return self.x, self.y, self.z

    def get_wkt(self):
        """Return a "well know text" (WKT) geometry string. ::

            >>> pnt = Point(10, 100)
            >>> pnt.get_wkt()
            'POINT(10.000000 100.000000)'

        .. warning::

            Only ``POINT`` (2/3D) are supported, ``POINTM`` and ``POINT`` with:
            ``XYZM`` are not supported yet.
        """
        return "POINT(%s)" % ' '.join(['%f' % coord
                                      for coord in self.coords()])

    def get_wkb(self):
        """Return a "well know binary" (WKB) geometry buffer

        .. warning::

            Not implemented yet.

        """
        pass

    def distance(self, pnt):
        """Calculate distance of 2 points, using the Vect_points_distance
        C function, If one of the point have z == None, return the 2D distance.

        :param pnt: the point for calculate the distance
        :type pnt: a Point object or a tuple with the coordinates


            >>> pnt0 = Point(0, 0, 0)
            >>> pnt1 = Point(1, 0)
            >>> pnt0.distance(pnt1)
            1.0
            >>> pnt1.z = 1
            >>> pnt1
            Point(1.000000, 0.000000, 1.000000)
            >>> pnt0.distance(pnt1)
            1.4142135623730951

        """
        if self.is2D or pnt.is2D:
            return libvect.Vect_points_distance(self.x, self.y, 0,
                                                pnt.x, pnt.y, 0, 0)
        else:
            return libvect.Vect_points_distance(self.x, self.y, self.z,
                                                pnt.x, pnt.y, pnt.z, 1)

    def buffer(self, dist=None, dist_x=None, dist_y=None, angle=0,
               round_=True, tol=0.1):
        """Return the buffer area around the point, using the
        ``Vect_point_buffer2`` C function.

        :param dist: the distance around the point
        :type dist: num
        :param dist_x: the distance along x
        :type dist_x: num
        :param dist_y: the distance along y
        :type dist_y: num
        :param angle: the angle between 0x and major axis
        :type angle: num
        :param round_: to make corners round
        :type round_: bool
        :param tol: fix the maximum distance between theoretical arc and
                    output segments
        :type tol: float
        :returns: the buffer as Area object

        >>> pnt = Point(0, 0)
        >>> area = pnt.buffer(10)
        >>> area.boundary                              #doctest: +ELLIPSIS
        Line([Point(10.000000, 0.000000),...Point(10.000000, 0.000000)])
        >>> area.centroid
        Point(0.000000, 0.000000)
        >>> area.isles
        []

        """
        if dist is not None:
            dist_x = dist
            dist_y = dist
        elif not dist_x or not dist_y:
            raise TypeError('TypeError: buffer expected 1 arguments, got 0')
        bound = Line()
        p_points = ctypes.pointer(bound.c_points)
        libvect.Vect_point_buffer2(self.x, self.y,
                                   dist_x, dist_y,
                                   angle, int(round_), tol,
                                   p_points)
        return Area(boundary=bound, centroid=self)


class Line(Geo):
    """Instantiate a new Line with a list of tuple, or with a list of Point. ::

        >>> line = Line([(0, 0), (1, 1), (2, 0), (1, -1)])
        >>> line                               #doctest: +NORMALIZE_WHITESPACE
        Line([Point(0.000000, 0.000000),
              Point(1.000000, 1.000000),
              Point(2.000000, 0.000000),
              Point(1.000000, -1.000000)])

    ..
    """
    # geometry type
    gtype = libvect.GV_LINE

    def __init__(self, points=None, **kargs):
        super(Line, self).__init__(**kargs)
        if points is not None:
            for pnt in points:
                self.append(pnt)

    def __getitem__(self, key):
        """Get line point of given index,  slice allowed. ::

            >>> line = Line([(0, 0), (1, 1), (2, 2), (3, 3)])
            >>> line[1]
            Point(1.000000, 1.000000)
            >>> line[-1]
            Point(3.000000, 3.000000)
            >>> line[:2]
            [Point(0.000000, 0.000000), Point(1.000000, 1.000000)]

        ..
        """
        #TODO:
        # line[0].x = 10 is not working
        #pnt.c_px = ctypes.pointer(self.c_points.contents.x[indx])
        # pnt.c_px = ctypes.cast(id(self.c_points.contents.x[indx]),
        # ctypes.POINTER(ctypes.c_double))
        if isinstance(key, slice):
            #import pdb; pdb.set_trace()
            #Get the start, stop, and step from the slice
            return [Point(self.c_points.contents.x[indx],
                          self.c_points.contents.y[indx],
                    None if self.is2D else self.c_points.contents.z[indx])
                    for indx in range(*key.indices(len(self)))]
        elif isinstance(key, int):
            if key < 0:  # Handle negative indices
                key += self.c_points.contents.n_points
            if key >= self.c_points.contents.n_points:
                raise IndexError('Index out of range')
            return Point(self.c_points.contents.x[key],
                         self.c_points.contents.y[key],
                         None if self.is2D else self.c_points.contents.z[key])
        else:
            raise ValueError("Invalid argument type: %r." % key)

    def __setitem__(self, indx, pnt):
        """Change the coordinate of point. ::

            >>> line = Line([(0, 0), (1, 1)])
            >>> line[0] = (2, 2)
            >>> line
            Line([Point(2.000000, 2.000000), Point(1.000000, 1.000000)])

        ..
        """
        x, y, z = get_xyz(pnt)
        self.c_points.contents.x[indx] = x
        self.c_points.contents.y[indx] = y
        self.c_points.contents.z[indx] = z

    def __iter__(self):
        """Return a Point generator of the Line"""
        return (self.__getitem__(i) for i in range(self.__len__()))

    def __len__(self):
        """Return the number of points of the line."""
        return self.c_points.contents.n_points

    def __str__(self):
        return self.get_wkt()

    def __repr__(self):
        return "Line([%s])" % ', '.join([repr(pnt) for pnt in self.__iter__()])

    def get_pnt(self, distance, angle=0, slope=0):
        """Return a Point object on line in the specified distance, using the
        `Vect_point_on_line` C function.
        Raise a ValueError If the distance exceed the Line length. ::

            >>> line = Line([(0, 0), (1, 1)])
            >>> line.get_pnt(5)      #doctest: +ELLIPSIS +NORMALIZE_WHITESPACE
            Traceback (most recent call last):
                ...
            ValueError: The distance exceed the lenght of the line,
            that is: 1.414214
            >>> line.get_pnt(1)
            Point(0.707107, 0.707107)

        ..
        """
        # instantiate an empty Point object
        maxdist = self.length()
        if distance > maxdist:
            str_err = "The distance exceed the lenght of the line, that is: %f"
            raise ValueError(str_err % maxdist)
        pnt = Point(0, 0, -9999)
        libvect.Vect_point_on_line(self.c_points, distance,
                                   pnt.c_points.contents.x,
                                   pnt.c_points.contents.y,
                                   pnt.c_points.contents.z,
                                   angle, slope)
        pnt.is2D = self.is2D
        return pnt

    def append(self, pnt):
        """Appends one point to the end of a line, using the
        ``Vect_append_point`` C function.

        :param pnt: the point to add to line
        :type pnt: a Point object or a tuple with the coordinates

            >>> line = Line()
            >>> line.append((10, 100))
            >>> line
            Line([Point(10.000000, 100.000000)])
            >>> line.append((20, 200))
            >>> line
            Line([Point(10.000000, 100.000000), Point(20.000000, 200.000000)])

        Like python list.
        """
        x, y, z = get_xyz(pnt)
        libvect.Vect_append_point(self.c_points, x, y, z)

    def bbox(self, bbox=None):
        """Return the bounding box of the line, using ``Vect_line_box``
        C function. ::

            >>> line = Line([(0, 0), (0, 1), (2, 1), (2, 0)])
            >>> bbox = line.bbox()
            >>> bbox
            Bbox(1.0, 0.0, 2.0, 0.0)

        ..
        """
        bbox = bbox if bbox else Bbox()
        libvect.Vect_line_box(self.c_points, bbox.c_bbox)
        return bbox

    def extend(self, line, forward=True):
        """Appends points to the end of a line.

        :param line: it is possible to extend a line, give a list of points,
                     or directly with a line_pnts struct.
        :type line: Line object ot list of points
        :param forward: if forward is True the line is extend forward otherwise
                        is extend backward. The method use the
                        `Vect_append_points` C function.
        :type forward: bool

            >>> line = Line([(0, 0), (1, 1)])
            >>> line.extend( Line([(2, 2), (3, 3)]) )
            >>> line                           #doctest: +NORMALIZE_WHITESPACE
            Line([Point(0.000000, 0.000000),
                  Point(1.000000, 1.000000),
                  Point(2.000000, 2.000000),
                  Point(3.000000, 3.000000)])

        """
        # set direction
        if forward:
            direction = libvect.GV_FORWARD
        else:
            direction = libvect.GV_BACKWARD
        # check if is a Line object
        if isinstance(line, Line):
            c_points = line.c_points
        else:
            # instantiate a Line object
            lin = Line()
            for pnt in line:
                # add the points to the line
                lin.append(pnt)
            c_points = lin.c_points

        libvect.Vect_append_points(self.c_points, c_points, direction)

    def insert(self, indx, pnt):
        """Insert new point at index position and move all old points at
        that position and above up, using ``Vect_line_insert_point``
        C function.

        :param indx: the index where add new point
        :type indx: int
        :param pnt: the point to add
        :type pnt: a Point object

            >>> line = Line([(0, 0), (1, 1)])
            >>> line.insert(0, Point(1.000000, -1.000000) )
            >>> line                           #doctest: +NORMALIZE_WHITESPACE
            Line([Point(1.000000, -1.000000),
                  Point(0.000000, 0.000000),
                  Point(1.000000, 1.000000)])

        """
        if indx < 0:  # Handle negative indices
            indx += self.c_points.contents.n_points
        if indx >= self.c_points.contents.n_points:
            raise IndexError('Index out of range')
        x, y, z = get_xyz(pnt)
        libvect.Vect_line_insert_point(self.c_points, indx, x, y, z)

    def length(self):
        """Calculate line length, 3D-length in case of 3D vector line, using
        `Vect_line_length` C function.  ::

            >>> line = Line([(0, 0), (1, 1), (0, 1)])
            >>> line.length()
            2.414213562373095

        ..
        """
        return libvect.Vect_line_length(self.c_points)

    def length_geodesic(self):
        """Calculate line length, usig `Vect_line_geodesic_length` C function.
        ::

            >>> line = Line([(0, 0), (1, 1), (0, 1)])
            >>> line.length_geodesic()
            2.414213562373095

        ..
        """
        return libvect.Vect_line_geodesic_length(self.c_points)

    def distance(self, pnt):
        """Calculate the distance between line and a point.

        :param pnt: the point to calculate distance
        :type pnt: a Point object or a tuple with the coordinates

        Return a namedtuple with:

            * point: the closest point on the line,
            * dist: the distance between these two points,
            * spdist: distance to point on line from segment beginning
            * sldist: distance to point on line form line beginning along line

        The distance is compute using the ``Vect_line_distance`` C function.

            >>> point = Point(2.3, 0.5)
            >>> line = Line([(0, 0), (2, 0), (3, 0)])
            >>> line.distance(point)           #doctest: +NORMALIZE_WHITESPACE
            LineDist(point=Point(2.300000, 0.000000),
                     dist=0.5, spdist=0.2999999999999998, sldist=2.3)
        """
        # instantite outputs
        cx = ctypes.c_double(0)
        cy = ctypes.c_double(0)
        cz = ctypes.c_double(0)
        dist = ctypes.c_double(0)
        sp_dist = ctypes.c_double(0)
        lp_dist = ctypes.c_double(0)

        libvect.Vect_line_distance(self.c_points,
                                   pnt.x, pnt.y, 0 if pnt.is2D else pnt.z,
                                   0 if self.is2D else 1,
                                   ctypes.byref(cx), ctypes.byref(cy),
                                   ctypes.byref(cz), ctypes.byref(dist),
                                   ctypes.byref(sp_dist),
                                   ctypes.byref(lp_dist))
        # instantiate the Point class
        point = Point(cx.value, cy.value, cz.value)
        point.is2D = self.is2D
        return LineDist(point, dist.value, sp_dist.value, lp_dist.value)

    def get_first_cat(self):
        """Fetches FIRST category number for given vector line and field, using
        the ``Vect_get_line_cat`` C function.

        .. warning::

            Not implemented yet.
        """
        # TODO: add this method.
        libvect.Vect_get_line_cat(self.map, self.id, self.field)
        pass

    def pop(self, indx):
        """Return the point in the index position and remove from the Line.

           :param indx: the index where add new point
           :type indx: int

            >>> line = Line([(0, 0), (1, 1), (2, 2)])
            >>> midle_pnt = line.pop(1)
            >>> midle_pnt
            Point(1.000000, 1.000000)
            >>> line
            Line([Point(0.000000, 0.000000), Point(2.000000, 2.000000)])

        """
        if indx < 0:  # Handle negative indices
            indx += self.c_points.contents.n_points
        if indx >= self.c_points.contents.n_points:
            raise IndexError('Index out of range')
        pnt = self.__getitem__(indx)
        libvect.Vect_line_delete_point(self.c_points, indx)
        return pnt

    def delete(self, indx):
        """Remove the point in the index position.
           :param indx: the index where add new point
           :type indx: int

            >>> line = Line([(0, 0), (1, 1), (2, 2)])
            >>> line.delete(-1)
            >>> line
            Line([Point(0.000000, 0.000000), Point(1.000000, 1.000000)])

        """
        if indx < 0:  # Handle negative indices
            indx += self.c_points.contents.n_points
        if indx >= self.c_points.contents.n_points:
            raise IndexError('Index out of range')
        libvect.Vect_line_delete_point(self.c_points, indx)

    def prune(self):
        """Remove duplicate points, i.e. zero length segments, using
        `Vect_line_prune` C function. ::

            >>> line = Line([(0, 0), (1, 1), (1, 1), (2, 2)])
            >>> line.prune()
            >>> line                           #doctest: +NORMALIZE_WHITESPACE
            Line([Point(0.000000, 0.000000),
                  Point(1.000000, 1.000000),
                  Point(2.000000, 2.000000)])

        ..
        """
        libvect.Vect_line_prune(self.c_points)

    def prune_thresh(self, threshold):
        """Remove points in threshold, using the ``Vect_line_prune_thresh``
        C funtion.

        :param threshold: the threshold value where prune points
        :type threshold: num

            >>> line = Line([(0, 0), (1.0, 1.0), (1.2, 0.9), (2, 2)])
            >>> line.prune_thresh(0.5)
            >>> line                     #doctest: +SKIP +NORMALIZE_WHITESPACE
            Line([Point(0.000000, 0.000000),
                  Point(1.000000, 1.000000),
                  Point(2.000000, 2.000000)])

        .. warning ::

            prune_thresh is not working yet.
        """
        libvect.Vect_line_prune(self.c_points, ctypes.c_double(threshold))

    def remove(self, pnt):
        """Delete point at given index and move all points above down, using
        `Vect_line_delete_point` C function.

        :param pnt: the point to remove
        :type pnt: a Point object or a tuple with the coordinates

            >>> line = Line([(0, 0), (1, 1), (2, 2)])
            >>> line.remove((2, 2))
            >>> line[-1]
            Point(1.000000, 1.000000)

        ..
        """
        for indx, point in enumerate(self.__iter__()):
            if pnt == point:
                libvect.Vect_line_delete_point(self.c_points, indx)
                return
        raise ValueError('list.remove(x): x not in list')

    def reverse(self):
        """Reverse the order of vertices, using `Vect_line_reverse`
        C function. ::

            >>> line = Line([(0, 0), (1, 1), (2, 2)])
            >>> line.reverse()
            >>> line                           #doctest: +NORMALIZE_WHITESPACE
            Line([Point(2.000000, 2.000000),
                  Point(1.000000, 1.000000),
                  Point(0.000000, 0.000000)])

        ..
        """
        libvect.Vect_line_reverse(self.c_points)

    def segment(self, start, end):
        """Create line segment. using the ``Vect_line_segment`` C function.

        :param start: distance from the begining of the line where
                      the segment start
        :type start: float
        :param end: distance from the begining of the line where
                    the segment end
        :type end: float

        ::
            #            x (1, 1)
            #            |
            #            |-
            #            |
            #   x--------x (1, 0)
            # (0, 0) ^

            >>> line = Line([(0, 0), (1, 0), (1, 1)])
            >>> line.segment(0.5, 1.5)         #doctest: +NORMALIZE_WHITESPACE
            Line([Point(0.500000, 0.000000),
                  Point(1.000000, 0.000000),
                  Point(1.000000, 0.500000)])
        """
        line = Line()
        libvect.Vect_line_segment(self.c_points, start, end, line.c_points)
        return line

    def tolist(self):
        """Return a list of tuple. ::

            >>> line = Line([(0, 0), (1, 1), (2, 0), (1, -1)])
            >>> line.tolist()
            [(0.0, 0.0), (1.0, 1.0), (2.0, 0.0), (1.0, -1.0)]

        ..
        """
        return [pnt.coords() for pnt in self.__iter__()]

    def toarray(self):
        """Return an array of coordinates. ::

            >>> line = Line([(0, 0), (1, 1), (2, 0), (1, -1)])
            >>> line.toarray()                 #doctest: +NORMALIZE_WHITESPACE
            array([[ 0.,  0.],
                   [ 1.,  1.],
                   [ 2.,  0.],
                   [ 1., -1.]])

        ..
        """
        return np.array(self.tolist())

    def get_wkt(self):
        """Return a Well Known Text string of the line. ::

            >>> line = Line([(0, 0), (1, 1), (1, 2)])
            >>> line.get_wkt()                 #doctest: +ELLIPSIS
            'LINESTRING(0.000000 0.000000, ..., 1.000000 2.000000)'

        ..
        """
        return "LINESTRING(%s)" % ', '.join([
               ' '.join(['%f' % coord for coord in pnt.coords()])
               for pnt in self.__iter__()])

    def from_wkt(self, wkt):
        """Create a line reading a WKT string.

        :param wkt: the WKT string containing the LINESTRING
        :type wkt: str

            >>> line = Line()
            >>> line.from_wkt("LINESTRING(0 0,1 1,1 2)")
            >>> line                           #doctest: +NORMALIZE_WHITESPACE
            Line([Point(0.000000, 0.000000),
                  Point(1.000000, 1.000000),
                  Point(1.000000, 2.000000)])

        ..
        """
        match = re.match('LINESTRING\((.*)\)', wkt)
        if match:
            self.reset()
            for coord in match.groups()[0].strip().split(','):
                self.append(tuple([float(e) for e in coord.split(' ')]))
        else:
            return None

    def get_wkb(self):
        """Return a WKB buffer.

        .. warning::

            Not implemented yet.
        """
        pass

    def buffer(self, dist=None, dist_x=None, dist_y=None,
               angle=0, round_=True, caps=True, tol=0.1):
        """Return the buffer area around the line, using the
        ``Vect_line_buffer2`` C function.

        :param dist: the distance around the line
        :type dist: num
        :param dist_x: the distance along x
        :type dist_x: num
        :param dist_y: the distance along y
        :type dist_y: num
        :param angle: the angle between 0x and major axis
        :type angle: num
        :param round_: to make corners round
        :type round_: bool
        :param tol: fix the maximum distance between theoretical arc and
                    output segments
        :type tol: float
        :returns: the buffer as Area object

        >>> line = Line([(0, 0), (0, 2)])
        >>> area = line.buffer(10)
        >>> area.boundary                              #doctest: +ELLIPSIS
        Line([Point(-10.000000, 0.000000),...Point(-10.000000, 0.000000)])
        >>> area.centroid
        Point(0.000000, 0.000000)
        >>> area.isles
        []

        ..
        """
        if dist is not None:
            dist_x = dist
            dist_y = dist
        elif not dist_x or not dist_y:
            raise TypeError('TypeError: buffer expected 1 arguments, got 0')
        p_bound = ctypes.pointer(ctypes.pointer(libvect.line_pnts()))
        pp_isle = ctypes.pointer(ctypes.pointer(
                                 ctypes.pointer(libvect.line_pnts())))
        n_isles = ctypes.pointer(ctypes.c_int())
        libvect.Vect_line_buffer2(self.c_points,
                                  dist_x, dist_y, angle,
                                  int(round_), int(caps), tol,
                                  p_bound, pp_isle, n_isles)
        return Area(boundary=Line(c_points=p_bound.contents),
                    centroid=self[0],
                    isles=[Line(c_points=pp_isle[i].contents)
                           for i in range(n_isles.contents.value)])

    def reset(self):
        """Reset line, using `Vect_reset_line` C function. ::

            >>> line = Line([(0, 0), (1, 1), (2, 0), (1, -1)])
            >>> len(line)
            4
            >>> line.reset()
            >>> len(line)
            0
            >>> line
            Line([])

        ..
        """
        libvect.Vect_reset_line(self.c_points)

    def nodes(self):
        """Return the nodes in the line"""
        if self.is_with_topology():
            n1 = ctypes.c_int()
            n2 = ctypes.c_int()
            libvect.Vect_get_line_nodes(self.c_mapinfo, self.id,
                                        ctypes.byref(n1),
                                        ctypes.byref(n2))
            return (Node(n1.value, self.c_mapinfo),
                    Node(n2.value, self.c_mapinfo))


class Node(object):
    def __init__(self, v_id, c_mapinfo):
        self.id = v_id  # vector id
        self.c_mapinfo = c_mapinfo
        self.is2D = bool(libvect.Vect_is_3d(self.c_mapinfo) != 1)
        self.nlines = libvect.Vect_get_node_n_lines(self.c_mapinfo, self.id)

    def __len__(self):
        return self.nlines

    def __iter__(self):
        return self.ilines()

    def __repr__(self):
        return "Node(%d)" % self.id

    def coords(self):
        """Return a tuple with the node coordinates."""
        x = ctypes.c_double()
        y = ctypes.c_double()
        z = ctypes.c_double()
        libvect.Vect_get_node_coor(self.c_mapinfo, self.id, ctypes.byref(x),
                                   ctypes.byref(y), ctypes.byref(z))
        return (x.value, y.value) if self.is2D else (x.value, y.value, z.value)

    def ilines(self, only_in=False, only_out=False):
        """Return a generator with all lines id connected to a node.
        The line id is negative if line is ending on the node and positive if
        starting from the node.

        :param only_in: Return only the lines that are ending in the node
        :type only_in: bool
        :param only_out: Return only the lines that are starting in the node
        :type only_out: bool
        """
        for iline in range(self.nlines):
            lid = libvect.Vect_get_node_line(self.c_mapinfo, self.id, iline)
            if (not only_in and lid > 0) or (not only_out and lid < 0):
                yield lid

    def lines(self, only_in=False, only_out=False):
        """Return a generator with all lines connected to a node.

        :param only_in: Return only the lines that are ending in the node
        :type only_in: bool
        :param only_out: Return only the lines that are starting in the node
        :type only_out: bool
        """
        for iline in self.ilines(only_in, only_out):
            yield Line(v_id=abs(iline), c_mapinfo=self.c_mapinfo)

    def angles(self):
        """Return a generator with all lines angles in a node."""
        for iline in range(self.nlines):
            yield libvect.Vect_get_node_line_angle(self.c_mapinfo,
                                                   self.id, iline)


class Boundary(Line):
    """
    """
    # geometry type
    gtype = libvect.GV_BOUNDARY

    def __init__(self, lines=None, left=None, right=None,
                 **kargs):
        v_id = kargs.get('v_id', 0)
        self.dir = libvect.GV_FORWARD if v_id > 0 else libvect.GV_BACKWARD
        super(Boundary, self).__init__(**kargs)
        self.c_left = ctypes.pointer(ctypes.c_int())
        self.c_right = ctypes.pointer(ctypes.c_int())
        #self.get_left_right()

    @property
    def left_id(self):
        return self.c_left.contents.value

    @property
    def right_id(self):
        return self.c_right.contents.value

    def __repr__(self):
        return "Boundary(v_id=%r)" % self.id

    def _get_centroid(self, side, idonly=False):
        if side > 0:
            v_id = libvect.Vect_get_area_centroid(self.c_mapinfo, side)
            v_id = v_id if v_id else None
            if idonly:
                return v_id
            else:
                cntr = Centroid(v_id=v_id, c_mapinfo=self.c_mapinfo)
                return cntr

    def get_left_centroid(self, idonly=False):
        """Return left value

        :param idonly: True to return only the cat of feature
        :type idonly: bool
        """
        return self._get_centroid(self.left_id, idonly)

    def get_right_centroid(self, idonly=False):
        """Return right value

        :param idonly: True to return only the cat of feature
        :type idonly: bool
        """
        return self._get_centroid(self.left_id, idonly)

    def get_left_right(self):
        """Return left and right value"""

        libvect.Vect_get_line_areas(self.c_mapinfo, self.id,
                                    self.c_left, self.c_right)
        return self.c_left.contents.value, self.c_right.contents.value

    def area(self):
        """Return the area of the polygon.

            >>> bound = Boundary(points=[(0, 0), (0, 2), (2, 2), (2, 0),
            ...                          (0, 0)])
            >>> bound.area()
            4.0

        """
        libgis.G_begin_polygon_area_calculations()
        return libgis.G_area_of_polygon(self.c_points.contents.x,
                                        self.c_points.contents.y,
                                        self.c_points.contents.n_points)


class Centroid(Point):
    """The Centroid class inherit from the Point class.
    Centroid contains an attribute with the C Map_info struct, and attributes
    with the id of the Area. ::

        >>> centroid = Centroid(x=0, y=10)
        >>> centroid
        Centoid(0.000000, 10.000000)
        >>> from grass.pygrass.vector import VectorTopo
        >>> geo = VectorTopo('geology')
        >>> geo.open(mode='r')
        >>> centroid = Centroid(v_id=1, c_mapinfo=geo.c_mapinfo)
        >>> centroid
        Centoid(893202.874416, 297339.312795)

    ..
    """
    # geometry type
    gtype = libvect.GV_CENTROID

    def __init__(self, area_id=None, **kargs):
        super(Centroid, self).__init__(**kargs)
        self.area_id = area_id
        if self.id and self.c_mapinfo and self.area_id is None:
            self.area_id = self.get_area_id()
        elif self.c_mapinfo and self.area_id and self.id is None:
            self.id = self.get_centroid_id()
        if self.area_id is not None:
            self.read()

        #self.c_pline = ctypes.pointer(libvect.P_line()) if topology else None

    def __repr__(self):
        return "Centoid(%s)" % ', '.join(['%f' % co for co in self.coords()])

    def get_centroid_id(self):
        """Return the centroid_id, using the c_mapinfo and an area_id
        attributes of the class, and calling the Vect_get_area_centroid
        C function, if no centroid_id were found return None"""
        centroid_id = libvect.Vect_get_area_centroid(self.c_mapinfo,
                                                     self.area_id)
        return centroid_id if centroid_id != 0 else None

    def get_area_id(self):
        """Return the area_id, using the c_mapinfo and an centroid_id
        attributes of the class, and calling the Vect_get_centroid_area
        C function, if no area_id were found return None"""
        area_id = libvect.Vect_get_centroid_area(self.c_mapinfo,
                                                 self.id)
        return area_id if area_id != 0 else None


class Isle(Geo):
    """An Isle is an area contained by another area.
    """
    def __init__(self, **kargs):
        super(Isle, self).__init__(**kargs)
        #self.area_id = area_id

    def __repr__(self):
        return "Isle(%d)" % (self.id)

    def boundaries(self):
        """Return a list of boundaries"""
        ilist = Ilist()
        libvect.Vect_get_isle_boundaries(self.c_mapinfo, self.id,
                                         ilist.c_ilist)
        return ilist

    def bbox(self, bbox=None):
        """Return bounding box of Isle"""
        bbox = bbox if bbox else Bbox()
        libvect.Vect_get_isle_box(self.c_mapinfo, self.id, bbox.c_bbox)
        return bbox

    def points(self):
        """Return a Line object with the outer ring points"""
        line = Line()
        libvect.Vect_get_isle_points(self.c_mapinfo, self.id, line.c_points)
        return line

    def points_geos(self):
        """Return a Line object with the outer ring points
        """
        return libvect.Vect_get_isle_points_geos(self.c_mapinfo, self.id)

    def area_id(self):
        """Returns area id for isle."""
        return libvect.Vect_get_isle_area(self.c_mapinfo, self.id)

    def alive(self):
        """Check if isle is alive or dead (topology required)"""
        return bool(libvect.Vect_isle_alive(self.c_mapinfo, self.id))

    def contain_pnt(self, pnt):
        """Check if point is in area.

        :param pnt: the point to remove
        :type pnt: a Point object or a tuple with the coordinates
        """
        bbox = self.bbox()
        return bool(libvect.Vect_point_in_island(pnt.x, pnt.y,
                                                 self.c_mapinfo, self.id,
                                                 bbox.c_bbox.contents))

    def area(self):
        """Return the area value of an Isle"""
        border = self.points()
        return libgis.G_area_of_polygon(border.c_points.contents.x,
                                        border.c_points.contents.y,
                                        border.c_points.contents.n_points)

    def perimeter(self):
        """Return the perimeter value of an Isle.
        """
        border = self.points()
        return libvect.Vect_line_geodesic_length(border.c_points)


class Isles(object):
    def __init__(self, c_mapinfo, area_id=None):
        self.c_mapinfo = c_mapinfo
        self.area_id = area_id
        self._isles_id = None
        self._isles = None
        if area_id:
            self._isles_id = self.get_isles_id()
            self._isles = self.get_isles()

    def __len__(self):
        return libvect.Vect_get_area_num_isles(self.c_mapinfo, self.area_id)

    def __repr__(self):
        return "Isles(%r)" % self.area_id

    def __getitem__(self, key):
        if self._isles is None:
            self.get_isles()
        return self._isles[key]

    def get_isles_id(self):
        """Return the id of isles"""
        return [libvect.Vect_get_area_isle(self.c_mapinfo, self.area_id, i)
                for i in range(self.__len__())]

    def get_isles(self):
        """Return isles"""
        return [Isle(v_id=isle_id, c_mapinfo=self.c_mapinfo)
                for isle_id in self._isles_id]

    def select_by_bbox(self, bbox):
        """Vect_select_isles_by_box

        .. warning::

            Not implemented yet.

        """
        pass


class Area(Geo):
    """
    Vect_build_line_area,
    Vect_find_area,
    Vect_get_area_box,
    Vect_get_area_points_geos,
    Vect_get_centroid_area,

    Vect_get_isle_area,
    Vect_get_line_areas,
    Vect_get_num_areas,
    Vect_get_point_in_area,
    Vect_isle_find_area,
    Vect_point_in_area,
    Vect_point_in_area_outer_ring,

    Vect_read_area_geos,
    Vect_remove_small_areas,
    Vect_select_areas_by_box,
    Vect_select_areas_by_polygon
    """
    # geometry type
    gtype = libvect.GV_AREA

    def __init__(self, boundary=None, centroid=None, isles=None, **kargs):
        super(Area, self).__init__(**kargs)
        self.boundary = None
        self.centroid = None
        self.isles = None
        if boundary and centroid:
            self.boundary = boundary
            self.centroid = centroid
            self.isles = isles if isles else []

        # set the attributes
        if self.attrs and self.cat:
            self.attrs.cat = self.cat

    def __repr__(self):
        return "Area(%d)" % self.id if self.id else "Area( )"

    def init_from_id(self, area_id=None):
        """Return an Area object"""
        if area_id is None and self.id is None:
            raise ValueError("You need to give or set the area_id")
        self.id = area_id if area_id is not None else self.id
        # get boundary
        self.get_points()
        # get isles
        self.get_isles()

    def get_points(self, line=None):
        """Return a Line object with the outer ring

        :param line: a Line object to fill with info from points of area
        :type line: a Line object
        """
        line = Line() if line is None else line
        libvect.Vect_get_area_points(self.c_mapinfo, self.id, line.c_points)
        return line

    def get_centroid(self, centroid=None):
        """Return the centroid

        :param centroid: a Centroid object to fill with info from centroid of area
        :type centroid: a Centroid object
        """
        centroid_id = libvect.Vect_get_area_centroid(self.c_mapinfo, self.id)
        if centroid_id:
            if centroid:
                centroid.id = centroid_id
                centroid.read()
                return centroid
            return Centroid(v_id=centroid_id, c_mapinfo=self.c_mapinfo,
                            area_id=self.id)

    def num_isles(self):
        return libvect.Vect_get_area_num_isles(self.c_mapinfo, self.id)

    def get_isles(self, isles=None):
        """Instantiate the boundary attribute reading area_id"""
        if isles is not None:
            isles.area_id = self.id
            return isles
        return Isles(self.c_mapinfo, self.id)

    def area(self):
        """Returns area of area without areas of isles.
        double Vect_get_area_area (const struct Map_info \*Map, int area)
        """
        return libvect.Vect_get_area_area(self.c_mapinfo, self.id)

    def alive(self):
        """Check if area is alive or dead (topology required)
        """
        return bool(libvect.Vect_area_alive(self.c_mapinfo, self.id))

    def bbox(self, bbox=None):
        """Return the Bbox of area

        :param bbox: a Bbox object to fill with info from bounding box of area
        :type bbox: a Bbox object
        """
        bbox = bbox if bbox else Bbox()
        libvect.Vect_get_area_box(self.c_mapinfo, self.id, bbox.c_bbox)
        return bbox

    def buffer(self, dist=None, dist_x=None, dist_y=None,
               angle=0, round_=True, caps=True, tol=0.1):
        """Return the buffer area around the area, using the
        ``Vect_area_buffer2`` C function.

        :param dist: the distance around the area
        :type dist: num
        :param dist_x: the distance along x
        :type dist_x: num
        :param dist_y: the distance along y
        :type dist_y: num
        :param angle: the angle between 0x and major axis
        :type angle: num
        :param round_: to make corners round
        :type round_: bool
        :param tol: fix the maximum distance between theoretical arc and
                    output segments
        :type tol: float
        :returns: the buffer as Area object

        """
        if dist is not None:
            dist_x = dist
            dist_y = dist
        elif not dist_x or not dist_y:
            raise TypeError('TypeError: buffer expected 1 arguments, got 0')
        p_bound = ctypes.pointer(ctypes.pointer(libvect.line_pnts()))
        pp_isle = ctypes.pointer(ctypes.pointer(
                                 ctypes.pointer(libvect.line_pnts())))
        n_isles = ctypes.pointer(ctypes.c_int())
        libvect.Vect_area_buffer2(self.c_mapinfo, self.id,
                                  dist_x, dist_y, angle,
                                  int(round_), int(caps), tol,
                                  p_bound, pp_isle, n_isles)
        return Area(boundary=Line(c_points=p_bound.contents),
                    centroid=self.centroid,
                    isles=[Line(c_points=pp_isle[i].contents)
                           for i in range(n_isles.contents.value)])

    def boundaries(self, ilist=False):
        """Creates list of boundaries for given area.

        int Vect_get_area_boundaries(const struct Map_info \*Map,
                                     int area, struct ilist \*List)
        """
        ilst = Ilist()
        libvect.Vect_get_area_boundaries(self.c_mapinfo, self.id,
                                         ilst.c_ilist)
        if ilist:
            return ilist
        return [Boundary(v_id, c_mapinfo=self.c_mapinfo) for v_id in ilst]

    def cats(self, cats=None):
        """Get area categories.

        :param cats: a Cats object to fill with info with area categories
        :type cats: a Cats object
        """
        cats = cats if cats else Cats()
        libvect.Vect_get_area_cats(self.c_mapinfo, self.id, cats.c_cats)
        return cats

    def get_first_cat(self):
        """Find FIRST category of given field and area.

        int Vect_get_area_cat(const struct Map_info \*Map, int area, int field)
        """
        pass

    def contain_pnt(self, pnt, bbox=None):
        """Check if point is in area.

        :param pnt: the point to analyze
        :type pnt: a Point object or a tuple with the coordinates
        :param bbox: the bounding box where run the analysis
        :type bbox: a Bbox object
        """
        bbox = bbox if bbox else self.bbox()
        return bool(libvect.Vect_point_in_area(pnt.x, pnt.y,
                                               self.c_mapinfo, self.id,
                                               bbox.c_bbox))

    def perimeter(self):
        """Calculate area perimeter.

        :return: double Vect_area_perimeter (const struct line_pnts \*Points)

        """
        border = self.get_points()
        return libvect.Vect_line_geodesic_length(border.c_points)

    def read(self, line=None, centroid=None, isles=None):
        self.boundary = self.get_points(line)
        self.centroid = self.get_centroid(centroid)
        #self.isles = self.get_isles(isles)
        if self.centroid:
            libvect.Vect_read_line(self.c_mapinfo, None, self.c_cats,
                                   self.centroid.id)


#
# Define a dictionary to convert the feature type to name and or object
#

GV_TYPE = {libvect.GV_POINT:    {'label': 'point',    'obj': Point},
           libvect.GV_LINE:     {'label': 'line',     'obj': Line},
           libvect.GV_BOUNDARY: {'label': 'boundary', 'obj': Boundary},
           libvect.GV_CENTROID: {'label': 'centroid', 'obj': Centroid},
           libvect.GV_FACE:     {'label': 'face',     'obj': None},
           libvect.GV_KERNEL:   {'label': 'kernel',   'obj': None},
           libvect.GV_AREA:     {'label': 'area',     'obj': Area},
           libvect.GV_VOLUME:   {'label': 'volume',   'obj': None}, }

GEOOBJ = {"areas": Area,
          "dblinks": None,
          "faces": None,
          "holes": None,
          "islands": Isle,
          "kernels": None,
          "line_points": None,
          "points": Point,
          "lines": Line,
          "nodes": Node,
          "volumes": None}


def c_read_next_line(c_mapinfo, c_points, c_cats):
    v_id = c_mapinfo.contents.next_line
    v_id = v_id if v_id != 0 else None
    ftype = libvect.Vect_read_next_line(c_mapinfo, c_points, c_cats)
    if ftype == -2:
        raise StopIteration()
    if ftype == -1:
        raise
    return ftype, v_id, c_points, c_cats


def read_next_line(c_mapinfo, table=None, writable=False,
                   c_points=None, c_cats=None, is2D=True):
    """Return the next geometry feature of a vector map."""
    c_points = c_points if c_points else ctypes.pointer(libvect.line_pnts())
    c_cats = c_cats if c_cats else ctypes.pointer(libvect.line_cats())
    ftype, v_id, c_points, c_cats = c_read_next_line(c_mapinfo, c_points,
                                                     c_cats)
    return GV_TYPE[ftype]['obj'](v_id=v_id, c_mapinfo=c_mapinfo,
                                 c_points=c_points, c_cats=c_cats,
                                 table=table, writable=writable, is2D=is2D)


def c_read_line(feature_id, c_mapinfo, c_points, c_cats):
    nmax = libvect.Vect_get_num_lines(c_mapinfo)
    if feature_id < 0:  # Handle negative indices
        feature_id += nmax + 1
    if feature_id > nmax:
        raise IndexError('Index out of range')
    if feature_id > 0:
        ftype = libvect.Vect_read_line(c_mapinfo, c_points, c_cats, feature_id)
        return feature_id, ftype, c_points, c_cats
    else:
        raise ValueError('The index must be >0, %r given.' % feature_id)


def read_line(feature_id, c_mapinfo, table=None, writable=False,
              c_points=None, c_cats=None, is2D=True):
    """Return a geometry object given the feature id and the c_mapinfo.
    """
    c_points = c_points if c_points else ctypes.pointer(libvect.line_pnts())
    c_cats = c_cats if c_cats else ctypes.pointer(libvect.line_cats())
    feature_id, ftype, c_points, c_cats = c_read_line(feature_id, c_mapinfo,
                                                      c_points, c_cats)
    if GV_TYPE[ftype]['obj'] is not None:
        return GV_TYPE[ftype]['obj'](v_id=feature_id, c_mapinfo=c_mapinfo,
                                     c_points=c_points, c_cats=c_cats,
                                     table=table, writable=writable, is2D=is2D)




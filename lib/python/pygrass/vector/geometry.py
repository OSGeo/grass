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

from grass.pygrass.utils import decode
from grass.pygrass.errors import GrassError, mapinfo_must_be_set

from grass.pygrass.vector.basic import Ilist, Bbox, Cats
from grass.pygrass.vector import sql

# For test purposes
test_vector_name = "geometry_doctest_map"

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
    def __init__(self, cat, table, writeable=False):
        self._cat = None
        self.cond = ''
        self.table = table
        self.cat = cat
        self.writeable = writeable

    def _get_cat(self):
        return self._cat

    def _set_cat(self, value):
        self._cat = value
        if value:
            # update condition
            self.cond = "%s=%d" % (self.table.key, value)

    cat = property(fget=_get_cat, fset=_set_cat,
                   doc="Set and obtain cat value")

    def __getitem__(self, keys):
        """Return the value stored in the attribute table.

        >>> from grass.pygrass.vector import VectorTopo
        >>> test_vect = VectorTopo(test_vector_name)
        >>> test_vect.open('r')
        >>> v1 = test_vect[1]
        >>> v1.attrs['name']
        'point'
        >>> v1.attrs['name', 'value']
        ('point', 1.0)
        >>> test_vect.close()

        """
        sqlcode = sql.SELECT_WHERE.format(cols=(keys if np.isscalar(keys)
                                                else ', '.join(keys)),
                                          tname=self.table.name,
                                          condition=self.cond)
        cur = self.table.execute(sqlcode)
        results = cur.fetchone()
        if results is not None:
            return results[0] if len(results) == 1 else results

    def __setitem__(self, keys, values):
        """Set value of a given column of a table attribute.

        >>> from grass.pygrass.vector import VectorTopo
        >>> test_vect = VectorTopo(test_vector_name)
        >>> test_vect.open('r')
        >>> v1 = test_vect[1]
        >>> v1.attrs['name']
        'point'

        >>> v1.attrs['name'] = "new_point_1"
        >>> v1.attrs['name']
        'new_point_1'

        >>> v1.attrs['name', 'value'] = "new_point_2", 100.
        >>> v1.attrs['name', 'value']
        ('new_point_2', 100.0)
        >>> v1.attrs['name', 'value'] = "point", 1.
        >>> v1.attrs.table.conn.commit()
        >>> test_vect.close()

        """
        if self.writeable:
            if np.isscalar(keys):
                keys, values = (keys, ), (values, )
            # check if key is a column of the table or not
            for key in keys:
                if key not in self.table.columns:
                    raise KeyError('Column: %s not in table' % key)
            # prepare the string using as paramstyle: qmark
            vals = ','.join(['%s=?' % k for k in keys])
            # "UPDATE {tname} SET {values} WHERE {condition};"
            sqlcode = sql.UPDATE_WHERE.format(tname=self.table.name,
                                              values=vals,
                                              condition=self.cond)
            self.table.execute(sqlcode, values=values)
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
           >>> test_vect = VectorTopo(test_vector_name)
           >>> test_vect.open('r')
           >>> v1 = test_vect[1]
           >>> v1.attrs.values()
           (1, 'point', 1.0)
            >>> test_vect.close()

        """
        #SELECT {cols} FROM {tname} WHERE {condition}
        cur = self.table.execute(sql.SELECT_WHERE.format(cols='*',
                                                         tname=self.table.name,
                                                         condition=self.cond))
        return cur.fetchone()

    def keys(self):
        """Return the column name of the attribute table.

           >>> from grass.pygrass.vector import VectorTopo
           >>> test_vect = VectorTopo(test_vector_name)
           >>> test_vect.open('r')
           >>> v1 = test_vect[1]
           >>> v1.attrs.keys()
           ['cat', 'name', 'value']
            >>> test_vect.close()

        """
        return self.table.columns.names()

    def commit(self):
        """Save the changes"""
        self.table.conn.commit()


class Geo(object):
    """
    Base object for different feature types
    """
    gtype = None

    def __init__(self, v_id=0, c_mapinfo=None, c_points=None, c_cats=None,
                 table=None, writeable=False, is2D=True, free_points=False,
                 free_cats=False):
        """Constructor of a geometry object

            :param v_id:      The vector feature id
            :param c_mapinfo: A pointer to the vector mapinfo structure
            :param c_points:  A pointer to a libvect.line_pnts structure, this
                              is optional, if not set an internal structure will
                              be allocated and free'd at object destruction
            :param c_cats:    A pointer to a libvect.line_cats structure, this
                              is optional, if not set an internal structure will
                              be allocated and free'd at object destruction
            :param table:     The attribute table to select attributes for
                              this feature
            :param writeable: Not sure what this is for?
            :param is2D:      If True this feature has two dimensions, False if
                              this feature has three dimensions
            :param free_points: Set this True if the provided c_points structure
                                should be free'd at object destruction, be aware
                                that no other object should free them, otherwise
                                you can expect a double free corruption segfault
            :param free_cats:   Set this True if the provided c_cats structure
                                should be free'd at object destruction, be aware
                                that no other object should free them, otherwise
                                you can expect a double free corruption segfault

        """
        self.id = v_id  # vector id
        self.c_mapinfo = c_mapinfo
        self.is2D = (is2D if is2D is not None else
                     bool(libvect.Vect_is_3d(self.c_mapinfo) != 1))

        # Set True if cats and points are allocated by this object
        # to free the cats and points structures on destruction
        self._free_points = False
        self._free_cats = False

        read = False
        # set c_points
        if c_points is None:
            self.c_points = ctypes.pointer(libvect.line_pnts())
            self._free_points = True
            read = True
        else:
            self.c_points = c_points
            self._free_points = free_points

        # set c_cats
        if c_cats is None:
            self.c_cats = ctypes.pointer(libvect.line_cats())
            self._free_cats = free_cats
            read = True
        else:
            self.c_cats = c_cats
            self._free_cats = True

        if self.id and self.c_mapinfo is not None and read:
            self.read()

        # set the attributes as last thing to do
        self.attrs = None
        if table is not None and self.cat is not None:
            self.attrs = Attrs(self.cat, table, writeable)

    def __del__(self):
        """Take care of the allocated line_pnts and line_cats allocation
        """
        if self._free_points == True and self.c_points:
            if self.c_points.contents.alloc_points > 0:
                #print("G_free(points) [%i]"%(self.c_points.contents.alloc_points))
                libgis.G_free(self.c_points.contents.x)
                libgis.G_free(self.c_points.contents.y)
                if self.c_points.contents.z:
                    libgis.G_free(self.c_points.contents.z)
        if self._free_cats == True and self.c_cats:
            if self.c_cats.contents.alloc_cats > 0:
                #print("G_free(cats) [%i]"%(self.c_cats.contents.alloc_cats))
                libgis.G_free(self.c_cats.contents.cat)

    @property
    def cat(self):
        if self.c_cats.contents.cat:
            return self.c_cats.contents.cat.contents.value

    def has_topology(self):
        if self.c_mapinfo is not None:
            return self.c_mapinfo.contents.level == 2
        else:
            return False

    @mapinfo_must_be_set
    def read(self):
        """Read and set the coordinates of the centroid from the vector map,
        using the centroid_id and calling the Vect_read_line C function"""
        self.id, ftype, c_points, c_cats = c_read_line(self.id, self.c_mapinfo,
                                                       self.c_points,
                                                       self.c_cats)

    def to_wkt(self):
        """Return a "well know text" (WKT) geometry string, this method uses
           the GEOS implementation in the vector library. ::

            >>> pnt = Point(10, 100)
            >>> pnt.to_wkt()
            'POINT (10.0000000000000000 100.0000000000000000)'
        """
        return decode(libvect.Vect_line_to_wkt(self.c_points, self.gtype, not self.is2D))

    def to_wkb(self):
        """Return a "well know binary" (WKB) geometry byte array, this method uses
           the GEOS implementation in the vector library. ::

            >>> pnt = Point(10, 100)
            >>> wkb = pnt.to_wkb()
            >>> len(wkb)
            21
        """
        size = ctypes.c_size_t()
        barray = libvect.Vect_line_to_wkb(self.c_points, self.gtype,
                                          not self.is2D, ctypes.byref(size))
        return(ctypes.string_at(barray, size.value))

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
        POINT Z (0.0000000000000000 0.0000000000000000 0.0000000000000000)


        >>> c_points = ctypes.pointer(libvect.line_pnts())
        >>> c_cats = ctypes.pointer(libvect.line_cats())
        >>> p = Point(c_points = c_points, c_cats=c_cats)
        >>> del p


        >>> c_points = ctypes.pointer(libvect.line_pnts())
        >>> c_cats = ctypes.pointer(libvect.line_cats())
        >>> p = Point(c_points=c_points, c_cats=c_cats, free_points=True,
        ...           free_cats=True)
        >>> del p

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
        return self.to_wkt()

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

    def to_wkt_p(self):
        """Return a "well know text" (WKT) geometry string Python implementation. ::

            >>> pnt = Point(10, 100)
            >>> pnt.to_wkt_p()
            'POINT(10.000000 100.000000)'

        .. warning::

            Only ``POINT`` (2/3D) are supported, ``POINTM`` and ``POINT`` with:
            ``XYZM`` are not supported yet.
        """
        return "POINT(%s)" % ' '.join(['%f' % coord
                                      for coord in self.coords()])

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
        >>> boundary, centroid = pnt.buffer(10)
        >>> boundary                              #doctest: +ELLIPSIS
        Line([Point(10.000000, 0.000000),...Point(10.000000, 0.000000)])
        >>> centroid
        Point(0.000000, 0.000000)

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
        return (bound, self)


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
        return self.to_wkt()

    def __repr__(self):
        return "Line([%s])" % ', '.join([repr(pnt) for pnt in self.__iter__()])

    def point_on_line(self, distance, angle=0, slope=0):
        """Return a Point object on line in the specified distance, using the
        `Vect_point_on_line` C function.
        Raise a ValueError If the distance exceed the Line length. ::

            >>> line = Line([(0, 0), (1, 1)])
            >>> line.point_on_line(5)      #doctest: +ELLIPSIS +NORMALIZE_WHITESPACE
            Traceback (most recent call last):
                ...
            ValueError: The distance exceed the length of the line,
            that is: 1.414214
            >>> line.point_on_line(1)
            Point(0.707107, 0.707107)

        ..
        """
        # instantiate an empty Point object
        maxdist = self.length()
        if distance > maxdist:
            str_err = "The distance exceed the length of the line, that is: %f"
            raise ValueError(str_err % maxdist)
        pnt = Point(0, 0, -9999)
        if not libvect.Vect_point_on_line(self.c_points, distance,
                                          pnt.c_points.contents.x,
                                          pnt.c_points.contents.y,
                                          pnt.c_points.contents.z,
                                          ctypes.pointer(ctypes.c_double(angle)),
                                          ctypes.pointer(ctypes.c_double(slope))):
            raise ValueError("Vect_point_on_line give an error.")
        pnt.is2D = self.is2D
        return pnt

    @mapinfo_must_be_set
    def alive(self):
        """Return True if this line is alive or False if this line is
           dead or its index is out of range.
        """
        return(bool(libvect.Vect_line_alive(self.c_mapinfo, self.id)))

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

    @mapinfo_must_be_set
    def first_cat(self):
        """Fetches FIRST category number for given vector line and field, using
        the ``Vect_get_line_cat`` C function.

        .. warning::

            Not implemented yet.
        """
        # TODO: add this method.
        # libvect.Vect_get_line_cat(self.c_mapinfo, self.id, self.field)
        pass

    def pop(self, indx):
        """Return the point in the index position and remove from the Line.

           :param indx: the index where add new point
           :type indx: int

            >>> line = Line([(0, 0), (1, 1), (2, 2)])
            >>> midle_pnt = line.pop(1)
            >>> midle_pnt                #doctest: +NORMALIZE_WHITESPACE
            Point(1.000000, 1.000000)
            >>> line                     #doctest: +NORMALIZE_WHITESPACE
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
            >>> line                     #doctest: +NORMALIZE_WHITESPACE
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
        C function.

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
            >>> line[-1]                     #doctest: +NORMALIZE_WHITESPACE
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

        :param start: distance from the beginning of the line where
                      the segment start
        :type start: float
        :param end: distance from the beginning of the line where
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

    def to_list(self):
        """Return a list of tuple. ::

            >>> line = Line([(0, 0), (1, 1), (2, 0), (1, -1)])
            >>> line.to_list()
            [(0.0, 0.0), (1.0, 1.0), (2.0, 0.0), (1.0, -1.0)]

        ..
        """
        return [pnt.coords() for pnt in self.__iter__()]

    def to_array(self):
        """Return an array of coordinates. ::

            >>> line = Line([(0, 0), (1, 1), (2, 0), (1, -1)])
            >>> line.to_array()                 #doctest: +NORMALIZE_WHITESPACE
            array([[ 0.,  0.],
                   [ 1.,  1.],
                   [ 2.,  0.],
                   [ 1., -1.]])

        ..
        """
        return np.array(self.to_list())

    def to_wkt_p(self):
        """Return a Well Known Text string of the line. ::

            >>> line = Line([(0, 0), (1, 1), (1, 2)])
            >>> line.to_wkt_p()                 #doctest: +ELLIPSIS
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
        >>> boundary, centroid, isles = line.buffer(10)
        >>> boundary                              #doctest: +ELLIPSIS
        Line([Point(-10.000000, 0.000000),...Point(-10.000000, 0.000000)])
        >>> centroid                     #doctest: +NORMALIZE_WHITESPACE
        Point(0.000000, 0.000000)
        >>> isles
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
        boundary = Line(c_points=p_bound.contents)
        isles = [Line(c_points=pp_isle[i].contents)
                 for i in range(n_isles.contents.value) if pp_isle[i]]
        return(boundary, self[0], isles)

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

    @mapinfo_must_be_set
    def nodes(self):
        """Return the start and end nodes of the line

           This method requires topology build.

           return: A tuple of Node objects that represent the
                   start and end point of this line.
        """
        if self.has_topology():
            n1 = ctypes.c_int()
            n2 = ctypes.c_int()
            libvect.Vect_get_line_nodes(self.c_mapinfo, self.id,
                                        ctypes.byref(n1),
                                        ctypes.byref(n2))
            return (Node(n1.value, self.c_mapinfo),
                    Node(n2.value, self.c_mapinfo))


class Node(object):
    """Node class for topological analysis of line neighbors.

       Objects of this class will be returned by the node() function
       of a Line object.

       All methods in this class require a proper setup of the Node
       objects. Hence, the correct id and a valid pointer to a mapinfo
       object must be provided in the constructions. Otherwise a segfault
       may happen.

    """
    def __init__(self, v_id, c_mapinfo, **kwords):
        """Construct a Node object

           param v_id: The unique node id
           param c_mapinfo: A valid pointer to the mapinfo object
           param **kwords: Ignored
        """
        self.id = v_id  # vector id
        self.c_mapinfo = c_mapinfo
        self._setup()

    @mapinfo_must_be_set
    def _setup(self):
        self.is2D = bool(libvect.Vect_is_3d(self.c_mapinfo) != 1)
        self.nlines = libvect.Vect_get_node_n_lines(self.c_mapinfo, self.id)

    def __len__(self):
        return self.nlines

    def __iter__(self):
        return self.ilines()

    def __repr__(self):
        return "Node(%d)" % self.id

    @mapinfo_must_be_set
    def alive(self):
        """Return True if this node is alive or False if this node is
           dead or its index is out of range.
        """
        return(bool(libvect.Vect_node_alive(self.c_mapinfo, self.id)))

    @mapinfo_must_be_set
    def coords(self):
        """Return a tuple with the node coordinates."""
        x = ctypes.c_double()
        y = ctypes.c_double()
        z = ctypes.c_double()
        libvect.Vect_get_node_coor(self.c_mapinfo, self.id, ctypes.byref(x),
                                   ctypes.byref(y), ctypes.byref(z))
        return (x.value, y.value) if self.is2D else (x.value, y.value, z.value)

    def to_wkt(self):
        """Return a "well know text" (WKT) geometry string. ::
        """
        return "POINT(%s)" % ' '.join(['%f' % coord
                                      for coord in self.coords()])

    def to_wkb(self):
        """Return a "well know binary" (WKB) geometry array. ::

           TODO: Must be implemented
        """
        raise Exception("Not implemented")

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

    @mapinfo_must_be_set
    def lines(self, only_in=False, only_out=False):
        """Return a generator with all lines connected to a node.

        :param only_in: Return only the lines that are ending in the node
        :type only_in: bool
        :param only_out: Return only the lines that are starting in the node
        :type only_out: bool
        """
        for iline in self.ilines(only_in, only_out):
            yield Line(v_id=abs(iline), c_mapinfo=self.c_mapinfo)

    @mapinfo_must_be_set
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

    def __init__(self, **kargs):
        super(Boundary, self).__init__(**kargs)
        v_id = kargs.get('v_id', 0)
        # not sure what it means that v_id is None
        v_id = 0 if v_id is None else v_id
        self.dir = libvect.GV_FORWARD if v_id > 0 else libvect.GV_BACKWARD
        self.c_left = ctypes.pointer(ctypes.c_int())
        self.c_right = ctypes.pointer(ctypes.c_int())

    @property
    def left_area_id(self):
        """Left side area id, only available after read_area_ids() was called"""
        return self.c_left.contents.value

    @property
    def right_area_id(self):
        """Right side area id, only available after read_area_ids() was called"""
        return self.c_right.contents.value

    def __repr__(self):
        return "Boundary([%s])" % ', '.join([repr(pnt) for pnt in self.__iter__()])

    @mapinfo_must_be_set
    def _centroid(self, side, idonly=False):
        if side > 0:
            v_id = libvect.Vect_get_area_centroid(self.c_mapinfo, side)
            v_id = v_id if v_id else None
            if idonly:
                return v_id
            else:
                cntr = Centroid(v_id=v_id, c_mapinfo=self.c_mapinfo)
                return cntr

    def left_centroid(self, idonly=False):
        """Return left centroid

        :param idonly: True to return only the cat of feature
        :type idonly: bool
        """
        return self._centroid(self.c_left.contents.value, idonly)

    def right_centroid(self, idonly=False):
        """Return right centroid

        :param idonly: True to return only the cat of feature
        :type idonly: bool
        """
        return self._centroid(self.c_right.contents.value, idonly)

    @mapinfo_must_be_set
    def read_area_ids(self):
        """Read and return left and right area ids of the boundary"""

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
        Centroid(0.000000, 10.000000)
        >>> from grass.pygrass.vector import VectorTopo
        >>> test_vect = VectorTopo(test_vector_name)
        >>> test_vect.open(mode='r')
        >>> centroid = Centroid(v_id=18, c_mapinfo=test_vect.c_mapinfo)
        >>> centroid
        Centroid(3.500000, 3.500000)
        >>> test_vect.close()

    ..
    """
    # geometry type
    gtype = libvect.GV_CENTROID

    def __init__(self, area_id=None, **kargs):
        super(Centroid, self).__init__(**kargs)
        self.area_id = area_id
        if self.id and self.c_mapinfo and self.area_id is None:
            self.area_id = self._area_id()
        elif self.c_mapinfo and self.area_id and self.id is None:
            self.id = self._centroid_id()
        if self.area_id is not None:
            self.read()

        #self.c_pline = ctypes.pointer(libvect.P_line()) if topology else None

    def __repr__(self):
        return "Centroid(%s)" % ', '.join(['%f' % co for co in self.coords()])

    @mapinfo_must_be_set
    def _centroid_id(self):
        """Return the centroid_id, using the c_mapinfo and an area_id
        attributes of the class, and calling the Vect_get_area_centroid
        C function, if no centroid_id were found return None"""
        centroid_id = libvect.Vect_get_area_centroid(self.c_mapinfo,
                                                     self.area_id)
        return centroid_id if centroid_id != 0 else None

    @mapinfo_must_be_set
    def _area_id(self):
        """Return the area_id, using the c_mapinfo and an centroid_id
        attributes of the class, and calling the Vect_centroid_area
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

    @mapinfo_must_be_set
    def boundaries(self):
        """Return a list of boundaries"""
        ilist = Ilist()
        libvect.Vect_get_isle_boundaries(self.c_mapinfo, self.id,
                                         ilist.c_ilist)
        return ilist

    @mapinfo_must_be_set
    def bbox(self, bbox=None):
        """Return bounding box of Isle"""
        bbox = bbox if bbox else Bbox()
        libvect.Vect_get_isle_box(self.c_mapinfo, self.id, bbox.c_bbox)
        return bbox

    @mapinfo_must_be_set
    def points(self):
        """Return a Line object with the outer ring points"""
        line = Line()
        libvect.Vect_get_isle_points(self.c_mapinfo, self.id, line.c_points)
        return line

    def to_wkt(self):
        """Return a Well Known Text string of the isle. ::

            For now the outer ring is returned

            TODO: Implement inner rings detected from isles
        """
        line = self.points()

        return "Polygon((%s))" % ', '.join([
               ' '.join(['%f' % coord for coord in pnt])
               for pnt in line.to_list()])

    def to_wkb(self):
        """Return a "well know text" (WKB) geometry array. ::
        """
        raise Exception("Not implemented")

    @mapinfo_must_be_set
    def points_geos(self):
        """Return a Line object with the outer ring points
        """
        return libvect.Vect_get_isle_points_geos(self.c_mapinfo, self.id)

    @mapinfo_must_be_set
    def area_id(self):
        """Returns area id for isle."""
        return libvect.Vect_get_isle_area(self.c_mapinfo, self.id)

    @mapinfo_must_be_set
    def alive(self):
        """Check if isle is alive or dead (topology required)"""
        return bool(libvect.Vect_isle_alive(self.c_mapinfo, self.id))

    @mapinfo_must_be_set
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
            self._isles_id = self.isles_ids()
            self._isles = self.isles()

    @mapinfo_must_be_set
    def __len__(self):
        return libvect.Vect_get_area_num_isles(self.c_mapinfo, self.area_id)

    def __repr__(self):
        return "Isles(%r)" % self.area_id

    def __getitem__(self, key):
        if self._isles is None:
            self.isles()
        return self._isles[key]

    @mapinfo_must_be_set
    def isles_ids(self):
        """Return the id of isles"""
        return [libvect.Vect_get_area_isle(self.c_mapinfo, self.area_id, i)
                for i in range(self.__len__())]

    @mapinfo_must_be_set
    def isles(self):
        """Return isles"""
        return [Isle(v_id=isle_id, c_mapinfo=self.c_mapinfo)
                for isle_id in self._isles_id]


class Area(Geo):
    """
    Vect_build_line_area,
    Vect_find_area,
    Vect_get_area_box,
    Vect_get_area_points_geos,
    Vect_centroid_area,

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

    def __init__(self, **kargs):
        super(Area, self).__init__(**kargs)

        # set the attributes
        #if self.attrs and self.cat:
        #    self.attrs.cat = self.cat

    def __repr__(self):
        return "Area(%d)" % self.id if self.id else "Area( )"

    @property
    def cat(self):
        centroid = self.centroid()
        return centroid.cat if centroid else None

    @mapinfo_must_be_set
    def points(self, line=None):
        """Return a Line object with the outer ring

        :param line: a Line object to fill with info from points of area
        :type line: a Line object
        """
        line = Line() if line is None else line
        libvect.Vect_get_area_points(self.c_mapinfo, self.id, line.c_points)
        return line

    @mapinfo_must_be_set
    def centroid(self):
        """Return the centroid

        :param centroid: a Centroid object to fill with info from centroid of area
        :type centroid: a Centroid object
        """
        centroid_id = libvect.Vect_get_area_centroid(self.c_mapinfo, self.id)
        if centroid_id:
            return Centroid(v_id=centroid_id, c_mapinfo=self.c_mapinfo,
                            area_id=self.id)

    @mapinfo_must_be_set
    def num_isles(self):
        return libvect.Vect_get_area_num_isles(self.c_mapinfo, self.id)

    @mapinfo_must_be_set
    def isles(self, isles=None):
        """Return a list of islands located in this area"""
        if isles is not None:
            isles.area_id = self.id
            return isles
        return Isles(self.c_mapinfo, self.id)

    @mapinfo_must_be_set
    def area(self):
        """Returns area of area without areas of isles.
        double Vect_get_area_area (const struct Map_info \*Map, int area)
        """
        return libvect.Vect_get_area_area(self.c_mapinfo, self.id)

    @mapinfo_must_be_set
    def alive(self):
        """Check if area is alive or dead (topology required)
        """
        return bool(libvect.Vect_area_alive(self.c_mapinfo, self.id))

    @mapinfo_must_be_set
    def bbox(self, bbox=None):
        """Return the Bbox of area

        :param bbox: a Bbox object to fill with info from bounding box of area
        :type bbox: a Bbox object
        """
        bbox = bbox if bbox else Bbox()
        libvect.Vect_get_area_box(self.c_mapinfo, self.id, bbox.c_bbox)
        return bbox

    @mapinfo_must_be_set
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
        :returns: the buffer as line, centroid, isles object tuple

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
        return (Line(c_points=p_bound.contents),
                self.centroid,
                [Line(c_points=pp_isle[i].contents)
                 for i in range(n_isles.contents.value)])

    @mapinfo_must_be_set
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
        return [Boundary(v_id=abs(v_id), c_mapinfo=self.c_mapinfo) for v_id in ilst]

    def to_wkt(self):
        """Return a "well know text" (WKT) area string, this method uses
           the GEOS implementation in the vector library. ::
        """
        return decode(libvect.Vect_read_area_to_wkt(self.c_mapinfo, self.id))

    def to_wkb(self):
        """Return a "well know binary" (WKB) area byte array, this method uses
           the GEOS implementation in the vector library. ::
        """
        size = ctypes.c_size_t()
        barray = libvect.Vect_read_area_to_wkb(self.c_mapinfo, self.id,
                                              ctypes.byref(size))
        return(ctypes.string_at(barray, size.value))

    @mapinfo_must_be_set
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

        ..warning: Not implemented
        """
        pass

    @mapinfo_must_be_set
    def contains_point(self, point, bbox=None):
        """Check if point is in area.

        :param point: the point to analyze
        :type point: a Point object or a tuple with the coordinates
        :param bbox: the bounding box where run the analysis
        :type bbox: a Bbox object
        """
        bbox = bbox if bbox else self.bbox()
        return bool(libvect.Vect_point_in_area(point.x, point.y,
                                               self.c_mapinfo, self.id,
                                               bbox.c_bbox))

    @mapinfo_must_be_set
    def perimeter(self):
        """Calculate area perimeter.

        :return: double Vect_area_perimeter (const struct line_pnts \*Points)

        """
        border = self.points()
        return libvect.Vect_line_geodesic_length(border.c_points)

    def read(self):
        pass


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
          "boundaries": Boundary,
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


def read_next_line(c_mapinfo, table=None, writeable=False,
                   c_points=None, c_cats=None, is2D=True):
    """Return the next geometry feature of a vector map."""

    # Take care of good memory management
    free_points = False
    if c_points == None:
        free_points = True

    free_cats = False
    if c_cats == None:
        free_cats = True

    c_points = c_points if c_points else ctypes.pointer(libvect.line_pnts())
    c_cats = c_cats if c_cats else ctypes.pointer(libvect.line_cats())
    ftype, v_id, c_points, c_cats = c_read_next_line(c_mapinfo, c_points,
                                                     c_cats)
    return GV_TYPE[ftype]['obj'](v_id=v_id, c_mapinfo=c_mapinfo,
                                 c_points=c_points, c_cats=c_cats,
                                 table=table, writeable=writeable, is2D=is2D,
                                 free_points=free_points, free_cats=free_cats)


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


def read_line(feature_id, c_mapinfo, table=None, writeable=False,
              c_points=None, c_cats=None, is2D=True):
    """Return a geometry object given the feature id and the c_mapinfo.
    """
    # Take care of good memory management
    free_points = False
    if c_points == None:
        free_points = True

    free_cats = False
    if c_cats == None:
        free_cats = True

    c_points = c_points if c_points else ctypes.pointer(libvect.line_pnts())
    c_cats = c_cats if c_cats else ctypes.pointer(libvect.line_cats())
    feature_id, ftype, c_points, c_cats = c_read_line(feature_id, c_mapinfo,
                                                      c_points, c_cats)
    if GV_TYPE[ftype]['obj'] is not None:
        return GV_TYPE[ftype]['obj'](v_id=feature_id, c_mapinfo=c_mapinfo,
                                     c_points=c_points, c_cats=c_cats,
                                     table=table, writeable=writeable, is2D=is2D,
                                     free_points=free_points,
                                     free_cats=free_cats)

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


Introduction to Vector classes
==============================

Details about the GRASS GIS vector architecture can be found in the
`GRASS GIS 7 Programmer's Manual: GRASS Vector Library
<http://grass.osgeo.org/programming7/vectorlib.html>`_.

PyGRASS has two classes for vector maps: :ref:`Vector-label` and
:ref:`VectorTopo-label`.  As the names suggest, the Vector class is
for vector maps, while VectorTopo opens vector maps with `GRASS GIS
topology <http://grass.osgeo.org/programming7/vlibTopology.html>`_.
VectorTopo is an extension of the Vector class, so supports all the
Vector class methods, with additions. The classes are part of the
:mod:`~pygrass.vector` module.

.. _Vector-label:

Vector
------

The :class:`~pygrass.vector.Vector` class is based on the
:class:`~pygrass.vector.abstract.Info` class, which provides methods
for accessing basic information about the vector map: ::

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

.. _VectorTopo-label:

VectorTopo
----------

The :class:`~pygrass.vector.VectorTopo` class allows vector maps to be loaded
along with an accompanying topology. The VectorTopo class interface has all the
same methods as the basic Vector class, but includes many more methods dependent
on the topology rules. Consult each class's documentation for a list of the
methods to compare what each class can do. Using the VectorTopo class is just
like the Vector class: ::

    >>> from grass.pygrass.vector import VectorTopo
    >>> municip = VectorTopo('boundary_municp_sqlite')
    >>> municip.is_open()
    False
    >>> municip.mapset
    ''
    >>> municip.exist()  # check if exist, and if True set mapset
    True
    >>> municip.mapset
    'user1'


Working with Vector Maps
------------------------

As the VectorTopo class is so similar to the Vector class, the following examples
exclusively demonstrate the VectorTopo class.

To begin using a vector map, it must first be opened: ::

    >>> from grass.pygrass.vector import VectorTopo
    >>> municip = VectorTopo('boundary_municp_sqlite')
    >>> municip.open(mode='r')

The ``open()`` method supports a number of option arguments (see the
:class:`~pygrass.vector.abstract.Info` documentation for a complete
list). In particular, the mode argument can take a a value of:

* 'r': read-only mode, vector features are read-only (attribute table
  is modifiable since are handle by a database);
* 'w': write-only mode, write a new vector map in case of an old
  vector map all the previous features will be removed/overwritten;
* 'rw': read-write mode, add new/update vector features without
  removing the existing ones. Add/remove vector layers.


The geometry of a vector map can be read sequentially using the ``next()`` method.
To return to the beginning, use the ``rewind()`` method.

    >>> municip.next()
    Boundary(v_id=1)
    >>> municip.next()
    Boundary(v_id=2)
    >>> municip.next()
    Boundary(v_id=3)
    >>> municip.rewind()
    >>> municip.next()
    Boundary(v_id=1)

If a vector map is opened with the mode ``w`` or ``rw``, then the user can write
new features to the dataset:

Open a new vector map:

    >>> new = VectorTopo('newvect')
    >>> new.exist()
    False

Define the new columns in the attribute table:

    >>> cols = [(u'cat',       'INTEGER PRIMARY KEY'),
    ...         (u'name',      'TEXT')]

Open the vector map in write mode:

    >>> new.open('w', tab_name='newvect', tab_cols=cols)

Import the geometry feature class and add two points:

    >>> from grass.pygrass.vector.geometry import Point
    >>> point0 = Point(636981.336043, 256517.602235)
    >>> point1 = Point(637209.083058, 257970.129540)

Write the two points to the map:

    >>> new.write(point0, cat=1, attrs=('pub',))
    >>> new.write(point1, cat=2, attrs=('resturant',))

Commit the DB changes (attributes):

    >>> new.table.conn.commit()
    >>> new.table.execute().fetchall()
    [(1, u'pub'), (2, u'resturnat')]

Close the vector map:

    >>> new.close()
    >>> new.exist()
    True

Now we can play with the map:

    >>> new.open(mode='r')
    >>> new.read(1)
    Point(636981.336043, 256517.602235)
    >>> new.read(2)
    Point(637209.083058, 257970.129540)
    >>> new.read(1).attrs['name']
    u'pub'
    >>> new.read(2).attrs['name']
    u'resturnat'
    >>> new.close()
    >>> new.remove()

Note the ``close()`` and ``remove()`` methods above. The ``close()`` method
ensure that the files are properly released by the operating system, and will
also build the topology (if called by the VectorTopo class). Take caution with
the ``remove()`` method; it is used here because this example map was temporary.
Calling this method will completely delete the map and its data from the file
system.


More Features of the VectorTopo Class
-------------------------------------

See the class documentation for a full list of methods, but here are some
examples using VectorTopo methods:

Get the number of primitives:

    >>> municip.num_primitive_of('line')
    0
    >>> municip.num_primitive_of('centroid')
    3579
    >>> municip.num_primitive_of('boundary')
    5128


Get the number of different feature types in the vector map: ::

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
    >>> municip.number_of("pizza")  # doctest: +ELLIPSIS +NORMALIZE_WHITESPACE
    Traceback (most recent call last):
        ...
    ValueError: vtype not supported, use one of: 'areas', ..., 'volumes'

Note that the method with raise a  ``ValueError`` if a non-supported vtype is
specified.

Accessing Attribute Tables
--------------------------

The GRASS philosophy stipulates that vector map features are independent from
their attributes, and that a vector map's attribute table(s) should not be
loaded unless explicitly specified, in case they are not needed.

Accessing a vector map's table(s) requires finding any links to tables, then
requesting the table from each of the returned links: ::

    >>> from grass.pygrass.vector import VectorTopo
    >>> municip = VectorTopo('census')
    >>> municip.open(mode='r')
    >>> dblinks = DBlinks(municip.c_mapinfo)
    >>> dblinks
    DBlinks([Link(1, census, sqlite)])
    >>> link = DBlinks[0]
    Link(1, census, sqlite)
    >>> table = link.table()

Here, :class:`~pygrass.vector.table.DBlinks` is a class that contains
all the links of a vector map. Each link is also a class
(:class:`~pygrass.vector.table.Link`) that contains a specific link's
parameters. The ``table()`` method of the link class return the linked
table as a table object (:class:`~pygrass.vector.table.Table`).

Geometry Classes
----------------

The vector package also includes a number of geometry classes,
including :class:`~pygrass.vector.geometry.Area`,
:class:`~pygrass.vector.geometry.Boundary`,
:class:`~pygrass.vector.geometry.Centroid`,
:class:`~pygrass.vector.geometry.Isle`,
:class:`~pygrass.vector.geometry.Line`, and
:class:`~pygrass.vector.geometry.Point` classes. Please consult the
:mod:`~pygrass.vector.geometry` module for a complete list of methods
for these classes, as there are many. Some basic examples are given
below.

Instantiate a Point object that could be 2 or 3D, default parameters are 0: ::

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

Create a Boundary and calculate its area: ::

    >>> bound = Boundary(points=[(0, 0), (0, 2), (2, 2), (2, 0),
    ...                          (0, 0)])
    >>> bound.area()
    4.0

Construct a Line feature and find its bounding box: ::

    >>> line = Line([(0, 0), (1, 1), (2, 0), (1, -1)])
    >>> line
    Line([Point(0.000000, 0.000000),
          Point(1.000000, 1.000000),
          Point(2.000000, 0.000000),
          Point(1.000000, -1.000000)])
    >>>bbox = line.bbox()
    >>> bbox
    Bbox(1.0, -1.0, 2.0, 0.0)

Buffer a Line feature and find the buffer centroid:

    >>> line = Line([(0, 0), (0, 2)])
    >>> area = line.buffer(10)
    >>> area.boundary
    Line([Point(-10.000000, 0.000000),...Point(-10.000000, 0.000000)])
    >>> area.centroid
    Point(0.000000, 0.000000)

More Examples
-------------

Find all areas larger than 10000m2: ::

    >>> big = [area for area in municip.viter('areas')
    ...        if area.alive() and area.area >= 10000]

The PyGRASS vector methods make complex operations rather easy. Notice the
``viter()`` method: this returns an iterator object of the vector features, so
the user can choose on which vector features to iterate without loading all the
features into memory.

We can then sort the areas by size: ::

    >>> from operator import methodcaller as method
    >>> big.sort(key = method('area'), reverse = True)  # sort the list
    >>> for area in big[:3]:
    ...     print area, area.area()
    Area(3102) 697521857.848
    Area(2682) 320224369.66
    Area(2552) 298356117.948

Or sort for the number of isles that are contained inside: ::

    >>> big.sort(key = lambda x: x.isles.__len__(), reverse = True)
    >>> for area in big[:3]:
    ...     print area, area.isles.__len__()
    ...
    Area(2682) 68
    Area(2753) 45
    Area(872) 42


Or can list only the areas containing isles: ::

    >>> area_with_isles = [area for area in big if area.isles]
    >>> area_with_isles                                   # doctest: +ELLIPSIS
    [Area(...), ..., Area(...)]


Of course is still possible work only with a specific area, with: ::

    >>> from grass.pygrass.vector.geometry import Area
    >>> area = Area(v_id=1859, c_mapinfo=municip.c_mapinfo)
    >>> area.area()
    39486.05401495844
    >>> area.bbox()  # north, south, east, west
    Bbox(175711.718494, 175393.514494, 460344.093986, 460115.281986)
    >>> area.isles
    Isles([])


Now, find an area with an island inside... ::

    >>> area = Area(v_id=2972, c_mapinfo=municip.c_mapinfo)
    >>> area.isles                                       # doctest: +ELLIPSIS
    Isles([Isle(1538), Isle(1542), Isle(1543), ..., Isle(2571)])
    >>> isle = area.isles[0]
    >>> isle.bbox()
    Bbox(199947.296494, 199280.969494, 754920.623987, 754351.812986)

.. _Vector library: http://grass.osgeo.org/programming7/vectorlib.html

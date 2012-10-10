
Vector
========

Instantiation and basic interaction. ::

    >>> from pygrass.vector import VectTopo
    >>> municip = VectTopo('boundary_municp_sqlite')
    >>> municip.is_open()
    False
    >>> municip.mapset
    ''
    >>> municip.exist()  # check if exist, and if True set mapset
    True
    >>> municip.mapset
    'user1'



Open the map with topology: ::

    >>> municip.open()

    get the number of primitive:
    >>> municip.num_primitive_of('line')
    0
    >>> municip.num_primitive_of('centroid')
    3579
    >>> municip.num_primitive_of('boundary')
    5128



ask for other feature in the vector map: ::

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


Suppose that we want to select all and only the areas that have an
area bigger than 10000m2: ::

    >>> big = [area for area in municip.viter('areas')
    ...        if area.alive() and area.area >= 10000]

it's pretty easy, isn't it?!? :-)

the method "viter" return an iterator object of the vector features,
in this way no memory is wasted... User can choose on which
vector features want to iterate...


then you can go on with python stuff like, sort by area dimension: ::

    >>> from operator import methodcaller as method
    >>> big.sort(key = method('area'), reverse = True)  # sort the list
    >>> for area in big[:3]:
    ...     print area, area.area()
    Area(3102) 697521857.848
    Area(2682) 320224369.66
    Area(2552) 298356117.948


or sort for the number of isles that are contained inside: ::

    >>> big.sort(key = lambda x: x.isles.__len__(), reverse = True)
    >>> for area in big[:3]:
    ...     print area, area.isles.__len__()
    ...
    Area(2682) 68
    Area(2753) 45
    Area(872) 42


or you may have only the list of the areas that contain isles inside, with: ::

    >>> area_with_isles = [area for area in big if area.isles]
    >>> area_with_isles                                   # doctest: +ELLIPSIS
    [Area(...), ..., Area(...)]



Of course is still possible work only with a specific area, with: ::

    >>> from pygrass.vector.geometry import Area
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


VectorTopo
----------

.. autoclass:: pygrass.vector.VectorTopo
    :members:

Vector
----------

.. autoclass:: pygrass.vector.Vector
    :members:


Vector Features
===============

Point
------

.. autoclass:: pygrass.vector.geometry.Point
    :members:


Line
-----

.. autoclass:: pygrass.vector.geometry.Line
    :members:

Boundary
--------

.. autoclass:: pygrass.vector.geometry.Boundary
    :members:

Isle
-----

.. autoclass:: pygrass.vector.geometry.Isle
    :members:


Isles
-----

.. autoclass:: pygrass.vector.geometry.Isles
    :members:

Area
--------

.. autoclass:: pygrass.vector.geometry.Boundary
    :members:

Utils
=====

Bbox
----

.. autoclass:: pygrass.vector.basic.Bbox
    :members:


BoxList
--------

.. autoclass:: pygrass.vector.basic.BoxList
    :members:

Ilist
-----

.. autoclass:: pygrass.vector.basic.Ilist
    :members:

Cats
-----

.. autoclass:: pygrass.vector.basic.Cats
    :members:
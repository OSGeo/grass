Raster elements
=================

.. _RasterCategory-label:

Categories
----------

All the raster classes support raster categories and share commons methods
to modify the raster category.
It is possible to check if the map has or not the categories with the
``has_cats`` method. ::

    >>> elev.has_cats()
    False

Opening a map that has category, for example the "landcove_1m" raster map
from the North Carolina mapset. The ``has_cats`` method return True. ::

    >>> land = raster.RasterRow('landcover_1m')
    >>> land.has_cats()
    True

Get and set the categories title, with: ::

    >>> land.cats_title
    'Rural area: Landcover'
    >>> land.cats_title = 'Rural area: Landcover2'
    >>> land.cats_title
    'Rural area: Landcover2'
    >>> land.cats_title = 'Rural area: Landcover'

Get the number of categories of the map with: ::

    >>> land.num_cats()
    11

See all the categories with: ::

    >>> land.cats
    [('pond', 1, None),
     ('forest', 2, None),
     ('developed', 3, None),
     ('bare', 4, None),
     ('paved road', 5, None),
     ('dirt road', 6, None),
     ('vineyard', 7, None),
     ('agriculture', 8, None),
     ('wetland', 9, None),
     ('bare ground path', 10, None),
     ('grass', 11, None)]

Access a single category, using Rast_get_ith_cat(), with: ::

    >>> land.cats[0]
    ('pond', 1, None)
    >>> land.cats['pond']
    ('pond', 1, None)
    >>> land.get_cat(0)
    ('pond', 1, None)
    >>> land.get_cat('pond')
    ('pond', 1, None)

Add new or change existing categories: ::

    >>> land.set_cat('label', 1)
    >>> land.get_cat('label')
    ('label', 1, None)
    >>> land.set_cat('pond', 1, 1)


Sort categories, with: ::

    >>> land.sort_cats()


Copy categories from another raster map with: ::

    >>> land.copy_cats(elev)

Read and Write: ::

    >>> land.read_cats()
    >>> #land.write_cats()

Get a Category object or set from a Category object: ::

    >>> cats = land.get_cats()
    >>> land.set_cats(cats)

Export and import from a file: ::

    >>> land.write_cats_rules('land_rules.csv', ';')
    >>> land.read_cats_rules('land_rules.csv', ';')

.. autoclass:: pygrass.raster.category.Category
    :members:

.. _Buffer-label:

Buffer
------

The buffer class is used to interact with a memory buffer of a map like a
raster row. The buffer class is based on the `numpy.ndarray`_ class. Therefore
all the nice feature of the ndarray are allowed.

.. autoclass:: pygrass.raster.buffer.Buffer
    :members:

.. _numpy.ndarray: http://docs.scipy.org/doc/numpy/reference/generated/numpy.ndarray.html


.. _RowIO-label:

RowIO
------

.. autoclass:: pygrass.raster.rowio.RowIO
    :members:

.. _Segment-label:

Segment
-------

.. autoclass:: pygrass.raster.segment.Segment
    :members:

.. _History-label:

History
--------

.. autoclass:: pygrass.raster.history.History
    :members:
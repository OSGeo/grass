.. _raster-label:

Raster
======

PyGrass use 4 different Raster classes, that respect the 4 different approaches
of C grass API.
The read access is row wise for :ref:`RasterRow-label` and
:ref:`RasterRowIO-label` and additionally
cached in the RowIO class. Booth classes write sequentially.
RowIO is row cached, :ref:`RasterSegment-label` and :ref:`RasterNumpy-label`
are tile cached for reading and writing therefore a randomly access is possible.
Hence RasterRow and RasterRowIO should be used in case for fast (cached)
row read access and RasterRow for fast sequential writing.
Segment and Numpy should be used for random access, but numpy only for files
not larger than 2GB.


==========================  =======================  ========  ============
Class Name                  C library                Read      Write
==========================  =======================  ========  ============
:ref:`RasterRow-label`      `Raster library`_        randomly  sequentially
:ref:`RasterRowIO-label`    `RowIO library`_         cached    no
:ref:`RasterSegment-label`  `Segmentation library`_  cached    randomly
:ref:`RasterNumpy-label`    `numpy.memmap`_          cached    randomly
==========================  =======================  ========  ============


All these classes share common methods and attributes, necessary to address
common tasks as rename, remove, open, close, exist, is_open.
In the next examples we instantiate a RasterRow object. ::

    >>> from pygrass import raster
    >>> elev = raster.RasterRow('elevation')
    >>> elev.name
    'elevation'
    >>> print(elev)
    elevation@PERMANENT
    >>> elev.exist()
    True
    >>> elev.is_open()
    False
    >>> new = raster.RasterRow('new')
    >>> new.exist()
    False
    >>> new.is_open()
    False


We can rename the map: ::

    >>> # setting the attribute
    >>> new.name = 'new_map'
    >>> print(new)
    new_map
    >>> # or using the rename methods
    >>> new.rename('new')
    >>> print(new)
    new




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

Access to single category, using Rast_get_ith_cat(), with: ::

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


.. _RasterRow-label:

RastRow
-------

PyGrass allow user to open the maps, in read and write mode,
row by row using the `Raster library`_, there is not support to read and write
to the same map at the same time, for this functionality, please see the
:ref:`RasterSegment-label` and :ref:`RasterNumpy-label` classes.
The RasterRow class allow to read in a randomly order the row from a map, but
it is only possible to write the map using only a sequence order, therefore every
time you are writing a new map, the row is add to the file as the last row. ::

    >>> raster = reload(raster)
    >>> elev = raster.RasterRow('elevation')
    >>> # the cols attribute is set from the current region only when the map is open
    >>> elev.cols
    >>> elev.open()
    >>> elev.is_open()
    True
    >>> elev.cols
    1500
    >>> # we can read the elevation map, row by row
    >>> for row in elev[:5]: print(row[:3])
    [ 141.99613953  141.27848816  141.37904358]
    [ 142.90461731  142.39450073  142.68611145]
    [ 143.81854248  143.54707336  143.83972168]
    [ 144.56524658  144.58493042  144.86477661]
    [ 144.99488831  145.22894287  145.57142639]
    >>> # we can open a new map in write mode
    >>> new = raster.RasterRow('new')
    >>> new.open('w', 'CELL')
    >>> # for each elev row we can perform computation, and write the result into
    >>> # the new map
    >>> for row in elev:
    ...     new.put_row(row < 144)
    ...
    >>> # close the maps
    >>> new.close()
    >>> elev.close()
    >>> # check if the map exist
    >>> new.exist()
    True
    >>> # we can open the map in read mode
    >>> new.open('r')
    >>> for row in new[:5]: print(row[:3])
    [1 1 1]
    [1 1 1]
    [1 1 1]
    [0 0 0]
    [0 0 0]
    >>> new.close()
    >>> new.remove()
    >>> new.exist()
    False


.. _RasterRowIO-label:

RasterRowIO
-----------

The RasterRowIO class use the grass `RowIO library`_, and implement a row
cache. The RasterRowIO class support only reading the raster, because the
raster rows can only be written in sequential order, writing by row id is not
supported by design. Hence, we should use the rowio lib only for caching rows
for reading and use the default row write access as in the RasterRow class. ::

    >>> raster = reload(raster)
    >>> elev = raster.RasterRowIO('elevation')
    >>> elev.open('r')
    >>> for row in elev[:5]: print(row[:3])
    [ 141.99613953  141.27848816  141.37904358]
    [ 142.90461731  142.39450073  142.68611145]
    [ 143.81854248  143.54707336  143.83972168]
    [ 144.56524658  144.58493042  144.86477661]
    [ 144.99488831  145.22894287  145.57142639]
    >>> elev.close()



.. _RasterSegment-label:

RastSegment
-----------

The RasterSegment class use the grass `Segmentation library`_, it work dividing
the raster map into small different files, that grass read load into the memory
and write to the hardisk.
The segment library allow to open a map in a read-write mode. ::

    >>> raster = reload(raster)
    >>> elev = raster.RasterSegment('elevation')
    >>> elev.open()
    >>> for row in elev[:5]: print(row[:3])
    [ 141.99613953  141.27848816  141.37904358]
    [ 142.90461731  142.39450073  142.68611145]
    [ 143.81854248  143.54707336  143.83972168]
    [ 144.56524658  144.58493042  144.86477661]
    [ 144.99488831  145.22894287  145.57142639]
    >>> new = raster.RasterSegment('new')
    >>> new.open('w', 'CELL')
    >>> for irow in xrange(elev.rows):
    ...     new[irow] = elev[irow] < 144
    ...
    >>> for row in new[:5]: print(row[:3])
    [1 1 1]
    [1 1 1]
    [1 1 1]
    [0 0 0]
    [0 0 0]

The RasterSegment class define two methods to read and write the map:

    * ``get_row`` that return the buffer object with the row that call the
      C function ``segment_get_row``. ::

        >>> # call explicity the method
        >>> elev_row0 = elev.get_row(0)
        >>> # call implicity the method
        >>> elev_row0 = elev[0]

    * ``get`` that return the value of the call map that call the
      C function ``segment_get``. ::

        >>> # call explicity the method
        >>> elev_val_0_0 = elev.get(0, 0)
        >>> # call implicity the method
        >>> elev_val_0_0 = elev[0, 0]

Similarly to write the map, with ``put_row``, to write a row and with ``put``
to write a single value to the map. ::

    >>> # compare the cell value get using the ``get`` method, and take the first
    >>> # value of the row with the ``get_row`` method
    >>> elev[0, 0] == elev[0][0]
    True
    >>> # write a new value to a cell,
    >>> new[0, 0] = 10
    >>> new[0, 0]
    10
    >>> new.close()
    >>> new.exist()
    True
    >>> new.remove()
    >>> elev.close()
    >>> elev.remove()



.. _RasterNumpy-label:

RasterNumpy
-----------

The RasterNumpy class, is based on the `numpy.memmap`_ class If you open an
existing map, the map will be copied on a binary format, and read to avoid
to load all the map in memory. ::

    >>> raster = reload(raster)
    >>> elev = raster.RasterNumpy('elevation', 'PERMANENT')
    >>> elev.open('r')
    >>> # in this case RasterNumpy is an extention of the numpy class
    >>> # therefore you may use all the fancy things of numpy.
    >>> elev[:5, :3]
    RasterNumpy([[ 141.99613953,  141.27848816,  141.37904358],
           [ 142.90461731,  142.39450073,  142.68611145],
           [ 143.81854248,  143.54707336,  143.83972168],
           [ 144.56524658,  144.58493042,  144.86477661],
           [ 144.99488831,  145.22894287,  145.57142639]], dtype=float32)
    >>> el = elev < 144
    >>> el[:5, :3]
    RasterNumpy([[1, 1, 1],
           [1, 1, 1],
           [1, 1, 1],
           [0, 0, 0],
           [0, 0, 0]], dtype=int32)
    >>> el.name == None
    True
    >>> # give a name to the new map
    >>> el.name = 'new'
    >>> el.exist()
    False
    >>> el.close()
    >>> el.exist()
    True
    >>> el.remove()



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


.. _Category-label:

Category
--------

.. autoclass:: pygrass.raster.category.Category
    :members:

.. _Raster library: http://grass.osgeo.org/programming7/rasterlib.html/
.. _RowIO library: http://grass.osgeo.org/programming7/rowiolib.html
.. _Segmentation library: http://grass.osgeo.org/programming7/segmentlib.html
.. _numpy.memmap: http://docs.scipy.org/doc/numpy/reference/generated/numpy.memmap.html


.. _raster-label:

Introduction to Raster classes
==============================

Details about the GRASS GIS raster architecture can be found in the
`GRASS GIS 7 Programmer's Manual: GRASS Raster Library <http://grass.osgeo.org/programming7/rasterlib.html>`_

PyGRASS uses 4 different Raster classes, that respect the 4 different approaches
of GRASS-C API. The classes use a standardized interface to keep methods
consistent between them. The read access is row wise for :ref:`RasterRow-label`
and :ref:`RasterRowIO-label` and additionally
cached in the RowIO class. Both classes write sequentially.
RowIO is row cached, :ref:`RasterSegment-label` and :ref:`RasterNumpy-label`
are tile cached for reading and writing; therefore, random access is possible.
Hence RasterRow and RasterRowIO should be used for fast (cached)
row read access and RasterRow for fast sequential writing.
RasterSegment and RasterNumpy should be used for random access.


==========================  =======================  ========  ============
Class Name                  C library                Read      Write
==========================  =======================  ========  ============
:ref:`RasterRow-label`      `Raster library`_        randomly  sequentially
:ref:`RasterRowIO-label`    `RowIO library`_         cached    no
:ref:`RasterSegment-label`  `Segmentation library`_  cached    randomly
:ref:`RasterNumpy-label`    `numpy.memmap`_          cached    randomly
==========================  =======================  ========  ============


These classes share common methods and attributes to address
common tasks as rename, remove, open, close, exist, is_open.
In the following example we instantiate a RasterRow object. ::

    >>> from grass.pygrass import raster
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

.. _RasterRow-label:

RasterRow
---------

The PyGrass :class:`~pygrass.raster.RasterRow` class allow user to open maps row
by row in either read or write mode using the `Raster library`_. Reading and writing
to the same map at the same time is not supported. For this functionality,
please see the :ref:`RasterSegment-label` and :ref:`RasterNumpy-label` classes.
The RasterRow class allows map rows to be read in any order, but map rows can
only be written in sequential order. Therefore, each now row written to a map is
added to the file as the last row. ::

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

The :class:`~pygrass.raster.RasterRowIO` class uses the GRASS `RowIO library`_, and implements a row
cache. The RasterRowIO class only supports reading rasters; because raster rows
can only be written in sequential order, writing by row id is not
supported by design. Hence, the rowio lib can only be used to cache rows
for reading, and any write access should use the :ref:`RasterRow-label` class. ::

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

RasterSegment
-------------

The :class:`~pygrass.raster.RasterSegment` class uses the GRASS `Segmentation library`_. The class divides
a raster map into small tiles stored on disk. Initialization of this class is
therefore intensive. However, this class has lower memory requirements, as GRASS
loads only currently-accessed tiles into memory. The segment library allow
opening maps in a read-write mode. ::

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

Due to the unique behavior of this class, the RasterSegment class defines two
methods to read a map:

    * ``get_row`` calls the C function ``Segment_get_row`` and returns a buffer
      object with the row. ::

        >>> # call explicity the method
        >>> elev_row0 = elev.get_row(0)
        >>> # call implicity the method
        >>> elev_row0 = elev[0]

    * ``get`` calls the C function ``Segment_get`` and returns the value of the
      map cell. ::

        >>> # call explicity the method
        >>> elev_val_0_0 = elev.get(0, 0)
        >>> # call implicity the method
        >>> elev_val_0_0 = elev[0, 0]

Similarly, writing to a map uses two methods: ``put_row`` to write a row and
``put`` to write a single value to the map. ::

    >>> # compare the cell value get using the ``get`` method, and take the first
    >>> # value of the row with the ``get_row`` method
    >>> # the methods are used internally by the index operators
    >>> elev[0, 0] == elev[0][0]
    True
    >>> # write a new value to a cell,
    >>> new[0, 0] = 10  # ``put`` is used internally by the index operators
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

The :class:`~pygrass.raster.RasterNumpy` class, is based on the `numpy.memmap`_ class
(refer to the linked documentation for details). If an existing map is opened,
small sections will be loaded into memory as accessed, but GRASS does not need to
load the whole file into memory. Memmap is a subclass of the `numpy.ndarray`
class, so RasterNumpy will behave like a numpy array and can be used with numpy
array operations. ::

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


.. _Raster library: http://grass.osgeo.org/programming7/rasterlib.html
.. _RowIO library: http://grass.osgeo.org/programming7/rowiolib.html
.. _Segmentation library: http://grass.osgeo.org/programming7/segmentlib.html
.. _numpy.memmap: http://docs.scipy.org/doc/numpy/reference/generated/numpy.memmap.html

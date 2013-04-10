"""!@package grass.script.array

@brief GRASS Python scripting module (2D and 3D raster with numpy)

Functions to use GRASS 2D and 3D rasters with NumPy.

Usage:

@code

>>> import grass.script as grass
>>> from grass.script import array as garray
>>> 
>>> # We create a temporary region that is only valid in this python session
... grass.use_temp_region()
>>> grass.run_command("g.region", n=80, e=120, t=60, s=0, w=0, b=0, res=20, res3=20)
0
>>> 
>>> # Lets create a raster map numpy array
... # based at the current region settings
... map2d_1 = garray.array()
>>> 
>>> # Write some data
... for y in range(map2d_1.shape[0]):
...     for x in range(map2d_1.shape[1]):
...         map2d_1[y][x] = y + x
... 
>>> # Lets have a look at the array
... print map2d_1
[[ 0.  1.  2.  3.  4.  5.]
 [ 1.  2.  3.  4.  5.  6.]
 [ 2.  3.  4.  5.  6.  7.]
 [ 3.  4.  5.  6.  7.  8.]]
>>> # This will write the numpy array as GRASS raster map
... # with name map2d_1
... map2d_1.write(mapname="map2d_1", overwrite=True)
 100%
0
>>> 
>>> # We create a new array and read map2d_1 to modify it
... map2d_2 = garray.array()
>>> # Don't do map2d_2 = map2d_1 % 3
... # because: this will overwrite the internal temporary filename
... map2d_2.read("map2d_1")
0
>>> map2d_2 %= 3
>>> # Show the result
... print map2d_2
[[ 0.  1.  2.  0.  1.  2.]
 [ 1.  2.  0.  1.  2.  0.]
 [ 2.  0.  1.  2.  0.  1.]
 [ 0.  1.  2.  0.  1.  2.]]
>>> # Write the result as new raster map with name map2d_2
... map2d_2.write(mapname="map2d_2", overwrite=True)
 100%
0
>>> 
>>> # Here we create a 3D raster map numpy array
... # based in the current region settings
... map3d_1 = garray.array3d()
>>> 
>>> # Write some data
... # Note: the 3D array has map[depth][row][column] order
... for z in range(map3d_1.shape[0]):
...     for y in range(map3d_1.shape[1]):
...         for x in range(map3d_1.shape[2]):
...             map3d_1[z][y][x] = z + y + x
... 
>>> # Lets have a look at the 3D array
... print map3d_1
[[[  0.   1.   2.   3.   4.   5.]
  [  1.   2.   3.   4.   5.   6.]
  [  2.   3.   4.   5.   6.   7.]
  [  3.   4.   5.   6.   7.   8.]]

 [[  1.   2.   3.   4.   5.   6.]
  [  2.   3.   4.   5.   6.   7.]
  [  3.   4.   5.   6.   7.   8.]
  [  4.   5.   6.   7.   8.   9.]]

 [[  2.   3.   4.   5.   6.   7.]
  [  3.   4.   5.   6.   7.   8.]
  [  4.   5.   6.   7.   8.   9.]
  [  5.   6.   7.   8.   9.  10.]]]
>>> # This will write the numpy array as GRASS 3D raster map
... # with name map3d_1
... map3d_1.write(mapname="map3d_1", overwrite=True)
Loading floating point data with 8 bytes ... (6x4x3)
 100%
0
>>> # We create a new 3D array and read map3d_1 to modify it
... map3d_2 = garray.array3d()
>>> # Don't do map3d_2 = map3d_1 % 3
... # because: this will overwrite the internal temporary filename
... map3d_2.read("map3d_1")
0
>>> map3d_2 %= 3
>>> # Show the result
... print map3d_2
[[[ 0.  1.  2.  0.  1.  2.]
  [ 1.  2.  0.  1.  2.  0.]
  [ 2.  0.  1.  2.  0.  1.]
  [ 0.  1.  2.  0.  1.  2.]]

 [[ 1.  2.  0.  1.  2.  0.]
  [ 2.  0.  1.  2.  0.  1.]
  [ 0.  1.  2.  0.  1.  2.]
  [ 1.  2.  0.  1.  2.  0.]]

 [[ 2.  0.  1.  2.  0.  1.]
  [ 0.  1.  2.  0.  1.  2.]
  [ 1.  2.  0.  1.  2.  0.]
  [ 2.  0.  1.  2.  0.  1.]]]
>>> # Write the result as new 3D raster map with name map3d_2
... map3d_2.write(mapname="map3d_2", overwrite=True)
Loading floating point data with 8 bytes ... (6x4x3)
 100%
0

@endcode

(C) 2010-2012 by Glynn Clements and the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Glynn Clements
"""

import os
import numpy

import core as grass

###############################################################################

class array(numpy.memmap):
    def __new__(cls, dtype=numpy.double):
        """!Define new numpy array

        @param cls
        @param dtype data type (default: numpy.double)
        """
        reg = grass.region()
        r = reg['rows']
        c = reg['cols']
        shape = (r, c)

        filename = grass.tempfile()

        self = numpy.memmap.__new__(
            cls,
            filename=filename,
            dtype=dtype,
            mode='w+',
            shape=shape)

        self.filename = filename
        return self

    def _close(self):
        numpy.memmap._close(self)
        if isinstance(self, array):
            grass.try_remove(self.filename)

    def read(self, mapname, null=None):
        """!Read raster map into array

        @param mapname name of raster map to be read
        @param null null value

        @return 0 on success
        @return non-zero code on failure
        """
        kind = self.dtype.kind
        size = self.dtype.itemsize

        if kind == 'f':
            flags = 'f'
        elif kind in 'biu':
            flags = 'i'
        else:
            raise ValueError(_('Invalid kind <%s>') % kind)

        if size not in [1, 2, 4, 8]:
            raise ValueError(_('Invalid size <%d>') % size)

        return grass.run_command(
            'r.out.bin',
            flags=flags,
            input=mapname,
            output=self.filename,
            bytes=size,
            null=null,
            quiet=True,
            overwrite=True)

    def write(self, mapname, title=None, null=None, overwrite=None):
        """!Write array into raster map

        @param mapname name for raster map
        @param title title for raster map
        @param null null value
        @param overwrite True for overwritting existing raster maps

        @return 0 on success
        @return non-zero code on failure
        """
        kind = self.dtype.kind
        size = self.dtype.itemsize

        if kind == 'f':
            if size == 4:
                flags = 'f'
            elif size == 8:
                flags = 'd'
            else:
                raise ValueError(_('Invalid FP size <%d>') % size)
            size = None
        elif kind in 'biu':
            if size not in [1, 2, 4]:
                raise ValueError(_('Invalid integer size <%d>') % size)
            flags = None
        else:
            raise ValueError(_('Invalid kind <%s>') % kind)

        reg = grass.region()

        return grass.run_command(
            'r.in.bin',
            flags=flags,
            input=self.filename,
            output=mapname,
            title=title,
            bytes=size,
            anull=null,
            overwrite=overwrite,
            north=reg['n'],
            south=reg['s'],
            east=reg['e'],
            west=reg['w'],
            rows=reg['rows'],
            cols=reg['cols'])

###############################################################################

class array3d(numpy.memmap):
    def __new__(cls, dtype=numpy.double):
        """!Define new 3d numpy array

        @param cls
        @param dtype data type (default: numpy.double)
        """
        reg = grass.region(True)
        r = reg['rows3']
        c = reg['cols3']
        d = reg['depths']
        shape = (d, r, c)

        filename = grass.tempfile()

        self = numpy.memmap.__new__(
            cls,
            filename=filename,
            dtype=dtype,
            mode='w+',
            shape=shape)

        self.filename = filename

        return self

    def _close(self):

        numpy.memmap._close(self)
        if isinstance(self, array):
            grass.try_remove(self.filename)

    def read(self, mapname, null=None):
        """!Read 3D raster map into array

        @param mapname name of 3D raster map to be read
        @param null null value

        @return 0 on success
        @return non-zero code on failure
        """
        kind = self.dtype.kind
        size = self.dtype.itemsize

        if kind == 'f':
            flags = None # default is double
        elif kind in 'biu':
            flags = 'i'
        else:
            raise ValueError(_('Invalid kind <%s>') % kind)

        if size not in [1, 2, 4, 8]:
            raise ValueError(_('Invalid size <%d>') % size)

        return grass.run_command(
            'r3.out.bin',
            flags=flags,
            input=mapname,
            output=self.filename,
            bytes=size,
            null=null,
            quiet=True,
            overwrite=True)

    def write(self, mapname, null=None, overwrite=None):
        """!Write array into 3D raster map

        @param mapname name for 3D raster map
        @param null null value
        @param overwrite True for overwriting existing raster maps

        @return 0 on success
        @return non-zero code on failure
        """
        kind = self.dtype.kind
        size = self.dtype.itemsize
        flags = None

        if kind == 'f':
            if size != 4 and size != 8:
                raise ValueError(_('Invalid FP size <%d>') % size)
        elif kind in 'biu':
            if size not in [1, 2, 4, 8]:
                raise ValueError(_('Invalid integer size <%d>') % size)
            flags = 'i'
        else:
            raise ValueError(_('Invalid kind <%s>') % kind)

        reg = grass.region(True)

        return grass.run_command(
            'r3.in.bin',
            flags=flags,
            input=self.filename,
            output=mapname,
            bytes=size,
            null=null,
            overwrite=overwrite,
            north=reg['n'],
            south=reg['s'],
            top=reg['t'],
            bottom=reg['b'],
            east=reg['e'],
            west=reg['w'],
            depths=reg['depths'],
            rows=reg['rows3'],
            cols=reg['cols3'])


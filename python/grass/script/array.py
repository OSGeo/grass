"""
Functions to use GRASS 2D and 3D rasters with NumPy.

Usage:

>>> import grass.script as gs
>>> from grass.script import array as garray
>>>
>>> # We create a temporary region that is only valid in this python session
... gs.use_temp_region()
>>> gs.run_command("g.region", n=80, e=120, t=60, s=0, w=0, b=0, res=20, res3=20)
>>>
>>> # Lets create a raster map numpy array
... # based at the current region settings
... map2d_1 = garray.array()
>>>
>>> # Write some data
... for y in range(map2d_1.shape[0]):
...     for x in range(map2d_1.shape[1]):
...         map2d_1[y,x] = y + x
...
>>> # Lets have a look at the array
... print(map2d_1)
[[0. 1. 2. 3. 4. 5.]
 [1. 2. 3. 4. 5. 6.]
 [2. 3. 4. 5. 6. 7.]
 [3. 4. 5. 6. 7. 8.]]
>>> # This will write the numpy array as GRASS raster map
... # with name map2d_1
... map2d_1.write(mapname="map2d_1", overwrite=True)
0
>>>
>>> # We create a new array from raster map2d_1 to modify it
... map2d_2 = garray.array(mapname="map2d_1")
>>> # Don't do map2d_2 = map2d_1 % 3
... # because: this will overwrite the internal temporary filename
... map2d_2 %= 3
>>> # Show the result
... print(map2d_2)
[[0. 1. 2. 0. 1. 2.]
 [1. 2. 0. 1. 2. 0.]
 [2. 0. 1. 2. 0. 1.]
 [0. 1. 2. 0. 1. 2.]]
>>> # Write the result as new raster map with name map2d_2
... map2d_2.write(mapname="map2d_2", overwrite=True)
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
...             map3d_1[z,y,x] = z + y + x
...
>>> # Lets have a look at the 3D array
... print(map3d_1)
[[[ 0.  1.  2.  3.  4.  5.]
  [ 1.  2.  3.  4.  5.  6.]
  [ 2.  3.  4.  5.  6.  7.]
  [ 3.  4.  5.  6.  7.  8.]]
<BLANKLINE>
 [[ 1.  2.  3.  4.  5.  6.]
  [ 2.  3.  4.  5.  6.  7.]
  [ 3.  4.  5.  6.  7.  8.]
  [ 4.  5.  6.  7.  8.  9.]]
<BLANKLINE>
 [[ 2.  3.  4.  5.  6.  7.]
  [ 3.  4.  5.  6.  7.  8.]
  [ 4.  5.  6.  7.  8.  9.]
  [ 5.  6.  7.  8.  9. 10.]]]
>>> # This will write the numpy array as GRASS 3D raster map
... # with name map3d_1
... map3d_1.write(mapname="map3d_1", overwrite=True)
0
>>> # We create a new 3D array from 3D raster map3d_1 to modify it
... map3d_2 = garray.array3d(mapname="map3d_1")
>>> # Don't do map3d_2 = map3d_1 % 3
... # because: this will overwrite the internal temporary filename
... map3d_2 %= 3
>>> # Show the result
... print(map3d_2)
[[[0. 1. 2. 0. 1. 2.]
  [1. 2. 0. 1. 2. 0.]
  [2. 0. 1. 2. 0. 1.]
  [0. 1. 2. 0. 1. 2.]]
<BLANKLINE>
 [[1. 2. 0. 1. 2. 0.]
  [2. 0. 1. 2. 0. 1.]
  [0. 1. 2. 0. 1. 2.]
  [1. 2. 0. 1. 2. 0.]]
<BLANKLINE>
 [[2. 0. 1. 2. 0. 1.]
  [0. 1. 2. 0. 1. 2.]
  [1. 2. 0. 1. 2. 0.]
  [2. 0. 1. 2. 0. 1.]]]
>>> # Write the result as new 3D raster map with name map3d_2
... map3d_2.write(mapname="map3d_2", overwrite=True)
0

(C) 2010-2021 by Glynn Clements and the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

.. sectionauthor:: Glynn Clements
"""

import numpy as np

from .utils import try_remove
from . import core as gcore
from grass.exceptions import CalledModuleError


###############################################################################


class _tempfile:
    def __init__(self, env=None):
        self.filename = gcore.tempfile(env=env)

    def __del__(self):
        try_remove(self.filename)


###############################################################################


class array(np.memmap):
    # pylint: disable-next=signature-differs; W0222
    def __new__(cls, mapname=None, null=None, dtype=np.double, env=None):
        """Define new numpy array

        :param cls:
        :param dtype: data type (default: numpy.double)
        :param env: environment
        """
        reg = gcore.region(env=env)
        r = reg["rows"]
        c = reg["cols"]
        shape = (r, c)

        tempfile = _tempfile(env)
        if mapname:
            kind = np.dtype(dtype).kind
            size = np.dtype(dtype).itemsize

            if kind == "f":
                flags = "f"
            elif kind in "biu":
                flags = "i"
            else:
                raise ValueError(_("Invalid kind <%s>") % kind)

            if size not in {1, 2, 4, 8}:
                raise ValueError(_("Invalid size <%d>") % size)

            gcore.run_command(
                "r.out.bin",
                flags=flags,
                input=mapname,
                output=tempfile.filename,
                bytes=size,
                null=null,
                quiet=True,
                overwrite=True,
                env=env,
            )

        self = np.memmap.__new__(
            cls, filename=tempfile.filename, dtype=dtype, mode="r+", shape=shape
        )

        self.tempfile = tempfile
        self.filename = tempfile.filename
        self._env = env
        return self

    def write(self, mapname, title=None, null=None, overwrite=None, quiet=None):
        """Write array into raster map

        :param str mapname: name for raster map
        :param str title: title for raster map
        :param null: null value
        :param bool overwrite: True for overwriting existing raster maps

        :return: 0 on success
        :return: non-zero code on failure
        """
        kind = self.dtype.kind
        size = self.dtype.itemsize

        if kind == "f":
            if size == 4:
                flags = "f"
            elif size == 8:
                flags = "d"
            else:
                raise ValueError(_("Invalid FP size <%d>") % size)
            size = None
        elif kind in "biu":
            if size not in {1, 2, 4}:
                raise ValueError(_("Invalid integer size <%d>") % size)
            flags = None
        else:
            raise ValueError(_("Invalid kind <%s>") % kind)

        # ensure all array content is written to the file
        self.flush()

        reg = gcore.region(env=self._env)

        try:
            gcore.run_command(
                "r.in.bin",
                flags=flags,
                input=self.filename,
                output=mapname,
                title=title,
                bytes=size,
                anull=null,
                overwrite=overwrite,
                quiet=quiet,
                north=reg["n"],
                south=reg["s"],
                east=reg["e"],
                west=reg["w"],
                rows=reg["rows"],
                cols=reg["cols"],
                env=self._env,
            )
        except CalledModuleError:
            return 1
        else:
            return 0


###############################################################################


class array3d(np.memmap):
    # pylint: disable-next=signature-differs; W0222
    def __new__(cls, mapname=None, null=None, dtype=np.double, env=None):
        """Define new 3d numpy array

        :param cls:
        :param dtype: data type (default: numpy.double)
        :param env: environment
        """
        reg = gcore.region(True)
        r = reg["rows3"]
        c = reg["cols3"]
        d = reg["depths"]
        shape = (d, r, c)

        tempfile = _tempfile()
        if mapname:
            kind = np.dtype(dtype).kind
            size = np.dtype(dtype).itemsize

            if kind == "f":
                flags = None  # default is double
            elif kind in "biu":
                flags = "i"
            else:
                raise ValueError(_("Invalid kind <%s>") % kind)

            if size not in {1, 2, 4, 8}:
                raise ValueError(_("Invalid size <%d>") % size)

            gcore.run_command(
                "r3.out.bin",
                flags=flags,
                input=mapname,
                output=tempfile.filename,
                bytes=size,
                null=null,
                quiet=True,
                overwrite=True,
                env=env,
            )

        self = np.memmap.__new__(
            cls, filename=tempfile.filename, dtype=dtype, mode="r+", shape=shape
        )

        self.tempfile = tempfile
        self.filename = tempfile.filename
        self._env = env

        return self

    def write(self, mapname, null=None, overwrite=None, quiet=None):
        """Write array into 3D raster map

        :param str mapname: name for 3D raster map
        :param null: null value
        :param bool overwrite: True for overwriting existing raster maps

        :return: 0 on success
        :return: non-zero code on failure
        """
        kind = self.dtype.kind
        size = self.dtype.itemsize
        flags = None

        if kind == "f":
            if size not in {4, 8}:
                raise ValueError(_("Invalid FP size <%d>") % size)
        elif kind in "biu":
            if size not in {1, 2, 4, 8}:
                raise ValueError(_("Invalid integer size <%d>") % size)
            flags = "i"
        else:
            raise ValueError(_("Invalid kind <%s>") % kind)

        # ensure all array content is written to the file
        self.flush()

        reg = gcore.region(True, env=self._env)

        try:
            gcore.run_command(
                "r3.in.bin",
                flags=flags,
                input=self.filename,
                output=mapname,
                bytes=size,
                null=null,
                overwrite=overwrite,
                quiet=quiet,
                north=reg["n"],
                south=reg["s"],
                top=reg["t"],
                bottom=reg["b"],
                east=reg["e"],
                west=reg["w"],
                depths=reg["depths"],
                rows=reg["rows3"],
                cols=reg["cols3"],
                env=self._env,
            )

        except CalledModuleError:
            return 1
        else:
            return 0

"""!@package grass.script.array

@brief GRASS Python scripting module

Functions to use GRASS rasters with NumPy.

Usage:

@code
from grass.script import array as garray
...
@endcode

(C) 2010 by Glynn Clements and the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Glynn Clements
"""

import os
import numpy

import core as grass

# i18N
import gettext
gettext.install('grasslibs', os.path.join(os.getenv("GISBASE"), 'locale'), unicode=True)

class array(numpy.memmap):
    def __new__(cls, dtype = numpy.double):
	reg = grass.region()
	r = reg['rows']
	c = reg['cols']
	shape = (r, c)

	filename = grass.tempfile()

	self = numpy.memmap.__new__(
	    cls,
	    filename = filename,
	    dtype = dtype,
	    mode = 'w+',
	    shape = shape)

	self.filename = filename
	return self

    def _close(self):
	numpy.memmap._close(self)
	if isinstance(self, array):
	    grass.try_remove(self.filename)

    def read(self, mapname, null = None):
	kind = self.dtype.kind
	size = self.dtype.itemsize

	if kind == 'f':
	    flags = 'f'
	elif kind in 'biu':
	    flags = 'i'
	else:
	    raise ValueError(_('invalid kind <%s>') % kind)

	if size not in [1,2,4,8]:
	    raise ValueError(_('invalid size <%d>') % size)

	return grass.run_command(
	    'r.out.bin',
	    flags = flags,
	    input = mapname,
	    output = self.filename,
	    bytes = size,
	    null = null,
	    quiet = True)
	

    def write(self, mapname, title = None, null = None, overwrite = None):
	kind = self.dtype.kind
	size = self.dtype.itemsize

	if kind == 'f':
	    if size == 4:
		flags = 'f'
	    elif size == 8:
		flags = 'd'
	    else:
		raise ValueError(_('invalid FP size <%d>') % size)
	    size = None
	elif kind in 'biu':
	    if size not in [1,2,4]:
		raise ValueError(_('invalid integer size <%d>') % size)
	    flags = None
	else:
	    raise ValueError(_('invalid kind <%s>') % kind)

	reg = grass.region()

	return grass.run_command(
	    'r.in.bin',
	    flags = flags,
	    input = self.filename,
	    output = mapname,
	    title = title,
	    bytes = size,
	    anull = null,
	    overwrite = overwrite,
	    verbose = True,
	    north = reg['n'],
	    south = reg['s'],
	    east  = reg['e'],
	    west  = reg['w'],
	    rows  = reg['rows'],
	    cols  = reg['cols'])

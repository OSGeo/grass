"""!@package grass.script.raster3d

@brief GRASS Python scripting module (raster3d functions)

Raster3d related functions to be used in Python scripts.

Usage:

@code
from grass.script import raster3d as grass

grass.raster3d_info(map)
...
@endcode

(C) 2008-2009 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Glynn Clements
@author Martin Landa <landa.martin gmail.com>
@author Soeren Gebbert <soeren.gebbert gmail.com>
"""

import os
import string

from core import *

# add raster history

# run "r3.info -rstgip ..." and parse output

def raster3d_info(map):
    """!Return information about a raster3d map (interface to
    `r3.info'). Example:

    \code
    >>> grass.raster3d_info('volume')
    {'tiledimz': 1, 'tbres': 1.0, 'tiledimx': 27, 'tiledimy': 16, 'north': 60.490001999999997, 'tilenumy': 1, 'tilenumz': 1, 
    'min': 1.0, 'datatype': '"DCELL"', 'max': 1.0, 'top': 0.5, 'bottom': -0.5, 'west': -3.2200000000000002, 'tilenumx': 1, 
    'ewres': 0.98222219, 'east': 23.299999, 'nsres': 0.99937511999999995, 'Timestamp': '"none"', 'south': 44.5}
    \endcode

    @param map map name
    
    @return parsed raster3d info
    """

    def float_or_null(s):
        if s == 'NULL':
            return None
        else:
            return float(s)

    s = read_command('r3.info', flags='rg', map=map)
    kv = parse_key_val(s)
    for k in ['min', 'max']:
        kv[k] = float_or_null(kv[k])
    for k in ['north', 'south', 'east', 'west', 'top', 'bottom']:
        kv[k] = float(kv[k])
    for k in ['nsres', 'ewres', 'tbres']:
        kv[k] = float_or_dms(kv[k])
    for k in ['tilenumx', 'tilenumy', 'tilenumz']:
        kv[k] = int(kv[k])
    for k in ['tiledimx', 'tiledimy', 'tiledimz']:
        kv[k] = int(kv[k])
    return kv

# interface to r3.mapcalc

def mapcalc3d(exp, quiet = False, verbose = False, overwrite = False, **kwargs):
    """!Interface to r3.mapcalc.

    @param exp expression
    @param quiet True to run quietly (<tt>--q</tt>)
    @param verbose True to run verbosely (<tt>--v</tt>)
    @param overwrite True to enable overwriting the output (<tt>--o</tt>)
    @param kwargs
    """
    t = string.Template(exp)
    e = t.substitute(**kwargs)

    if run_command('r3.mapcalc', expression = e,
                   quiet = quiet,
                   verbose = verbose,
                   overwrite = overwrite) != 0:
        fatal(_("An error occurred while running r3.mapcalc"))

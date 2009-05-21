"""
@package grass.script.raster

@brief GRASS Python scripting module

Raster related functions to be used in Python scripts.

Usage:

@code
from grass.script import core, raster as grass

grass.parser()
grass.raster_history(map)
...
@endcode

(C) 2008-2009 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Glynn Clements
@author Martin Landa <landa.martin gmail.com>
"""

import os
import string

from core import *

# add raster history

def raster_history(map):
    """Set the command history for a raster map to the command used to
    invoke the script (interface to `r.support').

    @return True on success
    @return False on failure
    """
    current_mapset = gisenv()['MAPSET']
    if find_file(name = map)['mapset'] == current_mapset:
        run_command('r.support', map = map, history = os.environ['CMDLINE'])
        return True
    
    warning("Unable to write history for <%s>. Raster map <%s> not found in current mapset." % (map, map))
    return False

# run "r.info -rgstmpud ..." and parse output

def raster_info(map):
    """Return information about a raster map (interface to `r.info')."""
    s = read_command('r.info', flags = 'rgstmpud', map = map)
    kv = parse_key_val(s)
    for k in ['min', 'max', 'north', 'south', 'east', 'west']:
	kv[k] = float(kv[k])
    for k in ['nsres', 'ewres']:
	kv[k] = float_or_dms(kv[k])
    return kv

# interface to r.mapcalc

def mapcalc(exp, **kwargs):
    t = string.Template(exp)
    e = t.substitute(**kwargs)
    if run_command('r.mapcalc', expression = e) != 0:
	fatal("An error occurred while running r.mapcalc")

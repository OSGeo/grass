"""!@package grass.script.raster

@brief GRASS Python scripting module (raster functions)

Raster related functions to be used in Python scripts.

Usage:

@code
from grass.script import raster as grass

grass.raster_history(map)
...
@endcode

(C) 2008-2011 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Glynn Clements
@author Martin Landa <landa.martin gmail.com>
"""

import os
import string
import types

from core import *

# add raster history

def raster_history(map):
    """!Set the command history for a raster map to the command used to
    invoke the script (interface to `r.support').

    @param map map name

    @return True on success
    @return False on failure
    """
    current_mapset = gisenv()['MAPSET']
    if find_file(name = map)['mapset'] == current_mapset:
        run_command('r.support', map = map, history = os.environ['CMDLINE'])
        return True
    
    warning(_("Unable to write history for <%(map)s>. "
              "Raster map <%(map)s> not found in current mapset." % { 'map' : map, 'map' : map}))
    return False

# run "r.info -gre ..." and parse output

def raster_info(map):
    """!Return information about a raster map (interface to
    `r.info'). Example:

    \code
    >>> grass.raster_info('elevation')
    {'north': 228500.0, 'timestamp': '"none"', 'min': 55.578792572021499,
    'datatype': 'FCELL', 'max': 156.32986450195301, 'ewres': 10.0,
    'vertical_datum': '', 'west': 630000.0, 'units': '',
    'title': 'South-West Wake county: Elevation NED 10m (elev_ned10m)',
    'east': 645000.0, 'nsres': 10.0, 'south': 215000.0}
    \endcode

    @param map map name
    
    @return parsed raster info
    """

    def float_or_null(s):
        if s == 'NULL':
            return None
        else:
            return float(s)

    s = read_command('r.info', flags = 'gre', map = map)
    kv = parse_key_val(s)
    for k in ['min', 'max']:
        kv[k] = float_or_null(kv[k])
    for k in ['north', 'south', 'east', 'west']:
        kv[k] = float(kv[k])
    for k in ['nsres', 'ewres']:
        kv[k] = float_or_dms(kv[k])
    return kv

# interface to r.mapcalc

def mapcalc(exp, quiet = False, verbose = False, overwrite = False, **kwargs):
    """!Interface to r.mapcalc.

    @param exp expression
    @param quiet True to run quietly (<tt>--q</tt>)
    @param verbose True to run verbosely (<tt>--v</tt>)
    @param overwrite True to enable overwriting the output (<tt>--o</tt>)
    @param kwargs
    """
    t = string.Template(exp)
    e = t.substitute(**kwargs)

    if run_command('r.mapcalc', expression = e,
                   quiet = quiet,
                   verbose = verbose,
                   overwrite = overwrite) != 0:
        fatal(_("An error occurred while running r.mapcalc"))


def mapcalc_start(exp, quiet = False, verbose = False, overwrite = False, **kwargs):
    """!Interface to r.mapcalc, doesn't wait for it to finish, returns Popen object.

    \code
    >>> expr1 = '"%s" = "%s" * 10' % (output, input)
    >>> expr2 = '...'   # etc.
    >>> # launch the jobs:
    >>> p1 = grass.mapcalc_start(expr1)
    >>> p2 = grass.mapcalc_start(expr2)   # etc.
    ...
    >>> # wait for them to finish:
    >>> p1.wait()
    >>> p2.wait()   # etc.
    \endcode

    @param exp expression
    @param quiet True to run quietly (<tt>--q</tt>)
    @param verbose True to run verbosely (<tt>--v</tt>)
    @param overwrite True to enable overwriting the output (<tt>--o</tt>)
    @param kwargs
    
    @return Popen object
    """
    t = string.Template(exp)
    e = t.substitute(**kwargs)

    return start_command('r.mapcalc', expression = e,
                        quiet = quiet,
                        verbose = verbose,
                        overwrite = overwrite)

# interface to r.what
def raster_what(map, coord):
    """!TODO"""
    if type(map) in (types.StringType, types.UnicodeType):
        map_list = [map]
    else:
        map_list = map

    coord_list = list()
    if type(coord) is types.TupleType:
        coord_list.append('%f,%f' % (coord[0], coord[1]))
    else:
        for e, n in coord:
            coord_list.append('%f,%f' % (e, n))
    
    sep = '|'
    # separator '|' not included in command
    # because | is causing problems on Windows
    # change separator?
    cmdParams = dict(quiet = True,
                     flags = 'rf',
                     map = ','.join(map_list),
                     coordinates = ','.join(coord_list),
                     null = _("No data"))
    
    ret = read_command('r.what',
                       **cmdParams)
    data = list()
    if not ret:
        return data

    labels = (_("value"), _("label"), _("color"))
    for item in ret.splitlines():
        line = item.split(sep)[3:]
        for i, map_name in enumerate(map_list):
            tmp_dict = {}
            tmp_dict[map_name] = {}
            for j in range(len(labels)):
                tmp_dict[map_name][labels[j]] = line[i*len(labels)+j]

            data.append(tmp_dict)

    return data

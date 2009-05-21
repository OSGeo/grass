"""
@package grass.script.vector

@brief GRASS Python scripting module

Vector related functions to be used in Python scripts.

Usage:

@code
from grass.script import core, vector as grass

grass.parser()
grass.vector_db(map)
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

from core import *

# run "v.db.connect -g ..." and parse output

def vector_db(map, **args):
    """Return the database connection details for a vector map
    (interface to `v.db.connect -g').

    @param map vector map

    @return dictionary { layer : { 'layer', 'table, 'database', 'driver', 'key' }
    """
    s = read_command('v.db.connect', flags = 'g', map = map, fs = ';', **args)
    result = {}
    
    for l in s.splitlines():
	f = l.split(';')
	if len(f) != 5:
	    continue
        
        if '/' in f[0]:
            f1 = f[0].split('/')
            layer = f1[0]
            name = f1[1]
        else:
            layer = f[0]
            name = ''
            
	result[int(layer)] = {
            'layer'    : layer,
            'name'     : name,
            'table'    : f[1],
            'key'      : f[2],
            'database' : f[3],
            'driver'   : f[4] }
    
    return result

def vector_layer_db(map, layer):
    """Return the database connection details for a vector map layer.
    If db connection for given layer is not defined, fatal() is called."""
    try:
        f = vector_db(map)[int(layer)]
    except KeyError:
	grass.fatal("Database connection not defined for layer %s" % layer)

    return f

# run "v.info -c ..." and parse output

def vector_columns(map, layer = None, **args):
    """Return a dictionary of the columns for the database table connected to
    a vector map (interface to `v.info -c').
    """
    s = read_command('v.info', flags = 'c', map = map, layer = layer, quiet = True, **args)
    result = {}
    for line in s.splitlines():
	f = line.split('|')
	if len(f) == 2:
            result[f[1]] = f[0]
    
    return result

# add vector history

def vector_history(map):
    """Set the command history for a vector map to the command used to
    invoke the script (interface to `v.support').
    """
    run_command('v.support', map = map, cmdhist = os.environ['CMDLINE'])

# run "v.info -t" and parse output

def vector_info_topo(map):
    """Return information about a vector map (interface to `v.info -t')."""
    s = read_command('v.info', flags = 't', map = map)
    return parse_key_val(s, val_type = int)

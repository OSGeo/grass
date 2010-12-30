"""!@package grass.script.vector

@brief GRASS Python scripting module (vector functions)

Vector related functions to be used in Python scripts.

Usage:

@code
from grass.script import vector as grass

grass.vector_db(map)
...
@endcode

(C) 2008-2010 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Glynn Clements
@author Martin Landa <landa.martin gmail.com>
"""

import os
import types
import __builtin__

from core import *

# i18N
import gettext
gettext.install('grasslibs', os.path.join(os.getenv("GISBASE"), 'locale'), unicode=True)

# run "v.db.connect -g ..." and parse output

def vector_db(map, **args):
    """!Return the database connection details for a vector map
    (interface to `v.db.connect -g'). Example:
    
    \code
    >>> grass.vector_db('lakes')
    {1: {'layer': '1', 'name': '',
    'database': '/home/martin/grassdata/nc_spm_08/PERMANENT/dbf/',
    'driver': 'dbf', 'key': 'cat', 'table': 'lakes'}}
    \endcode

    @param map vector map
    @param args

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
    """!Return the database connection details for a vector map layer.
    If db connection for given layer is not defined, fatal() is called.
    
    @param map map name
    @param layer layer number
    
    @return parsed output
    """
    try:
        f = vector_db(map)[int(layer)]
    except KeyError:
	fatal(_("Database connection not defined for layer %s") % layer)

    return f

# run "v.info -c ..." and parse output

def vector_columns(map, layer = None, getDict = True, **args):
    """!Return a dictionary (or a list) of the columns for the
    database table connected to a vector map (interface to `v.info
    -c').

    @code
    >>> vector_columns(urbanarea, getDict = True)
    {'UA_TYPE': {'index': 4, 'type': 'CHARACTER'}, 'UA': {'index': 2, 'type': 'CHARACTER'}, 'NAME': {'index': 3, 'type': 'CHARACTER'}, 'OBJECTID': {'index': 1, 'type': 'INTEGER'}, 'cat': {'index': 0, 'type': 'INTEGER'}}

    >>> vector_columns(urbanarea, getDict = False)
    ['cat', 'OBJECTID', 'UA', 'NAME', 'UA_TYPE']
    @endcode
    
    @param map map name
    @param layer layer number or name (None for all layers)
    @param getDict True to return dictionary of columns otherwise list of column names is returned
    @param args (v.info's arguments)
    
    @return dictionary/list of columns
    """
    s = read_command('v.info', flags = 'c', map = map, layer = layer, quiet = True, **args)
    if getDict:
        result = dict()
    else:
        result = list()
    i = 0
    for line in s.splitlines():
	ctype, cname = line.split('|')
        if getDict:
            result[cname] = { 'type' : ctype,
                              'index' : i }
        else:
            result.append(cname)
        i+=1
    
    return result

# add vector history

def vector_history(map):
    """!Set the command history for a vector map to the command used to
    invoke the script (interface to `v.support').

    @param map mapname

    @return v.support output
    """
    run_command('v.support', map = map, cmdhist = os.environ['CMDLINE'])

# run "v.info -t" and parse output

def vector_info_topo(map):
    """!Return information about a vector map (interface to `v.info
    -t'). Example:

    \code
    >>> grass.vector_info_topo('lakes')
    {'kernels': 0, 'lines': 0, 'centroids': 15279,
    'boundaries': 27764, 'points': 0, 'faces': 0,
    'primitives': 43043, 'islands': 7470, 'nodes': 35234, 'map3d': 0, 'areas': 15279}
    \endcode
    
    @param map map name

    @return parsed output
    """
    s = read_command('v.info', flags = 't', map = map)
    return parse_key_val(s, val_type = int)

# interface for v.db.select

def vector_db_select(map, layer = 1, **kwargs):
    """!Get attribute data of selected vector map layer.

    Function returns list of columns and dictionary of values ordered by
    key column value. Example:

    \code
    >>> print grass.vector_select('lakes')['values'][3]
    ['3', '19512.86146', '708.44683', '4', '55652', 'LAKE/POND', '39000', '']
    \endcode

    @param map map name
    @param layer layer number
    @param kwargs v.db.select options

    @return dictionary ('columns' and 'values')
    """
    try:
        key = vector_db(map = map)[layer]['key']
    except KeyError:
        error(_('Missing layer %(layer)d in vector map <%(map)s>') % \
                  { 'layer' : layer, 'map' : map })
        return { 'columns' : [], 'values' : {} }
        
    if kwargs.has_key('columns'):
        if key not in kwargs['columns'].split(','):
            # add key column if missing
            debug("Adding key column to the output")
            kwargs['columns'] += ',' + key
    
    ret = read_command('v.db.select',
                       map = map,
                       layer = layer,
                       fs = '|', **kwargs)
    
    if not ret:
        error(_('vector_select() failed'))
        return { 'columns' : [], 'values' : {} }
    
    columns = []
    values = {}
    for line in ret.splitlines():
        if not columns:
            columns = line.split('|')
            key_index = columns.index(key)
            continue
        
        value = line.split('|')
        key_value = int(value[key_index])
        values[key_value] = line.split('|')
    
    return { 'columns' : columns,
             'values' : values }

# interface to v.what
def vector_what(map, coord, distance = 0.0):
    """!Query vector map at given locations
    
    To query one vector map at one location
    @code
    print grass.vector_what(map = 'archsites', coord = (595743, 4925281), distance = 250)

    [{'Category': 8, 'Map': 'archsites', 'Layer': 1, 'Key_column': 'cat',
      'Database': '/home/martin/grassdata/spearfish60/PERMANENT/dbf/',
      'Mapset': 'PERMANENT', 'Driver': 'dbf',
      'Attributes': {'str1': 'No_Name', 'cat': '8'},
      'Table': 'archsites', 'Type': 'Point', 'Id': 8}]
    @endcode

    To query one vector map at more locations
    @code
    for q in grass.vector_what(map = ('archsites', 'roads'), coord = (595743, 4925281),
                               distance = 250):
        print q['Map'], q['Attributes']
                            
    archsites {'str1': 'No_Name', 'cat': '8'}
    roads {'label': 'interstate', 'cat': '1'}
    @endcode

    To query more vector maps at one location
    @code
    for q in grass.vector_what(map = 'archsites', coord = [(595743, 4925281), (597950, 4918898)],
                               distance = 250):
        print q['Map'], q['Attributes']

    archsites {'str1': 'No_Name', 'cat': '8'}
    archsites {'str1': 'Bob_Miller', 'cat': '22'}
    @endcode

    @param map vector map(s) to query given as string or list/tuple
    @param coord coordinates of query given as tuple (easting, northing) or list of tuples
    @param distance query threshold distance (in map units)

    @return parsed list
    """
    if os.environ.has_key("LC_ALL"):
        locale = os.environ["LC_ALL"]
        os.environ["LC_ALL"] = "C"

    if type(map) in (types.StringType, types.UnicodeType):
        map_list = [map]
    else:
        map_list = map
    
    layer_list = ['-1'] * len(map_list)
    
    coord_list = list()
    if type(coord) is types.TupleType:
        coord_list.append('%f,%f' % (coord[0], coord[1]))
    else:
        for e, n in coord:
            coord_list.append('%f,%f' % (e, n))
    
    ret = read_command('v.what',
                       quiet      = True,
                       flags      = 'ag',
                       map        = ','.join(map_list),
                       layer      = ','.join(layer_list),
                       east_north = ','.join(coord_list),
                       distance   = float(distance))
    
    if os.environ.has_key("LC_ALL"):
        os.environ["LC_ALL"] = locale
        
    data = list()
    if not ret:
        return data
    
    dict_attrb = None
    for item in ret.splitlines():
        try:
            key, value = __builtin__.map(lambda x: x.strip(), item.split('=', 1))
        except ValueError:
            continue
        if key in ('East', 'North'):
            continue
        
        if key == 'Map':
            dict_main  = { 'Map' : value }
            dict_attrb = None
            data.append(dict_main)
            continue
        else:
            if dict_attrb is not None:
                dict_attrb[key] = value
            else:
                if key in ('Category', 'Layer', 'Id'):
                    dict_main[key] = int(value)
                else:
                    dict_main[key] = value
            if key == 'Key_column':
                # skip attributes
                dict_attrb = dict()
                dict_main['Attributes'] = dict_attrb
    
    return data

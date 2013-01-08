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
import copy
import __builtin__

from core import *

# run "v.db.connect -g ..." and parse output

def vector_db(map, **args):
    """!Return the database connection details for a vector map
    (interface to `v.db.connect -g'). Example:
    
    \code
    >>> grass.vector_db('lakes')
    {1: {'layer': 1, 'name': '',
    'database': '/home/martin/grassdata/nc_spm_08/PERMANENT/dbf/',
    'driver': 'dbf', 'key': 'cat', 'table': 'lakes'}}
    \endcode
    
    @param map vector map
    @param args other v.db.connect's arguments
    
    @return dictionary
    """
    s = read_command('v.db.connect', quiet = True, flags = 'g', map = map, sep = ';', **args)
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
            'layer'    : int(layer),
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
    'primitives': 43043, 'islands': 7470, 'nodes': 35234, 'map3d': False, 'areas': 15279}
    \endcode
    
    @param map map name

    @return parsed output
    """
    s = read_command('v.info', flags = 't', map = map)
    ret = parse_key_val(s, val_type = int)
    if 'map3d' in ret:
        ret['map3d'] = bool(ret['map3d'])
    
    return ret


# run "v.info -get ..." and parse output

def vector_info(map):
    """!Return information about a vector map (interface to
    `v.info'). Example:

    \code
    >>> grass.vector_info('random_points')
    {'comment': '', 'projection': 'x,y', 'creator': 'soeren', 'holes': 0, 
     'primitives': 20, 'kernels': 0, 'scale': '1:1', 'title': '', 
     'west': 0.046125489999999998, 'top': 2376.133159, 'boundaries': 0, 
     'location': 'XYLocation', 'nodes': 0, 'east': 0.97305646000000001, 
     'source_date': 'Mon Aug 29 10:55:57 2011', 'north': 0.9589993, 
     'format': 'native', 'faces': 0, 'centroids': 0, 
     'digitization_threshold': '0.000000', 'islands': 0, 'level': 2, 
     'mapset': 'test', 'areas': 0, 'name': 'random_points', 
     'database': '/home/soeren/grassdata', 'bottom': 22.186596999999999, 
     'lines': 0, 'points': 20, 'map3d': True, 'volumes': 0, 'num_dblinks': 0, 
     'organization': '', 'south': 0.066047099999999997}
    
    \endcode
    @param map map name
    
    @return parsed vector info
    """

    s = read_command('v.info', flags = 'get', map = map)
    
    kv = parse_key_val(s)
    for k in ['north', 'south', 'east', 'west', 'top', 'bottom']:
	kv[k] = float(kv[k])
    for k in ['level', 'num_dblinks']:
	kv[k] = int(kv[k])
    for k in ['nodes', 'points', 'lines', 'boundaries', 'centroids', 'areas', 'islands', 'primitives']:
	kv[k] = int(kv[k])
    if 'map3d' in kv:
        kv['map3d'] = bool(int(kv['map3d']))
        if kv['map3d']:
            for k in ['faces', 'kernels', 'volumes', 'holes']:
                kv[k] = int(kv[k])

    return kv


# interface for v.db.select

def vector_db_select(map, layer = 1, **kwargs):
    """!Get attribute data of selected vector map layer.

    Function returns list of columns and dictionary of values ordered by
    key column value. Example:

    \code
    >>> print grass.vector_db_select('lakes')['columns']
    ['cat', 'AREA', 'PERIMETER', 'FULL_HYDRO', 'FULL_HYDR2', 'FTYPE', 'FCODE', 'NAME']
    >>> print grass.vector_db_select('lakes')['values'][3]
    ['3', '19512.86146', '708.44683', '4', '55652', 'LAKE/POND', '39000', '']
    >>> print grass.vector_db_select('lakes', columns = 'FTYPE')['values'][3]
    ['LAKE/POND']
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
        
    if 'columns' in kwargs:
        if key not in kwargs['columns'].split(','):
            # add key column if missing
            debug("Adding key column to the output")
            kwargs['columns'] += ',' + key
    
    ret = read_command('v.db.select',
                       map = map,
                       layer = layer,
                       **kwargs)
    
    if not ret:
        error(_('vector_db_select() failed'))
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
def vector_what(map, coord, distance = 0.0, ttype = None):
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

    To query one vector map with multiple layers (no additional parameters required)
    @code
    for q in grass.vector_what(map = 'some_map', coord = (596532.357143,4920486.21429), distance = 100.0):
        print q['Map'], q['Layer'], q['Attributes']

    new_bug_sites 1 {'str1': 'Beetle_site', 'GRASSRGB': '', 'cat': '80'}
    new_bug_sites 2 {'cat': '80'}
    @endcode

    To query more vector maps at one location
    @code
    for q in grass.vector_what(map = ('archsites', 'roads'), coord = (595743, 4925281),
                               distance = 250):
        print q['Map'], q['Attributes']
                            
    archsites {'str1': 'No_Name', 'cat': '8'}
    roads {'label': 'interstate', 'cat': '1'}
    @endcode

    To query one vector map at more locations
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
    @param ttype list of topology types (default of v.what are point, line, area, face)

    @return parsed list
    """
    if "LC_ALL" in os.environ:
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
    
    cmdParams = dict(quiet      = True,
                     flags      = 'ag',
                     map        = ','.join(map_list),
                     layer      = ','.join(layer_list),
                     coordinates = ','.join(coord_list),
                     distance   = float(distance))
    if ttype:
        cmdParams['type'] = ','.join(ttype)

    ret = read_command('v.what',
                       **cmdParams)
    
    if "LC_ALL" in os.environ:
        os.environ["LC_ALL"] = locale
        
    data = list()
    if not ret:
        return data
    
    dict_attrb = None
    dict_map = None
    dict_layer = None
    attr_pseudo_key = 'Attributes'
    for item in ret.splitlines():
        try:
            key, value = __builtin__.map(lambda x: x.strip(), item.split('=', 1))
        except ValueError:
            continue
        if key in ('East', 'North'):
            continue
        
        if key == 'Map':
            # attach the last one from the previous map
            if dict_layer is not None:
                dict_main = copy.copy(dict_map)
                dict_main.update(dict_layer)
                data.append(dict_main)
            dict_map  = { key : value }
            dict_layer = None
            dict_attrb = None
        elif key == 'Layer':
            # attach the last the previous Layer
            if dict_layer is not None:
                dict_main = copy.copy(dict_map)
                dict_main.update(dict_layer)
                data.append(dict_main)
            dict_layer = { key: int(value) }
            dict_attrb = None
        elif key == 'Key_column':
            dict_layer[key] = value
            dict_attrb = dict()
            dict_layer[attr_pseudo_key] = dict_attrb
        elif dict_attrb is not None:
            dict_attrb[key] = value
        elif dict_layer is not None:
            if key == 'Category':
                dict_layer[key] = int(value)
            else:
                dict_layer[key] = value
        else:
            dict_map[key] = value
            # TODO: there are some keys which has non-string values
            # examples: Sq_Meters, Hectares, Acres, Sq_Miles
    
    # attach the last one
    if dict_layer is not None:
        dict_main = copy.copy(dict_map)
        dict_main.update(dict_layer)
        data.append(dict_main)
    
    return data

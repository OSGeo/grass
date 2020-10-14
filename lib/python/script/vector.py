"""
Vector related functions to be used in Python scripts.

Usage:

::

    from grass.script import vector as grass
    grass.vector_db(map)

(C) 2008-2010 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

.. sectionauthor:: Glynn Clements
.. sectionauthor:: Martin Landa <landa.martin gmail.com>
"""
from __future__ import absolute_import
import os
import types

try:
    import __builtin__
    bytes = str
except ImportError:
    # python3
    import builtins as __builtin__
    unicode = str

from .utils import parse_key_val
from .core import *
from grass.exceptions import CalledModuleError


def vector_db(map, env=None, **kwargs):
    """Return the database connection details for a vector map
    (interface to `v.db.connect -g`). Example:

    >>> vector_db('geology') # doctest: +ELLIPSIS, +NORMALIZE_WHITESPACE
    {1: {'layer': 1, ... 'table': 'geology'}}

    :param str map: vector map
    :param kwargs: other v.db.connect's arguments
    :param env: environment

    :return: dictionary
    """
    s = read_command('v.db.connect', quiet=True, flags='g', map=map, sep=';',
                     env=env, **kwargs)
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
            'layer': int(layer),
            'name': name,
            'table': f[1],
            'key': f[2],
            'database': f[3],
            'driver': f[4] }

    return result


def vector_layer_db(map, layer, env=None):
    """Return the database connection details for a vector map layer.
    If db connection for given layer is not defined, fatal() is called.

    :param str map: map name
    :param layer: layer number
    :param env: environment

    :return: parsed output
    """
    try:
        f = vector_db(map, env=env)[int(layer)]
    except KeyError:
        fatal(_("Database connection not defined for layer %s") % layer)

    return f

# run "v.info -c ..." and parse output


def vector_columns(map, layer=None, getDict=True, env=None, **kwargs):
    """Return a dictionary (or a list) of the columns for the
    database table connected to a vector map (interface to `v.info -c`).

    >>> vector_columns('geology', getDict=True) # doctest: +NORMALIZE_WHITESPACE
    {'PERIMETER': {'index': 2, 'type': 'DOUBLE PRECISION'}, 'GEOL250_':
    {'index': 3, 'type': 'INTEGER'}, 'SHAPE_area': {'index': 6, 'type':
    'DOUBLE PRECISION'}, 'onemap_pro': {'index': 1, 'type': 'DOUBLE
    PRECISION'}, 'SHAPE_len': {'index': 7, 'type': 'DOUBLE PRECISION'},
    'cat': {'index': 0, 'type': 'INTEGER'}, 'GEOL250_ID': {'index': 4, 'type':
    'INTEGER'}, 'GEO_NAME': {'index': 5, 'type': 'CHARACTER'}}

    >>> vector_columns('geology', getDict=False) # doctest: +NORMALIZE_WHITESPACE
    ['cat',
     'onemap_pro',
     'PERIMETER',
     'GEOL250_',
     'GEOL250_ID',
     'GEO_NAME',
     'SHAPE_area',
     'SHAPE_len']

    :param str map: map name
    :param layer: layer number or name (None for all layers)
    :param bool getDict: True to return dictionary of columns otherwise list
                         of column names is returned
    :param kwargs: (v.info's arguments)
    :param env: environment

    :return: dictionary/list of columns
    """
    s = read_command('v.info', flags='c', map=map, layer=layer, quiet=True,
                     env=env, **kwargs)
    if getDict:
        result = dict()
    else:
        result = list()
    i = 0
    for line in s.splitlines():
        ctype, cname = line.split('|')
        if getDict:
            result[cname] = {'type': ctype, 'index': i}
        else:
            result.append(cname)
        i += 1

    return result


def vector_history(map, replace=False, env=None):
    """Set the command history for a vector map to the command used to
    invoke the script (interface to `v.support`).

    :param str map: mapname
    :param bool replace: Replace command line instead of appending it
    :param env: environment

    :return: v.support output
    """
    run_command('v.support', map=map, cmdhist=os.environ['CMDLINE'],
                flags='h' if replace else None, env=env)


def vector_info_topo(map, layer=1, env=None):
    """Return information about a vector map (interface to `v.info -t`).
    Example:

    >>> vector_info_topo('geology') # doctest: +NORMALIZE_WHITESPACE
    {'lines': 0, 'centroids': 1832, 'boundaries': 3649, 'points': 0,
    'primitives': 5481, 'islands': 907, 'nodes': 2724, 'map3d': False,
    'areas': 1832}

    :param str map: map name
    :param int layer: layer number
    :param env: environment

    :return: parsed output
    """
    s = read_command('v.info', flags='t', layer=layer, map=map,
                     env=env)
    ret = parse_key_val(s, val_type=int)
    if 'map3d' in ret:
        ret['map3d'] = bool(ret['map3d'])

    return ret


def vector_info(map, layer=1, env=None):
    """Return information about a vector map (interface to
    `v.info`). Example:

    >>> vector_info('geology') # doctest: +ELLIPSIS, +NORMALIZE_WHITESPACE
    {'comment': '', 'projection': 'Lambert Conformal Conic' ... 'south': 10875.8272320917}

    :param str map: map name
    :param int layer: layer number
    :param env: environment

    :return: parsed vector info
    """

    s = read_command('v.info', flags='get', layer=layer, map=map,
                     env=env)

    kv = parse_key_val(s)
    for k in ['north', 'south', 'east', 'west', 'top', 'bottom']:
        kv[k] = float(kv[k])
    for k in ['level', 'num_dblinks']:
        kv[k] = int(kv[k])
    for k in ['nodes', 'points', 'lines', 'boundaries', 'centroids', 'areas',
              'islands', 'primitives']:
        kv[k] = int(kv[k])
    if 'map3d' in kv:
        kv['map3d'] = bool(int(kv['map3d']))
        if kv['map3d']:
            for k in ['faces', 'kernels', 'volumes', 'holes']:
                kv[k] = int(kv[k])

    return kv


def vector_db_select(map, layer=1, env=None, **kwargs):
    """Get attribute data of selected vector map layer.

    Function returns list of columns and dictionary of values ordered by
    key column value. Example:

    >>> print vector_db_select('geology')['columns']
    ['cat', 'onemap_pro', 'PERIMETER', 'GEOL250_', 'GEOL250_ID', 'GEO_NAME', 'SHAPE_area', 'SHAPE_len']
    >>> print vector_db_select('geology')['values'][3]
    ['3', '579286.875', '3335.55835', '4', '3', 'Zml', '579286.829631', '3335.557182']
    >>> print vector_db_select('geology', columns = 'GEO_NAME')['values'][3]
    ['Zml']

    :param str map: map name
    :param int layer: layer number
    :param kwargs: v.db.select options
    :param env: environment

    :return: dictionary ('columns' and 'values')
    """
    try:
        key = vector_db(map=map, env=env)[layer]['key']
    except KeyError:
        error(_('Missing layer %(layer)d in vector map <%(map)s>') %
              {'layer': layer, 'map': map})
        return {'columns': [], 'values': {}}

    include_key = True
    if 'columns' in kwargs:
        if key not in kwargs['columns'].split(','):
            # add key column if missing
            include_key = False
            debug("Adding key column to the output")
            kwargs['columns'] += ',' + key

    ret = read_command('v.db.select', map=map, layer=layer,
                       env=env, **kwargs)

    if not ret:
        error(_('vector_db_select() failed'))
        return {'columns': [], 'values': {}}

    columns = []
    values = {}
    for line in ret.splitlines():
        if not columns:
            columns = line.split('|')
            key_index = columns.index(key)
            # discard key column
            if not include_key:
                columns = columns[:-1]
            continue

        value = line.split('|')
        key_value = int(value[key_index])
        if not include_key:
            # discard key column
            values[key_value] = value[:-1]
        else:
            values[key_value] = value

    return {'columns': columns, 'values': values}


json = None
orderedDict = None


def vector_what(map, coord, distance=0.0, ttype=None,
                encoding=None, skip_attributes=False,
                layer=None, multiple=False, env=None):
    """Query vector map at given locations

    To query one vector map at one location

    ::

        print grass.vector_what(map='archsites', coord=(595743, 4925281),
                                distance=250)

        [{'Category': 8, 'Map': 'archsites', 'Layer': 1, 'Key_column': 'cat',
          'Database': '/home/martin/grassdata/spearfish60/PERMANENT/dbf/',
          'Mapset': 'PERMANENT', 'Driver': 'dbf',
          'Attributes': {'str1': 'No_Name', 'cat': '8'},
          'Table': 'archsites', 'Type': 'Point', 'Id': 8}]

    To query one vector map with multiple layers (no additional parameters
    required)

    ::

        for q in grass.vector_what(map='some_map', distance=100.0,
                                   coord=(596532.357143,4920486.21429)):
            print q['Map'], q['Layer'], q['Attributes']

        new_bug_sites 1 {'str1': 'Beetle_site', 'GRASSRGB': '', 'cat': '80'}
        new_bug_sites 2 {'cat': '80'}

    To query more vector maps at one location

    ::

        for q in grass.vector_what(map=('archsites', 'roads'),
                                   coord=(595743, 4925281), distance=250):
            print q['Map'], q['Attributes']

        archsites {'str1': 'No_Name', 'cat': '8'}
        roads {'label': 'interstate', 'cat': '1'}

    To query one vector map at more locations

    ::

        for q in grass.vector_what(map='archsites', distance=250,
                                   coord=[(595743, 4925281), (597950, 4918898)]):
            print q['Map'], q['Attributes']

        archsites {'str1': 'No_Name', 'cat': '8'}
        archsites {'str1': 'Bob_Miller', 'cat': '22'}

    :param map: vector map(s) to query given as string or list/tuple
    :param coord: coordinates of query given as tuple (easting, northing) or
                  list of tuples
    :param distance: query threshold distance (in map units)
    :param ttype: list of topology types (default of v.what are point, line,
                  area, face)
    :param encoding: attributes encoding
    :param skip_attributes: True to skip quering attributes
    :param layer: layer number or list of layers (one for each vector),
                  if None, all layers (-1) are used
    :param multiple: find multiple features within threshold distance
    :param env: environment

    :return: parsed list
    """
    if not env:
        env = os.environ.copy()
    if "LC_ALL" in env:
        env["LC_ALL"] = "C"

    if isinstance(map, (bytes, unicode)):
        map_list = [map]
    else:
        map_list = map

    if layer:
        if isinstance(layer, (tuple, list)):
            layer_list = [str(l) for l in layer]
        else:
            layer_list = [str(layer)]
        if len(layer_list) != len(map_list):
            raise ScriptError(_("Number of given vector maps ({m}) "
                                "differs from number of layers ({l})").format(m=len(map_list),
                                                                              l=len(layer_list)))
    else:
        layer_list = ['-1'] * len(map_list)

    coord_list = list()
    if isinstance(coord, tuple):
        coord_list.append('%f,%f' % (coord[0], coord[1]))
    else:
        for e, n in coord:
            coord_list.append('%f,%f' % (e, n))

    flags = 'j'
    if not skip_attributes:
        flags += 'a'
    if multiple:
        flags += 'm'
    cmdParams = dict(quiet=True,
                     flags=flags,
                     map=','.join(map_list),
                     layer=','.join(layer_list),
                     coordinates=','.join(coord_list),
                     distance=float(distance))
    if ttype:
        cmdParams['type'] = ','.join(ttype)

    try:
        ret = read_command('v.what', env=env,
                           **cmdParams).strip()
    except CalledModuleError as e:
        raise ScriptError(e.msg)

    data = list()
    if not ret:
        return data

    # lazy import
    global json
    global orderedDict
    if json is None:
        import json
    if orderedDict is None:
        try:
            from collections import OrderedDict
            orderedDict = OrderedDict
        except ImportError:
            orderedDict = dict

    kwargs = {}
    if encoding:
        kwargs['encoding'] = encoding

    if sys.version_info[0:2] > (2, 6):
        kwargs['object_pairs_hook'] = orderedDict

    try:
        result = json.loads(ret, **kwargs)
    except ValueError:
        raise ScriptError(_("v.what output is not valid JSON format:\n {ret}").format(ret=ret))

    if multiple:
        for vmap in result['Maps']:
            features = vmap.pop('Features', None)
            if features:
                for feature in features:
                    cats = feature.pop('Categories', None)
                    if cats:
                        for cat in cats:
                            tmp = feature.copy()
                            tmp.update(cat)
                            tmp2 = vmap.copy()
                            tmp2.update(tmp)
                            data.append(tmp2)
    else:
        for vmap in result['Maps']:
            cats = vmap.pop('Categories', None)
            if cats:
                for cat in cats:
                    tmp = vmap.copy()
                    tmp.update(cat)
                    data.append(tmp)
            else:
                data.append(vmap)
    return data

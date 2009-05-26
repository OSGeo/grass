"""
@package grass.script.db

@brief GRASS Python scripting module

Database related functions to be used in Python scripts.

Usage:

@code
from grass.script import core, db as grass

grass.parser()
grass.db_describe(table)
...
@endcode

(C) 2008-2009 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Glynn Clements
@author Martin Landa <landa.martin gmail.com>
"""

from core import *

def db_describe(table, **args):
    """Return the list of columns for a database table
    (interface to `db.describe -c'). Example:

    \code
    >>> grass.db_describe('lakes')
    {'nrows': 15279, 'cols': [['cat', 'INTEGER', '11'], ['AREA', 'DOUBLE PRECISION', '20'],
    ['PERIMETER', 'DOUBLE PRECISION', '20'], ['FULL_HYDRO', 'DOUBLE PRECISION', '20'],
    ['FULL_HYDR2', 'DOUBLE PRECISION', '20'], ['FTYPE', 'CHARACTER', '24'],
    ['FCODE', 'INTEGER', '11'], ['NAME', 'CHARACTER', '99']], 'ncols': 8}
    \endcode

    @param table table name
    @param args

    @return parsed module output
    """
    s = read_command('db.describe', flags = 'c', table = table, **args)
    if not s:
	return None
    cols = []
    result = {}
    for l in s.splitlines():
	f = l.split(':')
	key = f[0]
	f[1] = f[1].lstrip(' ')
	if key.startswith('Column '):
	    n = int(key.split(' ')[1])
	    cols.insert(n, f[1:])
	elif key in ['ncols', 'nrows']:
	    result[key] = int(f[1])
	else:
	    result[key] = f[1:]
    result['cols'] = cols
    return result

# run "db.connect -p" and parse output

def db_connection():
    """Return the current database connection parameters
    (interface to `db.connect -p'). Example:

    \code
    >>> grass.db_connection()
    {'group': 'x', 'schema': '', 'driver': 'dbf', 'database': '$GISDBASE/$LOCATION_NAME/$MAPSET/dbf/'}
    \endcode

    @return parsed output of db.connect
    """
    s = read_command('db.connect', flags = 'p')
    return parse_key_val(s, sep = ':')

"""!@package grass.script.db

@brief GRASS Python scripting module (database functions)

Database related functions to be used in Python scripts.

Usage:

@code
from grass.script import db as grass

grass.db_describe(table)
...
@endcode

(C) 2008-2009, 2012 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Glynn Clements
@author Martin Landa <landa.martin gmail.com>
"""

import tempfile as pytempfile # conflict with core.tempfile

from core import *

def db_describe(table, **args):
    """!Return the list of columns for a database table
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
        fatal(_("Unable to describe table <%s>") % table)
    
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

# run "db.connect -g" and parse output

def db_table_exist(table, **args):
    """!Check if table exists.

    If no driver or database are given, then default settings is used
    (check db_connection()).

    @param table table name
    @param driver DB driver 
    @param database DB to check

    @return True for success, False otherwise
    """
    nuldev = file(os.devnull, 'w+')
    ret = run_command('db.describe', flags = 'c', table = table,
                      stdout = nuldev, stderr = nuldev, **args)
    nuldev.close()
    
    if ret == 0:
        return True
    return False

def db_connection():
    """!Return the current database connection parameters
    (interface to `db.connect -g'). Example:

    \code
    >>> grass.db_connection()
    {'group': 'x', 'schema': '', 'driver': 'dbf', 'database': '$GISDBASE/$LOCATION_NAME/$MAPSET/dbf/'}
    \endcode

    @return parsed output of db.connect
    """
    return parse_command('db.connect', flags = 'g')

def db_select(sql = None, filename = None, table = None, **args):
    """!Perform SQL select statement

    Note: one of <em>sql</em>, <em>filename</em>, or <em>table</em>
    arguments must be provided.
    
    Examples:
    
    \code
    grass.db_select(sql = 'SELECT cat,CAMPUS FROM busstopsall WHERE cat < 4')

    (('1', 'Vet School'), ('2', 'West'), ('3', 'North'))
    \endcode
    
    \code
     grass.db_select(filename = '/path/to/sql/file')
    \endcode

    Simplyfied usage 
    
    \code
    grass.db_select(table = 'busstopsall')
    \endcode

    performs <tt>SELECT * FROM busstopsall</tt>.

    @param sql SQL statement to perform (or None)
    @param filename name of file with SQL statements (or None)
    @param table name of table to query (or None)
    @param args  see \gmod{db.select} arguments
    """
    fname = tempfile(create = False)
    if sql:
        args['sql'] = sql
    elif filename:
        args['input'] = filename
    elif table:
        args['table'] = table
    else:
        fatal(_("Programmer error: '%(sql)s', '%(filename)s', or '%(table)s' must be provided") %
              {'sql': 'sql', 'filename': 'filename', 'table': 'table'} )
    
    if 'sep' not in args:
        args['sep'] = '|'
    
    ret = run_command('db.select', quiet = True,
                      flags = 'c',
                      output = fname,
                      **args)
    
    if ret != 0:
        fatal(_("Fetching data failed"))
    
    ofile = open(fname)
    result = map(lambda x: tuple(x.rstrip(os.linesep).split(args['sep'])),
                 ofile.readlines())
    ofile.close()
    try_remove(fname)
        
    return tuple(result)


def db_table_in_vector(table):
    """Return the name of vector connected to the table.
       It returns False if no vectors are connected to the table.

       Example

       @params table name of table to query

    """
    from vector import vector_db
    nuldev = file(os.devnull, 'w')
    used = []
    vects = list_strings('vect')
    for vect in vects:
        for f in vector_db(vect, stderr=nuldev).itervalues():
            if not f:
                continue
            if f['table'] == table:
                used.append(vect)
                break
    return used

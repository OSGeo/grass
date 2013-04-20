# -*- coding: utf-8 -*-
"""
Created on Wed Aug  8 15:29:21 2012

@author: pietro



"""
import ctypes
import numpy as np

try:
    from collections import OrderedDict
except:
    from grass.pygrass.orderdict import OrderedDict

import grass.lib.vector as libvect
from grass.pygrass.gis import Mapset
from grass.pygrass.errors import DBError
from grass.script.db import db_table_in_vector
from grass.script.core import warning
import sys
import sql


DRIVERS = ('sqlite', 'pg')


def get_path(path):
    """Return the full path to the database; replacing environment variable
    with real values ::

    >>> path = '$GISDBASE/$LOCATION_NAME/$MAPSET/sqlite/sqlite.db'
    >>> new_path = get_path(path)
    >>> from grass.script.core import gisenv
    >>> import os
    >>> new_path2 = os.path.join(gisenv()['GISDBASE'], gisenv()['LOCATION_NAME'],
    ...                          gisenv()['MAPSET'], 'sqlite', 'sqlite.db')
    >>> new_path == new_path2
    True
    """
    if "$" not in path:
        return path
    else:
        mapset = Mapset()
        path = path.replace('$GISDBASE', mapset.gisdbase)
        path = path.replace('$LOCATION_NAME', mapset.location)
        path = path.replace('$MAPSET', mapset.name)
        return path


class Filters(object):
    """Help user to build a simple sql query. ::

        >>> filter = Filters('table')
        >>> filter.get_sql()
        'SELECT * FROM table;'
        >>> filter.where("area<10000").get_sql()
        'SELECT * FROM table WHERE area<10000;'
        >>> filter.select("cat", "area").get_sql()
        'SELECT cat, area FROM table WHERE area<10000;'
        >>> filter.order_by("area").limit(10).get_sql()
        'SELECT cat, area FROM table WHERE area<10000 ORDER BY area LIMIT 10;'

    ..
    """
    def __init__(self, tname):
        self.tname = tname
        self._select = None
        self._where = None
        self._orderby = None
        self._limit = None

    def __repr__(self):
        return "Filters(%r)" % self.get_sql()

    def select(self, *args):
        """Create the select query"""
        cols = ', '.join(args) if args else '*'
        select = sql.SELECT[:-1]
        self._select = select.format(cols=cols, tname=self.tname)
        return self

    def where(self, condition):
        """Create the where condition"""
        self._where = 'WHERE {condition}'.format(condition=condition)
        return self

    def order_by(self, orderby):
        """Create the order by condition"""
        if not isinstance(orderby, str):
            orderby = ', '.join(orderby)
        self._orderby = 'ORDER BY {orderby}'.format(orderby=orderby)
        return self

    def limit(self, number):
        """Create the limit condition"""
        if not isinstance(number, int):
            raise ValueError("Must be an integer.")
        else:
            self._limit = 'LIMIT {number}'.format(number=number)
        return self

    def get_sql(self):
        """Return the SQL query"""
        sql_list = list()
        if self._select is None:
            self.select()
        sql_list.append(self._select)

        if self._where is not None:
            sql_list.append(self._where)
        if self._orderby is not None:
            sql_list.append(self._orderby)
        if self._limit is not None:
            sql_list.append(self._limit)
        return "%s;" % ' '.join(sql_list)

    def reset(self):
        """Clean internal variables"""
        self._select = None
        self._where = None
        self._orderby = None
        self._limit = None


class Columns(object):
    """Object to work with columns table.

    It is possible to instantiate a Columns object given the table name and
    the database connection.

    For a sqlite table: ::

        >>> import sqlite3
        >>> path = '$GISDBASE/$LOCATION_NAME/PERMANENT/sqlite/sqlite.db'
        >>> cols_sqlite = Columns('census',
        ...                       sqlite3.connect(get_path(path)))
        >>> cols_sqlite.tname
        'census'

    For a postgreSQL table: ::

        >>> import psycopg2 as pg                              #doctest: +SKIP
        >>> cols_pg = Columns('boundary_municp_pg',
        ...                   pg.connect('host=localhost dbname=grassdb')) #doctest: +SKIP
        >>> cols_pg.tname #doctest: +SKIP
        'boundary_municp_pg'                                   #doctest: +SKIP

    ..
    """
    def __init__(self, tname, connection, key='cat'):
        self.tname = tname
        self.conn = connection
        self.key = key
        self.odict = None
        self.update_odict()

    def __contains__(self, item):
        return item in self.names()

    def __repr__(self):
        return "Columns(%r)" % self.items()

    def __getitem__(self, key):
        return self.odict[key]

    def __setitem__(self, name, new_type):
        self.cast(name, new_type)
        self.update_odict(self)

    def __iter__(self):
        return self.odict.__iter__()

    def __len__(self):
        return self.odict.__len__()

    def __eq__(self, obj):
        return obj.tname == self.tname and obj.odict == self.odict

    def is_pg(self):
        """Return True if is a psycopg connection. ::

            >>> import sqlite3
            >>> path = '$GISDBASE/$LOCATION_NAME/PERMANENT/sqlite/sqlite.db'
            >>> cols_sqlite = Columns('census',
            ...                       sqlite3.connect(get_path(path)))
            >>> cols_sqlite.is_pg()
            False
            >>> import psycopg2 as pg #doctest: +SKIP
            >>> cols_pg = Columns('boundary_municp_pg',
            ...                   pg.connect('host=localhost dbname=grassdb')) #doctest: +SKIP
            >>> cols_pg.is_pg() #doctest: +SKIP
            True

        ..
        """
        return hasattr(self.conn, 'xid')

    def update_odict(self):
        """Read columns name and types from table and update the odict
        attribute.
        """
        if self.is_pg():
            # is a postgres connection
            cur = self.conn.cursor()
            cur.execute("SELECT oid,typname FROM pg_type")
            diz = dict(cur.fetchall())
            odict = OrderedDict()
            import psycopg2 as pg
            try:
                cur.execute(sql.SELECT.format(cols='*', tname=self.tname))
                descr = cur.description
                for column in descr:
                    name, ctype = column[:2]
                    odict[name] = diz[ctype]
            except pg.ProgrammingError:
                pass
            self.odict = odict
        else:
            # is a sqlite connection
            cur = self.conn.cursor()
            cur.execute(sql.PRAGMA.format(tname=self.tname))
            descr = cur.fetchall()
            odict = OrderedDict()
            for column in descr:
                name, ctype = column[1:3]
                odict[name] = ctype
            self.odict = odict
        values = ','.join(['?', ] * self.__len__())
        kv = ','.join(['%s=?' % k for k in self.odict.keys()])
        where = "%s=?" % self.key
        self.insert_str = sql.INSERT.format(tname=self.tname, values=values)
        self.update_str = sql.UPDATE_WHERE.format(tname=self.tname,
                                                  values=kv, condition=where)

    def sql_descr(self, remove=None):
        """Return a string with description of columns.
           Remove it is used to remove a columns.::

            >>> import sqlite3
            >>> path = '$GISDBASE/$LOCATION_NAME/PERMANENT/sqlite/sqlite.db'
            >>> cols_sqlite = Columns('census',
            ...                       sqlite3.connect(get_path(path)))
            >>> cols_sqlite.sql_descr()                   # doctest: +ELLIPSIS
            u'cat INTEGER, OBJECTID INTEGER, AREA DOUBLE PRECISION, ...'
            >>> import psycopg2 as pg                         # doctest: +SKIP
            >>> cols_pg = Columns('boundary_municp_pg',
            ...                   pg.connect('host=localhost dbname=grassdb')) # doctest: +SKIP
            >>> cols_pg.sql_descr()                 # doctest: +ELLIPSIS +SKIP
            'cat int4, objectid int4, area float8, perimeter float8, ...'
        """
        if remove:
            return ', '.join(['%s %s' % (key, val) for key, val in self.items()
                             if key != remove])
        else:
            return ', '.join(['%s %s' % (key, val)
                              for key, val in self.items()])

    def types(self):
        """Return a list with the column types. ::

            >>> import sqlite3
            >>> path = '$GISDBASE/$LOCATION_NAME/PERMANENT/sqlite/sqlite.db'
            >>> cols_sqlite = Columns('census',
            ...                       sqlite3.connect(get_path(path)))
            >>> cols_sqlite.types()                       # doctest: +ELLIPSIS
            [u'INTEGER', u'INTEGER', ...]
            >>> import psycopg2 as pg                         # doctest: +SKIP
            >>> cols_pg = Columns('boundary_municp_pg',
            ...                   pg.connect('host=localhost dbname=grassdb')) # doctest: +SKIP
            >>> cols_pg.types()                     # doctest: +ELLIPSIS +SKIP
            ['int4', 'int4', 'float8', 'float8', 'float8', ...]


        ..
        """
        return self.odict.values()

    def names(self, remove=None, unicod=True):
        """Return a list with the column names.
           Remove it is used to remove a columns.::

            >>> import sqlite3
            >>> path = '$GISDBASE/$LOCATION_NAME/PERMANENT/sqlite/sqlite.db'
            >>> cols_sqlite = Columns('census',
            ...                       sqlite3.connect(get_path(path)))
            >>> cols_sqlite.names()                      # doctest: +ELLIPSIS
            [u'cat', u'OBJECTID', u'AREA', u'PERIMETER', ...]
            >>> import psycopg2 as pg                         # doctest: +SKIP
            >>> cols_pg = Columns('boundary_municp_pg',       # doctest: +SKIP
            ...                   pg.connect('host=localhost dbname=grassdb'))
            >>> cols_pg.names()                     # doctest: +ELLIPSIS +SKIP
            ['cat', 'objectid', 'area', 'perimeter', ...]


        ..
        """
        if remove:
            nams = self.odict.keys()
            nams.remove(remove)
        else:
            nams = self.odict.keys()
        if unicod:
            return nams
        else:
            return [str(name) for name in nams]

    def items(self):
        """Return a list of tuple with column name and column type.  ::

            >>> import sqlite3
            >>> path = '$GISDBASE/$LOCATION_NAME/PERMANENT/sqlite/sqlite.db'
            >>> cols_sqlite = Columns('census',
            ...                       sqlite3.connect(get_path(path)))
            >>> cols_sqlite.items()                       # doctest: +ELLIPSIS
            [(u'cat', u'INTEGER'), ...]
            >>> import psycopg2 as pg                         # doctest: +SKIP
            >>> cols_pg = Columns('boundary_municp_pg',
            ...                   pg.connect('host=localhost dbname=grassdb')) # doctest: +SKIP
            >>> cols_pg.items()                     # doctest: +ELLIPSIS +SKIP
            [('cat', 'int4'), ('objectid', 'int4'), ('area', 'float8'), ...]

        ..
        """
        return self.odict.items()

    def add(self, col_name, col_type):
        """Add a new column to the table. ::

            >>> import sqlite3
            >>> path = '$GISDBASE/$LOCATION_NAME/$MAPSET/sqlite/sqlite.db'
            >>> from grass.pygrass.functions import copy, remove
            >>> copy('census','mycensus','vect')
            >>> cols_sqlite = Columns('mycensus',
            ...                       sqlite3.connect(get_path(path)))
            >>> cols_sqlite.add('n_pizza', 'INT')
            >>> 'n_pizza' in cols_sqlite
            True
            >>> import psycopg2 as pg                         # doctest: +SKIP
            >>> cols_pg = Columns('boundary_municp_pg',
            ...                   pg.connect('host=localhost dbname=grassdb'))  #doctest: +SKIP
            >>> cols_pg.add('n_pizza', 'INT')                 # doctest: +SKIP
            >>> 'n_pizza' in cols_pg                          # doctest: +SKIP
            True
            >>> remove('mycensus', 'vect')

        ..
        """
        valid_type = ('DOUBLE PRECISION', 'INT', 'DATE')
        if 'VARCHAR' in col_type or col_type.upper() not in valid_type:
            str_err = "Type is not supported, supported types are: %s"
            raise TypeError(str_err % ", ".join(valid_type))
        cur = self.conn.cursor()
        cur.execute(sql.ADD_COL.format(tname=self.tname,
                                       cname=col_name,
                                       ctype=col_type))
        self.conn.commit()
        cur.close()
        self.update_odict()

    def rename(self, old_name, new_name):
        """Rename a column of the table. ::

            >>> import sqlite3
            >>> path = '$GISDBASE/$LOCATION_NAME/$MAPSET/sqlite/sqlite.db'
            >>> from grass.pygrass.functions import copy, remove
            >>> copy('census','mycensus','vect')
            >>> cols_sqlite = Columns('mycensus',
            ...                       sqlite3.connect(get_path(path)))
            >>> cols_sqlite.add('n_pizza', 'INT')
            >>> 'n_pizza' in cols_sqlite
            True
            >>> cols_sqlite.rename('n_pizza', 'n_pizzas')  # doctest: +ELLIPSIS
            >>> 'n_pizza' in cols_sqlite
            False
            >>> 'n_pizzas' in cols_sqlite
            True

            >>> import psycopg2 as pg                         # doctest: +SKIP
            >>> cols_pg = Columns('boundary_municp_pg',
            ...                   pg.connect('host=localhost dbname=grassdb')) # doctest: +SKIP
            >>> cols_pg.rename('n_pizza', 'n_pizzas')         # doctest: +SKIP
            >>> 'n_pizza' in cols_pg                          # doctest: +SKIP
            False
            >>> 'n_pizzas' in cols_pg                         # doctest: +SKIP
            True
            >>> remove('mycensus', 'vect')

        ..
        """
        cur = self.conn.cursor()
        if self.is_pg():
            cur.execute(sql.RENAME_COL.format(tname=self.tname,
                                              old_name=old_name,
                                              new_name=new_name))
            self.conn.commit()
            cur.close()
            self.update_odict()
        else:
            cur.execute(sql.ADD_COL.format(tname=self.tname,
                                           cname=new_name,
                                           ctype=str(self.odict[old_name])))
            cur.execute(sql.UPDATE.format(tname=self.tname,
                                          new_col=new_name,
                                          old_col=old_name))
            self.conn.commit()
            cur.close()
            self.update_odict()
            self.drop(old_name)

    def cast(self, col_name, new_type):
        """Change the column type. ::

            >>> import sqlite3
            >>> path = '$GISDBASE/$LOCATION_NAME/$MAPSET/sqlite/sqlite.db'
            >>> from grass.pygrass.functions import copy, remove
            >>> copy('census','mycensus','vect')
            >>> cols_sqlite = Columns('mycensus',
            ...                       sqlite3.connect(get_path(path)))
            >>> cols_sqlite.add('n_pizzas', 'INT')
            >>> cols_sqlite.cast('n_pizzas', 'float8')  # doctest: +ELLIPSIS
            Traceback (most recent call last):
              ...
            DBError: 'SQLite does not support to cast columns.'
            >>> import psycopg2 as pg                         # doctest: +SKIP
            >>> cols_pg = Columns('boundary_municp_pg',
            ...                   pg.connect('host=localhost dbname=grassdb')) # doctest: +SKIP
            >>> cols_pg.cast('n_pizzas', 'float8')            # doctest: +SKIP
            >>> cols_pg['n_pizzas']                           # doctest: +SKIP
            'float8'
            >>> remove('mycensus', 'vect')

            .. warning ::

               It is not possible to cast a column with sqlite
            ..
        """
        if self.is_pg():
            cur = self.conn.cursor()
            cur.execute(sql.CAST_COL.format(tname=self.tname, col=col_name,
                                            ctype=new_type))
            self.conn.commit()
            cur.close()
            self.update_odict()
        else:
            # sqlite does not support rename columns:
            raise DBError('SQLite does not support to cast columns.')

    def drop(self, col_name):
        """Drop a column from the table. ::

            >>> import sqlite3
            >>> path = '$GISDBASE/$LOCATION_NAME/$MAPSET/sqlite/sqlite.db'
            >>> from grass.pygrass.functions import copy, remove
            >>> copy('census','mycensus','vect')
            >>> cols_sqlite = Columns('mycensus',
            ...                       sqlite3.connect(get_path(path)))
            >>> cols_sqlite.drop('CHILD')                 # doctest: +ELLIPSIS
            >>> 'CHILD' in cols_sqlite
            False

            >>> import psycopg2 as pg                         # doctest: +SKIP
            >>> cols_pg = Columns('boundary_municp_pg',
            ...                   pg.connect('host=localhost dbname=grassdb')) # doctest: +SKIP
            >>> cols_pg.drop('CHILD') # doctest: +SKIP
            >>> 'CHILD' in cols_pg # doctest: +SKIP
            False
            >>> remove('mycensus','vect')

            ..
        """
        cur = self.conn.cursor()
        if self.is_pg():
            cur.execute(sql.DROP_COL.format(tname=self.tname,
                                            cname=col_name))
        else:
            desc = str(self.sql_descr(remove=col_name))
            names = ', '.join(self.names(remove=col_name, unicod=False))
            queries = sql.DROP_COL_SQLITE.format(tname=self.tname,
                                                 keycol=self.key,
                                                 coldef=desc,
                                                 colnames=names).split('\n')
            for query in queries:
                cur.execute(query)
        self.conn.commit()
        cur.close()
        self.update_odict()


class Link(object):
    """Define a Link between vector map and the attributes table.

    It is possible to define a Link object or given all the information
    (layer, name, table name, key, database, driver): ::

        >>> link = Link(1, 'link0', 'census', 'cat',
        ...             '$GISDBASE/$LOCATION_NAME/PERMANENT/sqlite/sqlite.db', 'sqlite')
        >>> link.number                                       # doctest: +SKIP
        1
        >>> link.name
        'link0'
        >>> link.table_name
        'census'
        >>> link.key
        'cat'
        >>> link.database
        '$GISDBASE/$LOCATION_NAME/PERMANENT/sqlite/sqlite.db'
        >>> link.driver
        'sqlite'
        >>> link
        Link(1, link0, sqlite)


    It is possible to change parameters with: ::

        >>> link.driver = 'pg'                                # doctest: +SKIP
        >>> link.driver                                       # doctest: +SKIP
        'pg'
        >>> link.driver = 'postgres'                # doctest: +ELLIPSIS +SKIP
        Traceback (most recent call last):
          ...
        TypeError: Driver not supported, use: sqlite, pg.
        >>> link.driver                                       # doctest: +SKIP
        'pg'
        >>> link.number = 0                         # doctest: +ELLIPSIS +SKIP
        Traceback (most recent call last):
          ...
        TypeError: Number must be positive and greater than 0.


    Or given a c_fieldinfo object that is a ctypes pointer to the field_info C
    struct. ::

        >>> link = Link(c_fieldinfo = ctypes.pointer(libvect.field_info()))


    ..
    """
    def _get_layer(self):
        return self.c_fieldinfo.contents.number

    def _set_layer(self, number):
        if number <= 0:
            raise TypeError("Number must be positive and greater than 0.")
        self.c_fieldinfo.contents.number = number

    layer = property(fget=_get_layer, fset=_set_layer)

    def _get_name(self):
        return self.c_fieldinfo.contents.name

    def _set_name(self, name):
        self.c_fieldinfo.contents.name = name

    name = property(fget=_get_name, fset=_set_name)

    def _get_table(self):
        return self.c_fieldinfo.contents.table

    def _set_table(self, new_name):
        self.c_fieldinfo.contents.table = new_name

    table_name = property(fget=_get_table, fset=_set_table)

    def _get_key(self):
        return self.c_fieldinfo.contents.key

    def _set_key(self, key):
        self.c_fieldinfo.contents.key = key

    key = property(fget=_get_key, fset=_set_key)

    def _get_database(self):
        return self.c_fieldinfo.contents.database

    def _set_database(self, database):
        self.c_fieldinfo.contents.database = database

    database = property(fget=_get_database, fset=_set_database)

    def _get_driver(self):
        return self.c_fieldinfo.contents.driver

    def _set_driver(self, driver):
        if driver not in ('sqlite', 'pg'):
            str_err = "Driver not supported, use: %s." % ", ".join(DRIVERS)
            raise TypeError(str_err)
        self.c_fieldinfo.contents.driver = driver

    driver = property(fget=_get_driver, fset=_set_driver)

    def __init__(self, layer=1, name=None, table=None, key=None,
                 database=None, driver=None, c_fieldinfo=None):
        if c_fieldinfo is not None:
            self.c_fieldinfo = c_fieldinfo
        else:
            self.c_fieldinfo = ctypes.pointer(libvect.field_info())
            self.layer = layer
            self.name = name
            self.table_name = table
            self.key = key
            self.database = database
            self.driver = driver

    def __repr__(self):
        return "Link(%d, %s, %s)" % (self.layer, self.name, self.driver)

    def connection(self):
        """Return a connection object. ::

            >>> link = Link(1, 'link0', 'census', 'cat',
            ...             '$GISDBASE/$LOCATION_NAME/PERMANENT/sqlite/sqlite.db',
            ...             'sqlite')
            >>> conn = link.connection()
            >>> cur = conn.cursor()
            >>> cur.execute("SELECT cat,TOTAL_POP,PERIMETER FROM %s" %
            ...             link.table_name)              # doctest: +ELLIPSIS
            <sqlite3.Cursor object at ...>
            >>> cur.fetchone()
            (1, 44, 757.669)
            >>> cur.close()
            >>> conn.close()

        ...
        """
        if self.driver == 'sqlite':
            import sqlite3
            # Numpy is using some custom integer data types to efficiently
            # pack data into memory. Since these types aren't familiar to
            # sqlite, you'll have to tell it about how to handle them.
            for t in (np.int8, np.int16, np.int32, np.int64, np.uint8,
                      np.uint16, np.uint32, np.uint64):
                sqlite3.register_adapter(t, long)
            return sqlite3.connect(get_path(self.database))
        elif self.driver == 'pg':
            try:
                import psycopg2
                db = ' '.join(self.database.split(','))
                return psycopg2.connect(db)
            except ImportError:
                er = "You need to install psycopg2 to connect with this table."
                raise ImportError(er)
        else:
            str_err = "Driver is not supported yet, pleas use: sqlite or pg"
            raise TypeError(str_err)

    def table(self):
        """Return a Table object. ::

            >>> link = Link(1, 'link0', 'census', 'cat',
            ...             '$GISDBASE/$LOCATION_NAME/PERMANENT/sqlite/sqlite.db',
            ...             'sqlite')
            >>> table = link.table()
            >>> table.filters.select('cat', 'TOTAL_POP', 'PERIMETER')
            Filters('SELECT cat, TOTAL_POP, PERIMETER FROM census;')
            >>> cur = table.execute()
            >>> cur.fetchone()
            (1, 44, 757.669)
            >>> cur.close()

        ..
        """
        return Table(self.table_name, self.connection(), self.key)

    def info(self):
        """Print information of the link. ::

            >>> link = Link(1, 'link0', 'census', 'cat',
            ...             '$GISDBASE/$LOCATION_NAME/PERMANENT/sqlite/sqlite.db',
            ...             'sqlite')
            >>> link.info()
            layer:     1
            name:      link0
            table:     census
            key:       cat
            database:  $GISDBASE/$LOCATION_NAME/PERMANENT/sqlite/sqlite.db
            driver:    sqlite

        ..
        """
        print "layer:    ", self.layer
        print "name:     ", self.name
        print "table:    ", self.table_name
        print "key:      ", self.key
        print "database: ", self.database
        print "driver:   ", self.driver


class DBlinks(object):
    """Interface containing link to the table DB. ::

        >>> from grass.pygrass.vector import VectorTopo
        >>> cens = VectorTopo('census')
        >>> cens.open()
        >>> dblinks = DBlinks(cens.c_mapinfo)
        >>> dblinks
        DBlinks([Link(1, census, sqlite)])
        >>> dblinks[0]
        Link(1, census, sqlite)
        >>> dblinks['census']
        Link(1, census, sqlite)

    ..
    """
    def __init__(self, c_mapinfo):
        self.c_mapinfo = c_mapinfo

    def __len__(self):
        return self.num_dblinks()

    def __iter__(self):
        return (self.by_index(i)
                for i in xrange(self.num_dblinks()))

    def __getitem__(self, item):
        """

        """
        if isinstance(item, int):
                return self.by_index(item)
        else:
            return self.by_name(item)

    def __repr__(self):
        return "DBlinks(%r)" % [link for link in self.__iter__()]

    def by_index(self, indx):
        c_fieldinfo = libvect.Vect_get_dblink(self.c_mapinfo, indx)
        return Link(c_fieldinfo=c_fieldinfo)

    def by_layer(self, layer):
        c_fieldinfo = libvect.Vect_get_field(self.c_mapinfo, layer)
        return Link(c_fieldinfo=c_fieldinfo)

    def by_name(self, name):
        c_fieldinfo = libvect.Vect_get_field_by_name(self.c_mapinfo, name)
        return Link(c_fieldinfo=c_fieldinfo)

    def num_dblinks(self):
        return libvect.Vect_get_num_dblinks(self.c_mapinfo)

    def add(self, link):
        """Add a new link.
           Need to open vector map in write mode::

            >>> from grass.pygrass.vector import VectorTopo
            >>> municip = VectorTopo('census')
            >>> municip.open()
            >>> dblinks = DBlinks(municip.c_mapinfo)
            >>> dblinks
            DBlinks([Link(1, census, sqlite)])
            >>> link = Link(2, 'pg_link', 'boundary_municp_pg', 'cat',
            ...             'host=localhost dbname=grassdb', 'pg') # doctest: +SKIP
            >>> dblinks.add(link)                             # doctest: +SKIP
            >>> dblinks                                       # doctest: +SKIP
            DBlinks([Link(1, boundary_municp, sqlite)])

        ..
        """
        #TODO: check if open in write mode or not.
        libvect.Vect_map_add_dblink(self.c_mapinfo,
                                    link.layer, link.name, link.table_name,
                                    link.key, link.database, link.driver)

    def remove(self, key, force=False):
        """Remove a link. If force set to true remove also the table ::

            >>> from grass.pygrass.vector import VectorTopo
            >>> municip = VectorTopo('census')
            >>> municip.open()
            >>> dblinks = DBlinks(municip.c_mapinfo)
            >>> dblinks
            DBlinks([Link(1, census, sqlite)])
            >>> dblinks.remove('pg_link')                     # doctest: +SKIP
            >>> dblinks  # need to open vector map in write mode
            DBlinks([Link(1, census, sqlite)])

        ..
        """
        if force:
            link = self.by_name(key)
            table = link.table()
            table.drop()
        if isinstance(key, str):
            key = self.from_name_to_num(key)
        libvect.Vect_map_del_dblink(self.c_mapinfo, key)

    def from_name_to_num(self, name):
        """
        Vect_get_field_number
        """
        return libvect.Vect_get_field_number(self.c_mapinfo, name)


class Table(object):
    """::

        >>> import sqlite3
        >>> path = '$GISDBASE/$LOCATION_NAME/PERMANENT/sqlite/sqlite.db'
        >>> tab_sqlite = Table(name='census',
        ...                    connection=sqlite3.connect(get_path(path)))
        >>> tab_sqlite.name
        'census'
        >>> import psycopg2                                   # doctest: +SKIP
        >>> tab_pg = Table('boundary_municp_pg',
        ...                psycopg2.connect('host=localhost dbname=grassdb',
        ...                                 'pg'))            # doctest: +SKIP
        >>> tab_pg.columns                          # doctest: +ELLIPSIS +SKIP
        Columns([('cat', 'int4'), ...])

    ..
    """
    def _get_name(self):
        """Private method to return the name of table"""
        return self._name

    def _set_name(self, new_name):
        """Private method to set the name of table"""
        old_name = self._name
        cur = self.conn.cursor()
        cur.execute(sql.RENAME_TAB.format(old_name=old_name,
                                          new_name=new_name))
        cur.commit()
        cur.close()

    name = property(fget=_get_name, fset=_set_name)

    def __init__(self, name, connection, key='cat'):
        self._name = name
        self.conn = connection
        self.key = key
        self.columns = Columns(self.name,
                               self.conn,
                               self.key)
        self.filters = Filters(self.name)

    def __repr__(self):
        """::

            >>> import sqlite3
            >>> path = '$GISDBASE/$LOCATION_NAME/PERMANENT/sqlite/sqlite.db'
            >>> tab_sqlite = Table(name='census',
            ...                    connection=sqlite3.connect(get_path(path)))
            >>> tab_sqlite
            Table('census')

        ..
        """
        return "Table(%r)" % (self.name)

    def __iter__(self):
        cur = self.execute()
        return (cur.fetchone() for _ in xrange(self.__len__()))

    def __len__(self):
        """Return the number of rows"""
        return self.n_rows()

    def drop(self, name=None, force=False):
        """Private method to drop table from database"""
        if name:
            name = name
        else:
            name = self.name
        used = db_table_in_vector(name)
        if len(used) > 0:
            warning(_("Deleting table <%s> which is attached to following map(s):") % table)
            for vect in used:
                warning("%s" % vect)
            if not force:
                warning(_("You must use the force flag to actually remove it. Exiting."))
                sys.exit(0)
            else:
                cur = self.conn.cursor()
                cur.execute(sql.DROP_TAB.format(tname=name))
        else:
            cur = self.conn.cursor()
            cur.execute(sql.DROP_TAB.format(tname=name))

    def n_rows(self):
        """Return the number of rows

        >>> import sqlite3
        >>> path = '$GISDBASE/$LOCATION_NAME/PERMANENT/sqlite/sqlite.db'
        >>> tab_sqlite = Table(name='census',
        ...                    connection=sqlite3.connect(get_path(path)))
        >>> tab_sqlite.n_rows()
        2537
        """
        cur = self.conn.cursor()
        cur.execute(sql.SELECT.format(cols='Count(*)', tname=self.name))
        number = cur.fetchone()[0]
        cur.close()
        return number

    def execute(self, sql_code=None):
        """Execute SQL code from a given string or build with filters and
        return a cursor object. ::

            >>> import sqlite3
            >>> path = '$GISDBASE/$LOCATION_NAME/PERMANENT/sqlite/sqlite.db'
            >>> tab_sqlite = Table(name='census',
            ...                    connection=sqlite3.connect(get_path(path)))
            >>> tab_sqlite.filters.select('cat', 'TOTAL_POP').order_by('AREA')
            Filters('SELECT cat, TOTAL_POP FROM census ORDER BY AREA;')
            >>> cur = tab_sqlite.execute()
            >>> cur.fetchone()
            (1856, 0)

        ..
        """
        try:
            sqlc = sql_code if sql_code else self.filters.get_sql()
            cur = self.conn.cursor()
            #if hasattr(self.cur, 'executescript'):
            #    return cur.executescript(sqlc)
            return cur.execute(sqlc)
        except:
            raise ValueError("The SQL is not correct:\n%r" % sqlc)

    def insert(self, values, many=False):
        """Insert a new row"""
        cur = self.conn.cursor()
        if many:
            return cur.executemany(self.columns.insert_str, values)
        return cur.execute(self.columns.insert_str, values)

    def update(self, key, values):
        """Update a column for each row"""
        cur = self.conn.cursor()
        vals = list(values)
        vals.append(key)
        return cur.execute(self.columns.update_str, vals)

    def create(self, cols, name=None):
        """Create a new table"""
        cur = self.conn.cursor()
        coldef = ',\n'.join(['%s %s' % col for col in cols])
        if name:
            newname = name
        else:
            newname = self.name
        cur.execute(sql.CREATE_TAB.format(tname=newname, coldef=coldef))
        self.conn.commit()
        cur.close()
        self.columns.update_odict()

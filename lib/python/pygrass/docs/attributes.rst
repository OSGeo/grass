Attributes
===========

It is possible to access to the vector attributes with: ::

    >>> from pygrass.vector import Vector
    >>> municip = Vector('boundary_municp_sqlite')
    >>> municip.open()
    >>> municip.dblinks
    DBlinks([[Link(1, boundary_municp, sqlite)]])

The vector map have a ``table`` attributes that contain a Table object, that
have some useful attributes like: layer, name, driver, etc.

    >>> link = municip.dblinks[1]
    >>> link.number
    1
    >>> link.name
    'boundary_municp'
    >>> link.table_name
    'boundary_municp_sqlite'
    >>> link.driver
    'sqlite'
    >>> link.database                                     # doctest: +ELLIPSIS
    '.../sqlite.db'
    >>> link.key
    'cat'

It is possible to change values, like: ::

    >>> link.name = 'boundary'
    >>> link.driver = 'pg'
    >>> link.database = 'host=localhost,dbname=grassdb'
    >>> link.key = 'gid'

Now change again to old values: ::

    >>> link.name = 'boundary_municp_sqlite'
    >>> link.driver = 'sqlite'
    >>> link.database = '$GISDBASE/$LOCATION_NAME/$MAPSET/sqlite.db'
    >>> link.key = 'cat'

Link object have methods that return a
:ref:`Connection object <python:library:sqlite3:connection-objects>`, and to
return a Table object: ::

    >>> conn = link.connection()
    >>> cur = conn.cursor()
    >>> import sql
    >>> cur.execute(sql.SELECT.format(cols=', '.join(['cat', 'AREA']),
    ...                               tname=link.name))   # doctest: +ELLIPSIS
    <sqlite3.Cursor object at ...>
    >>> cur.fetchone()
    (1, 0.0)
    >>> cur.close()
    >>> conn.close()

From the Link object we can instantiate a Table object that allow user to
make simple query with the Filters object: ::

    >>> table = link.table()
    >>> table.filters.select('cat', 'COUNTY',
    ...                      'AREA','PERIMETER').order_by('AREA').limit(3)
    Filters('SELECT cat, COUNTY, AREA, PERIMETER FROM boundary_municp_sqlite ORDER BY AREA LIMIT 3;')
    >>> cur = table.execute()
    >>> for row in cur.fetchall():
    ...     print repr(row)
    ...
    (1, u'SURRY', 0.0, 1415.331)
    (2, u'SURRY', 0.0, 48286.011)
    (3, u'CASWELL', 0.0, 5750.087)


Then we can get table information about table columns, from the columns
attribute that is an instantiation of a Columns class.


    >>> table.columns                                     # doctest: +ELLIPSIS
    Columns([(u'cat', u'integer'), ..., (u'ACRES', u'double precision')])
    >>> table.columns.names()                             # doctest: +ELLIPSIS
    [u'cat', u'OBJECTID', u'AREA', u'PERIMETER', ..., u'ACRES']
    >>> table.columns.types()                             # doctest: +ELLIPSIS
    [u'integer', u'integer', u'double precision', ..., u'double precision']


.. note ::
    If the map use postgresql it is possible to: add/rename/cast/remove columns
    the sqlite does not support these operations.


For people that are used to the standardized Python SQL 2.0 interface:

    * http://docs.python.org/library/sqlite3.html
    * http://www.python.org/dev/peps/pep-0249/

Therefore advanced user can just use, the connect attribute to build
a new cursor object and interact with the database. ::

    >>> cur = table.conn.cursor()
    >>> cur.execute("SELECT * FROM %s" % table.name)     # doctest: +ELLIPSIS
    <sqlite3.Cursor object at ...>
    >>> cur.fetchone()[:5]                               # doctest: +ELLIPSIS
    (1, 1, 0.0, 1415.331, 2.0)
    >>> # Close communication with the database
    >>> cur.close()
    >>> conn.close()



Link
-------

.. autoclass:: pygrass.vector.table.Link
    :members:

DBlinks
-------

.. autoclass:: pygrass.vector.table.DBlinks
    :members:

Filters
-------

.. autoclass:: pygrass.vector.table.Filters
    :members:

Columns
-------

.. autoclass:: pygrass.vector.table.Columns
    :members:

Table
-----

.. autoclass:: pygrass.vector.table.Table
    :members:
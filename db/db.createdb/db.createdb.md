## DESCRIPTION

*db.createdb* allows the user to create a new empty database through
different drivers. A working database connection needs to be
established, see *[db.login](db.login.html)*.

Currently only [SQLite](grass-sqlite.html) and
[PostgreSQL](grass-pg.html) database drivers are supported.

## EXAMPLES

### Create a new SQLite file-based database

Note that the standard GRASS GIS SQLite database is by default generated
in the user\'s current mapset. This example shows an out-of-mapset
database file creation:

```
db.createdb driver=sqlite database=/opt/sqlite.db
```

### Create a new PostgreSQL database

Create a new PostgreSQL database (after the PostgreSQL connection got
established through the *pg* driver):

```
db.createdb driver=pg database=grassdb
```

Create a new PostgreSQL database (after the PostgreSQL connection got
established through the *odbc* driver):

```
db.createdb driver=odbc database=grassdb
```

## TODO

Support other database drivers, too.

## SEE ALSO

*[db.columns](db.columns.html), [db.connect](db.connect.html),
[db.describe](db.describe.html), [db.drivers](db.drivers.html),
[db.dropdb](db.dropdb.html), [db.droptable](db.droptable.html),
[db.execute](db.execute.html), [db.login](db.login.html),
[db.tables](db.tables.html)*

[GRASS SQL interface](sql.html)

## AUTHORS

Radim Blazek, ITC-Irst, Trento, Italy\
SQLite and PostgreSQL support by Martin Landa, Czech Technical
University in Prague, Czech Republic

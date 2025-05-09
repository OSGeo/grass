## DESCRIPTION

*db.createdb* allows the user to create a new empty database through
different drivers. A working database connection needs to be
established, see *[db.login](db.login.md)*.

Currently only [SQLite](grass-sqlite.md) and [PostgreSQL](grass-pg.md)
database drivers are supported.

## EXAMPLES

### Create a new SQLite file-based database

Note that the standard GRASS GIS SQLite database is by default generated
in the user's current mapset. This example shows an out-of-mapset
database file creation:

```sh
db.createdb driver=sqlite database=/opt/sqlite.db
```

### Create a new PostgreSQL database

Create a new PostgreSQL database (after the PostgreSQL connection got
established through the *pg* driver):

```sh
db.createdb driver=pg database=grassdb
```

Create a new PostgreSQL database (after the PostgreSQL connection got
established through the *odbc* driver):

```sh
db.createdb driver=odbc database=grassdb
```

## TODO

Support other database drivers, too.

## SEE ALSO

*[db.columns](db.columns.md), [db.connect](db.connect.md),
[db.describe](db.describe.md), [db.drivers](db.drivers.md),
[db.dropdb](db.dropdb.md), [db.droptable](db.droptable.md),
[db.execute](db.execute.md), [db.login](db.login.md),
[db.tables](db.tables.md)*

[GRASS SQL interface](sql.md)

## AUTHORS

Radim Blazek, ITC-Irst, Trento, Italy  
SQLite and PostgreSQL support by Martin Landa, Czech Technical
University in Prague, Czech Republic

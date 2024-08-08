## DESCRIPTION

*db.dropdb* removes an existing database using given database
**driver**. Currently only [SQLite](grass-sqlite.html) and
[PostgreSQL](grass-pg.html) database drivers are supported.

## EXAMPLES

### Drop (delete) an existing database connected through SQLite driver

Note that the standard GRASS GIS SQLite database is by default found in
the user\'s current mapset. This example shows an out-of-mapset database
removal:

```
db.dropdb driver=sqlite database=/opt/sqlite.db
```

### Drop an existing database connected through PostgreSQL driver

```
db.dropdb driver=pg database=grassdb
```

## TODO

Support other database drivers, too.

## SEE ALSO

*[db.createdb](db.createdb.html), [db.describe](db.describe.html),
[db.droptable](db.droptable.html), [db.execute](db.execute.html),
[db.login](db.login.html), [db.tables](db.tables.html)*

[GRASS SQL interface](sql.html)

## AUTHORS

Radim Blazek, ITC-Irst, Trento, Italy\
SQLite and PostgreSQL support by Martin Landa, Czech Technical
University in Prague, Czech Republic

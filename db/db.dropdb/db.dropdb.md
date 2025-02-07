## DESCRIPTION

*db.dropdb* removes an existing database using given database
**driver**. Currently only [SQLite](grass-sqlite.md) and
[PostgreSQL](grass-pg.md) database drivers are supported.

## EXAMPLES

### Drop (delete) an existing database connected through SQLite driver

Note that the standard GRASS GIS SQLite database is by default found in
the user's current mapset. This example shows an out-of-mapset database
removal:

```sh
db.dropdb driver=sqlite database=/opt/sqlite.db
```

### Drop an existing database connected through PostgreSQL driver

```sh
db.dropdb driver=pg database=grassdb
```

## TODO

Support other database drivers, too.

## SEE ALSO

*[db.createdb](db.createdb.md), [db.describe](db.describe.md),
[db.droptable](db.droptable.md), [db.execute](db.execute.md),
[db.login](db.login.md), [db.tables](db.tables.md)*

[GRASS SQL interface](sql.md)

## AUTHORS

Radim Blazek, ITC-Irst, Trento, Italy  
SQLite and PostgreSQL support by Martin Landa, Czech Technical
University in Prague, Czech Republic

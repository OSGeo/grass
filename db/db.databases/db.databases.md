## DESCRIPTION

*db.databases* lists all databases for a given **driver** and optionally
**location**.

## NOTES

Currently supported database drivers are *[SQLite](grass-sqlite.html)*,
*[PostgreSQL](grass-pg.html)*, and *[ODBC](grass-odbc.html)*.

Default **location** for SQLite driver is the full path for the current
mapset. For PostgreSQL driver it\'s empty connection string.

## EXAMPLES

List SQLite databases in the current mapset:

```
db.databases driver=sqlite
```

List SQLite databases in a given directory:

```
db.databases driver=sqlite location=/opt/sqlite
```

List PostgreSQL databases from database server running on given port:

```
db.databases driver=pg location="host=server_name port=5333"
```

## SEE ALSO

*[db.columns](db.columns.html), [db.describe](db.describe.html),
[db.drivers](db.drivers.html), [db.execute](db.execute.html),
[db.login](db.login.html), [db.tables](db.tables.html)*

[GRASS SQL interface](sql.html)

## AUTHORS

Radim Blazek, ITC-Irst, Trento, Italy\
Updated for GRASS 7 by Martin Landa, Czech Technical University in
Prague, Czech Republic

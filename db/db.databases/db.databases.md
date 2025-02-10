## DESCRIPTION

*db.databases* lists all databases for a given **driver** and optionally
**location**.

## NOTES

Currently supported database drivers are *[SQLite](grass-sqlite.md)*,
*[PostgreSQL](grass-pg.md)*, and *[ODBC](grass-odbc.md)*.

Default **location** for SQLite driver is the full path for the current
mapset. For PostgreSQL driver it's empty connection string.

## EXAMPLES

List SQLite databases in the current mapset:

```sh
db.databases driver=sqlite
```

List SQLite databases in a given directory:

```sh
db.databases driver=sqlite location=/opt/sqlite
```

List PostgreSQL databases from database server running on given port:

```sh
db.databases driver=pg location="host=server_name port=5333"
```

## SEE ALSO

*[db.columns](db.columns.md), [db.describe](db.describe.md),
[db.drivers](db.drivers.md), [db.execute](db.execute.md),
[db.login](db.login.md), [db.tables](db.tables.md)*

[GRASS SQL interface](sql.md)

## AUTHORS

Radim Blazek, ITC-Irst, Trento, Italy  
Updated for GRASS 7 by Martin Landa, Czech Technical University in
Prague, Czech Republic

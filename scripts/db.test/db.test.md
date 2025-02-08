## DESCRIPTION

*db.test* tests database driver and database server running set of SQL
queries. Database must exist and connection must be set by *db.connect*.

## EXAMPLE

Test current SQL backend driver:

```sh
db.connect -p
db.test test=test1
```

## SEE ALSO

*[GRASS SQL interface](sql.md), [db.connect](db.connect.md),
[db.describe](db.describe.md), [db.drivers](db.drivers.md)*

## AUTHOR

Radim Blazek

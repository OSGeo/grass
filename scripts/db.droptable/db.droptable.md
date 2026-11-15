## DESCRIPTION

*db.droptable* drops an attribute table. If the **-f** force flag is not
given then nothing is removed, instead a preview of the action to be
taken is printed.

## NOTES

*db.droptable* is a front-end to *[db.execute](db.execute.md)* to allow
easier usage. To some extent it is verified if the table is connected to
a vector map to avoid accidental table removal.

## EXAMPLES

### Removing an attribute table from default database

```sh
# show default database
db.connect -p

# show available tables
db.tables -p

# this will show what would happen
db.droptable table=sometable

# actually drop the table
db.droptable -f table=sometable
```

### Removing an attribute table from given database

*db.droptable* allows defining optionally **driver** and **database**
options different from default connection settings (`db.connect -p`).

```sh
# drop the table from SQLite database
db.droptable -f table=sometable driver=sqlite database=/opt/sqlite.db
```

## SEE ALSO

*[db.dropdb](db.dropdb.md), [db.dropcolumn](db.dropcolumn.md),
[db.execute](db.execute.md), [db.login](db.login.md),
[db.connect](db.connect.md), [db.tables](db.tables.md),
[db.describe](db.describe.md), [v.db.droptable](v.db.droptable.md),*

[GRASS SQL interface](sql.md)

## AUTHORS

Markus Neteler  
Driver and database options added by Martin Landa, Czech Technical
University in Prague, Czech Republic

## DESCRIPTION

*db.droptable* drops an attribute table. If the **-f** force flag is not
given then nothing is removed, instead a preview of the action to be
taken is printed.

## NOTES

*db.droptable* is a front-end to *[db.execute](db.execute.html)* to
allow easier usage. To some extent it is verified if the table is
connected to a vector map to avoid accidental table removal.

## EXAMPLES

### Removing an attribute table from default database

::: code
    # show default database
    db.connect -p

    # show available tables
    db.tables -p

    # this will show what would happen
    db.droptable table=sometable

    # actually drop the table
    db.droptable -f table=sometable
:::

### Removing an attribute table from given database

*db.droptable* allows defining optionally **driver** and **database**
options different from default connection settings (`db.connect -p`).

::: code
    # drop the table from SQLite database
    db.droptable -f table=sometable driver=sqlite database=/opt/sqlite.db
:::

## SEE ALSO

*[db.dropdb](db.dropdb.html), [db.dropcolumn](db.dropcolumn.html),
[db.execute](db.execute.html), [db.login](db.login.html),
[db.connect](db.connect.html), [db.tables](db.tables.html),
[db.describe](db.describe.html), [v.db.droptable](v.db.droptable.html),*

[GRASS SQL interface](sql.html)

## AUTHORS

Markus Neteler\
Driver and database options added by Martin Landa, Czech Technical
University in Prague, Czech Republic

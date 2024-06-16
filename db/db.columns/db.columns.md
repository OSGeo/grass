## DESCRIPTION

*db.columns* lists all columns for a give table. Connection to databases
are supported through dbf, shp, odbc and pg drivers.

## NOTE

If parameters for database connection are already set with
[db.connect](db.connect.html), they are taken as default values and do
not need to be spcified each time.

## EXAMPLES

### List columns of a PostgreSQL attribute table

::: code
    db.columns table=zipcodes_wake driver=pg database=grassdb
:::

*If the database parameters are already set, the columns can be listed
directly*\

::: code
    db.columns table=zipcodes_wake
:::

### List columns from Shape file with DBF attribute table

::: code
    db.columns table=zipcodes_wake driver=dbf database=/grassdata/nc_spm_08/PERMANENT/dbf/
:::

### List columns of table in SQLite database

Note that the SQLite backend is the default setting.

::: code
    db.columns driver=sqlite table=archsites database='$GISDBASE/$LOCATION_NAME/$MAPSET/sqlite/sqlite.db'
:::

## SEE ALSO

*[db.connect](db.connect.html), [db.describe](db.describe.html),
[db.drivers](db.drivers.html), [db.droptable](db.droptable.html),
[db.execute](db.execute.html), [db.login](db.login.html),
[db.tables](db.tables.html), [GRASS SQL interface](sql.html)*

[GRASS SQL interface](sql.html)

## AUTHOR

Radim Blazek, ITC-Irst, Trento, Italy

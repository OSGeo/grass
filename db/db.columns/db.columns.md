## DESCRIPTION

*db.columns* lists all columns for a given table. Connection to databases
are supported through dbf, shp, odbc and pg drivers.

## NOTE

If parameters for database connection are already set with
[db.connect](db.connect.md), they are taken as default values and do not
need to be specified each time.

## EXAMPLES

### List columns of a PostgreSQL attribute table

```sh
db.columns table=zipcodes_wake driver=pg database=grassdb
```

*If the database parameters are already set, the columns can be listed
directly*  

```sh
db.columns table=zipcodes_wake
```

### List columns from Shape file with DBF attribute table

```sh
db.columns table=zipcodes_wake driver=dbf database=/grassdata/nc_spm_08/PERMANENT/dbf/
```

### List columns of table in SQLite database

Note that the SQLite backend is the default setting.

```sh
db.columns driver=sqlite table=archsites database='$GISDBASE/$LOCATION_NAME/$MAPSET/sqlite/sqlite.db'
```

### List columns using Python

```python
import grass.script as gs

data = gs.parse_command("db.columns", table="bridges", format="json")
print(data)
```

Possible output:

```text
['cat', 'OBJECTID', 'BRIDGES__1', 'SIPS_ID', 'TYPE', 'CLASSIFICA', 'BRIDGE_NUM', 'FEATURE_IN', 'FACILITY_C', 'LOCATION', 'YEAR_BUILT', 'WIDTH', 'CO_', 'CO_NAME']
```

## SEE ALSO

*[db.connect](db.connect.md), [db.describe](db.describe.md),
[db.drivers](db.drivers.md), [db.droptable](db.droptable.md),
[db.execute](db.execute.md), [db.login](db.login.md),
[db.tables](db.tables.md), [GRASS SQL interface](sql.md)*

[GRASS SQL interface](sql.md)

## AUTHOR

Radim Blazek, ITC-Irst, Trento, Italy

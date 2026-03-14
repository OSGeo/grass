## DESCRIPTION

*db.columns* lists all columns for a given table. Connection to databases
are supported through dbf, shp, odbc and pg drivers.

## NOTE

If parameters for database connection are already set with
[db.connect](db.connect.md), they are taken as default values and do not
need to be specified each time.

## EXAMPLES

### List columns

List of columns in shell:

```sh
db.columns table=zipcodes_wake format=list
```

```text
cat
OBJECTID
WAKE_ZIPCO
PERIMETER
ZIPCODE_
ZIPCODE_ID
ZIPNAME
ZIPNUM
ZIPCODE
NAME
SHAPE_Leng
SHAPE_Area
```

List of columns in Python:

```python
from grass.tools import Tools

columns = Tools().db_columns(table="zipcodes_wake", format="json")
print(list(columns))
```

```text
['cat', 'OBJECTID', 'WAKE_ZIPCO', 'PERIMETER', 'ZIPCODE_', 'ZIPCODE_ID', 'ZIPNAME', 'ZIPNUM', 'ZIPCODE', 'NAME', 'SHAPE_Leng', 'SHAPE_Area']
```

### List detailed column information

List column types in plain format:

```sh
db.columns -e table=zipcodes_wake
```

```text
cat: INTEGER
OBJECTID: INTEGER
WAKE_ZIPCO: DOUBLE PRECISION
PERIMETER: DOUBLE PRECISION
ZIPCODE_: DOUBLE PRECISION
ZIPCODE_ID: DOUBLE PRECISION
ZIPNAME: CHARACTER
ZIPNUM: DOUBLE PRECISION
ZIPCODE: CHARACTER
NAME: CHARACTER
SHAPE_Leng: DOUBLE PRECISION
SHAPE_Area: DOUBLE PRECISION
```

List column types in JSON format:

```sh
db.columns -e table=zipcodes_wake format=json
```

```json
[
    {
        "name": "cat",
        "sql_type": "INTEGER",
        "is_number": true
    },
    {
        "name": "OBJECTID",
        "sql_type": "INTEGER",
        "is_number": true
    },
    {
        "name": "WAKE_ZIPCO",
        "sql_type": "DOUBLE PRECISION",
        "is_number": true
    },

    ...
]
```

Create an SQL-like column list:

```sh
db.columns -e table=zipcodes_wake format=list separator=comma
```

```text
cat INTEGER,OBJECTID INTEGER,WAKE_ZIPCO DOUBLE PRECISION,PERIMETER DOUBLE PRECISION,ZIPCODE_ DOUBLE PRECISION,ZIPCODE_ID DOUBLE PRECISION,ZIPNAME CHARACTER,ZIPNUM DOUBLE PRECISION,ZIPCODE CHARACTER,NAME CHARACTER,SHAPE_Leng DOUBLE PRECISION,SHAPE_Area DOUBLE PRECISION
```

List column types in CSV format:

```sh
db.columns -e table=zipcodes_wake format=csv
```

```text
name,sql_type,is_number
cat,INTEGER,true
OBJECTID,INTEGER,true
WAKE_ZIPCO,DOUBLE PRECISION,true
PERIMETER,DOUBLE PRECISION,true
ZIPCODE_,DOUBLE PRECISION,true
ZIPCODE_ID,DOUBLE PRECISION,true
ZIPNAME,CHARACTER,false
ZIPNUM,DOUBLE PRECISION,true
ZIPCODE,CHARACTER,false
NAME,CHARACTER,false
SHAPE_Leng,DOUBLE PRECISION,true
SHAPE_Area,DOUBLE PRECISION,true
```

### List columns of a PostgreSQL attribute table

```sh
db.columns table=zipcodes_wake driver=pg database=grassdb
```

If the database parameters are already set, the columns can be listed
directly:

```sh
db.columns table=zipcodes_wake
```

## SEE ALSO

*[db.connect](db.connect.md), [db.describe](db.describe.md),
[db.drivers](db.drivers.md), [db.droptable](db.droptable.md),
[db.execute](db.execute.md), [db.login](db.login.md),
[db.tables](db.tables.md), [GRASS SQL interface](sql.md)*

[GRASS SQL interface](sql.md)

## AUTHOR

Radim Blazek, ITC-Irst, Trento, Italy

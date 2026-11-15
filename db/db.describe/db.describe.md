## DESCRIPTION

*db.describe* displays table information. If parameter **-c** is used
only column names instead of full column descriptions is given.

## NOTE

If parameters for database connection are already set with
[db.connect](db.connect.md), they are taken as default values and do not
need to be specified each time.

## EXAMPLES

*List column descriptions of table in SQLite database (note that this is
the default setting)*  

```sh
db.describe driver=sqlite table=hospitals \
   database='$GISDBASE/$LOCATION_NAME/$MAPSET/sqlite/sqlite.db'

# or simply
db.describe myarchsites
```

### DBF example

```sh
db.describe -c table=hospitals database='$GISDBASE/$LOCATION_NAME/PERMANENT/dbf/' \
            driver=dbf
ncols: 16
nrows: 160
Column 1: cat:INTEGER:11
Column 2: OBJECTID:INTEGER:11
Column 3: AREA:DOUBLE PRECISION:20
[...]
```

```sh
db.describe table=hospitals database='$GISDBASE/$LOCATION_NAME/PERMANENT/dbf/' \
            driver=dbf
table:hospitals
description:
insert:yes
delete:yes
ncols:16
nrows:160

column:cat
description:
type:INTEGER
len:11
scale:0
precision:10
default:
nullok:yes
select:yes
update:yes

column:OBJECTID
description:
type:INTEGER
[...]
```

### JSON Output

```sh
db.describe table=hospitals format=json
```

```sh
{
    "table": "hospitals",
    "description": "",
    "insert": null,
    "delete": null,
    "ncols": 16,
    "nrows": 160,
    "columns": [
        {
            "position": 1,
            "column": "cat",
            "description": "",
            "type": "INTEGER",
            "length": 20,
            "scale": 0,
            "precision": 0,
            "default": null,
            "nullok": true,
            "select": null,
            "update": null
        },
        {
            "position": 2,
            "column": "OBJECTID",
            "description": "",
            "type": "INTEGER",
            "length": 20,
            "scale": 0,
            "precision": 0,
            "default": null,
            "nullok": true,
            "select": null,
            "update": null
        },
        {
            "position": 3,
            "column": "AREA",
            "description": "",
            "type": "DOUBLE PRECISION",
            "length": 20,
            "scale": 0,
            "precision": 0,
            "default": null,
            "nullok": true,
            "select": null,
            "update": null
        },
        {
            "position": 4,
            "column": "PERIMETER",
            "description": "",
            "type": "DOUBLE PRECISION",
            "length": 20,
            "scale": 0,
            "precision": 0,
            "default": null,
            "nullok": true,
            "select": null,
            "update": null
        },
        {
            "position": 5,
            "column": "HLS_",
            "description": "",
            "type": "INTEGER",
            "length": 20,
            "scale": 0,
            "precision": 0,
            "default": null,
            "nullok": true,
            "select": null,
            "update": null
        },
        {
            "position": 6,
            "column": "HLS_ID",
            "description": "",
            "type": "INTEGER",
            "length": 20,
            "scale": 0,
            "precision": 0,
            "default": null,
            "nullok": true,
            "select": null,
            "update": null
        },
        {
            "position": 7,
            "column": "NAME",
            "description": "",
            "type": "CHARACTER",
            "length": 45,
            "scale": 0,
            "precision": 0,
            "default": null,
            "nullok": true,
            "select": null,
            "update": null
        },
        {
            "position": 8,
            "column": "ADDRESS",
            "description": "",
            "type": "CHARACTER",
            "length": 35,
            "scale": 0,
            "precision": 0,
            "default": null,
            "nullok": true,
            "select": null,
            "update": null
        },
        {
            "position": 9,
            "column": "CITY",
            "description": "",
            "type": "CHARACTER",
            "length": 16,
            "scale": 0,
            "precision": 0,
            "default": null,
            "nullok": true,
            "select": null,
            "update": null
        },
        {
            "position": 10,
            "column": "ZIP",
            "description": "",
            "type": "CHARACTER",
            "length": 5,
            "scale": 0,
            "precision": 0,
            "default": null,
            "nullok": true,
            "select": null,
            "update": null
        },
        {
            "position": 11,
            "column": "COUNTY",
            "description": "",
            "type": "CHARACTER",
            "length": 12,
            "scale": 0,
            "precision": 0,
            "default": null,
            "nullok": true,
            "select": null,
            "update": null
        },
        {
            "position": 12,
            "column": "PHONE",
            "description": "",
            "type": "CHARACTER",
            "length": 14,
            "scale": 0,
            "precision": 0,
            "default": null,
            "nullok": true,
            "select": null,
            "update": null
        },
        {
            "position": 13,
            "column": "CANCER",
            "description": "",
            "type": "CHARACTER",
            "length": 4,
            "scale": 0,
            "precision": 0,
            "default": null,
            "nullok": true,
            "select": null,
            "update": null
        },
        {
            "position": 14,
            "column": "POLYGONID",
            "description": "",
            "type": "INTEGER",
            "length": 20,
            "scale": 0,
            "precision": 0,
            "default": null,
            "nullok": true,
            "select": null,
            "update": null
        },
        {
            "position": 15,
            "column": "SCALE",
            "description": "",
            "type": "DOUBLE PRECISION",
            "length": 20,
            "scale": 0,
            "precision": 0,
            "default": null,
            "nullok": true,
            "select": null,
            "update": null
        },
        {
            "position": 16,
            "column": "ANGLE",
            "description": "",
            "type": "DOUBLE PRECISION",
            "length": 20,
            "scale": 0,
            "precision": 0,
            "default": null,
            "nullok": true,
            "select": null,
            "update": null
        }
    ]
}
```

## SEE ALSO

*[db.columns](db.columns.md), [db.droptable](db.droptable.md),
[db.execute](db.execute.md), [db.login](db.login.md),
[db.tables](db.tables.md), [GRASS SQL interface](sql.md)*

## AUTHOR

Radim Blazek, ITC-Irst, Trento, Italy

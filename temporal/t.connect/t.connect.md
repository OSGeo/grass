## DESCRIPTION

*t.connect* allows the user to set the temporal database connection. The
default setting is that the temporal database of type *sqlite3* is
located in the current mapset directory.

The **-p** flag will display the current temporal database connection
parameters. Use the **format** option to specify the output format: `plain`,
 `shell`, or `json` — with `plain` being the default.

**format=json** always returns a JSON array. By default it contains one entry
per mapset on the current search path.

The optional **mapset** option can be used to extend or restrict the selection
of mapsets to report the connection information for. Use `.` for the current
mapset only, `*` for all mapsets in the project, or a comma-separated list for
specific mapsets. The **mapset** option requires both the **-p** flag and
**format=json**; the **format=plain** and **format=shell** do not support
multi-mapset output.

The **-g** flag is deprecated and will be removed in a future release. Please
use **format=shell** option with the **-p** flag instead.

The **-c** flag will silently check if the temporal database connection
parameters have been set, and if not will set them to use GRASS's
default values.

## NOTES

Setting the connection with *t.connect* will not test the connection for
validity. Hence a database connection will not be established.

The connection values are stored in the mapset's `VAR` file. The **-d**
flag will set the default TGIS connection parameters. A SQLite database
"tgis/sqlite.db" will be created in the current mapset directory. It
will be located in the "tgis" sub-directory to not interfere with the
*sqlite3* database used for vector attribute storage.

In case you have tens of thousands of maps to register in the temporal
database or you need concurrent read and write access in the temporal
database, consider to use a PostgreSQL connection instead.

Be aware that you have to set the PostgreSQL connection explicitly in
every mapset that should store temporal information in the temporal
database.

PostgreSQL and SQLite databases can not be mixed in a project.

## EXAMPLES

### Print the current connection in different formats

Print in plain format (default):

```sh
t.connect -p
```

```text
driver:sqlite
database:$GISDBASE/$LOCATION_NAME/$MAPSET/tgis/sqlite.db
```

Print in shell format:

```sh
t.connect -p format=shell
```

```text
driver=sqlite
database=$GISDBASE/$LOCATION_NAME/$MAPSET/tgis/sqlite.db
```

Print as JSON (reports all mapsets on the search path as an array):

```sh
t.connect -p format=json
```

```json
[
    {
        "mapset": "PERMANENT",
        "driver": "sqlite",
        "database": "$GISDBASE/$LOCATION_NAME/PERMANENT/tgis/sqlite.db"
    },
    {
        "mapset": "user1",
        "driver": "sqlite",
        "database": "$GISDBASE/$LOCATION_NAME/user1/tgis/sqlite.db"
    }
]
```

Restrict JSON output to the current mapset only:

```sh
t.connect -p format=json mapset=.
```

```json
[
    {
        "mapset": "user1",
        "driver": "sqlite",
        "database": "$GISDBASE/$LOCATION_NAME/user1/tgis/sqlite.db"
    }
]
```

### Print connection for a subset of mapsets

Restrict JSON output to specific mapsets:

```sh
t.connect -p format=json mapset=PERMANENT,user1
```

```json
[
    {
        "mapset": "PERMANENT",
        "driver": "sqlite",
        "database": "$GISDBASE/$LOCATION_NAME/PERMANENT/tgis/sqlite.db"
    },
    {
        "mapset": "user1",
        "driver": "sqlite",
        "database": "$GISDBASE/$LOCATION_NAME/user1/tgis/sqlite.db"
    }
]
```

Use `mapset=*` to include all mapsets in the project, not just those on the
search path:

```sh
t.connect -p format=json mapset=*
```

### SQLite

The SQLite database is created automatically when used the first time.

```sh
# use single quotes here
t.connect driver=sqlite database='$GISDBASE/$LOCATION_NAME/PERMANENT/tgis/sqlite.db'
t.connect -p
t.info -s
```

### PostgreSQL

When using a PostgreSQL database, the user will need to specify the TGIS
database connection for each mapset.

```sh
t.connect driver=pg database="dbname=grass_test user=soeren password=abcdefgh"
t.connect -p
t.info -s
```

## AUTHOR

Soeren Gebbert, Thünen Institute of Climate-Smart Agriculture

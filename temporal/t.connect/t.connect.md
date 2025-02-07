## DESCRIPTION

*t.connect* allows the user to set the temporal database connection. The
default setting is that the temporal database of type *sqlite3* is
located in the current mapset directory.

The **-p** flag will display the current temporal database connection
parameters.

The **-pg** flag will display the current temporal database connection
parameters using shell style.

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

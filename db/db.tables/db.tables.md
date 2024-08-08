## DESCRIPTION

*db.tables* lists all tables for a given database.

## NOTE

If parameters for database connection are already set with
[db.connect](db.connect.html), they are taken as default values and do
not need to be spcified each time.

## EXAMPLES

### List all tables if database connection is already set

```
db.tables -p
```

### List all DBF tables

```
db.tables driver=dbf database=/grassdata/nc_spm_08/user1/PERMANENT/dbf
```

### List all tables in SQLite database (note that this is the default setting)

```
db.tables -p driver=sqlite database='$GISDBASE/$LOCATION_NAME/$MAPSET/sqlite/sqlite.db'
```

## SEE ALSO

*[db.columns](db.columns.html), [db.droptable](db.droptable.html),
[db.login](db.login.html), [db.execute](db.execute.html), [GRASS SQL
interface](sql.html)*

## AUTHOR

Unknown

## DESCRIPTION

*db.tables* lists all tables for a given database.

## NOTE

If parameters for database connection are already set with
[db.connect](db.connect.md), they are taken as default values and do not
need to be specified each time.

## EXAMPLES

### List all tables if database connection is already set

```sh
db.tables -p
```

### List all DBF tables

```sh
db.tables driver=dbf database=/grassdata/nc_spm_08/user1/PERMANENT/dbf
```

### List all tables in SQLite database (note that this is the default setting)

```sh
db.tables -p driver=sqlite database='$GISDBASE/$LOCATION_NAME/$MAPSET/sqlite/sqlite.db'
```

## SEE ALSO

*[db.columns](db.columns.md), [db.droptable](db.droptable.md),
[db.login](db.login.md), [db.execute](db.execute.md), [GRASS SQL
interface](sql.md)*

## AUTHOR

Unknown

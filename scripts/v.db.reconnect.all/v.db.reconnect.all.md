## DESCRIPTION

*v.db.reconnect.all* changes database connection of all layers of all
vector maps in the current mapset from the source (**old_database**) to
the target (**new_database**) database. If a link does not match the
**old_database** it is left untouched.

If no new database is given, the default datase of the mapset is used as
printed by `db.connect -g`. If no old database is given, all layers
without a link in the new database will be liniked to the new database.
If an old database is given, only links in the old database will be
changed.

Optionally attribute tables in **new_database** can be created if not
exist by **-c** flag. In this case *v.db.reconnect.all* also tries to
create an index on key column (usually "cat" column).

## NOTES

The value of the **old_database** option needs to be the exact string
which appears as the fourth field printed by `v.db.connect -g`.

```sh
v.db.connect -g map=census
1/census|census|cat|/home/user/grassdata/nc_spm_base/PERMANENT/dbf/|dbf
```

*v.db.reconnect.all* respect also variables to be substituted. In the
example above, database
`/home/user/grassdata/nc_spm_base/PERMANENT/dbf/` can be also defined as
`'$GISDBASE/$LOCATION_NAME/$MAPSET/dbf/'` (see examples).

Attribute tables from **old_database** are after reconnecting left
untouched. *v.db.reconnect.all* deletes those tables automatically only
when **-d** flag is given.

Also note that *v.db.reconnect.all* doesn't change default database
driver or database (`db.connect -p`). Default database connection
settings for newly created attribute data can be defined by
*[db.connect](db.connect.md)*.

## EXAMPLES

In the examples below are assumed, that attribute tables are linked to
the vector maps through [DBF](grass-dbf.md) database driver.

### Reconnect DBF attribute tables to SQLite database

Reconnect [DBF](grass-dbf.md) attribute tables linked to the vector maps
in the current mapset to [SQLite](grass-sqlite.md) database:

```sh
v.db.reconnect.all old_database='$GISDBASE/$LOCATION_NAME/$MAPSET/dbf/' \
 new_driver=sqlite new_database='$GISDBASE/$LOCATION_NAME/$MAPSET/sqlite/sqlite.db'
```

If attribute table doesn't exist in the target database
(**new_database**) then the module prints an error message.

### Convert DBF attribute tables to SQLite database

For coping DBF tables to SQLite database and reconnecting them for all
vector maps in the current mapset must be defined also **-c** flag.

```sh
v.db.reconnect.all -c old_database='$GISDBASE/$LOCATION_NAME/$MAPSET/dbf/' \
 new_driver=sqlite new_database='$GISDBASE/$LOCATION_NAME/$MAPSET/sqlite/sqlite.db'
```

or alternatively

```sh
# set default connection (sqlite)
db.connect -d
# verify default connection
db.connect -g
# reconnect
v.db.reconnect.all -c old_database='$GISDBASE/$LOCATION_NAME/$MAPSET/dbf/'
```

To automatically remove original DBF attribute tables after reconnecting
the vector maps use **-d** flag. Note that attribute tables will be
deleted *permanently* from the source database. This option should be
used very carefully!

### Convert GRASS 6 vector map to GRASS 7

To become usable in GRASS 7, all vector maps in a mapset need to be
updated:

```sh
# first rebuild topology for all vector maps
v.build.all

# set new default db connection (to SQLite default)
db.connect -d

# copy attribute tables from old DB to new SQLite DB, delete old tables in DBF format
v.db.reconnect.all -cd
```

## SEE ALSO

*[v.db.connect](v.db.connect.md), [db.connect](db.connect.md),
[db.copy](db.copy.md), [db.createdb](db.createdb.md),
[db.droptable](db.droptable.md)*

*[GRASS SQL interface](sql.md)*

## AUTHORS

Radim Blazek  
Major update by Martin Landa, Czech Technical University in Prague,
Czech Republic

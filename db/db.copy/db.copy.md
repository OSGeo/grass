## DESCRIPTION

*db.copy* allows the user to copy a table between two databases.
Databases can be connected through different drivers (see examples
below).

## NOTES

Attribute tables can be copied using *db.copy* and, when to be
associated to a vector map, assigned to the map with
*[v.db.connect](v.db.connect.md)*. Current connection settings are saved
in the file *\$LOCATION/vector_map/dbln*.

## EXAMPLES

### From DBF to PostgreSQL

*Storing table 'geonames.dbf' (in current directory) into PostgreSQL
through ODBC:*  

```sh
db.copy from_driver=dbf from_database='$GISDBASE/$LOCATION_NAME/PERMANENT/dbf' \
  from_table=geonames to_driver=pg to_database="host=pgserver,dbname=testdb" \
  to_table=geonames
```

### From PostgreSQL to DBF

```sh
db.copy from_driver=pg  from_database="host=pgserver.example.org,dbname=testdb" \
  from_table=origtable to_driver=dbf \
  to_database='$GISDBASE/$LOCATION_NAME/$MAPSET/dbf' to_table=origtable
```

### From PostgreSQL to PostgreSQL with condition

```sh
db.copy from_driver=pg  from_database="host=localhost,dbname=testdb" \
  from_table=geonames to_driver=pg to_database="host=localhost,dbname=testdb" \
  to_table=selection where="cat < 500"
```

### From DBF to SQLite

```sh
db.copy from_driver=dbf from_database='$GISDBASE/$LOCATION_NAME/PERMANENT/dbf' \
   from_table=geonames_features to_driver=sqlite \
   to_database='$GISDBASE/$LOCATION_NAME/$MAPSET/sqlite/sqlite.db' to_table=geonames_features

# convenient viewer:
sqlitebrowser $HOME/grassdata/nc_spm_08/user1/sqlite/sqlite.db
```

### From SQLite to DBF

```sh
db.copy from_driver=sqlite from_database='$GISDBASE/$LOCATION_NAME/$MAPSET/sqlite/sqlite.db' \
   from_table=ammprv to_driver=dbf to_database='$GISDBASE/$LOCATION_NAME/$MAPSET/dbf/' \
   to_table=ammprv
```

## SEE ALSO

*[db.connect](db.connect.md), [db.drivers](db.drivers.md),
[db.login](db.login.md), [v.db.connect](v.db.connect.md),
[v.clean](v.clean.md)*

[GRASS SQL interface](sql.md)

## AUTHOR

Radim Blazek, ITC-irst, Trento, Italy

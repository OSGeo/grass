## DESCRIPTION

*v.db.connect* prints or sets database connection for a vector map. The
user can add or remove link to attribute table on the certain layer.

## NOTE

Connection information (driver, database, table, key) is stored for each
map, in the file

```sh
<database>/<project>/<mapset>/vector/<map>/dbln
```

If parameters for database connection are already set with
[db.connect](db.connect.md), they are taken as default values and do not
need to be specified each time.

When printing database connection (*p* or *g* flag) the parameter
*layer* is ignored, i.e. **all** connections are printed to the output.

**Attention:** Removing a vector map will also delete all tables linked
to it! If you use [v.db.connect](db.connect.md) to link further tables
to your map, it is advisable to make a copy from those tables first and
connect the copied tables to the vector map (see also
[v.overlay](v.overlay.md)).

## EXAMPLE

Note: The default database backend setting is SQLite.

### Print database connection

Print all database connection parameters for vector map.

```sh
v.db.connect -p map=roads
```

Print column types and names of table linked to vector map.

```sh
v.db.connect -c map=roads
```

### Connect vector map to database (DBF driver)

Connect vector map to DBF table without or with variables.  

Using default DB connection:

```sh
v.db.connect map=vectormap table=table
```

Using hardcoded path to DBF directory (not recommended):  

```sh
v.db.connect map=vectormap table=table \
             database=/home/user/grassdata/spearfish60/PERMANENT/dbf
```

Using variable as DBF directory definition, single quotes must be
used:  

```sh
v.db.connect map=vectormap table=table \
             database='$GISDBASE/$LOCATION_NAME/$MAPSET/dbf/'
```

Connect vector map layer 2 and key ID to database with variables (note:
if needed, single quotes must be used for the *database* parameter):

```sh
v.db.connect map=vectormap table=table layer=2 key=ID
```

### Connect vector map to database (SQLite driver)

Very similar to DBF driver example above.

```sh
db.connect driver=sqlite database='$GISDBASE/$LOCATION_NAME/$MAPSET/sqlite/sqlite.db'
db.tables -p
v.db.connect map=vectormap table=table driver=sqlite \
             database='$GISDBASE/$LOCATION_NAME/$MAPSET/sqlite/sqlite.db'
v.db.connect -p map=vectormap
```

### Connect vector map to database (MySQL driver)

```sh
# note: connection which requires password
db.connect driver=mysql database="host=dbserver.foo.org,dbname=my_database"
db.login user=joshua [password=xxx]
# ... or enter password interactively.

db.tables -p

# connect external table to layer 2:
v.db.connect map=my_map table=my_mysql_table key=baz layer=2
v.db.connect -p my_map
```

### Connect vector map to database (PostgreSQL driver)

```sh
# note: connection without password being asked
v.db.connect map=vectormap table=table layer=1 key=oid driver=pg \
             database="host=myserver.itc.it,dbname=mydb,user=name" \
             table=mytable key=id
```

### Store geometry in GRASS but attributes in PostgreSQL

This example illustrates a mixed data storage with possibility to update
attributes in an external PostgreSQL database:

```sh
# Check current settings for attribute storage:
db.connect -p

# Import table from PostgreSQL to new map
# (NOTE: output map name needs to be different from table name in
#        case that GRASS is connected to PostgreSQL):
v.in.db driver=pg database="host=localhost,dbname=meteo" \
        table=mytable x=lon y=lat key=cat out=mytable

v.db.connect map=mytable -p

# Cancel table connection between map and attribute table:
v.db.connect map=mytable -d
v.db.connect map=mytable -p

# Drop table which was replicated due to import:
db.tables -p
echo "DROP TABLE mytable" | db.execute
db.tables -p

# reconnect map to table in PostgreSQL:
v.db.connect map=mytable driver=pg database="host=localhost,dbname=meteo" \
        table=mytable key=cat

# Now the geometry is stored in GRASS while the attributes are stored
# in PostgreSQL.
```

An alternative is to create a "view" of only ID, x, y \[,z\] columns and
to use [v.in.db](v.in.db.md) on this view, then connect the original
table to the geometry. This will be faster if the original table is very
large.

### Store geometry in GRASS but attributes in PostGIS

This example illustrated a mixed data storage with possibility top
update attributes in external PostGIS database:

```sh
# Check current settings for attribute storage:
db.connect -p

# Import table from PostGIS to new map
# (NOTE: output map name needs to be different from table name in
#        case that GRASS is connected to PostGIS):
v.in.db driver=pg database="host=localhost,dbname=meteo" \
        table=mytable x="x(geom)" y="y(geom)" key=cat out=mytable

v.db.connect map=mytable -p

# Cancel table connection between map and attribute table:
v.db.connect map=mytable -d
v.db.connect map=mytable -p

# Drop table which was replicated due to import:
db.tables -p
echo "DROP TABLE mytable" | db.execute
db.tables -p

# reconnect map to table in PostGIS:
v.db.connect map=mytable driver=pg database="host=localhost,dbname=meteo" \
        table=mytable key=cat

# Now the geometry is stored in GRASS while the attributes are stored
# in PostGIS.
```

## SEE ALSO

*[db.connect](db.connect.md), [db.copy](db.copy.md),
[db.tables](db.tables.md), [v.db.addtable](v.db.addtable.md),
[v.db.droptable](v.db.droptable.md),
[v.db.addcolumn](v.db.addcolumn.md),
[v.db.dropcolumn](v.db.dropcolumn.md), [v.external](v.external.md),
[v.in.db](v.in.db.md), [v.overlay](v.overlay.md)*

*[GRASS SQL interface](sql.md)*

## AUTHOR

Radim Blazek, ITC-Irst, Trento, Italy

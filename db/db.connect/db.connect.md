## DESCRIPTION

*db.connect* allows the user to set database connection parameters.
These parameters are then taken as default values by modules so that the
user does not need to enter the parameters each time.

The default database backend in GRASS GIS is [SQLite](grass-sqlite.md)
(since version 7).

## NOTES

Values are stored in the mapset's `VAR` file; the connection is not
tested for validity.

The **-p** flag will display the current connection parameters.

The **-c** flag will silently check if the connection parameters have
been set, and if not will set them to use GRASS's default values.
(useful in scripts before you attempt to create a new database table)

To connect a vector map to a database table, use
*[v.db.connect](v.db.connect.md)* or
*[v.db.addtable](v.db.addtable.md)*.

## EXAMPLES

### SQLite (default backend)

Local storage:  

```sh
db.connect -d
db.connect -p
db.tables -p
```

The SQLite database file is created automatically when used the first
time.

See [SQLite](grass-sqlite.md) database driver for details.

### PostgreSQL (local connection)

Local storage, database tables stored in database "mydb" (may require
the use of *[db.login](db.login.md)*):  

```sh
db.connect driver=pg database=mydb
db.login user=myname pass=secret
db.connect -p
db.tables -p
```

See [PostgreSQL](grass-pg.md) database driver for details.

### PostgreSQL (network connection)

Network storage, database tables stored in database "mydb" (may require
the use of *[db.login](db.login.md)*):  

```sh
db.connect driver=pg database=mydb
db.login user=myname pass=secret host=myserver.com port=6666
db.connect -p
db.tables -p
```

See [PostgreSQL](grass-pg.md) database driver for details.

### MySQL (local connection)

Local storage, database tables stored in database "mydb" (may require
the use of *[db.login](db.login.md)*):  

```sh
db.connect driver=mysql database=mydb
db.login user=myname pass=secret
db.connect -p
db.tables -p
```

See [MySQL](grass-mysql.md) database driver for details.

### MySQL (network connection)

Network storage, database tables stored in database "mydb" (may require
the use of *[db.login](db.login.md)*):  

```sh
db.connect driver=mysql database=mydb
db.login user=myname pass=secret host=myserver.com
db.connect -p
db.tables -p
```

See [MySQL](grass-mysql.md) database driver for details.

### ODBC

Network storage, database tables stored in database "mydb" (may require
the use of *[db.login](db.login.md)*):  

```sh
db.connect driver=odbc database=mydb
db.login user=myname pass=secret
db.connect -p
db.tables -p
```

See [ODBC](grass-odbc.md) database driver for details.

### DBF (local, not recommended)

Local storage (the dbf/ subdirectory in the mapset must exist or must be
created by the user):  

```sh
db.connect driver=dbf database='$GISDBASE/$LOCATION_NAME/$MAPSET/dbf/'
db.tables -p
```

See [DBF](grass-dbf.md) database driver for details.

## SEE ALSO

*[db.columns](db.columns.md), [db.copy](db.copy.md),
[db.drivers](db.drivers.md), [db.login](db.login.md),
[db.tables](db.tables.md), [v.db.addtable](v.db.addtable.md),
[v.db.connect](v.db.connect.md)*

[GRASS SQL interface](sql.md)

## AUTHORS

Main author: Radim Blazek, ITC-Irst, Trento, Italy  
GRASS 7 improvements: Martin Landa, Markus Metz

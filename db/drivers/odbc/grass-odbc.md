---
description: ODBC DATABASE DRIVER
---

# ODBC DATABASE DRIVER

Communication between GRASS and ODBC database for attribute management:

|                     |                   |                |                   |                  |
|:-------------------:|:-----------------:|:--------------:|:-----------------:|:----------------:|
| GRASS module \<-\>  |      \<--\>       | ODBC Interface |      \<--\>       |      RDBMS       |
|     ***GRASS***     | ***DBMI driver*** | ***unixODBC*** | ***ODBC driver*** | ***PostgreSQL*** |
|                     |                   |                |                   |   ***Oracle***   |
|                     |                   |                |                   |    ***...***     |

## Supported SQL commands

All SQL commands supported by ODBC.

## Operators available in conditions

All SQL operators supported by ODBC.

## EXAMPLE

In this example we copy the dbf file of a SHAPE map into ODBC, then
connect GRASS to the ODBC DBMS. Usually the table will be already
present in the DBMS.

### Defining the ODBC connection

#### MS-Windows

On MS-Windows, in order to be able to connect, the ODBC connection needs
to be configured using dedicated tools (tool called "ODBC Data Source
Administrator") and give a name to that connection. This name is then
used as database name when accessing from a client via ODBC.

#### Linux

Configure ODBC driver for selected database (manually or with
'ODBCConfig'). ODBC drivers are defined in /etc/odbcinst.ini. Here an
example:

```sh
 [PostgreSQL]
 Description     = ODBC for PostgreSQL
 Driver          = /usr/lib/libodbcpsql.so
 Setup           = /usr/lib/libodbcpsqlS.so
 FileUsage       = 1
```

Create DSN (data source name). The DSN is used as database name in db.\*
modules. Then DSN must be defined in $HOME/.odbc.ini (for this user
only) or in /etc/odbc.ini for (for all users) \[watch out for the
database name which appears twice and also for the PostgreSQL protocol
version\]. Omit blanks at the beginning of lines:

```sh
 [grass6test]
 Description             = PostgreSQL
 Driver                  = PostgreSQL
 Trace                   = No
 TraceFile               =

 Database                = grass6test
 Servername              = localhost
 UserName                = neteler
 Password                =
 Port                    = 5432
 Protocol                = 8.0

 ReadOnly                = No
 RowVersioning           = No
 ShowSystemTables        = No
 ShowOidColumn           = No
 FakeOidIndex            = No
 ConnSettings            =
```

Configuration of an DSN without GUI is described on
<https://www.unixodbc.org/odbcinst.html>, but odbc.ini and .odbc.ini may
be created by the 'ODBCConfig' tool. You can easily view your DSN
structure by 'DataManager'. Configuration with GUI is described on
<https://www.unixodbc.org/doc/UserManual/>

To find out about your PostgreSQL protocol, run:  

```sh
psql -V
```

### Using the ODBC driver

Now create a new database if not yet existing:

```sh
db.createdb driver=odbc database=grass6test
```

To store a table 'mytable.dbf' (here: in current directory) into
PostgreSQL through ODBC, run:

```sh
db.connect driver=odbc database=grass6test
db.copy from_driver=dbf from_database=./ from_table=mytable \
        to_driver=odbc to_database=grass6test to_table=mytable
```

Next link the map to the attribute table (now the ODBC table is used,
not the dbf file):

```sh
v.db.connect map=mytable.shp table=mytable key=ID \
             database=grass6test driver=odbc
v.db.connect -p
```

Finally a test: Here we should see the table columns (if the ODBC
connection works):

```sh
db.tables -p
db.columns table=mytable
```

Now the table name 'mytable' should appear.  
Doesn't work? Check with 'isql \<databasename\>' if the ODBC-PostgreSQL
connection is really established.

Note that you can also connect mySQL, Oracle etc. through ODBC to GRASS.

You can also check the vector map itself concerning a current link to a
table:

```sh
v.db.connect -p mytable.shp
```

which should print the database connection through ODBC to the defined
RDBMS.

## SEE ALSO

*[db.connect](db.connect.md), [v.db.connect](v.db.connect.md), [unixODBC
web site](https://www.unixodbc.org), [SQL support in GRASS GIS](sql.md)*

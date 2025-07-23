---
description: SQLite DATABASE DRIVER
---

# SQLite DATABASE DRIVER

The SQLite driver is the default DBMI backend.

## Creating a SQLite database

GRASS is automatically creating the SQLite database if it is not yet
existing when the first table is created in the SQLite database. It is
sufficient to define the connection (see next step).

## Connecting GRASS to SQLite

The database name 'sqlite.db' is at user's choice. Also the file storage
location can be freely chosen. If the database does not exist, it will
be automatically created when database content is created:

```sh
# example for storing DB in mapset directory (keep single quotes):
db.connect driver=sqlite database='$GISDBASE/$LOCATION_NAME/$MAPSET/sqlite/sqlite.db'
db.connect -p
```

## Supported SQL commands

All SQL commands supported by SQLite (for limitations, see SQLite help
page: [SQL As Understood By SQLite](https://www.sqlite.org/lang.html)
and [Unsupported SQL](https://www.sqlite.org/omitted.html)).

## Operators available in conditions

All SQL operators supported by SQLite.

## Browsing table data in DB

A convenient SQLite front-end is
[sqlitebrowser](https://sqlitebrowser.org/). To open a DB file stored
within the current mapset, the following way is suggested (corresponds
to above database connection):

```sh
# fetch GRASS variables as shell environment variables:
eval `g.gisenv`
# use double quotes:
sqlitebrowser "$GISDBASE/$LOCATION_NAME/$MAPSET"/sqlite/sqlite.db
```

## SEE ALSO

*[db.connect](db.connect.md), [db.execute](db.execute.md),
[db.select](db.select.md)*  
  
*[SQL support in GRASS GIS](sql.md)*  
  
*[SQLite web site](https://www.sqlite.org), [SQLite
manual](https://www.sqlite.org/quickstart.html), [sqlite - Management
Tools](https://www2.sqlite.org/cvstrac/wiki?p=ManagementTools)*

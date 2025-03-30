---
description: MySQL embedded driver
---

# MySQL embedded driver

MySQL database driver enables GRASS to store vector attributes in MySQL
embedded database without necessity to run MySQL server.

## Driver and database name

GRASS modules require 2 parameters to connect to a database. Those
parameters are 'driver' and 'database'. For MySQL embedded driver the
parameter 'driver' should be set to value 'mesql'. The parameter
'database' is a full path to the directory where database tables are
stored. The best place is a directory in the mapset. The directory must
be created before use of the driver. In the name of database it is
possible to use 3 variables:

- $GISDBASE - path to current GISBASE
- $LOCATION_NAME - name of current location
- $MAPSET - name of current mapset

Examples of connection parameters:

```sh
db.connect driver=mesql database='$GISDBASE/$LOCATION_NAME/$MAPSET/mysql'
db.connect driver=mesql database=/home/user1/db
```

## Data types, indexes

For more information about supported data types and indexes see the
documentation for [MySQL (mysql) driver](grass-mysql.md).

## Database type

Because database closing was found very slow if InnoDB was used, the
InnoDB storage is disabled by default (hardcoded '--skip-innodb' server
option).

## Note

The embedded server is started with hardcoded '--bootstrap' option to
avoid warning about missing "mysql.time_zone_leap_second table". This
can be fixed in future.

## Troubleshooting: SQL syntax error

Attempting to use a reserved SQL word as column or table name will
result in a "SQL syntax" error. The list of reserved words for MySQL can
be found in the [MySQL
manual](https://dev.mysql.com/doc/refman/8.4/en/keywords.html#keywords-in-current-series).

## AUTHOR

Radim Blazek

Credits: Development of the driver was sponsored by
[Faunalia](https://www.faunalia.it) (Italy) as part of a project for
[ATAC](https://www.atac.roma.it/).

## SEE ALSO

*[db.connect](db.connect.md), [SQL support in GRASS GIS](sql.md)*

---
description: MySQL DATABASE DRIVER
---

# MySQL DATABASE DRIVER

MySQL database driver enables GRASS to store vector attributes in MySQL
server.

Because vector attribute tables are created automatically when a new
vector is written and the name of the table is the same as the name of
the vector it is good practice to create a new database for each GRASS
mapset.

## Creating a MySQL database

A new database is created within MySQL:

```sh
mysql> CREATE DATABASE mydb;
```

See the MySQL manual for details.

## Driver and database name

GRASS modules require 2 parameters to connect to a database. Those
parameters are 'driver' and 'database'. For MySQL driver the parameter
'driver' should be set to value 'mysql'. The parameter 'database' can be
given in two formats:

- Database name - in case of connection from localhost
- String of comma separated list of key=value options. Supported options
  are:
  - dbname - database name
  - host - host name or IP address
  - port - server port number

Examples of connection parameters:

```sh
db.connect driver=mysql database=mytest
db.connect driver=mysql database='dbname=mytest,host=test.grass.org'
```

## Data types

GRASS supports almost all MySQL data types with following limitations:

- Binary columns (BINARY, VARBINARY, TINYBLOB, MEDIUMBLOB, BLOB,
  LONGBLOB) are not not supported. If a table with binary column(s) is
  used in GRASS a warning is printed and only the supported columns are
  returned in query results.
- Columns of type SET and ENUM are represented as string (VARCHAR).
- Very large integers in columns of type BIGINT can be lost or corrupted
  because GRASS does not support 64 bin integeres on most platforms.
- GRASS does not currently distinguish types TIMESTAMP and DATETIME.
  Both types are in GRASS interpreted as TIMESTAMP.

## Indexes

GRASS modules automatically create index on key column of vector
attributes table. The index on key column is important for performance
of modules which update the attribute table, for example v.to.db,
v.distance and v.what.rast.

## Privileges

Because MySQL does not support groups of users and because only MySQL
'root' can grant privileges to other users GRASS cannot automatically
grant select privileges on created tables to group of users.

If you want to give privilege to read data from your mapset to other
users you have to ask your MySQL server administrator to grant select
privilege to them on the MySQL database used for that mapset. For
example, to allow everybody to read data in from your database 'mydb':  

```sh
shell> mysql --user=root mysql
mysql> GRANT SELECT ON mydb.* TO ''@'%';
```

## Schemas

Because MySQL does not support database schemas the parameter 'schema'
of module db.connect should never be set to any value. If you set that
parameter for MySQL driver GRASS will try to write tables to the
specified schema which will result in errors.

## Groups

MySQL does not support user groups. Any settings specified by 'group'
parameter of module db.connect are ignored by GRASS for MySQL driver.

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

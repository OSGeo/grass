---
description: DBF DATABASE DRIVER
---

# DBF DATABASE DRIVER

The DBF driver is a file based attribute table driver.

## Defining the DBF driver

The DBF driver is a file based driver, in theory no user interaction is
required. However, if the settings should be set back from a different
driver to the DBF driver, the following step is required:

```sh
# keep single quotes:
db.connect driver=dbf database='$GISDBASE/$LOCATION_NAME/$MAPSET/dbf/'
db.connect -p
```

The dbf/ subdirectory in the mapset must exist or must be created by the
user.

## Creating a DBF table

DBF tables are created by GRASS when generating a vector map with
attributes and having defined the DBF as attribute driver.

If a DBF table has to be created manually, [db.execute](db.execute.md)
can be used or a spreadsheet application. Also [db.copy](db.copy.md) is
sometimes useful as well as [db.in.ogr](db.in.ogr.md) to import external
tables.

## Supported SQL commands by DBF driver

```sh
  ALTER TABLE table ADD [COLUMN] columndef
  ALTER TABLE table DROP COLUMN colname
  CREATE TABLE table ( columndefs )
  DROP TABLE table
  SELECT columns FROM table
  SELECT columns FROM table WHERE condition
  SELECT columns FROM table ORDER BY column
  DELETE FROM table
  DELETE FROM table WHERE condition
  INSERT INTO table VALUES (value1[,value2,...])
  INSERT INTO table ( column1[,column2,...] ) VALUES (value1[,value2,...])
  UPDATE table SET assignment1[,assignment2,...]
  UPDATE table SET assignment1[,assignment2,...] WHERE condition
```

## Operators available in conditions

```sh
  "="  : equal
  "<"  : smaller than
  "<=" : smaller/equal than
  ">"  : larger than
  ">=" : larger/equal than
  "<>" : not equal
  "~"  : Substring matching  (non-standard SQL)
  "%"  : Substring matching  (limited functionality)
```

Arithmetic expressions using constants and field values are allowed in
condition clauses and in the RHS of assignments.  
Usual precedence rules and bracketing (using '(' and ')') are
supported.  
Type conversion is performed if necessary (experimental).

Conditions allow boolean expressions using the AND, OR and NOT
operators, with the usual precedence rules.

NULLs can be tested by 'colname IS NULL' in conditions. The negation is
'colname NOT NULL'.

Sorting: Empty fields in a character column are sorted to the end.

## LIMITATIONS OF THE DBF DRIVER

The DBF driver supports only a **few SQL statements** since the DBF
tables are intended for simple table storage. DBF column names are
limited to 10 characters (as defined in the DBF specifications). For
example,

- aggregate functions (sum, count, min, max,...) are **not** supported
  in SELECT clauses;
- mathematic functions (sin, cos, exp, log,...) are **not** supported in
  expressions;
- SQL query with IN are **not** supported.

## ERROR MESSAGES

An error message such as:

```sh
DBMI-DBF driver error:
SQL parser error: syntax error, unexpected NAME processing 'IN'..
```

indicates that an unsupported SQL statement (here, 'IN') was used. The
only solution is to switch the DBMI backend to a real SQL engine
(SQLite, PostgreSQL, MySQL etc.). See [SQL support in GRASS
GIS](sql.md).

An error message such as:

```sh
DBMI-DBF driver error:
SQL parser error: syntax error, unexpected DESC, expecting NAME processing 'DESC'
```

indicates that a column name corresponds to a reserved SQL word (here:
'DESC'). A different column name should be used. If this happens during
import with *v.in.ogr*, the *cnames* parameter can be used to assign
different column names on the fly.

## SEE ALSO

*[db.connect](db.connect.md), [SQL support in GRASS GIS](sql.md)  
[DBF Specifications](http://shapelib.maptools.org/dbf_api.html)
(Shapelib)*

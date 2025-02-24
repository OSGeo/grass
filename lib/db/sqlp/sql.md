---
description: SQL support in GRASS GIS
---

# SQL support in GRASS GIS

Vector points, lines and areas usually have attribute data that are
stored in DBMS. The attributes are linked to each vector object using a
category number (attribute ID, usually the "cat" integer column). The
category numbers are stored both in the vector geometry and the
attribute table.

GRASS GIS supports various RDBMS ([Relational database management
system](https://en.wikipedia.org/wiki/Relational_database_management_system))
and embedded databases. SQL ([Structured Query
Language](https://en.wikipedia.org/wiki/Sql)) queries are directly
passed to the underlying database system. The set of supported SQL
commands depends on the RDMBS and database driver selected.

## Database drivers

The default database driver used by GRASS GIS 8 is SQLite. GRASS GIS
handles multiattribute vector data by default. The *db.\** set of
commands provides basic SQL support for attribute management, while the
*v.db.\** set of commands operates on vector maps.

Note: The list of available database drivers can vary in various binary
distributions of GRASS GIS:

|                           |                                                            |                                             |
|---------------------------|------------------------------------------------------------|---------------------------------------------|
| [sqlite](grass-sqlite.md) | Data storage in SQLite database files (default DB backend) | <https://sqlite.org/>                       |
| [dbf](grass-dbf.md)       | Data storage in DBF files                                  | <http://shapelib.maptools.org/dbf_api.html> |
| [pg](grass-pg.md)         | Data storage in PostgreSQL RDBMS                           | <https://postgresql.org/>                   |
| [mysql](grass-mysql.md)   | Data storage in MySQL RDBMS                                | <https://www.mysql.com/>                    |
| [odbc](grass-odbc.md)     | Data storage via UnixODBC (PostgreSQL, Oracle, etc.)       | <https://www.unixodbc.org/>                 |
| [ogr](grass-ogr.md)       | Data storage in OGR files                                  | [https://gdal.org/](https://gdal.org)       |

## NOTES

### Database table name restrictions

- No dots are allowed as SQL does not support '.' (dots) in table names.

- Supported table name characters are only:  

  ```sh
  [A-Za-z][A-Za-z0-9_]*
  ```

- A table name must start with a character, not a number.

- Text-string matching requires the text part to be 'single quoted'.
  When run from the command line multiple queries should be contained in
  "double quotes". e.g.  

  ```sh
  d.vect map where="individual='juvenile' and area='beach'"
  ```

- Attempts to use a reserved SQL word (depends on database backend) as
  column or table name will cause a "SQL syntax error".

- An error message such as "`dbmi: Protocol error`" either indicates an
  invalid column name or an unsupported column type (then the GRASS SQL
  parser needs to be extended).

- DBF column names are limited to 10 characters (DBF API definition).

### Database table column types

The supported types of columns depend on the database backend. However,
all backends should support VARCHAR, INT, DOUBLE PRECISION and DATE.

## EXAMPLES

### Display of vector feature selected by attribute query

Display all vector points except for *LAMAR* valley and *extensive
trapping* (brackets are superfluous in this example):

```sh
g.region vector=schools_wake -p
d.mon wx0
d.vect roadsmajor

# all schools
d.vect schools_wake fcol=black icon=basic/diamond col=white size=13

# numerical selection: show schools with capacity of above 1000 kids:
d.vect schools_wake fcol=blue icon=basic/diamond col=white size=13 \
    where="CAPACITYTO > 1000"

# string selection: all schools outside of Raleigh
#   along with higher level schools in Raleigh
d.vect schools_wake fcol=red icon=basic/diamond col=white size=13 \
    where="ADDRCITY <> 'Raleigh' OR (ADDRCITY = 'Raleigh' AND GLEVEL = 'H')"
```

Select all attributes from table where *CORECAPACI* column values are
smaller than 200 (children):

```sh
# must be run from the mapset which contains the table
echo "SELECT * FROM schools_wake WHERE CORECAPACI < 200" | db.select input=-
```

Example of subquery expressions from a list (not supported for DBF
driver):

```sh
v.db.select schools_wake where="ADDRCITY IN ('Apex', 'Wendell')"
```

### Example of pattern matching

```sh
# field contains string:
#  for DBF driver:
v.extract schools_wake out=elementary_schools where="NAMELONG LIKE 'ELEM'"
#  for SQLite driver:
v.extract schools_wake out=rivers_noce where="DES LIKE '%NOCE%'"
v.extract schools_wake out=elementary_schools where="NAMELONG LIKE '%ELEM%'"

# match exactly number of characters (here: 2), does not work for DBF driver:
v.db.select mysites where="id LIKE 'P__'"

#define wildcard:
v.db.select mysites where="id LIKE 'P%'"
```

### Example of null handling

```sh
v.db.addcolumn map=roads col="nulltest int"
v.db.update map=roads col=nulltest value=1 where="cat > 2"
d.vect roads where="nulltest is null"
v.db.update map=roads col=nulltest value=2 where="cat <= 2"
```

### Update of attributes

Examples of complex expressions in updates (using `v.db.*` modules):

```sh
v.db.addcolumn map=roads column="exprtest double precision"
v.db.update map=roads column=exprtest value="cat/nulltest"
v.db.update map=roads column=exprtest value="cat/nulltest+cat" where="cat=1"

# using data from another column
v.db.update map=roads column=exprtest qcolumn="(cat*100.)/SHAPE_LEN."
```

Examples of more complex expressions in updates (using `db.*` modules):

```sh
echo "UPDATE roads SET exprtest=null"
echo "UPDATE roads SET exprtest=cat/2" | db.execute
echo "UPDATE roads SET exprtest=cat/2+cat/3" | db.execute
echo "UPDATE roads SET exprtest=NULL WHERE cat>2" | db.execute
echo "UPDATE roads SET exprtest=cat/3*(cat+1) WHERE exprtest IS NULL" | db.execute"
```

Instead of creating and updating new columns with an expression, you can
use the expression directly in a command:

```sh
d.vect roads where="(cat/3*(cat+1))>8"
d.vect roads where="cat>exprtest"
```

### Example of changing a SQL type (type casting)

*Note: not supported for [DBF driver](grass-dbf.md).*

North Carolina data set: convert string column to double precision:

```sh
# first copy map into current mapset
g.copy vect=geodetic_pts,mygeodetic_pts
v.db.addcolumn mygeodetic_pts col="zval double precision"

# the 'z_value' col contains 'N/A' strings, not to be converted
v.db.update mygeodetic_pts col=zval \
            qcol="CAST(z_value AS double precision)" \
            where="z_value <> 'N/A'"
```

### Example of concatenation of fields

*Note: not supported for [DBF driver](grass-dbf.md).*

```sh
v.db.update vectormap column=column3 qcolumn="column1 || column2"
```

### Example of conditions

Conditions (like if statements) are usually written as CASE statement in
SQL:

```sh
v.db.update vectormap column=species qcolumn="CASE WHEN col1 >= 12 THEN cat else NULL end"

# a more complex example with nested conditions
v.db.update vectormap column=species qcolumn="CASE WHEN col1 >= 1 THEN cat WHEN row = 13 then 0 ELSE NULL end"
```

## SEE ALSO

*[db.connect](db.connect.md), [db.select](db.select.md),
[db.execute](db.execute.md), [v.db.connect](v.db.connect.md),
[v.db.select](v.db.select.md), [v.db.update](v.db.update.md)*

[Database management in GRASS GIS](databaseintro.md), [Help pages for
database modules](database.md)

## AUTHOR

Radim Blazek

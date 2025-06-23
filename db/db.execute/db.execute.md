## DESCRIPTION

*db.execute* allows the user to execute SQL statements.

## NOTES

*db.execute* only executes SQL statements and does not return any data.
If you need data returned from the database, use
*[db.select](db.select.md)*.

If parameters for database connection are already set with
*[db.connect](db.connect.md)*, they are taken as default values and do
not need to be specified each time.

If you have a large number of SQL commands to process, it is much faster
to place all the SQL statements into a text file and use **input** file
parameter than it is to process each statement individually in a loop.
If multiple instruction lines are given, each SQL line must end with a
semicolon.

Please see the individual *[GRASS SQL interface](sql.md)* for how to
create a new database.

## EXAMPLES

Create a new table with columns 'cat' and 'soiltype':

```sh
db.execute sql="CREATE TABLE soils (cat integer, soiltype varchar(10))"
```

Create a new table using a file with SQL statements

```sh
db.execute driver=odbc database=grassdb input=file.sql
```

Insert new row into attribute table:

```sh
db.execute sql="INSERT INTO mysites (id,name,east,north) values (30,'Ala',1657340,5072301)"
```

Update attribute entries to new value based on SQL rule:

```sh
db.execute sql="UPDATE roads SET travelcost=5 WHERE cat=1"
```

Update attribute entries to new value based on SQL rule:

```sh
db.execute sql="UPDATE dourokukan SET testc=50 WHERE testc is NULL"
```

Delete selected rows from attribute table:

```sh
db.execute sql="DELETE FROM gsod_stationlist WHERE latitude < -91"
```

Add new column to attribute table:

```sh
db.execute sql="ALTER TABLE roads ADD COLUMN length double"
```

Column type conversion - update new column from existing column (all
drivers except for DBF):

```sh
# 'z_value' is varchar and 'z' is double precision:
echo "UPDATE geodetic_pts SET z = CAST(z_value AS numeric)" | db.execute input=-
```

Drop column from attribute table:

```sh
db.execute sql="ALTER TABLE roads DROP COLUMN length"
```

Drop table (not supported by all drivers):

```sh
db.execute sql="DROP TABLE fmacopy"
```

Update attribute with multiple SQL instructions in file (e.g.,
`file.sql`, instruction line must end with a semicolon):

```sh
UPDATE roads SET travelcost=5 WHERE cat=1;
UPDATE roads SET travelcost=2 WHERE cat=2;

db.execute input=file.sql
```

Join table 'myroads' to table 'extratab' based on common 'cat' column
values (not supported by DBF driver):

```sh
db.execute sql="UPDATE extratab SET names=(SELECT label FROM myroads WHERE extratab.cat=myroads.cat)"
```

## SEE ALSO

*[db.columns](db.columns.md), [db.describe](db.describe.md),
[db.drivers](db.drivers.md), [db.droptable](db.droptable.md),
[db.login](db.login.md), [db.select](db.select.md),
[db.tables](db.tables.md),*

*[GRASS SQL interface](sql.md)*

## AUTHOR

CERL

## DESCRIPTION

*db.select* prints result of selection from database based on SQL
statement read from input file or from standard input to standard
output. Each individual query has to be written on one single line and
different queries have to be written on separate lines.

## NOTE

If parameters for database connection are already set with
*[db.connect](db.connect.html)*, they are taken as default values and do
not need to be specified each time. Output will be displayed to standard
output or can be directed to a file (option **output**).

## EXAMPLES

### Basic usage

::: code
    db.select sql="select * from roads"
:::

or

::: code
    echo "select * from roads" | db.select input=-
:::

or

::: code
    db.select input=file.sql
:::

or

::: code
    cat file.sql | db.select input=-
:::

Select all from table roads:

::: code
    db.select -c driver=odbc database=mydb table=hospitals \
              input=file.sql output=result.csv
:::

Select some string attribute, exclude others:

::: code
    db.select sql="SELECT * FROM archsites WHERE str1 <> 'No Name'"
:::

Select some string attribute with ZERO length:

::: code
    db.select sql="SELECT * FROM archsites WHERE str1 IS NULL"
:::

Select coordinates from PostGIS table:

::: code
    db.select sql="SELECT x(geo),y(geo) FROM localizzazione"
:::

### Execute multiple SQL statements

::: code
    cat file.sql
    SELECT * FROM busstopsall WHERE cat = 1
    SELECT cat FROM busstopsall WHERE cat > 4 AND cat < 8

    db.select input=file.sql
:::

### Count number of cases falling into same position

When multiple observation have the spatial coordinates, they can still
be counted (if needed, coordinates can be uploaded to the attribute
table by *v.to.db*:

::: code
    db.select sql="SELECT long,lat,site_id,department,obs,COUNT(long) as count_cases \
                   FROM diseases GROUP BY long,lat"
:::

## SEE ALSO

*[db.connect](db.connect.html), [db.describe](db.describe.html),
[db.drivers](db.drivers.html), [db.droptable](db.droptable.html),
[db.execute](db.execute.html), [db.login](db.login.html),
[db.tables](db.tables.html)*

*[GRASS SQL interface](sql.html)*

## AUTHORS

Original author unknown (probably CERL)\
Modifications by Radim Blazek, ITC-Irst, Trento, Italy\
Support for multiple statements by Martin Landa, Czech Technical
University in Prague

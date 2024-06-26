## DESCRIPTION

*db.in.ogr* imports attribute tables in various formats as supported by
the [OGR library](http://www.gdal.org/) on the local system (DBF, CSV,
PostgreSQL, SQLite, MySQL, ODBC, etc.). Optionally a unique **key** (ID)
column can be added to the table.

## EXAMPLES

### Import CSV file

Limited type recognition can be done for Integer, Real, String, Date,
Time and DateTime columns through a descriptive file with same name as
the CSV file, but .csvt extension (see details
[here](http://www.gdal.org/drv_csv.html)).

::: code
    # NOTE: create koeppen_gridcode.csvt first for automated type recognition
    db.in.ogr input=koeppen_gridcode.csv output=koeppen_gridcode
    db.select table=koeppen_gridcode
:::

### Import DBF table

Import of a DBF table with additional unique key column (e.g., needed
for *[v.in.db](v.in.db.html)*).

::: code
    db.in.ogr input=/path/to/mydata.dbf output=census_raleigh key=myid
    db.describe -c census_raleigh
:::

### Import of a SQLite table

::: code
    db.in.ogr input=/path/to/sqlite.db db_table=census_raleigh output=census_raleigh
:::

### Import of a PostgreSQL table

::: code
    # HINT: if the database contains spatial tables, but you want to import a non-spatial
    table, set the environmental variable PG_LIST_ALL_TABLES to YES before importing

    db.in.ogr input="PG:host=localhost dbname=ecad user=neteler" \
              db_table=ecad_verona_tmean output=ecad_verona_tmean
    db.select table=ecad_verona_tmean
    db.describe -c ecad_verona_tmean
:::

### Import XLS file

To force reading headers, define environmental variable
`OGR_XLS_HEADERS='FORCE'`. Parameter **db_table** refers to the list
within XLS file.

::: code
    export OGR_XLS_HEADERS='FORCE'
    db.in.ogr input=address.xls db_table=address_data
:::

## SEE ALSO

*[db.select](db.select.html), [v.in.ogr](v.in.ogr.html),
[v.in.db](v.in.db.html)*

[GRASS SQL interface](sql.html)

## AUTHOR

Markus Neteler

## DESCRIPTION

*db.in.ogr* imports attribute tables in various formats as supported by
the [OGR library](https://gdal.org/) on the local system (DBF, CSV,
PostgreSQL, SQLite, MySQL, ODBC, etc.). Optionally a unique **key** (ID)
column can be added to the table.

## EXAMPLES

### Import CSV file

Limited type recognition can be done for Integer, Real, String, Date,
Time and DateTime columns through a descriptive file with same name as
the CSV file, but .csvt extension (see details
[here](https://gdal.org/en/stable/drivers/vector/csv.html)).

```sh
# NOTE: create koeppen_gridcode.csvt first for automated type recognition
db.in.ogr input=koeppen_gridcode.csv output=koeppen_gridcode gdal_doo="AUTODETECT_TYPE=YES"
db.describe koeppen_gridcode -c
db.select table=koeppen_gridcode
```

### Import DBF table

Import of a DBF table with additional unique key column (e.g., needed
for *[v.in.db](v.in.db.md)*).

```sh
db.in.ogr input=/path/to/mydata.dbf output=census_raleigh key=myid
db.describe -c census_raleigh
```

### Import of a SQLite table

```sh
db.in.ogr input=/path/to/sqlite.db db_table=census_raleigh output=census_raleigh
```

### Import of a PostgreSQL table

```sh
# HINT: if the database contains spatial tables, but you want to import a non-spatial
table, set the environmental variable PG_LIST_ALL_TABLES to YES before importing

db.in.ogr input="PG:host=localhost dbname=ecad user=neteler" \
          db_table=ecad_verona_tmean output=ecad_verona_tmean
db.select table=ecad_verona_tmean
db.describe -c ecad_verona_tmean
```

### Import XLS file

To force reading headers, define environmental variable
`OGR_XLS_HEADERS='FORCE'`. Parameter **db_table** refers to the list
within XLS file.

```sh
export OGR_XLS_HEADERS='FORCE'
db.in.ogr input=address.xls db_table=address_data
```

## SEE ALSO

*[db.select](db.select.md), [v.in.ogr](v.in.ogr.md),
[v.in.db](v.in.db.md)*

[GRASS SQL interface](sql.md)

## AUTHOR

Markus Neteler

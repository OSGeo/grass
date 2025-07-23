## DESCRIPTION

*db.out.ogr* exports GRASS GIS attribute tables into various formats as
supported by the OGR driver on the local system (CSV, DBF, PostgreSQL,
SQLite, MySQL, ODBC, etc.).

The *output* parameter is used to define the output file name (if the
path is not defined, the module will attempt to write to the current
directory). In case of a database connection as output, the connection
string has to be specified.

The *layer* parameter is needed if the attribute table to be exported is
linked as non-default layer to a vector map.

## EXAMPLES

### Export of GRASS GIS attribute table to a CSV table file (default format)

```sh
db.out.ogr input=precip_30ynormals output=precip_30ynormals.csv
```

### Export of a GRASS GIS attribute table to a DBF table

```sh
db.out.ogr input=precip_30ynormals output=precip_30ynormals.dbf format=DBF
```

### Export of GRASS GIS attribute table into a PostgreSQL table

```sh
db.out.ogr input=precip_30ynormals \
           output="PG:host=localhost dbname=meteo user=neteler" \
           format=PostgreSQL
# verify
echo "SELECT * FROM precip_30ynormals" | psql meteo
```

## SEE ALSO

*[db.tables](db.tables.md)*

*[GRASS SQL interface](sql.md)*

## AUTHORS

Markus Neteler  
Converted to Python by Glynn Clements

## DESCRIPTION

*v.in.db* creates a new vector point map from database table or file
containing coordinates.

## NOTES

If GRASS comes with [OGR](grass-ogr.md) support than *v.in.db* allows
importing data from different input files, eg. CSV or MS Excel (assuming
that GDAL/OGR library is compiled with this support).

*v.in.db* creates key column "cat" automatically when **key** option is
not given. Note that this operation is possible to perform only when
**-t** flag is not given. Currently, automated creation of key column is
supported only when default DB driver for output vector map is [SQLite
driver](grass-sqlite.md) otherwise **key** option must be specified by
the user. Default DB driver is defined by *[db.connect](db.connect.md)*.

## EXAMPLES

### Creating a map from PostgreSQL table

```sh
v.in.db driver=pg database="host=myserver.itc.it,dbname=mydb" \
        table=pat_stazioni x=east y=north z=quota key=id output=pat_stazioni
```

If an ID column is not present in the PostgreSQL table, a new column
should be added. See [PostgreSQL DB driver](grass-pg.md) page for
details.

### Creating a map from PostGIS

To extract coordinate values from PostGIS, functions have to be used:

```sh
v.in.db driver=pg database="host=myserver.itc.it,dbname=mydb" \
        table=station x="x(geom)" y="y(geom)" z="z(geom)" key=id out=meteostations
```

If an ID column is not present in the PostgreSQL table, a new column
should be added. See [PostgreSQL DB driver](grass-pg.md) page for
details.

Alternatively a vector point map can be imported from PostGIS database
using *[v.in.ogr](v.in.ogr.md)*.

### Creating a map from Open Document spreadsheet (ODS) file

A new vector point map is created from given sheet in ODS file. The
**database** option points to the ODS file. Option **table** is the name
of selected spreadsheet list, the **key** option is the identifier
column:

```sh
# preview table structure with OGR tool (table name is "Layer name" here):
ogrinfo -al -so meteodata.ods

# import sheet from ODS into map
v.in.db key=ID table=mysheet x=long y=lat z=height output=meteodata \
         driver=ogr database=meteodata.ods
```

### Creating a map from MS Excel file

A new vector point map is created from given sheet in MS Excel file. The
**database** option points to the file in MS Excel format. Option
**table** is name of the selected spreadsheet "List1":

```sh
v.in.db table=List1 x=long y=lat z=height output=meteodata \
         driver=ogr database=meteodata.xls
```

Note that in this example the **key** option is omitted. In this case
*v.in.db* tries to add key column automatically. This requires
[SQLite](grass-sqlite.md) to be a default DB driver.

### Creating a map from DBF table

A new 3D point vector map is created from DBF table. Column 'idcol'
contains unique row IDs. The **database** option is the directory where
the DBF file is stored.

```sh
v.in.db driver=dbf database=/home/user/tables/ table=pointsfile x=x y=y z=z \
        key=idcol out=dtmpoints
```

To check result:

```sh
v.info dtmpoints
v.info -c dtmpoints
```

If DB driver for output vector map is different from SQLite driver and
an ID column is missing in the DBF file, it has to be added beforehand,
e.g. with OpenOffice. Alternatively, import the table with
*[db.in.ogr](db.in.ogr.md)* into GRASS and then with *v.in.db* from the
imported table (*[db.in.ogr](db.in.ogr.md)* optionally adds an unique ID
column).

### Creating a point map from DBF table for selected records only

The user can import only selected vector points from a table using the
**where** parameter (see above for general DBF handling):

```sh
v.in.db driver=dbf  database=/home/user/tables/ table=pointsfile x=x y=y z=z \
        key=idcol out=dtmpoints where="x NOT NULL and z > 100"
```

### Creating a map from SQLite table

A new vector point map is created from table in SQLite database file.
Column 'idcol' contains unique row IDs. The **database** option is the
the SQLite database file.

```sh
v.in.db driver=sqlite database=/home/user/tables/mysqlite.db table=pointsfile x=x y=y z=z \
        key=idcol out=dtmpoints
```

## SEE ALSO

*[db.execute](db.execute.md), [db.in.ogr](db.in.ogr.md),
[v.info](v.info.md), [v.in.geonames](v.in.geonames.md),
[v.in.ogr](v.in.ogr.md), [v.to.db](v.to.db.md)*

[SQL support in GRASS GIS](sql.md)

## AUTHORS

Radim Blazek  
Various updates for GRASS 7 by Martin Landa, Czech Technical University
in Prague, Czech Republic

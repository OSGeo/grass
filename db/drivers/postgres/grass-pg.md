---
description: PostgreSQL DATABASE DRIVER
---

# PostgreSQL DATABASE DRIVER

PostgreSQL database driver enables GRASS to store vector attributes in
PostgreSQL server.

## Creating a PostgreSQL database

A new database is created with `createdb`, see the [PostgreSQL
manual](https://www.postgresql.org/docs/manuals/) for details.

## Connecting GRASS to PostgreSQL

```sh
# example for connecting to a PostgreSQL server:
db.connect driver=pg database=mydb
db.login user=myname password=secret host=myserver.osgeo.org  # port=5432
db.connect -p
db.tables -p
```

### Username and password

From the [PostgresQL
manual](https://www.postgresql.org/docs/10/static/libpq-pgpass.html):

The file *.pgpass* in a user's home directory can contain passwords to
be used if the connection requires a password (and no password has been
specified otherwise). On Microsoft Windows the file is named
*%APPDATA%\postgresql\pgpass.conf* (where *%APPDATA%* refers to the
Application Data subdirectory in the user's profile). Alternatively, a
password file can be specified using the connection parameter passfile
or the environment variable PGPASSFILE. This file should contain lines
of the following format:

```sh
hostname:port:database:username:password
```

## Supported SQL commands

All SQL commands supported by PostgreSQL. It's not possible to use
C-like escapes (with backslash like \n etc) within the SQL syntax.

## Operators available in conditions

All SQL operators supported by PostgreSQL.

## Adding an unique ID column

Import vector module require an unique ID column which can be generated
as follows in a PostgreSQL table:

```sh
db.execute sql="ALTER TABLE mytable ADD ID integer"
db.execute sql="CREATE SEQUENCE mytable_seq"
db.execute sql="UPDATE mytable SET ID = nextval('mytable_seq')"
db.execute sql="DROP SEQUENCE mytable_seq"
```

## Attribute import into PostgreSQL

CSV import into PostgreSQL:

```sh
\h copy
COPY t1 FROM 'filename' USING DELIMITERS ',';
```

## Geometry import from PostgreSQL table into GRASS

*[v.in.db](v.in.db.md)* creates a new vector (points) map from a
database table containing coordinates. See [here](v.in.db.md) for
examples.

## PostGIS: PostgreSQL with vector geometry

[PostGIS](https://postgis.net/): adds geographic object support to
PostgreSQL.

### Example: Import from PostGIS

In an existing PostGIS database, create the following table:

```sh
CREATE TABLE test
(
 id serial NOT NULL,
 mytime timestamp DEFAULT now(),
 text varchar,
 wkb_geometry geometry,
 CONSTRAINT test_pkey PRIMARY KEY (id)
) WITHOUT OIDS;

# insert value
INSERT INTO test (text, wkb_geometry)
 VALUES ('Name',geometryFromText('POLYGON((600000 200000,650000
 200000,650000 250000,600000 250000,600000 200000))',-1));

# register geometry column
select AddGeometryColumn ('postgis', 'test', 'geometry', -1, 'GEOMETRY', 2);
```

GRASS can import this PostGIS polygon map as follows:

```sh
v.in.ogr input="PG:host=localhost dbname=postgis user=neteler" layer=test \
         output=test type=boundary,centroid
v.db.select test
v.info -t test
```

#### Geometry Converters

- [PostGIS with shp2pgsql](https://postgis.net/workshops/postgis-intro/loading_data.html#loading-with-shp2pgsql):
  `shp2pgsql -D lakespy2 lakespy2 test > lakespy2.sql`
- [e00pg](https://e00pg.sourceforge.net/): E00 to PostGIS filter, see
  also *[v.in.e00](v.in.e00.md)*.
- GDAL/OGR [ogrinfo and ogr2ogr](https://gdal.org/): GIS vector format
  converter and library, e.g. ArcInfo or SHAPE to PostGIS.  
  `ogr2ogr -f "PostgreSQL" shapefile ??`

## SEE ALSO

*[db.connect](db.connect.md), [db.execute](db.execute.md)*

[Database management in GRASS GIS](databaseintro.md)  
[Help pages for database modules](database.md)  
[SQL support in GRASS GIS](sql.md)  

## REFERENCES

- [PostgreSQL web site](https://www.postgresql.org/)
- [pgAdmin graphical user interface](https://www.pgadmin.org/)
- [GDAL/OGR PostgreSQL driver
  documentation](https://gdal.org/en/stable/drivers/vector/pg.html)

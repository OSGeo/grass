PostgreSQL database driver enables GRASS to store vector attributes in
PostgreSQL server.

## Creating a PostgreSQL database

A new database is created with `createdb`, see the [PostgreSQL
manual](http://www.postgresql.org/docs/manuals/) for details.

## Connecting GRASS to PostgreSQL

```
# example for connecting to a PostgreSQL server:
db.connect driver=pg database=mydb
db.login user=myname password=secret host=myserver.osgeo.org  # port=5432
db.connect -p
db.tables -p
```

### Username and password

From the [PostgresQL
manual](https://www.postgresql.org/docs/10/static/libpq-pgpass.html):

The file *.pgpass* in a user\'s home directory can contain passwords to
be used if the connection requires a password (and no password has been
specified otherwise). On Microsoft Windows the file is named
*%APPDATA%\\postgresql\\pgpass.conf* (where *%APPDATA%* refers to the
Application Data subdirectory in the user\'s profile). Alternatively, a
password file can be specified using the connection parameter passfile
or the environment variable PGPASSFILE. This file should contain lines
of the following format:

```
hostname:port:database:username:password
```

## Supported SQL commands

All SQL commands supported by PostgreSQL. It\'s not possible to use
C-like escapes (with backslash like \\n etc) within the SQL syntax.

## Operators available in conditions

All SQL operators supported by PostgreSQL.

## Adding an unique ID column

Import vector module require an unique ID column which can be generated
as follows in a PostgreSQL table:

```
db.execute sql="ALTER TABLE mytable ADD ID integer"
db.execute sql="CREATE SEQUENCE mytable_seq"
db.execute sql="UPDATE mytable SET ID = nextval('mytable_seq')"
db.execute sql="DROP SEQUENCE mytable_seq"
```

## Attribute import into PostgreSQL

CSV import into PostgreSQL:

```
\h copy
COPY t1 FROM 'filename' USING DELIMITERS ',';
```

## Geometry import from PostgreSQL table into GRASS

*[v.in.db](v.in.db.html)* creates a new vector (points) map from a
database table containing coordinates. See [here](v.in.db.html) for
examples.

## PostGIS: PostgreSQL with vector geometry

[PostGIS](http://postgis.refractions.net/): adds geographic object
support to PostgreSQL.

### Example: Import from PostGIS

In an existing PostGIS database, create the following table:

```
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

```
v.in.ogr input="PG:host=localhost dbname=postgis user=neteler" layer=test \
         output=test type=boundary,centroid
v.db.select test
v.info -t test
```

#### Geometry Converters

-   [PostGIS with shp2pgsql](http://postgis.refractions.net/download/):\
    `shp2pgsql -D lakespy2 lakespy2 test > lakespy2.sql`
-   [e00pg](http://e00pg.sourceforge.net/): E00 to PostGIS filter, see
    also *[v.in.e00](v.in.e00.html)*.
-   GDAL/OGR [ogrinfo and ogr2ogr](http://www.gdal.org/): GIS vector
    format converter and library, e.g. ArcInfo or SHAPE to PostGIS.\
    `ogr2ogr -f "PostgreSQL" shapefile ??`

## SEE ALSO

*[db.connect](db.connect.html), [db.execute](db.execute.html)*

[Database management in GRASS GIS](databaseintro.html)\
[Help pages for database modules](database.html)\
[SQL support in GRASS GIS](sql.html)\

## REFERENCES

-   [PostgreSQL web site](http://www.postgresql.org/)
-   [pgAdmin graphical user interface](http://www.pgadmin.org/)
-   [GDAL/OGR PostgreSQL driver
    documentation](http://www.gdal.org/drv_pg.html)

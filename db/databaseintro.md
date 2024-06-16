### Attribute management in general

GRASS can be linked to one or many database management systems (DBMS).
The *db.\** set of commands provides basic SQL support for attribute
management, while the *v.db.\** set of commands operates on the vector
map (see [Vector introduction](vectorintro.html)).

### Available drivers

Available drivers are listed in [SQL support in GRASS GIS](sql.html).

**Notes**:\
The DBF driver provides only very limited SQL support (as DBF is not an
SQL DB) while the other DBMS backends (such as SQLite, PostgreSQL, MySQL
etc) provide full SQL support since the SQL commands are sent directly
to the DBMS. For this reason, the SQLite driver is the default DBMI
backend.

### DB connection management

The current database management settings are shown or modified with
[db.connect](db.connect.html) for current mapset. Available DBMI drivers
are listed with [db.drivers](db.drivers.html). Some DBMI backends
require a user/password for driver/database to be set with
[db.login](db.login.html). In order to test a driver, run
[db.test](db.test.html).

### Attribute data import and export

Attribute data can be imported with [db.in.ogr](db.in.ogr.html) from
various formats and exported with [db.out.ogr](db.out.ogr.html). To
internally copy a a full table or selectively parts of it, use
[db.copy](db.copy.html).

Further conversion tools:

-   [MDB Tools](http://sourceforge.net/projects/mdbtools): Convert
    MS-Access data to SQL, DBF, etc.
-   [Using OpenOffice.org with SQL
    Databases](https://grasswiki.osgeo.org/wiki/Openoffice.org_with_SQL_Databases)

### SQL commands

GRASS supports two main SQL operations, execution of an SQL statement
([db.execute](db.execute.html)) and selection of data from a table
([db.select](db.select.html)). See the [SQL help page](sql.html) for
examples.

### Managing the default DBMI settings

Per default vector map attributes are stored in SQLite tables. This
default definition can be modified with [db.connect](db.connect.html).
If an external DBMS is used, [db.login](db.login.html) may be required.

### Creating a database

Specific commands are explained on the individual driver pages (these
pages are only available if driver was compiled in this installation):

-   DBF: see [DBF](grass-dbf.html) page
-   SQLite: [SQLite](grass-sqlite.html) page
-   mySQL: [mySQL](grass-mysql.html) and [meSQL](grass-mesql.html) pages
-   ODBC: [ODBC](grass-odbc.html) page (connect to Oracle, etc.)
-   PostgreSQL: [PostgreSQL](grass-pg.html) and PostGIS page

### Metadata

All columns for a given table are listed with
[db.columns](db.columns.html). The command
[db.describe](db.describe.html) describes a table in detail. To list all
available tables for a given database, run [db.tables](db.tables.html).

### Table maintenance

To drop a column from a selected attribute table, use
[db.dropcolumn](db.dropcolumn.html). With
[db.droptable](db.droptable.html) an attribute table can be deleted.

### Database Schema

Currently schema support only works for PostgreSQL connections. Default
schema can be set with [db.connect](db.connect.html). Note that the
default schema will be used by all db.\* modules.

[db.tables](db.tables.html) returns \'schema.table\' if schemas are
available in the database.

### Migrating to a different database engine

To migrate a GRASS database table (or a GRASS vector map) to a different
DBMI engine, the best solution is to create a new MAPSET, define the
DBMI settings accordingly with [db.connect](db.connect.html) and if
needed, [db.login](db.login.html). Then the table of interest can be
copied over with [db.copy](db.copy.html) from the original MAPSET.
Likewise, a vector map including its table(s) are copied from the
original MAPSET to the current MAPSET with [g.copy](g.copy.html).

### See also

-   [Introduction into raster data processing](rasterintro.html)
-   [Introduction into 3D raster data (voxel)
    processing](raster3dintro.html)
-   [Introduction into vector data processing](vectorintro.html)
-   [Introduction into image processing](imageryintro.html)
-   [Introduction into temporal data processing](temporalintro.html)
-   [Projections and spatial transformations](projectionintro.html)

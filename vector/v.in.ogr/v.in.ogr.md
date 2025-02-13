## DESCRIPTION

*v.in.ogr* imports vector data from files and database connections
supported by the [OGR](https://gdal.org/) library) into the current
project (previously called location) and mapset.

If the **layer** parameter is not given, all available OGR layers are
imported as separate GRASS layers into one GRASS vector map. If several
OGR layer names are given, all these layers are imported as separate
GRASS layers into one GRASS vector map.

The optional **spatial** parameter defines spatial query extents. This
parameter allows the user to restrict the region to a spatial subset
while importing the data. All vector features completely or partially
falling into this rectangle subregion are imported. The **-r** current
region flag is identical, but uses the current region settings as the
spatial bounds (see *[g.region](g.region.md)*).

### Supported Vector Formats

*v.in.ogr* uses the OGR library which supports various vector data
formats including [ESRI
Shapefile](https://gdal.org/en/stable/drivers/vector/shapefile.html),
[Mapinfo File](https://gdal.org/en/stable/drivers/vector/mitab.html), UK
.NTF, SDTS, TIGER, IHO S-57 (ENC), DGN, GML, GPX, AVCBin, REC, Memory,
OGDI, and PostgreSQL, depending on the local OGR installation. For
details see the [OGR format
overview](https://gdal.org/en/stable/drivers/vector/). The **-f** prints
a list of the vector formats supported by the system's OGR (Simple
Features Library). The OGR (Simple Features Library) is part of the
[GDAL](https://gdal.org) library, hence GDAL needs to be installed to
use *v.in.ogr*.

The list of actually supported formats can be printed by **-f** flag.

### Topology cleaning

Topology cleaning on areas is automatically performed, but may fail in
special cases. In these cases, a **snap** threshold value is estimated
from the imported vector data and printed out at the end. The vector
data can then be imported again with the suggested **snap** threshold
value which is incremented by powers of 10 until either an estimated
upper limit for the threshold value is reached or the topology cleaning
on areas was successful. In some cases, manual cleaning might be
required or areas are truly overlapping, e.g. buffers created with
non-topological software.

The **min_area** threshold value is being specified as area size in map
units with the exception of latitude-longitude projects in which it is
being specified solely in square meters.

The **snap** threshold value is used to snap boundary vertices to each
other if the distance in map units between two vertices is not larger
than the threshold. Snapping is by default disabled with -1. See also
the *[v.clean](v.clean.md)* manual.

### Overlapping polygons

When importing overlapping polygons, the overlapping parts will become
new areas with multiple categories, one unique category for each
original polygon. An original polygon will thus be converted to multiple
areas with the same shared category. These multiple areas will therefore
also link to the same entry in the attribute table. A single category
value may thus refer to multiple non-overlapping areas which together
represent the original polygon overlapping with another polygon. The
original polygon can be recovered by using *[v.extract](v.extract.md)*
with the desired category value or **where** statement and the **-d**
flag to dissolve common boundaries.

## Project Creation

*v.in.ogr* attempts to preserve coordinate reference system (CRS)
information when importing datasets if the source format includes such
information, and if the OGR driver supports it. If the CRS of the source
dataset does not match the CRS of the current project *v.in.ogr* will
report an error message ("Coordinate reference system of dataset does
not appear to match current project").

If the user wishes to ignore the difference between the coordinate
system of the source data and the current project, they may pass the
**-o** flag to override the CRS check.

If the user wishes to import the data with the full CRS definition, it
is possible to have *v.in.ogr* automatically create a new project based
on the CRS and extents of the file being read. This is accomplished by
passing the name to be used for the new project via the **project**
parameter. Upon completion of the command, a new project will have been
created (with only a PERMANENT mapset), and the vector map will have
been imported with the indicated **output** name into the PERMANENT
mapset.

An interesting wrapper command around *v.in.ogr* is
[v.import](v.import.md) which reprojects (if needed) the vector dataset
during import to the CRS of the current project.

## NOTES

### Table column names: supported characters

The characters which are eligible for table column names are limited by
the SQL standard. Supported are:

```sh
[A-Za-z][A-Za-z0-9_]*
```

This means that SQL neither supports '.' (dots) nor '-' (minus) nor '#'
in table column names. Also a table name must start with a character,
not a number.

*v.in.ogr* converts '.', '-' and '#' to '\_' (underscore) during import.
The **-w** flag changes capital column names to lowercase characters as
a convenience for SQL usage (lowercase column names avoid the need to
quote them if the attribute table is stored in a SQL DBMS such as
PostgreSQL). The **columns** parameter is used to define new column
names during import.

The DBF database specification limits column names to 10 characters. If
the default DB is set to DBF and the input data contains longer
column/field names, they will be truncated. If this results in multiple
columns with the same name then *v.in.ogr* will produce an error. In
this case you will either have to modify the input data or use
*v.in.ogr*'s **columns** parameter to rename columns to something
unique. (hint: copy and modify the list given with the error message).
Alternatively, change the local DB with *[db.connect](db.connect.md)*.

### File encoding

When importing ESRI Shapefiles the OGR library tries to read the
LDID/codepage setting from the .dbf file and use it to translate string
fields to UTF-8. LDID "87 / 0x57" is treated as ISO8859_1 which may not
be appropriate for many languages. Unfortunately it is not clear what
other values may be appropriate (see example below). To change encoding
the user can set up
[`SHAPE_ENCODING`](https://gdal.org/en/stable/user/configoptions.html)
environmental variable or simply to define encoding value using
**encoding** parameter.

Value for **encoding** also affects text recoding when importing DXF
files. For other formats has encoding value no effect.

### Defining the key column

Option **key** specifies the column name used for feature categories.
This column must be integer. If not specified, categories numbers are
generated starting with 1 and stored in the column named "cat".

### Supports of multiple geometry columns

Starting with GDAL 1.11 the library supports multiple geometry columns
in OGR. By default *v.in.ogr* reads all geometry columns from given
layer. The user can choose desired geometry column by **geometry**
option, see [example below](#multiple-geometry-columns).

### Latitude-longitude data: Vector postprocessing after import

For vector data like a grid, horizontal lines need to be broken at their
intersections with vertical lines (**v.clean ... tool=break**).

## EXAMPLES

The command imports various vector formats:

### SHAPE files

```sh
v.in.ogr input=/home/user/shape_data/test_shape.shp output=grass_map
```

Alternate method:

```sh
v.in.ogr input=/home/user/shape_data layer=test_shape output=grass_map
```

Define encoding value for attribute data (in this example we expect
attribute data in
[Windows-1250](https://en.wikipedia.org/wiki/Windows-1250) encoding; ie.
in Central/Eastern European languages that use Latin script, Microsoft
Windows encoding).

```sh
v.in.ogr input=/home/user/shape_data/test_shape.shp output=grass_map encoding=cp1250
```

### MapInfo files

```sh
v.in.ogr input=./ layer=mapinfo_test output=grass_map
```

### Arc Coverage

We import the Arcs and Label points, the module takes care to build
areas.

```sh
v.in.ogr input=gemeinden layer=LAB,ARC type=centroid,boundary output=mymap
```

### E00 file

See also *[v.in.e00](v.in.e00.md)*.

First we have to convert the E00 file to an Arc Coverage with
'avcimport' ([AVCE00
tools](http://avce00.maptools.org/avce00/index.html), use *e00conv*
first in case that *avcimport* fails):

```sh
avcimport e00file coverage
v.in.ogr input=coverage layer=LAB,ARC type=centroid,boundary output=mymap
```

### SDTS files

You have to select the CATD file.

```sh
v.in.ogr input=CITXCATD.DDF output=cities
```

### TIGER files

```sh
v.in.ogr input=input/2000/56015/ layer=CompleteChain,PIP output=t56015_all \
type=boundary,centroid snap=-1
```

### PostGIS tables

Import polygons as areas:

```sh
v.in.ogr input="PG:host=localhost dbname=postgis user=postgres" layer=polymap \
output=polygons type=boundary,centroid
```

If the table containing the polygons are in a specific schema, you can
use:

```sh
v.in.ogr input="PG:host=localhost dbname=postgis user=postgres" \
layer=myschema.polymap \
output=polygons type=boundary,centroid
```

Generally, *v.in.ogr* just follows the
[format-specific](https://gdal.org/en/stable/drivers/vector/) syntax
defined by the OGR library.

### OpenStreetMap (OSM)

[OSM data](https://gdal.org/en/stable/drivers/vector/osm.html) are
available in .osm (XML based) and .pbf (optimized binary) formats. The
.pbf format is recommended because file sizes are smaller. The OSM
driver will categorize features into 5 layers :

- **points**: "node" features that have significant tags attached.
- **lines**: "way" features that are recognized as non-area.
- **multilinestrings**: "relation" features that form a
  multilinestring(type = 'multilinestring' or type = 'route').
- **multipolygons**: "relation" features that form a multipolygon (type
  = 'multipolygon' or type = 'boundary'), and "way" features that are
  recognized as area.
- **other_relations**: "relation" features that do not belong to any of
  the above layers.

It is recommended to import one layer at a time, and to select features
with the **where** option, e.g. to import roads, use

```sh
v.in.ogr where="highway >< ''"
```

i.e. the OSM tag *highway* must be set.

When importing administrative boundaries from OSM, it is important to
not only select administrative boundaries, but also the admin level to
be imported (valid range is 1 - 11), e.g. with

```sh
v.in.ogr where="boundary = 'administrative' and admin_level = '1'"
```

The OSM topological model differs from the GRASS topological model. OSM
topologically correct connections of lines can be on all vertices of a
line. During import, lines are automatically split at those vertices
where an OSM connection to another line exists.

Import of OSM data requires a configuration file, defined with the
OSM_CONFIG_FILE configuration option. In the data folder of the GDAL
distribution, you can find a [osmconf.ini
file](https://github.com/OSGeo/gdal/blob/master/gdal/data/osmconf.ini)
that can be customized to fit your needs. See [OSM map
features](https://wiki.openstreetmap.org/wiki/Map_Features) for keys and
their values. You should set "other_tags=no" to avoid problems with
import or querying the imported vector. Once a OSM_CONFIG_FILE has been
created, OSM data can be imported with e.g.

```sh
export OSM_CONFIG_FILE=/path/to/osmconf.ini
v.in.ogr input=name.pbf layer=lines output=osm_data
```

### Oracle Spatial

Note that you have to set the environment-variables
`ORACLE_BASE, ORACLE_SID, ORACLE_HOME` and `TNS_ADMIN` accordingly.

```sh
v.in.ogr input=OCI:username/password@database_instance output=grasslayer layer=roads_oci
```

### Multiple geometry columns

This example shows how to work with data which contain multiple geometry
per feature. The number of geometry columns per feature can be checked
by *[v.external](v.external.md)* together with **-t** flag.

```sh
v.external -t input=20141130_ST_UKSH.xml.gz
...
Okresy,point,1,DefinicniBod
Okresy,multipolygon,1,OriginalniHranice
Okresy,multipolygon,1,GeneralizovaneHranice
...
```

In our example layer "Okresy" has three geometry columns:
"DefinicniBod", "OriginalniHranice" and "GeneralizovanaHranice". By
default *v.in.ogr* reads data from all three geometry columns. The user
can specify desired geometry column by **geometry** option, in this case
the module will read geometry only from the specified geometry column.
In the example below, the output vector map will contain only geometry
saved in "OriginalniHranice" geometry column.

```sh
v.in.ogr input=20141130_ST_UKSH.xml.gz layer=Okresy geometry=OriginalniHranice
```

## WARNINGS

If a message like

```sh
WARNING: Area size 1.3e-06, area not imported
```

appears, the **min_area** may be adjusted to a smaller value so that all
areas are imported. Otherwise tiny areas are filtered out during import
(useful to polish digitization errors or non-topological data).

If a message like

```sh
Try to import again, snapping with at least 1e-008: 'snap=1e-008'
```

appears, then the map to be imported contains topological errors. The
message suggests a value for the *snap* parameter to be tried. For more
details, see above in *[Topology
Cleaning](v.in.ogr.md#topology-cleaning)*.

## ERROR MESSAGES

### SQL syntax errors

Depending on the currently selected SQL driver, error messages such as
follows may arise:

```sh
DBMI-SQLite driver error:
Error in sqlite3_prepare():
near "ORDER": syntax error
```

Or:

```sh
DBMI-DBF driver error:
SQL parser error:
syntax error, unexpected DESC, expecting NAME processing 'DESC
```

This indicates that a column name in the input dataset corresponds to a
reserved SQL word (here: 'ORDER' and 'DESC' respectively). A different
column name has to be used in this case. The **columns** parameter can
be used to assign different column names on the fly in order to avoid
using reserved SQL words. For a list of SQL reserved words for SQLite
(the default driver), see
[here](https://www.sqlite.org/lang_keywords.html).

### Projection errors

```sh
Coordinate reference system of dataset does not appear to match the current project.
```

Here you need to create or use a project whose CRS matches that of the
vector data you wish to import. Try using **project** parameter to
create a new project based upon the CRS information in the file. If
desired, you can then reproject it to another project with
*[v.proj](v.proj.md)*.

## REFERENCES

- [OGR vector library](https://gdal.org/)
- [OGR vector library C
  API](https://gdal.org/en/stable/api/vector_c_api.html) documentation

## SEE ALSO

*[db.connect](db.connect.md), [v.clean](v.clean.md),
[v.extract](v.extract.md), [v.build.polylines](v.build.polylines.md),
[v.edit](v.edit.md), [v.external](v.external.md),
[v.import](v.import.md), [v.in.db](v.in.db.md), [v.in.e00](v.in.e00.md),
[v.out.ogr](v.out.ogr.md)*

GRASS GIS Wiki page: Import of [Global
datasets](https://grasswiki.osgeo.org/wiki/Global_datasets)

## AUTHORS

Original author: Radim Blazek, ITC-irst, Trento, Italy  
Location and spatial extent support by Markus Neteler and Paul Kelly  
Various improvements by Markus Metz  
Multiple geometry columns support by Martin Landa, OSGeoREL, Czech
Technical University in Prague, Czech Republic

## DESCRIPTION

*v.external* creates new vector map as a link to external OGR layer or
PostGIS feature table. OGR (Simple Features Library) is part of the
[GDAL](http://www.gdal.org) library, so you need to install GDAL to use
*v.external* for external OGR layers. Note that a PostGIS feature table
can be linked also using built-in *GRASS-PostGIS data driver* (requires
GRASS to be built with PostgreSQL support).

## NOTES

The simple feature data model used by OGR (or PostGIS) is very different
from the topological format used by GRASS. Instead of true topology, so
called \'pseudo topology\' is created for data linked by *v.external*.
User should learn the difference between those two formats, because
**some modules** working correctly with GRASS native data, **can produce
wrong results** with input vector maps created by *v.external*.

**Limitations:**

Due to these data model differences *v.external* does not work with all
data formats. In general, for all formats that do not have a key column
(e.g. SHAPE file), attributes are not accessible, and attributes would
get lost when modifying the geometries. Therefore it is generally not
safe to link vector data with *v.external*. In many cases it does not
make sense to use *v.external* linked data with simple features, instead
vector data should be imported with *v.import* or *v.in.ogr* to get true
topology support. Importantly, point cloud data which do not have
topology, can be linked with *v.external* as long as there are no
attributes attached to these point cloud data, or if the format of the
point cloud data has a key column that allows linking vector geometries
to attributes.

See *[v.db.connect](v.db.connect.html)* for an example of maintaining
attributes in external DBMS in also writable mode.

### Supported OGR vector formats

To list supported OGR formats, type

::: code
    v.external -f
:::

For details see [GDAL web site](https://gdal.org/drivers/vector/).

## EXAMPLES

### ESRI Shapefile

Assuming that \'test_shape.shp\' is located in directory
\'/home/user/shape_data\'.

::: code
    v.external input=/home/user/shape_data layer=test_shape output=grass_map
:::

### PostGIS layers

By default, PostGIS links are created by built-in PostGIS support, ie.
using *GRASS-PostGIS data driver*. If the environment variable
`GRASS_VECTOR_OGR` exists, or GRASS is compiled without PostgreSQL
support then GRASS will use OGR-PostgreSQL driver for creating a link.

List of layers for given data source can be printed by **-l** flag.

::: code
    v.external input="PG:host=localhost user=postgres dbname=postgis" -l

    ...
    polymap
    ...
:::

::: code
    v.external input="PG:host=localhost user=postgres dbname=postgis" layer=polymap
:::

Note: Authentication details (user password) can be preferably defined
by *[db.login](db.login.html)*.

### MapInfo files

Assuming that \'mapinfo_test\' MapInfo file is located in the current
directory (\".\").

::: code
    v.external input=./ layer=mapinfo_test output=grass_map
:::

### SDTS files

Note: you have to select the CATD file

::: code
    v.external input=CITXCATD.DDF output=cities
:::

### TIGER files

::: code
    v.external input=input/2000/56015/ layer=CompleteChain,PIP output=t56015_all
:::

### Linking subset of features

By **where** option only subset of features can be linked. In the
example below only one feature (in this case a vector tile) will be
linked as a new GRASS vector map.

::: code
    v.external in="PG:dbname=tiles" layer=wrs2_descending where="pr=191026"
    ...
    Number of areas: 1
    ...
:::

## REFERENCES

[OGR vector library C API](https://gdal.org/api/) documentation

## SEE ALSO

*[v.external.out](v.external.out.html), [v.clean](v.clean.html),
[v.db.connect](v.db.connect.html), [v.import](v.import.html),
[v.in.db](v.in.db.html), [v.in.ogr](v.in.ogr.html),
[v.out.ogr](v.out.ogr.html)*

[GDAL Library](http://www.gdal.org/)\
[PostGIS](http://postgis.org/)

See also GRASS [user wiki
page](https://grasswiki.osgeo.org/wiki/Working_with_external_data_in_GRASS_7)
for more examples.

## AUTHORS

Radim Blazek, ITC-Irst, Trento, Italy\
PostGIS support by Martin Landa, GeoForAll (OSGeoREL) Lab, Czech
Technical University in Prague, Czech Republic

## DESCRIPTION

*v.external.out* instructs GRASS to write vector maps in external data
format (e.g. ESRI Shapefile, Mapinfo, and others) using [OGR
library](https://gdal.org/). PostGIS data can be also written by
built-in [GRASS-PostGIS data
provider](https://trac.osgeo.org/grass/wiki/Grass7/VectorLib/PostGISInterface).

## NOTES

Number of available output formats (`v.external.out -f`) depends on OGR
installation. 'PostgreSQL' format is presented also when GRASS comes
with PostgreSQL support (check for '--with-postgres' in `g.version -b`
output).

To store geometry and attribute data in PostGIS database ('PostgreSQL'
format) GRASS uses built-in *GRASS-PostGIS data provider*. PostGIS data
can be written also by OGR library when `GRASS_VECTOR_OGR` environmental
variable is defined or GRASS is not compiled with PostgreSQL support.

Creation **options** refer to the output format specified by **format**
option. See the list of valid creation options at [OGR formats
specification page](https://gdal.org/en/stable/drivers/vector/), example
for [ESRI
Shapefile](https://gdal.org/en/stable/drivers/vector/shapefile.html) or
[PostgreSQL/PostGIS](https://gdal.org/en/stable/drivers/vector/pg.html)
format (section "Layer Creation Options"). Options are comma-separated
pairs (`key=value`), the options are case-insensitive, eg.
`options="SCHEMA=myschema,FID=cat"`.

### PostgreSQL/PostGIS Creation Options

Note that built-in *GRASS-PostGIS data provider* (**format=PostgreSQL**)
supports different creation **options** compared to PostgreSQL/PostGIS
driver from OGR library:

- `SCHEMA=<name>` - name of schema where to create feature tables. If
  schema doesn't exists, it's automatically created when writing PostGIS
  data.
- `FID=<column>` - name of column which will be used as primary key
  (feature id), default: `fid`
- `GEOMETRY_NAME=<column>` name of column which will be used for storing
  geometry data in feature table, default: `geom`
- `SPATIAL_INDEX=YES|NO` - enable/disable spatial index on geometry
  column, default: YES
- `PRIMARY_KEY=YES|NO` - enable/disable primary key on FID column,
  default: YES
- `SRID=<value>` - spatial reference identifier, default: not defined
- `TOPOLOGY=YES|NO` - enable/disable native [PostGIS
  topology](https://grasswiki.osgeo.org/wiki/PostGIS_Topology), default:
  NO

Options relevant only to topological output (`TOPOLOGY=YES`):

- `TOPOSCHEMA_NAME=<schema name>` - name of PostGIS Topology schema
  (relevant only for `TOPOLOGY=YES`), default: `topo_<input>`
- `TOPOGEOM_NAME=<column>` - name of column which will be used for
  storing topogeometry data in feature table, default: `topo`
- `TOPO_TOLERANCE=<value>` - tolerance for PostGIS Topology schema, see
  [CreateTopology](https://postgis.net/docs/CreateTopology.html)
  function for details, default: `0`
- `TOPO_GEO_ONLY=YES|NO` - store in PostGIS Topology schema only data
  relevant to Topo-Geo data model, default: `NO`
- `SIMPLE_FEATURE=YES|NO` - build simple features geometry in
  `GEOMETRY_NAME` column from topogeometry data, default: NO

Note that topological output requires **PostGIS version 2 or later**.

## EXAMPLES

### ESRI Shapefile

*v.external.out* can be used along with *[v.external](v.external.md)* to
process external geodata in GRASS while writing out the results directly
eg. in ESRI Shapefile format:

```sh
# register Shapefile in GRASS mapset:
v.external input=/path/to/shapefiles layer=cities

# define output directory for GRASS calculation results:
v.external.out output=$HOME/gisoutput

# do something (here: spatial query), write output directly as Shapefile
v.select ainput=cities atype=point binput=forests btype=area operator=within output=fcities
```

Current settings can be printed using **-p** or **-g** flag.

```sh
v.external.out -p

output: /path/to/home/gisoutput
format: ESRI Shapefile
```

### PostGIS (simple features)

PostGIS data can be accessed directly using *GRASS-PostGIS data
provider* (GRASS must be compiled with PostgreSQL support).

```sh
# register PostGIS table in GRASS mapset:
v.external output=PG:dbname=gisdb layer=cities

# define output PostGIS database for GRASS calculation results stored as simple features:
v.external.out output=PG:dbname=gisdb format=PostgreSQL

# do some processing...
```

*Note:* If the environment variable `GRASS_VECTOR_OGR` is defined, or
GRASS is compiled without PostgreSQL support then GRASS will use
PostgreSQL driver from OGR library for reading and writing PostGIS data.

### PostGIS Topology

```sh
# define output PostGIS database for GRASS calculation results stored as topological elements:
v.external.out output=PG:dbname=gisdb format=PostgreSQL options=topology=YES

# do some processing...
```

*Note:* PostGIS topological access is supported only in built-in
*GRASS-PostGIS data provider*.

### GRASS native format

To restore original settings, ie. use the GRASS native format, type:

```sh
v.external.out -r
```

### Restore settings

Current settings can be stored to file by specifying **output** option.

```sh
# define output PostGIS database for GRASS calculation with
# results stored as topological elements:
v.external.out output=PG:dbname=gisdb format=PostgreSQL \
  options=topology=YES savesettings=gisdb_topo.txt

# ... and do some processing in PostGIS Topology
```

Back to native format:

```sh
v.external.out -r

# do some processing in native format
```

Restore previous settings from "gisdb_topo.txt" file by specifying
**loadsettings** option.

```sh
v.external.out loadsettings=gisdb_topo.txt

# ... and do some processing in PostGIS Topology
```

## REFERENCES

- [GRASS-OGR data
  provider](https://trac.osgeo.org/grass/wiki/Grass7/VectorLib/OGRInterface)
- [OGR vector library C API](https://gdal.org/en/stable/api/)
  documentation
- [GRASS-PostGIS data
  provider](https://trac.osgeo.org/grass/wiki/Grass7/VectorLib/PostGISInterface)
- [libpq - C
  Library](https://www.postgresql.org/docs/9.1/static/libpq.html)

## SEE ALSO

*[v.external](v.external.md), [v.in.ogr](v.in.ogr.md),
[v.out.ogr](v.out.ogr.md), [v.out.postgis](v.out.postgis.md)*

See also GRASS [user wiki
page](https://grasswiki.osgeo.org/wiki/Working_with_external_data_in_GRASS_7)
for more examples.

## AUTHOR

Martin Landa, Czech Technical University in Prague, Czech Republic
(development supported by Fondazione Edmund Mach and Comune di Trento,
Italy)

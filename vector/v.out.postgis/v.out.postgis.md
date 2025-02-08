## DESCRIPTION

*v.out.postgis* exports an existing GRASS vector map layer to PostGIS
feature table. Features without category are skipped.

By default GRASS GIS topological features are converted into simple
features (see [OGC Simple Feature
Access](https://www.ogc.org/publications/standard/sfa/) specification
for details). Flag **-l** allows to export vector features as
topological elements stored in [PostGIS
Topology](https://postgis.net/docs/Topology.html) schema. Note that
topological export requires PostGIS version 2 or later.

Additional creation options can be defined by **options** parameter:

- `FID=<column>` - name of column which will be used as primary key
  (feature id), default: `fid`
- `GEOMETRY_NAME=<column>` name of column which will be used for storing
  geometry data in feature table, default: `geom`
- `SPATIAL_INDEX=YES|NO` - enable/disable creating spatial index on
  geometry column, default: `YES`
- `PRIMARY_KEY=YES|NO` - enable/disable adding primary key on FID
  column, default: `YES`
- `SRID=<value>` - spatial reference identifier, default: not defined

PostGIS Topology related options (relevant only for **-l** flag):

- `TOPOSCHEMA_NAME=<schema name>` - name of PostGIS Topology schema,
  default: `topo_<input>`
- `TOPOGEOM_NAME=<column>` - name of column which will be used for
  storing topogeometry data in feature table, default: `topo`
- `TOPO_TOLERANCE=<value>` - tolerance for PostGIS Topology schema, see
  [CreateTopology](https://postgis.net/docs/CreateTopology.html)
  function for details, default: `0`
- `TOPO_GEO_ONLY=YES|NO` - store in PostGIS Topology schema only data
  relevant to Topo-Geo data model, default: `NO`

Creation **options** are comma-separated pairs (`key=value`), the
options are case-insensitive. Note that **options** defined by
*[v.external.out](v.external.out.md)* are ignored by *v.out.postgis*.

*v.out.postgis* optionally also creates a new vector map in the current
mapset if **output_link** is defined.

## NOTES

By default *v.out.postgis* exports vector data as *simple features*, ie.
boundaries and centroids (forming topological areas) become polygons,
isles become holes. Geometry of simple feature elements is stored in
PostGIS feature table in the column named "geom". Name of the geometry
column can be changed by **options=**`GEOMETRY_NAME=<column>`. Note that
for exporting vector features as simple features can be alternatively
used [PostgreSQL
driver](https://gdal.org/en/stable/drivers/vector/pg.html) from
[OGR](https://gdal.org/) library through *[v.out.ogr](v.out.ogr.md)*
module.

Also note that it's allowed to store in the feature table only features
of the same type, ie. mixing of points and lines is not currently
possible. The feature type is determined for output feature table from
the first vector feature accessing data sequentially (**type=auto**).
Vector features of other types are during export skipped. User can
choose preferable feature type by **type** parameter. Only single type
is currently allowed (see TODO section for details).

*v.out.postgis* currently supports only three basic output simple
feature types: Points, Linestrings and Polygons. Also 3D features of the
same type are supported, eg. 3D points are exported as `PointZ` simple
feature. Faces are exported as 3D polygons. 3D features are written to
the output automatically if input vector map is 3D. If **-2** flag is
given then the output is always 2D (z-coordinate is silently ignored for
3D input vector maps).

Multigeometries are not currently supported. Features with the same
category are exported as multiple single features.

*v.out.postgis* also allows exporting vector features as *topological
elements* in [PostGIS Topology](https://postgis.net/docs/Topology.html)
schema. PostGIS Topology extension uses three tables to store basic
topological elements which forms topological objects like areas or isles
in GRASS terminology. *Nodes* (0-dimensional topological elements) are
stored in "node" table, *edges* (1-dimensional elements) in "edge" table
and *faces* (2-dimensional elements) in "face" table.

- GRASS nodes are stored in *node* table
- GRASS points are stored in *node* table as regular nodes
- GRASS centroids are stored in *node* table as regular nodes
  ("containing_face" refers to related area)
- GRASS lines are stored in *edge* table
- GRASS boundaries are stored in *edge* table
- GRASS areas are stored in *face* table

Tables *node*, *edge* and *face* are stored in given topological schema.
By default *v.out.postgis* defines its name as `topo_<input>`.
Alternatively, the name for topology schema can be defined by
**options=**`TOPOSCHEMA_NAME=<name>`.

## EXAMPLES

### Export Simple Features

Export vector map "urbanarea" as feature table "urbanarea" located in
database "grass", schema "public". Note that this database schema is
automatically used when not defined by the user.

```sh
v.out.postgis input=urbanarea output="PG:dbname=grass"
```

GRASS areas are converted into polygons, isles into holes. We can check
the number or created polygons by simple SQL query below.

```sh
db.select driver=pg database=grass \
 sql="SELECT ST_GeometryType(geom) as geom_type, count(*) from urbanarea group by geom_type"

geom_type|count
ST_Polygon|657
```

*Note:* same procedure can be done by *[v.out.ogr](v.out.ogr.md)*
module, eg.

```sh
v.out.ogr input=urbanarea output="PG:dbname=grass" format=PostgreSQL
```

In this case GRASS vector data are exported to PostGIS database using
OGR library, namely using PostgreSQL driver. Contrary to the
*[v.out.ogr](v.out.ogr.md)* module, *v.out.postgis* is using directly
PostGIS data provider which is part of GRASS vector engine. Besides
that, *v.out.postgis* is optimized for PostGIS export including
topological access to the data.

### Export data into specific database schema

Database schema for storing exported data can be defined by
**output_layer** as `<schema_name>.<table_name>`. If the specified
schema doesn't exist in the database, then it's automatically created.

Export vector map "bridges" as feature table in database schema
"grassout".

```sh
v.out.postgis input=bridges output="PG:dbname=grass" output_layer=grassout.bridges
```

### Export data with creation options

Example below demonstrates how to define name for geometry column and
disable building spatial index. Spatial reference system is defined by
`srid` identifier which corresponds in this case with EPSG 3358 (North
Carolina dataset).

```sh
v.out.postgis input=roadsmajor output="PG:dbname=grass" options="GEOMETRY_NAME=wkb_geometry,SPATIAL_INDEX=NO,SRID=3358"
```

### Link exported data

Exported data can be linked as vector map created in the current mapset
by specifying **output_link** parameter. In the example below vector map
"busstopsall" from PERMANENT mapset is exported into "grass" PostGIS
database. *v.out.postgis* after successful export also creates in the
current mapset GRASS vector map as a link to the PostGIS feature table.

```sh
v.out.postgis input=busstopsall@PERMANENT output="PG:dbname=grass" output_link=busstopsall_pg
```

Created link can be checked by *[v.info](v.info.md)*:

```sh
 v.info busstopsall_pg

...
 |----------------------------------------------------------------------------|
 | Map format:      PostGIS (PostgreSQL)                                      |
 | DB table:        public.busstopsall                                        |
 | DB name:         grass                                                     |
 | Geometry column: geom                                                      |
 | Feature type:    point                                                     |
 | Topology:        pseudo (simple features)                                  |
 |----------------------------------------------------------------------------|
...

```

### Export data without attributes

*v.out.postgis* allows ignoring attributes when exporting vector
features by specifying **-t** flag. Command below exports vector
features without attributes. The feature will contain only two columns,
the fid and geometry column.

```sh
v.out.postgis -t input=railroads output="PG:dbname=grass"
```

### Export topological data

By default *v.out.postgis* exports data as simple features. Flag **-l**
allows exporting data as topological elements instead of simple
features. Export topological elements is stored in [PostGIS
Topology](https://postgis.net/docs/Topology.html) schema.

```sh
v.out.postgis -l input=busroutesall output="PG:dbname=grass"
```

For more info about PostGIS Topology implementation in GRASS see the
[wiki page](https://grasswiki.osgeo.org/wiki/PostGIS_Topology).

## TODO

- Multi-feature export
- Allow mixed features (points, lines)
- Support other simple feature types like GeometryCollection and others
- Implement missing options from *[v.out.ogr](v.out.ogr.md)*: **-a**,
  **-s**, **-c**, **-p**, **-n**
- Add options: **cats**, **where**

## REQUIREMENTS

- PostGIS 2.x or later for topological export (flag **-l**)

## REFERENCES

- [OGC Simple Feature
  Access](https://www.ogc.org/publications/standard/sfa/) specification
- [PostGIS Topology](https://postgis.net/docs/Topology.html)
  documentation
- [GRASS-PostGIS data
  provider](https://grass.osgeo.org/programming8/vlibPg.html)

## SEE ALSO

*[v.out.ogr](v.out.ogr.md), [v.external](v.external.md),
[v.external.out](v.external.out.md), [v.in.ogr](v.in.ogr.md)*

See also [PostGIS](https://grasswiki.osgeo.org/wiki/PostGIS) and
[PostGIS Topology](https://grasswiki.osgeo.org/wiki/PostGIS_Topology)
wiki page from GRASS User Wiki.

## AUTHOR

Martin Landa, Czech Technical University in Prague, Czech Republic
(development supported by Fondazione Edmund Mach and Comune di Trento,
Italy)

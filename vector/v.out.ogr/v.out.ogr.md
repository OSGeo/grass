## DESCRIPTION

*v.out.ogr* converts GRASS vector map layer to any of the supported
[OGR](https://gdal.org/) vector formats (including OGC GeoPackage, ESRI
Shapefile, SpatiaLite or GML).

OGR (Simple Features Library) is part of the [GDAL](https://gdal.org)
library, so you need to install this library to use *v.out.ogr*.

The OGR library supports many various formats including:

- [OGC GeoPackage](https://gdal.org/en/stable/drivers/vector/gpkg.html)
- [ESRI
  Shapefile](https://gdal.org/en/stable/drivers/vector/shapefile.html)
- [PostGIS](https://gdal.org/en/stable/drivers/vector/pg.html)
- [SpatiaLite](https://gdal.org/en/stable/drivers/vector/sqlite.html)
- [CSV](https://gdal.org/en/stable/drivers/vector/csv.html)
- [GML](https://gdal.org/en/stable/drivers/vector/gml.html)
- [KML](https://gdal.org/en/stable/drivers/vector/kml.html)
- [Mapinfo File](https://gdal.org/en/stable/drivers/vector/mitab.html)
- [TIGER](https://gdal.org/en/stable/drivers/vector/tiger.html)
- ... and many others

The list of supported formats is printed with the *-l* flag.

For further available other supported formats go
[here](https://gdal.org/en/stable/drivers/vector/).

## NOTES

By default, islands will appear as holes inside surrounding areas. To
export polygons with holes into, e.g., a Esri Shapefile, and make the
holes appear as filled areas, the flag **-c** has to be used.

*v.out.ogr* exports 3D vector data as 2.5D simple features if possible
(not supported by all formats). For exporting 3D vector data as 2D
simple features, use **-2** flag.

By default, *v.out.ogr* converts GRASS vector data to single simple
features. If GRASS feature has more categories defined in the given
**layer**, then this feature is exported multiple times. GRASS features
without category are skipped by default. To export features also without
category, the **-c** flag must be given.

When **-m** flag is given, *v.out.ogr* export GRASS vector data as
multi-features. Multi-feature is formed by GRASS features with the same
category. When **-c** flag is given, the module also export GRASS
features without category as one multi-feature. Note that multi-features
are not supported by all formats.

ESRI Shapefile export: note that due to the DBF table specifications
column names may not be longer than 10 characters.

Performance: export to SQLite based formats including OGC GeoPackage may
become faster with the environmental variable `OGR_SQLITE_CACHE=1024`
being set (value depends on available RAM, see [OGR
ConfigOptions](https://trac.osgeo.org/gdal/wiki/ConfigOptions#OGR_SQLITE_CACHE)).

## EXAMPLES

### Export to OGC GeoPackage

Export lines from a GRASS vector map to OGC GeoPackage format:

```sh
v.out.ogr input=roadsmajor type=line output=roadsmajor.gpkg
```

Export areas from GRASS vector map to OGC GeoPackage format, converting
islands (holes) to filled polygons:

```sh
v.out.ogr -c input=areas_islands type=area output=areas_islands.gpkg
```

Export mixed geometry type GRASS vector map to OGC GeoPackage format:

```sh
v.out.ogr input=generic_vector output=mixed_geometry.gpkg
```

### Export to ESRI Shapefile

Export lines from GRASS vector map to Shapefile format:

```sh
v.out.ogr input=roadsmajor type=line format=ESRI_Shapefile output=lines.shp
```

Export areas from GRASS vector map to Shapefile format, converting
islands (holes) to filled polygons:

```sh
v.out.ogr -c input=areas_islands type=area format=ESRI_Shapefile output=areas_islands.shp
```

Export 3D lines from GRASS vector map to Shapefile format:

```sh
v.out.ogr input=lines_3d type=line format=ESRI_Shapefile output=lines_3d.shp lco="SHPT=ARCZ"
```

Export 3D points (e.g., Lidar points) from GRASS vector map to Shapefile
format

```sh
v.out.ogr points_3d type=point format=ESRI_Shapefile output=points_3d.shp lco="SHPT=POINTZ"
```

Export 3D faces from GRASS vector map to Shapefile format:

```sh
v.out.ogr input=objects_3d type=face format=ESRI_Shapefile output=faces_3d.shp lco="SHPT=POLYGONZ"
```

Export 3D faces from GRASS vector map to Shapefile format, automatic 3D
setting:

```sh
v.out.ogr input=objects_3d type=face format=ESRI_Shapefile output=faces_3d.shp"
```

### Export to GML

Export lines from GRASS vector map to GML format (generates
'/tmp/testogr.gml' file with layer 'testogr'):

```sh
v.out.ogr input=multi type=line output=/tmp/testogr.gml output_layer=testogr format=GML
```

### Export to PostgreSQL/PostGIS

Export areas from GRASS vector map to PostGIS database:

```sh
v.out.ogr input=polygons type=area output="PG:host=localhost dbname=postgis user=postgres" output_layer=polymap format=PostgreSQL
```

*Note:* For exporting GRASS vector data to PostGIS database can be also
used *[v.out.postgis](v.out.postgis.md)* module. This module is not
based on OGR library and supports beside simple features also
topological format (PostGIS Topology).

### Export to KML (Google Earth)

Export faces (3D vectors) from GRASS vector map to KML format for Google
Earth:

```sh
v.out.ogr input=buildings_3d output=buildings_3d.kml output_layer=buildings_3d format=KML type=face
```

Generate and export GRASS vector "asteroid" map (faces, 3D vectors) to
KML format for Google Earth:

```sh
# near Raleigh (NC, USA)
g.region n=35.73952587 s=35.73279182 w=-78.68263928 e=-78.67499517

# two layers of random points
v.random -z output=random3d_a n=10 zmin=0 zmax=200
v.random -z output=random3d_b n=15 zmin=400 zmax=600

# merge into one 3D points map
v.patch input=random3d_a,random3d_b output=random3d

# generate 3D convex hull
v.hull input=random3d output="random3d_hull"

# export to KML 3D
v.out.ogr input=random3d_hull output=random3d_hull.kml format=KML type=face dsco="AltitudeMode=absolute"

# now open KML file 'random3d_hull.kml' in Google Earth or NASA WorldWind or ...
```

## REFERENCES

- [OGR vector library](https://gdal.org/)
- [OGR vector library C API](https://gdal.org/api/) documentation

## SEE ALSO

*[v.out.postgis](v.out.postgis.md), [db.out.ogr](db.out.ogr.md),
[v.external](v.external.md), [v.external.out](v.external.out.md),
[v.in.ogr](v.in.ogr.md), [v.pack](v.pack.md)*

## AUTHORS

Radim Blazek, ITC-Irst, Trento, Italy  
Some contributions: Markus Neteler  
Multi-feature support by Martin Landa, Czech Technical University in
Prague, 2013

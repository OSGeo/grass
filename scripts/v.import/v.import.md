## DESCRIPTION

*v.import* imports vector data from files and database connections
supported by the [OGR](https://gdal.org/) library into the current
project (previously called location) and mapset. If the coordinate
reference system (CRS) of the input does not match the CRS of the
project, the input is reprojected into the current project. In case that
the CRS of the input map does match the CRS of the project, the input is
imported directly.

### Supported Vector Formats

*v.import* uses the OGR library which supports various vector data
formats including [ESRI
Shapefile](https://gdal.org/en/stable/drivers/vector/shapefile.html),
[Mapinfo File](https://gdal.org/en/stable/drivers/vector/mitab.html), UK
.NTF, SDTS, TIGER, IHO S-57 (ENC), DGN, GML, GPX, AVCBin, REC, Memory,
OGDI, and PostgreSQL, depending on the local OGR installation. For
details see the [OGR web
site](https://gdal.org/en/stable/drivers/vector/). The OGR (Simple
Features Library) is part of the [GDAL](https://gdal.org) library, hence
GDAL needs to be installed to use *v.import*.

The list of actually supported formats can be printed by **-f** flag.

## NOTES

*v.import* checks the CRS metadata of the dataset to be imported against
that of the current project. If not identical a related error message is
shown.  
To override this CRS check (i.e. to use current project's CRS) by
assuming that the dataset has the same CRS as the current project the
**-o** flag can be used. This is also useful when geodata to be imported
do not contain any CRS metadata at all. The user must be sure that the
CRS is identical in order to avoid introducing data errors.

### Topology cleaning

When importing polygons, non-topological polygons are converted to
topological areas. If the input polygons contain errors (unexpected
overlapping areas, small gaps between polygons, or warnings about being
unable to calculate centroids), the import might need to be repeated
using a *snap* value as suggested in the output messages. The default
value of `snap=-1` means that no snapping will be done.

The *snap* threshold defines the maximal distance from one to another
vertex in map units (for latitude-longitude projects in degrees). If
there is no other vertex within *snap* distance, no snapping will be
done. Note that a too large value can severely damage area topology,
beyond repair.

*Post-processing:* Snapped boundaries may need to be cleaned with
*v.clean*, using its tools *break,rmdupl,rmsa*. For details, refer to
the *v.clean* manual page.

## EXAMPLE

```sh
# import SHAPE file at full extent and reproject to current project CRS
v.import input=research_area.shp output=research_area extent=input
```

## SEE ALSO

*[v.clean](v.clean.md), [v.in.lines](v.in.lines.md),
[v.in.ogr](v.in.ogr.md), [v.proj](v.proj.md)*

## AUTHORS

Markus Metz  
Improvements: Martin Landa, Anna Petrasova

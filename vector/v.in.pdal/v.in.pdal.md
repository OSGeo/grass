## DESCRIPTION

Imports a point cloud (e.g. from a LAS file) as vector points with the
PDAL library. *v.in.pdal* supports the following filters:

- 2D region filter
- Z coordinates filter
- return filter
- class filter

## EXAMPLES

Import only XYZ coordinates of points, limit the import to the current
computational region and reproject to the current project's coordinate
reference system during the import:

```sh
v.in.pdal input=points.las output=points -c -r -w
```

## REFERENCES

- V. Petras, A. Petrasova, J. Jeziorska, H. Mitasova (2016): *Processing
  UAV and lidar point clouds in GRASS GIS*. XXIII ISPRS Congress 2016
  \[[ISPRS
  Archives](https://doi.org/10.5194/isprs-archives-XLI-B7-945-2016),
  [ResearchGate](https://www.researchgate.net/publication/304340172_Processing_UAV_and_lidar_point_clouds_in_GRASS_GIS)\]

## SEE ALSO

*[r.in.pdal](r.in.pdal.md), [g.region](g.region.md),
[v.vect.stats](v.vect.stats.md), [v.in.ogr](v.in.ogr.md)*

## AUTHOR

Vaclav Petras, [NCSU GeoForAll
Lab](https://geospatial.ncsu.edu/geoforall/)

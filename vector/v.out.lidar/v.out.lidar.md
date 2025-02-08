## DESCRIPTION

*v.out.lidar* converts GRASS vector map to a LiDAR point clouds in LAS
format using the [libLAS](https://liblas.org) library.

The **-r** flag limits the export to the current computational region
extent (see *[g.region](g.region.md)*). The **where** option limits the
export by attributes (applied only when the columns are used for
export).

LAS format stores the coordinates as integers rounding the decimal
places. Before that a scale is applied to preserve a certain number of
decimal places. This scale can be set using **las_xyscale** and
**las_xscale** options. For example, the scale value 0.01 will preserve
two decimal places while the value 1.0 will preserve none.

## NOTES

The typical file extensions for the LAS format are .las and .laz
(compressed). The compressed LAS (.laz) format can be exported only if
libLAS has been compiled with [LASzip](https://laszip.org/) support. It
is also good when libLAS was compiled with GDAL. This is needed when
working with projections.

## EXAMPLE

Generate fractal surface and export is as point in LAS format:

```sh
g.region raster=elevation res=100
r.surf.fractal output=fractals
r.to.vect input=fractals output=fractals type=point -z
v.out.lidar input=fractals output=fractals.las
```

## REFERENCES

[ASPRS LAS
format](https://www.asprs.org/committee-general/laser-las-file-format-exchange-activities.html)  
[LAS library](https://liblas.org/)  

## SEE ALSO

*[v.out.ogr](v.out.ogr.md)*

## AUTHOR

Vaclav Petras

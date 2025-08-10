## DESCRIPTION

*v.in.region* creates a new vector map from current region extent.

If the output of *v.in.region* is to be used for raster reprojection,
the **-d** flag should be used after setting the region to the raster
map to be reprojected with *[r.proj](r.proj.md)*.

## EXAMPLE

The example is based on the North Carolina sample data. To create a
bounding box vector map based on a raster map, the computational region
is first set to the raster map. Then a vector bounding box is created
based on the actual computational region (in this case precisely
reflecting the pixel geometry of the raster map), resulting in a new
vector polygon:

```sh
g.region raster=soils_Kfactor -p
v.in.region output=soils_Kfactor_bbox
v.info map=soils_Kfactor_bbox
```

## SEE ALSO

*[g.region](g.region.md), [r.proj](r.proj.md), [v.info](v.info.md)*

## AUTHOR

Radim Blazek

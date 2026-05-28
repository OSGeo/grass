## DESCRIPTION

*v.to.rast3* converts a GRASS 3D vector point map to a GRASS 3D raster
map.

## NOTES

When converting from a 3D vector point layer to a 3D raster map a vector
point is converted into a single 3D raster cell (voxel) representing the
location of the vector point. As 3D raster cell value the database
attribute of the vector point is stored with floating point precision.

![Result of the v.to.rast3 test](v_to_rast3_test.png)  
*Fig: This screenshot shows the result of the v.to.rast3 test.
 Visualized are the cube of the GRASS region, the vector points as black dots
 and the voxel cells as wireframe model.
 Only cells with non-null values are shown.*

## SEE ALSO

*[g.region](g.region.md)*

## AUTHORS

Original s.to.rast3: Jaro Hofierka, Geomodel s.r.o.  
Updated by Radim Blazek

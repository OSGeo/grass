## DESCRIPTION

*v.decimate* reduces number of points in the input vector map and copies
them over to the output vector map. Different point decimation
techniques can be applied to reduce the number of points.

Two main decimation techniques are:

- count-based decimation (**skip**, **preserve**, **offset** and
  **limit** options)
- grid-based decimation (**-g** flag)

The grid-based decimation will remove points based on:

- similar z coordinates (**-z** flag and **zdiff** option)
- same categories (**-c** flag)
- count of points (**-f** flag and **cell_limit** option)

The grid-based decimation is currently using a 2D grid, so the points
are placed and compared within this 2D grid. The comparison can happen
using z coordinates or categories. Note that although the grid is only
2D, the module works with 3D points.

The grid-based decimation extent and resolution depend on the current
computational region as set by *[g.region](g.region.md)*. As a
consequence, the output is limited only to computational region in this
case.

TODO: Currently, any output is limited by the region.

The count-based decimation result highly depends on how the data are
ordered in the input. This applies especially to **offset** and
**limit** options where the resulting shape and densities can be
surprising. The options **skip** and **preserve** are influenced by
order of points in a similar way but they usually keep relative density
of points (which may or may not be desired). On the other hand, the
grid-based decimation will generally result in more even density of
output points (see Figure 1).

Besides decimation, point count can be reduced by applying different
selections or filters, these are:

- selection by category (**cats** option)
- selection by z values (**zrange** option)

## NOTES

The grid-based decimation requires all points which will be saved in
output to fit into the computer's memory (RAM). It is advantageous to
have the region only in the area with the points, otherwise unnecessary
memory is allocated. Higher (finer) resolutions and higher amount of
preserved points per cell require more memory. The count-based
decimation has no limitation regarding the available memory.

Significant speed up can be gained using **-b** flag which disables
building of topology for the output vector map. This may limit the use
of the vector map by some modules, but for example, this module works
without topology as well.

## EXAMPLES

Keep only every forth point, throw away the rest:

```sh
v.decimate input=points_all output=points_decimated_every_4 preserve=4
```

Keep only points within a grid cell (given by the current computational
region) which has unique categories (e.g. LIDAR classes):

```sh
v.decimate input=points_all output=points_decimated_unique_cats layer=1 -g -c
```

![original points](v_decimate_original.png)
![decimation result with every forth point preserved](v_decimate_count.png)
![grid-based decimation result with points with unique categories in each grid cell](v_decimate_grid_cat.png)  
*Figure 1: Comparison of original points, decimation result with every
forth point preserved, and grid-based decimation result with points with
unique categories in each grid cell*

Keep only points with category 2 and keep only approximately 80% of the
points:

```sh
v.decimate input=points_all output=points_decimated_ skip=5 cats=2 layer=1
```

## REFERENCES

- Petras, V., Petrasova, A., Jeziorska, J., Mitasova, H. (2016).
  Processing UAV and LiDAR point clouds in grass GIS. The International
  Archives of Photogrammetry, Remote Sensing and Spatial Information
  Sciences, 41, 945
  ([DOI](https://doi.org/10.5194/isprsarchives-XLI-B7-945-2016))

## SEE ALSO

*[v.extract](v.extract.md), [v.outlier](v.outlier.md),
[v.select](v.select.md), [v.category](v.category.md),
[v.build](v.build.md), [v.in.pdal](v.in.pdal.md),
[g.region](g.region.md)*

## AUTHOR

Vaclav Petras, [NCSU GeoForAll
Lab](https://geospatial.ncsu.edu/geoforall/)

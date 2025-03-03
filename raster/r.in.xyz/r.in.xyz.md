## DESCRIPTION

The *r.in.xyz* module will load and bin ungridded x,y,z ASCII data into
a new raster map. The user may choose from a variety of statistical
methods in creating the new raster. Gridded data provided as a stream of
x,y,z points may also be imported.

Please note that the current region extents and resolution are used for
the import. It is therefore recommended to first use the **-s** flag to
get the extents of the input points to be imported, then adjust the
current region accordingly, and only then proceed with the actual
import.

*r.in.xyz* is designed for processing massive point cloud datasets, for
example raw LIDAR or sidescan sonar swath data. It has been tested with
datasets as large as tens of billion of points (705GB in a single file).

Available statistics for populating the raster are (**method**):

> |               |                                               |
> |---------------|-----------------------------------------------|
> | *n*           | number of points in cell                      |
> | *min*         | minimum value of points in cell               |
> | *max*         | maximum value of points in cell               |
> | *range*       | range of points in cell                       |
> | *sum*         | sum of points in cell                         |
> | *mean*        | average value of points in cell               |
> | *stddev*      | standard deviation of points in cell          |
> | *variance*    | variance of points in cell                    |
> | *coeff_var*   | coefficient of variance of points in cell     |
> | *median*      | median value of points in cell                |
> | *percentile*  | p-*th* percentile of points in cell |
> | *skewness*    | skewness of points in cell                    |
> | *trimmean*    | trimmed mean of points in cell                |

- *Variance* and derivatives use the biased estimator (n). \[subject to
  change\]
- *Coefficient of variance* is given in percentage and defined as
  `(stddev/mean)*100`.

It is also possible to bin and store another data column (e.g.
backscatter) while simultaneously filtering and scaling both the data
column values and the z range.

## NOTES

### Gridded data

If data is known to be on a regular grid *r.in.xyz* can reconstruct the
map perfectly as long as some care is taken to set up the region
correctly and that the data's native map projection is used. A typical
method would involve determining the grid resolution either by examining
the data's associated documentation or by studying the text file. Next
scan the data with *r.in.xyz*'s **-s** (or **-g**) flag to find the
input data's bounds. GRASS uses the cell-center raster convention where
data points fall within the center of a cell, as opposed to the
grid-node convention. Therefore you will need to grow the region out by
half a cell in all directions beyond what the scan found in the file.
After the region bounds and resolution are set correctly with
*[g.region](g.region.md)*, run *r.in.xyz* using the *n* method and
verify that n=1 at all places. *[r.univar](r.univar.md)* can help. Once
you are confident that the region exactly matches the data proceed to
run *r.in.xyz* using one of the *mean, min, max*, or *median* methods.
With n=1 throughout, the result should be identical regardless of which
of those methods are used.

### Memory use

While the **input** file can be arbitrarily large, *r.in.xyz* will use a
large amount of system memory for large raster regions (10000x10000). If
the module refuses to start complaining that there isn't enough memory,
use the **percent** parameter to run the module in several passes. In
addition using a less precise map format (`CELL` \[integer\] or `FCELL`
\[floating point\]) will use less memory than a `DCELL` \[double
precision floating point\] **output** map. Methods such as *n, min, max,
sum* will also use less memory, while *stddev, variance, and coeff_var*
will use more. The aggregate functions *median, percentile, skewness*
and *trimmed mean* will use even more memory and may not be appropriate
for use with arbitrarily large input files.

The default map **type**=`FCELL` is intended as compromise between
preserving data precision and limiting system resource consumption. If
reading data from a `stdin` stream, the program can only run using a
single pass.

### Setting region bounds and resolution

You can use the **-s** scan flag to find the extent of the input data
(and thus point density) before performing the full import. Use
*[g.region](g.region.md)* to adjust the region bounds to match. The
**-g** shell style flag prints the extent suitable as parameters for
*[g.region](g.region.md)*. A suitable resolution can be found by
dividing the number of input points by the area covered. e.g.

```sh
wc -l inputfile.txt
g.region -p
# points_per_cell = n_points / (rows * cols)

g.region -e
# UTM project:
# points_per_sq_m = n_points / (ns_extent * ew_extent)

# Lat/Lon project:
# points_per_sq_m = n_points / (ns_extent * ew_extent*cos(lat) * (1852*60)^2)
```

If you only intend to interpolate the data with
*[r.to.vect](r.to.vect.md)* and *[v.surf.rst](v.surf.rst.md)*, then
there is little point to setting the region resolution so fine that you
only catch one data point per cell -- you might as well use
"`v.in.ascii -zbt`" directly.

### Filtering

Points falling outside the current region will be skipped. This includes
points falling *exactly* on the southern region bound. (to capture those
adjust the region with "`g.region s=s-0.000001`"; see
*[g.region](g.region.md)*)

Blank lines and comment lines starting with the hash symbol (`#`) will
be skipped.

The **zrange** parameter may be used for filtering the input data by
vertical extent. Example uses might include preparing multiple raster
sections to be combined into a 3D raster array with
*[r.to.rast3](r.to.rast3.md)*, or for filtering outliers on relatively
flat terrain.

In varied terrain the user may find that *min* maps make for a good
noise filter as most LIDAR noise is from premature hits. The *min* map
may also be useful to find the underlying topography in a forested or
urban environment if the cells are over sampled.

The user can use a combination of *r.in.xyz* **output** maps to create
custom filters. e.g. use *[r.mapcalc](r.mapcalc.md)* to create a
`mean-(2*stddev)` map. \[In this example the user may want to include a
lower bound filter in *[r.mapcalc](r.mapcalc.md)* to remove highly
variable points (small *n*) or run *[r.neighbors](r.neighbors.md)* to
smooth the stddev map before further use.\]

### Alternate value column

The **value_column** parameter can be used in specialized cases when you
want to filter by z-range but bin and store another column's data. For
example if you wanted to look at backscatter values between 1000 and
1500 meters elevation. This is particularly useful when using *r.in.xyz*
to prepare depth slices for a 3D raster — the **zrange** option defines
the depth slice but the data values stored in the voxels describe an
additional dimension. As with the z column, a filtering range and
scaling factor may be applied.

### Reprojection

If the raster map is to be reprojected, it may be more appropriate to
reproject the input points with *[m.proj](m.proj.md)* or *cs2cs* before
running *r.in.xyz*.

### Interpolation into a DEM

The vector engine's topographic abilities introduce a finite memory
overhead per vector point which will typically limit a vector map to
approximately 3 million points (~ 1750^2 cells). If you want more, use
the *[r.to.vect](r.to.vect.md)* **-b** flag to skip building topology.
Without topology, however, all you'll be able to do with the vector map
is display with *[d.vect](d.vect.md)* and interpolate with
*[v.surf.rst](v.surf.rst.md)*. Run *[r.univar](r.univar.md)* on your
raster map to check the number of non-NULL cells and adjust bounds
and/or resolution as needed before proceeding.

Typical commands to create a DEM using a regularized spline fit:

```sh
r.univar lidar_min
r.to.vect -z type=point in=lidar_min out=lidar_min_pt
v.surf.rst in=lidar_min_pt elev=lidar_min.rst
```

### Import of x,y,string data

*r.in.xyz* is expecting numeric values as z column. In order to perform
a occurrence count operation even on x,y data with non-numeric
attribute(s), the data can be imported using either the x or y
coordinate as a fake z column for **method**=`n` (count number of points
per grid cell), the z values are ignored anyway.

## EXAMPLES

### Import of x,y,z ASCII into DEM

Sometimes elevation data are delivered as x,y,z ASCII files instead of a
raster matrix. The import procedure consists of a few steps: calculation
of the map extent, setting of the computational region accordingly with
an additional extension into all directions by half a raster cell in
order to register the elevation points at raster cell centers.

Note: if the z column is separated by several spaces from the coordinate
columns, it may be sufficient to adapt the **z** position value.

```sh
# Important: observe the raster spacing from the ASCII file:
# ASCII file format (example):
# 630007.5 228492.5 141.99614
# 630022.5 228492.5 141.37904
# 630037.5 228492.5 142.29822
# 630052.5 228492.5 143.97987
# ...
# In this example the distance is 15m in x and y direction.

# detect extent, print result as g.region parameters
r.in.xyz input=elevation.xyz separator=space -s -g
# ... n=228492.5 s=215007.5 e=644992.5 w=630007.5 b=55.578793 t=156.32986

# set computational region, along with the actual raster resolution
# as defined by the point spacing in the ASCII file:
g.region n=228492.5 s=215007.5 e=644992.5 w=630007.5 res=15 -p

# now enlarge computational region by half a raster cell (here 7.5m) to
# store the points as cell centers:
g.region n=n+7.5 s=s-7.5 w=w-7.5 e=e+7.5 -p

# import XYZ ASCII file, with z values as raster cell values
r.in.xyz input=elevation.xyz separator=space method=mean output=myelev

# univariate statistics for verification of raster values
r.univar myelev
```

### Import of LiDAR data and DEM creation

Import the [Jockey's Ridge, NC, LIDAR
dataset](https://grassbook.org/ncexternal/index.html) (compressed file
"lidaratm2.txt.gz"), and process it into a clean DEM:

```sh
# scan and set region bounds
r.in.xyz -s -g separator="," in=lidaratm2.txt
g.region n=35.969493 s=35.949693 e=-75.620999 w=-75.639999
g.region res=0:00:00.075 -a

# create "n" map containing count of points per cell for checking density
r.in.xyz in=lidaratm2.txt out=lidar_n separator="," method=n zrange=-2,50

# check point density [rho = n_sum / (rows*cols)]
r.univar lidar_n
# create "min" map (elevation filtered for premature hits)
r.in.xyz in=lidaratm2.txt out=lidar_min separator="," method=min zrange=-2,50

# set computational region to area of interest
g.region n=35:57:56.25N s=35:57:13.575N w=75:38:23.7W e=75:37:15.675W

# check number of non-null cells (try and keep under a few million)
r.univar lidar_min

# convert to points
r.to.vect -z type=point in=lidar_min out=lidar_min_pt

# interpolate using a regularized spline fit
v.surf.rst in=lidar_min_pt elev=lidar_min.rst

# set color scale to something interesting
r.colors lidar_min.rst rule=bcyr -n -e

# prepare a 1:1:1 scaled version for NVIZ visualization (for lat/lon input)
r.mapcalc "lidar_min.rst_scaled = lidar_min.rst / (1852*60)"
r.colors lidar_min.rst_scaled rule=bcyr -n -e
```

## TODO

- Support for multiple map output from a single run.  
  `method=string[,string,...] output=name[,name,...]`  
  This can be easily handled by a wrapper script, with the added benefit
  of it being very simple to parallelize that way.

## KNOWN ISSUES

- "`nan`" can leak into *coeff_var* maps.  
  Cause unknown. Possible work-around: "`r.null setnull=nan`"

If you encounter any problems (or solutions!) please contact the GRASS
Development Team.

## SEE ALSO

*[g.region](g.region.md), [m.proj](m.proj.md),
[r.fillnulls](r.fillnulls.md), [r.in.ascii](r.in.ascii.md),
[r.in.pdal](r.in.pdal.md), [r3.in.xyz](r3.in.xyz.md),
[r.mapcalc](r.mapcalc.md), [r.neighbors](r.neighbors.md),
[r.out.xyz](r.out.xyz.md), [r.to.rast3](r.to.rast3.md),
[r.to.vect](r.to.vect.md), [r.univar](r.univar.md),
[v.in.ascii](v.in.ascii.md), [v.surf.rst](v.surf.rst.md)*

*[v.lidar.correction](v.lidar.correction.md),
[v.lidar.edgedetection](v.lidar.edgedetection.md),
[v.lidar.growing](v.lidar.growing.md), [v.outlier](v.outlier.md),
[v.surf.bspline](v.surf.bspline.md)*

*[pv](http://www.ivarch.com/programs/pv.shtml)* - The UNIX pipe viewer
utility

Overview: [Interpolation and
Resampling](https://grasswiki.osgeo.org/wiki/Interpolation) in GRASS GIS

## AUTHORS

Hamish Bowman, Department of Marine Science, University of Otagom New
Zealand  
Extended by Volker Wichmann to support the aggregate functions *median,
percentile, skewness* and *trimmed mean*.

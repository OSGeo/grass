---
description: Raster data processing in GRASS GIS
index: raster
---

# Raster data processing in GRASS GIS

## Raster maps in general

A "raster map" is a data layer consisting of a gridded array of cells.
It has a certain number of rows and columns, with a data point (or null
value indicator) in each cell. These may exist as a 2D grid or as a 3D
cube made up of many smaller cubes, i.e. a stack of 2D grids.

The geographic boundaries of the raster map are described by the north,
south, east, and west fields. These values describe the lines which
bound the map at its edges. These lines do NOT pass through the center
of the grid cells at the edge of the map, but along the edge of the map
itself. i.e. the geographic extent of the map is described by the outer
bounds of all cells within the map.

As a general rule in GRASS GIS:

1. Raster output maps have their bounds and resolution equal to those
    of the current computational region.
2. Raster input maps are automatically cropped/padded and rescaled
    (using nearest-neighbour resampling) to match the current region.
3. Processing NULL (no data) values produces NULL values.
4. Input raster maps are automatically masked if a raster mask is active,
   The mask is managed by the [r.mask](r.mask.md) tool, and
   it is represented by a raster map called `MASK` by default.
   Unless specified otherwise, the raster mask is only applied
   when *reading* raster maps which typically results in NULL values
   in the output for areas outside of the mask.

There are a few exceptions to this: `r.in.*` programs read the data
cell-for-cell, with no resampling. When reading non-georeferenced data,
the imported map will usually have its lower-left corner at (0,0) in the
project's coordinate system; the user needs to use
[r.region](r.region.md) to "place" the imported map.

Some programs which need to perform specific types of resampling (e.g.
[r.resamp.rst](r.resamp.rst.md)) read the input maps at their original
resolution then do the resampling themselves.

[r.proj](r.proj.md) has to deal with two regions (source and
destination) simultaneously; both will have an impact upon the final
result.

## Raster import and export

The module [r.in.gdal](r.in.gdal.md) offers a common interface for many
different raster formats. Additionally, it also offers options such as
on-the-fly project creation or extension of the default region to match
the extent of the imported raster map. For special cases, other import
modules are available. The full map is always imported.

For importing scanned maps, the user will need to create a x,y-project,
scan the map in the desired resolution and save it into an appropriate
raster format (e.g. tiff, jpeg, png, pbm) and then use
[r.in.gdal](r.in.gdal.md) to import it. Based on reference points the
scanned map can be recified to obtain geocoded data.

Raster maps are exported with [r.out.gdal](r.out.gdal.md) into common
formats. Also [r.out.bin](r.out.bin.md), [r.out.vtk](r.out.vtk.md),
[r.out.ascii](r.out.ascii.md) and other export modules are available.
They export the data according to the current region settings. If those
differ from the original map, the map is resampled on the fly (nearest
neighbor algorithm). In other words, the output will have as many rows
and columns as the current region. To export maps with various grid
spacings (e.g, 500x500 or 200x500), you can just change the region
resolution with [g.region](g.region.md) and then export the map. The
resampling is done with nearest neighbor algorithm in this case. If you
want some other form of resampling, first change the region, then
explicitly resample the map with e.g.
[r.resamp.interp](r.resamp.interp.md) or
[r.resamp.stats](r.resamp.stats.md), then export the resampled map.

GRASS GIS raster map exchange between different projects with the same
CRS can be done in a lossless way using the [r.pack](r.pack.md) and
[r.unpack](r.unpack.md) modules.

## Metadata

The [r.info](r.info.md) module displays general information about a map
such as region extent, data range, data type, creation history, and
other metadata. Metadata such as map title, units, vertical datum etc.
can be updated with [r.support](r.support.md). Timestamps are managed
with [r.timestamp](r.timestamp.md). Region extent and resolution are
managed with [r.region](r.region.md).

## Raster map operations

### Resampling methods and interpolation methods

GRASS raster map processing is always performed in the current region
settings (see [g.region](g.region.md)), i.e. the current region extent
and current raster resolution is used. If the resolution differs from
that of the input raster map(s), on-the-fly resampling is performed
(nearest neighbor resampling). If this is not desired, the input map(s)
has/have to be resampled beforehand with one of the dedicated modules.

The built-in nearest-neighbour resampling of raster data calculates the
centre of each region cell, and takes the value of the raster cell in
which that point falls.

If the point falls exactly upon a grid line, the exact result will be
determined by the direction of any rounding error. One consequence of
this is that downsampling by a factor which is an even integer will
always sample exactly on the boundary between cells, meaning that the
result is ill-defined.

The following modules are available for reinterpolation of "filled"
raster maps (continuous data) to a different resolution:

- [r.resample](r.resample.md) uses the built-in resampling, so it should
  produce identical results as the on-the-fly resampling done via the
  raster import modules.
- [r.resamp.interp](r.resamp.interp.md) Resampling with nearest
  neighbor, bilinear, and bicubic method: **method=nearest** uses the
  same algorithm as [r.resample](r.resample.md), but not the same code,
  so it may not produce identical results in cases which are decided by
  the rounding of floating-point numbers.  
  For [r.resamp.interp](r.resamp.interp.md) **method=bilinear** and
  **method=bicubic**, the raster values are treated as samples at each
  raster cell's centre, defining a piecewise-continuous surface. The
  resulting raster values are obtained by sampling the surface at each
  region cell's centre. As the algorithm only interpolates, and doesn't
  extrapolate, a margin of 0.5 (for bilinear) or 1.5 (for bicubic) cells
  is lost from the extent of the original raster. Any samples taken
  within this margin will be null.
- [r.resamp.rst](r.resamp.rst.md) Regularized Spline with Tension (RST)
  interpolation 2D: Behaves similarly, i.e. it computes a surface
  assuming that the values are samples at each raster cell's centre, and
  samples the surface at each region cell's centre.
- [r.resamp.bspline](r.resamp.bspline.md) Bicubic or bilinear spline
  interpolation with Tykhonov regularization.
- For [r.resamp.stats](r.resamp.stats.md) without **-w**, the value of
  each region cell is the chosen aggregate of the values from all of the
  raster cells whose centres fall within the bounds of the region
  cell.  
  With **-w**, the samples are weighted according to the proportion of
  the raster cell which falls within the bounds of the region cell, so
  the result is normally unaffected by rounding error (a minuscule
  difference in the position of the boundary results in the addition or
  subtraction of a sample weighted by a minuscule factor; also, The min
  and max aggregates can't use weights, so **-w** has no effect for
  those).
- [r.fillnulls](r.fillnulls.md) for Regularized Spline with Tension
  (RST) interpolation 2D for hole filling (e.g., SRTM DEM)

Furthermore, there are modules available for reinterpolation of "sparse"
(scattered points or lines) maps:

- Inverse distance weighted average (IDW) interpolation
  ([r.surf.idw](r.surf.idw.md))
- Interpolating from contour lines ([r.contour](r.contour.md))
- Various vector modules for interpolation

For Lidar and similar data, [r.in.pdal](r.in.pdal.md) and
[r.in.xyz](r.in.xyz.md) support loading and binning of ungridded x,y,z
ASCII data into a new raster map. The user may choose from a variety of
statistical methods in creating the new raster map.

Otherwise, for interpolation of scattered data, use the *v.surf.\** set
of modules.

### Raster masks

If a raster map named "MASK" exists, most GRASS raster modules will
operate only on data falling inside the masked area, and treat any data
falling outside of the mask as if its value were NULL. The mask is only
applied when *reading* an existing GRASS raster map, for example when
used in a module as an input map.
While the mask raster map can be managed directly,
the [r.mask](r.mask.md) tool is a convenient way to create
and manage masks.

Alternatively, `GRASS_MASK` environment variable can be used to specify
the raster map which will be used as a mask.

The mask is read as an integer map. If the mask raster is actually a floating-point
map, the values will be converted to integers using the map's
quantisation rules (this defaults to round-to-nearest, but can be
changed with r.quant).

(see [r.mask](r.mask.md))

## Raster map statistics

A couple of commands are available to calculate local statistics
([r.neighbors](r.neighbors.md)), and global statistics
([r.statistics](r.statistics.md), [r.surf.area](r.surf.area.md)).
Profiles and transects can be generated ([d.profile](d.profile.md),
[r.profile](r.profile.md), [r.transect](r.transect.md)) as well as
histograms ([d.histogram](d.histogram.md)) and polar diagrams
([d.polar](d.polar.md)). Univariate statistics ([r.univar](r.univar.md))
and reports are also available ([r.report](r.report.md),
[r.stats](r.stats.md), [r.volume](r.volume.md)). Since
[r.univar](r.univar.md) may be slow for extended statistics these can be
calculated using [r.stats.quantile](r.stats.quantile.md). Without a
zones input raster, the [r.quantile](r.quantile.md) module will be
significantly more efficient for calculating percentiles with large
maps. For calculating univariate statistics from a raster map based on
vector polygon map and upload statistics to new attribute columns, see
[v.rast.stats](v.rast.stats.md). Category or object oriented statistics
can be computed with [r.statistics](r.statistics.md). For floating-point
cover map support for this, see the alternative
[r.stats.zonal](r.stats.zonal.md). For quantile calculations with
support for floating-point cover maps, see the alternative
[r.stats.quantile](r.stats.quantile.md).

## Raster map algebra and aggregation

The [r.mapcalc](r.mapcalc.md) command provides raster map algebra
methods. The [r.resamp.stats](r.resamp.stats.md) command resamples
raster map layers using various aggregation methods, the
[r.statistics](r.statistics.md) command aggregates one map based on a
second map. [r.resamp.interp](r.resamp.interp.md) resamples raster map
layers using interpolation.

## Regression analysis

Both linear ([r.regression.line](r.regression.line.md)) and multiple
regression ([r.regression.multi](r.regression.multi.md)) are supported.

## Hydrologic modeling toolbox

Watershed modeling related modules are
[r.basins.fill](r.basins.fill.md), [r.water.outlet](r.water.outlet.md),
[r.watershed](r.watershed.md), and [r.terraflow](r.terraflow.md). Water
flow related modules are [r.carve](r.carve.md), [r.drain](r.drain.md),
[r.fill.dir](r.fill.dir.md), [r.fillnulls](r.fillnulls.md),
[r.flow](r.flow.md), and [r.topidx](r.topidx.md). Flooding can be
simulated with [r.lake](r.lake.md). Hydrologic simulation model are
available as [r.sim.sediment](r.sim.sediment.md),
[r.sim.water](r.sim.water.md), and [r.topmodel](r.topmodel.md).

## Raster format

In GRASS GIS, raster data can be stored as 2D or 3D grids.

### 2D raster maps

2D rasters support three data types (for technical details, please refer
to the Wiki article [GRASS raster
semantics](https://grasswiki.osgeo.org/wiki/GRASS_raster_semantics)):

- 32bit signed integer (CELL),
- single-precision floating-point (FCELL), and
- double-precision floating-point (DCELL).

In most GRASS GIS resources, 2D raster maps are usually called "raster"
maps.

### 3D raster maps

The 3D raster map type is usually called "3D raster" but other names
like "RASTER3D", "voxel", "volume", "GRID3D" or "3d cell" are yet
common. 3D rasters support only single- and double-precision
floating-point. 3D raster's single-precision data type is most often
called "float", and the double-precision one "double".

### No-data management and data portability

GRASS GIS distinguishes NULL and zero. When working with NULL data, it
is important to know that operations on NULL cells lead to NULL cells.

The GRASS GIS raster format is architecture independent and portable
between 32bit and 64bit machines.

## Raster compression

All GRASS GIS raster map types are by default ZSTD compressed if
available, otherwise ZLIB compressed. Through the environment variable
`GRASS_COMPRESSOR` the compression method can be set to RLE, ZLIB, LZ4,
BZIP2, or ZSTD.

Important: the NULL file compression can be turned off with
`export GRASS_COMPRESS_NULLS=0`. Raster maps with NULL file compression
can only be opened with GRASS GIS 7.2.0 or later. NULL file compression
for a particular raster map can be managed with **r.null -z**.

Integer (CELL type) raster maps can be compressed with RLE if the
environment variable `GRASS_COMPRESSOR` exists and is set to RLE.
However, this is not recommended.

Floating point (FCELL, DCELL) raster maps never use RLE compression;
they are either compressed with ZLIB, LZ4, BZIP2, ZSTD or are
uncompressed.

**RLE**  
**DEPRECATED** Run-Length Encoding, poor compression ratio but fast. It
is kept for backwards compatibility to read raster maps created with
GRASS 6. It is only used for raster maps of type CELL. FCELL and DCELL
maps are never and have never been compressed with RLE.

**ZLIB**  
ZLIB's deflate is the default compression method for all raster maps, if
ZSTD is not available. GRASS GIS 8 uses by default 1 as ZLIB compression
level which is the best compromise between speed and compression ratio,
also when compared to other available compression methods. Valid levels
are in the range \[1, 9\] and can be set with the environment variable
`GRASS_ZLIB_LEVEL`.

**LZ4**  
LZ4 is a very fast compression method, about as fast as no compression.
Decompression is also very fast. The compression ratio is generally
higher than for RLE but worse than for ZLIB. LZ4 is recommended if disk
space is not a limiting factor.

**BZIP2**  
BZIP2 can provide compression ratios much higher than the other methods,
but only for large raster maps (\> 10000 columns). For large raster
maps, disk space consumption can be reduced by 30 - 50% when using BZIP2
instead of ZLIB's deflate. BZIP2 is the slowest compression and
decompression method. However, if reading from / writing to a storage
device is the limiting factor, BZIP2 compression can speed up raster map
processing. Be aware that for smaller raster maps, BZIP2 compression
ratio can be worse than other compression methods.

**ZSTD**  
ZSTD (Zstandard) provides compression ratios higher than ZLIB but lower
than BZIP2 (for large data). ZSTD compresses up to 4x faster than ZLIB,
and usually decompresses 6x faster than ZLIB. ZSTD is the default
compression method if available.

In the internal cellhd file, the value for "compressed" is 1 for RLE, 2
for ZLIB, 3 for LZ4,4 for BZIP2, and 5 for ZSTD.

Obviously, decompression is controlled by the raster map's compression,
not the environment variable.

## See also

- [Introduction into 3D raster data (voxel)
  processing](raster3dintro.md)
- [Introduction into vector data processing](vectorintro.md)
- [Introduction into image processing](imageryintro.md)
- [Introduction into temporal data processing](temporalintro.md)
- [Database management](databaseintro.md)
- [Projections and spatial transformations](projectionintro.md)

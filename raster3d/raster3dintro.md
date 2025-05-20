---
description: 3D raster data in GRASS GIS
index: raster3d
---

# 3D raster data in GRASS GIS

## 3D raster maps in general

GRASS GIS is one of the few GIS software packages with 3D raster data
support. Data are stored as a 3D raster with 3D cells of a given volume.
3D rasters are designed to support representations of trivariate
continuous fields. The vertical dimension supports spatial and temporal
units. Hence space time 3D raster with different temporal resolutions
can be created and processed.

GRASS GIS 3D raster maps use the same coordinate system as 2D raster
maps (row count from north to south) with an additional z dimension
(depth) counting from bottom to top. The upper left corner (NW) is the
origin. 3D rasters are stored using a tile cache based approach. This
allows arbitrary read and write operations in the created 3D raster. The
size of the tiles can be specified at import time with a given import
module such as [r3.in.ascii](r3.in.ascii.md) or the data can be retiled
using [r3.retile](r3.retile.md) after import or creation.

![3D raster map coordinate system](raster3d_layout.png)  
*The 3D raster map coordinate system and the internal tile layout of the
RASTER3D library*

## Terminology and naming

In GRASS GIS terminology, continuous 3D data represented by regular grid
or lattice is called *3D raster map*. 3D raster map works in 3D in the
same was as (2D) raster map in 2D, so it is called the same except for
the additional 3D. Some literature or other software may use terms such
as 3D grid, 3D lattice, 3D matrix, 3D array, volume, voxel, voxel model,
or voxel cube. Note that terms volume and volumetric often refer to
measuring volume (amount) of some substance which may or may not be
related to 3D rasters.

Note that GRASS GIS uses the term 3D raster map or just 3D raster for
short, rather than 3D raster layer because term map emphasizes the
mapping of positions to values which is the purpose of 3D raster map (in
mathematics, map or mapping is close to a term function) On the other
hand, the term layer emphasizes overlaying or stacking up. The former is
not the only operation done with data and the latter could be confusing
in case of 3D raster data.

3D raster map is divided into cells in the same way as the (2D) raster
map. A cell is a cube or a (rectangular) cuboid depending on the
resolution. The resolution influences volume of one cell. Some
literature or other software may use terms such as volume, volume unit,
volumetric pixel, volume pixel, or voxel. Note that voxel can be
sometimes used to refer to a whole 3D raster and that for example in 3D
computer graphics, voxel can denote object with some complicated shape.

Type of map and element name in GRASS GIS is called `raster_3d`. The
module family prefix is `r3`. Occasionally, 3D raster related things can
be referred differently, for example according to a programming language
standards. This might be the case of some functions or classes in
Python.

In GRASS GIS 3D rasters as stored in tiles which are hidden from user
most of the time. When analyzing or visualizing 3D rasters user can
create slices or cross sections. Slices can be horizontal, vertical, or
general plains going through a 3D raster. Slices, especially the
horizontal ones, may be called layers in some literature or some other
software. Cross sections are general functions, e.g. defined by 2D
raster, going through a 3D raster. Another often used term is an
isosuface which has the same relation to 3D raster as contour (isoline)
to a 2D raster. An isosurface is a surface that represent places with a
constant value.

When 3D raster is used in the way that vertical dimension represents
time 3D raster can be referred to as space time cubes (STC) or space
time cube 3D raster. Some literature may also use space time voxel cube,
space time voxel model or some other combination.

## 3D raster import

### Import from external files

The modules [r3.in.ascii](r3.in.ascii.md) and [r3.in.bin](r3.in.bin.md)
supports generic x,y,z ASCII and binary array import.

In case of CSV tables, the modules [v.in.ascii](v.in.ascii.md) (using
the **-z** flag) may be a choice to first import the 3D points as vector
points and the convert them to 3D raster (see below).

Import of 3D (LiDAR) points and their statistics can be done using
[r3.in.lidar](r3.in.lidar.md) for LiDAR data and
[r3.in.xyz](r3.in.xyz.md) for CSV and other ASCII text formats.

### Conversion from 3D vector points

3D rasters can be generated from 3D point vector data
([v.to.rast3](v.to.rast3.md)). Always the full map is imported.

### Conversion from 2D raster maps

3D raster can also be created based on 2D elevation map(s) and value
raster map(s) ([r.to.rast3elev](r.to.rast3elev.md)). Alternatively, a 3D
raster can be composed of several 2D raster maps (stack of maps). 2D
rasters are considered as slices in this case and merged into one 3D
raster map ([r.to.rast3](r.to.rast3.md)).

## 3D region settings and 3D mask

GRASS GIS 3D raster map processing is always performed in the current 3D
region settings (see [g.region](g.region.md), *-p3* flags), i.e. the
current region extent, vertical extent and current 3D resolution are
used. If the 3D resolution differs from that of the input raster map(s),
on-the-fly resampling is performed (nearest neighbor resampling). If
this is not desired, the input map(s) has/have to be reinterpolated
beforehand with one of the dedicated modules. Masks can be set
([r3.mask](r3.mask.md)).

## 3D raster analyses and operations

Powerful 3D raster map algebra is implemented in
[r3.mapcalc](r3.mapcalc.md). A 3D groundwater flow model is implemented
in [r3.gwflow](r3.gwflow.md).

## 3D raster conversion to vector or 2D raster maps

Slices from a 3D raster map can be converted to a 2D raster map
([r3.to.rast](r3.to.rast.md)). Cross sectional 2D raster map can be
extracted from 3D raster map based on a 2D elevation map
([r3.cross.rast](r3.cross.rast.md)).

## 3D raster statistics

3D raster statistics can be calculated with [r3.stats](r3.stats.md) and
[r3.univar](r3.univar.md).

## 3D raster interpolation

From 3D vector points, GRASS 3D raster maps can be interpolated
([v.vol.rst](v.vol.rst.md)). Results are 3D raster maps, however 2D
raster maps can be also extracted.

## 3D raster export

The modules [r3.out.ascii](r3.out.ascii.md) and
[r3.out.bin](r3.out.bin.md) support the export of 3D raster maps as
ASCII or binary files. The output of these modules can be imported with
the corresponding import modules noted above.

NetCDF export of 3D raster maps can be performed using the module
[r3.out.netcdf](r3.out.netcdf.md). It supports 3D raster maps with
spatial dimensions and temporal (vertical) dimension.

## Working with 3D visualization software

GRASS GIS can be used for visualization of 3D rasters, however it has
also tools to easily export the data into other visualization packages.

GRASS GIS 3D raster maps can be exported to VTK using
[r3.out.vtk](r3.out.vtk.md). VTK files can be visualized with the *[VTK
Toolkit](https://vtk.org)*, *[Paraview](https://www.paraview.org)* and
*[MayaVi](https://github.com/enthought/mayavi)*. Moreover, GRASS GIS 2D
raster maps can be exported to VTK with [r.out.vtk](r.out.vtk.md) and
GRASS GIS vector maps can be exported to VTK with
[v.out.vtk](v.out.vtk.md).

Alternatively, GRASS 3D raster maps can be imported and exported from/to
*[Vis5D](https://vis5d.sourceforge.net/)* ([r3.in.v5d](r3.in.v5d.md),
[r3.out.v5d](r3.out.v5d.md)).

## 3D raster data types

3D raster's single-precision data type is most often called "FCELL" or
"float", and the double-precision one "DCELL" or "double".

## See also

- [Introduction into raster data processing](rasterintro.md)
- [Introduction into vector data processing](vectorintro.md)
- [Introduction into image processing](imageryintro.md)
- [Introduction into temporal data processing](temporalintro.md)
- [Projections and spatial transformations](projectionintro.md)
- [wxGUI 3D View Mode](wxGUI.nviz.md)
- *[m.nviz.image](m.nviz.image.md)*

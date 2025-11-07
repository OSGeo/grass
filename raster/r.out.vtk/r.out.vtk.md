## DESCRIPTION

Outputs *raster* maps in *VTK-ASCII* format. *Map's* are valid raster
map's in the current mapset. *output* is the name of an VTK-ASCII file
which will be written in the current working directory. If *output* is
not specified then **stdout** is used. The module is sensitive to region
settings (set with g.region).

Elevation, scaling, point/celldata, vector and RGB Data are supported.
If the map is in LL projection, the elevation values will automatically
scaled to degrees. It is supposed that the elevation values are provided
in meters. If the elevation values are in a different unit than meters,
use the scale parameter to convert the units.

If no elevation map is given, the user can set the height of the map by
one value. Point or cell data are available. Also scaling is supported
for this elevation value. The elevation value must be provided in
meters.

The RGB input requires three raster maps: red, green, blue - in this
order. The maps must have values between 0 and 255, otherwise you will
get lots of warnings and the values are set to 0. More than one RGB
dataset (3 maps) is not supported.

The vector input requires three raster maps: x, y, z -- defining the
vector coordinates - in this order. More than one vector dataset (3
maps) is not supported.

## NOTES

This filter generates:

- *structured points* with *celldata* or *pointdata* if no elevationfile
  is given
- *structured grid* (not recommendet) with *pointdata* if an
  elevationfile is given
- *polydataset* with *pointdata* if an elevationfile is given (default)

and puts this in a simple VTK-ASCII file. Nor XML or binary output are
supported. It is possible to choose more then one raster map to be
written to the VTK-ASCII file. Each cell-/pointdata is named like the
raster map it represents. You can visualize this file with the *[VTK
Toolkit](https://vtk.org/)*, *[Paraview](https://www.paraview.org/)* and
*[MayaVi](https://github.com/enthought/mayavi)* which are based on VTK.
If you have a raster map with partly no data, use the threshold filter
in paraview to visualize the valid data. Just filter all data which is
greater/lesser than the chosen null value in the VTK-ASCII file.  
If elevation map is chosen, a polygonal grid is created with *quads*,
but the user can choose also *triangle strips* or *vertices*. These
dataformats a documented at *[VTK Toolkit](https://vtk.org/)*.

If the "-c" flag is used and the data should be visualised together with
other data exported via \*.out.vtk modules, be sure the "-c" flag was
also set in these modules. But this will only work with data from the
SAME location (The reference point for the coordinates transformation is
based on the center point of the default region).

### Difference between point- and celldata

r.out.vtk can export raster cells with different representations.

- *pointdata* -- the cells/values are represented by the center of the
  cell. Instead of cells, points are created. Each point can hold
  different values, but the user can only visualize one value at a time.
  These points can be connected in different ways.
- *celldata* -- is only provided if no elevation map is given. The cells
  are created with the same height and width as in GRASS. Each cell can
  hold different values, but the user can only visualize one value at a
  time.

### Paraview RGB visualization notes

To achieve proper RGB overlay:

- In Paraview, click "Apply"
- Select the "Display" tab and choose "Color by" to switch from input
  scalars to rgb scalars
- Disable the "Map Scalars" check button in the display tab to avoid the
  use of a lookup table

## EXAMPLE

### Simple Spearfish example

```sh
# set region
g.region n=4926970 s=4914857 w=591583 e=607793 res=50 -p

# export the data
r.out.vtk input=elevation.10m,slope,aspect elevation=elevation.10m output=/tmp/out.vtk

# visualize in Paraview or other VTK viewer:
paraview --data=/tmp/out.vtk
```

### Spearfish example with RGB data

```sh
#set the region
g.region n=4926990 s=4914840 w=591570 e=607800 res=30 -p

# using r.in.wms to create RGB data to get a satellite coverage
r.in.wms layers=global_mosaic mapserver=http://wms.jpl.nasa.gov/wms.cgi \
         output=wms_global_mosaic

# export the data to VTK
r.out.vtk rgbmaps=wms_global_mosaic.red,wms_global_mosaic.green,wms_global_mosaic.blue \
          elevation=elevation.10m output=/tmp/out.vtk

# visualize in Paraview or other VTK viewer:
paraview --data=/tmp/out.vtk
```

## SEE ALSO

*[r3.out.vtk](r3.out.vtk.md), [r.out.ascii](r.out.ascii.md),
[g.region](g.region.md)*  
[GRASS and Paraview Wiki
page](https://grasswiki.osgeo.org/wiki/GRASS_and_Paraview)

## AUTHOR

Soeren Gebbert

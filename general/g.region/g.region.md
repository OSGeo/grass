## DESCRIPTION

The *g.region* module allows the user to manage the settings of the
current geographic region. These regional boundaries can be set by the
user directly and/or set from a region definition file (stored under the
`windows` directory in the user's current mapset). The user can create,
modify, and store as many geographic region definitions as desired for
any given mapset. However, only one of these geographic region
definitions will be current at any given moment, for a specified mapset;
i.e., GRASS programs that respect the geographic region settings will
use the current geographic region settings.

## DEFINITIONS

### Region

In GRASS, a *region* refers to a geographic area with some defined
boundaries, based on a specific map coordinate system and map
projection. Each region also has associated with it the specific
east-west and north-south resolutions of its smallest units (rectangular
units called "cells").

The region's boundaries are given as the northernmost, southernmost,
easternmost, and westernmost points that define its extent (cell edges).
The north and south boundaries are commonly called *northings*, while
the east and west boundaries are called *eastings*.

The region's cell resolution defines the size of the smallest piece of
data recognized (imported, analyzed, displayed, stored, etc.) by GRASS
modules affected by the current region settings. The north-south and
east-west cell resolutions need not be the same, thus allowing
non-square data cells to exist.

Typically all raster and display modules are affected by the current
region settings, but not vector modules. Some special modules diverge
from this rule, for example raster import modules and *v.in.region*.

### Default Region

Each GRASS project (previously called location) has a fixed geographic
region, called the default geographic region (stored in the region file
`DEFAULT_WIND` under the special mapset `PERMANENT`), that defines the
extent of the data base. While this provides a starting point for
defining new geographic regions, user-defined geographic regions need
not fall within this geographic region. The current region can be reset
to the default region with the **-d** flag. The default region is
initially set when the project is first created and can be reset using
the **-s** flag.

### Current Region

Each mapset has a current geographic region. This region defines the
geographic area in which all GRASS displays and raster analyses will be
done. Raster data will be resampled, if necessary, to meet the cell
resolutions of the current geographic region setting.

### Saved Region

Each GRASS MAPSET may contain any number of pre-defined, and named,
geographic regions. These region definitions are stored in the user's
current mapset location under the `windows` directory (also referred to
as the user's saved region definitions). Any of these pre-defined
geographic regions may be selected, by name, to become the current
geographic region. Users may also access saved region definitions stored
under other mapsets in the current project, if these mapsets are
included in the user's mapset search path or the '@' operator is used
(`region_name@mapset`).

## NOTES

After all updates have been applied, the current region's southern and
western boundaries are (silently) adjusted so that the north/south
distance is a multiple of the north/south resolution and that the
east/west distance is a multiple of the east/west resolution.

With the **-a** flag all four boundaries are adjusted to be even
multiples of the resolution, aligning the region to the resolution
supplied by the user. The default is to align the region resolution to
match the region boundaries.

The **-m** flag will report the region resolution in meters. The
resolution is calculated by averaging the resolution at the region
boundaries. This resolution is calculated by dividing the geodesic
distance in meters at the boundary by the number of rows or columns. For
example the east / west resolution (ewres) is determined from an average
of the geodesic distances at the North and South boundaries divided by
the number of columns.

The **-p** (or **-g**) option is recognized last. This means that all
changes are applied to the region settings before printing occurs.

The **-g** flag prints the current region settings in shell script
style. This format can be given back to *g.region* on its command line.
This may also be used to save region settings as shell environment
variables with the UNIX eval command, "`` eval `g.region -g` ``".

With **-u** flag current region is not updated even if one or more
options for changing region is used (**res=**, **raster=**, etc). This
can be used for example to print modified region values for further use
without actually modifying the current region. Similarly, **-o** flag
forces to update current region file even when e.g., only printing was
specified. Flag **-o** was added in GRASS GIS version 8 to simulate
*g.region* behavior in prior versions when current region file was
always updated unless **-u** was specified.

### Additional parameter information

Option **zoom** shrinks current region settings to the smallest region
encompassing all non-NULL data in the named raster map layer that fall
inside the user's current region. In this way you can tightly zoom in on
isolated clumps within a bigger map.

If the user also includes the **raster** option on the command
line, **zoom** will set the current region settings to the
smallest region encompassing all non-NULL data in the named **zoom** map
that fall inside the region stated in the cell header for the named
**raster** map.

Option **align** sets the current resolution equal to that of the provided
raster map, and align the current region to a row and column edge in the
named map. Alignment only moves the existing region edges outward to the edges
of the next nearest cell in the named raster map - not to the named map's
edges. To perform the latter function, use the **raster**=*name* option.

## EXAMPLES

### Printing extent and raster resolution in 2D and 3D

This will print the current region in the format:

```sh
g.region -p
```

```sh
projection: 1 (UTM)
zone:       13
datum:      nad27
ellipsoid:  clark66
north:      4928000
south:      4914000
west:       590000
east:       609000
nsres:      20
ewres:      20
rows:       700
cols:       950
```

This will print the current region and the 3D region (used for voxels)
in the format:

```sh
g.region -p3
```

```sh
projection: 1 (UTM)
zone:       13
datum:      nad27
ellipsoid:  clark66
north:      4928000
south:      4914000
west:       590000
east:       609000
top:        1.00000000
bottom:     0.00000000
nsres:      20
nsres3:     20
ewres:      20
ewres3:     20
tbres:      1
rows:       700
rows3:      700
cols:       950
cols3:      950
depths:     1
```

The **-g** option prints the region in the following script style
(key=value) format:

```sh
g.region -g
```

```sh
n=4928000
s=4914000
w=590000
e=609000
nsres=20
ewres=20
rows=700
cols=950
```

The **-bg** option prints the region in the following script style
(key=value) format plus the boundary box in latitude-longitude/WGS84:

```sh
g.region -bg
```

```sh
n=4928000
s=4914000
w=590000
e=609000
nsres=20
ewres=20
rows=700
cols=950
LL_W=-103.87080682
LL_E=-103.62942884
LL_N=44.50164277
LL_S=44.37302019
```

The **-l** option prints the region in the following format:

```sh
g.region -l
```

```sh
long: -103.86789484 lat: 44.50165890 (north/west corner)
long: -103.62895703 lat: 44.49904013 (north/east corner)
long: -103.63190061 lat: 44.37303558 (south/east corner)
long: -103.87032572 lat: 44.37564292 (south/west corner)
rows:       700
cols:       950
Center longitude: 103:44:59.170374W [-103.74977]
Center latitude:  44:26:14.439781N [44.43734]
```

This will print the current region in the format (latitude-longitude
project):

```sh
g.region -pm
```

```sh
projection: 3 (Latitude-Longitude)
zone:       0
ellipsoid:  wgs84
north:      90N
south:      40N
west:       20W
east:       20E
nsres:      928.73944902
ewres:      352.74269109
rows:       6000
cols:       4800
```

Note that the resolution is here reported in meters, not decimal
degrees.

### Changing extent and raster resolution using values

`g.region n=7360100 e=699000`  
This will reset the northing and easting for the current region, but leave
the south edge, west edge, and the region cell resolutions unchanged.

`g.region n=51:36:05N e=10:10:05E s=51:29:55N w=9:59:55E res=0:00:01`  
This will reset the northing, easting, southing, westing and resolution for
the current region, here in DMS latitude-longitude style (decimal
degrees and degrees with decimal minutes can also be used).

`g.region -dp s=698000`  
This will set the current region from the default region for the GRASS
project, reset the south edge to 698000, and then print the result.

`g.region n=n+1000 w=w-500`  
The n=*value* may also be specified as a function of its current value:
n=n+*value* increases the current northing, while n=n-*value* decreases
it. This is also true for s=*value*, e=*value*, and w=*value*. In this
example the current region's northern boundary is extended by 1000 units
and the current region's western boundary is decreased by 500 units.

`g.region n=s+1000 e=w+1000`  
This form allows the user to set the region boundary values relative to
one another. Here, the northern boundary coordinate is set equal to 1000
units larger than the southern boundary's coordinate value, and the
eastern boundary's coordinate value is set equal to 1000 units larger
than the western boundary's coordinate value. The corresponding forms
`s=n-value` and
`w=e-value` may be used to set the values of the region's southern and
western boundaries, relative to the northern and eastern boundary
values.

### Changing extent and raster resolution using maps

`g.region raster=soils`  
This form will make the current region settings exactly the same as
those given in the cell header file for the raster map layer *soils*.

`g.region raster=soils zoom=soils`  
This form will first look up the cell header file for the raster map
layer *soils*, use this as the current region setting, and then shrink
the region down to the smallest region which still encompasses all
non-NULL data in the map layer *soils*. Note that if the parameter
*raster=soils* were not specified, the zoom would shrink to encompass
all non-NULL data values in the soils map that were located within the
*current region* settings.

`g.region -up raster=soils`  
The **-u** option suppresses the re-setting of the current region
definition. This can be useful when it is desired to only extract region
information. In this case, the cell header file for the soils map layer
is printed without changing the current region settings.

`g.region -up zoom=soils save=soils`  
This will zoom into the smallest region which encompasses all non-NULL
soils data values, and save the new region settings in a file to be
called *soils* and stored under the `windows` directory in the user's
current mapset. The current region settings are not changed.

### Changing extent and raster resolution in 3D

`g.region b=0 t=3000 tbres=200 res3=100 g.region -p3`  
This will define the 3D region for voxel computations. In this example a
volume with bottom (0m) to top (3000m) at horizontal resolution (100m)
and vertical resolution (200m) is defined.

### Using g.region in a shell in combination with OGR

Extracting a spatial subset of the external vector map `soils.shp` into
new external vector map `soils_cut.shp` using the OGR *ogr2ogr* tool:  

```sh
eval `g.region -g`
ogr2ogr -spat $w $s $e $n soils_cut.shp soils.shp
```

This requires that the project and the SHAPE file CRS' match.

### Using g.region in a shell in combination with GDAL

Extracting a spatial subset of the external raster map
`p016r035_7t20020524_z17_nn30.tif` into new external raster map
`p016r035_7t20020524_nc_spm_wake_nn30.tif` using the GDAL *gdalwarp*
tool:  

```sh
eval `g.region -g`
gdalwarp -t_srs "`g.proj -wf`" -te $w $s $e $n \
         p016r035_7t20020524_z17_nn30.tif \
         p016r035_7t20020524_nc_spm_wake_nn30.tif
```

Here the input raster map does not have to match the project's
coordinate reference system since it is reprojected on the fly.

### JSON Output

```sh
g.region -p format=json
```

```sh
{
    "projection": "99 (Lambert Conformal Conic)",
    "zone": 0,
    "datum": "nad83",
    "ellipsoid": "a=6378137 es=0.006694380022900787",
    "region": {
        "north": 320000,
        "south": 10000,
        "west": 120000,
        "east": 935000,
        "ns-res": 500,
        "ns-res3": 1000,
        "ew-res": 500,
        "ew-res3": 1000
    },
    "top": 500,
    "bottom": -500,
    "tbres": 100,
    "rows": 620,
    "rows3": 310,
    "cols": 1630,
    "cols3": 815,
    "depths": 10,
    "cells": 1010600,
    "cells3": 2526500
}
```

```sh
g.region -l format=json
```

```sh
{
    "nw_long": -78.688888505507336,
    "nw_lat": 35.743893244701788,
    "ne_long": -78.669097826118957,
    "ne_lat": 35.743841072010554,
    "se_long": -78.669158624787542,
    "se_lat": 35.728968779193615,
    "sw_long": -78.688945667963168,
    "sw_lat": 35.729020942542441,
    "center_long": -78.679022655614958,
    "center_lat": 35.736431420327719,
    "rows": 165,
    "cols": 179
}
```

## SEE ALSO

*[g.access](g.access.md), [g.mapsets](g.mapsets.md),
[g.proj](g.proj.md)  
Environment variables: [GRASS_REGION and
WIND_OVERRIDE](variables.md#list-of-selected-internal-grass-environment-variables)*

## AUTHOR

Michael Shapiro, U.S.Army Construction Engineering Research Laboratory

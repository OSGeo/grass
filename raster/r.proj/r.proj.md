## DESCRIPTION

*r.proj* is used to reproject a raster map from the coordinate reference
system (CRS) of the input project into a CRS of a specified project
(previously called location). The CRS information is taken from the
current PROJ_INFO files, as set and viewed with *[g.proj](g.proj.md)*.

### Introduction

#### Map projections

Map projections are a method of representing information from a curved
surface (usually a spheroid) in two dimensions, typically to allow
indexing through cartesian coordinates. There are a wide variety of
projections, with common ones divided into a number of classes,
including cylindrical and pseudo-cylindrical, conic and pseudo-conic,
and azimuthal methods, each of which may be conformal, equal-area, or
neither.

The particular projection chosen depends on the purpose of the project,
and the size, shape and location of the area of interest. For example,
normal cylindrical projections are good for maps which are of greater
extent east-west than north-south and in equatorial regions, while conic
projections are better in mid-latitudes; transverse cylindrical
projections are used for maps which are of greater extent north-south
than east-west; azimuthal projections are used for polar regions.
Oblique versions of any of these may also be used. Conformal projections
preserve angular relationships, and better preserve arc-length, while
equal-area projections are more appropriate for statistical studies and
work in which the amount of material is important.

Projections are defined by precise mathematical relations, so the method
of projecting coordinates from a geographic reference frame
(latitude-longitude) into a projected cartesian reference frame (eg
metres) is governed by these equations. Inverse projections can also be
achieved. The public-domain Unix software package *PROJ* \[1\] has been
designed to perform these transformations, and the user's manual
contains a detailed description of over 100 useful projections. This
also includes a programmers library of the projection methods to support
other software development.

Thus, converting a vector map - in which objects are located with
arbitrary spatial precision - from one projection into another is
usually accomplished by a simple two-step process: first the location of
all the points in the map are converted from the source through an
inverse projection into latitude-longitude, and then through a forward
projection into the target. (Of course the procedure will be one-step if
either the source or target is in geographic coordinates.)

Converting a raster map, or image, between different projections,
however, involves additional considerations. A raster may be considered
to represent a sampling of a process at a regular, ordered set of
locations. The set of locations that lie at the intersections of a
cartesian grid in one projection will not, in general, coincide with the
sample points in another projection. Thus, the conversion of raster maps
involves an interpolation step in which the values of points at
intermediate locations relative to the source grid are estimated.

#### Reprojecting vector maps within the GRASS GIS

GIS data capture, import and transfer often requires a reprojection
step, since the source or client will frequently be in a different CRS
to the working CRS.

In some cases it is convenient to do the conversion outside the package,
prior to import or after export, using software such as *PROJ*'s
*[cs2cs](https://proj.org/apps/cs2cs.html)* \[1\]. This is an easy
method for converting an ASCII file containing a list of coordinate
points, since there is no topology to be preserved and *cs2cs* can be
used to process simple lists using a one-line command. The *m.proj*
module provides a handy front end to `cs2cs`.

Vector maps is generally more complex, as parts of the data stored in
the files will describe topology, and not just coordinates. In GRASS GIS
the *[v.proj](v.proj.md)* module is provided to reproject vector maps,
transferring topology and attributes as well as node coordinates. This
program uses the CRS definition and parameters which are stored in the
PROJ_INFO and PROJ_UNITS files in the PERMANENT mapset directory for
every GRASS project.

### Design of r.proj

As discussed briefly above, the fundamental step in reprojecting a
raster is resampling the source grid at locations corresponding to the
intersections of a grid in the target CRS. The basic procedure for
accomplishing this, therefore, is as follows:

*r.proj* converts a map to a new CRS. It reads a map from a different
project, reprojects it and writes it out to the current project. The
reprojected data is resampled with one of four different methods:
nearest neighbor, bilinear, bicubic interpolation or lanczos.

The **method=nearest** method, which performs a nearest neighbor
assignment, is the fastest of the three resampling methods. It is
primarily used for categorical data such as a land use classification,
since it will not change the values of the data cells. The
**method=bilinear** method determines the new value of the cell based on
a weighted distance average of the 4 surrounding cells in the input map.
The **method=bicubic** method determines the new value of the cell based
on a weighted distance average of the 16 surrounding cells in the input
map. The **method=lanczos** method determines the new value of the cell
based on a weighted distance average of the 25 surrounding cells in the
input map. Compared to bicubic, lanczos puts a higher weight on cells
close to the center and a lower weight on cells away from the center,
resulting in slightly better contrast.

The bilinear, bicubic and lanczos interpolation methods are most
appropriate for continuous data and cause some smoothing. The amount of
smoothing decreases from bilinear to bicubic to lanczos. These options
should not be used with categorical data, since the cell values will be
altered.

In the bilinear, bicubic and lanczos methods, if any of the surrounding
cells used to interpolate the new cell value are NULL, the resulting
cell will be NULL, even if the nearest cell is not NULL. This will cause
some thinning along NULL borders, such as the coasts of land areas in a
DEM. The **bilinear_f**, **bicubic_f** and **lanczos_f** interpolation
methods can be used if thinning along NULL edges is not desired. These
methods "fall back" to simpler interpolation methods along NULL borders.
That is, from lanczos to bicubic to bilinear to nearest.

If nearest neighbor assignment is used, the output map has the same
raster format as the input map. If any of the interpolations is used,
the output map is written as floating point.

Note that, following normal GRASS conventions, the coverage and
resolution of the resulting grid is set by the current region settings,
which may be adjusted using *[g.region](g.region.md)*. The target raster
will be relatively unbiased for all cases if its grid has a similar
resolution to the source, so that the resampling/interpolation step is
only a local operation. If the resolution is changed significantly, then
the behaviour of the generalisation or refinement will depend on the
model of the process being represented. This will be very different for
categorical versus numerical data. Note that three methods for the local
interpolation step are provided.

*r.proj* supports general datum transformations, making use of the
*PROJ* co-ordinate system translation library.

## NOTES

If **output** is not specified it is set to be the same as input map
name.  
If **mapset** is not specified, its name is assumed to be the same as
the current mapset's name.  
If **dbase** is not specified it is assumed to be the current database.
The user only has to specify **dbase** if the source project is stored
in another separate GRASS database.

To avoid excessive time consumption when reprojecting a map the region
and resolution of the target project should be set appropriately
beforehand.

A simple way to do this is to check the projected bounds of the input
map in the current project's CRS using the **-p** flag. The **-g** flag
reports the same thing, but in a form which can be directly cut and
pasted into a *[g.region](g.region.md)* command. After setting the
region in that way you might check the cell resolution with "*g.region
-p*" then snap it to a regular grid with *[g.region](g.region.md)*'s
**-a** flag. E.g. `g.region -a res=5 -p`. Note that this is just a rough
guide.

A more involved, but more accurate, way to do this is to generate a
vector "box" map of the region in the source project using *[v.in.region
-d](v.in.region.md)*. This "box" map is then reprojected into the target
project with *[v.proj](v.proj.md)*. Next the region in the target
project is set to the extent of the new vector map with
*[g.region](g.region.md)* along with the desired raster resolution
(*g.region -m* can be used in Latitude/Longitude projects to measure the
geodetic length of a pixel). *r.proj* is then run for the raster map the
user wants to reproject. In this case a little preparation goes a long
way.

When reprojecting whole-world maps the user should disable map-trimming
with the **-n** flag. Trimming is not useful here because the module has
the whole map in memory anyway. Besides that, world "edges" are hard (or
impossible) to find in CRSs other than latitude-longitude so results may
be odd with trimming.

## EXAMPLES

### Inline method

With GRASS running in the destination project use the **-g** flag to
show the input map's bounds once reprojected into the current working
CRS, then use that to set the region bounds before performing the
reprojection:

```sh
# calculate where output map will be
r.proj input=elevation project=ll_wgs84 mapset=user1 -p
Source cols: 8162
Source rows: 12277
Local north: -4265502.30382993
Local south: -4473453.15255565
Local west: 14271663.19157564
Local east: 14409956.2693866

# same calculation, but in a form which can be cut and pasted into a g.region call
r.proj input=elevation project=ll_wgs84 mapset=user1 -g
n=-4265502.30382993 s=-4473453.15255565 w=14271663.19157564 e=14409956.2693866 rows=12277 cols=8162

g.region n=-4265502.30382993 s=-4473453.15255565 \
  w=14271663.19157564 e=14409956.2693866 rows=12277 cols=8162 -p
projection: 99 (Mercator)
zone:       0
datum:      wgs84
ellipsoid:  wgs84
north:      -4265502.30382993
south:      -4473453.15255565
west:       14271663.19157564
east:       14409956.2693866
nsres:      16.93824621
ewres:      16.94352828
rows:       12277
cols:       8162
cells:      100204874

# round resolution to something cleaner
g.region res=17 -a -p
projection: 99 (Mercator)
zone:       0
datum:      wgs84
ellipsoid:  wgs84
north:      -4265487
south:      -4473465
west:       14271653
east:       14409965
nsres:      17
ewres:      17
rows:       12234
cols:       8136
cells:      99535824

# finally, perform the reprojection
r.proj input=elevation project=ll_wgs84 mapset=user1 memory=800
```

### v.in.region method

```sh

# In the source project, use v.in.region to generate a bounding box around the
# region of interest:

v.in.region -d output=bounds type=area

# Now switch to the target project and import the vector bounding box
# (you can run v.proj -l to get a list of vector maps in the source project):

v.proj input=bounds project=source_project_name output=bounds_reprojected

# Set the region in the target project with that of the newly-imported vector
# bounds map, and align the resolution to the desired cell resolution of the
# final, reprojected raster map:

g.region vector=bounds_reprojected res=5 -a

# Now reproject the raster into the target project

r.proj input=elevation.dem output=elevation.dem.reproj \
  project=source_project_name mapset=PERMANENT res=5 method=bicubic
```

## REFERENCES

1. Evenden, G.I. (1990) [Cartographic projection procedures for the
    UNIX environment - a user's manual.](https://proj.org) USGS
    Open-File Report 90-284 (OF90-284.pdf) See also there: Interim
    Report and 2nd Interim Report on Release 4, Evenden 1994).
2. Richards, John A. (1993), Remote Sensing Digital Image Analysis,
    Springer-Verlag, Berlin, 2nd edition.

[PROJ](https://proj.org): Projection/datum support library

Further reading:

- [ASPRS Grids and
  Datum](https://www.asprs.org/asprs-publications/grids-and-datums)
- [Projections Transform List](http://geotiff.maptools.org/proj_list/)
  (PROJ)
- [Coordinate operations](https://proj.org/operations/index.html) by
  PROJ (projections, conversions, transformations, pipeline operator)
- [MapRef - The Collection of Map Projections and Reference Systems for
  Europe](https://mapref.org)
- [Information and Service System for European Coordinate Reference
  Systems - CRS](https://www.crs-geo.eu)

## SEE ALSO

*[g.region](g.region.md), [g.proj](g.proj.md),
[i.rectify](i.rectify.md), [m.proj](m.proj.md),
[r.support](r.support.md), [r.stats](r.stats.md), [v.proj](v.proj.md),
[v.in.region](v.in.region.md)*

The 'gdalwarp' and 'gdal_translate' utilities are available from the
[GDAL](https://gdal.org/en/stable/programs/index.html) project.

## AUTHORS

Martin Schroeder, University of Heidelberg, Germany  
Man page text from S.J.D. Cox, AGCRC, CSIRO Exploration & Mining,
Nedlands, WA  
Updated by [Morten Hulden](mailto:morten@untamo.net)  
Datum transformation support and cleanup by Paul Kelly  
Support of PROJ5+ by Markus Metz, [mundialis](https://www.mundialis.de)

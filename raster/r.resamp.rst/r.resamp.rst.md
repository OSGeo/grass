## DESCRIPTION

*r.resamp.rst* reinterpolates the values a from given raster map (named
*input*) to a new raster map (named *elev*). This module is intended for
reinterpolation of continuous data to a different resolution rather than
for interpolation from scattered data (use the *v.surf.\** modules for
that purpose).

The extent of all resulting raster maps is taken from the settings of
the actual computational region (which may differ from the extent of the
input raster map). The resolution of the computational region however
has to be aligned to the resolution of the input map to avoid artefacts.

Reinterpolation (resampling) is done to higher, same or lower resolution
specified by the *ew_res* and *ns_res* parameters.

All resulting raster maps are created using the settings of the current
region (which may be different from that of the *input* raster map).

Optionally, and simultaneously with interpolation, topographic
parameters are computed from an input raster map containing z-values of
elevation/depth: slope, aspect, profile curvature (measured in the
direction of steepest slope), tangential curvature (measured in the
direction of a tangent to contour line) and/or mean curvature are
computed from and saved as raster maps as specified by the options
*slope, aspect, pcurv, tcurv, mcurv* respectively.

If the *-d* flag is set the program outputs partial derivatives fx, fy,
fxx, fxy, and fyy instead of slope, aspect and curvatures.

For noisy data it is possible to define spatially variable smoothing by
providing a raster map named by the *smooth* option containing smoothing
parameters. With the smoothing parameter set to zero (*smooth* is not
given or contains zero data), the resulting surface passes exactly
through the data points.

The user can also define a raster map (named with *maskmap*) which will
be used as a mask. The interpolation is skipped for cells which have
zero or NULL value in the mask.

Zero values will be assigned to these cells in all output raster maps.

The *zmult* parameter allows the user to rescale the z-values which may
be useful, e.g., for transformation of elevations given in feet to
meters, so that the proper values of slopes and curvatures can be
computed. The default value is 1.

A regularized spline with tension method is used for the interpolation.
The *tension* parameter tunes the character of the resulting surface
from thin plate to membrane. Higher values of tension parameter reduce
the overshoots that can appear in surfaces with rapid change of
gradient.

The *-t* flag can be set to use "dnorm independent tension".

The interpolation is performed for overlapping rectangular segments. The
user can define the width of overlap (in number of cells) with the
*overlap* option. The default value is 3.  

## NOTES

*r.resamp.rst* uses regularized spline with tension for interpolation
(as described in Mitasova and Mitas, 1993).

The region is temporarily changed while writing output files with
desired resolution. Topographic parameters are computed in the same way
as in the *v.surf.rst* module. (See also Mitasova and Hofierka, 1993)

The raster map used with the *smooth* option should contain variable
smoothing parameters. These can be derived from errors, slope, etc.
using the *r.mapcalc* module.

The program gives warning when significant overshoots appear and higher
tension should be used. However, with tension set too high the resulting
surface changes its behavior to a membrane (rubber sheet stretched over
the data points resulting in a peak or pit in each given point and
everywhere else the surface goes rapidly to trend). Smoothing can be
used to reduce the overshoots. When overshoots occur the resulting
*elev* file will have white color in the locations of overshoots since
the color table for the output file is the same as colortable for raster
input file.

The program checks the numerical stability of the algorithm by
computation of values at given points, and prints the maximum difference
found into the history file of raster map *elev* (view with *r.info*).
An increase in tension is suggested if the difference is unacceptable.
For computations with smoothing set to 0 this difference should be 0.
With a smoothing parameter greater than zero the surface will not pass
through the data points exactly, and the higher the parameter the closer
the surface will be to the trend.

The program writes the values of parameters used in computation into the
comment part of the *elev* map history file. Additionally the following
values are also written to assist in the evaluation of results and
choosing of suitable parameters:

- minimum and maximum z values in the data file (zmin_data, zmax_data)
  and in the interpolated raster map (zmin_int, zmax_int),
- maximum difference between the given and interpolated z value at a
  given point (errtotal),
- rescaling parameter used for normalization (dnorm), which influences
  the tension.

The program gives a warning when the user wants to interpolate outside
the region given by the *input* raster map's header data. Zooming into
the area where the points are is suggested in this case.

When a mask is used, the program uses all points in the given region for
interpolation, including those in the area which is masked out, to
ensure proper interpolation along the border of the mask. It therefore
does not mask out the data points; if this is desirable, it must be done
outside *r.resamp.rst* before processing.

## EXAMPLE

Resampling the Spearfish 30m resolution elevation model to 15m:

```sh
# set computation region to original map (30m)
g.region raster=elevation.dem -p

# resample to 15m
r.resamp.rst input=elevation.dem ew_res=15 ns_res=15 elevation=elev15

# set computation region to resulting map
g.region raster=elev15 -p

# verify
r.univar elev15 -g
```

## REFERENCES

Mitas, L., Mitasova, H., 1999, Spatial Interpolation. In: P.Longley,
M.F. Goodchild, D.J. Maguire, D.W.Rhind (Eds.), Geographical Information
Systems: Principles, Techniques, Management and Applications, Wiley,
481-492.

Mitasova, H. and Mitas, L., 1993. Interpolation by regularized spline
with tension: I. Theory and implementation, Mathematical Geology No.25
p.641-656.

Mitasova, H. and Hofierka, L., 1993. Interpolation by regularized spline
with tension: II. Application to terrain modeling and surface geometry
analysis, Mathematical Geology No.25 p.657-667.

Talmi, A. and Gilat, G., 1977. Method for smooth approximation of data,
Journal of Computational Physics , 23, pp 93-123.

Wahba, G., 1990. Spline models for observational data, CNMS-NSF Regional
Conference series in applied mathematics, 59, SIAM, Philadelphia,
Pennsylvania.

## SEE ALSO

[g.region](g.region.md), [r.info](r.info.md),
[r.resample](r.resample.md), [r.mapcalc](r.mapcalc.md),
[r.surf.contour](r.surf.contour.md), [v.surf.rst](v.surf.rst.md)

Overview: [Interpolation and
Resampling](https://grasswiki.osgeo.org/wiki/Interpolation) in GRASS GIS

## AUTHORS

*Original version of program (in FORTRAN):*  
Lubos Mitas, NCSA, University of Illinois at Urbana Champaign, Il  
Helena Mitasova, US Army CERL, Champaign, Illinois

*Modified program (translated to C, adapted for GRASS , segmentation
procedure):*  
Irina Kosinovsky, US Army CERL.  
Dave Gerdes, US Army CERL.

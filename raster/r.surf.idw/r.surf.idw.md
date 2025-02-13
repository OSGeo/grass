## DESCRIPTION

*r.surf.idw* fills a grid cell (raster) matrix with interpolated values
generated from input raster data points. It uses a numerical
approximation technique based on distance squared weighting of the
values of nearest data points. The number of nearest data points used to
determined the interpolated value of a cell can be specified by the user
(default: 12 nearest data points).

If there is a current working mask, it applies to the output raster map.
Only those cells falling within the mask will be assigned interpolated
values. The search procedure for the selection of nearest neighboring
points will consider all input data, without regard to the mask. The
**-e** flag is the error analysis option that interpolates values only
for those cells of the input raster map which have non-zero values and
outputs the difference (see [NOTES](#surface-generation-error-analysis) below).

The **npoints** parameter defines the number of nearest data points used
to determine the interpolated value of an output raster cell.

## NOTES

*r.surf.idw* is a surface generation utility which uses inverse distance
squared weighting (as described in *Applied Geostatistics* by E. H.
Isaaks and R. M. Srivastava, Oxford University Press, 1989) to assign
interpolated values. The implementation includes a customized data
structure somewhat akin to a sparse matrix which enhances the efficiency
with which nearest data points are selected. For latitude/longitude
projections, distances are calculated from point to point along a
geodesic.

Unlike
*[r.surf.idw2](https://grass.osgeo.org/grass8/manuals/addons/r.surf.idw2.html)*
(addon), which processes all input data points in each interpolation
cycle, *r.surf.idw* attempts to minimize the number of input data for
which distances must be calculated. Execution speed is therefore a
function of the search effort, and does not increase appreciably with
the number of input data points.

*r.surf.idw* will generally outperform *r.surf.idw2* except when the
input data layer contains few non-zero data, i.e. when the cost of the
search exceeds the cost of the additional distance calculations
performed by *r.surf.idw2*. The relative performance of these utilities
will depend on the comparative speed of boolean, integer and floating
point operations on a particular platform.

Worst case search performance by *r.surf.idw* occurs when the
interpolated cell is located outside of the region in which input data
are distributed. It therefore behooves the user to employ a mask when
geographic region boundaries include large areas outside the general
extent of the input data.

The degree of smoothing produced by the interpolation will increase
relative to the number of nearest data points considered. The utility
may be used with regularly or irregularly spaced input data. However,
the output result for the former may include unacceptable
nonconformities in the surface pattern.

### Surface-generation error analysis

The **-e** flag option provides a standard surface-generation error
analysis facility. It produces an output raster map of the difference of
interpolated values minus input values for those cells whose input data
are non-zero. For each interpolation cycle, the known value of the cell
under consideration is ignored, and the remaining input values are used
to interpolate a result. The output raster map may be compared to the
input raster map to analyze the distribution of interpolation error.
This procedure may be helpful in choosing the number of nearest
neighbors considered for surface generation.

## KNOWN ISSUES

Module *r.surf.idw* works only for integer (CELL) raster maps.

## SEE ALSO

*[r.surf.contour](r.surf.contour.md), [r.surf.gauss](r.surf.gauss.md),
[r.surf.fractal](r.surf.fractal.md), [r.surf.random](r.surf.random.md),
[v.surf.idw](v.surf.idw.md), [v.surf.rst](v.surf.rst.md)*

Overview: [Interpolation and
Resampling](https://grasswiki.osgeo.org/wiki/Interpolation) in GRASS GIS

## AUTHOR

Greg Koerper  
Global Climate Research Project  
U.S. EPA Environmental Research Laboratory  
200 S.W. 35th Street, JSB  
Corvallis, OR 97333

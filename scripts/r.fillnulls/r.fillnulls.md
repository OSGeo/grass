## DESCRIPTION

*r.fillnulls* fills NULL pixels (no data areas) in input raster map and
stores filled data to a new output raster map. The fill areas are
interpolated from the no data area boundaries buffer using
*[v.surf.rst](v.surf.rst.md)* regularized spline interpolation with
tension (**method=rst**) or *[r.resamp.bspline](r.resamp.bspline.md)*
cubic or linear spline interpolation with Tykhonov regularization.

## NOTES

Each area boundary buffer is set to three times the map resolution to
get nominally three points around the edge. This way the algorithm
interpolates into the hole with a trained slope and curvature at the
edges, in order to avoid that such a flat plane is generated in a hole.
The width of edge area can be adjusted by changing the edge parameter.

During the interpolation following warning may occur when using the RST
method:

```text
Warning: strip exists with insufficient data
Warning: taking too long to find points for interpolation --please change the region to area where your points are
```

This warning is generated if large data holes exist within the surface.
As the idea of *r.fillnulls* is to fill such holes, the user may ignore
the warning. The interpolation will be continued. However, the user may
pay attention to below notes.

If interpolation fails, temporary raster and vector maps are left in
place to allow unfilled map holes (NULL areas) to be identified and
manually repaired.

When using the default RST method, the algorithm is based on
*[v.surf.rst](v.surf.rst.md)* regularized splines with tension
interpolation module which interpolates the raster cell values for NULL
data areas from the boundary values of the NULL data area. An eventual
raster mask is respected during the NULL data area(s) filling. The
interpolated values are patched into the NULL data area(s) of the input
map and saved into a new raster map. Otherwise, either the linear or
cubic spline interpolation with Tykhonov regularization can be selected
(based on *[r.resamp.bspline](r.resamp.bspline.md)*).

## WARNING

Depending on the shape of the NULL data area(s) problems may occur due
to an insufficient number of input cell values for the interpolation
process. Most problems will occur if a NULL data area reaches a large
amount of the map boundary. The user will have to carefully check the
result using *[r.mapcalc](r.mapcalc.md)* (generating a difference map to
the input map and applying the "differences" color table with
*[r.colors](r.colors.md)*) and/or to query individual cell values.

RST method stores temporary maps on hard disk. It will require at least
as much free space as one extra input raster map takes.

## EXAMPLE

In this example, the SRTM elevation map in the North Carolina sample
dataset is filtered for outlier elevation values; missing pixels are
then re-interpolated to obtain a complete elevation map:

```sh
g.region raster=elev_srtm_30m -p
d.mon wx0
d.histogram elev_srtm_30m

# remove SRTM outliers, i.e. SRTM below 50m (esp. lakes), leading to no data areas
r.mapcalc "elev_srtm_30m_filt = if(elev_srtm_30m < 50.0, null(), elev_srtm_30m)"
d.histogram elev_srtm_30m_filt
d.rast elev_srtm_30m_filt

# using the default RST method to fill these holes in DEM
r.fillnulls input=elev_srtm_30m_filt output=elev_srtm_30m_rst tension=20

# using the bilinear method to fill these holes in DEM
r.fillnulls input=elev_srtm_30m_filt output=elev_srtm_30m_bilin method=bilinear

d.histogram elev_srtm_30m_rst
d.rast elev_srtm_30m_rst

d.erase
d.histogram elev_srtm_30m_bilin
d.rast elev_srtm_30m_bilin

r.mapcalc "diff_rst_bilin = elev_srtm_30m_rst - elev_srtm_30m_bilin"
r.colors diff_rst_bilin color=differences

r.univar -e diff_rst_bilin
d.erase
d.rast diff_rst_bilin
d.legend diff_rst_bilin
```

## REFERENCES

- Mitas, L., Mitasova, H., 1999, Spatial Interpolation. In: P.Longley,
  M.F. Goodchild, D.J. Maguire, D.W.Rhind (Eds.), Geographical
  Information Systems: Principles, Techniques, Management and
  Applications, Wiley, pp.481-492
- Mitasova H., Mitas L.,  Brown W.M.,  D.P. Gerdes, I. Kosinovsky,
  Baker, T.1995, Modeling spatially and temporally distributed
  phenomena: New methods and tools for GRASS GIS. *International Journal
  of GIS*, 9 (4), special issue on Integrating GIS and Environmental
  modeling, 433-446.
- [Mitasova H. and Mitas L.
  1993](http://fatra.cnr.ncsu.edu/~hmitaso/gmslab/papers/lmg.rev1.ps):
  Interpolation by Regularized Spline with Tension: I. Theory and
  Implementation, *Mathematical Geology* 25, 641-655.
- [Mitasova H. and Hofierka L.
  1993](http://fatra.cnr.ncsu.edu/~hmitaso/gmslab/papers/hmg.rev1.ps):
  Interpolation by Regularized Spline with Tension: II. Application to
  Terrain Modeling and Surface Geometry Analysis, *Mathematical Geology*
  25, 657-667.

## SEE ALSO

*[r.fill.dir](r.fill.dir.md), [r.mapcalc](r.mapcalc.md),
[r.resamp.bspline](r.resamp.bspline.md),
[v.surf.bspline](v.surf.bspline.md), [v.surf.rst](v.surf.rst.md),
[v.fill.holes](v.fill.holes.md)*

## AUTHORS

Markus Neteler, University of Hannover and Fondazione Edmund Mach  
Improvement by Hamish Bowman, NZ

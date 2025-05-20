## DESCRIPTION

*v.vol.rst* interpolates values to a 3-dimensional raster map from
3-dimensional point data (e.g. temperature, rainfall data from climatic
stations, concentrations from drill holes etc.) given in a 3-D vector
point file named **input**.  The size of the output 3D raster map
**elevation** is given by the current 3D region. Sometimes, the user may
want to get a 2-D map showing a modelled phenomenon at a crossection
surface. In that case, **cross_input** and **cross_output** options must
be specified, with the output 2D raster map **cross_output** containing
the crossection of the interpolated volume with a surface defined by
**cross_input** 2D raster map. As an option, simultaneously with
interpolation, geometric parameters of the interpolated phenomenon can
be computed (magnitude of gradient, direction of gradient defined by
horizontal and vertical angles), change of gradient, Gauss-Kronecker
curvature, or mean curvature). These geometric parameters are saved as
3D raster maps **gradient, aspect_horizontal, aspect_vertical,
ncurvature, gcurvature, mcurvature**, respectively. Maps
**aspect_horizontal** and **aspect_vertical** are in degrees.

At first, data points are checked for identical positions and points
that are closer to each other than given **dmin** are removed.
Parameters **wscale** and **zscale** allow the user to re-scale the
w-values and z-coordinates of the point data (useful e.g. for
transformation of elevations given in feet to meters, so that the proper
values of gradient and curvatures can be computed). Rescaling of
z-coordinates (**zscale**) is also needed when the distances in vertical
direction are much smaller than the horizontal distances; if that is the
case, the value of **zscale** should be selected so that the vertical
and horizontal distances have about the same magnitude.

Regularized spline with tension method is used in the interpolation. The
**tension** parameter controls the distance over which each given point
influences the resulting volume (with very high tension, each point
influences only its close neighborhood and the volume goes rapidly to
trend between the points). Higher values of tension parameter reduce the
overshoots that can appear in volumes with rapid change of gradient. For
noisy data, it is possible to define a global smoothing parameter,
**smooth**. With the smoothing parameter set to zero (**smooth=0**) the
resulting volume passes exactly through the data points. When smoothing
is used, it is possible to output a vector map **deviations** containing
deviations of the resulting volume from the given data.

The user can define a 2D raster map named **maskmap**, which will be
used as a mask. The interpolation is skipped for 3-dimensional cells
whose 2-dimensional projection has a zero value in the mask. Zero values
will be assigned to these cells in all output 3D raster maps.

If the number of given points is greater than 700, segmented processing
is used. The region is split into 3-dimensional "box" segments, each
having less than **segmax** points and interpolation is performed on
each segment of the region. To ensure the smooth connection of segments,
the interpolation function for each segment is computed using the points
in the given segment and the points in its neighborhood. The minimum
number of points taken for interpolation is controlled by **npmin** ,
the value of which must be larger than **segmax** and less than 700.
This limit of 700 was selected to ensure the numerical stability and
efficiency of the algorithm.

### SQL support

Using the **where** parameter, the interpolation can be limited to use
only a subset of the input vectors.

```sh
# preparation as in above example
v.vol.rst elevrand_3d wcol=soilrange elevation=soilrange zscale=100 where="soilrange > 3"
```

### Cross validation procedure

Sometimes it can be difficult to figure out the proper values of
interpolation parameters. In this case, the user can use a
crossvalidation procedure using **-c** flag (a.k.a. "jack-knife" method)
to find optimal parameters for given data. In this method, every point
in the input point file is temporarily excluded from the computation and
interpolation error for this point location is computed. During this
procedure no output grid files can be simultanuously computed. The
procedure for larger datasets may take a very long time, so it might be
worth to use just a sample data representing the whole dataset.

*Example (based on [Slovakia3d
dataset](https://grassbook.org/data_menu2nd.php)):*

```sh
v.info -c precip3d
g.region n=5530000 s=5275000 w=4186000 e=4631000 res=500 -p
v.vol.rst -c input=precip3d wcolumn=precip zscale=50 segmax=700 cvdev=cvdevmap tension=10
v.db.select cvdevmap
v.univar cvdevmap col=flt1 type=point
```

Based on these results, the parameters will have to be optimized. It is
recommended to plot the CV error as curve while modifying the
parameters.

The best approach is to start with **tension**, **smooth** and
**zscale** with rough steps, or to set **zscale** to a constant
somewhere between 30-60. This helps to find minimal RMSE values while
then finer steps can be used in all parameters. The reasonable range is
**tension**=10...100, **smooth**=0.1...1.0, **zscale**=10...100.

In *v.vol.rst* the tension parameter is much more sensitive to changes
than in *v.surf.rst*, therefore the user should always check the result
by visual inspection. Minimizing CV does not always provide the best
result, especially when the density of data are insufficient. Then the
optimal result found by CV is an oversmoothed surface.

## NOTES

The vector points map must be a 3D vector map (x, y, z as geometry). The
module [v.in.db](v.in.db.md) can be used to generate a 3D vector map
from a table containing x,y,z columns. Also, the input data should be in
a projected coordinate system, such as Universal Transverse Mercator.
The module does not appear to have support for geographic (Lat/Long)
coordinates as of May 2009.

*v.vol.rst* uses regularized spline with tension for interpolation from
point data (as described in Mitasova and Mitas, 1993). The
implementation has an improved segmentation procedure based on Oct-trees
which enhances the efficiency for large data sets.

Geometric parameters - magnitude of gradient (**gradient**), horizontal
(**aspect_horizontal**) and vertical (**aspect_vertical)**aspects,
change of gradient (**ncurvature**), Gauss-Kronecker (**gcurvature**)
and mean curvatures (**mcurvature**) are computed directly from the
interpolation function so that the important relationships between these
parameters are preserved. More information on these parameters can be
found in Mitasova et al., 1995 or Thorpe, 1979.

The program gives warning when significant overshoots appear and higher
tension should be used. However, with tension too high the resulting
volume will have local maximum in each given point and everywhere else
the volume goes rapidly to trend. With a smoothing parameter greater
than zero, the volume will not pass through the data points and the
higher the parameter the closer the volume will be to the trend. For
theory on smoothing with splines see Talmi and Gilat, 1977 or Wahba,
1990.

If a visible connection of segments appears, the program should be rerun
with higher **npmin** to get more points from the neighborhood of given
segment.

If the number of points in a vector map is less than 400, **segmax**
should be set to 400 so that segmentation is not performed when it is
not necessary.

The program gives a warning when the user wants to interpolate outside
the "box" given by minimum and maximum coordinates in the input vector
map. To remedy this, zoom into the area encompassing the input vector
data points.

For large data sets (thousands of data points), it is suggested to zoom
into a smaller representative area and test whether the parameters
chosen (e.g. defaults) are appropriate.

The user must run *g.region* before the program to set the 3D region for
interpolation.

## EXAMPLES

Spearfish example (we first simulate 3D soil range data):

```sh
g.region -dp
# define volume
g.region res=100 tbres=100 res3=100 b=0 t=1500 -ap3

### First part: generate synthetic 3D data (true 3D soil data preferred)
# generate random positions from elevation map (2D)
r.random elevation.10m vector=elevrand n=200 seed=42

# generate synthetic values
v.to.db elevrand option=coor col=x,y
v.db.select elevrand

# create new 3D map
v.in.db elevrand out=elevrand_3d x=x y=y z=value key=cat
v.info -c elevrand_3d
v.info -t elevrand_3d

# remove the now superfluous 'x', 'y' and 'value' (z) columns
v.db.dropcolumn elevrand_3d col=x
v.db.dropcolumn elevrand_3d col=y
v.db.dropcolumn elevrand_3d col=value

# add attribute to have data available for 3D interpolation
# (Soil range types taken from the USDA Soil Survey)
d.mon wx0
d.rast soils.range
d.vect elevrand_3d
v.db.addcolumn elevrand_3d col="soilrange integer"
v.what.rast elevrand_3d col=soilrange rast=soils.range

# fix 0 (no data in raster map) to NULL:
v.db.update elevrand_3d col=soilrange value=NULL where="soilrange=0"
v.db.select elevrand_3d

# optionally: check 3D points in Paraview
v.out.vtk input=elevrand_3d output=elevrand_3d.vtk type=point precision=2
paraview --data=elevrand_3d.vtk

### Second part: 3D interpolation from 3D point data
# interpolate volume to "soilrange" voxel map
v.vol.rst input=elevrand_3d wcol=soilrange elevation=soilrange zscale=100

# visualize I: in GRASS GIS wxGUI
g.gui
# load: 2D raster map: elevation.10m
#       3D raster map: soilrange

# visualize II: export to Paraview
r.mapcalc "bottom = 0.0"
r3.out.vtk -s input=soilrange top=elevation.10m bottom=bottom dp=2 output=volume.vtk
paraview --data=volume.vtk
```

## KNOWN ISSUES

**deviations** file is written as 2D and deviations are not written as
attributes.

## REFERENCES

Hofierka J., Parajka J., Mitasova H., Mitas L., 2002, Multivariate
Interpolation of Precipitation Using Regularized Spline with Tension.
Transactions in GIS  6, pp. 135-150.

[Mitas, L., Mitasova, H.](http://fatra.cnr.ncsu.edu/~hmitaso/gmslab/),
1999, Spatial Interpolation. In: P.Longley, M.F. Goodchild, D.J.
Maguire, D.W.Rhind (Eds.), Geographical Information Systems: Principles,
Techniques, Management and Applications, Wiley, pp.481-492

Mitas L., Brown W. M., Mitasova H., 1997, [Role of dynamic cartography
in simulations of landscape processes based on multi-variate
fields.](http://fatra.cnr.ncsu.edu/~hmitaso/gmslab/lcgfin/cg-mitas.html)
Computers and Geosciences, Vol. 23, No. 4, pp. 437-446 (includes CDROM
and WWW: <www.elsevier.nl/locate/cgvis>)

Mitasova H., Mitas L.,  Brown W.M.,  D.P. Gerdes, I. Kosinovsky, Baker,
T.1995, Modeling spatially and temporally distributed phenomena: New
methods and tools for GRASS GIS. International Journal of GIS, 9 (4),
special issue on Integrating GIS and Environmental modeling, 433-446.

Mitasova, H., Mitas, L., Brown, B., Kosinovsky, I., Baker, T., Gerdes,
D. (1994): [Multidimensional interpolation and visualization in GRASS
GIS](http://fatra.cnr.ncsu.edu/~hmitaso/gmslab/viz/ches.html)

[Mitasova H. and Mitas L.
1993](http://fatra.cnr.ncsu.edu/~hmitaso/gmslab/papers/lmg.rev1.ps):
Interpolation by Regularized Spline with Tension: I. Theory and
Implementation, *Mathematical Geology* 25, 641-655.

[Mitasova H. and Hofierka J.
1993](http://fatra.cnr.ncsu.edu/~hmitaso/gmslab/papers/hmg.rev1.ps):
Interpolation by Regularized Spline with Tension: II. Application to
Terrain Modeling and Surface Geometry Analysis, *Mathematical Geology*
25, 657-667.

Mitasova, H., 1992 : New capabilities for interpolation and topographic
analysis in GRASS, GRASSclippings 6, No.2 (summer), p.13.

Wahba, G., 1990 : Spline Models for Observational Data, CNMS-NSF
Regional Conference series in applied mathematics, 59, SIAM,
Philadelphia, Pennsylvania.

Mitas, L., Mitasova H., 1988 : General variational approach to the
interpolation problem, Computers and Mathematics with Applications 16,
p. 983

Talmi, A. and Gilat, G., 1977 : Method for Smooth Approximation of Data,
Journal of Computational Physics, 23, p.93-123.

Thorpe, J. A. (1979): Elementary Topics in Differential Geometry.
Springer-Verlag, New York, pp. 6-94.

## SEE ALSO

*[g.region](g.region.md), [v.in.ascii](v.in.ascii.md),
[r3.mask](r3.mask.md), [v.in.db](v.in.db.md),
[v.surf.rst](v.surf.rst.md), [v.univar](v.univar.md)*

## AUTHOR

Original version of program (in FORTRAN) and GRASS enhancements:  
Lubos Mitas, NCSA, University of Illinois at Urbana-Champaign, Illinois,
USA, since 2000 at Department of Physics, North Carolina State
University, Raleigh, USA <lubos_mitas@ncsu.edu>  
Helena Mitasova, Department of Marine, Earth and Atmospheric Sciences,
North Carolina State University, Raleigh, USA, <hmitaso@unity.ncsu.edu>

Modified program (translated to C, adapted for GRASS, new segmentation
procedure):  
Irina Kosinovsky, US Army CERL, Champaign, Illinois, USA  
Dave Gerdes, US Army CERL, Champaign, Illinois, USA

Modifications for g3d library, geometric parameters, cross-validation,
deviations:  
Jaro Hofierka, Department of Geography and Regional Development,
University of Presov, Presov, Slovakia, <hofierka@fhpv.unipo.sk>,
<http://www.geomodel.sk>

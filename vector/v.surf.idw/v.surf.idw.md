## DESCRIPTION

*v.surf.idw* fills a raster matrix with interpolated values generated
from a set of irregularly spaced vector data points using numerical
approximation (weighted averaging) techniques. The interpolated value of
a cell is determined by values of nearby data points and the distance of
the cell from those input points. In comparison with other methods,
numerical approximation allows representation of more complex surfaces
(particularly those with anomalous features), restricts the spatial
influence of any errors, and generates the interpolated surface from the
data points.

Values to interpolate are read from **column** option. If this option is
not given than the program uses *categories* as values to interpolate or
*z-coordinates* if the input vector map is 3D.

## NOTES

The amount of memory used by this program is related to the number of
vector points in the current region. If the vector point map is very
dense (i.e., contains many data points), the program may not be able to
get all the memory it needs from the system. The time required to
execute is related to the resolution of the current region, after an
initial delay determined by the time taken to read the input vector
points map.

Note that vector features without category in given **layer** are
*skipped*.

If the user has a mask set, then interpolation is only done for those
cells that fall within the mask. The module has two separate modes of
operation for selecting the vector points that are used in the
interpolation:

Simple, non-indexed mode (activated by **-n** flag)  
When the **-n** flag is specified, all vector points in the input vector
map are searched through in order to find the **npoints** closest points
to the centre of each cell in the output raster map. This mode of
operation can be slow in the case of a very large number of vector
points.

Default, indexed mode  
By default (i.e. if **-n** flag is *not* specified), prior to the
interpolation, input vector points are indexed according to which output
raster cell they fall into. This means that only cells nearby the one
being interpolated need to be searched to find the **npoints** closest
input points, and the module can run many times faster on dense input
maps. It should be noted that:

- Only vector points that lie within the current region are used in the
  interpolation. If there are points outside the current region, this
  may have an effect on the interpolated value of cells near the edges
  of the region, and this effect will be more pronounced the fewer
  points there are. If you wish to also include points outside the
  region in the interpolation, then either use the **-n** flag, or set
  the region to a larger extent (covering all input points) and use a
  mask to limit interpolation to a smaller area.
- If more than **npoints** points fall within a given cell then, rather
  than interpolating, these points are aggregated by taking the mean.
  This avoids the situation where some vector points can be discarded
  and not used in the interpolation, for very dense input maps. Again,
  use the **-n** flag if you wish to use only the **npoints** closest
  points to the cell centre under all circumstances.

The **power** parameter defines an exponential distance weight. Greater
values assign greater influence to values closer to the point to be
interpolated. The interpolation function peaks sharply over the given
data points for 0 \< *p* \< 1 and more smoothly for larger values. The
default value for the power parameter is 2.

By setting **npoints**=1, the module can be used to calculate raster
Voronoi diagrams (Thiessen polygons).

## SEE ALSO

*[g.region](g.region.md), [r.surf.contour](r.surf.contour.md),
[r.surf.idw](r.surf.idw.md), [r.surf.gauss](r.surf.gauss.md),
[r.surf.fractal](r.surf.fractal.md), [r.surf.random](r.surf.random.md),
[v.surf.rst](v.surf.rst.md)*

Overview: [Interpolation and
Resampling](https://grasswiki.osgeo.org/wiki/Interpolation) in GRASS GIS

## AUTHORS

Michael Shapiro, U.S. Army Construction Engineering Research
Laboratory  
Improved algorithm (indexes points according to cell and ignores points
outside current region) by Paul Kelly

## DESCRIPTION

*r.resample* resamples the data values in a user-specified raster input
map layer *name* (bounded by the current geographic region and masked by
the current mask), and produces a new raster output map layer *name*
containing the results of the resampling. The category values in the new
raster output map layer will be the same as those in the original,
except that the resolution and extent of the new raster output map layer
will match those of the current geographic region settings (see
*[g.region](g.region.md)*). *r.resample* is intended for resampling of
discrete raster data (such as land cover, geology or soil type) to a
different resolution. Continuous data (such as elevation or temperature)
usually require reinterpolation when changing resolution, see
*[r.resamp.interp](r.resamp.interp.md)*.

## NOTES

The method by which resampling is conducted is "nearest neighbor" (see
*[r.neighbors](r.neighbors.md)*). The resulting raster map layer will
have the same resolution as the resolution of the current geographic
region (set using *[g.region](g.region.md)*).

The resulting raster map layer may be identical to the original raster
map layer. The *r.resample* program will copy the color table and
history file associated with the original raster map layer for the
resulting raster map layer and will create a modified category file
which contains description of only those categories which appear in
resampled file.

When the user resamples a GRASS *reclass* file, a true raster map is
created by *r.resample*.

## SEE ALSO

*[g.region](g.region.md), [r.mapcalc](r.mapcalc.md),
[r.mfilter](r.mfilter.md), [r.neighbors](r.neighbors.md),
[r.rescale](r.rescale.md), [r.resamp.interp](r.resamp.interp.md)*

Overview: [Interpolation and
Resampling](https://grasswiki.osgeo.org/wiki/Interpolation) in GRASS GIS

## AUTHOR

Michael Shapiro, U.S.Army Construction Engineering Research Laboratory

## DESCRIPTION

*t.rast.contour* is designed to produce a space time vector dataset of
specified contours from a space time raster dataset. This module works
as a front-end to [r.contour](r.contour.html) and therefore supports all
parameter of this module. Hence, all raster map layers in a space time
raster dataset are successively passed to [r.contour](r.contour.html)
that computes the contour lines. Please refer to the
[r.contour](r.contour.html) documentation for a detailed description.
The new generated vector contour map layers will be registered in the
output space time vector dataset, using the same time stamps as their
raster map layer origins.

This module supports the parallel processing of
[r.contour](r.contour.html) module instances. The number of parallel
processes can be set with the *nprocs* option. However, this will only
work in conjunction with the *-t* flag, that avoids the creation of
attribute tables. The parallel creation of attribute tables is not
supported.

The *where* option allows selecting subsets of the input space time
raster dataset.

The flag *-n* can be used to force the registration of empty vector map
layers. Empty vector maps may occur in case that empty raster map layers
should be converted into vector map layers, or in case the chosen steps
or contour levels are not present in the raster map layers.

## SEE ALSO

*[r.contour](r.contour.html), [t.rast.db.select](t.vect.db.select.html),
[t.info](t.info.html)*

## AUTHOR

Sören Gebbert, Geoinformatikbüro Dassau

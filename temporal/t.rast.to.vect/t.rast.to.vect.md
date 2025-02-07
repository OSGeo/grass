## DESCRIPTION

*t.rast.to.vect* is designed to convert a space time raster dataset into
a space time vector dataset. This module works as a front-end to
[r.to.vect](r.to.vect.md) and therefore supports all parameter of this
module. Hence, all raster map layers in a space time raster dataset are
passed to [r.to.vect](r.to.vect.md) that converts them into vector map
layers (using point,line or area as conversion criteria). Please refer
to the [r.to.vect](r.to.vect.md) documentation for a detailed
description of the raster to vector conversion options. The new
generated vector map layers will be registered in the output space time
vector dataset, using the same time stamps as their raster map layer
origins.

This module supports the parallel processing of
[r.to.vect](r.to.vect.md) module instances. The number of parallel
processes can be set with the *nprocs* option. However, this will only
work in conjunction with the *-t* flag, that avoids the creation of
attribute tables. The parallel creation of attribute tables is not
supported.

The *where* option allows selecting subsets of the input space time
raster dataset.

The flag *-n* can be used to force the registration of empty vector map
layers. Empty vector maps may occur in case that empty raster map layers
should be converted into vector map layers.

## SEE ALSO

*[r.to.vect](r.to.vect.md), [t.rast.db.select](t.vect.db.select.md),
[t.info](t.info.md)*

## AUTHOR

Sören Gebbert, Geoinformatikbüro Dassau

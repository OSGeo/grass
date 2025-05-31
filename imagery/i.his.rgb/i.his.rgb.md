## DESCRIPTION

*i.his.rgb* is an image processing program that processes three input
raster map layers as hue, intensity and saturation components and
produces three output raster map layers representing the red, green and
blue components of this data. The output raster map layers are created
by a standard hue-intensity-saturation (HIS) to red-green-blue (RGB)
color transformation. Each output raster map layer is given a linear
gray scale color table. The current geographic region and mask settings
are respected.

## NOTES

It is not possible to process three bands with *i.his.rgb* and then
exactly recover the original bands with *i.rgb.his*. This is due to loss
of precision because of integer computations and rounding. Tests have
shown that more than 70% of the original cell values will be reproduced
exactly after transformation in both directions and that 99% will be
within plus or minus 1. A few cell values may differ significantly from
their original values.

## SEE ALSO

*[i.rgb.his](i.rgb.his.md), [r.colors](r.colors.md)*

## AUTHOR

David Satnik, GIS Laboratory, Central Washington University

with acknowledgements to Ali Vali, Univ. of Texas Space Research Center,
for the core routine.

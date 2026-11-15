## DESCRIPTION

*i.zc* is an image processing module used for edge detection. The raster
map produced shows the location of "boundaries" on the input map.
Boundaries tend to be found in regions of changing cell values and tend
to run perpendicular to the direction of the slope. The algorithm used
for edge detection is one of the "zero-crossing" algorithms and is
discussed briefly below.

## NOTES

The procedure to find the "edges" in the image is as follows:

1. The Fourier transform of the image is taken,
2. The Fourier transform of the Laplacian of a two-dimensional Gaussian
    function is used to filter the transformed image,
3. The result is run through an inverse Fourier transform,
4. The resulting image is traversed in search of places where the image
    changes from positive to negative or from negative to positive,
5. Each cell in the map where the value crosses zero (with a change in
    value greater than the threshold value) is marked as an edge and an
    orientation is assigned to it. The resulting raster map layer is
    output.

The **width=** parameter determines the x-y extent of the Gaussian
filter. The default value is **9**; higher and lower values can be
tested by the user. Increasing the width will result in finding "edges"
representing more gradual changes in cell values.

The **threshold=** parameter determines the "sensitivity" of the
Gaussian filter. The default value is **1**; higher and lower values can
be tested by the user. Increasing the threshold value will result in
fewer edges being found.

The **orientations=** value is the number of azimuth directions the
cells on the output raster map layer are categorized into (similar to
the aspect raster map layer produced by
*[r.slope.aspect](r.slope.aspect.md)*. For example, a value of **16**
would result in detected edges being categorized into one of 16 bins
depending on the direction of the edge at that point.

The current region definition and mask settings are respected when
reading the input map.

## SEE ALSO

*[i.fft](i.fft.md), [i.ifft](i.ifft.md), [r.mapcalc](r.mapcalc.md),
[r.mfilter](r.mfilter.md), [r.slope.aspect](r.slope.aspect.md)*

## AUTHOR

David Satnik, GIS Laboratory, Central Washington University

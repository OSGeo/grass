## DESCRIPTION

*r.circle* creates an output raster map centered on the *x,y* values
specified with the *coordinate* parameter, out to the edge of the
current region. The output cell values increase linearly from the
specified center. The *min* and *max* parameters control the inner and
outer output raster map radii, respectively.

The *mult* parameter can be used to multiply the output raster cells by
a common factor. Note that this parameter does not affect the output
raster position or size; only the z-values are changed with this
parameter.

Binary-output raster maps (solid circles of one value) can be created
with the **-b** flag. Raster maps so created can be used to create
binary filters for use in *i.ifft* (inverse Fourier transformations;
apply filter with *r.mask*).

## EXAMPLES

Generate a raster circle at current map center with a radius of 300m and
outwardly increasing raster values:

```sh
EASTCENTER=`g.region -c |  awk ' /center easting:/ { print $3 }'`
NORTHCENTER=`g.region -c | awk ' /center northing:/ { print $3 }'`
r.circle output=circle coordinate=${EASTCENTER},${NORTHCENTER} max=300
```

Generate a binary raster ring around current map center with an inner
radius of 500m and an outer radius of 1000m:

```sh
EASTCENTER=`g.region -c |  awk ' /center easting:/ { print $3 }'`
NORTHCENTER=`g.region -c | awk ' /center northing:/ { print $3 }'`
r.circle -b output=circle coordinate=${EASTCENTER},${NORTHCENTER} min=500 max=1000
```

## SEE ALSO

*[g.region](g.region.md), [g.remove](g.remove.md),
[g.rename](g.rename.md), [i.fft](i.fft.md), [i.ifft](i.ifft.md),
[r.mask](r.mask.md)*

## AUTHORS

Bill Brown, U.S. Army Construction Engineering Research Laboratory  
Additional flag/min/max parameter by Markus Neteler, University of
Hannover

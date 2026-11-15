## DESCRIPTION

**r.out.ppm3** converts 3 GRASS raster layers (R,G,B) to a PPM image
file, using the current region.

This program converts a GRASS raster map to a PPM image file using the
the current region settings.

To get the full area and resolution of the raster map, run:

```sh
g.region raster=[mapname]
```

before running *r.out.ppm3*.

## NOTES

One pixel is written for each cell value, so if ew_res and ns_res
differ, the aspect ratio of the resulting image will be off.

## SEE ALSO

*[r.out.ppm](r.out.ppm.md),* *[r.in.gdal](r.in.gdal.md),*
*[d.rgb](d.rgb.md)*

## AUTHOR

Glynn Clements  
Based upon *r.out.ppm* and *d.rgb*.

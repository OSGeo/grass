## DESCRIPTION

*r.out.ppm* converts a GRASS raster map into a PPM image at the pixel
resolution of the CURRENTLY DEFINED REGION. To get the resolution and
region settings of the raster map, run:

```sh
g.region -p raster=[mapname]
```

before running *r.out.ppm*.

By default the PPM file created is 24-bit color, rawbits storage. You
can use the **-g** flag to force *r.out.ppm* to output an 8-bit
greyscale instead. The greyscale conversion uses the NTSC conversion:

```sh
Y = .30*Red + .59*Green + .11*Blue
```

One pixel is written for each cell value, so if `ew_res` and `ns_res`
differ, the aspect ratio of the resulting image will be off.

## NOTES

A few ppm file comments are written: the name of the GRASS raster map,
resolution, etc. Although these are perfectly legal, I've found one PD
image utility that chokes on them, so if you need a commentless PPM
file, use '`out=- > outfile.ppm`'. (When sending output to stdout, no
comments are written.)

## HINTS

You can create a PNG image with NULL values represented by a transparent
background by using the [PNG driver](pngdriver.md) with
[GRASS_RENDER_TRANSPARENT](variables.md) set to TRUE. Alternatively, you
can use the *pnmtopng* program from
[netpbm](https://netpbm.sourceforge.net) to do this:

```sh
r.out.ppm raster
pnmtopng -transparent white raster.ppm > raster.png
```

## SEE ALSO

*[d.out.file](d.out.file.md), [r.out.ascii](r.out.ascii.md),
[r.out.gdal](r.out.gdal.md), [r.out.mpeg](r.out.mpeg.md),
[r.out.png](r.out.png.md), [r.out.ppm3](r.out.ppm3.md)*

## AUTHOR

Bill Brown, UIUC

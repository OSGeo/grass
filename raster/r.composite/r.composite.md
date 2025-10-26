## DESCRIPTION

This program combines three raster maps to form a composite RGB map. For
each input map layer, the corresponding component from the map's color
table is used (e.g. for the red map, the red component is used, and so
on). In general, the maps should use a grey-scale color table.

## NOTES

The default number of intensity levels for each component is 32,
resulting in a total of 32768 possible colors (equivalent to 15 bits per
pixel). If significantly more levels than this are used, not only will
*r.composite* take longer to run, but displaying the resulting layer
with *[d.rast](d.rast.md)* will also be significantly slower.

Floyd-Steinberg dithering is optionally used with the **-d** flag.

## EXAMPLES

### RGB composite of three satellite bands

Color composite of a LANDSAT scene (North Carolina sample dataset):

```sh
g.region raster=lsat7_2002_10
r.composite blue=lsat7_2002_10 green=lsat7_2002_20 red=lsat7_2002_30 \
            output=lsat7_2002_rgb
```

### RGB composite with dithering

Creating a composite RGB raster using 32 color levels per layer, with
dithering:

```sh
r.composite -d red=elevation.r green=elevation.g blue=elevation.b \
            output=elev.composite
```

## SEE ALSO

*[d.rast](d.rast.md), [d.rgb](d.rgb.md), [r.blend](r.blend.md),
[r.colors](r.colors.md), [r.rgb](r.rgb.md)*

*[Wikipedia Entry: Floyd-Steinberg
dithering](https://en.wikipedia.org/wiki/Floyd-Steinberg_dithering)*

## AUTHOR

Glynn Clements

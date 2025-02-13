## DESCRIPTION

*d.his* displays the result obtained by combining hue, intensity, and
saturation (HIS) values from user-specified input raster map layers.

*HIS* stands for hue, intensity, and saturation. This program produces a
raster map layer providing a visually pleasing combination of hue,
intensity, and saturation values from two or three user-specified raster
map layers.

The human brain automatically interprets the vast amount of visual
information available according to basic rules. Color, or *hue*, is used
to categorize objects. Shading, or *intensity*, is interpreted as
three-dimensional texturing. Finally, the degree of haziness, or
*saturation*, is associated with distance or depth. This program allows
data from up to three raster map layers to be combined into an image
which retains the original information in terms of *hue*, *intensity*,
and *saturation*.

## OPTIONS

This program can be run non-interactively or interactively. It will run
non-interactively if the user specifies on the command line the name of
a map containing hue values (**hue**), and the name(s) of map(s)
containing intensity values (**intensity**) and/or saturation values
(**saturation**). The resulting image will be displayed in the active
display frame on the graphics monitor.

Alternately, the user can run the program interactively by typing
**d.his** without naming parameter values on the command line. In this
case, the program will prompt the user for parameter values using the
standard GRASS GUI interface.

While any raster map layer can be used to represent the hue information,
map layers with a few very distinct colors work best. Only raster map
layers representing continuously varying data like elevation, aspect,
weights, intensities, or amounts can suitably be used to provide
intensity and saturation information.

For example, a visually pleasing image can be made by using a watershed
map for the *hue* factor, an aspect map for the *intensity* factor, and
an elevation map for *saturation*. (The user may wish to leave out the
elevation information for a first try.) Ideally, the resulting image
should resemble the view from an aircraft looking at a terrain on a
sunny day with a bit of haze in the valleys.

The **brighten** option does not truly represent a percentage, but
calling it that makes the option easy to understand, and it sounds
better than *Normalized Scaling Factor*.

## THE PROCESS

Each map cell is processed individually. First, the working color is set
to the color of the corresponding cell in the map layer chosen to
represent *hue*. Second, this color is multiplied by the *red* intensity
of that cell in the *intensity* map layer. This map layer should have an
appropriate gray-scale color table associated with it. You can ensure
this by using the color manipulation capabilities of
*[r.colors](r.colors.md)*. Finally, the color is made somewhat
gray-based on the *red* intensity of that cell in the *saturation* map
layer. Again, this map layer should have a gray-scale color table
associated with it.

## NOTES

The name is misleading. The actual conversion used is

```sh
  H.i.s + G.(1-s)

where

  H   is the R,G,B color from the hue map
  i   is the red value from the intensity map
  s   is the red value from the saturation map
  G   is 50% gray (R = G = B = 0.5)

```

Either (but not both) of the intensity or the saturation map layers may
be omitted. This means that it is possible to produce output images that
represent combinations of *his, hi,* or *hs*.

Users wishing to store the result in new raster map layers instead of
displaying it on the monitor should use the command *[r.his](r.his.md)*.

## EXAMPLE

```sh
g.region raster=elevation
r.relief input=elevation output=elevation_shaded_relief

d.mon wx0
d.his hue=elevation intensity=elevation_shaded_relief brighten=50
```

## SEE ALSO

*[d.colortable](d.colortable.md), [d.frame](d.frame.md),
[d.rgb](d.rgb.md), [d.shade](d.shade.md), [r.colors](r.colors.md),
[r.his](r.his.md), [i.his.rgb](i.his.rgb.md), [i.rgb.his](i.rgb.his.md)*

## AUTHOR

James Westervelt, U.S. Army Construction Engineering Research Laboratory

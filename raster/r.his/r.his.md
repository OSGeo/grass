## DESCRIPTION

*HIS* stands for hue, intensity, and saturation. *r.his* produces red,
green and blue raster map layers providing a visually pleasing
combination of hue, intensity, and saturation values from two or three
user-specified raster map layers.

The human brain automatically interprets the vast amount of visual
information available according to basic rules. Color, or *hue*, is used
to categorize objects. Shading, or *intensity*, is interpreted as
three-dimensional texturing. Finally, the degree of haziness, or
*saturation*, is associated with distance or depth. This program allows
data from up to three raster map layers to be combined into a color
image (in the form of separate red, green and blue raster map layers)
which retains the original information in terms of *hue*, *intensity*,
and *saturation*.

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

### The Process

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
represent combinations of *his, hi,* or *hs*. The separate *red*,
*green* and *blue* maps can be displayed on the graphics monitor using
*[d.rgb](d.rgb.md)*, or combined into a composite RGB layer using
*[r.composite](r.composite.md)*. Users wishing to simply display an
*his* composite image without actually generating any layers should use
the program *[d.his](d.his.md)*.

## EXAMPLES

Recreate the following example for *d.his* using *r.his*. First, create
shaded relief and show it.

```sh
g.region raster=elevation
r.relief input=elevation output=elevation_shaded_relief

d.mon wx0
d.his hue=elevation intensity=elevation_shaded_relief brighten=50
```

Second, compute lighter version of color of shaded relief. Then convert
from HIS model to RGB and show the result.

```sh
r.mapcalc "elevation_shaded_relief_bright_50 = #elevation_shaded_relief * 1.5"
r.colors elevation_shaded_relief_bright_50 color=grey255
r.his hue=elevation intensity=elevation_shaded_relief_bright_50 \
      red=shadedmap_r green=shadedmap_g blue=shadedmap_b

d.mon wx1
d.rgb red=shadedmap_r green=shadedmap_g blue=shadedmap_b
```

## SEE ALSO

*[d.his](d.his.md), [d.colortable](d.colortable.md), [d.rgb](d.rgb.md),
[r.blend](r.blend.md), [r.colors](r.colors.md),
[r.composite](r.composite.md), [r.mapcalc](r.mapcalc.md),
[r.shade](r.shade.md), [i.his.rgb](i.his.rgb.md),
[i.rgb.his](i.rgb.his.md)*

## AUTHOR

Glynn Clements (based upon *[d.his](d.his.md)*)

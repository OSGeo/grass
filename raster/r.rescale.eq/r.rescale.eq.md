## DESCRIPTION

The *r.rescale.eq* program rescales the range of category values
appearing in a raster map layer with equalized histogram. A new raster
map layer, and an appropriate category file and color table based upon
the original raster map layer, are generated with category labels that
reflect the original category values that produced each category. This
command is useful for producing representations with a reduced number of
categories from a raster map layer with a large range of category values
(e.g., elevation). *Rescaled* map layers are appropriate for use in such
GRASS GIS commands as *[r.stats](r.stats.md)*,
*[r.report](r.report.md)*, and *[r.coin](r.coin.md)*.

## EXAMPLE

To rescale an elevation raster map layer with category values ranging
from 1090 meters to 1800 meters into the range 0-255, the following
command line could be used (without the *from* parameter, the full value
range will be used):

```sh
r.rescale.eq input=elevation from=1090,1800 output=elevation.255 to=0,255
```

## NOTES

Category values that fall beyond the input range will become NULL. This
allows the user to select a subset of the full category value range for
rescaling if desired. This also means that the user should know the
category value range for the input raster map layer. The user can
request the *r.rescale.eq* program to determine this range, or can
obtain it using the *[r.describe](r.describe.md)* or
*[r.info](r.info.md)* command. If the category value range is determined
using *r.rescale.eq*, the input raster map layer is examined, and the
minimum and maximum non-NULL category values are selected as the input
range.

## SEE ALSO

*[r.coin](r.coin.md), [r.describe](r.describe.md), [r.info](r.info.md),
[r.mapcalc](r.mapcalc.md), [r.reclass](r.reclass.md),
[r.rescale](r.rescale.md), [r.report](r.report.md),
[r.resample](r.resample.md), [r.stats](r.stats.md)*

## AUTHOR

Michael Shapiro, U.S.Army Construction Engineering Research Laboratory

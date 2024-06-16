## DESCRIPTION

The *r.rescale* program rescales the range of category values appearing
in a raster map layer. A new raster map layer, and an appropriate
category file and color table based upon the original raster map layer,
are generated with category labels that reflect the original category
values that produced each category. This command is useful for producing
representations with a reduced number of categories from a raster map
layer with a large range of category values (e.g., elevation).
*Rescaled* map layers are appropriate for use in such GRASS GIS commands
as *[r.stats](r.stats.html)*, *[r.report](r.report.html)*, and
*[r.coin](r.coin.html)*.

## EXAMPLE

To rescale an elevation raster map layer with category values ranging
from 1090 meters to 1800 meters into the range 0-255, the following
command line could be used (without the *from* parameter, the full value
range will be used):

::: code
    r.rescale input=elevation from=1090,1800 output=elevation.255 to=0,255
:::

## NOTES

Category values that fall beyond the input range will become NULL. This
allows the user to select a subset of the full category value range for
rescaling if desired. This also means that the user should know the
category value range for the input raster map layer. The user can
request the *r.rescale* program to determine this range, or can obtain
it using the *[r.describe](r.describe.html)* or *[r.info](r.info.html)*
command. If the category value range is determined using *r.rescale*,
the input raster map layer is examined, and the minimum and maximum
non-NULL category values are selected as the input range.

## SEE ALSO

*[r.coin](r.coin.html), [r.describe](r.describe.html),
[r.info](r.info.html), [r.mapcalc](r.mapcalc.html),
[r.reclass](r.reclass.html), [r.rescale.eq](r.rescale.eq.html),
[r.report](r.report.html), [r.resample](r.resample.html),
[r.stats](r.stats.html), [r.univar](r.univar.html)*

## AUTHOR

Michael Shapiro, U.S.Army Construction Engineering Research Laboratory

## DESCRIPTION

*d.rast* displays the specified raster map in the active display frame
on the graphics monitor.

## EXAMPLE

Display raster map \"elevation\":

```
d.rast map=elevation
```

::: {align="center" style="margin: 10px"}
![d.rast elevation](d_rast_elevation.png){border="0"}\
*Figure: elevation raster map visualization*
:::

Display raster map \"elevation\" but only the raster cells with values
between 75 and 80 meters:

```
d.rast map=elevation values=75-80
```

::: {align="center" style="margin: 10px"}
![d.rast elevation with
values](d_rast_elevation_values.png){border="0"}\
*Figure: elevation raster map showing values between 75 and 80 meters*
:::

Display raster map \"landuse96_28m\" but only categories 1 and 2:

```
d.rast landuse96_28m values=1,2
```

::: {align="center" style="margin: 10px"}
![d.rast landuse](d_rast_landuse.png){border="0"}\
*Figure: landuse raster map showing categories 1 and 2*
:::

## SEE ALSO

*[d.rast.arrow](d.rast.arrow.html), [d.rast.num](d.rast.num.html),
[d.rast.leg](d.rast.leg.html), [d.legend](d.legend.html),
[d.mon](d.mon.html), [d.erase](d.erase.html), [d.vect](d.vect.html)*

*[wxGUI](wxGUI.html)*

## AUTHOR

James Westervelt, U.S. Army Construction Engineering Research Laboratory

## DESCRIPTION

*d.rast* displays the specified raster map in the active display frame
on the graphics monitor.

## EXAMPLE

Display raster map "elevation":

```shell
d.rast map=elevation
```

<div align="center" style="margin: 10px">

<img src="d_rast_elevation.png" data-border="0"
alt="d.rast elevation" />
*Figure: elevation raster map visualization*

</div>

Display raster map "elevation" but only the raster cells with values
between 75 and 80 meters:

```shell
d.rast map=elevation values=75-80
```

<div align="center" style="margin: 10px">

<img src="d_rast_elevation_values.png" data-border="0"
alt="d.rast elevation with values" />
*Figure: elevation raster map showing values between 75 and 80 meters*

</div>

Display raster map "landuse96_28m" but only categories 1 and 2:

```shell
d.rast landuse96_28m values=1,2
```

<div align="center" style="margin: 10px">

<img src="d_rast_landuse.png" data-border="0" alt="d.rast landuse" />
*Figure: landuse raster map showing categories 1 and 2*

</div>

## SEE ALSO

*[d.rast.arrow](d.rast.arrow.md), [d.rast.num](d.rast.num.md),
[d.rast.leg](d.rast.leg.md), [d.legend](d.legend.md), [d.mon](d.mon.md),
[d.erase](d.erase.md), [d.vect](d.vect.md)*

*[wxGUI](wxGUI.md)*

## AUTHOR

James Westervelt, U.S. Army Construction Engineering Research Laboratory

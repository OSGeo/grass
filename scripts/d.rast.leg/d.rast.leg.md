## DESCRIPTION

*d.rast.leg* shows a raster map along with its legend. It first clears
the entire screen, then divides it into a main (left) and a minor
(right) frames, and displays a raster map in the main frame and the map
legend in the minor frame. The main frame remains active when the
program finishes.

## NOTES

The legend may be flipped with the **-f** flag.

If the **lines** parameter is not given then the legend frame will
display as many lines as number of categories in the map, otherwise, it
will display the first **lines** minus 1 categories with the rest being
truncated.

The user may adjust the **lines** parameter or the size of graphics
window to get an appropriate result.

The user may specify a second raster map with the **raster** parameter
from which the legend is generated. This is useful to visualize (time)
series of raster maps with a common static legend instead of the default
dynamic legend.

To remove all frames when clearing the display, use `d.erase -f`.

## EXAMPLE

In this example, the polar diagram of the 'aspect' angle map in the
North Carolina sample dataset is generated:

```sh
g.region raster=landclass96 -p
d.rast.leg landclass96
```

## SEE ALSO

*[d.legend](d.legend.md), [d.erase](d.erase.md), [d.rast](d.rast.md)*

## AUTHORS

Jianping Xu, Scott Madry, Rutgers University  
Markus Neteler

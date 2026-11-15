## DESCRIPTION

*d.northarrow* displays a north arrow symbol at the given screen
coordinates. If no coordinates are given it will draw the north arrow in
the bottom right of the display. It can draw the north arrow in a number
of styles (see the [wiki
page](https://grasswiki.osgeo.org/wiki/Cartography#Display_monitors) for
details). With certain styles of north arrow label 'N' is displayed by
default, and can be changed with option **label**, for example for
different languages. The label can be hidden with **-t** flag.

North arrow can be rotated, for example to align with true north, not
grid north. The angle in degrees counter-clockwise (or radians with
**-r** flag) can be specified with option **rotation**. Label is rotated
together with the arrow, unless flag **-w** is specified.

## EXAMPLES

Display a north arrow symbol as a basic compas with label NORTH, rotated
by 8 degrees with label, with black line and gray fill:  

```sh
d.mon wx0
d.northarrow style=basic_compas rotation=8 label=NORTH -w color=black fill_color=gray
d.mon -r
```

## SEE ALSO

*[d.barscale](d.barscale.md), [d.graph](d.graph.md),
[d.grid](d.grid.md), [d.legend](d.legend.md)*

## AUTHORS

Hamish Bowman, *Department of Geology, University of Otago, New
Zealand*  
Improvements as part of GSoC 2016 by Adam Laza, *CTU in Prague*

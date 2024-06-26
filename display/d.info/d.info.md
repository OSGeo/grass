## DESCRIPTION

*d.info* displays information about the active display monitor. Display
monitors are maintained by *[d.mon](d.mon.html)*.

## EXAMPLES

::: code
    d.mon start=cairo

    d.info -r
    rectangle: 0.000000 640.000000 0.000000 480.000000
:::

## NOTES

Units are screen pixels (except for **-g** flag where map units are
used).\
Where two numbers are given the format is: width, height.\
Where four numbers are given the format is: left, right, top, bottom.

Note: GRASS display pixel coordinates are measured from the top left.

## SEE ALSO

*[d.mon](d.mon.html), [d.vect](d.vect.html), [d.rast](d.rast.html)*

## AUTHOR

Glynn Clements

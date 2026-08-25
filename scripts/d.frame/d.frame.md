## DESCRIPTION

*d.frame* manages display frames on the current user's graphics monitor.
Graphics are displayed in rectangular frames on whatever graphics
monitor the user is currently directing GRASS display output to (defined
by *[d.mon](d.mon.md)* module). These frames are created and managed
with this module.

Note that GRASS frame contents *are not* retained when one frame covers
another. You cannot shuffle frames from top to bottom and then back
again. They simply define rectangular areas on the screen where
subsequent drawing will occur.

## NOTES

The coordinates for the **at** option are stated in the form
*top,bottom,left,right* values are in percent. The upper-left corner of
the graphics monitor always is at location 0,0 while the monitor's
lower-right corner is always at 100,100.

If the user has created multiple display frames that overlap one
another, whatever the user displays in the active frame will overwrite
those portions of the underlying frame where these frames overlap.

## EXAMPLE

```sh
# start a new graphics monitor, the data will be rendered to
# /tmp/map.png image output file of size 600x540px
d.mon cairo out=/tmp/map.png width=600 height=540 --o

# set up region
g.region raster=elevation

# remove all frames and erase the current graphics monitor
d.frame -e

# create a first frame and display 'landuse96_28m' raster map including text label
# order: bottom,top,left,right - in percent
d.frame -c frame=first at=0,50,0,50
d.rast landuse96_28m
d.text text='Landuse' bgcolor=220:220:220 color=black size=6

# create a second frame and display 'streams' vector map
d.frame -c frame=second at=0,50,50,100
d.vect streams color=blue
d.text text='Streams' bgcolor=220:220:220 color=black size=6

# create a third frame and display 'elevation' raster map including text label and scale
d.frame -c frame=third at=50,100,0,50
d.rast elevation
d.text text='Elevation' bgcolor=220:220:220 color=black size=6
d.barscale at=0,10 style=line bgcolor=none

# create a fourth frame and display RGB composition map including text label
d.frame -c frame=fourth at=50,100,50,100
d.rgb red=lsat7_2002_30 green=lsat7_2002_20 blue=lsat7_2002_10
d.text text='RGB true colors' bgcolor=220:220:220 color=black size=6

# release the current graphics monitor
d.mon -r
```

![d.frame example](d_frame.png)  
*Figure: d.frame example*

## SEE ALSO

*[d.erase](d.erase.md), [d.info](d.info.md), [d.mon](d.mon.md),
[d.redraw](d.redraw.md)*

[GRASS environment variables for
rendering](variables.md#list-of-selected-grass-environment-variables-for-rendering)
(GRASS_RENDER_FRAME)

## AUTHORS

Martin Landa, Czech Technical University in Prague, Czech Republic

Based on *d.frame* from GRASS 6:  
James Westervelt, U.S. Army Construction Engineering Research
Laboratory  
Michael Shapiro, U.S. Army Construction Engineering Research Laboratory

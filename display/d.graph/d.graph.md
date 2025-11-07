## DESCRIPTION

*d.graph* draws graphics that are described either from standard input
(default), or within a file (if an **input** file name is identified on
the command line). If graphics commands are entered from standard input,
a *CTRL-d* is used to signal the end of input to *d.graph*. Coordinates
are given either as a percentage of frame height and width (default) or
in geographic coordinates (with the **-m** flag).

The program can be run interactively or non-interactively. The user can
run the program completely non-interactively by specifying the name of a
graphics file containing the *d.graph* graphics commands. If run
non-interactively the *d.graph* command is saved to the display's dedraw
history. The user can also elect to run the program partially
interactively, by specifying any/all of the parameters *except* the
graphics file **input=***name* parameter on the command line. In this
case, *d.graph* will expect the user to input *d.graph* graphics
commands from standard input (i.e., the keyboard) and will (silently)
prompt the user for these graphics commands.

Alternately, the user can simply type **d.graph** on the command line,
and be prompted for the values of all parameters. In this case, the user
is presented with the standard GRASS GUI interface.

The default coordinate system used is 0-100 percent of the active frame
in x and similarly 0-100 in y, regardless of the graphics monitor
display frame size and aspect. The (0,0) location is the lower left
corner of the active graphics monitor display frame. All values may be
floating point. If the **-m** flag is given, geographic coordinates will
be used instead.

## COMMANDS

The graphics language is simple, and uses the following commands:  

**\#** *comment*  
A line of comment which is ignored in the processing.

**move** *xpos ypos*  
The current location is updated to *xpos ypos*. Unless the **-m** flag
is used, values are stated as a percent of the active display frame's
horizontal (*xpos*) and vertical (*ypos*) size, and may be floating
point values. Values are between 0-100. **Note.** A space must separate
*xpos* and *ypos*.

**draw** *xpos ypos*  
A line is drawn in the current color from the current location to the
new location *xpos ypos*, which then becomes the current location.
Unless the **-m** flag is used, values are stated as a percent of the
active display frame's horizontal (*xpos*) and vertical (*ypos*) size,
and may be floating point values. Values are between 0-100. **Note.** A
space must separate *xpos* and *ypos*.

**polygon**  
   *xpos ypos*  
   *xpos ypos*  
  ...  
The coordinates appearing beneath the word *polygon*, one pair per line,
circumscribe a polygon that is to be filled with the current color.

**polyline**  
   *xpos ypos*  
   *xpos ypos*  
  ...  
The coordinates appearing beneath the word *polyline*, one pair per
line, circumscribe a polygon that is not to be filled with color.

**color** *color*  
Sets the current color to that stated; subsequent graphics will be drawn
in the stated color, until the current color is set to a different
color. Options are *red*, *orange*, *yellow*, *green*, *blue*, *indigo*,
*violet*, *brown*, *magenta*, *gray*, *white*, *black*, an R:G:B triplet
(separated by colons), or the word "none" (draws in the default
background color).

**text** *line-of-text*  
The stated text is drawn at the current location using the current
color, and the new current location is then positioned at the end of the
text string.

**size** *xper yper*  
Subsequent text will be drawn such that the text is *xper* percent of
the graphics monitor display frame wide and *yper* percent of the frame
high. By default, the text size is set to 5 percent of the active
frame's width and 5 percent of the frame's height. If only one value is
given, then that value will be used for both x and y scaling.
A space must separate *xper* and *yper*.

**symbol** *type size xper yper \[line_color \[fill_color\]\]*  
A symbol is drawn at the given size on the display monitor. The *xper*
and *yper* options define the center of the icon and are given as a
percentage of the display frame (`0,0` is lower left). The symbol can be
any of those stored in `$GISBASE/etc/symbol/` (e.g. *basic/circle*) or
stored in the user's mapset directory in the form
`$MAPSET/symbol/`*type/name*. The colors may be either a standard color
name, an R:G:B triplet, or "none". If using an R:G:B triplet, each color
value can range from 0-255. If not specified the default *line_color* is
black and the default *fill_color* is grey.

**rotation** *angle*  
Subsequent text and symbols will be drawn such that they are rotated
*angle* degrees counter-clockwise from east.

**icon** *type size x y*  
Draws an icon of types *o*, *x*, or *+* with specified *size* (in %) at
location *x,y*. Note: type *o* designates a square.

**width** *value*  
Subsequent lines (including non-FreeType text) will be drawn with the
given pixel thickness.  
The default value is 0.

## EXAMPLES

For an example use of *d.graph*, examine the contents of the command
file *[grass_logo.txt](grass_logo.txt)* located in the *d.graph* source
code directory. It will draw the CERL GRASS logo using the *d.graph*
graphing commands stored in the file. Note that the coordinates in the
*[grass_logo.txt](grass_logo.txt)* file were taken directly off an image
drawn by hand on graph paper.

A dynamic example can be found in the *d.polar* shell script.

### Draw a "star" symbol at a given map coordinate

```sh
echo "symbol basic/star 20 2264417 5413182 black red" | d.graph -m
```

### Split the screen into quadrants

```sh
d.frame -s full_screen

d.graph << EOF
  color 80:80:120
  polygon
   0 49.75
   0 50.25
   100 50.25
   100 49.75
  polygon
   49.85 0
   50.15 0
   50.15 100
   49.85 100
EOF
```

## NOTES

*d.graph* remembers the last screen location (*xpos ypos*) to which the
user moved, even after the user erases the display frame. If the user
runs *d.graph* repeatedly, and wishes to start anew with the default
(xpos ypos) screen location, the user should *clear* the display frame
between runs of *d.graph*.

## LIMITATIONS

There are no automated ways of generating graphic images. It is
anticipated that GRASS user sites will write programs to convert output
from a resident graphics editor into GRASS *d.graph* format. (e.g. EPS
-\> *d.graph*, perhaps with the help of a
[pstoedit](http://www.pstoedit.net/) plugin)

## SEE ALSO

*[d.font](d.font.md), [d.labels](d.labels.md), [d.polar](d.polar.md),
[d.text](d.text.md), [d.where](d.where.md)*

## AUTHOR

James Westervelt, U.S. Army Construction Engineering Research Laboratory

## DESCRIPTION

*v.label* makes a label-file from a GRASS vector map with labels created
from attributes in the attached table. If no label file name is given,
the name of the source map is used.

## NOTES

If the *fontsize* option is given then the *space* option is determined
automatically *from the current display window*, otherwise the *space*
option should be set roughly the same as the *size* option.

A description of the labels file follows.

The file is located in `$MAPSET/paint/labels/`. The file is a plain-text
ASCII file containing the following fields:

**TEXT**:  
Lines in multiple line labels will appear one above the next. More than
one line of text can be specified by notating the end of a line with a
'**`\n`**'.  
(e.g. `SPEARFISH`**`\n`**`SOUTH DAKOTA)`.

**LOCATION**:  
Determines where the text will be located on the image. The user
specifies the easting and northing, and (optionally) specifies a
vertical and horizontal offset from the specified easting/northing.
These offsets are provided to allow finer placement of labels and are
measured in local pixels. Thus in [*d.labels*](d.labels.md) the offset
is measured in screen pixels, and in [*ps.map*](ps.map.md) the offset is
measured in PostScript points (i.e. 1/72" steps).

**PLACEMENT**:  
Determines which part of the label to which the location refers. If
placement is unspecified, the label is centered (*center*), by default.
Label placement may be specified as:

```sh
    lower left    (lower left corner of the text)
    lower right    (lower right corner of the text)
    lower center    (bottom center of the text)

    upper left    (upper left corner of the text)
    upper right    (upper right corner of the text)
    upper center    (top center of the text)

    center    (center of the text)

```

**FONT**:  
This specifies the font to use.

The following fonts are available for use with
[*d.labels*](d.labels.md):

```sh
  cyrilc gothgbt gothgrt gothitt greekc greekcs greekp greeks
  italicc italiccs italict romanc romancs romand romans romant
  scriptc scripts
```

Alternatively the path to a FreeType (.ttf) font may be given. (for
*d.labels* only)

The word *standard* can be used to specify the default font (which is
*romans*).

Note [*ps.map*](ps.map.md) can override this setting to use other fonts.
Its default font is Helvetica.

**TEXT SIZE**:  
This determines the size of the letters. The *size* specifies the
vertical height of the letters in meters on the ground. Thus text will
grow or shrink depending on the scale at which the map is drawn.
Alternatively *fontsize* can set the font size in normal font points.

**TEXT COLOR**:  
This selects the text color. If unspecified, the label's text is drawn
in *black*, by default. The text color can be specified in one of
several ways:

1. By color name: *aqua*, *black*, *blue*, *brown*, *cyan*, *gray*,
   *green*, *grey*, *indigo*, *magenta*, *orange*, *purple*, *red*,
   *violet*, *white*, *yellow*
2. As red, green, blue component values. (0-255)  
    for example: `128:100:200`
3. Specify "`none`" to suppress the lettering.

**WIDTH**:  
This determines the line thickness of the border box.  
The maximum value is 25.0.

**HIGHLIGHT COLOR**:  
The text can be highlighted in another color so that it appears to be in
two colors. The text is drawn first in this color at a wider line width,
and then redrawn in the text color at the regular line width. No
highlight color ("`none`") is used by default, if unspecified by the
user. To specify use of no highlight color, specify "`none`". (See TEXT
COLOR above for a list of permissible color names.)

**HIGHLIGHT WIDTH**:  
Specifies how far from the text lines (in units of pixels) the highlight
color should extend. The default highlight width is set to *0* (i.e., no
highlight color).

**BACKGROUND COLOR**:  
Text may be boxed in a solid color by specifying a background color.
Specify "`none`" for no background. The default background color
setting, if unspecified by the user, is *white*. (See TEXT
COLOR above for a list of permissible color names.)

**BORDER COLOR**:  
Select a color for the border around the background. Specify "`none`" to
suppress the border. The default border color used, if unspecified, is
*black*. (See TEXT COLOR above for a list of permissible
color names.)

**OPAQUE TO VECTORS**:  
*yes\|no*. This field only has meaning if a background color is
selected. *yes* will prevent vector lines from entering the background.
*no* will allow vector lines to enter the background. The default
setting, if unspecified by the user, is *yes*.

## EXAMPLE

Spearfish example with TrueType font (path may differ):

```sh
v.label -a map=roads column=label labels=lroads \
        font=/usr/X11R6/lib/X11/fonts/TTF/luximri.ttf
d.vect roads
d.labels lroads
```

Since the label files are simple text files, you can merge them together
if you like. For example if you set the label colors based on database
attributes using multiple runs with the **where** option. This example
uses the standard UNIX `cat` program.

```sh
cd $MAPSET/paint/labels/
cat file1 file2 file3 file4 > file_all
```

## SEE ALSO

*[d.labels](d.labels.md), [ps.map](ps.map.md)*

## AUTHORS

Philip Verhagen (original s.label)  
Radim Blazek (GRASS 6 port)  
Hamish Bowman (enhancements)

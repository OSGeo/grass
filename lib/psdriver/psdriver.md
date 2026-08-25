---
description: PostScript DISPLAY DRIVER
---

# PostScript DISPLAY DRIVER

*PS display driver* to create PostScript files.

## DESCRIPTION

The PS driver generates a PostScript file from GRASS display commands.

## USAGE

### Environment variables

The PS driver can be enabled by setting **GRASS_RENDER_IMMEDIATE**
variable, eg.

```sh
export GRASS_RENDER_IMMEDIATE=ps
```

Several environment variables affect the operation of the PS driver:

- **GRASS_RENDER_WIDTH=xxx**  
  the width of the image map (default is 640).
- **GRASS_RENDER_HEIGHT=yyy**  
  the height of the image map (default is 480).
- **GRASS_RENDER_TRUECOLOR=\[TRUE\|FALSE\]**  
  sets true-color support. Default is FALSE.
- **GRASS_RENDER_FILE**  
  name of output file. If it ends with ".eps" an EPS file will be
  created.
- **GRASS_RENDER_PS_PAPER**  
  sets the screen dimensions and margins to fit a standard paper size,
  see also GRASS_RENDER_WIDTH, GRASS_RENDER_HEIGHT.
- **GRASS_RENDER_PS_LANDSCAPE**  
  if `TRUE`, the screen is rotated 90 degrees counter-clockwise so that
  a "landscape" screen fits better on "portrait" paper.
- **GRASS_RENDER_PS_HEADER**  
  if `FALSE`, the output is appended to any existing file, and no prolog
  or setup sections are generated.
- **GRASS_RENDER_PS_TRAILER**  
  if `FALSE`, no trailer section is generated.

### Example

```sh
export GRASS_RENDER_IMMEDIATE=ps
export GRASS_RENDER_TRUECOLOR=TRUE

g.region raster=elevation
d.rast elevation
d.vect roadsmajor color=red
```

This writes a file named `map.ps` in your current directory.

## NOTES

The resolution of the output files is defined by current region extents.
Use `g.region -p` to get the number of rows and cols and use the
environment variables to set the image size. If you would like a larger
image, multiply both rows and cols by the same whole number to preserve
the aspect ratio.

GRASS_RENDER_TRUECOLOR requires either PostScript level 2 or level 1
plus the colorimage and setrgbcolor operators (this is the case for
colour printers which pre-date level 2 PostScript).

Masked images (`d.rast`, `d.rgb`, `d.his -n`) require PostScript level
3.

## SEE ALSO

*[Cairo driver](cairodriver.md), [PNG driver](pngdriver.md), [HTML
driver](htmldriver.md), [variables](variables.md)*  
  
*[d.rast](d.rast.md), [d.vect](d.vect.md), [d.mon](d.mon.md),
[d.erase](d.erase.md), [d.redraw](d.redraw.md)*

## AUTHOR

Glynn Clements, 2007

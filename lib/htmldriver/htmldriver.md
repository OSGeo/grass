---
description: HTML DISPLAY DRIVER
---

# HTML DISPLAY DRIVER

*HTML display driver* to create HTML image maps.

## DESCRIPTION

The HTML driver allows the generation of HTML image maps for area vector
data. HTML image maps are used in conjunction with images to provide
unique URL targets for different portions of an image. The HTML driver
can create both client-side image maps embedded into HTML files, or
server-side image maps used by web server software.

Polygons can at most have 100 vertices (this limit imposed by HTML image
map formats, see **GRASS_RENDER_HTMLMAXPOINTS** below.) The driver will
attempt to trim polygons that have more that 100 vertices by removing
vertices with the least amount of angle to the next vertice. Also, any
polygon that is entirely bounded by another polygon will be discarded.

Text written to the driver before polygons are used as the HREF tag for
all subsequent polygons written. All polygons that exist in a vector map
will have the same HREF tag.

The only GRASS display commands that should be used with this driver
are:

- *[d.text](d.text.md)* - pass href information for resulting image
  maps.
- *[d.vect](d.vect.md)* - draw polygons from a vector map.

## USAGE

### Environment variables

The HTML driver can be enabled by setting **GRASS_RENDER_IMMEDIATE**
variable, eg.

```sh
export GRASS_RENDER_IMMEDIATE=html
```

Several environment variables affect the operation of the HTML driver:

- **GRASS_RENDER_WIDTH=xxx**  
  the width of the image map (default is 640).
- **GRASS_RENDER_HEIGHT=yyy**  
  the height of the image map (default is 480).
- **GRASS_RENDER_HTMLTYPE=type**  
  type of image map to create (default is CLIENT):
  **`CLIENT`**    Netscape/IE client-side image map (NAME="map").
  **`APACHE`**    Apache/NCSA server-side image map.
  **`RAW`**         Raw url and polygon vertices (*url  x1  y1  x2  y2
  .....*), suitable for conversion to CERN server format, or any other
  format with user supplied conversion program.
- **GRASS_RENDER_FILE=filename**  
  specifies the resulting file to store the html image map, default is
  `htmlmap`. Files without absolute path names are written in the
  current directory where the driver was started.  
  *Any existing file of the same name is overwritten without warning.*
- **GRASS_RENDER_HTMLMINDIST=n**  
  specifies the minimum distance in pixels that a point must change from
  the previous point to keep in the list of vertices for a polygon. The
  default is `2`, which means that a point's x and y difference from the
  previous point must change by a number of pixels greater than this
  value. This parameter helps to eliminate closely spaced points.
- **GRASS_RENDER_HTMLMINBBOX=n**  
  specifies the minimum bounding box dimensions to record a polygon as a
  clickable area. The default is `2`, which means that a polygon with a
  bounding box of less than this value is not included. This parameter
  helps to eliminate polygons than are a point or line.
- **GRASS_RENDER_HTMLMAXPOINTS=n**  
  specifies the maximum number of vertices included in a polygon's
  clickable area. The default is `99`. Some browsers can only
  accommodate polygons of 100 vertices or less. The HTMLMAP driver
  automatically ensures that a polygon is closed by making the last
  point the same as the first point.

### Example

Start up the driver

```sh
g.region vector=zipcodes_wake
d.mon start=html
```

Display text strings (HREF's) and polygons

```sh
echo "https://en.wikipedia.org/wiki/Raleigh,_North_Carolina" | d.text
d.vect map=zipcodes_wake where="ZIPNAME = 'RALEIGH'"
echo "https://en.wikipedia.org/wiki/Cary,_North_Carolina" | d.text
d.vect map=zipcodes_wake where="ZIPNAME = 'CARY'" fill_color=180:200:210
```

Stop the driver once all polygon have been displayed. This will create a
file named 'htmlmap' in your current directory:

```sh
d.mon stop=html
```

You will also want to create an image for your image map. Use the PNG
driver and other utilities to create .gif or .jpg files. *The following
example is somewhat out of date and refers to options available in GRASS
5.*

```sh
# using previous GRASS_RENDER_WIDTH & GRASS_RENDER_HEIGHT
d.mon start=png
d.rast map=terrain
d.vect map=area51  fillcolor=white  linecolor=blue
d.vect map=roswell fillcolor=yellow linecolor=blue
d.vect map=states  color=green
d.vect map=roads   color=black
d.mon stop=png


# make the region the same as the newly created cell for ppm export
g.region save=saved.reg
g.region raster=D_cell
r.out.ppm -q input=D_cell output=alien.ppm

# use the netpbm utilities to create a gif (quantize if needed)
ppmquant 128 <alien.ppm |  ppmtogif >alien.gif

# assemble some html with the image and the image map
echo '<html><body><img src="alien.gif" usemap="#map">' >alien.html
cat htmlmap                                           >>alien.html
echo '</body></html>'                                 >>alien.html

# don't forget to reset your region
g.region region=saved.reg

# take a look and test it out
netscape file:`pwd`/alien.html &
```

## NOTES

HTML was adapted from the CELL driver in GRASS 4.3. Point-in-polygon
test code was lifted from Randolph Franklin's web page, see

- <http://www.ecse.rpi.edu/Homepages/wrf/>
- <http://www.ecse.rpi.edu/Homepages/wrf/research/geom/pnpoly.html>

If you create an HTML file with two or more images and image maps, you
will need to edit the map names. The HTML driver creates its map with
the name `map`. A small sed script can easily change the map name:

```sh
sed -e 's/NAME="map"/NAME="foomap"/' < htmlmap > foomap.html
```

## SEE ALSO

*[Cairo driver](cairodriver.md), [PNG driver](pngdriver.md), [HTML
driver](htmldriver.md), [variables](variables.md)*  
  
*[d.rast](d.rast.md), [d.vect](d.vect.md), [d.mon](d.mon.md),
[d.erase](d.erase.md), [d.redraw](d.redraw.md)*

## AUTHOR

Glynn Clements

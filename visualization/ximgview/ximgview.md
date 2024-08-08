## DESCRIPTION

*ximgview* is a simple X11 image viewer for 32-bpp BMP images, as can be
created with the *[PNG](pngdriver.html)* and *[cairo](cairodriver.html)*
drivers. The display is continually refreshed.

## NOTES

The display driver must be configure to map the file, with
*GRASS_RENDER_FILE_MAPPED=TRUE*. This ensures that the file will remain
a constant size, rather than being truncated whenever it is updated.

## EXAMPLE

(bash shell syntax)

```
export GRASS_RENDER_FILE=map.bmp
export GRASS_RENDER_WIDTH=640
export GRASS_RENDER_HEIGHT=480
export GRASS_RENDER_IMMEDIATE=PNG
export GRASS_RENDER_FILE_MAPPED=TRUE
export GRASS_RENDER_FILE_READ=TRUE

d.erase
ximgview $GRASS_RENDER_FILE percent=50 &
d.rast elevation.dem
d.vect roads
```

## SEE ALSO

*[PNG driver](pngdriver.html)\
[cairo driver](cairodriver.html)\
[wximgview](wximgview.html)*

## AUTHOR

Glynn Clements

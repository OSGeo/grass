## DESCRIPTION

*wxpyimgview* is a simple wxWidgets image viewer for 32-bpp BMP images,
as can be created with the *[PNG](pngdriver.md)* and
*[cairo](cairodriver.md)* drivers. The display is continually refreshed.

## NOTES

The display driver must be configure to map the file, with
*GRASS_RENDER_FILE_MAPPED=TRUE*. This ensures that the file will remain
a constant size, rather than being truncated whenever it is updated.

## SEE ALSO

*[PNG driver](pngdriver.md), [cairo driver](cairodriver.md)*

## AUTHOR

Glynn Clements

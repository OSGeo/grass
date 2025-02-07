## DESCRIPTION

*r.out.png* exports a GRASS GIS raster map in non-georeferenced Portable
Network Graphics (PNG) image format, respecting the current region
resolution and bounds.

Optionally the user can choose to export a World File (.wld) to provide
basic georeferencing support. When used with the transparency flag this
can create images useful for KML, TMS, or WMS overlays. (e.g. for use in
Google Earth or as OpenLayers tiles) If output is redirected to stdout,
the world file will be called `png_map.wld`.

## EXAMPLE

The example is based on the North Carolina sample data.

Export of the soil map to PNG format with world file:

```sh
g.region raster=soils_Kfactor -p
# export PNG file with additional world file
r.out.png input=soils_Kfactor output=soils_Kfactor -w
# verify
gdalinfo soils_Kfactor.png
```

## SEE ALSO

*[r.out.gdal](r.out.gdal.md), [r.out.ppm](r.out.ppm.md),
[r.out.ascii](r.out.ascii.md), [r.import](r.import.md)*

## AUTHORS

Alex Shevlakov  
Hamish Bowman

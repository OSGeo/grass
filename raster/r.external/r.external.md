## DESCRIPTION

*r.external* allows a user to link a GDAL supported raster file to a
binary raster map layer, from any GDAL supported raster map format, with
an optional title. The file is not imported but just registered as GRASS
raster map.

## NOTES

In essence, *r.external* creates a read-only link to the original
dataset which is only valid if the original dataset remains at the
originally indicated directory and filename.

## NULL data handling

GDAL-linked (*r.external*) maps do not have or use a NULL bitmap, hence
*r.null* cannot manipulate them directly. Here NULL cells are those
whose value matches the value reported by the GDALGetRasterNoDataValue()
function.

To introduce additional NULL values to a computation based on a
GDAL-linked raster, the user needs to either create a mask with with
*r.mask* and then "apply" it using e.g. *r.resample* or *r.mapcalc*, or
use *r.mapcalc* to create a copy with the appropriate categories changed
to NULL (`if()` condition).

## EXAMPLES

### RGB Orthophoto from GeoTIFF

```sh
# import of all channels (each channel will become a GRASS raster map):
r.external input=/home/user/data/maps/059100.tif output=ortho
g.region raster=ortho.3 -p
d.rgb r=ortho.1 g=ortho.2 b=ortho.3
r.composite r=ortho.1 g=ortho.2 b=ortho.3 output=ortho.rgb
```

### Processing workflow without data import and export

External raster maps to be processed can be directly linked using
*r.external*; likewise, results can be written out to standard raster
formats with *r.external.out* (GDAL supported formats):

```sh
# register GeoTIFF file to be used in current mapset:
r.external input=terra_lst1km20030314.LST_Day.tif output=modis_celsius

# define output directory for files resulting from GRASS calculation:
r.external.out directory=$HOME/gisoutput/ format="GTiff"

# perform GRASS calculation (here: extract pixels > 20 deg C)
# this stores the output map directly as GeoTIFF:
r.mapcalc "warm.tif = if(modis_celsius > 20.0, modis_celsius, null() )"

# cease GDAL output connection and turn back to write GRASS raster files:
r.external.out -r

# now use the resulting file elsewhere
gdalinfo $HOME/gisoutput/warm.tif
```

## REFERENCES

GDAL Pages: [https://gdal.org/](https://gdal.org)  

## SEE ALSO

*[r.import](r.import.md), [r.in.gdal](r.in.gdal.md),
[r.external.out](r.external.out.md)*

*[v.import](v.import.md), [v.in.ogr](v.in.ogr.md),
[v.external](v.external.md), [v.external.out](v.external.out.md)*

## AUTHOR

Glynn Clements

## DESCRIPTION

*r.external.out* instructs GRASS GIS to write subsequently generated
raster maps as data files (e.g. GeoTIFF) using GDAL instead of storing
them in GRASS raster format in the current mapset.

Any new raster map is immediately written out through GDAL as a file.

## NOTES

A relative directory path (parameter *directory*) is interpreted
relative to the current mapset directory, not the current directory
where the command was launched. An unspecified or empty directory (which
will occur if the user passes a simple filename for *output*) results in
the output file being placed in the "gdal/" subdirectory of the current
mapset directory.

## EXAMPLES

### Storing results from raster data analysis directly as GeoTIFF

The module *r.external.out* is used to write out processing results
directly in GeoTIFF format (any GDAL supported format can be used here):

```sh
# define output directory for files resulting from GRASS calculation(s)
# and target format:
mkdir $HOME/gisoutput/
# hint: the create options are not mandatory
r.external.out directory=$HOME/gisoutput/ format="GTiff" option="BIGTIFF=YES,COMPRESS=DEFLATE"
# prepare sample analysis
g.region raster=elevation -p

# perform GRASS calculation (here: filter by height, write > 120m, NULL otherwise)
# this will store the output map directly as GeoTIFF, so we use .tif extension:
r.mapcalc "elev_filt.tif = if(elevation > 120.0, elevation, null() )"

# ...the "elev_filt.tif" is immediately written.

# cease GDAL output connection and turn back to write out GRASS raster files:
r.external.out -r

# verify resulting file
gdalinfo $HOME/gisoutput/elev_filt.tif
```

### Complete workflow using only external geodata while processing in GRASS GIS

The module *r.external.out* can be used along with *r.external* to
process external geodata in GRASS while writing out the results directly
in GeoTIFF:

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

*[r.in.gdal](r.in.gdal.md), [r.out.gdal](r.out.gdal.md),
[r.external](r.external.md)*

## AUTHOR

Glynn Clements

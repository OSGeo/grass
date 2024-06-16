## DESCRIPTION

*t.rast.import* imports a space time raster dataset archive that was
exported with [t.rast.export](t.rast.export.html).

## NOTES

Optionally a base map name can be provided to avoid that existing raster
maps are overwritten by the map names that are used in the STRDS
archive.

The **directory** is used as work directory in case of import but can
also be used as a data directory when using GeoTIFF for the data
exchange.

## EXAMPLE

The North Carolina space time dataset contains a data package called
*lst_daily.tar.bzip2* with daily data from MODIS LST. Import it by
running:

::: code
    t.rast.import input=lst_daily.tar.bzip2 output=lst_daily \
                  basename=lst directory=/tmp
:::

## SEE ALSO

*[t.rast.export](t.rast.export.html), [t.create](t.create.html),
[t.info](t.info.html), [r.in.gdal](r.in.gdal.html),
[r.unpack](r.unpack.html), [t.vect.import](t.vect.import.html)*

## AUTHOR

Sören Gebbert, Thünen Institute of Climate-Smart Agriculture

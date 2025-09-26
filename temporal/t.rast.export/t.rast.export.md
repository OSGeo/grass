## DESCRIPTION

*t.rast.export* exports a space time raster dataset (strds) as a tar
archive. The archive contains the raster maps either as GeoTIFF files or
as GRASS binary files exported using *r.pack*. The map specific color
tables are exported in case of GeoTIFF files. In addition several
metadata files are created in the archive that describe the temporal
layout. All time stamps are stored in the file "list.txt", for each map
one row. The name of the map, the start time and the end time are
written. In case of a time instance, the start time is equal to the end
time. The "init.txt" file stores the temporal type, the number of maps,
the chosen export format and some other metadata. The "proj.txt" file
stores the coordinate reference system information as a proj4 string of
the project the space time raster dataset was exported from. The file
"readme.txt" describes the file format. The output of *r.info* for each
raster map in the space time dataset is stored in "metadata.txt".

The tar archive can be compressed using the **compress** option. Gzip
and bzip2 (default) are available. A **where** option can be specified,
to export only a subset of the space time dataset. Archives exported
with *t.rast.export* can be imported with
*[t.rast.import](t.vect.import.md)*.

## NOTES

The name of output file has to carry the suffix of the archive type, the
following suffix can be used:

- **.tar** in the case of **compress=no**
- **.tar.bzip2** in the case of **compress=bzip2**
- **.tar.gzip** in the case of **compress=gzip**

## EXAMPLE

In this example, all the raster maps of 2012 of "tempmean_monthly" will
be exported:

```sh
t.rast.export input=tempmean_monthly output=tempmean_monthly.tar.bzip2 \
              where="start_time >= '2012-01-01' and start_time < '2013-01-01'"

tar xvjf precipitation_daily.tar.bzip2

2012_01_tempmean.tif
2012_01_tempmean.color
2012_02_tempmean.tif
2012_02_tempmean.color
2012_03_tempmean.tif
2012_03_tempmean.color
2012_04_tempmean.tif
2012_04_tempmean.color
2012_05_tempmean.tif
2012_05_tempmean.color
2012_06_tempmean.tif
2012_06_tempmean.color
2012_07_tempmean.tif
2012_07_tempmean.color
2012_08_tempmean.tif
2012_08_tempmean.color
2012_09_tempmean.tif
2012_09_tempmean.color
2012_10_tempmean.tif
2012_10_tempmean.color
2012_11_tempmean.tif
2012_11_tempmean.color
2012_12_tempmean.tif
2012_12_tempmean.color
list.txt
proj.txt
init.txt
readme.txt
metadata.txt


cat init.txt
stds_type=strds
format=GTiff
temporal_type=absolute
semantic_type=mean
number_of_maps=48
north=320000.0
south=10000.0
east=935000.0
west=120000.0


cat proj.txt
+proj=lcc
+lat_1=36.16666666666666
+lat_2=34.33333333333334
+lat_0=33.75
+lon_0=-79
+x_0=609601.22
+y_0=0
+no_defs
+a=6378137
+rf=298.257222101
+towgs84=0.000,0.000,0.000
+to_meter=1


cat list.txt
2012_01_tempmean|2012-01-01 00:00:00|2012-02-01 00:00:00
2012_02_tempmean|2012-02-01 00:00:00|2012-03-01 00:00:00
2012_03_tempmean|2012-03-01 00:00:00|2012-04-01 00:00:00
2012_04_tempmean|2012-04-01 00:00:00|2012-05-01 00:00:00
2012_05_tempmean|2012-05-01 00:00:00|2012-06-01 00:00:00
2012_06_tempmean|2012-06-01 00:00:00|2012-07-01 00:00:00
2012_07_tempmean|2012-07-01 00:00:00|2012-08-01 00:00:00
2012_08_tempmean|2012-08-01 00:00:00|2012-09-01 00:00:00
2012_09_tempmean|2012-09-01 00:00:00|2012-10-01 00:00:00
2012_10_tempmean|2012-10-01 00:00:00|2012-11-01 00:00:00
2012_11_tempmean|2012-11-01 00:00:00|2012-12-01 00:00:00
2012_12_tempmean|2012-12-01 00:00:00|2013-01-01 00:00:00


cat readme.txt
This space time raster dataset was exported with t.rast.export of GRASS GIS 7

Files:
       *.tif  -- GeoTIFF raster files
     *.color  -- GRASS GIS raster color rules
     proj.txt -- Projection information in PROJ format
     init.txt -- GRASS GIS space time raster dataset information
     list.txt -- Time series file, lists all maps by name with interval
                 time stamps in ISO-Format. Field separator is |
 metadata.txt -- The output of t.info
   readme.txt -- This file
```

## SEE ALSO

*[t.rast.import](t.rast.import.md), [t.create](t.create.md),
[t.info](t.info.md), [r.out.gdal](r.out.gdal.md), [r.pack](r.pack.md),
[t.vect.export](t.vect.export.md)*

## AUTHOR

Sören Gebbert, Thünen Institute of Climate-Smart Agriculture

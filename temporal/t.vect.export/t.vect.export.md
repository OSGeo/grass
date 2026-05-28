## DESCRIPTION

*t.vect.export* exports a space time vector dataset (stvds) to a tar
archive as either GML (using *[v.out.ogr](v.out.ogr.md)*) or GRASS pack
files. In addition to the pack or GML files, several metadata files are
also created in the archive that describe the temporal layout. All time
stamps are stored in the file "list.txt", for each map one row. The name
of the map, the start time and the end time are written. In case of a
time instance, the start time is equal to the end time. The "init.txt"
file stores the temporal type, the number of maps, the chosen export
format and some other metadata. The "proj.txt" file stores the
coordinate reference system information as a proj4 string of the project
the space time vector dataset was exported from. The file "readme.txt"
describes the file format. The output of *v.info* for each vector map in
the space time dataset is stored in "metadata.txt".

The tar archive can be compressed using the **compress** option. Gzip
and bzip2 (default) are available. A **where** option can be specified,
to export only a subset of the space time dataset. Archives exported
with *t.vect.export* can be imported with
*[t.vect.import](t.vect.import.md)*.

## NOTES

The name of output file has to carry the suffix of the archive type, the
following suffix can be used:

- **.tar** in the case of **compress=no**
- **.tar.bzip2** in the case of **compress=bzip2**
- **.tar.gzip** in the case of **compress=gzip**

## EXAMPLE

In this example, five vector maps are created and registered in a single
space time vector dataset named *random_locations*. Each vector map
represents random locations within the boundary of the state taken at 1
month intervals.

```sh
t.vect.export input=shoreline output=shoreline_nc.tar.bzip2

tar xvfj shoreline_nc.tar.bzip2
shoreline_1849_1873.xml
shoreline_1849_1873.xsd
shoreline_1925_1946.xml
shoreline_1925_1946.xsd
shoreline_1970_1988.xml
shoreline_1970_1988.xsd
shoreline_1997.xml
shoreline_1997.xsd
shoreline_1998.xml
shoreline_1998.xsd
shoreline_2003.xml
shoreline_2003.xsd
shoreline_2004.xml
shoreline_2004.xsd
shoreline_2009.xml
shoreline_2009.xsd
list.txt
proj.txt
init.txt
readme.txt
metadata.txt


cat init.txt
stds_type=stvds
format=GML
temporal_type=relative
semantic_type=mean
relative_time_unit=years
number_of_maps=8
north=1039175.31479
south=9403.301982
east=3052352.00337
west=651481.84739

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
shoreline_1849_1873|1849|1873
shoreline_1925_1946|1925|1946
shoreline_1970_1988|1970|1988
shoreline_1997|1997|1997
shoreline_1998|1998|1998
shoreline_2003|2003|2003
shoreline_2004|2004|2004
shoreline_2009|2009|2009


cat readme.txt
This space time vector dataset was exported with t.vect.export of GRASS GIS 7

Files:
       *.xml  -- Vector GML files
     proj.txt -- Projection information in PROJ format
     init.txt -- GRASS GIS space time vector dataset information
     list.txt -- Time series file, lists all maps by name with interval
                 time stamps in ISO-Format. Field separator is |
 metadata.txt -- The output of t.info
   readme.txt -- This file
```

## SEE ALSO

*[t.vect.import](t.vect.import.md), [t.create](t.create.md),
[t.info](t.info.md), [v.out.ogr](v.out.ogr.md), [v.unpack](v.unpack.md),
[t.rast.export](t.rast.export.md)*

## AUTHOR

Sören Gebbert, Thünen Institute of Climate-Smart Agriculture

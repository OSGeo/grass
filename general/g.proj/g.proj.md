## DESCRIPTION

*g.proj* provides a means of converting a coordinate reference system
(CRS) description between various formats.

For an introduction to map projections (with PROJ),see the manual page
of [r.proj](r.proj.md).

If compiled without [OGR](https://gdal.org/) present, the functionality
is limited to:

- Reporting the CRS information for the current project (previously
  called location), either in conventional GRASS (**-p** flag) or PROJ
  (**-j** flag) format
- Changing the datum, or reporting and modifying the datum
  transformation parameters, for the current project

When compiled with OGR, functionality is increased and allows output of
the CRS information in the Well-Known Text (WKT) format popularised by
proprietary GIS. In addition, if one of the parameters *georef*, *wkt*,
*proj4* or *epsg* is specified, rather than being read from the current
project, the CRS information is imported from an external source as
follows:

- With **georef**=*filename* g.proj attempts to invoke GDAL and OGR in turn
to read a georeferenced file *filename*. The CRS information will be read
from this file. If the file is not georeferenced or cannot be read,
XY (unprojected) will be used.

- When using **wkt**=*filename*, the file *filename* should contain a CRS
description in WKT format with or without line-breaks (e.g. a '.prj' file).
If **-** is given for the filename, the WKT description will be read from
stdin rather than a file.

- **proj4**=*description* should be a CRS description in [PROJ](https://proj.org/)
format, enclosed in quotation marks if there are any spaces. If **-** is
given for *description*, the PROJ description will be read from stdin
rather than as a directly-supplied command-line parameter.

- **epsg**=*number* should correspond to the index number of a valid co-ordinate
system in the [EPSG database](https://epsg.org/search/by-name). EPSG
code support is based upon a local copy of the GDAL CSV co-ordinate
system and datum information files, stored in the directory
`$GISBASE/etc/proj/ogr_csv`. These can be updated if necessary to
support future revisions of the EPSG database.

If datum information is incorrect or missing in the input co-ordinate
system definition (e.g. PROJ descriptions have very limited support for
specifying datum names), a GRASS datum abbreviation can instead be
supplied using the *datum* parameter. This will override any datum
contained in the input co-ordinate system, and discard any datum
transformation parameters. Enter datum=*list* to return a list of all
the datums supported by GRASS. Since any existing datum transformation
parameters will have been discarded, the *datumtrans* parameter should
in general always be used in conjunction with *datum*.

The **-p**, **-j**, **-w**, etc. flags are all functional when importing
CRS information from an external source, meaning that *g.proj* can be
used to convert between representations of the information. It is
**not** required that either the input or output be in GRASS format.

In addition however, if the **-c** flag is specified, *g.proj* will
create new GRASS CRS files (PROJ_INFO, PROJ_UNITS, WIND and
DEFAULT_WIND) based on the imported information. If the *project*
parameter is specified in addition to **-c**, then a new project will be
created. Otherwise the CRS information files in the current project will
be overwritten. The program will **not** warn before doing this.

The final mode of operation of *g.proj* is to report on the datum
information and datum transformation parameters associated with the
co-ordinate system. The **-d** flag will report a human-readable summary
of this.

## NOTES

If the input co-ordinate system contains a datum name but no
transformation parameters, and there is more than one suitable parameter
set available (according to the files datum.table and
datumtransform.table in `$GISBASE/etc/proj`), *g.proj* will check the
value of the *datumtrans* option and act according to the following:

- **-1:** List available parameter sets in a GUI-parsable (but also
  human-readable) format and exit.
- **0 (default):** Continue without specifying parameters - if used when
  creating a project, other GRASS modules will use the "default" (likely
  non-optimum) parameters for this datum if necessary in the future.
- **Any other number less than or equal to the number of parameter sets
  available for this datum:** Choose this parameter set and add it to
  the co-ordinate system description.

If the **-t** flag is specified, the module will attempt to change the
datum transformation parameters using one of the above two methods
**even if** a valid parameter set is already specified in the input
co-ordinate system. This can be useful to change the datum information
for an existing project.

Output is simply based on the input CRS information. g.proj does **not**
attempt to verify that the co-ordinate system thus described matches an
existing system in use in the world. In particular, this means there are
no EPSG Authority codes in the WKT output.

WKT format shows the false eastings and northings in the projected unit
(e.g. meters, feet) but in PROJ format it should always be given in
meters.

The maximum size of input WKT or PROJ CRS descriptions is limited to
8000 bytes.

## EXAMPLES

### Print information

Print the CRS information for the current project:  

```sh
g.proj -p
```

Print the CRS information for the current project in JSON format:

```sh
g.proj -p format=json
```

Print the CRS information for the current project in shell format:

```sh
g.proj -p format=shell
```

Print the CRS information for the current project in WKT format:

```sh
g.proj -p format=wkt
```

Print the CRS information for the current project in PROJ.4 format:

```sh
g.proj -p format=proj4
```

List the possible datum transformation parameters for the current
project:  

```sh
g.proj -t datumtrans=-1
```

### Create projection (PRJ) file

Create a '.prj' file in ESRI format corresponding to the current
project:  

```sh
g.proj -wef > irish_grid.prj
```

### Read CRS from file

Read the CRS information from a GeoTIFF file and print it in PROJ
format:  

```sh
g.proj -jf georef=ASTER_DEM20020508161837.tif
```

Convert the PROJ CRS description contained in a text file to WKT
format:  

```sh
cat proj4.description | g.proj -w proj4=-
```

### Create new project

Create a new project with the coordinate system referred to by EPSG code
4326 (Latitude-Longitude/WGS84), without explicitly specifying datum
transformation parameters:  

```sh
g.proj -c epsg=4326 project=latlong
```

Create a new project with the coordinate system referred to by EPSG code
3857 ([Pseudo-Mercator
Projection](https://spatialreference.org/ref/epsg/3857/))  

```sh
g.proj -c epsg=3857 project=google
```

Create a new project with the coordinate system referred to by EPSG code
29900 (Irish Grid), selecting datum transformation parameter set no.
2:  

```sh
# list available datums for EPSG code 29900
g.proj -t datumtrans=-1 epsg=29900
g.proj -c epsg=29900 datumtrans=2 project=irish_grid
```

Create a new project with the same coordinate system as the current
project, but forcing a change to datum transformation parameter set no.
1:  

```sh
g.proj -c project=newloc -t datumtrans=1
```

Create a new project with the coordinate system from a WKT definition
stored in a text file:  

```sh
g.proj -c wkt=irish_grid.prj project=irish_grid
```

Create a new project from a PROJ description, explicitly specifying a
datum and using the default datum transformation parameters:  

```sh
g.proj -c project=spain proj4="+proj=utm +zone=30 +ellps=intl" datum=eur50 datumtrans=0
```

### Using g.proj output for GDAL/OGR tools

Reproject external raster map to current GRASS project (does not always
make sense!) using the GDAL 'gdalwarp' tool. We recommend to use the
ERDAS/Img format and not to use the ESRI style of WKT:  

```sh
# example for 30x30 pixel resolution (enforce with -tr to avoid odd values)
gdalwarp -of HFA -tr 30 30 -t_srs "`g.proj -wf`" aster.img aster_tmerc.img
```

Reproject external vector map to current GRASS project using the OGR
'ogr2ogr' tool:  

```sh
ogr2ogr -t_srs "`g.proj -wf`" polbnda_italy_GB_ovest.shp polbnda_italy_LL.shp
```

### Using g.proj JSON output with pandas

Using the CRS information for the current project in JSON format with pandas:

```python
import grass.script as gs
import pandas as pd

# Run g.proj to get CRS information in JSON format.
proj_data = gs.parse_command("g.proj", flags="p", format="json")

df = pd.DataFrame.from_dict(proj_data, orient='index')
print(df)
```

```sh
                               0
name     Lambert Conformal Conic
proj                         lcc
datum                      nad83
a                      6378137.0
es          0.006694380022900787
lat_1          36.16666666666666
lat_2          34.33333333333334
lat_0                      33.75
lon_0                        -79
x_0                    609601.22
y_0                            0
no_defs                  defined
unit                       Meter
units                     Meters
meters                         1
```

## REFERENCES

[PROJ](https://proj.org): Projection/datum support library  
[GDAL raster library and toolset](https://gdal.org)  
[OGR vector library and toolset](https://gdal.org/)

Further reading:

- [ASPRS Grids and
  Datum](https://www.asprs.org/asprs-publications/grids-and-datums)
- [MapRef - The Collection of Map Projections and Reference Systems for
  Europe](https://mapref.org)
- [Projections Transform List](http://geotiff.maptools.org/proj_list/)
  (PROJ)

## SEE ALSO

*[m.proj](m.proj.md), [r.proj](r.proj.md), [v.proj](v.proj.md),
[r.import](r.import.md), [r.in.gdal](r.in.gdal.md),
[v.import](v.import.md), [v.in.ogr](v.in.ogr.md)*

## AUTHOR

Paul Kelly

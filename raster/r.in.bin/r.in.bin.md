## DESCRIPTION

*r.in.bin* allows the user to create a (binary) GRASS raster map layer
from a variety of binary raster data formats.

The **-s** flag is used for importing two's-complement signed data.

The **-h** flag is used to read region information from a Generic
Mapping Tools (GMT) type binary header. It is compatible with GMT binary
grid types 1 and 2.

The north, south, east, and west field values are the coordinates of the
edges of the geographic region. The rows and cols values describe the
dimensions of the matrix of data to follow. If the input is a
[GMT](https://www.generic-mapping-tools.org/) binary array (-h flag),
the six dimension fields (north, south, east, west, rows and cols) are
obtained from the GMT header. If the bytes field is entered incorrectly
an error will be generated suggesting a closer bytes value.

*r.in.bin* can be used to import numerous binary arrays including:
ETOPO30, ETOPO-5, ETOPO-2, Globe DEM, BIL, AVHRR and GMT binary arrays
(ID 1 & 2).

## NOTES

If optional parameters are not supplied, **r.in.bin** attempts to
calculate them. For example if the rows and columns parameters are not
entered, **r.in.bin** automatically calculates them by subtracting south
from north and west from east. This will only produce correct results if
the raster resolution equals 1. Also, if the north, south, east, and
west parameters are not entered, **r.in.bin** assigns them from the rows
and columns parameters. In the AVHRR example (see below), the raster
would be assigned a north=128, south=0, east=128, west=0.

The geographic coordinates north, south, east, and west describe the
outer edges of the geographic region. They run along the edges of the
cells at the edge of the geographic region and *not* through the center
of the cells at the edges.

Eastern limit of geographic region (in projected coordinates must be
east of the west parameter value, but in geographical coordinates will
wrap around the globe; user errors can be detected by comparing the
*ewres* and *nsres* values of the imported map layer carefully).  
Western limit of geographic region (in projected coordinates must be
west of the east parameter value, but in geographical coordinates will
wrap around the globe; user errors can be detected by comparing the
*ewres* and *nsres* values of the imported map layer carefully).

Notes on (non)signed data:

If you use the **-s** flag, the highest bit is the sign bit. If this is
1, the data is negative, and the data interval is half of the unsigned
(not exactly).

This flag is only used if **bytes=** 1. If **bytes** is greater than 1,
the flag is ignored.

## EXAMPLES

### GTOPO30 DEM

The following is a sample call of *r.in.bin* to import [GTOPO30
DEM](http://edcdaac.usgs.gov/gtopo30/gtopo30.asp) data:

```sh
r.in.bin -sb input=E020N90.DEM output=gtopo30 bytes=2 north=90 south=40
east=60 west=20 r=6000 c=4800
```

(you can add "anull=-9999" if you want sea level to have a NULL value)

### GMT

The following is a sample call of *r.in.bin* to import a GMT type 1
(float) binary array:

```sh
r.in.bin -hf input=sample.grd output=sample.grass
```

(-b could be used to swap bytes if required)

### AVHRR

The following is a sample call of *r.in.bin* to import an AVHRR image:

```sh
r.in.bin in=p07_b6.dat out=avhrr c=128 r=128
```

### ETOPO2

The following is a sample call of *r.in.bin* to import [ETOPO2
DEM](http://www.ngdc.noaa.gov/mgg/image/2minrelief.html) data (here full
data set):

```sh
r.in.bin ETOPO2.dos.bin out=ETOPO2min r=5400 c=10800 n=90 s=-90 w=-180 e=180 bytes=2
r.colors ETOPO2min rules=terrain
```

### TOPEX/SRTM30 PLUS

The following is a sample call of *r.in.bin* to import [SRTM30
PLUS](http://topex.ucsd.edu/WWW_html/srtm30_plus.html) data:

```sh
r.in.bin -sb input=e020n40.Bathymetry.srtm output=e020n40_topex \
         bytes=2 north=40 south=-10 east=60 west=20 r=6000 c=4800
r.colors e020n40_topex rules=etopo2
```

### GPCP

The following is a sample call of *r.in.bin* to import GPCP 1DD v1.2
data:

```sh
YEAR="2000"
MONTH="01"
# number of days of this month
MDAYS=`date -d"${YEAR}-${MONTH}-01 + 1 month - 1 day" +%d`
r.in.bin in=gpcp_1dd_v1.2_p1d.${YEAR}${MONTH} out=gpcp_${YEAR}.${MONTH}. \
         order=big bytes=4 -f header=1440 anull=-99999 \
         n=90 s=-90 w=0 e=360 rows=180 cols=360 bands=$MDAYS
```

The following is a sample call of *r.in.bin* to import GPCP v2.2 data:

```sh
r.in.bin in=gpcp_v2.2_psg.1979 out=gpcp_1979. \
         order=big bytes=4 -f header=576 anull=-99999 \
         n=90 s=-90 w=0 e=360 rows=72 cols=144 bands=12
```

## SEE ALSO

*[r.import](r.import.md), [r.out.bin](r.out.bin.md),
[r.in.ascii](r.in.ascii.md), [r.out.ascii](r.out.ascii.md),
[r.in.gdal](r.in.gdal.md), [r.out.gdal](r.out.gdal.md),
[r.in.srtm](r.in.srtm.md)*

## AUTHORS

Jacques Bouchard, France (<bouchard@onera.fr>)  
Bob Covill, Canada (<bcovill@tekmap.ns.ca>)  
Markus Metz  
Man page: Zsolt Felker (<felker@c160.pki.matav.hu>)

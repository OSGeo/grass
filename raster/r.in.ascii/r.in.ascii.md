## DESCRIPTION

*r.in.ascii* allows a user to create a (binary) GRASS raster map layer
from an ASCII raster input file with (optional) TITLE.

The GRASS ASCII **input** file has a header section which describes the
location and size of the data, followed by the data itself.

The header has 6 lines:

```sh
north:   xxxxxx.xx
south:   xxxxxx.xx
east:    xxxxxx.xx
west:    xxxxxx.xx
rows:    r
cols:    c
```

The north, south, east, and west field values entered are the
coordinates of the edges of the geographic region. The rows and cols
field values entered describe the dimensions of the matrix of data to
follow. The data which follows is *r* rows of *c* integers.

Optionally the following parameters can be defined in the header
section:

```sh
null: nn
type: float
multiplier: 2.
```

"null" defines a string or number to be converted to NULL value (no
data).  
"type" defines the data type (int, float double) and is not required.  
"multiplier" is an optional parameter to multiply each cell value.

## NOTES

The geographic coordinates north, south, east, and west describe the
outer edges of the geographic region. They run along the edges of the
cells at the edge of the geographic region and *not* through the center
of the cells at the edges. The NW value occurs at the beginning of the
first line of data, and the SW value occurs at the beginning of the last
line of data.

The data (which follows the header section) must contain `r` *x* `c`
values, but it is not necessary that all the data for a row be on one
line. A row may be split over many lines.

The imported cell type can be forced using the **type** option, default
is auto-detection.

The header information in ESRI Raster ASCII files differs from GRASS. To
convert an Arc/Info (ArcView) ASCII grid file into GRASS, see
*[r.in.gdal](r.in.gdal.md)*.

SURFER (Golden Software) ASCII files may be imported by passing the
**-s** flag.

## EXAMPLE

The following is a sample **input** file to *r.in.ascii*:

```sh
north:                   4299000.00
south:                   4247000.00
east:                     528000.00
west:                     500000.00
rows:                         10
cols:                         15
null:                      -9999

1 2 3 4 5 6 7 8 9 10 11 12 13 14 15
1 2 3 4 5 6 7 8 9 10 11 12 13 14 15
1 2 3 4 5 6 7 8 9 10 11 12 13 14 15
1 2 3 4 5 6 7 8 9 10 11 12 13 14 15
1 2 3 4 5 6 7 8 9 10 11 12 13 14 15
1 2 3 4 5 6 7 8 9 10 11 12 13 14 15
1 2 3 4 5 6 7 8 9 10 11 12 13 14 15
1 2 3 4 5 6 7 8 9 10 11 12 13 14 15
1 2 3 4 5 6 7 8 9 10 11 12 13 14 15
1 2 3 4 5 6 7 8 9 10 11 12 13 14 15
```

## SEE ALSO

*[r.import](r.import.md), [r.out.ascii](r.out.ascii.md),
[r.in.gdal](r.in.gdal.md), [r.out.gdal](r.out.gdal.md),
[r.in.bin](r.in.bin.md), [r3.in.ascii](r3.in.ascii.md)*

## AUTHORS

Michael Shapiro, U.S. Army Construction Engineering Research
Laboratory  
Surfer support by Roger Miller

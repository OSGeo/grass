## DESCRIPTION

*v.in.ascii* converts a vector map in [GRASS ASCII vector
format](vectorascii.md) to a vector map in binary format. The module may
import two formats:

- **standard** contains all data types, each coordinate on one row
- **point** (default) reads only points, each point defined on one row.
  Values are separated by a user-definable delimiter. If the **columns**
  option is not defined, default names are used. It is possible to
  specify the column order for the x,y,z coordinates and category
  values.

*[v.out.ascii](v.out.ascii.md)* performs the function of *v.in.ascii* in
reverse; i.e., it converts vector maps in binary format to GRASS ASCII
vector format. These two companion programs are useful both for
importing and exporting vector maps between GRASS and other software,
and for transferring data between machines.

## NOTES

The input is read from the file specified by the **input** option or
from standard input.

The field separator may be a character, the word '`tab`' (or '`\t`') for
tab, '`space`' (or ' ') for a blank, or '`comma`' (or ',') for a comma.

An attribute table is only created if it is needed, i.e. when at least
one attribute column is present in the input file besides geometry
columns. The attribute column will be auto-scanned for type, but may be
explicitly declared along with the geometry columns using the
**columns** parameter.

Use the **-z** flag to convert ASCII data into a 3D vector map.

In special cases of data import, such as the import of large LIDAR
datasets (millions of data points), it may be necessary to disable
topology support (vector level 1) due to memory constraints. This is
done with the **-b** flag. As only very few vector modules support
points data processing at vector level 1, usually topology is required
(vector level 2). Therefore it is recommended that the user first try to
import the data without creating a database (the **-t** flag) or within
a subregion (the **-r** flag) before resorting to the disabling of
topology.

If old version is requested, the **output** files from
*[v.out.ascii](v.out.ascii.md)* is placed in the
`$LOCATION/$MAPSET/dig_ascii/` and `$LOCATION/$MAPSET/dig_att`
directory.

### Import of files without category column

If the input file does not contain a category column, there is the
possibility to auto-generate these IDs (categories). To automatically
add an additional column named 'cat', the **cat** parameter must be set
to the virtual column number 0 (`cat=0`). This is the default action if
the **cat** parameter is not set.

### Importing from a spreadsheet

Data may be imported from many spreadsheet programs by saving the
spreadsheet as a comma separated variable (.csv) text file, and then
using the **separator=','** or **separator=comma** option with
*v.in.ascii* in **points** mode. If the input file contains any header
lines, such as column headings, the **skip** parameter should be used.
These skipped header lines will be written to the map's history file for
later reference (read with `v.info -h`). The skip option only works in
**points** mode.

Any line starting with the hash character ('`#`') will be treated as a
comment and skipped completely if located in the main data file. If
located in the header, as defined by the **skip** parameter, it will be
treated as a header line and written to the history file.

### Import of sexagesimal degree (degree, minutes, seconds, DMS)

The import of DMS formatted degrees is supported (in this case no sign
but N/S, E/W characters are used to indicate the hemispheres). While the
positions are internally translated into decimal degrees during the
import, the original DMS values are maintained in the attribute table.
This requires both the latitude and the longitude columns to be defined
as `varchar()`, not as numbers. A warning will be issued which can be
ignored. See [GRASS ASCII vector format specification](vectorascii.md)
for details.

### Importing only selected columns

Although *v.in.ascii* doesn't have an option to specify which columns
should be imported, you can use a shell filter to achieve the same
effect, e.g.:

```sh
# Print out the column number for each field, supposing the file has a header
head -1 input_file | tr '<the_field_separator_character>' '\n' | cat -n
# From the listing, select the columns you want and feed them to v.in.ascii
# use input=- to read from stdin
cut -d<the_field_separator_character> -f<comma-separated_list_of_columns> input_file | v.in.ascii in=- <your_options>
```

## EXAMPLES

### Example 1a) - standard format mode

Sample ASCII polygon vector map for 'standard' format mode. The two
areas will be assigned categories 20 and 21. For details on the
structure of standard format data files see the second reference at the
bottom of this page.

```sh
echo "ORGANIZATION: GRASS Development Team
DIGIT DATE:   1/9/2005
DIGIT NAME:   -
MAP NAME:     test
MAP DATE:     2005
MAP SCALE:    10000
OTHER INFO:   Test polygons
ZONE:  0
MAP THRESH:   0.500000
VERTI:
B  6
 5958812.48844435 3400828.84221011
 5958957.29887089 3400877.11235229
 5959021.65906046 3400930.7458436
 5959048.47580612 3400973.65263665
 5959069.92920264 3401032.64947709
 5958812.48844435 3400828.84221011
C  1 1
 5958952.42189184 3400918.23126419
 1 20
B  4
 5959010.9323622 3401338.36037757
 5959096.7459483 3401370.54047235
 5959091.38259917 3401450.99070932
 5959010.9323622 3401338.36037757
C  1 1
 5959063.08352122 3401386.98533277
 1 21" | v.in.ascii in=- format=standard output=test_polygons
```

### Example 1b) - standard format mode

Sample ASCII 3D line vector map for 'standard' format mode with
simplified input (note the space field separator). Note the **-z** flag
indicating 3D vector input, and the **-n** flag indicating no vector
header should be expected from the input file. The first line in this
example specifies that the line (L) has 5 vertices and 1 category. The
last line specifies the layer (1) and the category value (321).

```sh
echo "L 5 1
591336 4927369 1224
594317 4925341 1292
599356 4925162 1469
602396 4926653 1235
607524 4925431 1216
1 321 " | v.in.ascii -zn in=- out=line3d format=standard
```

This can be used to create a vector line of a GPS track: the GPS points
have to be stored into a file with a preceding 'L' and the number of
points (per line).

### Example 2 - point format mode

Generate a 2D points vector map 'coords.txt' as ASCII file:

```sh
1664619|5103481
1664473|5095782
1664273|5101919
1663427|5105234
1663709|5102614
```

Import into GRASS:

```sh
v.in.ascii input=coords.txt output=mymap
```

As the **cat** option is set to 0 by default, an extra column 'cat'
containing the category numbers will be auto-generated.

### Example 3 - point format mode

Generate a 2D points vector map 'points.dat' as ASCII file:

```sh
1|1664619|5103481|studna
2|1664473|5095782|kadibudka
3|1664273|5101919|hruska
4|1663427|5105234|mysi dira
5|1663709|5102614|mineralni pramen
```

Import into GRASS:

```sh
cat points.dat | v.in.ascii in=- out=mypoints x=2 y=3 cat=1 \
    columns='cat int, x double precision, y double precision, label varchar(20)'
```

The module is reading from standard input, using the default '\|' (pipe)
delimiter.

### Example 4 - point format mode - CSV table

Import of a 3D points CSV table ('points3d.csv') with attributes:

```sh
"num","X","Y","Z","T"
1,2487491.643,5112118.33,120.5,18.62
2,2481985.459,5109162.78,123.9,18.46
3,2478284.289,5105331.04,98.3,19.61
```

Import into GRASS:

```sh
# import: skipping the header line, categories generated automatically,
# column names defined with type:
v.in.ascii -z in=points3d.csv out=mypoints3D separator=comma \
  columns="num integer, x double precision, y double precision, z double precision, temp double precision" \
  x=2 y=3 z=4 skip=1
# verify column types
v.info -c mypoints3D
# verify table content
v.db.select mypoints3D
```

### Example 5 - point format mode

Generating a 3D points vector map from DBMS (idcol must be an integer
column):  

```sh
echo "select east,north,elev,idcol from mytable" | db.select -c | v.in.ascii in=- -z out=mymap
```

With **in=-**, the module is reading from standard input, using the
default '\|' (pipe) delimiter.  
The import works for 2D maps as well (no elev column and no '-z' flag).

### Example 6 - point format mode

Generate a 3D points vector map 'points3d.dat' with attributes as ASCII
file:

```sh
593493.1|4914730.2|123.1|studna|well
591950.2|4923000.5|222.3|kadibudka|outhouse
589860.5|4922000.0|232.3|hruska|pear
590400.5|4922820.8|143.2|mysi dira|mouse hole
593549.3|4925500.7|442.6|mineralni pramen|mineral spring
600375.7|4925235.6|342.2|kozi stezka|goat path
```

Import into GRASS:

```sh
#As the 'cat' option is set to 0 by default, an extra column 'cat'
#containing the IDs will be auto-generated (no need to define that):
cat points3d.dat | v.in.ascii in=- -z z=3 cat=0 out=mypoints3D \
    columns='x double precision, y double precision, z double precision, \
    label_cz varchar(20), label_en varchar(20)'
v.info -c mypoints3D
v.info mypoints3D
```

### Example 7 - point format mode

Generate points file by clicking onto the map:

```sh
#For LatLong projects:
d.where -d -l | awk '{printf "%f|%f|point\n", $1, $2}' | v.in.ascii in=- out=points \
    columns='x double precision, y double precision, label varchar(20)'

#For other projections:
d.where | awk '{printf "%f|%f|point\n", $1, $2}' | v.in.ascii in=- out=points \
    columns='x double precision, y double precision, label varchar(20)'
```

The 'point' string (or some similar entry) is required to generate a
database table. When simply piping the coordinates (and optionally
height) without additional column(s) into *v.in.ascii*, only the vector
map geometry will be generated.

### Example 8 - point format mode

Convert ground control points into vector points:

```sh
cat $MAPSET/group/$GROUP/POINTS | v.in.ascii in=- out=$GROUP_gcp separator=space skip=3 \
    col='x double precision, y double precision, x_target double precision, \
    y_target double precision, ok int'
```

## REFERENCES

[SQL command notes](sql.md) for creating databases,  
[GRASS ASCII vector format](vectorascii.md) specification

## SEE ALSO

*[db.execute](db.execute.md), [r.in.ascii](r.in.ascii.md),
[r.in.poly](r.in.poly.md), [r.in.xyz](r.in.xyz.md),
[v.build](v.build.md), [v.build.polylines](v.build.polylines.md),
[v.centroids](v.centroids.md), [v.clean](v.clean.md),
[v.db.connect](v.db.connect.md), [v.import](v.import.md),
[v.info](v.info.md), [v.in.lines](v.in.lines.md),
[v.in.mapgen](v.in.mapgen.md), [v.out.ascii](v.out.ascii.md)*

## AUTHORS

Michael Higgins, U.S.Army Construction Engineering Research Laboratory  
James Westervelt, U.S.Army Construction Engineering Research
Laboratory  
Radim Blazek, ITC-Irst, Trento, Italy

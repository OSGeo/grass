## DESCRIPTION

*r.what* outputs the category values and (optionally) the category
labels associated with user-specified locations on raster input map(s).
Locations are specified as geographic x,y coordinate pairs (i.e., pair
of eastings and northings); the user can also (optionally) associate a
label with each location.

The input coordinates can be entered directly on the command line via
**coordinates** parameter, or redirected via `stdin` from an input text
file, script, or piped from another program (like
*[v.out.ascii](v.out.ascii.md)*). Coordinates can be given also as a
vector points map (**points**).

If none of the above input methods are used and the module is run from
the terminal prompt, the program will interactively query the user for
point locations and labels.

Each line of the input consists of an easting, a northing, and an
optional label, which are separated by spaces. In interactive mode, the
word "`end`" must be typed after the last pair of input coordinates.

*r.what* output consists of the input geographic location and label,
and, for each user-named raster map layer, the category value, and (if
the **-f** label flag is specified) the category label associated with
the cell(s) at this geographic location.

## EXAMPLES

### Input coordinates given as an option

The module's **coordinates** parameter can be used to enter coordinate
pairs directly. The maximum number of pairs will be limited by your
system's maximum input line length (e.g. 4096 characters).

```sh
g.region raster=landuse96_28m,aspect -p
r.what map=landuse96_28m,aspect coordinates=633614.08,224125.12,632972.36,225382.87 -f

633614.08|224125.12||2|Low Intensity Developed|209.5939|209 degrees ccw from east
632972.36|225382.87||15|Southern Yellow Pine|140.7571|140 degrees ccw from east
```

### Input coordinates given as a vector points map

Coordinates can be read from existing vector points map by specifying
**points** option. Other features than points or centroids are ignored.
Example: query North Carolina county number for each community college:

```sh
g.region raster=boundary_county_500m -p
r.what map=boundary_county_500m points=comm_colleges

145096.859150|154534.264884||39
616341.437150|146049.750884||51
...
```

### Input coordinates given as a vector points map with cats

Coordinates can be read from existing vector points map by specifying
**points** option. Other features than points or centroids are ignored.
Using the **v** flag you can get also the cat for each feature. Example:
query North Carolina county number for each community college:

```sh
g.region raster=boundary_county_500m -p
r.what map=boundary_county_500m points=comm_colleges -v

1|145096.859150|154534.264884||39
2|616341.437150|146049.750884||51
...
```

### Input coordinates given as a vector points map, output into CSV file

Coordinates can be read from existing vector points map by specifying
**points** option. Other features than points or centroids are ignored.
The output is stored in a CSV file including header row. Example: query
North Carolina county number for each community college:

```sh
g.region raster=boundary_county_500m -p
r.what map=boundary_county_500m points=comm_colleges \
       separator=comma output=result.csv -n

cat result.csv
easting,northing,site_name,boundary_county_500m
145096.859150,154534.264884,,39
616341.437150,146049.750884,,51
410595.719150,174301.828884,,71
...
```

### Input from a text file containing coordinates

The contents of an ASCII text file can be redirected to *r.what* as
follows. If we have a file called *input_coord.txt* containing the
whitespace separated coordinates and optionally labels, the resulting
raster map values are extracted:

```sh
cat input_coord.txt
633614.08 224125.12 site 1
632972.36 225382.87 site 2

r.what map=landuse96_28m,aspect < input_coord.txt

633614.08|224125.12|site 1|2|209.5939
632972.36|225382.87|site 2|15|140.7571
```

### Input from standard input on the command line

Input coordinates may be given directly from standard input (`stdin`),
for example (input data appears between the "`EOF`" markers):

```sh
r.what map=landuse96_28m,aspect << EOF
633614.08 224125.12 site 1
632972.36 225382.87 site 2
EOF

633614.08|224125.12|site 1|2|209.5939
632972.36|225382.87|site 2|15|140.7571
```

```sh
echo "633614.08 224125.12" | r.what map=landuse96_28m,aspect

633614.08|224125.12||2|209.5939
```

### Input coordinates piped from another program

The input coordinates may be "piped" from the standard output (`stdout`)
of another program. In the next example, vector point coordinates are
piped from the *[v.out.ascii](v.out.ascii.md)* module.

```sh
v.out.ascii comm_colleges separator=space | r.what map=boundary_county_500m

145096.8591495|154534.26488388|1|39
616341.4371495|146049.75088388|2|51
410595.7191495|174301.82888388|3|71
...
```

### Output containing raster map category labels

Here we use the **-f** label flag to enable the output of category
labels associated with the raster cell(s), as well as values
(categorical maps only).

```sh
r.what -f map=landuse96_28m,aspect << EOF
633614.08 224125.12 site 1
632972.36 225382.87 site 2
EOF

633614.08|224125.12|site 1|2|Low Intensity Developed|209.5939|209 degrees ccw from east
632972.36|225382.87|site 2|15|Southern Yellow Pine|140.7571|140 degrees ccw from east
```

## NOTE

The maximum number of raster map layers that can be queried at one time
is 400.

## TODO

- Fix **400 maps** limit

## SEE ALSO

*[r.category](r.category.md), [r.report](r.report.md),
[r.stats](r.stats.md), [r.series](r.series.md), [r.univar](r.univar.md),
[v.what](v.what.md), [v.what.rast](v.what.rast.md),
[v.what.vect](v.what.vect.md)*

## AUTHORS

Michael Shapiro, U.S. Army Construction Engineering Research
Laboratory  
Vector point input added by Martin Landa, Czech Technical University in
Prague, Czech Republic

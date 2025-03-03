## DESCRIPTION

*r.category* prints the category values and labels for the raster map
layer specified by **map**=*name* to standard output. You can also use
it to set category labels for a raster map.

The user can specify all needed parameters on the command line, and run
the program non-interactively. If the user does not specify any
categories (e.g., using the optional **cats**=*range*\[,*range*,...\]
argument), then all the category values and labels for the named raster
map layer that occur in the map are printed. The entire *map* is read
using *[r.describe](r.describe.md)*, to determine which categories occur
in the *map*. If a listing of categories is specified, then the labels
for those categories only are printed. The *cats* may be specified as
single category values, or as ranges of values. The user may also
(optionally) specify that a field separator other than a space or tab be
used to separate the category value from its corresponding category
label in the output, by using the
**separator**=*character*\|*space*\|*tab* option (see example below). If
no field separator is specified by the user, a tab is used to separate
these fields in the output, by default.

The output is sent to standard output in the form of one category per
line, with the category value first on the line, then an ASCII TAB
character (or whatever single character or space is specified using the
**separator** parameter), then the label for the category.

## NOTES

Any ASCII TAB characters which may be in the label are replaced by
spaces.

The output from *r.category* can be redirected into a file, or piped
into another program.

### Input from a file

The **rules** option allows the user to assign category labels from
values found in a file (without header). The label can refer to a single
category, range of categories, floating point value, or a range of
floating point values. The format is given as follows (when separator is
set to colon; no white space must be used after the separator):

```sh
cat:Label
val1:val2:Label
```

If the filename is given as "-", the category labels are read from
`stdin`

### Default and dynamic category labels

Default and dynamic category labels can be created for categories that
are not explicitly labeled. The coefficient line can be followed by
explicit category labels which override the format label generation.

```sh
   0:no data
   2:   .
   5:   .             ## explicit category labels
   7:   .
```

explicit labels can be also of the form:

```sh
   5.5:5:9 label description
   or
   15:30  label description
```

In the format line

- `$1` refers to the value `num*5.0+1000` (ie, using the first 2
  coefficients)
- `$2` refers to the value `num*5.0+1005` (ie, using the last 2
  coefficients)

`$1.2` will print `$1` with 2 decimal places.

Also, the form `$?xxx$yyy$` translates into `yyy` if the category is 1,
xxx otherwise. The `$yyy$` is optional. Thus

`$1 meter$?s`

will become:  
`1 meter` (for category 1)  
`2 meters` (for category 2), etc.

`format='Elevation: $1.2 to $2.2 feet' ## Format Statement`  
`coefficients="5.0,1000,5.0,1005" ## Coefficients`

The format and coefficients above would be used to generate the
following statement in creation of the format appropriate category
string for category "num":

`sprintf(buff,"Elevation: %.2f to %.2f feet", num*5.0+1000, num*5.0*1005)`

Note: while both the format and coefficient lines must be present a
blank line for the format string will effectively suppress automatic
label generation.

To use a "`$`" in the label without triggering the plural test, put
"`$$`" in the format string.

Use 'single quotes' when using a "`$`" on the command line to avoid
unwanted shell substitution.

## EXAMPLES

North Carolina sample dataset:

### Printing categories

```sh
r.category map=landclass96
1   developed
2   agriculture
3   herbaceous
4   shrubland
5   forest
6   water
7   sediment
```

prints the values and labels associated with all of the categories in
the *landclass96* raster map layer.

```sh
r.category map=landclass96 cats=2,5-7
2   agriculture
5   forest
6   water
7   sediment
```

prints only the category values and labels for *landclass96* map layer
categories `2` and `5` through `7`.

```sh
r.category map=landclass96 cats=3,4 separator=comma
3,herbaceous
4,shrubland
```

prints the values and labels for *landclass96* map layer categories `3`
and `4`, but uses "`,`" (instead of a tab) as the character separating
the category values from the category values in the output.

```sh
r.category map=landclass96 cats=3,4 output_format=json
```

generates the following JSON output:

```json
[
    {
        "category": 3,
        "description": "herbaceous"
    },
    {
        "category": 4,
        "description": "shrubland"
    }
]
```

### Adding categories

Example for defining new category labels, using a colon as separator:

```sh
r.category diseasemap separator=":" rules=- << EOF
1:potential absence
2:potential presence
EOF
```

This sets the category values 1 and 2 to respective text labels.
Alternatively, the rules can be stored in an ASCII text file and loaded
via the *rules* parameter.

## SEE ALSO

UNIX Manual entries for *awk* and *sort*

*[d.what.rast](d.what.rast.md), [r.coin](r.coin.md),
[r.describe](r.describe.md), [r.support](r.support.md)*

## AUTHORS

Michael Shapiro, U.S. Army Construction Engineering Research
Laboratory  
Hamish Bowman, University of Otago, New Zealand (label creation options)

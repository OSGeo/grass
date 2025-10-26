## DESCRIPTION

*r.reclass* creates an *output* map layer based on an *input* integer
raster map layer. The output map layer will be a reclassification of the
input map layer based on reclass rules input to *r.reclass*, and can be
treated in much the same way that raster maps are treated. A *TITLE* for
the output map layer may be (optionally) specified by the user.

The reclass rules are read from standard input (i.e., from the keyboard,
redirected from a file, or piped through another program).

Before using *r.reclass* the user must know the following:

1. The new categories desired; and, which old categories fit into which
    new categories.
2. The names of the new categories.

## NOTES

In fact, the *r.reclass* program does *not* generate any new raster map
layers (in the interests of disk space conservation). Instead, a
**reclass table** is stored which will be used to reclassify the
original raster map layer each time the new (reclassed) map name is
requested. As far as the user (and programmer) is concerned, that raster
map has been created.

*r.reclass* only works on an *integer* input raster map; if the input
map is instead floating point data, you must multiply the input data by
some factor to achieve whole number input data, otherwise *r.reclass*
will round the raster values down to the next integer.

Also note that although the user can generate a *r.reclass* map which is
based on another *r.reclass* map, the new *r.reclass* map will be stored
in GRASS as a reclass of the *original* raster map on which the first
reclassed map was based. Therefore, while GRASS allows the user to
provide *r.reclass* map layer information which is based on an already
reclassified map (for the user's convenience), no *r.reclass* map layer
(i.e., *reclass table*) will ever be *stored* as a *r.reclass* of a
*r.reclass*.

To convert a reclass map to a regular raster map layer, set your
geographic region settings to match the settings in the header for the
reclass map (with "`g.region raster=reclass_map`", or viewable by
running *[r.info](r.info.md)*) and then run
*[r.resample](r.resample.md)*.

*[r.mapcalc](r.mapcalc.md)* can be used to convert a reclass map to a
regular raster map layer as well:

```sh
  r.mapcalc "raster_map = reclass_map"
```

where *raster_map* is the name to be given to the new raster map, and
*reclass_map* is an existing reclass map.

Because *r.reclass* generates internally simply a table by referencing
some original raster map layer rather than creating a full new reclassed
raster map layer, a *r.reclass* map layer will no longer be accessible
if the original raster map layer, upon which it was based, is later
removed. Therefore, attempting to remove a raster map layer from which a
*r.reclass* has been derived is only possible if the original map is
removed first. Alternatively, a *r.reclass* map can be removed including
its base map by using

*[g.remove](g.remove.md)*'s **-b** flag.

A *r.reclass* map is not a true raster map layer. Rather, it is a table
of reclassification values which reference the input raster map layer.
Therefore, users who wish to retain reclassified map layers must also
save the original input raster map layers from which they were
generated. Alternatively *r.recode* can be used.

Category values which are not explicitly reclassified to a new value by
the user will be reclassified to NULL.

### Reclass Rules

Each line of input must have the following format:

**input_categories**=*output_category* \[*label*\]

where each line of input specifies the category values in the input
raster map layer to be reclassified to the new *output_category*
category value. Specification of a *label* to be associated with the new
output map layer category is optional. If specified, it is recorded as
the category label for the new category value. The equal sign = is
required. The *input_category(ies)* may consist of single category
values or a range of such values in the format "*low* thru *high*." The
word "thru" must be present.

To include all (remaining) values the asterix "\*" can be used. This
rule has to be set as last rule. No further rules are accepted after
setting this rule. The special rule "`* = *`" specifies that all
categories not explicitly set by one of the above rules should be passed
through unaltered instead of being set to NULL.

Categories to become no data are specified by setting the output
category value to "`NULL`".

A line containing only the word **end** terminates the input.

## EXAMPLES

### Reclass rules examples

The following examples may help clarify the reclass rules.

The first example reclassifies categories 1, 2 and 3 in the input raster
map layer "roads" to category 1 with category label "good quality" in
the output map layer, and reclassifies input raster map layer categories
4 and 5 to category 2 with the label "poor quality" in the output map
layer.

```sh
    1 2 3   = 1    good quality
    4 5     = 2    poor quality
```

The following example reclassifies categories 1, 3 and 5 in the input
raster map layer to category 1 with category label "poor quality" in the
output map layer, and reclassifies input raster map layer categories 2,
4, and 6 to category 2 with the label "good quality" in the output map
layer. All other values are reclassified to NULL.

```sh
    1 3 5   = 1    poor quality
    2 4 6   = 2    good quality
    *       = NULL
```

The following example reclassifies input raster map layer categories 1
thru 10 to output map layer category 1, input map layer categories 11
thru 20 to output map layer category 2, and input map layer categories
21 thru 30 to output map layer category 3, all without labels. The range
from 30 to 40 is reclassified as NULL.

```sh
     1 thru 10  = 1
    11 thru 20  = 2
    21 thru 30  = 3
    30 thru 40  = NULL
```

The following example shows overlapping rules. Subsequent rules override
previous rules. Therefore, the below example reclassifies input raster
map layer categories 1 thru 19 and 51 thru 100 to category 1 in the
output map layer, input raster map layer categories 20 thru 24 and 26
thru 50 to the output map layer category 2, and input raster map layer
category 25 to the output category 3.

```sh
     1 thru 100 = 1    poor quality
    20 thru 50  = 2    medium quality
    25          = 3    good quality
```

The previous example could also have been entered as:

```sh
     1 thru 19  51 thru 100 = 1    poor quality
    20 thru 24  26 thru 50  = 2    medium quality
    25              = 3    good quality
```

or as:

```sh
     1 thru 19   = 1    poor quality
    51 thru 100  = 1
    20 thru 24   = 2
    26 thru 50   = 2    medium quality
    25       = 3    good quality
```

The final example was given to show how the labels are handled. If a new
category value appears in more than one rule (as is the case with new
category values 1 and 2), the last label which was specified becomes the
label for that category. In this case the labels are assigned exactly as
in the two previous examples.

### Usage example

In this example, the 21 classes of the landuse map (North Carolina
sample dataset) are simplified to 7 classes:

```sh
r.category landuse96_28m
0   not classified
1   High Intensity Developed
2   Low Intensity Developed
3   Cultivated
[...]
20  Water Bodies
21      Unconsolidated Sediment

# use this command or save rules with editor in textfile "landuserecl.txt"
echo "0 = NULL
1 2     = 1 developed
3       = 2 agriculture
4 6     = 3 herbaceous
7 8 9   = 4 shrubland
10 thru 18 = 5 forest
20      = 6 water
21      = 7 sediment" > landuserecl.txt

r.reclass input=landuse96_28m output=landclass96_recl \
  rules=landuserecl.txt \
  title="Simplified landuse classes 1996"

# verify result
r.category landuse96_recl
1   developed
2   agriculture
3   herbaceous
4   shrubland
5   forest
6   water
7   sediment
```

## SEE ALSO

*[r.recode](r.recode.md), [r.resample](r.resample.md),
[r.rescale](r.rescale.md)*

## AUTHORS

James Westervelt,  
Michael Shapiro  
U.S.Army Construction Engineering Research Laboratory

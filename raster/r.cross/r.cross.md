## DESCRIPTION

*r.cross* creates an *output* raster map layer representing all unique
combinations of category values in the raster input layers
(**input**=*name,name,name*, ...). At least two, but not more than 30,
*input* map layers must be specified. The user must also specify a name
to be assigned to the *output* raster map layer created by *r.cross*.

## OPTIONS

With the **-z** flag NULL values are not crossed. This means that if a
NULL value occurs in any input data layer, this combination is ignored,
even if other data layers contain non-NULL data. In the example given
below, use of the **-z** option would cause 3 categories to be generated
instead of 5.

If the **-z** flag is not specified, then map layer combinations in
which some values are NULL will be assigned a unique category value in
the resulting output map.

Category values in the new *output* map layer will be the cross-product
of the category values from these existing *input* map layers.

## EXAMPLE

For example, suppose that, using two raster map layers, the following
combinations occur:

```sh
          map1   map2
          ___________
          NULL    1
          NULL    2
           1      1
           1      2
           2      4
```

*r.cross* would produce a new raster map layer with 5 categories:

```sh
          map1   map2   output
          ____________________
          NULL    1       0
          NULL    2       1
           1      1       2
           1      2       3
           2      4       4
```

Note: The actual category value assigned to a particular combination in
the *result* map layer is dependent on the order in which the
combinations occur in the input map layer data and can be considered
essentially random. The example given here is illustrative only.

## SUPPORT FILES

The category file created for the *output* raster map layer describes
the combinations of input map layer category values which generated each
category. In the above example, the category labels would be:

```sh
          category   category
          value      label
          ______________________________
             0       layer1(0) layer2(1)
             1       layer1(0) layer2(2)
             2       layer1(1) layer2(1)
             3       layer1(1) layer2(2)
             4       layer1(2) layer2(4)
```

A random color table is also generated for the *output* map layer.

## SEE ALSO

*[r.covar](r.covar.md), [r.stats](r.stats.md)*

## AUTHOR

Michael Shapiro, U.S. Army Construction Engineering Research Laboratory

## DESCRIPTION

***r.describe*** prints a terse listing of category values found in a
user-specified raster map layer.

***r.describe*** ignores the current geographic region and mask, and
reads the full extent of the input raster map. This functionality is
useful if the user intends to *reclassify* or *rescale* the data, since
these functions (*[r.reclass](r.reclass.md)* and
*[r.rescale](r.rescale.md)*) also ignore the current *geographic region*
and *mask*.

The ***nv*** parameter sets the string to be used to represent `NULL`
values in the module output; the default is '\*'.

The ***nsteps*** parameter sets the number of quantisation steps to
divide into the input raster map.

## NOTES

If the user selects the **-r** flag, a range of category values found in
the raster map layer will be printed. The range is divided into three
groups: negative, positive, and zero. If negative values occur, the
minimum and maximum negative values will be printed. If positive values
occur, the minimum and maximum positive values will be printed. If zero
occurs, this will be indicated. The range report will generally run
faster than the full list (the default output).

The **-d** flag can be used to force *r.describe* to respect the current
region extents when reporting raster map categories. The default behavior
is to read the full extent of the input raster map.

If the **-1** flag is specified, the output appears with one category
value/range per line.

The **-n** flag suppresses the reporting of `NULL` values.

## EXAMPLES

The following examples are from the North Carolina sample dataset:

### Print the full list of raster map categories

=== "Command line"

    ```sh
    r.describe map=zipcodes
    ```

    Output:

    ```text
    27511 27513 27518 27529 27539 27601 27603-27608 27610
    ```

 === "Python (grass.script)"

    ```python
    import grass.script as gs

    ranges = gs.parse_command("r.describe", map="zipcodes")
    ```

    The JSON output looks like:

    ```json
    {
        "has_nulls": false,
        "ranges": [
            {
                "min": 27511,
                "max": 27511
            },
            {
                "min": 27513,
                "max": 27513
            },
            {
                "min": 27518,
                "max": 27518
            },
            {
                "min": 27529,
                "max": 27529
            },
            {
                "min": 27539,
                "max": 27539
            },
            {
                "min": 27601,
                "max": 27601
            },
            {
                "min": 27603,
                "max": 27608
            },
            {
                "min": 27610,
                "max": 27610
            }
        ]
    }
    ```

### Print the raster range only

=== "Command line"

    ```sh
    r.describe map=zipcodes -r
    ```

    Output:

    ```text
    27511 thru 27610
    ```

 === "Python (grass.script)"

    ```python
    import grass.script as gs

    gs.parse_command("r.describe", map="zipcodes", flags="r", format="json")
    ```

    The returned dictionary looks like:

    ```text
    {'has_nulls': False, 'ranges': [{'min': 27511, 'max': 27610}]}
    ```

### Print raster map categories, one category per line

=== "Command line"

    ```sh
    r.describe map=zipcodes -1
    ```

    Output:

    ```text
    27511
    27513
    27518
    27529
    27539
    27601
    27603
    27604
    27605
    27606
    27607
    27608
    27610
    ```

 === "Python (grass.script)"

    ```python
    import grass.script as gs

    gs.parse_command("r.describe", map="zipcodes", flags="1", format="json")
    ```

    The returned dictionary looks like:

    ```text
    {'has_nulls': False, 'values': [27511, 27513, 27518, 27529, 27539, 27601, 27603, 27604, 27605, 27606, 27607, 27608, 27610]}
    ```

## SEE ALSO

*[g.region](g.region.md), [r.mask](r.mask.md),
[r.reclass](r.reclass.md), [r.report](r.report.md),
[r.rescale](r.rescale.md), [r.stats](r.stats.md),
[r.univar](r.univar.md)*

## AUTHOR

Michael Shapiro, U.S. Army Construction Engineering Research Laboratory

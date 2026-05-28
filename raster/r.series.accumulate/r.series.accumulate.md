## DESCRIPTION

*r.series.accumulate* calculates (accumulated) raster value using
growing degree days (GDDs)/Winkler indices's, Biologically Effective
Degree Days (BEDD), Huglin heliothermal indices or an average approach
from several input maps for a given day. Accumulation of e.g.
degree-days to growing degree days (GDDs) can be done by providing a
*basemap* with GDDs of the previous day.

The flag **-a** determines the average computation of the input raster
maps. In case the flag is not set, the average calculation is:

```sh
    average = (min + max) / 2
```

In case the flag was set, the calculation changes to arithmetic mean

```sh
    average = sum(input maps) / (number of input maps)
```

**GDD** Growing Degree Days are calculated as

```sh
    gdd = average - lower
```

In case the **-a** is set, the Winkler indices are calculated instead of
GDD, usually accumulated for the period April 1st to October
31st (northern hemisphere) or the period October
1st to April 30t (southern hemisphere).

**BEDDs** Biologically Effective Degree Days are calculated as

```sh
    bedd = average - lower
```

with an optional upper *cutoff* applied to the average instead of the
temperature values.

The **Huglin heliothermal index** is calculated as

```sh
    huglin = (average + max) / 2 - lower
```

usually accumulated for the period April 1st to September
30th (northern hemisphere) or the period September
1st to April 30th (southern hemisphere).

**Mean** raster values are calculated as

```sh
    mean = average
```

For all the formulas *min* is the minimum value, *max* the maximum value
and *average* the average value. The *min*, *max* and *average* values
are automatically calculated from the input maps.

The *shift* and *scale* values are applied directly to the input values.
The *lower* and *upper* maps, as well as the *range* options are applied
to constrain the accumulation. In case the *lower* and *upper* maps are
not provided the *limits* option with default values will be applied.

If an existing map is provided with the *basemap* option, the values of
this map are added to the output.

## NOTES

The *scale* and *shift* parameters are used to transform input values
with

```sh
    new = old * scale + shift
```

With the *-n* flag, any cell for which any of the corresponding input
cells are NULL is automatically set to NULL (NULL propagation) and the
accumulated value is not calculated.

Negative results are set to 0 (zero).

Without the *-n* flag, all non-NULL cells are used for calculation.

If the *range=* option is given, any values which fall outside that
range will be treated as if they were NULL. Note that the range is
applied to the scaled and shifted input data. The *range* parameter can
be set to *low,high* thresholds: values outside of this range are
treated as NULL (i.e., they will be ignored by most aggregates, or will
cause the result to be NULL if -n is given). The *low,high* thresholds
are floating point, so use *-inf* or *inf* for a single threshold (e.g.,
*range=0,inf* to ignore negative values, or *range=-inf,-200.4* to
ignore values above -200.4).

The maximum number of raster maps that can be processed is given by the
user-specific limit of the operating system. For example, the soft
limits for users are typically 1024 files. The soft limit can be changed
with e.g. `ulimit -n 4096` (UNIX-based operating systems) but it cannot
be higher than the hard limit. If the latter is too low, you can as
superuser add an entry in:

```sh
/etc/security/limits.conf
# <domain>      <type>  <item>         <value>
your_username  hard    nofile          4096
```

This will raise the hard limit to 4096 files. Also have a look at the
overall limit of the operating system

```sh
cat /proc/sys/fs/file-max
```

which on modern Linux systems is several 100,000 files.

Use the **-z** flag to analyze large amounts of raster maps without
hitting open files limit and the *file* option to avoid hitting the size
limit of command line arguments. Note that the computation using the
*file* option is slower than with the *input* option. For every single
row in the output map(s) all input maps are opened and closed. The
amount of RAM will rise linearly with the number of specified input
maps. The *input* and *file* options are mutually exclusive: the former
is a comma separated list of raster map names and the latter is a text
file with a new line separated list of raster map names.

## EXAMPLES

Example with MODIS Land Surface Temperature, transforming values from
Kelvin \* 50 to degrees Celsius:

```sh
r.series.accumulate in=MOD11A1.Day,MOD11A1.Night,MYD11A1.Day,MYD11A1.Night out=MCD11A1.GDD \
      scale=0.02 shift=-273.15 limits=10,30
```

## SEE ALSO

*[g.list](g.list.md), [g.region](g.region.md), [r.series](r.series.md),
[r.series.interp](r.series.interp.md)*

[Hints for large raster data
processing](https://grasswiki.osgeo.org/wiki/Large_raster_data_processing)

## REFERENCES

- Jones, G.V., Duff, A.A., Hall, A., Myers, J.W., 2010. Spatial analysis
  of climate in winegrape growing regions in the Western United States.
  Am. J. Enol. Vitic. 61, 313-326.

## AUTHORS

Markus Metz and Soeren Gebbert (based on r.series)

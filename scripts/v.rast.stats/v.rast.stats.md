## DESCRIPTION

*v.rast.stats* calculates basic univariate statistics from a raster map
only for the parts covered by the specified vector map. The vector map
will be rasterized according to the raster map resolution. Then
univariate statistics are calculated per vector category (cat) from the
raster map and the results uploaded to the vector map attribute table. A
new column is generated in the attribute table for each statistic
requested in **method** (if not already present).

The univariate statistics include the number of raster cells counted,
the number of raster NULL cells counted, minimum and maximum cell
values, range, average, standard deviation, variance, coefficient of
variation, sum, first quartile, median, third quartile, and percentile.

## NOTES

*v.rast.stats* is only meaningful for lines or polygons.

The module may take a long time to run if the raster region contains a
large number of cells. In this case the **--verbose** flag may be used
to track progress.

The script stops if a (prefixed) upload column is already present in the
vector map attribute table, unless otherwise instructed with the **-c**
continue flag. The column prefix will be separated from the statistic
name with an underscore. For example with a prefix of "`elev`" the sum
column will be named `elev_sum`.

If a DBF database is being used, note that column names are restricted
by the DBF specification to 10 characters. Therefore it is advised to be
economical in the use of the column prefix when using DBF as any
additional characters will be chopped off.

The script internally aligns the raster grid cells to the grid of the
first input raster map.

If an area has several categories in the selected layer (equivalent to
overlapping polygons in Simple Features), only one category will be kept
during the rasterization process. Statistics for the skipped categories
will thus be partial.

For example, if there are three areas: area 1 with cat 1, area 2 with
cat 2, area 3 with cats 1, 2. Only one category value of area 3 will be
used for rasterization, the other category value will be skipped. Thus
statistics for the used category value will be complete, while
statistics for the skipped category value will be incomplete.

Large amounts of system memory can be used when extended statistics
(*first_quartile,median,third_quartile,percentile* ) are being requested
with a very large region setting. If the region is too large the module
should display memory allocation errors. Basic statistics can be
calculated using any size input region.

## EXAMPLES

Example to upload DEM statistics to ZIP codes vector map (North Carolina
sample dataset):

```sh
g.copy vect=zipcodes_wake,myzipcodes_wake
# set computational region to DEM:
g.region raster=elevation -p
# calculate selected DEM statistics, upload to vector map table:
v.rast.stats myzipcodes_wake raster=elevation \
  column_prefix=elev method=minimum,maximum,average,range,stddev,percentile \
  percentile=95
# verify results:
v.info -c myzipcodes_wake
v.db.select myzipcodes_wake
v.univar myzipcodes_wake column=elev_range type=centroid
```

## SEE ALSO

*[r.univar](r.univar.md), [v.univar](v.univar.md),
[v.vect.stats](v.vect.stats.md), [v.what.rast](v.what.rast.md),
[v.what.rast3](v.what.rast3.md), [v.what.vect](v.what.vect.md)*

## AUTHOR

Markus Neteler, CEA (for the [EDEN EU/FP6
Project](https://cordis.europa.eu/project/id/10284))

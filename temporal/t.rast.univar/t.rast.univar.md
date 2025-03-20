## DESCRIPTION

*t.rast.univar* calculates univariate statistics from the non-null cells
for each registered raster map of a space time raster dataset.

By default it returns the name of the map, the semantic label of the
map, the start and end date of the map and the following values: mean,
minimum and maximum vale, mean_of_abs, standard deviation, variance,
coeff_var, number of null cells, total number of cells.

Using the *e* flag it can calculate also extended statistics: first
quartile, median value, third quartile and percentile 90.

If a *zones* raster map is provided, statistics are computed for each
zone (category) in that input raster map. The *zones* option does not
support space time raster datasets (STRDS) but only a single, static
raster map.

Space time raster datasets may contain raster maps with varying spatial
extent like for example series of scenes of satellite images. With the
*region_relation* option, computations can be limited to maps of the
space time raster dataset that have a given spatial relation to the
current computational region. Supported spatial relations are:

- "overlaps": process only maps that spatially overlap ("intersect")
  with the current computational region
- "is_contained": process only maps that are fully within the current
  computational region
- "contains": process only maps that contain (fully cover) the current
  computational region

## EXAMPLE

Obtain the univariate statistics for the raster space time dataset
"tempmean_monthly" (precision reduced to 2 decimals in this example):

```sh
t.rast.univar -e tempmean_monthly
id|start|end|mean|min|max|mean_of_abs|stddev|variance|coeff_var|sum|null_cells|cells|first_quartile|median|third_quartile|percentile_90
2009_01_tempmean@climate_2009_2012|2009-01-01 00:00:00|2009-02-01 00:00:00|3.90|-3.38|7.43|3.95|1.79|3.20|45.91|1977967.31|503233|1010600|2.80|3.92|5.21|6.23
2009_02_tempmean@climate_2009_2012|2009-02-01 00:00:00|2009-03-01 00:00:00|5.91|-1.82|8.01|5.92|1.63|2.65|27.53|2999555.60|503233|1010600|5.44|6.26|7.07|7.48
...
2012_11_tempmean@climate_2009_2012|2012-11-01 00:00:00|2012-12-01 00:00:00|8.03|1.79|10.91|8.03|1.32|1.73|16.41|4072472.77|503233|1010600|7.49|8.13|8.96|9.48
2012_12_tempmean@climate_2009_2012|2012-12-01 00:00:00|2013-01-01 00:00:00|8.71|1.76|11.98|8.71|1.72|2.95|19.74|4418403.77|503233|1010600|7.84|8.95|9.99|10.67
```

## SEE ALSO

*[t.create](t.create.md), [t.info](t.info.md) [t.create](r.univar.md),*

## AUTHOR

Sören Gebbert, Thünen Institute of Climate-Smart Agriculture  
Stefan Blumentrath (support for zones, parallel processing, and spatial
relations)

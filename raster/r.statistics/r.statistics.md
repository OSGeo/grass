## DESCRIPTION

*r.statistics* is a tool to analyse exploratory statistics of a
categorical "cover layer" according to how it intersects with objects in
a "base layer". A variety of standard statistical measures are possible
(called "zonal statistics" in some GIS). All cells in the base layer are
considered one object for the analysis. For some applications, one will
first want to prepare the input data so that all areas of contiguous
cell category values in the base layer are uniquely identified, which
can be done with *r.clump*.  
The available methods are the following:

- average deviation
- average
- diversity
- kurtosis
- maximum
- median
- minimum
- mode
- skewness
- standard deviation
- sum
- variance

The calculations will be performed on each area of data of the cover
layers which fall within each unique value, or category, of the base
layer.

Setting the *-c* flag the category labels of the covering raster layer
will be used. This is nice to avoid the GRASS limitation to integer in
raster maps because using category values floating point numbers can be
stored.

All calculations create an output layer. The output layer is a
reclassified version of the base layer with identical category values,
but modified category labels - the results of the calculations are
stored in the category labels of the output layer.

## NOTES

For floating-point cover map support, see the alternative
*[r.stats.zonal](r.stats.zonal.md)*. For quantile calculations with
support for floating-point cover maps, see the alternative
*[r.stats.quantile](r.stats.quantile.md)*.

## EXAMPLES

Calculation of average elevation of each field in the Spearfish region:

```sh
r.statistics base=fields cover=elevation.dem out=elevstats method=average
r.category elevstats
r.mapcalc "fieldelev = @elevstats"
r.univar fieldelev
```

## SEE ALSO

*[r.category](r.category.md), [r.clump](r.clump.md),
[r.mode](r.mode.md), [r.mapcalc](r.mapcalc.md),
[r.neighbors](r.neighbors.md), [r.stats.quantile](r.stats.quantile.md),
[r.stats.zonal](r.stats.zonal.md), [r.univar](r.univar.md)*

## AUTHOR

Martin Schroeder, Geographisches Institut Heidelberg, Germany

## DESCRIPTION

*r.quantile* computes quantiles in a manner suitable for use with large
amounts of data. It is using two passes.

## NOTES

Quantiles are calculated following algorithm 7 from Hyndman and Fan
(1996), which is also the default in R and numpy.

## EXAMPLE

Calculation of elevation quantiles (printed to standard-out):

```sh
g.region raster=elevation -p
r.quantile input=elevation percentiles=0.1,1,10,25,50,75,90,99,99.9
```

The output of *r.quantile* can be used for quantile classification:

```sh
g.region raster=elevation -p
r.quantile elevation quantiles=5 -r --quiet | r.recode elevation \
           out=elev_quant5 rules=-
```

## REFERENCES

- Hyndman and Fan (1996) *Sample Quantiles in Statistical Packages*,
  **American Statistician**. American Statistical Association. 50 (4):
  361-365. DOI:
  [10.2307/2684934](https://doi.org/10.2307/2684934%3E10.2307/2684934)
- [*Engineering Statistics Handbook:
  Percentile*](https://www.itl.nist.gov/div898/handbook/prc/section2/prc262.htm),
  NIST

## SEE ALSO

*[r.mode](r.mode.md), [r.quant](r.quant.md), [r.recode](r.recode.md),
[r.series](r.series.md), [r.stats](r.stats.md),
[r.stats.quantile](r.stats.quantile.md),
[r.stats.zonal](r.stats.zonal.md), [r.statistics](r.statistics.md),
[r.univar](r.univar.md), [v.rast.stats](v.rast.stats.md)*

## AUTHORS

Glynn Clements  
Markus Metz

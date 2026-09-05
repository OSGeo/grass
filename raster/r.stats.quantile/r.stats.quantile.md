## DESCRIPTION

*r.stats.quantile* is a tool to analyse exploratory statistics of a
floating-point "cover layer" according to how it intersects with objects
in a "base layer". It provides quantile calculations as selected "zonal
statistics".

## NOTES

*r.stats.quantile* is intended to be a partial replacement for
*[r.statistics](r.statistics.md)*, with support for floating-point cover
maps. It provides quantile calculations, which are absent from
*[r.stats.zonal](r.stats.zonal.md)*.

Quantiles are calculated following algorithm 7 from Hyndman and Fan
(1996), which is also the default in R and numpy.

The **t** flag has been deprecated and replaced by the **format=csv**
option.

The default separator will be **:** to maintain backward compatibility;
however, if **format=csv** is given, then the default separator will be **comma**.

## EXAMPLE

In this example, the raster polygon map `zipcodes` in the North Carolina
sample dataset is used to calculate quantile raster statistics using the
`elevation` raster map:

```sh
g.region raster=zipcodes -p

# print quantiles
r.stats.quantile base=zipcodes cover=elevation quantiles=3 -p
27511:0:33.333333:134.717392
27511:1:66.666667:143.985723
27513:0:33.333333:140.669993
27513:1:66.666667:146.279449
27518:0:33.333333:115.140101
27518:1:66.666667:129.893723
[...]

# write out percentile raster maps
r.stats.quantile base=zipcodes cover=elevation percentiles=25,50,75 \
  output=zipcodes_elev_q25,zipcodes_elev_q50,zipcodes_elev_q75
```

Using the JSON format option and Python to parse the
output:

```python
import grass.script as gs

data = gs.parse_command(
    "r.stats.quantile", base="zipcodes", cover="elevation", flags="p", format="json"
)
print(data[0])
```

Possible output:

```text
{'category': 27511, 'percentiles': [{'percentile': 50, 'value': 139.62598419189453}]}
```

The whole JSON may look like this:

```json
[
 {
  "category": 27511,
  "percentiles": [
   {
    "percentile": 50,
    "value": 139.62598419189453
   }
  ]
 },
 {
  "category": 27513,
  "percentiles": [
   {
    "percentile": 50,
    "value": 143.7049102783203
   }
  ]
 },
 {
  "category": 27518,
  "percentiles": [
   {
    "percentile": 50,
    "value": 122.53437805175781
   }
  ]
 }
]
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

*[r.quantile](r.quantile.md), [r.stats.zonal](r.stats.zonal.md),
[r.statistics](r.statistics.md)*

## AUTHORS

Glynn Clements  
Markus Metz

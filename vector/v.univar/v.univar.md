## DESCRIPTION

*v.univar* calculates univariate statistics on (by default) an attribute
of, or, through the **-d** flag on distance between, vector map
features. Attributes are read per feature and per category value. This
means that if the map contains several features with the same category
value, the attribute is read as many times as there are features. On the
other hand, if a feature has more than one category value, each
attribute value linked to each of the category values of the feature is
read. For statistics on one attribute per category value, instead of one
attribute per feature and per category, see
[v.db.univar](v.db.univar.md).

Extended statistics (**-e**) adds median, 1st and 3rd quartiles, and
90th percentile to the output.

## NOTES

When using the **-d** flag, univariate statistics of distances between
vector features are calculated. The distances from all features to all
other features are used. Since the distance from feature A to feature B
is the same like the distance from feature B to feature A, that distance
is considered only once, i.e. all pairwise distances between features
are used. Depending on the selected vector **type**, distances are
calculated as follows:

- **type=point**: point distances are considered;
- **type=line**: line to line distances are considered;
- **type=area**: not supported, use **type=centroid** instead (and see
  [v.distance](v.distance.md) for calculating distances between areas)

## EXAMPLES

The examples are based on the North Carolina sample dataset.

### Example dataset preparation

```sh
g.region raster=elevation -p
v.random output=samples npoints=100
v.db.addtable map=samples columns="heights double precision"
v.what.rast map=samples rast=elevation column=heights
v.db.select map=samples
```

### Calculate height attribute statistics

```sh
v.univar -e samples column=heights type=point

number of features with non NULL attribute: 100
number of missing attributes: 0
number of NULL attributes: 0
minimum: 57.2799
maximum: 148.903
range: 91.6235
sum: 10825.6
mean: 108.256
mean of absolute values: 108.256
population standard deviation: 20.2572
population variance: 410.356
population coefficient of variation: 0.187123
sample standard deviation: 20.3593
sample variance: 414.501
kurtosis: -0.856767
skewness: 0.162093
1st quartile: 90.531
median (even number of cells): 106.518
3rd quartile: 126.274
90th percentile: 135.023
```

### Compare to statistics of original raster map

```sh
r.univar -e elevation

total null and non-null cells: 2025000
total null cells: 0

Of the non-null cells:
----------------------
n: 2025000
minimum: 55.5788
maximum: 156.33
range: 100.751
mean: 110.375
mean of absolute values: 110.375
standard deviation: 20.3153
variance: 412.712
variation coefficient: 18.4057 %
sum: 223510266.558102
1st quartile: 94.79
median (even number of cells): 108.88
3rd quartile: 126.792
90th percentile: 138.66
```

### Calculate statistic of distance between sampling points

```sh
v.univar -d samples type=point

number of primitives: 100
number of non zero distances: 4851
number of zero distances: 0
minimum: 69.9038
maximum: 18727.7
range: 18657.8
sum: 3.51907e+07
mean: 7254.33
mean of absolute values: 7254.33
population standard deviation: 3468.53
population variance: 1.20307e+07
population coefficient of variation: 0.478132
sample standard deviation: 3468.89
sample variance: 1.20332e+07
kurtosis: -0.605406
skewness: 0.238688
```

### Output in JSON format

```sh
v.univar -e samples column=heights type=point format=json
```

will output the results in JSON format:

```json
{
    "n": 1832,
    "missing": 0,
    "nnull": 0,
    "min": 166.946991,
    "max": 2729482.25,
    "range": 2729315.3030090001,
    "sum": 78876146.145385057,
    "mean": 43054.664926520229,
    "mean_abs": 43054.664926520229,
    "population_stddev": 132689.08650029532,
    "population_variance": 17606393676.282852,
    "population_coeff_variation": 3.0818747916573215,
    "sample_stddev": 132725.31560308655,
    "sample_variance": 17616009401.938931,
    "kurtosis": 139.15698418811229,
    "skewness": 9.7065048189730767,
    "first_quartile": 3699.3234859999998,
    "median": 10308.4453125,
    "third_quartile": 29259.074218999998,
    "percentiles": [
        {
            "percentile": 90,
            "value": 86449.734375
        }
    ]
}
```

## SEE ALSO

*[db.univar](db.univar.md), [r.univar](r.univar.md),
[v.db.univar](v.db.univar.md), [v.distance](v.distance.md),
[v.neighbors](v.neighbors.md), [v.qcount](v.qcount.md)*

## AUTHORS

Radim Blazek, ITC-irst

extended by:  
Hamish Bowman, University of Otago, New Zealand  
Martin Landa

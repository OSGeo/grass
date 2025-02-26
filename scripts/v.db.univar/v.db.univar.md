## DESCRIPTION

*v.db.univar* calculates basic univariate statistics for numeric
attributes in a vector attribute table. It will calculate minimum,
maximum, range, mean, standard deviation, variance, coefficient of
variation, quartiles, median, and 90th percentile.

*v.db.univar* uses *db.univar* which in turn uses *db.select* to get the
attribute values on which it calculates the statistics. This means that
statistics are calculated based on the entries in the attribute table,
not based on the features in the map. One attribute value is read from
each line in the attribute table, whether there are no, one or several
features with the category value referenced by that line, or whether any
features have more than one category value. For feature-based, instead
of attribute table-based, univariate statistics on attributes see
[v.univar](v.univar.md).

## NOTES

A database connection must be defined for the selected vector layer.

## EXAMPLES

### Univariate statistics on attribute table column

In this example, the 30 years precipitation data table is statistically
analysed (North Carolina sample dataset) and univariate statistics
performed:

```sh
# show columns of attribute table connected to precipitation map
v.info -c precip_30ynormals

# univariate statistics on 30 years annual precipitation in NC
v.db.univar precip_30ynormals column=annual
 Number of values: 136
 Minimum: 947.42
 Maximum: 2329.18
 Range: 1381.76
 Mean: 1289.31147058823
 [...]
```

### Univariate statistics on randomly sampled data points

In this example, random points are sampled from the elevation map (North
Carolina sample dataset) and univariate statistics performed:

```sh
g.region raster=elevation -p
v.random output=samples n=100
v.db.addtable samples column="heights double precision"
v.what.rast samples raster=elevation column=heights
v.db.select samples

v.db.univar samples column=heights
```

### JSON output

This uses the JSON output of the module which is passed using a pipe (in
Bash or other unix-like shell) to the *jq* tool which selects just the
relevant statistic.

```sh
v.db.univar precip_30ynormals column=annual format=json | jq .statistics.mean
```

## SEE ALSO

*[db.univar](db.univar.md), [r.univar](r.univar.md),
[v.univar](v.univar.md), [db.select](db.select.md),
[d.vect.thematic](d.vect.thematic.md), [v.random](v.random.md)*

*[GRASS SQL interface](sql.md)*

## AUTHORS

Michael Barton, Arizona State University

and authors of *r.univar.sh* (Markus Neteler et al.)

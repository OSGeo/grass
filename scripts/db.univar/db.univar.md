## DESCRIPTION

*db.univar* calculates basic univariate statistics for numeric
attributes in a data table. It will calculate minimum, maximum, range,
mean, standard deviation, variance, coefficient of variation, quartiles,
median, and 90th percentile. It uses *db.select* to create list values
for statistical calculations. *NOTES* If the database and driver are not
specified, the default values set in *db.connect* will be used.

## EXAMPLE

In this example, random points are sampled from the elevation map (North
Carolina sample dataset) and univariate statistics performed:

```sh
g.region raster=elevation -p
v.random output=samples n=100
v.db.addtable samples column="heights double precision"
v.what.rast samples raster=elevation column=heights
v.db.select samples

db.univar samples column=heights
```

## SEE ALSO

*[v.db.univar](v.db.univar.md), [r.univar](r.univar.md),
[v.univar](v.univar.md), [db.select](db.select.md),
[d.vect.thematic](d.vect.thematic.md)*

## AUTHORS

Michael Barton, Arizona State University

and authors of *r.univar.sh*

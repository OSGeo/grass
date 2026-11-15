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

### Python example with JSON output

Using the example above, print the 90th percentile in Python:

```python
import grass.script as gs

data = gs.parse_command(
    "db.univar", table="samples", column="heights", format="json", flags="e"
)
print(data["percentiles"])
```

Output:

```text
[{'percentile': 90.0, 'value': 139.0807}]
```

The JSON will look like this:

```json
{
    "n": 100,
    "min": 62.71209,
    "max": 147.2162,
    "range": 84.50410999999998,
    "mean": 110.95470289999996,
    "mean_abs": 110.95470289999996,
    "variance": 362.63393287085927,
    "stddev": 19.042949689343278,
    "coeff_var": 0.17162814366242862,
    "sum": 11095.470289999996,
    "first_quartile": 95.56498,
    "median": 107.5519,
    "third_quartile": 125.1526,
    "percentiles": [
        {
            "percentile": 90.0,
            "value": 139.0807
        }
    ]
}
```

## SEE ALSO

*[v.db.univar](v.db.univar.md), [r.univar](r.univar.md),
[v.univar](v.univar.md), [db.select](db.select.md),
[d.vect.thematic](d.vect.thematic.md)*

## AUTHORS

Michael Barton, Arizona State University

and authors of *r.univar.sh*

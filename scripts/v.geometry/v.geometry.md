## DESCRIPTION

*v.geometry* prints geometry metrics of vector features. Output is
available as JSON, CSV, or plain text.

One or more **metric** values can be requested per invocation. When
multiple metrics are given, they are computed in parallel and the
results are merged by category. Supported metrics are:

- `area`, `perimeter`, `compactness`, `fractal_dimension`, `bbox` -
  for areas (and boundaries that form areas)
- `length`, `slope`, `sinuosity`, `azimuth`, `start`, `end` - for
  lines and boundaries
- `coordinates` - for points and centroids
- `count` - number of features per category

Values are aggregated per category.

Measures of lengths and areas are reported in meters by default; use
the **units** option to change this.

## NOTES

*v.geometry* is a read-only front-end to *[v.to.db](v.to.db.md)* and
accepts the same set of units. It does not read from or write to the
attribute table, so no table needs to be attached to the input map.

For writing metrics back into an attribute table, use
*[v.to.db](v.to.db.md)* directly. Features of *v.to.db* that are not
purely geometric, such as cross-layer attribute queries
(`option=query`), are intentionally not exposed here.

Records are keyed by category. When multiple metrics are requested, the
values for each category are merged into one record. If a category is
shared across features of different types (for example a line and an
area with the same cat), their metrics would end up in the same record
even though they describe different features. To avoid this, *v.geometry*
rejects combinations of metrics that belong to different feature-type
families (area, line, point); the `count` metric is universal and can
be combined with any of them. Run *v.geometry* separately for each
feature type if you need metrics from more than one family.

When `count` is combined with another metric (for example
`metric=area,count`), the count is aligned to the categories covered
by that metric. Categories that only appear in the count result are
dropped so every merged record carries both values, and the reported
count total reflects only the aligned features.

## EXAMPLES

Report area sizes of geology polygons in hectares:

```sh
v.geometry map=geology metric=area units=hectares
```

Compute area, perimeter, and compactness in a single call:

```sh
v.geometry map=geology metric=area,perimeter,compactness
```

Consume metrics from Python:

```python
import grass.script as gs

data = gs.parse_command("v.geometry", map="geology", metric="compactness")
for record in data["records"]:
    print(record["category"], record["compactness"])
```

Sample JSON output for `metric=length`:

```json
{
    "units": {
        "length": "meters"
    },
    "totals": {
        "length": 10426.657857419743
    },
    "records": [
        {
            "category": 1,
            "length": 4554.943058982206
        },
        {
            "category": 2,
            "length": 5871.714798437538
        }
    ]
}
```

## SEE ALSO

*[r.object.geometry](r.object.geometry.md),
[v.category](v.category.md), [v.db.join](v.db.join.md),
[v.report](v.report.md), [v.to.db](v.to.db.md),
[v.univar](v.univar.md)*

## AUTHORS

Anna Petrasova, NCSU GeoForAll Laboratory

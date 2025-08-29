## DESCRIPTION

*v.what* outputs the features associated with
user-specified location(s) in user-specified vector map(s).
The tool operates on features which are vector geometry objects,
such as point or area. The result is a list of these features
along with their associated categories for the layer specified by *layer*.
If there are no categories for the specified layer, the feature
is not included in the result. With `layer=-1` (all layers), all features
are included in the result regardless of their categories.

### Output content

By default, the closest feature is returned for each coordinate and each
vector map if the feature fulfills the geometry query and layer selection
criteria. With the *-m* flag, all matching features are returned, not just
the closest one.

If an attribute database connection is defined for a given layer
and the *-a* flag is specified, attributes from the associated attribute table
will be returned for each category associated with the feature.

The tool operates on features defined as vector geometry objects as opposed
to features defined by categories. Consequently, the output is organized
by geometry IDs, to which possible categories and attributes are attached.
If multiple geometries have the same category, the same set of attributes
is repeated for each geometry.

The output also includes the coordinates used in the query,
the vector map name, and the mapset. For vector lines,
the length is returned. The *-d* flag returns internal topological
information.

### JSON output

With `format="json"`, a list of matching features is returned.
Each feature includes the geometry ID (`id`), geometry type (`type`),
vector map name (`map` and `mapset`), and the relevant part of the spatial
query (`coordinates`).
If the feature has associated categories for the given *layer*,
they are included under `data` in a list of items with `layer` and `category` values.
With *-a*, `data` will also include `attributes` for each `category`.

A feature is not included in the result if there are no categories
for the specified layer. For `layer=-1`, all features
are included in the result and each feature's `data` will contain all
associated categories in all layers.

With the *-m* flag, each list item contains coordinates, a vector map name,
and a list of matching features under the `features` key.
In other words, rather than being organized by feature, the list now contains
lists of features nested under each combination of coordinate pair and vector map.

## NOTES

The *-g* and *-j* flags are deprecated and will be removed in a future release.
Please, use `format="json"` instead.

The behavior of the *-d* flag for internal topology information is not guaranteed.

## EXAMPLE

North Carolina sample dataset example:

Query polygon at given position:

```sh
v.what zipcodes_wake coordinates=637502.25,221744.25
```

Find closest hospital to given position within given distance (search
radius):

```sh
v.what hospitals coordinates=542690.4,204802.7 distance=2000000
```

## SEE ALSO

*[d.what.rast](d.what.rast.md), [d.what.vect](d.what.vect.md),
[v.rast.stats](v.rast.stats.md), [v.vect.stats](v.vect.stats.md),
[v.what.rast](v.what.rast.md), [v.what.rast3](v.what.rast3.md),
[v.what.vect](v.what.vect.md), [r.what](r.what.md)*

## AUTHOR

Trevor Wiens  
Edmonton, Alberta, Canada

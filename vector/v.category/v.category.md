## DESCRIPTION

*v.category* attaches, copies, deletes or reports categories to/from/of
vector geometry objects. Further on, *v.category* adds a number given by
the *cat* option to categories of the selected layer. These categories
(IDs) are used to assign IDs or to group geometry objects into
categories (several different geometry objects share the same category).
These categories are also used to link geometry object(s) to attribute
records (from an attribute table linked to vector map).

## NOTES

Use *[v.to.db](v.to.db.md)* to upload related categories to a linked
attribute table.

The **type** parameter specifies the type of geometry objects to which
the category is added; it is similar to an input filter - only the
geometry specified in 'type' is processed.

If the **type** parameter is set to **centroid** and the **option**
parameter set to **add**, new categories will be added to existing
centroids. Note however, that new centroids cannot be created this way.
To do so, they must be added manually using *[wxGUI vector
digitizer](wxGUI.vdigit.md)* or by running *v.category* with the type
parameter set to area.

If categories are copied with *option=transfer*, a warning is issued if
categories already exit in the layer they are copied to. In this case
the user must make sure beforehand that copying categories from one
layer to another layer does not cause undesired grouping of different
geometry objects into the same categories. This can be avoided by
specifying only one *layer*. The module will then find the next free
layer number and copy categories to there. The new layer number is
reported at the end.

Areas are a special case because it is impossible to attach a cat to an
area without a centroid; in this case, the module places new centroids
in areas automatically for **type=area**.

The **cat** parameter is only used with **option**=*add*,
**option**=*sum* and **option**=*del*.

Categories can be deleted for the given layer with *option=del*. If
**cat** is set to *-1*, all categories for the given layer are deleted.
If **cat** is zero or positive, only this category value will be
deleted. By default, **cat** is set to *1* which means that only
categories of value *1* will be deleted.

With **option=report**, the module reports for each layer and type the
total number of categories, the minimum and the maximum category number.
If there are e.g. two lines with line 1 having category 1 and line 1
having categories 1 and 2, then there are a total of three category
values with minimum 1 and maximum 2.

The **ids** parameter specifies the list of feature IDs to which the
operation is performed; by default, all vector feature ids are
processed. The *feature ID* is an internal (unique) geometry ID that all
vector primitives possess, and is separate from any category the feature
may also possess. Use

```sh
  v.edit map=inputname tool=select
```

to find out the geometry ids of certain features.

## EXAMPLES

### Report vector categories

```sh
v.category input=testmap option=report

LAYER/TABLE 1/testmap:
type       count        min        max
point          0          0          0
line        1379          1       1379
boundary       0          0          0
centroid       0          0          0
area           0          0          0
all         1379          1       1379
```

Report vector categories in JSON format:

```sh
v.category input=testmap option=report format=json

[
  {
      "type": "line",
      "layer": 1,
      "count": 1379,
      "min": 1,
      "max": 1379
  },
  {
      "type": "all",
      "layer": 1,
      "count": 1379,
      "min": 1,
      "max": 1379
  }
]
```

### Delete all vector categories in layer 1

```sh
v.category input=testmap output=outmap option=del cat=-1
```

### Add vector categories in layer 1 with step=2

```sh
v.category input=outmap output=stepmap option=add step=2

# report
v.category input=stepmap option=report
LAYER/TABLE 1/outmap:
type       count        min        max
point          0          0          0
line        1379          1       2757
boundary       0          0          0
centroid       0          0          0
area           0          0          0
all         1379          1       2757
```

### Add categories/centroids to a vector map without categories

```sh
v.category input=wkt output=wktnew option=add
```

Results can be tested using *[d.what.vect](d.what.vect.md)*.

### Copy categories from layer 1 to layer 2,3,4,5,6,7 and 8

Existing layer will be overwritten, non-existing will be created.

```sh
v.category input=observer output=observer_new option=transfer layer=1,2,3,4,5,6,7,8
```

### Print vector categories of given layer

Print vector categories from the first layer, only for feature ids 1-50.

```sh
v.category input=roads option=print layer=1 id=1-50
```

Print vector categories from the first layer, only for feature ids 1-50 in JSON format.

```sh
v.category input=roads option=print layer=1 id=1-50 format=json
```

### Print only layer numbers in JSON format

```sh
v.category input=roads option=layers format=json
```

### Using v.category JSON output with pandas

Using report option in JSON format with pandas:

```python
import grass.script as gs
import pandas as pd

# Run v.category command with report option.
data = gs.parse_command(
    "v.category",
    input="bridges",
    option="report",
    format="json",
)

df = pd.DataFrame(data)
print(df)
```

```sh
    type  layer  count  min    max
0  point      1  10938    1  10938
1    all      1  10938    1  10938
```

Using print option with the first layer, only for feature ids 1-5 in JSON
format with pandas:

```python
import grass.script as gs
import pandas as pd

# Run v.category command with print option.
data = gs.parse_command(
    "v.category",
    input="bridges",
    option="print",
    ids="1-5",
    format="json",
)

df = pd.DataFrame(data)
print(df)
```

```sh
    id  layer  category
0   1      1         1
1   2      1         2
2   3      1         3
3   4      1         4
4   5      1         5
```

## SEE ALSO

*[v.centroids](v.centroids.md), [v.db.connect](v.db.connect.md),
[v.edit](v.edit.md), [v.to.db](v.to.db.md)*

## AUTHOR

Radim Blazek, ITC-irst, Trento, Italy  
Modified (the id parameter) by Martin Landa, FBK-irst (formerly
ITC-irst), Trento, Italy, 2008/02

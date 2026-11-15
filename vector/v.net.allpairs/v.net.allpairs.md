## DESCRIPTION

*v.net.allpairs* computes the shortest path between each selected node
and all other selected nodes. The output is a vector with the selected
nodes and the shortest paths.

## NOTES

An attribute table is created and linked to layer *arc_layer*. The table
contains four columns: *cat*, *from_cat*, *to_cat*, *cost*. Each *cat*
entry denotes the category of the shortest path from the node with
category *from_cat* to the node with category *to_cat*. If points are
specified by **cats, layer** or **where** parameters then the table is
filled only for the selected points.  
If **arc_backward_column** is not given then then the same costs are
used for forward and backward arcs.

## EXAMPLE

Find shortest path along roads from selected archsites (Spearfish sample
dataset):

```sh
# prepare network: connect archsites to roads with threshold 200
v.net input=roads@PERMANENT points=archsites@PERMANENT \
output=roads_net operation=connect thresh=200

# verify result
v.category input=roads_net option=report

# only lines should have a category in layer 1
# only points should have a category in layer 2

# shortest path between all points with categories 1 - 5 in layer 2
v.net.allpairs input=roads_net cats=1-5 out=roads_net_all
v.db.select roads_net_all
```

Result in matrix form:

```sh
from\to 1       3       4       5
1   0       18820.386   17206.651   17373.274
3   18820.386   0       1739.079    9040.575
4   17206.651   1739.079    0       7426.84
5   17373.274   9040.575    7426.84     0
```

## SEE ALSO

*[v.net.path](v.net.path.md), [v.net.distance](v.net.distance.md)*

## AUTHORS

Daniel Bundala, Google Summer of Code 2009, Student  
Wolf Bergenheim, Mentor  
Markus Metz

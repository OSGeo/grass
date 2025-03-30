## DESCRIPTION

*v.net.components* computes the weakly and strongly connected components
in a network.

## NOTES

Two nodes, *u* and *v* are in the same strongly connected component if
there are directed paths from *u* to *v* and from *v* to *u*. The nodes
are in the same weakly connected component if, ignoring edge directions,
there is a path between them.

The type of components is specified by **method** parameter.
*v.net.components* creates a table and links it to layer 1. This table
contains only two integer columns: *cat* and *comp*. If a point or both
endpoints of a line belong to the same component then the point/line is
written to the output map and appropriate information is stored in the
table. If **-a** flag is set then new points are added on the nodes
without points. These points have category numbers larger than any
category used in the input map.

One-way roads can be defined by assigning a cost of -1 to the
appropriate cost column (**arc_column** or **arc_backward_column**).
This affects only strongly connected components. Network nodes can be
closed by assigning a cost of -1 to the node cost column. All nodes with
a cost \< 0 can not be traversed and are end points, while all nodes
with a cost ≥ 0 can be traversed. This affects both weakly and strongly
connected components.

## EXAMPLES

Any road network should form a single strongly connected component.
Otherwise, it is impossible to travel between some places.

```sh
v.net.components input=roads output=roads_components method=strong
```

## SEE ALSO

*[v.net](v.net.md), [v.category](v.category.md)*

## AUTHORS

Daniel Bundala, Google Summer of Code 2009, Student  
Wolf Bergenheim, Mentor  
Markus Metz

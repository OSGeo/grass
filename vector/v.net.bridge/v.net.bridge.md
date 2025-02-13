## DESCRIPTION

*v.net.bridge* finds bridges and articulation points in a network.

## NOTES

Bridge in a network is an edge/line whose removal would disconnect the
(sub-)network. A node is an articulation point if its removal would
disconnect the (sub-)network. For more information and formal
definitions check the wikipedia entries:
[bridge](https://en.wikipedia.org/wiki/Bridge_%28graph_theory%29) and
[articulation point](https://en.wikipedia.org/wiki/Cut_vertex).

The output of the module contains the selected features. For
**method=bridge**, lines corresponding to bridges are copied from the
input map to the output map. On the other hand, for
**method=articulation**, points are created on the positions of
articulation points.

In GRASS GIS, *line* is not always a single line segment. It might be,
and often is, a sequence of line segments between two intersections.
Also, articulation point is a standard graph theoretic terminology which
is slightly misleading in GRASS. An articulation point in graph theory
is an articulation *node* in GRASS terminology.

## SEE ALSO

*[v.net](v.net.md), [v.category](v.category.md)*

## AUTHORS

Daniel Bundala, Google Summer of Code 2009, Student  
Wolf Bergenheim, Mentor

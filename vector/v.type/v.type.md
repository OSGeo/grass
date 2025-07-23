## DESCRIPTION

*v.type* changes the type of geometry primitives.

The following vector object types are defined in GRASS GIS:

- point: a point;
- line: a directed sequence of connected vertices with two endpoints
  called nodes;
- boundary: the border line describing an area;
- centroid: a point within a closed ring of boundaries;
- area: the topological composition of a closed ring of boundaries and a
  centroid;
- face: a 3D area;
- kernel: a 3D centroid in a volume (not yet implemented);
- volume: a 3D corpus, the topological composition of faces and kernel
  (not yet implemented).

Lines and boundaries can be composed of multiple vertices.

Area topology also holds information about isles. These isles are
located within that area, not touching the boundaries of the outer area.
Isles are holes inside the area, and can consist of one or more areas.
They are used internally to maintain correct topology for areas.

## EXAMPLES

*Convert lines to area boundaries*  

```sh
v.type input=map_l output=map_b from_type=line to_type=boundary
```

In order to create areas, centroids must be added with **v.centroids**.

## SEE ALSO

*[v.centroids](v.centroids.md), [v.to.points](v.to.points.md)*

## AUTHOR

Radim Blazek, ITC-Irst, Trento, Italy

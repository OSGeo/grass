## DESCRIPTION

*v.voronoi* creates a Voronoi diagram (Thiessen polygons) from points or
centroids.

The bounds of the output map are limited by the current region (see
*[g.region](g.region.md)*).

The *-a* flag can be used to create a Voronoi diagram for areas.

The *-s* flag can be used to extract the center line of areas or
skeletons of areas with *thin* \>= 0. Smaller values for the *thin*
option will preserve more detail, while negative values will extract
only the center line.

## NOTES

*v.voronoi* suffers from numerical instability, results can sometimes
contain many artefacts. When creating Voronoi diagrams for areas or
skeletons for areas, it is highly recommended to simplify the areas
first with *[v.generalize](v.generalize.md)*.

Voronoi diagrams may be used for nearest-neighbor flood filling. Give
the centroids attributes (start with
*[v.db.addcolumn](v.db.addcolumn.md)*), then optionally convert the
result to a raster map with *[v.to.rast](v.to.rast.md)*.

The extraction of skeletons and center lines with the *-s* flag is a
brute force approach. Faster and more accurate algorithms to extract
skeletons from areas exist but are not yet implemented. In the meantime,
skeletons and center lines can be simplified with the Douglas-Peucker
algorithm: *[v.generalize method=douglas](v.generalize.md)*.

## EXAMPLE

### Voronoi diagram for points

This example uses the hospitals in the North Carolina dataset.

```sh
g.region -p raster=elev_state_500m
v.voronoi input=hospitals output=hospitals_voronoi
```

Result:

![Voronoi diagram for hospitals in North Carolina](v_voronoi_points.png)  
*Voronoi diagram for hospitals in North Carolina*

### Voronoi diagram for areas

This example uses urban areas in the North Carolina dataset.

```sh
g.region -p n=162500 s=80000 w=727000 e=846000 res=500
v.voronoi input=urbanarea output=urbanarea_voronoi -a
```

Result:

![Voronoi diagram for urban areas in North Carolina](v_voronoi_areas.png)  
*Voronoi diagram for urban areas in North Carolina*

### Skeletons and center lines of areas

This example uses urban areas in the North Carolina dataset.

```sh
g.region -p n=161000 s=135500 w=768500 e=805500 res=500
v.voronoi input=urbanarea output=urbanarea_centerline -s
v.voronoi input=urbanarea output=urbanarea_skeleton -s thin=2000
```

Result:

![Skeleton and center line for urban areas in North Carolina](v_voronoi_skeleton.png)  
*Skeleton (blue) and center line (red) for urban areas in North Carolina*

## REFERENCES

*Steve J. Fortune, (1987). A Sweepline Algorithm for Voronoi Diagrams,
Algorithmica 2, 153-174 ([DOI](https://doi.org/10.1007/BF01840357)).*

## SEE ALSO

*[g.region](g.region.md), [v.delaunay](v.delaunay.md),
[v.hull](v.hull.md)*

[Voronoi diagram
(Wikipedia)](https://en.wikipedia.org/wiki/Voronoi_diagram)

## AUTHORS

James Darrell McCauley, Purdue University  
GRASS 5 update, improvements: [Andrea Aime](mailto:aaime@libero.it),
Modena, Italy  
GRASS 5.7 update: Radim Blazek  
Markus Metz

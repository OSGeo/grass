## DESCRIPTION

GRASS defines vector areas as composite entities consisting of a set of
closed boundaries and a centroid. The attribute information associated
with that area is linked to the centroid. The *v.centroids* module adds
centroids to closed boundaries in the **input** file and assigns a
category number to them. The starting value as well as the increment
size may be set using optional parameters.

Multiple attributes may be linked to a single vector entity through
numbered fields referred to as layers. Refer to *v.category* for more
details, as *v.centroids* is simply a frontend to that module.

The boundary itself is often stored without any category reference as it
can mark the border between two adjacent areas. Thus it would be
ambiguous as to which feature the attribute would belong. In some cases
it may, for example, represent a road between two parcels of land. In
this case it is entirely appropriate for the boundary to contain
category information.

## EXAMPLES

Create an area from a closed line using North Carolina sample dataset:

::: code
    v.type input=busroute11 output=busroute11_boundary from_type=line to_type=boundary
    v.centroids input=busroute11_boundary output=busroute11_area
:::

::: {align="center" style="margin: 10px"}
[![v.centroids example](v_centroids.png){width="600" height="300"
border="0"}](v_centroids.png)\
*Figure: Creating area from closed line*
:::

## SEE ALSO

*[v.category](v.category.html)*

## AUTHORS

module: M. Hamish Bowman, Dept. Marine Science, Otago University, New
Zealand\
help page: Trevor Wiens

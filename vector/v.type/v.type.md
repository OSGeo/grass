## DESCRIPTION

*v.type* changes the type of geometry primitives.

## EXAMPLES

*Convert lines to area boundaries*\

::: code
    v.type input=map_l output=map_b from_type=line to_type=boundary
:::

In order to create areas, centroids must be added with **v.centroids**.

## SEE ALSO

*[v.centroids](v.centroids.html), [v.to.points](v.to.points.html)*

## AUTHOR

Radim Blazek, ITC-Irst, Trento, Italy

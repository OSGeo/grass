## DESCRIPTION

*v.build* builds support files for GRASS vector maps. These support
files supply topology and category information including spatial index
that are needed by other GRASS modules.

GRASS is generating these support files automatically, only in rare
cases the user has to (re)build them.

Refer to [vector data processing in GRASS GIS](vectorintro.md) for more
information on GRASS vector data model.

## NOTES

*v.build* creates support files only for vector maps located in the
user's current mapset. It's not possible to rebuild support files
(**option=build**) for vector maps from other mapsets.

In case of errors, the user can optionally generate an **error** vector
map containing the erroneous vectors for later inspection.

If **error** vector map is specified, *v.build* checks:

- isolated boundaries (which are not forming any areas),
- centroids outside of area,
- duplicated centroids.

Extensive checks for topological errors (flag **-e**) also includes:

- lines or boundaries of zero length,
- intersecting boundaries, i.e. overlapping areas,
- areas without centroids that are not isles.

## EXAMPLES

### Build topology

Note that **option=build** also recreates spatial and category indices,
not only topology. For linked OGR layers (see
*[v.external](v.external.md)*) also feature index is created.

```sh
v.build map=urbanarea option=build
```

Note that the vector map *urbanarea* must be located in the current
mapset.

### Dump topology or indices

Dump options print topology, spatial, category or feature index to
standard output. Such information can also be printed for vector maps
from other mapsets. A description of the vector topology is available in
the [GRASS GIS 8 Programmer's
Manual](https://grass.osgeo.org/programming8/vlibTopology.html), section
"Vector library topology management".

```sh
v.build map=urbanarea option=dump
```

## SEE ALSO

*[v.build.all](v.build.all.md),
[v.build.polylines](v.build.polylines.md), [v.edit](v.edit.md),
[v.split](v.split.md), [v.support](v.support.md)*

See also *[wxGUI vector digitizer](wxGUI.vdigit.md)*.

## AUTHORS

Dave Gerdes, U.S.Army Construction Engineering Research Laboratory,  
Michael Higgins, U.S.Army Construction Engineering Research
Laboratory,  
Radim Blazek, ITC-irst, Trento, Italy

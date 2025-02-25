## DESCRIPTION

The *r.region* program allows the user to manage the boundaries of a
raster map. These boundaries can be set by the user directly and/or set
from a region definition file (stored under the `windows` directory in
the user's current mapset), a raster or vector map, or a 3dview file.

The **align** parameter sets the current resolution equal to that of the
named raster map, and align the boundaries to a row and column edge in
the named map. Alignment only moves the existing boundaries outward to
the edges of the next nearest cell in the named raster map -- not to the
named map's edges. To perform the latter function, use the
**raster**=*name* option.

## NOTES

After all updates have been applied, the raster map's resolution
settings are recomputed from the boundaries and the number of rows and
columns in the raster map.

The n=*value* may also be specified as a function of its current value:
n=n+*value* increases the current northing, while n=n-*value* decreases
it. This is also true for s=*value*, e=*value*, and w=*value*.

## EXAMPLES

Assign absolute coordinates to map:

```sh
r.region map=mymap n=220750 s=220000 w=638300 e=639000
```

Shift map (using offset, here by +100 map units in the NS direction, -50
in the EW direction):

```sh
r.region map=mymap n=n+100 e=e-50 w=w-50 s=s+100
```

## SEE ALSO

*[r.support](r.support.md), [g.region](g.region.md),
[v.transform](v.transform.md)*

## AUTHOR

Glynn Clements  
Based upon *g.region*

## DESCRIPTION

The *g.copy* module creates a copy of existing raster maps, vector maps,
or other elements. The copy is always created in the current mapset. The
source data can be in the current mapset, in an explicitly specified
mapset, or in a mapset which is in the current mapset search path
(typically the PERMANENT mapset).

The maps and other elements to copy are specified in pairs
**from**,**to** according to their types. Although typically only one
map is copied in one module call, multiple pairs can be provided for
each type and multiple types can be provided at the same time.

### Relation to mapsets

A user may access data stored under the other mapsets listed in their
mapset search path. However, the user may only modify data stored under
their own current mapset. *g.copy* allows the user to copy existing data
files **from** other mapsets **to** the user's current mapset
(`g.mapset -p`). The files to be copied must exist in the user's current
mapset search path (`g.mapsets -p`) and project; output is sent to the
relevant data element directory(ies) under the user's current mapset.

### Behavior on error

Errors typically occur when a map or other element does not exist,
**from** and **to** are the same, **to** element already exists and
overwriting (e.g., by **--overwrite**) is not enabled, or the **to**
element has an illegal name. When only one map or other element is
requested to be copied and the copying is not possible or fails, an
error is reported.

If multiple maps or other elements are copied in one command, *g.copy*
attempts to copy as much as possible even when problems occur with one
of the elements. In that case, copying of the element causing problems
is skipped, and *g.copy* proceeds with copying the remaining elements.
If nothing can be copied or an error occurred during one of the copy
operations, an error message is reported after other possible copy
operations were performed.

## EXAMPLES

If the user wished to copy the existing raster file *soils* to a file
called *soils.ph* and to copy an existing vector map *roads* to a file
called *rds.old*, the user could type:

```sh
g.copy raster=soils,soils.ph
g.copy vector=roads,rds.old

# or even combined:
g.copy raster=soils,soils.ph vector=roads,rds.old
```

Data files can also be specified by their mapsets. For example, the
below command copies the raster map named *soils* from the mapset
*wilson* to a new file called *soils* to be placed under the user's
current mapset:

```sh
g.copy raster=soils@wilson,soils
```

If no mapset name is specified, *g.copy* searches for the named **from**
map in each of the mapset directories listed in the user's current
mapset search path in the order in which mapsets are listed there (see
*[g.mapsets](g.mapsets.md)*).

## SEE ALSO

*[g.access](g.access.md), [g.list](g.list.md),
[g.mapsets](g.mapsets.md), [g.remove](g.remove.md),
[g.rename](g.rename.md)*

## AUTHOR

Michael Shapiro, U.S. Army Construction Engineering Research Laboratory

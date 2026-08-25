## DESCRIPTION

*g.rename* allows the user to rename data base element files in the
user's current mapset. The user can specify all necessary information to
*g.rename* on the command line, by specifying: the type of data base
element to be renamed (one or more of: **raster**, **raster_3d**,
**vector**, **icon**, **labels**, **region**, and **group**); the
specific file element in the current mapset to be renamed (*old*); and
the new name to be assigned to this file element (*new*) in the current
mapset. The file element *old* is then renamed to *new*.

Users can also simply type *g.rename --help* without arguments on the
command line, to receive a menu of existing data base element types and
files from which to choose for possible renaming:

```sh
       raster   raster map(s) to be renamed
    raster_3d   3D raster map(s) to be renamed
       vector   vector map(s) to be renamed
       labels   paint label file(s) to be renamed
       region   region definition(s) to be renamed
        group   imagery group(s) to be renamed
```

## NOTES

If a data base element has support files (e.g., as is commonly the case
with raster maps), these support files also are renamed.

If the user attempts to rename a file to itself by setting the *new*
file name equal to the *old* file name (e.g., **g.rename
raster=soils,soils**), *g.rename* will not execute the rename, but
instead state that no rename is needed. However, *g.rename* will allow
the user to overwrite other existing files in the current mapset by
making the *new* file name that of an already existing file.

For portability reasons, *g.rename* is ignoring case of names. To change
the case of a map name, first rename the map to a name which differs by
more than case, then rename it to the intended name.

## EXAMPLE

```sh
# rename raster map
g.rename raster=oldrast,newrast

# rename vector map
g.rename vector=oldvect,newvect

# combined renaming
g.rename raster=oldrast,newrast vector=oldvect,newvect
```

## SEE ALSO

*[g.copy](g.copy.md), [g.list](g.list.md), [g.remove](g.remove.md)*

## AUTHOR

Michael Shapiro, U.S.Army Construction Engineering Research Laboratory

## DESCRIPTION

*v.patch* allows the user to combine any number of vector maps together
to create one composite vector map. If the table structures are
identical, attributes are transferred to the new table.

## NOTES

Any vectors that are duplicated among the maps being patched together
(e.g., border lines) will have to be edited or removed after *v.patch*
is run. Such editing can be done automatically using
*[v.clean](v.clean.md)*.

Lines may need to be snapped with *[v.clean](v.clean.md)
tool=snap,break,rmdupl*.

Boundaries may need to be cleaned with *[v.clean](v.clean.md)
tool=break,rmdupl,rmsa* repeatedly until the *rmsa* tool (Remove small
angles at nodes) no longer modifies any boundaries. If vector topology
is still not clean, boundaries may also need to be snapped with
*[v.clean](v.clean.md) tool=snap,break,rmdupl*.

When using the *-e* flag, *v.patch* shifts category (cat) values in the
output so that category numbers from the different input maps do not
overlap. This shift is applied to both the category values of the
features and the category values in the attribute tables. Hence, there
is no need to run *[v.category](v.category.md)* and
*[v.db.update](v.db.update.md)* beforehand.

When using the *-a* flag, the user has to make sure that the features in
the different maps added to the output map do not have overlapping
category numbers, unless identical category numbers reflect identical
attributes, otherwise the attributes of the added maps are lost. To
avoid this, the user can use *v.category option=sum* to change category
values of some of the maps before patching.

## EXAMPLES

Patch together two maps with mixed feature types:

```sh
   v.patch input=geology,streams out=geol_streams
```

Append one map to another:

```sh
   g.copy vect=roads,transport
   v.patch -a input=railroads output=transport --overwrite
```

## SEE ALSO

*[v.clean](v.clean.md), [v.build](v.build.md), [v.select](v.select.md),
[v.overlay](v.overlay.md)*

## AUTHORS

Dave Gerdes, U.S.Army Construction Engineering Research Laboratory  
Radim Blazek, ITC-Irst, Trento, Italy

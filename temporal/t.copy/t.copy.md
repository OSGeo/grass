## DESCRIPTION

The purpose of **t.copy** is to create a copy of a space time dataset.
The new space time dataset will be located in the current mapset,
whereas the old space time dataset can be located in a different mapset
as long as this mapset is in the search path.

If the *-c* flag is given, the maps of the old space time dataset will
also be copied to the current mapset, otherwise the original maps will
be simply registered in the temporal database. The new copies will have
the same name as the old maps.

## NOTES

A fully qualified name for the input space-time dataset is only required
if space-time datasets with the same name exist in different mapsets.

## EXAMPLE

In the North Carolina sample dataset with the separately available
mapset *modis_lst* included, copy the space-time raster dataset
*LST_Day_monthly@modis_lst* to the current mapset *user1*:

```sh
g.mapsets mapset=modis_lst operation=add
t.copy input=LST_Day_monthly@modis_lst type=strds output=LST_Day_monthly
```

## SEE ALSO

*[t.rast.extract](t.rast.extract.md),
[t.rast3d.extract](t.rast3d.extract.md),
[t.vect.extract](t.vect.extract.md), [t.info](t.info.md)*

## AUTHOR

Markus Metz, [mundialis](https://www.mundialis.de)

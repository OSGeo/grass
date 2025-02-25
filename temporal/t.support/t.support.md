## DESCRIPTION

The *t.support* module is dedicated to modify and update the metadata of
a space time dataset.

The title, description and the semantic type can be modified.

The flag *-u* allows updating the STDS metadata from registered map
layers. This is useful in case the map layers have been modified without
using temporal commands.

The flag *-m* will update the metadata from registered maps, but also
checks if the registered map layers have been removed from the spatial
database. It deletes missing map layers from the space time dataset
register table and the temporal database.

## EXAMPLES

Modification of title and description of space time raster dataset *A*.

```sh
t.support type=strds input=tempmean_monthly title="Monthly temperature for North Carolina" \
          description="Dataset with monthly temperature for North Carolina"
```

Update the metadata of space time raster dataset *A* and check for
removed map layers.

```sh
t.support -m type=strds input=tempmean_monthly
```

## SEE ALSO

*[t.create](t.create.md), [t.info](t.info.md)*

## AUTHOR

Sören Gebbert, Thünen Institute of Climate-Smart Agriculture

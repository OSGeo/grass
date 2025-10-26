## DESCRIPTION

*t.create* is used to create space time datasets of type raster (STRDS),
3D raster (STR3DS) and vector (STVDS).

Space time datasets represent spatio-temporal fields in the temporal
GRASS framework. They are designed to collect any amount of time stamped
maps with time intervals and time instances. The temporal type of a
space time dataset can be absolute (means with a fixed date) or relative
(only sequential maps) and must be set during dataset creation along
with the name and the description.

Time stamped maps can registered in and unregistered from space time
datasets. The spatio-temporal extent as well as the metadata of a space
time dataset is derived from its registered maps. Hence the metadata is
dependent from the dataset type (raster, 3D raster, vector).

## EXAMPLE

### Absolute STRDS dataset

Create a raster space time datasets

```sh
t.create type=strds temporaltype=absolute \
         output=precipitation_monthly \
         title="Monthly precipitation" \
         description="Dataset with monthly precipitation"
```

### Relative STVDS dataset

Create a vector space time datasets

```sh
t.create type=stvds temporaltype=relative \
         output=precipitation_monthly_30y \
         title="Monthly precipitation 30 years" \
         description="Test dataset with monthly average \
         precipitation in the last 30 year"
```

## SEE ALSO

*[t.register](t.register.md), [t.remove](t.remove.md),
[t.info](t.info.md)*

## AUTHOR

Sören Gebbert, Thünen Institute of Climate-Smart Agriculture

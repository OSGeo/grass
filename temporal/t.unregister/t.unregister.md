## DESCRIPTION

The *t.unregister* module is designed to unregister raster, 3D raster
and vector map layers from space time datasets and the temporal
database.

Map layer that should be unregistered from the temporal database can be
specified as a list of comma separated map names or using a text file,
that contains one map layer name per line. By default the map type that
should be unregistered is set to raster. The option*type* must be used
to specify 3D raster or vector map layer types.

## INPUT FILE FORMAT

Specification of map names:

```sh
a1
a2
a3
a4
a5
a6
```

### NOTE

In case the *input* option is used to specify a space time dataset the
maps are only unregistered from the space time dataset, but not from the
temporal database. The reason is that maps can be registered in multiple
space time datasets and there is a need to unregister them from a
specific STDS without affecting other STDS.

## EXAMPLE

In this example a precipitation map it is registered into temperature
dataset, so the wrong map will be unregister.

```sh

t.register -i type=raster input=tempmean_monthly@climate_2009_2012 \
    maps=2012_01_precip \
    start="2013-01-01" increment="1 month"

# We unregister raster map 2012_01_precip from a space time dataset,
# the raster maps are still present in the temporal database
t.unregister type=raster input=tempmean_monthly@climate_2009_2012 maps=2012_01_precip

# We unregister raster map 2012_01_precip from the temporal database, hence
# the time stamps are removed
t.unregister type=raster maps=2012_01_precip
```

## SEE ALSO

*[t.create](t.create.md), [t.info](t.info.md)*

## AUTHOR

Sören Gebbert, Thünen Institute of Climate-Smart Agriculture

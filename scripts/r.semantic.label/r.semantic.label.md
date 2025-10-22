## DESCRIPTION

*r.semantic.label* allows assigning a semantic label information to a
single raster map or to a list of specified raster maps. Semantic label
can be defined by **semantic_label** option. Already assigned semantic
label can be removed from a specified raster map by
**operation=remove**. The module also allows printing detailed semantic
label information already assigned to a raster map by
**operation=print**.

Either a single raster map or a list of raster maps can be given by
**map** option.

## NOTES

Note that *only raster maps from the current mapsets* can be modified.

For more information about semantic label concept and supported sensors
(generic multispectral system, Landsat-5, Landsat-7, Landsat-8,
Sentinel-2) see the *[i.band.library](i.band.library.md)* module.

Semantic labels are supported by temporal GRASS modules. Name of STRDS
can be extended by band identifier in order to filter the result by a
semantic label. See
*[t.register](t.register.md#support-for-semantic-labels)*,
*[t.rast.list](t.rast.list.md#filtering-the-result-by-semantic-label)*,
*[t.info](t.info.md#space-time-dataset-with-semantic-labels-assigned)*
and *[t.rast.mapcalc](t.rast.mapcalc.md#semantic-label-filtering)*
modules for examples.

## EXAMPLES

### Assign semantic label to a single raster map

```sh
r.semantic.label map=T33UVR_20180506T100031_B01 semantic_label=S2_1
```

### Assign semantic label to a list of raster maps

```sh
r.semantic.label map=T33UVR_20180506T100031_B01,T33UVR_20180521T100029_B01 semantic_label=S2_1,S2_1
```

### Assign different semantic labels to a list of raster maps

```sh
r.semantic.label map=T33UVR_20180506T100031_B01,T33UVR_20180506T100031_B02 semantic_label=S2_1,S2_2
```

### Remove semantic label from a list of raster maps

```sh
r.semantic.label map=T33UVR_20180506T100031_B01,T33UVR_20180506T100031_B02 operation=remove
```

### Print semantic label information about single raster map

```sh
r.semantic.label map=T33UVR_20180506T100031_B01 operation=print
```

### Print extended semantic label information for a list of raster map

```sh
r.semantic.label map=T33UVR_20180506T100031_B01,T33UVR_20180506T100031_B02 operation=print
```

## KNOWN ISSUES

*r.semantic.label* allows managing semantic labels only related to 2D
raster maps.

## SEE ALSO

*[i.band.library](i.band.library.md), [r.info](r.info.md),
[r.support](r.support.md)*

## AUTHORS

Martin Landa  
Development sponsored by [mundialis GmbH & Co.
KG](https://www.mundialis.de/en) (for the [openEO](https://openeo.org)
EU H2020 grant 776242)

## DESCRIPTION

*t.list* lists any dataset that is registered in the temporal database.
Datasets are raster, 3D raster and vector maps as well as their
corresponding space time datasets (STRDS, STR3DS and STVDS). The type of
the dataset can be specified using the *type* option, default is STRDS.
By default all datasets with relative and absolute time are listed.
However, the user has the ability to specify a single temporal type with
the *temporaltype* option. The user can define the columns that should
be printed for each dataset and the order of the datasets. In addition a
SQL WHERE statement can be specified to select a subset of the requested
datasets.

## NOTES

The SQL *where* and *sort* expression will be applied for each temporal
database that was found in accessible mapsets. Hence sorting works only
on mapset basis.

Temporal databases stored in other mapsets can be used as long as they
are in the user's current mapset search path (managed with
[g.mapsets](g.mapsets.md)).

## EXAMPLES

Obtain the list of space time raster dataset(s):

```sh
t.list strds
----------------------------------------------
Space time raster datasets with absolute time available in mapset <climate_2000_2012>:
tempmean_monthly@climate_2000_2012
```

Obtain the list of space time raster datasets in a specific mapset (Note
that the target mapset must be in the user's search path):

```sh
# strds in PERMANENT
t.list strds where="mapset = 'PERMANENT'"
----------------------------------------------

# strds in climate_2000_2012
t.list strds where="mapset = 'climate_2000_2012'"
----------------------------------------------
Space time raster datasets with absolute time available in mapset <climate_2000_2012>:
precip_abs@climate_2000_2012
precipitation@climate_2000_2012
tempmean@climate_2000_2012
```

The *where* option can also be used to list the stds with a certain
pattern in their name, i.e. as the pattern option in
[g.list](g.list.md).

```sh
# strds whose name start with "precip"
t.list type=strds where="name LIKE 'precip%'"
----------------------------------------------
Space time raster datasets with absolute time available in mapset <climate_1970_2012>:
precip_abs@climate_1970_2012
precipitation@climate_1970_2012
```

The user can also obtain the list of time stamped raster maps. These
maps might be registered in strds or not. The output of the following
command can vary according to the accessible mapsets specified through
[g.mapsets](g.mapsets.md).

```sh
t.list raster
Time stamped raster maps with absolute time available in mapset <climate_2000_2012>:
2009_01_tempmean@climate_2000_2012
2009_02_tempmean@climate_2000_2012
2009_03_tempmean@climate_2000_2012
...
2012_10_tempmean@climate_2000_2012
2012_11_tempmean@climate_2000_2012
2012_12_tempmean@climate_2000_2012
```

## SEE ALSO

*[g.list](g.list.md), [t.create](t.create.md), [t.info](t.info.md),
[t.rast.list](t.rast.list.md), [t.rast3d.list](t.rast3d.list.md),
[t.vect.list](t.vect.list.md)*

## AUTHOR

Sören Gebbert, Thünen Institute of Climate-Smart Agriculture

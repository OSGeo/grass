## DESCRIPTION

*t.rast.neighbors* performs [r.neighbors](r.neighbors.md) computations
on the maps of a space time raster dataset (STRDS). This module supports
the options that are available in [r.neighbors](r.neighbors.md).

The user must provide an input and an output space time raster dataset
and the basename of the resulting raster maps. The resulting STRDS will
have the same temporal resolution as the input dataset. With the **-e**
flag, resulting maps can be registered in an existing STRDS, that e.g.
may have been created with a previous run of *t.rast.neighbors*. All
maps will be processed using the current region settings unless the
**-r** flag is selected. In the latter case, the computaional region is
set to each raster map selected from the input STRDS.

The user can select a subset of the input space time raster dataset for
processing using a SQL WHERE statement or using the **region_relation**
for spatial selection of raster maps. For the spatial map selection the
current computational region is used, even when the **-r** flag is
given. The number of CPU's to be used for parallel processing can be
specified with the *nprocs* option to speedup the computation on
multi-core system.

Semantic labels are needed to relate output raster maps to input raster
maps. E.g. with *method=stddev*, the user needs to know the spatial
extent, the time stamp and the semantic label to determine which stddev
map corresponds to which input map.

## EXAMPLE

To smooth the maps contained in a space time dataset run:

```sh
t.rast.neighbors input=tempmean_monthly output=smooth_tempmean_monthly \
                 basename=tmean_smooth size=5 method=average nprocs=4

# show some info about the new space time dataset
t.info smooth_tempmean_monthly
 +-------------------- Space Time Raster Dataset -----------------------------+
 |                                                                            |
 +-------------------- Basic information -------------------------------------+
 | Id: ........................ smooth_tempmean_monthly@climate_2000_2012
 | Name: ...................... smooth_tempmean_monthly
 | Mapset: .................... climate_2000_2012
 | Creator: ................... lucadelu
 | Temporal type: ............. absolute
 | Creation time: ............. 2014-11-27 11:41:36.444579
 | Modification time:.......... 2014-11-27 11:41:39.978232
 | Semantic type:.............. mean
 +-------------------- Absolute time -----------------------------------------+
 | Start time:................. 2009-01-01 00:00:00
 | End time:................... 2013-01-01 00:00:00
 | Granularity:................ 1 month
 | Temporal type of maps:...... interval
 +-------------------- Spatial extent ----------------------------------------+
 | North:...................... 320000.0
 | South:...................... 10000.0
 | East:.. .................... 935000.0
 | West:....................... 120000.0
 | Top:........................ 0.0
 | Bottom:..................... 0.0
 +-------------------- Metadata information ----------------------------------+
 | Raster register table:...... raster_map_register_ea1c9a83524e41a784d72744b08c6107
 | North-South resolution min:. 500.0
 | North-South resolution max:. 500.0
 | East-west resolution min:... 500.0
 | East-west resolution max:... 500.0
 | Minimum value min:.......... -6.428905
 | Minimum value max:.......... 18.867296
 | Maximum value min:.......... 4.247691
 | Maximum value max:.......... 28.767953
 | Aggregation type:........... None
 | Number of registered maps:.. 48
 |
 | Title:
 | Monthly precipitation
 | Description:
 | Dataset with monthly precipitation
 | Command history:
 | # 2014-11-27 11:41:36
 | t.rast.neighbors input="tempmean_monthly"
 |     output="smooth_tempmean_monthly" basename="tmean_smooth" size="5"
 |     method="average" nprocs="4"
 |
 +----------------------------------------------------------------------------+


# now compare the values between the original and the smoothed dataset

t.rast.list input=smooth_tempmean_monthly columns=name,start_time,min,max
name|start_time|min|max
tmean_smooth_1|2009-01-01 00:00:00|-3.361714|7.409861
tmean_smooth_2|2009-02-01 00:00:00|-1.820261|7.986794
tmean_smooth_3|2009-03-01 00:00:00|2.912971|11.799684
...
tmean_smooth_46|2012-10-01 00:00:00|9.38767|18.709297
tmean_smooth_47|2012-11-01 00:00:00|1.785653|10.911189
tmean_smooth_48|2012-12-01 00:00:00|1.784212|11.983857

t.rast.list input=tempmean_monthly columns=name,start_time,min,max
name|start_time|min|max
2009_01_tempmean|2009-01-01 00:00:00|-3.380823|7.426054
2009_02_tempmean|2009-02-01 00:00:00|-1.820261|8.006386
2009_03_tempmean|2009-03-01 00:00:00|2.656992|11.819274
...
2012_10_tempmean|2012-10-01 00:00:00|9.070884|18.709297
2012_11_tempmean|2012-11-01 00:00:00|1.785653|10.911189
2012_12_tempmean|2012-12-01 00:00:00|1.761019|11.983857
```

## SEE ALSO

*[r.neighbors](r.neighbors.md),
[t.rast.aggregate.ds](t.rast.aggregate.ds.md),
[t.rast.extract](t.rast.extract.md), [t.info](t.info.md),
[g.region](g.region.md), [r.mask](r.mask.md)*

## AUTHOR

Sören Gebbert, Thünen Institute of Climate-Smart Agriculture

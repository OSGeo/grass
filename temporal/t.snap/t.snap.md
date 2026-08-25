## DESCRIPTION

*t.snap* is designed to convert time instances of maps into time
intervals or to create valid temporal topologies for space time
datasets. Raster, 3D raster and vector space time datasets are supported
with absolute and relative time.

This module "snaps" the end time of each registered map of a space time
dataset to the start time of the map that is the temporal nearest
neighbour in the future. Maps with equal time stamps are not modified
and must be removed or modified to create a valid temporal topology. In
case the last map in the space time dataset is a time instance, the
granularity of the space time dataset will be used to create the time
interval.

## EXAMPLE

A raster space time dataset will be create using precipitation maps for
2012 then using absolute time in a space time raster dataset using an
increment of one month. At the end we snap the created time instances
resulting in time intervals.

```sh
# Generate data

t.create type=strds temporaltype=absolute \
         output=precipitation_monthly \
         title="Monthly precipitation" \
         description="Dataset with monthly precipitation"

t.register type=raster input=precipitation_monthly \
           maps=`g.list type=raster pattern="2012*precip" sep=comma` \
           start=2012-01-01 increment="1 months"

# please take attention to "Temporal type of maps" value
t.info type=strds input=precipitation_monthly

 +-------------------- Space Time Raster Dataset -----------------------------+
 |                                                                            |
 +-------------------- Basic information -------------------------------------+
 | Id: ........................ precipitation_monthly@climate_2009_2012
 | Name: ...................... precipitation_monthly
 | Mapset: .................... climate_2009_2012
 | Creator: ................... lucadelu
 | Temporal type: ............. absolute
 | Creation time: ............. 2014-11-28 15:52:30.801148
 | Modification time:.......... 2014-11-28 15:53:18.430773
 | Semantic type:.............. mean
 +-------------------- Absolute time -----------------------------------------+
 | Start time:................. 2012-01-01 00:00:00
 | End time:................... 2012-12-01 00:00:00
 | Granularity:................ 1 month
 | Temporal type of maps:...... point
 +-------------------- Spatial extent ----------------------------------------+
 | North:...................... 320000.0
 | South:...................... 10000.0
 | East:.. .................... 935000.0
 | West:....................... 120000.0
 | Top:........................ 0.0
 | Bottom:..................... 0.0
 +-------------------- Metadata information ----------------------------------+
 | Raster register table:...... raster_map_register_282454f66ff5455299526ec3c1db7362
 | North-South resolution min:. 500.0
 | North-South resolution max:. 500.0
 | East-west resolution min:... 500.0
 | East-west resolution max:... 500.0
 | Minimum value min:.......... 0.0
 | Minimum value max:.......... 95.58169
 | Maximum value min:.......... 132.413284
 | Maximum value max:.......... 356.502949
 | Aggregation type:........... None
 | Number of registered maps:.. 12
 |
 | Title:
 | Monthly precipitation
 | Description:
 | Dataset with monthly precipitation
 | Command history:
 | # 2014-11-28 15:52:30
 | t.create type="strds" temporaltype="absolute"
 |     output="precipitation_monthly" title="Monthly precipitation"
 |     description="Dataset with monthly precipitation"
 | # 2014-11-28 15:53:18
 | t.register type="rast" input="precipitation_monthly"
 |     maps="2012_01_precip,2012_02_precip, ... ,2012_11_precip,2012_12_precip"
 |     start="2012-01-01" increment="1 months"
 |
 +----------------------------------------------------------------------------+


# you can see that end time is not set
t.rast.list input=precipitation_monthly

name|mapset|start_time|end_time
2012_01_precip|climate_2009_2012|2012-01-01 00:00:00|None
2012_02_precip|climate_2009_2012|2012-02-01 00:00:00|None
2012_03_precip|climate_2009_2012|2012-03-01 00:00:00|None
2012_04_precip|climate_2009_2012|2012-04-01 00:00:00|None
2012_05_precip|climate_2009_2012|2012-05-01 00:00:00|None
2012_06_precip|climate_2009_2012|2012-06-01 00:00:00|None
2012_07_precip|climate_2009_2012|2012-07-01 00:00:00|None
2012_08_precip|climate_2009_2012|2012-08-01 00:00:00|None
2012_09_precip|climate_2009_2012|2012-09-01 00:00:00|None
2012_10_precip|climate_2009_2012|2012-10-01 00:00:00|None
2012_11_precip|climate_2009_2012|2012-11-01 00:00:00|None
2012_12_precip|climate_2009_2012|2012-12-01 00:00:00|None



t.snap type=strds input=precipitation_monthly

# please take attention to "Temporal type of maps" value again
t.info type=strds input=precipitation_monthly

 +-------------------- Space Time Raster Dataset -----------------------------+
 |                                                                            |
 +-------------------- Basic information -------------------------------------+
 | Id: ........................ precipitation_monthly@climate_2009_2012
 | Name: ...................... precipitation_monthly
 | Mapset: .................... climate_2009_2012
 | Creator: ................... lucadelu
 | Temporal type: ............. absolute
 | Creation time: ............. 2014-11-28 15:52:30.801148
 | Modification time:.......... 2014-11-28 15:54:28.739905
 | Semantic type:.............. mean
 +-------------------- Absolute time -----------------------------------------+
 | Start time:................. 2012-01-01 00:00:00
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
 | Raster register table:...... raster_map_register_282454f66ff5455299526ec3c1db7362
 | North-South resolution min:. 500.0
 | North-South resolution max:. 500.0
 | East-west resolution min:... 500.0
 | East-west resolution max:... 500.0
 | Minimum value min:.......... 0.0
 | Minimum value max:.......... 95.58169
 | Maximum value min:.......... 132.413284
 | Maximum value max:.......... 356.502949
 | Aggregation type:........... None
 | Number of registered maps:.. 12
 |
 | Title:
 | Monthly precipitation
 | Description:
 | Dataset with monthly precipitation
 | Command history:
 | # 2014-11-28 15:52:30
 | t.create type="strds" temporaltype="absolute"
 |     output="precipitation_monthly" title="Monthly precipitation"
 |     description="Dataset with monthly precipitation"
 | # 2014-11-28 15:53:18
 | t.register type="rast" input="precipitation_monthly"
 |     maps="2012_01_precip,2012_02_precip, ... ,2012_11_precip,2012_12_precip"
 |     start="2012-01-01" increment="1 months"
 | # 2014-11-28 15:54:28
 | t.snap type="strds" input="precipitation_monthly"
 |
 +----------------------------------------------------------------------------+


# now instead end time is set

t.rast.list input=precipitation_daily

2012_01_precip|climate_2009_2012|2012-01-01 00:00:00|2012-02-01 00:00:00
2012_02_precip|climate_2009_2012|2012-02-01 00:00:00|2012-03-01 00:00:00
2012_03_precip|climate_2009_2012|2012-03-01 00:00:00|2012-04-01 00:00:00
2012_04_precip|climate_2009_2012|2012-04-01 00:00:00|2012-05-01 00:00:00
2012_05_precip|climate_2009_2012|2012-05-01 00:00:00|2012-06-01 00:00:00
2012_06_precip|climate_2009_2012|2012-06-01 00:00:00|2012-07-01 00:00:00
2012_07_precip|climate_2009_2012|2012-07-01 00:00:00|2012-08-01 00:00:00
2012_08_precip|climate_2009_2012|2012-08-01 00:00:00|2012-09-01 00:00:00
2012_09_precip|climate_2009_2012|2012-09-01 00:00:00|2012-10-01 00:00:00
2012_10_precip|climate_2009_2012|2012-10-01 00:00:00|2012-11-01 00:00:00
2012_11_precip|climate_2009_2012|2012-11-01 00:00:00|2012-12-01 00:00:00
2012_12_precip|climate_2009_2012|2012-12-01 00:00:00|2013-01-01 00:00:00
```

## SEE ALSO

*[t.shift](t.shift.md), [t.create](t.create.md),
[t.register](t.register.md)*

## AUTHOR

Sören Gebbert, Thünen Institute of Climate-Smart Agriculture

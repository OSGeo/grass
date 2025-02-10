## DESCRIPTION

*t.rast.gapfill* fills temporal gaps in space time raster datasets using
linear interpolation. Temporal all gaps will be detected in the input
space time raster dataset automatically. The predecessor and successor
maps of the gaps will be identified and used to linear interpolate the
raster map between them.

## NOTES

This module uses [r.series.interp](r.series.interp.md) to perform the
interpolation for each gap independently. Hence several interpolation
processes can be run in parallel.

Each gap is re-sampled by the space time raster dataset granularity.
Therefore several time stamped raster map layers will be interpolated if
the gap is larger than the STRDS granularity.

## EXAMPLES

In this example we will create 3 raster maps and register them in the
temporal database an then in the newly created space time raster
dataset. There are gaps of one and two day size between the raster maps.
The values of the maps are chosen so that the interpolated values can be
estimated. We expect one map with a value of 2 for the first gap and two
maps (values 3.666 and 4.333) for the second gap after interpolation.

```sh
r.mapcalc expression="map1 = 1"
r.mapcalc expression="map2 = 3"
r.mapcalc expression="map3 = 5"

t.register type=raster maps=map1 start=2012-08-20 end=2012-08-21
t.register type=raster maps=map2 start=2012-08-22 end=2012-08-23
t.register type=raster maps=map3 start=2012-08-25 end=2012-08-26

t.create type=strds temporaltype=absolute \
         output=precipitation_daily \
         title="Daily precipitation" \
         description="Test dataset with daily precipitation"

t.register type=raster input=precipitation_daily maps=map1,map2,map3

# the output shows three missing maps
t.rast.list input=precipitation_daily columns=name,start_time,min,max

name|start_time|min|max
map1|2012-08-20 00:00:00|1.0|1.0
map2|2012-08-22 00:00:00|3.0|3.0
map3|2012-08-25 00:00:00|5.0|5.0


t.rast.list input=precipitation_daily method=deltagaps

id|name|mapset|start_time|end_time|interval_length|distance_from_begin
map1@PERMANENT|map1|PERMANENT|2012-08-20 00:00:00|2012-08-21 00:00:00|1.0|0.0
None|None|None|2012-08-21 00:00:00|2012-08-22 00:00:00|1.0|1.0
map2@PERMANENT|map2|PERMANENT|2012-08-22 00:00:00|2012-08-23 00:00:00|1.0|2.0
None|None|None|2012-08-23 00:00:00|2012-08-25 00:00:00|2.0|3.0
map3@PERMANENT|map3|PERMANENT|2012-08-25 00:00:00|2012-08-26 00:00:00|1.0|5.0

t.rast.gapfill input=precipitation_daily basename=gap

t.rast.list input=precipitation_daily columns=name,start_time,min,max

name|start_time|min|max
map1|2012-08-20 00:00:00|1.0|1.0
gap_6_1|2012-08-21 00:00:00|2.0|2.0
map2|2012-08-22 00:00:00|3.0|3.0
gap_7_1|2012-08-23 00:00:00|3.666667|3.666667
gap_7_2|2012-08-24 00:00:00|4.333333|4.333333
map3|2012-08-25 00:00:00|5.0|5.0

```

## SEE ALSO

*[r.series.interp](r.series.interp.md), [t.create](t.create.md),
[t.info](t.info.md)*

## AUTHOR

Sören Gebbert, Thünen Institute of Climate-Smart Agriculture

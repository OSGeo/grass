## DESCRIPTION

*t.rast.aggregate* temporally aggregates space time raster datasets by a
specific temporal granularity. This module support *absolute* and
*relative time*. The temporal granularity of absolute time can be
*seconds, minutes, hours, days, weeks, months* or *years*. Mixing of
granularities eg. "1 year, 3 months 5 days" is not supported. In case of
relative time the temporal unit of the input space time raster dataset
is used. The granularity must be specified with an integer value.

This module is sensitive to the current region and mask settings, hence
spatial extent and spatial resolution. In case the registered raster
maps of the input space time raster dataset have different spatial
resolutions, the default nearest neighbor resampling method is used for
runtime spatial aggregation.

## NOTES

The raster module *r.series* is used internally. Hence all aggregate
methods of *r.series* are supported. See the [r.series](r.series.md)
manual page for details.

This module will shift the start date for each aggregation process
depending on the provided temporal granularity. The following shifts
will performed:

- *granularity years*: will start at the first of January, hence
  14-08-2012 00:01:30 will be shifted to 01-01-2012 00:00:00
- *granularity months*: will start at the first day of a month, hence
  14-08-2012 will be shifted to 01-08-2012 00:00:00
- *granularity weeks*: will start at the first day of a week (Monday),
  hence 14-08-2012 01:30:30 will be shifted to 13-08-2012 01:00:00
- *granularity days*: will start at the first hour of a day, hence
  14-08-2012 00:01:30 will be shifted to 14-08-2012 00:00:00
- *granularity hours*: will start at the first minute of a hour, hence
  14-08-2012 01:30:30 will be shifted to 14-08-2012 01:00:00
- *granularity minutes*: will start at the first second of a minute,
  hence 14-08-2012 01:30:30 will be shifted to 14-08-2012 01:30:00

The specification of the temporal relation between the aggregation
intervals and the raster map layers is always formulated from the
aggregation interval viewpoint. Hence, the relation *contains* has to be
specified to aggregate map layer that are temporally located in an
aggregation interval.

Parallel processing is supported in case that more than one interval is
available for aggregation computation. Internally several *r.series*
modules will be started, depending on the number of specified parallel
processes (*nprocs*) and the number of intervals to aggregate.

## EXAMPLES

### Aggregation of monthly data into yearly data

In this example the user is going to aggregate monthly data into yearly
data, running:

```sh
t.rast.aggregate input=tempmean_monthly output=tempmean_yearly \
                 basename=tempmean_year \
                 granularity="1 years" method=average

t.support input=tempmean_yearly \
          title="Yearly precipitation" \
          description="Aggregated precipitation dataset with yearly resolution"

t.info tempmean_yearly
 +-------------------- Space Time Raster Dataset -----------------------------+
 |                                                                            |
 +-------------------- Basic information -------------------------------------+
 | Id: ........................ tempmean_yearly@climate_2000_2012
 | Name: ...................... tempmean_yearly
 | Mapset: .................... climate_2000_2012
 | Creator: ................... lucadelu
 | Temporal type: ............. absolute
 | Creation time: ............. 2014-11-27 10:25:21.243319
 | Modification time:.......... 2014-11-27 10:25:21.862136
 | Semantic type:.............. mean
 +-------------------- Absolute time -----------------------------------------+
 | Start time:................. 2009-01-01 00:00:00
 | End time:................... 2013-01-01 00:00:00
 | Granularity:................ 1 year
 | Temporal type of maps:...... interval
 +-------------------- Spatial extent ----------------------------------------+
 | North:...................... 320000.0
 | South:...................... 10000.0
 | East:.. .................... 935000.0
 | West:....................... 120000.0
 | Top:........................ 0.0
 | Bottom:..................... 0.0
 +-------------------- Metadata information ----------------------------------+
 | Raster register table:...... raster_map_register_514082e62e864522a13c8123d1949dea
 | North-South resolution min:. 500.0
 | North-South resolution max:. 500.0
 | East-west resolution min:... 500.0
 | East-west resolution max:... 500.0
 | Minimum value min:.......... 7.370747
 | Minimum value max:.......... 8.81603
 | Maximum value min:.......... 17.111387
 | Maximum value max:.......... 17.915511
 | Aggregation type:........... average
 | Number of registered maps:.. 4
 |
 | Title: Yearly precipitation
 | Monthly precipitation
 | Description: Aggregated precipitation dataset with yearly resolution
 | Dataset with monthly precipitation
 | Command history:
 | # 2014-11-27 10:25:21
 | t.rast.aggregate input="tempmean_monthly"
 |     output="tempmean_yearly" basename="tempmean_year" granularity="1 years"
 |     method="average"
 |
 | # 2014-11-27 10:26:21
 | t.support input=tempmean_yearly \
 |        title="Yearly precipitation" \
 |        description="Aggregated precipitation dataset with yearly resolution"
 +----------------------------------------------------------------------------+
```

### Different aggregations and map name suffix variants

Examples of resulting naming schemes for different aggregations when
using the **suffix** option:

#### Weekly aggregation

```sh
t.rast.aggregate input=daily_temp output=weekly_avg_temp \
  basename=weekly_avg_temp method=average granularity="1 weeks"

t.rast.list weekly_avg_temp
name|mapset|start_time|end_time
weekly_avg_temp_2003_01|climate|2003-01-03 00:00:00|2003-01-10 00:00:00
weekly_avg_temp_2003_02|climate|2003-01-10 00:00:00|2003-01-17 00:00:00
weekly_avg_temp_2003_03|climate|2003-01-17 00:00:00|2003-01-24 00:00:00
weekly_avg_temp_2003_04|climate|2003-01-24 00:00:00|2003-01-31 00:00:00
weekly_avg_temp_2003_05|climate|2003-01-31 00:00:00|2003-02-07 00:00:00
weekly_avg_temp_2003_06|climate|2003-02-07 00:00:00|2003-02-14 00:00:00
weekly_avg_temp_2003_07|climate|2003-02-14 00:00:00|2003-02-21 00:00:00
```

Variant with **suffix** set to granularity:

```sh
t.rast.aggregate input=daily_temp output=weekly_avg_temp \
  basename=weekly_avg_temp suffix=gran method=average \
  granularity="1 weeks"

t.rast.list weekly_avg_temp
name|mapset|start_time|end_time
weekly_avg_temp_2003_01_03|climate|2003-01-03 00:00:00|2003-01-10 00:00:00
weekly_avg_temp_2003_01_10|climate|2003-01-10 00:00:00|2003-01-17 00:00:00
weekly_avg_temp_2003_01_17|climate|2003-01-17 00:00:00|2003-01-24 00:00:00
weekly_avg_temp_2003_01_24|climate|2003-01-24 00:00:00|2003-01-31 00:00:00
weekly_avg_temp_2003_01_31|climate|2003-01-31 00:00:00|2003-02-07 00:00:00
weekly_avg_temp_2003_02_07|climate|2003-02-07 00:00:00|2003-02-14 00:00:00
weekly_avg_temp_2003_02_14|climate|2003-02-14 00:00:00|2003-02-21 00:00:00
```

#### Monthly aggregation

```sh
t.rast.aggregate input=daily_temp output=monthly_avg_temp \
  basename=monthly_avg_temp suffix=gran method=average \
  granularity="1 months"

t.rast.list monthly_avg_temp
name|mapset|start_time|end_time
monthly_avg_temp_2003_01|climate|2003-01-01 00:00:00|2003-02-01 00:00:00
monthly_avg_temp_2003_02|climate|2003-02-01 00:00:00|2003-03-01 00:00:00
monthly_avg_temp_2003_03|climate|2003-03-01 00:00:00|2003-04-01 00:00:00
monthly_avg_temp_2003_04|climate|2003-04-01 00:00:00|2003-05-01 00:00:00
monthly_avg_temp_2003_05|climate|2003-05-01 00:00:00|2003-06-01 00:00:00
monthly_avg_temp_2003_06|climate|2003-06-01 00:00:00|2003-07-01 00:00:00
```

#### Yearly aggregation

```sh
t.rast.aggregate input=daily_temp output=yearly_avg_temp \
  basename=yearly_avg_temp suffix=gran method=average \
  granularity="1 years"

t.rast.list yearly_avg_temp
name|mapset|start_time|end_time
yearly_avg_temp_2003|climate|2003-01-01 00:00:00|2004-01-01 00:00:00
yearly_avg_temp_2004|climate|2004-01-01 00:00:00|2005-01-01 00:00:00
```

## SEE ALSO

*[t.rast.aggregate.ds](t.rast.aggregate.ds.md),
[t.rast.extract](t.rast.extract.md), [t.info](t.info.md),
[r.series](r.series.md), [g.region](g.region.md), [r.mask](r.mask.md)*

[Temporal data processing
Wiki](https://grasswiki.osgeo.org/wiki/Temporal_data_processing)

## AUTHOR

Sören Gebbert, Thünen Institute of Climate-Smart Agriculture

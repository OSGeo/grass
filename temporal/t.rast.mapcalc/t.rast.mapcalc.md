## DESCRIPTION

*t.rast.mapcalc* performs spatio-temporal *mapcalc* expressions on maps
of temporally sampled space time raster datasets (STRDS). Spatial and
temporal operators and internal variables are available in the
expression string. The description of the spatial operators, functions
and internal variables is available in the *[r.mapcalc](r.mapcalc.md)*
manual page. The temporal functions are described in detail below.

This module expects several parameters. All space time raster datasets
that are referenced in the *mapcalc expression* must be listed in the
**inputs** option. The *first* space time raster dataset that is listed
as input will be used to temporally sample all other space time raster
datasets. The temporal sampling method can be chosen using the
**method** option. The order of the STRDS's in the mapcalc expression
can be different to the order of the STRDS's in the input option. The
resulting space time raster dataset must be specified in the **output**
option together with the **basename** of generated raster maps that are
registered in the resulting STRDS. Empty maps resulting from
map-calculation are not registered by default. This behavior can be
changed with the **-n** flag. The flag **-s** can be used to assure that
only spatially related maps in the STRDS's are processed. Spatially
related means that temporally related maps overlap in their spatial
extent.

The module *t.rast.mapcalc* supports parallel processing. The option
**nprocs** specifies the number of processes that can be started in
parallel.

A mapcalc expression must be provided to process the temporal sampled
maps. Temporal internal variables are available in addition to the
*[r.mapcalc](r.mapcalc.md)* spatial operators and functions:

The supported internal variables for relative and absolute time are:

- *td()* - This internal variable represents the size of the current
  sample time interval in days and fraction of days for absolute time,
  and in relative units in case of relative time.
- *start_time()* - This internal variable represents the time difference
  between the start time of the sample space time raster dataset and the
  start time of the current sample interval or instance. The time is
  measured in days and fraction of days for absolute time, and in
  relative units in case of relative time.
- *end_time()* - This internal variable represents the time difference
  between the start time of the sample space time raster dataset and the
  end time of the current sample interval. The time is measured in days
  and fraction of days for absolute time, and in relative units in case
  of relative time. The end_time() will be represented by null() in case
  of a time instance.

The supported internal variables for the current sample interval or
instance for absolute time are:

- *start_doy()* - Day of year (doy) from the start time \[1 - 366\]
- *start_dow()* - Day of week (dow) from the start time \[1 - 7\], the
  start of the week is Monday == 1
- *start_year()* - The year of the start time \[0 - 9999\]
- *start_month()* - The month of the start time \[1 - 12\]
- *start_week()* - Week of year of the start time \[1 - 54\]
- *start_day()* - Day of month from the start time \[1 - 31\]
- *start_hour()* - The hour of the start time \[0 - 23\]
- *start_minute()* - The minute of the start time \[0 - 59\]
- *start_second()* - The second of the start time \[0 - 59\]
- *end_doy()* - Day of year (doy) from the end time \[1 - 366\]
- *end_dow()* - Day of week (dow) from the end time \[1 - 7\], the start
  of the week is Monday == 1
- *end_year()* - The year of the end time \[0 - 9999\]
- *end_month()* - The month of the end time \[1 - 12\]
- *end_woy()* - Week of year (woy) of the end time \[1 - 54\]
- *end_day()* - Day of month from the start time \[1 - 31\]
- *end_hour()* - The hour of the end time \[0 - 23\]
- *end_minute()* - The minute of the end time \[0 - 59\]
- *end_second()* - The second of the end time \[0 - 59\].

The *end\_\** functions are represented by the null() internal variable
in case of time instances.

## NOTES

We will discuss the internal work of *t.rast.mapcalc* with an example.
Imagine we have two STRDS as input, each one of monthly granularity. The
first one *A* has 6 raster maps (a3 ... a8) with a temporal range from
March to August. The second STRDS *B* has 12 raster maps (b1 ... b12)
ranging from January to December. The value of the raster maps is the
number of the month from their interval start time. Dataset *A* will be
used to sample dataset *B* to create a dataset *C*. We want to add all
maps with equal time stamps if the month of the start time is May or
June, otherwise we multiply the maps. The command will look as follows:

```sh
t.rast.mapcalc input=A,B output=C basename=c method=equal \
    expression="if(start_month() == 5 || start_month() == 6, (A + B), (A * B))"
```

The resulting raster maps in dataset C can be listed with
*[t.rast.list](t.rast.list.md)*:

```sh
name    start_time              min     max
c_1     2001-03-01 00:00:00     9.0     9.0
c_2     2001-04-01 00:00:00     16.0    16.0
c_3     2001-05-01 00:00:00     10.0    10.0
c_4     2001-06-01 00:00:00     12.0    12.0
c_5     2001-07-01 00:00:00     49.0    49.0
c_6     2001-08-01 00:00:00     64.0    64.0
```

Internally the spatio-temporal expression will be analyzed for each time
interval of the sample dataset A, the temporal functions will be
replaced by numerical values, the names of the space time raster
datasets will be replaced by the corresponding raster maps. The final
expression will be passed to *[r.mapcalc](r.mapcalc.md)*, resulting in 6
runs:

```sh
r.mapcalc expression="c_1 = if(3 == 5 || 3 == 6, (a3 + b3), (a3 * b3))"
r.mapcalc expression="c_2 = if(4 == 5 || 4 == 6, (a4 + b4), (a4 * b4))"
r.mapcalc expression="c_3 = if(5 == 5 || 5 == 6, (a5 + b5), (a5 * b5))"
r.mapcalc expression="c_4 = if(6 == 5 || 6 == 6, (a6 + b6), (a6 * b6))"
r.mapcalc expression="c_5 = if(7 == 5 || 7 == 6, (a7 + b7), (a7 * b7))"
r.mapcalc expression="c_6 = if(8 == 5 || 8 == 6, (a8 + b8), (a8 * b8))"
```

Semantic labels present in the sample dataset A will be transferred to
the output dataset.

## EXAMPLES

The following command creates a new space time raster dataset
`january_under_0` that will set to null all cells with temperature above
zero in the January maps while keeping all the rest as in the original
time series. This will change the maximum values of all January maps in
the new STRDS as compared to the original one, `tempmean_monthly`.

```sh
t.rast.mapcalc input=tempmean_monthly output=january_under_0 basename=january_under_0 \
    expression="if(start_month() == 1 && tempmean_monthly > 0, null(), tempmean_monthly)"

# print minimum and maximum only for January in the new strds
t.rast.list january_under_0 columns=name,start_time,min,max | grep 01-01
name|start_time|min|max
january_under_0_01|2009-01-01 00:00:00|-3.380823|-7e-06
january_under_0_13|2010-01-01 00:00:00|-5.266929|-0.000154
january_under_0_25|2011-01-01 00:00:00|-4.968747|-6.1e-05
january_under_0_37|2012-01-01 00:00:00|-0.534994|-0.014581

# print minimum and maximum only for January in the original strds,
# note that the maximum is different
t.rast.list tempmean_monthly columns=name,start_time,min,max | grep 01-01
2009_01_tempmean|2009-01-01 00:00:00|-3.380823|7.426054
2010_01_tempmean|2010-01-01 00:00:00|-5.266929|5.71131
2011_01_tempmean|2011-01-01 00:00:00|-4.968747|4.967295
2012_01_tempmean|2012-01-01 00:00:00|-0.534994|9.69511
```

### Semantic label filtering

*t.rast.mapcalc* supports semantic label filtering similarly to
*[t.rast.list](t.rast.list.md#filtering-the-result-by-semantic-label)*.
In example below a new STRDS will be created and filled by NDVI
products.

```sh
t.rast.mapcalc inputs=test.S2_8,test.S2_4 output=ndvi basename=ndvi \
     expression="float(test.S2_8 - test.S2_4) / (test.S2_8 + test.S2_4)"
```

For more information about semantic label concept see
*[i.band.library](i.band.library.md)* module.

## SEE ALSO

*[r.mapcalc](r.mapcalc.md), [t.register](t.register.md),
[t.rast.list](t.rast.list.md), [t.info](t.info.md)*

[Temporal data processing
Wiki](https://grasswiki.osgeo.org/wiki/Temporal_data_processing)

## AUTHOR

Sören Gebbert, Thünen Institute of Climate-Smart Agriculture

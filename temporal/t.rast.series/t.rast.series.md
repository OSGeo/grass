## DESCRIPTION

The input of this module is a single space time raster dataset, the
output is a single raster map layer. A subset of the input space time
raster dataset can be selected using the **where** option. The sorting
of the raster map layer can be set using the **order** option. Be aware
that the order of the maps can significantly influence the result of the
aggregation (e.g.: slope). By default the maps are ordered by
**start_time**.

*t.rast.series* is a simple wrapper for the raster module **r.series**.
It supports a subset of the aggregation methods of **r.series**.

## NOTES

To avoid problems with too many open files, by default, the maximum
number of open files is set to 1000. If the number of input raster files
exceeds this number, the **-z** flag will be invoked. Because this will
slow down processing, the user can set a higher limit with the
**file_limit** parameter. Note that **file_limit** limit should not
exceed the user-specific limit on open files set by your operating
system. See the
[Wiki](https://grasswiki.osgeo.org/wiki/Large_raster_data_processing#Number_of_open_files_limitation)
for more information.

## Performance

To enable parallel processing, the user can specify the number of
threads to be used with the **nprocs** parameter (default 1). The
**memory** parameter (default 300 MB) can also be provided to determine
the size of the buffer in MB for computation. Both parameters are passed
to [r.series](r.series.md). To take advantage of the parallelization,
GRASS GIS needs to be compiled with OpenMP enabled.

## EXAMPLES

### Estimate the average temperature for the whole time series

Here the entire stack of input maps is considered:

```sh
t.rast.series input=tempmean_monthly output=tempmean_average method=average
```

### Estimate the average temperature for a subset of the time series

Here the stack of input maps is limited to a certain period of time:

```sh
t.rast.series input=tempmean_daily output=tempmean_season method=average \
  where="start_time >= '2012-06' and start_time <= '2012-08'"
```

### Climatology: single month in a multi-annual time series

By considering only a single month in a multi-annual time series the
so-called climatology can be computed. Estimate average temperature for
all January maps in the time series:

```sh
t.rast.series input=tempmean_monthly \
    method=average output=tempmean_january \
    where="strftime('%m', start_time)='01'"

# equivalently, we can use
t.rast.series input=tempmean_monthly \
    output=tempmean_january method=average \
    where="start_time = datetime(start_time, 'start of year', '0 month')"

# if we want also February and March averages

t.rast.series input=tempmean_monthly \
    output=tempmean_february method=average \
    where="start_time = datetime(start_time, 'start of year', '1 month')"

t.rast.series input=tempmean_monthly \
    output=tempmean_march method=average \
    where="start_time = datetime(start_time, 'start of year', '2 month')"
```

Generalizing a bit, we can estimate monthly climatologies for all months
by means of different methods

```sh
for i in `seq -w 1 12` ; do
  for m in average stddev minimum maximum ; do
    t.rast.series input=tempmean_monthly method=${m} output=tempmean_${m}_${i} \
    where="strftime('%m', start_time)='${i}'"
  done
done
```

## SEE ALSO

*[r.series](r.series.md), [t.create](t.create.md), [t.info](t.info.md)*

[Temporal data processing
Wiki](https://grasswiki.osgeo.org/wiki/Temporal_data_processing)

## AUTHOR

Sören Gebbert, Thünen Institute of Climate-Smart Agriculture

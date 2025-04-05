## DESCRIPTION

The module *t.register* has double functionality: it either only assigns
timestamps to raster, 3D raster and vector maps in the temporal database
(if *input* option is not provided, see below) or additionally, it also
registers them within input space time datasets (stds). The existing
timestamp modules [r.timestamp](r.timestamp.md),
[r3.timestamp](r3.timestamp.md) and [v.timestamp](v.timestamp.md) do not
register the maps in the temporal database of GRASS GIS. However,
timestamps that have been created with these modules can be read and
used by *t.register*. This works only for maps that are not already
registered in the temporal database.

If the *input* option is not used (i.e., no stds is provided), maps will
be only registered in the temporal database with assigned timestamps.
If, on the other hand, the *input* option is used and a stds is
provided, maps will be first registered in the temporal database (if not
registered before) and then, in the stds specified. If the user wants to
register maps that are already registered in the temporal database in a
different stds, there is no need to pass information regarding start and
end time, *t.register* will read timestamps from the temporal database
(i.e., in this case only passing map names will be enough).

Maps can be specified both with their fully qualified map name, meaning
e.g. *map@mapset* and without the mapset name included. If the mapset
name is not provided in the input, *t.register* will look for the given
map on the current search path and assign the matching mapset. If the
map is not found on the current search path the module will fail. Thus,
registering maps with fully qualified map name is slightly faster.

The module *t.register* supports absolute and relative time. The
absolute temporal type refers to a fixed date while the relative
temporal type refers to data without fixed timestamps (e.g., sequential
maps used to calculate multi-decadal averages).

Maps can be registered by command line argument (i.e., a list of comma
separated map names) or using an input file. The start time, end time
and a temporal increment can be provided through command line or in the
input file. End time and increment are mutually exclusive. The user can
register single maps or a list of maps at once. Maps can be registered
in several space time datasets using the same timestamp. For the case of
vector time series, the user can also register a single vector map
connected to different layers representing time steps using the
**map:layer** notation (See example below).

The *increment* option and the **-i** flag (to create time intervals)
work only in conjunction with the **start** option. If an input file
with timestamps (either start time or start time and end time) is used,
then the *increment* option and the **-i** flag are not supported.

Start time and end time with absolute time must be provided using the
format **yyyy-mm-dd HH:MM:SS +HHMM**. It is also supported to specify
only the date **yyyy-mm-dd**. In case of relative time, the temporal
unit (years, months, days, hours, minutes or seconds) must be provided.
In this case, the relative start time, end time and increment are
integers.

## NOTES

The timestamps of registered maps will be stored in the temporal
database and in the metadata of the grass maps in the spatial database.
This assures that timestamps can always be accessed with
*(r\|r3\|v).timestamp* and the temporal modules. Timestamps should only
be modified with *t.register* because the *(r\|r3\|v).timestamp* modules
have no access to the temporal database.

## INPUT FILE FORMAT

There are several options to register maps by means of a file. The input
file consists of a list of map names, optionally along with timestamps.
Each map name (and timestamps if provided) should be stored in a new line
in this file.

When only map names are provided, the *increment* option and the **-i**
flag are supported. However, when along with map names any kind of
timestamp is provided, as well, the *increment* option and the **-i**
are no longer supported.

Specification of map names only (*increment* option and **-i** flag
supported):

```sh
terra_lst_day20020113
terra_lst_day20020114
terra_lst_day20020115
terra_lst_day20020116
terra_lst_day20020117
```

Specification of map names and absolute start time (date) of the time
instances (no support for *increment* option nor **-i** flag):

```sh
terra_lst_day20020113|2002-01-13
terra_lst_day20020114|2002-01-14
terra_lst_day20020115|2002-01-15
terra_lst_day20020116|2002-01-16
terra_lst_day20020117|2002-01-17
```

Specification of map names and absolute start time (datetime) of the
time instances (no support for *increment* option nor **-i** flag):

```sh
terra_lst_day20020113|2002-01-13 10:30
terra_lst_day20020114|2002-01-14 10:30
terra_lst_day20020115|2002-01-15 10:30
terra_lst_day20020116|2002-01-16 10:30
terra_lst_day20020117|2002-01-17 10:30
```

Specification of map names and absolute time interval with start and end
time (no support for *increment* option nor **-i** flag):

```sh
prec_1|2001-01-01|2001-04-01
prec_2|2001-04-01|2001-07-01
prec_3|2001-07-01|2001-10-01
prec_4|2001-10-01|2002-01-01
prec_5|2002-01-01|2002-04-01
prec_6|2002-04-01|2002-07-01
```

Same as above but with fully qualified map names (no support for
*increment* option nor **-i** flag):

```sh
prec_1@PERMANENT|2001-01-01|2001-04-01
prec_2@PERMANENT|2001-04-01|2001-07-01
prec_3@PERMANENT|2001-07-01|2001-10-01
prec_4@PERMANENT|2001-10-01|2002-01-01
prec_5@PERMANENT|2002-01-01|2002-04-01
prec_6@PERMANENT|2002-04-01|2002-07-01
```

### Support for semantic labels

For more information about semantic labels and image collections see
*[i.band.library](i.band.library.md)* module.

Specification of map names and absolute start time (datetime) of the
time instances. The last column indicates related semantic label.

```sh
T33UYP_20190331T094039_B01|2019-03-31 09:40:39|S2_1
T33UYP_20190331T094039_B10|2019-03-31 09:40:39|S2_10
T33UYP_20190331T094039_B02|2019-03-31 09:40:39|S2_2
T33UYP_20190331T094039_B05|2019-03-31 09:40:39|S2_5
T33UYP_20190331T094039_B11|2019-03-31 09:40:39|S2_11
T33UYP_20190331T094039_B08|2019-03-31 09:40:39|S2_8
T33UYP_20190331T094039_B12|2019-03-31 09:40:39|S2_12
T33UYP_20190331T094039_B8A|2019-03-31 09:40:39|S2_8A
T33UYP_20190331T094039_B06|2019-03-31 09:40:39|S2_6
T33UYP_20190331T094039_B04|2019-03-31 09:40:39|S2_4
T33UYP_20190331T094039_B03|2019-03-31 09:40:39|S2_3
T33UYP_20190331T094039_B09|2019-03-31 09:40:39|S2_9
```

In this case *t.register* assigns to given raster maps a semantic label
similarly as *[r.semantic.label](r.semantic.label.md)* does. Such
registered raster maps is possible to [filter by a semantic
label](t.rast.list.md#filtering-the-result-by-semantic-label).

Please note that raster maps with semantic labels assigned can be
registered only in STRDS created in TGIS DB version 3 or higher. *Older
versions of TGIS DB are not supported.* TGIS DB version can be checked
*[t.connect](t.connect.md)* module.

## EXAMPLE

### North Carolina dataset

#### Using a text file

Register maps in an absolute space time dataset, creating a time
interval

```sh
# first:  prepare a text file with a list of input maps (see above)
# second: register maps
t.register -i type=raster input=precipitation_monthly \
    file=list_of_input_maps.txt start="2009-01-01" \
    increment="1 months"
```

#### Using *g.list* to generate the input

Register maps in an absolute space time dataset, creating a time
interval

```sh
t.register -i type=raster input=precipitation_monthly \
    maps=`g.list raster pattern="*precip*" sep=comma` start="2009-01-01" \
    increment="1 months"
```

#### Register a vector map with layers representing time steps

Assume a vector map of points that represent meteorological stations and
it is connected to different layers depicting daily time steps. In this
example, only the fifth layer of the vector map will be registered.

```sh
# the layer is specified behind the colon
t.register type=vector input=meteo_stations_nc_daily \
    maps=meteo_stations_nc:5 start="2009-01-05"
```

### Synthetic maps

In this example we create 6 raster maps that will be registered in a
single space time raster dataset named precip_abs using a monthly
temporal granularity. The **-i** flag generates time intervals of the
provided *increment*. The generated timestamps will be inspected using
*r.timestamp* and *t.rast.list*. We will register an additional map with
a timestamp that was set with *r.timestamp*.

```sh
r.mapcalc expression="prec_1 = 100"
r.mapcalc expression="prec_2 = 200"
r.mapcalc expression="prec_3 = 300"
r.mapcalc expression="prec_4 = 400"
r.mapcalc expression="prec_5 = 500"
r.mapcalc expression="prec_6 = 600"

t.create type=strds temporaltype=absolute \
    output=precip_abs title="Example" \
    descr="Example"

t.register -i type=raster input=precip_abs \
    maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6 \
    start="2001-01-01" increment="1 months"

r.timestamp prec_1
1 Jan 2001 00:00:00 / 1 Feb 2001 00:00:00

r.timestamp prec_2
1 Feb 2001 00:00:00 / 1 Mar 2001 00:00:00

t.rast.list input=precip_abs

name|mapset|start_time|end_time
prec_1|PERMANENT|2001-01-01 00:00:00|2001-02-01 00:00:00
prec_2|PERMANENT|2001-02-01 00:00:00|2001-03-01 00:00:00
prec_3|PERMANENT|2001-03-01 00:00:00|2001-04-01 00:00:00
prec_4|PERMANENT|2001-04-01 00:00:00|2001-05-01 00:00:00
prec_5|PERMANENT|2001-05-01 00:00:00|2001-06-01 00:00:00
prec_6|PERMANENT|2001-06-01 00:00:00|2001-07-01 00:00:00

r.mapcalc expression="prec_7 = 700"
r.timestamp map=prec_7 date="1 jul 2001 / 1 aug 2001"

t.register type=raster input=precip_abs maps=prec_7

t.rast.list input=precip_abs

name|mapset|start_time|end_time
prec_1|PERMANENT|2001-01-01 00:00:00|2001-02-01 00:00:00
prec_2|PERMANENT|2001-02-01 00:00:00|2001-03-01 00:00:00
prec_3|PERMANENT|2001-03-01 00:00:00|2001-04-01 00:00:00
prec_4|PERMANENT|2001-04-01 00:00:00|2001-05-01 00:00:00
prec_5|PERMANENT|2001-05-01 00:00:00|2001-06-01 00:00:00
prec_6|PERMANENT|2001-06-01 00:00:00|2001-07-01 00:00:00
prec_7|PERMANENT|2001-07-01 00:00:00|2001-08-01 00:00:00
```

### Importing and registering ECA&D climatic data

The European Climate Assessment & Dataset (ECA&D) project offers the
E-OBS dataset which is a daily gridded observational dataset for
precipitation, temperature and sea level pressure in Europe based on
ECA&D information. Download and decompress mean temperature data from:
[here](https://surfobs.climate.copernicus.eu/dataaccess/access_eobs.php#datafiles).

```sh
# import E-OBS V12 into a lat-long project (alternatively, use r.external)
r.in.gdal -oe input=tg_0.25deg_reg_1950-1964_v12.0.nc \
  output=temperature_mean offset=0
r.in.gdal -oe input=tg_0.25deg_reg_1965-1979_v12.0.nc \
  output=temperature_mean offset=5479 --o
r.in.gdal -oe input=tg_0.25deg_reg_1980-1994_v12.0.nc \
  output=temperature_mean offset=10957 --o
r.in.gdal -oe input=tg_0.25deg_reg_1995-2015_v12.0.nc \
  output=temperature_mean offset=16436 --o

# create STRDS
t.create type=strds output=temperature_mean_1950_2015_daily \
  temporaltype=absolute semantictype=mean \
  title="European mean temperature 1950-2015" \
  description="The European daily mean temperature from ECAD"

# create text file with all temperature_mean rasters, one per line,
# a) using a shell script
for i in `seq 1 23922` ; do
    echo temperature_mean.$i >> map_list.txt
done

# b) using a Python script
file = open("map_list.txt", "w")
for i in range(23922):
    file.write("temperature_mean.%i\n" % (i + 1))
file.close()

# register daily maps using the file created above
t.register -i type=raster input=temperature_mean_1950_2015_daily \
              file=map_list.txt start="1950-01-01" increment="1 days"
```

## SEE ALSO

*[r.timestamp](r.timestamp.md), [t.create](t.create.md),
[t.info](t.info.md)*

[Maps registration examples in Temporal data processing
Wiki](https://grasswiki.osgeo.org/wiki/Temporal_data_processing/maps_registration)

## AUTHOR

Sören Gebbert, Thünen Institute of Climate-Smart Agriculture

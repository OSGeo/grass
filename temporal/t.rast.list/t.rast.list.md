## DESCRIPTION

List time stamped raster map layers that are registered in a space time
raster dataset. *t.rast.list* provides several options to list map
layers and their metadata. Listing of map layer can be ordered by
metadata, metadata columns can be specified and SQL where conditions can
be provided to select a map layer subset of the input space time raster
dataset. Most of the raster map specific metadata is available for
column selection, sorting and SQL where statements. Using the **method**
option allows the specification of different methods to list map layers.
Method *list* is the default option and sensitive to the
**column**,**order** and **where** options.

To print interval length in days and distance from the begin use method
*delta*. Method *deltagap* will additionally print temporal gaps between
map layer. The *gran* method allows the listing of map layer sampled by
a user defined **granule**. As default the granularity of the space time
raster dataset is used for sampling.

While method *list* supports all columns except for interval_length and
distance_from_begin, methods *delta*, *deltagap*, and *gran* support
only the following columns: id, name, mapset, start_time, end_time,
interval_length, and distance_from_begin. The option **order** is only
available with method *list*.

Methods *cols* and *comma* are depreciated. The *cols* method is
replaced by the *plain* format and the *comma* method is replaced by the
*line* format.

The **format** option specifies the format of the output data. The
default *plain* format will simply print user specified metadata columns
of one map layer per line separated by a pipe by default. The *line*
format will list fully qualified map names (name and mapset) as a
comma-separated list of values that can be used as input for spatial
modules. The *csv* format will print data in the CSV format using comma
as the value separator (delimiter) and double quote for text field
quoting. The *json* format generates JSON and, if the PyYAML package is
installed, The *yaml* format generates YAML. The column (or item)
separator can be specified with the **separator** option for *plain*,
*line*, and *csv*.

## EXAMPLES

This example shows several options that are available for map layers
listing.

### Default query

The following command is the default one, returning standard information
like name, mapset, start_time, end_time of each map in the space time
dataset

```sh
t.rast.list tempmean_monthly
name|mapset|start_time|end_time
2009_01_tempmean|climate_2000_2012|2009-01-01 00:00:00|2009-02-01 00:00:00
2009_02_tempmean|climate_2000_2012|2009-02-01 00:00:00|2009-03-01 00:00:00
....
2012_11_tempmean|climate_2000_2012|2012-11-01 00:00:00|2012-12-01 00:00:00
2012_12_tempmean|climate_2000_2012|2012-12-01 00:00:00|2013-01-01 00:00:00
```

### Add more info

The following command let the user to choose the columns to show

```sh
t.rast.list tempmean_monthly columns=name,start_time,min,max
name|start_time|min|max
2009_01_tempmean|2009-01-01 00:00:00|-3.380823|7.426054
2009_02_tempmean|2009-02-01 00:00:00|-1.820261|8.006386
...
2009_01_tempmean|2009-01-01 00:00:00|-3.380823|7.426054
2009_02_tempmean|2009-02-01 00:00:00|-1.820261|8.006386
```

### Filtering the result by value

In this example the result is filtered showing only the maps with max
value major than 24

```sh
t.rast.list tempmean_monthly columns=name,start_time,min,max where="max > 24"
name|start_time|min|max
2009_06_tempmean|2009-06-01 00:00:00|15.962669|25.819681
2009_07_tempmean|2009-07-01 00:00:00|15.32852|26.103664
2009_08_tempmean|2009-08-01 00:00:00|16.37995|27.293282
....
2012_06_tempmean|2012-06-01 00:00:00|14.929379|24.000651
2012_07_tempmean|2012-07-01 00:00:00|18.455802|28.794653
2012_08_tempmean|2012-08-01 00:00:00|15.718526|26.151115
```

### Filtering the result by time range

In this example the result is filtered showing only the maps which fall
into a specified time range (from .. to):

```sh
t.rast.list tempmean_monthly columns=name,start_time,min,max \
  where="start_time > '2009-06-01 00:00:00' and start_time < '2012-08-01 00:00:00'"
name|start_time|min|max
2009_06_tempmean|2009-06-01 00:00:00|15.962669|25.819681
2009_07_tempmean|2009-07-01 00:00:00|15.32852|26.103664
2009_08_tempmean|2009-08-01 00:00:00|16.37995|27.293282
....
2012_06_tempmean|2012-06-01 00:00:00|14.929379|24.000651
2012_07_tempmean|2012-07-01 00:00:00|18.455802|28.794653
2012_08_tempmean|2012-08-01 00:00:00|15.718526|26.151115
```

### Filtering the result by selecting recurring timestamps

In this example the result is filtered showing only the maps which fall
into a specified recurring time range (here one month per year):

```sh
t.rast.list Tseasonal_fieldata_garda where="strftime('%m', start_time)='06'"
```

### Using method option

Method option is able to show raster in different way. By default *cols*
value is used, the value *comma* will print only the list of maps inside
the space time dataset:

```sh
t.rast.list method=comma input=tempmean_monthly
2009_01_tempmean@climate_2009_2012,2009_02_tempmean@climate_2009_2012,2009_03_tempmean@climate_2009_2012, \
2009_04_tempmean@climate_2009_2012,2009_05_tempmean@climate_2009_2012,2009_06_tempmean@climate_2009_2012, \
2009_07_tempmean@climate_2009_2012,2009_08_tempmean@climate_2009_2012,2009_09_tempmean@climate_2009_2012, \
2009_10_tempmean@climate_2009_2012,2009_11_tempmean@climate_2009_2012,2009_12_tempmean@climate_2009_2012, \
2010_01_tempmean@climate_2009_2012,2010_02_tempmean@climate_2009_2012,2010_03_tempmean@climate_2009_2012, \
2010_04_tempmean@climate_2009_2012,2010_05_tempmean@climate_2009_2012,2010_06_tempmean@climate_2009_2012, \
2010_07_tempmean@climate_2009_2012,2010_08_tempmean@climate_2009_2012,2010_09_tempmean@climate_2009_2012, \
2010_10_tempmean@climate_2009_2012,2010_11_tempmean@climate_2009_2012,2010_12_tempmean@climate_2009_2012, \
2011_01_tempmean@climate_2009_2012,2011_02_tempmean@climate_2009_2012,2011_03_tempmean@climate_2009_2012, \
2011_04_tempmean@climate_2009_2012,2011_05_tempmean@climate_2009_2012,2011_06_tempmean@climate_2009_2012, \
2011_07_tempmean@climate_2009_2012,2011_08_tempmean@climate_2009_2012,2011_09_tempmean@climate_2009_2012, \
2011_10_tempmean@climate_2009_2012,2011_11_tempmean@climate_2009_2012,2011_12_tempmean@climate_2009_2012, \
2012_01_tempmean@climate_2009_2012,2012_02_tempmean@climate_2009_2012,2012_03_tempmean@climate_2009_2012, \
2012_04_tempmean@climate_2009_2012,2012_05_tempmean@climate_2009_2012,2012_06_tempmean@climate_2009_2012, \
2012_07_tempmean@climate_2009_2012,2012_08_tempmean@climate_2009_2012,2012_09_tempmean@climate_2009_2012, \
2012_10_tempmean@climate_2009_2012,2012_11_tempmean@climate_2009_2012,2012_12_tempmean@climate_2009_2012
```

The *delta* value calculate the interval between maps and the distance
from the first map:

```sh
t.rast.list method=delta input=tempmean_monthly
id|name|mapset|start_time|end_time|interval_length|distance_from_begin
2009_01_tempmean@climate_2000_2012|2009_01_tempmean|climate_2000_2012|2009-01-01 00:00:00|2009-02-01 00:00:00|31.0|0.0
2009_02_tempmean@climate_2000_2012|2009_02_tempmean|climate_2000_2012|2009-02-01 00:00:00|2009-03-01 00:00:00|28.0|31.0
2009_03_tempmean@climate_2000_2012|2009_03_tempmean|climate_2000_2012|2009-03-01 00:00:00|2009-04-01 00:00:00|31.0|59.0
...
2012_10_tempmean@climate_2000_2012|2012_10_tempmean|climate_2000_2012|2012-10-01 00:00:00|2012-11-01 00:00:00|31.0|1369.0
2012_11_tempmean@climate_2000_2012|2012_11_tempmean|climate_2000_2012|2012-11-01 00:00:00|2012-12-01 00:00:00|30.0|1400.0
2012_12_tempmean@climate_2000_2012|2012_12_tempmean|climate_2000_2012|2012-12-01 00:00:00|2013-01-01 00:00:00|31.0|1430.0
```

The *gran* value it is used to return data sampled by a user defined
granule. As default the granularity of the space time raster dataset is
used for sampling.

```sh
t.rast.list  method=gran input=tempmean_monthly
id|name|mapset|start_time|end_time|interval_length|distance_from_begin
2009_01_tempmean@climate_2009_2012|2009_01_tempmean|climate_2009_2012|2009-01-01 00:00:00|2009-02-01 00:00:00|31.0|0.0
2009_02_tempmean@climate_2009_2012|2009_02_tempmean|climate_2009_2012|2009-02-01 00:00:00|2009-03-01 00:00:00|28.0|31.0
2009_03_tempmean@climate_2009_2012|2009_03_tempmean|climate_2009_2012|2009-03-01 00:00:00|2009-04-01 00:00:00|31.0|59.0
2009_04_tempmean@climate_2009_2012|2009_04_tempmean|climate_2009_2012|2009-04-01 00:00:00|2009-05-01 00:00:00|30.0|90.0
....
2012_09_tempmean@climate_2009_2012|2012_09_tempmean|climate_2009_2012|2012-09-01 00:00:00|2012-10-01 00:00:00|30.0|1339.0
2012_10_tempmean@climate_2009_2012|2012_10_tempmean|climate_2009_2012|2012-10-01 00:00:00|2012-11-01 00:00:00|31.0|1369.0
2012_11_tempmean@climate_2009_2012|2012_11_tempmean|climate_2009_2012|2012-11-01 00:00:00|2012-12-01 00:00:00|30.0|1400.0
2012_12_tempmean@climate_2009_2012|2012_12_tempmean|climate_2009_2012|2012-12-01 00:00:00|2013-01-01 00:00:00|31.0|1430.0
```

```sh
t.rast.list  method=gran input=tempmean_monthly gran="2 months"
id|name|mapset|start_time|end_time|interval_length|distance_from_begin
2009_01_tempmean@climate_2009_2012|2009_01_tempmean|climate_2009_2012|2009-01-01 00:00:00|2009-03-01 00:00:00|59.0|0.0
2009_03_tempmean@climate_2009_2012|2009_03_tempmean|climate_2009_2012|2009-03-01 00:00:00|2009-05-01 00:00:00|61.0|59.0
2009_05_tempmean@climate_2009_2012|2009_05_tempmean|climate_2009_2012|2009-05-01 00:00:00|2009-07-01 00:00:00|61.0|120.0
....
2012_07_tempmean@climate_2009_2012|2012_07_tempmean|climate_2009_2012|2012-07-01 00:00:00|2012-09-01 00:00:00|62.0|1277.0
2012_09_tempmean@climate_2009_2012|2012_09_tempmean|climate_2009_2012|2012-09-01 00:00:00|2012-11-01 00:00:00|61.0|1339.0
2012_11_tempmean@climate_2009_2012|2012_11_tempmean|climate_2009_2012|2012-11-01 00:00:00|2013-01-01 00:00:00|61.0|1400.0
```

For the *deltagaps* value you can see the example for space time vector
dataset [t.vect.list](t.vect.list.md#using-method-option)

### Reading raster names in Python

```python
result = json.loads(
    gs.read_command(
        "t.rast.list", input="tempmean_monthly", format="json"
    )
)
for item in result["data"]:
    print(item["name"])
```

### Filtering the result by semantic label

Semantic label can be assigned to raster maps by
*[r.semantic.label](r.semantic.label.md)* module or even when
registering raster maps into STRDS by
*[t.register](t.register.md#support-for-semantic-labels)*.

Name of STRDS can be extended by semantic label used for filtering. Name
of STRDS and semantic label is split by a *single dot*.

```sh
t.rast.list input=test.S2_1
```

Note that semantic label filtering is *supported by all temporal
modules*.

Also note that only STRDS can be filtered by semantic label, see
*[r.semantic.label](r.semantic.label.md#known-issues)* for current
limitations.

## SEE ALSO

*[g.list](g.list.md), [t.create](t.create.md), [t.info](t.info.md),
[t.list](t.list.md), [t.rast3d.list](t.rast3d.list.md),
[t.vect.list](t.vect.list.md)*

[Temporal data processing
Wiki](https://grasswiki.osgeo.org/wiki/Temporal_data_processing)

## AUTHOR

Sören Gebbert, Thünen Institute of Climate-Smart Agriculture

## DESCRIPTION

*t.info* reports information about any dataset that is registered in the
temporal database. Datasets are raster, 3D raster and vector maps as well
as their corresponding space time datasets (STRDS, STR3DS and STVDS).
This module reports the information that is stored in the temporal
database. These are basic information (id, name, mapset, creator,
creation time, temporal type), the temporal and spatial extent and
dataset type specific metadata. The user has to utilize *r.info*,
*r3.info*, *v.info* to report detailed information about raster,
3D raster and vector maps, since not all map specific information and
metadata are stored in the temporal database.

In addition, information about the chosen temporal database backend can
be reported using the **-d** flag. The **-h** flag reports the history of
the Space time dataset.

Output can be printed in different **format**s:

- *plain*: human readable, plain text format
- *shell*: shell script style with key value pairs separated by "="
- *json*: JSON format

While both *shell* and *json* format are machine readable, there are some
notable differences: shell format has singel quotes around *granularity*,
and datetime-strings in order to assist parsing on the command line. It
returns a None string for empty fields. JSON-format returns key-value-pairs
in valid JSON. Furthermore, JSON-format returns a comprehensive set of
metadata, including also the metadata fields: *proj*, *title*, *command*,
and *description* as well as information about the *tgis_db*. So, JSON
also covers the **-d** and **-h** flag.

## NOTES

Temporal databases stored in other mapsets can be used as long as they
are in the user's current mapset search path (managed with
[g.mapsets](g.mapsets.md)).

## EXAMPLES

### Temporal DBMI information

In order to obtain information about temporal DBMI backend, run:

```sh
t.info -d
 +------------------- Temporal DBMI backend information ----------------------+
 | DBMI Python interface:...... sqlite3
 | Temporal database string:... /grassdata/nc_spm_temporal_workshop/climate_2000_2012/tgis/sqlite.db
 | SQL template path:.......... /usr/local/grass-7.0.0/etc/sql
 | tgis_db_version .......... 2
 | creation_time .......... 2014-11-22 20:06:46.863733
 | tgis_version .......... 2
 +----------------------------------------------------------------------------+
```

### Space time dataset information in JSON format

In order to obtain comprehensive information about a space time dataset
in JSON format, run:

```sh
t.info input=tempmean_monthly format=json
{
    "mapset": "climate_2000_2012",
    "id": "tempmean_monthly@climate_2000_2012",
    "name": "tempmean_monthly",
    "creator": "stefan.blumentrath",
    "creation_time": "2014-11-27 08:50:48.443229",
    "temporal_type": "absolute",
    "semantic_type": "mean",
    "modification_time": "2014-11-27 09:44:32.800282",
    "start_time": "2009-01-01 00:00:00",
    "end_time": "2022-01-01 00:00:00",
    "granularity": "1 month",
    "map_time": "interval",
    "north": 320000.0,
    "south": 10000.0,
    "east": 935000.0,
    "west": 120000.0,
    "top": 0.0,
    "bottom": 0.0,
    "proj": "XY",
    "title": "Monthly precipitation",
    "description": "Dataset with monthly precipitation",
    "command": "# 2014-11-27 08:50:48\nt.create type="strds" ...\nt.register ...",
    "number_of_maps": 156,
    "min_max": 18.838764,
    "max_max": 29.513596,
    "min_min": -6.464337,
    "max_min": 4.247691,
    "nsres_min": 500.0,
    "nsres_max": 500.0,
    "ewres_min": 500.0,
    "ewres_max": 500.0,
    "aggregation_type": null,
    "number_of_semantic_labels": 0,
    "raster_register": "raster_map_register_d567423784c740bea1fba75dc7c0fa3d",
    "semantic_labels": null,
    "tgis_db": {
        "dbmi_python_interface": "sqlite3",
        "dbmi_string": "/grassdata/nc_spm_temporal_workshop/climate_2000_2012/tgis/sqlite.db",
        "sql_template_path": "/usr/local/grass-7.0.0/etc/sql",
        "tgis_version": 2,
        "tgis_db_version": 2,
        "creation_time": "2014-11-22 20:06:46.863733"
    }
}
```

The "granularity" is the smallest gap size between the found time
instances, i.e. it the greatest common divisor between all gaps in the
time series.

### Space time dataset information

In order to obtain information about a space time dataset, run:

```sh
t.info input=tempmean_monthly
 +-------------------- Space Time Raster Dataset -----------------------------+
 |                                                                            |
 +-------------------- Basic information -------------------------------------+
 | Id: ........................ tempmean_monthly@climate_2000_2012
 | Name: ...................... tempmean_monthly
 | Mapset: .................... climate_2000_2012
 | Creator: ................... lucadelu
 | Temporal type: ............. absolute
 | Creation time: ............. 2014-11-27 08:50:48.443229
 | Modification time:.......... 2014-11-27 09:44:32.800282
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
 | Raster register table:...... raster_map_register_d567423784c740bea1fba75dc7c0fa3d
 | North-South resolution min:. 500.0
 | North-South resolution max:. 500.0
 | East-west resolution min:... 500.0
 | East-west resolution max:... 500.0
 | Minimum value min:.......... -6.464337
 | Minimum value max:.......... 18.54137
 | Maximum value min:.......... 4.247691
 | Maximum value max:.......... 28.805381
 | Aggregation type:........... None
 | Number of registered maps:.. 48
 |
 | Title:
 | Monthly precipitation
 | Description:
 | Dataset with monthly precipitation
 | Command history:
 | # 2014-11-27 08:50:48
 | t.create type="strds" temporaltype="absolute"
 |     output="tempmean_monthly" title="Monthly precipitation"
 |     description="Dataset with monthly precipitation"
 | # 2014-11-27 09:44:32
 | t.register -i type="rast" input="tempmean_monthly" maps="2009_01_tempmean,...,2012_12_tempmean" start="2009-01-01" increment="1 months"
 |
 +----------------------------------------------------------------------------+
```

The "granularity" is the smallest gap size between the found time
instances, i.e. it the greatest common divisor between all gaps in the
time series.

### Temporal maps information

In order to obtain information about a map in a space time dataset, run:

```sh
t.info input=2009_01_tempmean type=raster
 +-------------------- Raster Dataset ----------------------------------------+
 |                                                                            |
 +-------------------- Basic information -------------------------------------+
 | Id: ........................ 2009_01_tempmean@climate_2000_2012
 | Name: ...................... 2009_01_tempmean
 | Mapset: .................... climate_2000_2012
 | Creator: ................... lucadelu
 | Temporal type: ............. absolute
 | Creation time: ............. 2014-11-27 09:44:26.280147
 +-------------------- Absolute time -----------------------------------------+
 | Start time:................. 2009-01-01 00:00:00
 | End time:................... 2009-02-01 00:00:00
 +-------------------- Spatial extent ----------------------------------------+
 | North:...................... 320000.0
 | South:...................... 10000.0
 | East:.. .................... 935000.0
 | West:....................... 120000.0
 | Top:........................ 0.0
 | Bottom:..................... 0.0
 +-------------------- Metadata information ----------------------------------+
 | Datatype:................... DCELL
 | Number of columns:.......... 620
 | Number of rows:............. 1630
 | Number of cells:............ 1010600
 | North-South resolution:..... 500.0
 | East-west resolution:....... 500.0
 | Minimum value:.............. -3.380823
 | Maximum value:.............. 7.426054
 | Registered datasets ........ tempmean_monthly@climate_2000_2012
 +----------------------------------------------------------------------------+
```

### Space time dataset with semantic labels assigned

This information is printed only when semantic labels have been assigned
to registered raster maps by *[r.semantic.label](r.semantic.label.md)*
or *[t.register](t.register.md#support-for-semantic-labels)* module.

```sh
t.info input=test
...
+-------------------- Metadata information ----------------------------------+
...
 | Number of registered bands:. 13
...
```

Similarly for temporal maps information:

```sh
t.info input=T33UYP_20190331T094039_B01 type=raster
...
 +-------------------- Metadata information ----------------------------------+
 | Semantic label:............. S2_1
...
```

## SEE ALSO

*[t.create](t.create.md), [t.list](t.list.md),
[t.register](t.register.md), [r.info](r.info.md), [r3.info](r3.info.md),
[v.info](v.info.md)*

## AUTHOR

Sören Gebbert, Thünen Institute of Climate-Smart Agriculture

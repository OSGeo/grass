## DESCRIPTION

*t.rast.univar* calculates univariate statistics from the non-null cells
for each registered raster map of a space time raster dataset.

By default it returns the name of the map, the semantic label of the
map, the start and end date of the map and the following values: mean,
minimum and maximum vale, mean_of_abs, standard deviation, variance,
coeff_var, number of null cells, total number of cells.

Using the *e* flag it can calculate also extended statistics: first
quartile, median value, third quartile and percentile 90.

If a *zones* raster map is provided, statistics are computed for each
zone (category) in that input raster map. The *zones* option does not
support space time raster datasets (STRDS) but only a single, static
raster map.

Space time raster datasets may contain raster maps with varying spatial
extent like for example series of scenes of satellite images. With the
*region_relation* option, computations can be limited to maps of the
space time raster dataset that have a given spatial relation to the
current computational region. Supported spatial relations are:

- "overlaps": process only maps that spatially overlap ("intersect")
  with the current computational region
- "is_contained": process only maps that are fully within the current
  computational region
- "contains": process only maps that contain (fully cover) the current
  computational region

## EXAMPLE

Obtain the univariate statistics for the raster space time dataset
"tempmean_monthly" (precision reduced to 2 decimals in this example):

```sh
t.rast.univar -e input=nc_lst_daily
id|semantic_label|start|end|mean|min|max|range|mean_of_abs|stddev|variance|coeff_var|sum|null_cells|cells|n|first_quartile|median|third_quartile
aqua_lst_day20020705@modis2002lst||2002-07-05 00:00:00|2002-07-06 00:00:00|36.24265068193782|25.730000000000018|45.77000000000004|20.04000000000002|36.24265068193782|2.6171016196159838|6.849220887396605|7.221054670044495|16252219.360000012|390977|839405|448428|34.49000000000001|36.07000000000005|37.83000000000004
aqua_lst_day20020706@modis2002lst||2002-07-06 00:00:00|2002-07-07 00:00:00|28.219656852808374|25.630000000000052|34.81|9.17999999999995|28.219656852808374|1.6350578714604542|2.6734142430247916|5.794038814819025|6960604.120000008|592747|839405|246658|27.110000000000014|27.970000000000027|28.970000000000027
aqua_lst_day20020707@modis2002lst||2002-07-07 00:00:00|2002-07-08 00:00:00|32.642155464172674|25.630000000000052|44.670000000000016|19.039999999999964|32.642155464172674|3.1874224286049917|10.159661738374142|9.764742503305083|23321318.54000002|124951|839405|714454|30.350000000000023|32.450000000000045|34.77000000000004
```

You can use format options as well to output the statistics in
JSON, CSV or Plain (default) formats. For example, to get JSON
output with extended statistics:

```sh
t.rast.univar -e input=nc_lst_daily format=json
```

```json
[
    {
        "id": "aqua_lst_day20020705@modis2002lst",
        "semantic_label": null,
        "start": "2002-07-05 00:00:00",
        "end": "2002-07-06 00:00:00",
        "n": 448428,
        "null_cells": 390977,
        "cells": 839405,
        "min": 25.730000000000018,
        "max": 45.77000000000004,
        "range": 20.04000000000002,
        "mean": 36.24265068193782,
        "mean_of_abs": 36.24265068193782,
        "stddev": 2.6171016196159838,
        "variance": 6.849220887396605,
        "coeff_var": 7.221054670044495,
        "sum": 16252219.360000012,
        "first_quartile": 34.49000000000001,
        "median": 36.07000000000005,
        "third_quartile": 37.83000000000004,
        "percentiles": [
            {
                "percentile": 90,
                "value": 40.01000000000005
            }
        ]
    },
    {
        "id": "aqua_lst_day20020706@modis2002lst",
        "semantic_label": null,
        "start": "2002-07-06 00:00:00",
        "end": "2002-07-07 00:00:00",
        "n": 246658,
        "null_cells": 592747,
        "cells": 839405,
        "min": 25.630000000000052,
        "max": 34.81,
        "range": 9.17999999999995,
        "mean": 28.219656852808374,
        "mean_of_abs": 28.219656852808374,
        "stddev": 1.6350578714604542,
        "variance": 2.6734142430247916,
        "coeff_var": 5.794038814819025,
        "sum": 6960604.120000008,
        "first_quartile": 27.110000000000014,
        "median": 27.970000000000027,
        "third_quartile": 28.970000000000027,
        "percentiles": [
            {
                "percentile": 90,
                "value": 30.450000000000045
            }
        ]
    },
    {
        "id": "aqua_lst_day20020707@modis2002lst",
        "semantic_label": null,
        "start": "2002-07-07 00:00:00",
        "end": "2002-07-08 00:00:00",
        "n": 714454,
        "null_cells": 124951,
        "cells": 839405,
        "min": 25.630000000000052,
        "max": 44.670000000000016,
        "range": 19.039999999999964,
        "mean": 32.642155464172674,
        "mean_of_abs": 32.642155464172674,
        "stddev": 3.1874224286049917,
        "variance": 10.159661738374142,
        "coeff_var": 9.764742503305083,
        "sum": 23321318.54000002,
        "first_quartile": 30.350000000000023,
        "median": 32.450000000000045,
        "third_quartile": 34.77000000000004,
        "percentiles": [
            {
                "percentile": 90,
                "value": 36.950000000000045
            }
        ]
    },
    ...
]
```

## SEE ALSO

*[t.create](t.create.md), [t.info](t.info.md) [t.create](r.univar.md),*

## AUTHOR

Sören Gebbert, Thünen Institute of Climate-Smart Agriculture  
Stefan Blumentrath (support for zones, parallel processing, and spatial
relations)

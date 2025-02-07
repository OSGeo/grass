## DESCRIPTION

The purpose of **t.rast.extract** is to extract a subset of a space time
raster dataset and to store that subset in a different space time raster
dataset. The **where** condition is used to select the subset. In
addition a *r.mapcalc* sub-expression can be specified that performs
operations on all maps of the selected subset. In this expression the
name of the input space time raster dataset can be used as simple map
name. Other STRDS than the input STRDS can not be specified, but any
raster map. In case a *r.mapcalc* sub-expression is defined, the base
name of the resulting raster maps must be specified. The *r.mapcalc*
expression can be used to select maps as well, since by default
resulting empty maps are not registered in the output space time raster
dataset and removed after processing. The number of parallel GRASS GIS
processes can be specified to speed up the processing.

If no *r.mapcalc* expression is defined, the selected maps are simply
registered in the new created output space time raster dataset to avoid
data duplication.

## NOTES

The *r.mapcalc* sub-expression should not contain the left side
`"map ="` of a full *r.mapcalc* expression, only the right side, eg.:

```sh
t.rast.extract input=tempmean_monthly where="start_time > '2010-01-05'" output=selected_tempmean_monthly basename=new_tmean_month expression="if(tempmean_monthly < 0, null(), tempmean_monthly)"
```

## EXAMPLE

```sh
t.rast.extract input=tempmean_monthly output=tempmean_monthly_later_2012 where="start_time >= '2012-01-01'"

t.rast.list tempmean_monthly_later_2012
name|mapset|start_time|end_time
2012_01_tempmean|climate_2000_2012|2012-01-01 00:00:00|2012-02-01 00:00:00
2012_02_tempmean|climate_2000_2012|2012-02-01 00:00:00|2012-03-01 00:00:00
2012_03_tempmean|climate_2000_2012|2012-03-01 00:00:00|2012-04-01 00:00:00
2012_04_tempmean|climate_2000_2012|2012-04-01 00:00:00|2012-05-01 00:00:00
2012_05_tempmean|climate_2000_2012|2012-05-01 00:00:00|2012-06-01 00:00:00
2012_06_tempmean|climate_2000_2012|2012-06-01 00:00:00|2012-07-01 00:00:00
2012_07_tempmean|climate_2000_2012|2012-07-01 00:00:00|2012-08-01 00:00:00
2012_08_tempmean|climate_2000_2012|2012-08-01 00:00:00|2012-09-01 00:00:00
2012_09_tempmean|climate_2000_2012|2012-09-01 00:00:00|2012-10-01 00:00:00
2012_10_tempmean|climate_2000_2012|2012-10-01 00:00:00|2012-11-01 00:00:00
2012_11_tempmean|climate_2000_2012|2012-11-01 00:00:00|2012-12-01 00:00:00
2012_12_tempmean|climate_2000_2012|2012-12-01 00:00:00|2013-01-01 00:00:00
```

## SEE ALSO

*[t.create](t.create.md), [t.info](t.info.md)*

## AUTHOR

Sören Gebbert, Thünen Institute of Climate-Smart Agriculture

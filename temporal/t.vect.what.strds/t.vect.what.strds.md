## DESCRIPTION

The module *t.vect.what.strds* samples a space time raster dataset
(STRDS) at the spatio-temporal locations of a space time vector dataset
(STVDS).

## EXAMPLE

The example is based on the space time vector dataset
\"precip_stations@climate_2009_2012\" created in the
[t.vect.observe.strds](t.vect.observe.strds.html) example. In the
example below, the module fills the new column \"new_temp\" with values
extracted from the \"tempmean_monthly\" space time raster dataset:

```
t.vect.what.strds input=precip_stations@climate_2009_2012 \
                  strds=tempmean_monthly@climate_2009_2012 \
                  column=new_temp method=average
```

## SEE ALSO

*[r.univar](r.univar.html), [v.univar](v.univar.html),
[v.what.rast](v.what.rast.html), [v.what.rast3](v.what.rast3.html),
[v.what.strds](v.what.strds.html), [v.what.vect](v.what.vect.html),
[t.create](t.create.html), [t.info](t.info.html)*

## AUTHOR

Sören Gebbert, Thünen Institute of Climate-Smart Agriculture

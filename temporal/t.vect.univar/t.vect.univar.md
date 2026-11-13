## DESCRIPTION

The module *t.vect.univar* computes univariate statistics of a space
time vector dataset based on a single attribute row.

## EXAMPLE

The example is based on the
[t.vect.observe.strds](t.vect.observe.strds.md) example; so create the
**precip_stations** space time vector dataset and after run the
following command:

```sh
t.vect.univar input=precip_stations col=month
id|start|end|n|nmissing|nnull|min|max|range|mean|mean_abs|population_stddev|population_variance|population_coeff_variation|sample_stddev|sample_variance|kurtosis|skewness
precip_stations_monthly@climate_2009_2012|2009-01-01 00:00:00|2009-02-01 00:00:00|132|0|4|-2.31832|7.27494|9.59326|3.44624|3.5316|1.79322|3.21564|0.520341|1.80005|3.24019|0.484515|-0.338519
precip_stations_monthly@climate_2009_2012|2009-02-01 00:00:00|2009-03-01 00:00:00|132|0|4|-0.654152|7.90613|8.56028|5.47853|5.48844|1.73697|3.01708|0.317051|1.74359|3.04011|0.875252|-1.0632
....
precip_stations_monthly@climate_2009_2012|2012-10-01 00:00:00|2012-11-01 00:00:00|132|0|4|9.67596|18.4654|8.78945|14.945|14.945|1.90659|3.6351|0.127574|1.91386|3.66285|-0.0848967|-0.700833
precip_stations_monthly@climate_2009_2012|2012-11-01 00:00:00|2012-12-01 00:00:00|132|0|4|3.56755|10.6211|7.05357|7.72153|7.72153|1.33684|1.78715|0.173132|1.34194|1.8008|0.90434|-0.863935
precip_stations_monthly@climate_2009_2012|2012-12-01 00:00:00|2013-01-01 00:00:00|132|0|4|3.04325|11.6368|8.5935|8.20147|8.20147|1.78122|3.17275|0.217183|1.78801|3.19697|-0.177991|-0.501295
```

The **format** option changes the output format. The default is
plain text; **format=csv** prints the same columns comma separated and
**format=json** prints one record per map with the statistics of
[v.univar](v.univar.md):

```sh
t.vect.univar input=precip_stations col=month format=json
```

```json
[
    {
        "id": "precip_stations_monthly@climate_2009_2012",
        "start": "2009-01-01 00:00:00",
        "end": "2009-02-01 00:00:00",
        "n": 132,
        "missing": 0,
        "nnull": 4,
        "min": -2.31832,
        "max": 7.27494,
        "range": 9.59326,
        "sum": 454.903,
        "mean": 3.44624,
        "mean_abs": 3.5316,
        "population_stddev": 1.79322,
        "population_variance": 3.21564,
        "population_coeff_variation": 0.520341,
        "sample_stddev": 1.80005,
        "sample_variance": 3.24019,
        "kurtosis": 0.484515,
        "skewness": -0.338519
    },
    ...
]
```

## SEE ALSO

*[t.create](t.create.md), [t.info](t.info.md)*

## AUTHOR

Sören Gebbert, Thünen Institute of Climate-Smart Agriculture

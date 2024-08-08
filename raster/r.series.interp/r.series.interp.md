## DESCRIPTION

*r.series.interp* interpolates new raster maps located temporal or
spatial in between existing raster maps. The interpolation is performed
at specific sampling positions. The sampling position for each output
map must be specified, as well as the data position of the input maps.
The following interpolation methods are supported.

-   linear: Linear interpolation. At least two input maps and data
    positions are required.

## EXAMPLES

Interpolate linear three new maps at 3 sampling positions in the
interval (0.0;1.0)\
First prepare the input maps:\

```
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 res3=10 -p3

r.mapcalc expr="prec_1 = 100"
r.mapcalc expr="prec_5 = 500"
```

Interpolate

```
r.series.interp --v input=prec_1,prec_5 datapos=0.0,1.0 \
                  output=prec_2,prec_3,prec_4 samplingpos=0.25,0.5,0.75 \
                  method=linear
```

Interpolate using the file option. First prepare the input file:\

```
echo "prec_2|0.25
prec_3|0.5
prec_4|0.75" >> outfile.txt
```

Interpolate:

```
r.series.interp --v input=prec_1,prec_5 datapos=0.0,1.0 file=outfile.txt method=linear
```

The resulting maps will have the values 200, 300 and 400.

## SEE ALSO

*[g.region](g.region.html), [r.series](r.series.html),
[r.series.accumulate](r.series.accumulate.html)*

[Hints for large raster data
processing](https://grasswiki.osgeo.org/wiki/Large_raster_data_processing)

## AUTHOR

Sören Gebbert

To compile the example simply use `make` (in this directory):

```shell
make
```

To run (the asterisks will match your operating system and version
specific directory and file):

```shell
../../../bin.*/grass* --tmp-location XY --exec bash <<EOF
g.region res=0.1
r.mapcalc -s expression='raster_1 = rand(0., 15)'
r.mapcalc -s expression='raster_2 = rand(0., 15)'
r.mapcalc -s expression='raster_3 = rand(0., 15)'
r.example.segmulti input=raster_1,raster_2,raster_3 output=raster_out
r.univar raster_1
r.univar raster_2
r.univar raster_3
r.univar raster_out
r.info -g raster_out
r.describe raster_out
g.gui -f
EOF

```

Both assumes you have GRASS GIS locally compiled.

To precisely time the execution, you can use *perf*:

```shell
perf stat -r 100 r.example.segmulti ... --overwrite
```

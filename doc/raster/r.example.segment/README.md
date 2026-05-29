To compile the example simply use `make` (in this directory):

```shell
make
```

To run (the asterisks will match your operating system and version
specific directory and file):

```shell
../../../bin.*/grass* --tmp-location XY --exec bash <<EOF
    g.region res=0.1
    r.mapcalc -s expression='raster_map_1 = rand(0., 15)'
    r.example.segment input=raster_map_1 output=raster_map_2
    r.univar raster_map_1
    r.univar raster_map_2
EOF
```

Both assumes you have GRASS GIS locally compiled.

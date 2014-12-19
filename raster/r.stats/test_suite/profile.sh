#spearfish
MAP=elevation.10m
g.region raster=$MAP
CMD="r.stats -a $MAP"

time $CMD

valgrind --tool=callgrind --trace-children=yes $CMD

kcachegrind callgrind.out.12345


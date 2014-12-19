# NC dataset, http://grass.osgeo.org/download/sample-data/

OUTFILE=callgrind.out.1196
MAP=elevation
export GRASS_OVERWRITE=1

g.region raster=$MAP -p

CMD="r.stats -a $MAP"

time $CMD

valgrind --tool=callgrind --callgrind-out-file=$OUTFILE --trace-children=yes $CMD

# http://kcachegrind.sourceforge.net
kcachegrind $OUTFILE

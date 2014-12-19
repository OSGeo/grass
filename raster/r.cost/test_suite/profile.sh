# NC dataset, http://grass.osgeo.org/download/sample-data/

OUTFILE=callgrind.out.1196
MAP=elevation
export GRASS_OVERWRITE=1

g.region raster=$MAP -p
r.mapcalc 'one = 1'
CMD="r.cost -k in=one start_points=schools_wake output=cost.test --o"

valgrind --tool=callgrind --callgrind-out-file=$OUTFILE --trace-children=yes $CMD

g.remove --q -f type=raster name=one

# http://kcachegrind.sourceforge.net
kcachegrind $OUTFILE

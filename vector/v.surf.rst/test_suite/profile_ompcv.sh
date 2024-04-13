# nc2008/grass7 mapset

OUTFILE=callgrind.out.1196

g.copy vect=elev_lid792_bepts,lidar_test
v.build lidar_test
g.region n=221230 s=220816 w=639090 e=639530 res=0.5 -p
CMD="v.surf.rst in=lidar_test cvdev=omp_cv_test nprocs=20 -c --overwrite"
time $CMD

valgrind --tool=callgrind --callgrind-out-file=$OUTFILE --trace-children=yes $CMD

# http://kcachegrind.sourceforge.net
kcachegrind $OUTFILE

exit 0

time $CMD
=========

--raster map not found?

# nc2008/grass7 mapset

OUTFILE=callgrind.out.1196

g.region n=221230 s=220404 w=638634 e=639530 res=2
CMD="v.surf.bspline in=lidar_test raster=elev.lidar_test_cubic method=bicubic --o"
valgrind --tool=callgrind --callgrind-out-file=$OUTFILE --trace-children=yes $CMD

# http://kcachegrind.sourceforge.net
kcachegrind $OUTFILE

99% of time is spent in gmath lib's G_math_cholesky_sband_decomposition()


time $CMD
=========

base serial:
real    2m58.174s
user    2m57.759s
sys     0m0.428s
--
real    2m58.215s
user    2m57.723s
sys     0m0.508s


## with OpenMP x6 cpus:
real    0m50.322s     3.5x speedup in clock time   (42% efficiency loss)
user    4m44.006s     1.6x increase in overall CPU time
sys     0m1.048s
--nostaic
real    0m50.792s
user    4m46.426s
sys     0m1.248s
--static again
real    0m50.168s
user    4m42.914s
sys     0m1.180s
--nostatic again
real    0m52.104s
user    4m54.334s
sys     0m1.244s

(static wins)


##OMP_NUM_THREADS=4:
real    1m30.730s
user    5m52.274s
sys     0m0.696s


##OMP_NUM_THREADS=2:
real    1m48.180s     1.6x speedup in clocktime
user    3m32.477s     1.2x increase in overall CPU time
sys     0m0.560s


##OMP_NUM_THREADS=1:
real    2m51.480s
user    2m51.031s
sys     0m0.464s


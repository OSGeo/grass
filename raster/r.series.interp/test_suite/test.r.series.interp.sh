# Test r.series.interp 
# We need to set a specific region in the
# @preprocess step of this test. We generate
# raster maps with r.mapcalc 
# The region setting should work for UTM and LL test locations
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 res3=10 -p3

r.mapcalc --o expr="prec_1 = 100"
r.mapcalc --o expr="prec_5 = 500"

r.mapcalc --o expr="map_10 = 10"
r.mapcalc --o expr="map_20 = 20"
r.mapcalc --o expr="map_30 = 30"
r.mapcalc --o expr="map_40 = 40"

# We create an input files containing empty lines. 
# However, r.series.interp should process the 
# valid raster maps and positions listed in the files.

# The first @test with map input and @precision=3
r.series.interp --o --v input=prec_1,prec_5 datapos=0.0,1.0 output=prec_2,prec_3,prec_4 samplingpos=0.25,0.5,0.75 method=linear

#r.out.ascii --o input=prec_2 output=test_1_prec_2.ref dp=3
#r.out.ascii --o input=prec_3 output=test_1_prec_3.ref dp=3
#r.out.ascii --o input=prec_4 output=test_1_prec_4.ref dp=3

# The second @test with file input and @precision=3
r.series.interp --o --v input=prec_1,prec_5 datapos=0.0,1.0 outfile=outfile_1.txt method=linear
r.series.interp --o --v infile=infile_2.txt outfile=outfile_2.txt method=linear

#r.out.ascii --o input=prec_2 output=test_2_prec_2.ref dp=3
#r.out.ascii --o input=prec_3 output=test_2_prec_3.ref dp=3
#r.out.ascii --o input=prec_4 output=test_2_prec_4.ref dp=3
#r.out.ascii --o input=map_12 output=test_2_map_12.ref dp=3
#r.out.ascii --o input=map_14 output=test_2_map_14.ref dp=3
#r.out.ascii --o input=map_16 output=test_2_map_16.ref dp=3
#r.out.ascii --o input=map_18 output=test_2_map_18.ref dp=3
#r.out.ascii --o input=map_25 output=test_2_map_25.ref dp=3
#r.out.ascii --o input=map_35 output=test_2_map_35.ref dp=3

# We need @tests to check the @failure handling, as outputs, file and sampling points
# are not handled by the grass parser
# No outputs
r.series.interp --o --v input=prec_1,prec_5 datapos=0.0,1.0 samplingpos=0.25,0.5,0.75
# No sampling points
r.series.interp --o --v input=prec_1,prec_5 datapos=0.0,1.0 output=prec_2,prec_3,prec_4 
# Output and file at once
r.series.interp --o --v  input=prec_1,prec_5 datapos=0.0,1.0 outfile=outfile_1.txt \
                  output=prec_2,prec_3,prec_4 samplingpos=0.25,0.5,0.75 method=linear
# Sampling points and file at once 
r.series.interp --o --v  input=prec_1,prec_5 datapos=0.0,1.0 outfile=outfile_1.txt samplingpos=0.25,0.5,0.75 method=linear
# Wrong input file
r.series.interp --o --v input=prec_1,prec_5 datapos=0.0,1.0 outfile=no_file_there method=linear
# Corrupt input file
r.series.interp --o --v  input=prec_1,prec_5 datapos=0.0,1.0 outfile=outfile_corrupt.txt method=linear


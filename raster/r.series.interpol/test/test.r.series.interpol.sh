# Test r.series.interpol 
# We need to set a specific region in the
# @preprocess step of this test. We generate
# raster maps with r.mapcalc 
# The region setting should work for UTM and LL test locations
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 res3=10 -p3

r.mapcalc --o expr="prec_1 = 100"
r.mapcalc --o expr="prec_5 = 500"

TMP_FILE=`g.tempfile pid=1`
TMP_FILE_CORRUPT=`g.tempfile pid=2`
# We create an input files containing empty lines. 
# However, r.series.interpol should process the 
# valid raster maps and positions listed in the files.

cat > "${TMP_FILE}" << EOF

prec_2|0.25


prec_3|0.5

prec_4|0.75


EOF

cat > "${TMP_FILE_CORRUPT}" << EOF
prec_2|0.25
prec_3|0.5
prec_4
EOF


# The first @test with map input and @precision=3
r.series.interpol --o --v input=prec_1,prec_5 output=prec_2,prec_3,prec_4 sampoints=0.25,0.5,0.75 method=linear

#r.out.ascii --o input=prec_2 output=test_1_prec_2.ref dp=3
#r.out.ascii --o input=prec_3 output=test_1_prec_3.ref dp=3
#r.out.ascii --o input=prec_4 output=test_1_prec_4.ref dp=3

# The second @test with file input and @precision=3
r.series.interpol --o --v input=prec_1,prec_5 file="${TMP_FILE}" method=linear

#r.out.ascii --o input=prec_2 output=test_2_prec_2.ref dp=3
#r.out.ascii --o input=prec_3 output=test_2_prec_3.ref dp=3
#r.out.ascii --o input=prec_4 output=test_2_prec_4.ref dp=3

# We need @tests to check the @failure handling, as outputs, file and sampling points
# are not handled by the grass parser
# No outputs
r.series.interpol --o --v input=prec_1,prec_5 sampoints=0.25,0.5,0.75
# No sampling points
r.series.interpol --o --v input=prec_1,prec_5 output=prec_2,prec_3,prec_4 
# Output and file at once
r.series.interpol --o --v  input=prec_1,prec_5 file="${TMP_FILE}" output=prec_2,prec_3,prec_4 sampoints=0.25,0.5,0.75 method=linear
# Sampling points and file at once 
r.series.interpol --o --v  input=prec_1,prec_5 file="${TMP_FILE}" sampoints=0.25,0.5,0.75 method=linear
# Wrong input file
r.series.interpol --o --v input=prec_1,prec_5 file=no_file_there method=linear
# Corrupt input file
r.series.interpol --o --v  input=prec_1,prec_5 file="${TMP_FILE_CORRUPT}" method=linear

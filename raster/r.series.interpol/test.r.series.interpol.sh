# Test r.series.interpol 
# We need to set a specific region in the
# @preprocess step of this test. We generate
# raster maps with r.mapcalc 
# The region setting should work for UTM and LL test locations
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 res3=10 -p3

r.mapcalc --o expr="prec_1 = 100"
r.mapcalc --o expr="prec_5 = 500"

TMP_FILE=`g.tempfile pid=1`
# We create an input files containing empty lines. However, r.series.interpol should process the 
# valid raster maps listed in the files.

cat > "${TMP_FILE}" << EOF

prec_2|0.25


prec_3|0.5

prec_4|0.75


EOF

# The first @test with map input and @precision=3
r.series.interpol --o --v input=prec_1,prec_5 output=prec_2,prec_3,prec_4 sampoint=0.25,0.5,0.75 method=linear

#r.out.ascii --o input=prec_2 output=test_1_prec_2.ref dp=3
#r.out.ascii --o input=prec_3 output=test_1_prec_3.ref dp=3
#r.out.ascii --o input=prec_4 output=test_1_prec_4.ref dp=3

# The second @test with file input and @precision=3
r.series.interpol --o --v input=prec_1,prec_5 file="${TMP_FILE}" method=linear

#r.out.ascii --o input=prec_2 output=test_2_prec_2.ref dp=3
#r.out.ascii --o input=prec_3 output=test_2_prec_3.ref dp=3
#r.out.ascii --o input=prec_4 output=test_2_prec_4.ref dp=3

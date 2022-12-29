# Test r.series basic aggregation functions with map and file inputs
# We need to set a specific region in the
# @preprocess step of this test. We generate
# raster maps with r.mapcalc 
# The region setting should work for UTM and LL test locations
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 res3=10 -p3

r.mapcalc --o expr="prec_1 = 100"
r.mapcalc --o expr="prec_2 = 200"
r.mapcalc --o expr="prec_3 = 300"
r.mapcalc --o expr="prec_4 = 400"
r.mapcalc --o expr="prec_5 = 500"
r.mapcalc --o expr="prec_6 = 600"

TMP_FILE=`g.tempfile pid=1`
TMP_FILE_WEIGHTS=`g.tempfile pid=2`
# We create an input files containing empty lines. However, r.series should process the 
# valid raster maps listed in the files.
cat > "${TMP_FILE}" << EOF
prec_1

prec_2
prec_3

prec_4

prec_5


prec_6


EOF

# The second file includes weights. The default weight of 1.0 
# must not be specified
cat > "${TMP_FILE_WEIGHTS}" << EOF
prec_1|3

prec_2|1.5
prec_3

prec_4|0.75

prec_5|0.6


prec_6|0.5


EOF


# The first @test with map input and @precision=3
r.series -z --o --v input=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6 output=test_1_prec_mean method=average
r.series    --o --v input=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6 output=test_1_prec_max method=maximum
r.series -z --o --v input=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6 output=test_1_prec_min method=minimum
r.series    --o --v input=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6 output=test_1_prec_count method=count
r.series -z --o --v input=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6 output=test_1_prec_range method=range
r.series    --o --v input=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6 output=test_1_prec_sum method=sum

#r.out.ascii --o input=test_1_prec_mean output=test_1_prec_mean.ref dp=3
#r.out.ascii --o input=test_1_prec_max output=test_1_prec_max.ref dp=3
#r.out.ascii --o input=test_1_prec_min output=test_1_prec_min.ref dp=3
#r.out.ascii --o input=test_1_prec_count output=test_1_prec_count.ref dp=3
#r.out.ascii --o input=test_1_prec_range output=test_1_prec_range.ref dp=3
#r.out.ascii --o input=test_1_prec_sum output=test_1_prec_sum.ref dp=3

# Now @test the weighting option 
r.series -z --o --v input=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6 weights=6,3,2,1.5,1.2,1 output=test_1_prec_mean_weight method=average
r.series    --o --v input=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6 weights=6,3,2,1.5,1.2,1 output=test_1_prec_max_weight method=maximum
r.series -z --o --v input=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6 weights=6,3,2,1.5,1.2,1 output=test_1_prec_min_weight method=minimum
r.series    --o --v input=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6 weights=6,3,2,1.5,1.2,1 output=test_1_prec_count_weight method=count
r.series -z --o --v input=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6 weights=6,3,2,1.5,1.2,1 output=test_1_prec_range_weight method=range
r.series    --o --v input=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6 weights=6,3,2,1.5,1.2,1 output=test_1_prec_sum_weight method=sum

#r.out.ascii --o input=test_1_prec_mean_weight output=test_1_prec_mean_weight.ref dp=3
#r.out.ascii --o input=test_1_prec_max_weight output=test_1_prec_max_weight.ref dp=3
#r.out.ascii --o input=test_1_prec_min_weight output=test_1_prec_min_weight.ref dp=3
#r.out.ascii --o input=test_1_prec_count_weight output=test_1_prec_count_weight.ref dp=3
#r.out.ascii --o input=test_1_prec_range_weight output=test_1_prec_range_weight.ref dp=3
#r.out.ascii --o input=test_1_prec_sum_weight output=test_1_prec_sum_weight.ref dp=3


# The second @test with file input and @precision=3
r.series -z --o --v file="${TMP_FILE}" output=test_2_prec_mean method=average
r.series    --o --v file="${TMP_FILE}" output=test_2_prec_max method=maximum
r.series -z --o --v file="${TMP_FILE}" output=test_2_prec_min method=minimum
r.series    --o --v file="${TMP_FILE}" output=test_2_prec_count method=count
r.series -z --o --v file="${TMP_FILE}" output=test_2_prec_range method=range
r.series    --o --v file="${TMP_FILE}" output=test_2_prec_sum method=sum

#r.out.ascii --o input=test_2_prec_mean output=test_2_prec_mean.ref dp=3
#r.out.ascii --o input=test_2_prec_max output=test_2_prec_max.ref dp=3
#r.out.ascii --o input=test_2_prec_min output=test_2_prec_min.ref dp=3
#r.out.ascii --o input=test_2_prec_count output=test_2_prec_count.ref dp=3
#r.out.ascii --o input=test_2_prec_range output=test_2_prec_range.ref dp=3
#r.out.ascii --o input=test_2_prec_sum output=test_2_prec_sum.ref dp=3

# Now @test the weighting 
r.series -z --o --v file="${TMP_FILE_WEIGHTS}" output=test_2_prec_mean_weight method=average
r.series    --o --v file="${TMP_FILE_WEIGHTS}" output=test_2_prec_max_weight method=maximum
r.series -z --o --v file="${TMP_FILE_WEIGHTS}" output=test_2_prec_min_weight method=minimum
r.series    --o --v file="${TMP_FILE_WEIGHTS}" output=test_2_prec_count_weight method=count
r.series -z --o --v file="${TMP_FILE_WEIGHTS}" output=test_2_prec_range_weight method=range
r.series    --o --v file="${TMP_FILE_WEIGHTS}" output=test_2_prec_sum_weight method=sum

#r.out.ascii --o input=test_2_prec_mean_weight output=test_2_prec_mean_weight.ref dp=3
#r.out.ascii --o input=test_2_prec_max_weight output=test_2_prec_max_weight.ref dp=3
#r.out.ascii --o input=test_2_prec_min_weight output=test_2_prec_min_weight.ref dp=3
#r.out.ascii --o input=test_2_prec_count_weight output=test_2_prec_count_weight.ref dp=3
#r.out.ascii --o input=test_2_prec_range_weight output=test_2_prec_range_weight.ref dp=3
#r.out.ascii --o input=test_2_prec_sum_weight output=test_2_prec_sum_weight.ref dp=3

# The third @test with multiple methods and outputs
r.series    --o --v input=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6 output=test_3_prec_mean,test_3_prec_max,test_3_prec_min method=average,maximum,minimum
r.series -z --o --v file="${TMP_FILE}" output=test_4_prec_mean,test_4_prec_max,test_4_prec_min method=average,maximum,minimum

#r.out.ascii --o input=test_1_prec_mean output=test_3_prec_mean.ref dp=3
#r.out.ascii --o input=test_1_prec_max output=test_3_prec_max.ref dp=3
#r.out.ascii --o input=test_1_prec_min output=test_3_prec_min.ref dp=3
#r.out.ascii --o input=test_2_prec_mean output=test_4_prec_mean.ref dp=3
#r.out.ascii --o input=test_2_prec_max output=test_4_prec_max.ref dp=3
#r.out.ascii --o input=test_2_prec_min output=test_4_prec_min.ref dp=3

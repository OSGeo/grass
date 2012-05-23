#!/bin/sh
# Space time raster dataset aggregation

# We need to set a specific region in the
# @preprocess step of this test. 
# The region setting should work for UTM and LL test locations
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 res3=10 -p3
# Data generation
r.mapcalc --o expr="prec_1 = rand(0, 550)"
r.mapcalc --o expr="prec_2 = rand(0, 450)"
r.mapcalc --o expr="prec_3 = rand(0, 320)"
r.mapcalc --o expr="prec_4 = rand(0, 510)"
r.mapcalc --o expr="prec_5 = rand(0, 300)"
r.mapcalc --o expr="prec_6 = rand(0, 650)"

t.create --o type=strds temporaltype=relative output=precip_abs1 title="A test" descr="A test"
t.register -i type=rast input=precip_abs1 maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6 start=0 unit=days increment=3

# The first @test

t.rast.aggregate --o --v input=precip_abs1 output=precip_abs2 base=prec_sum granularity=6 method=average sampling=start,during
t.info type=strds input=precip_abs2
r.info prec_sum_0
r.info prec_sum_1
r.info prec_sum_2
t.rast.aggregate --o --v input=precip_abs1 output=precip_abs2 base=prec_sum granularity=9 method=maximum sampling=start,during
t.info type=strds input=precip_abs2
r.info prec_sum_0
r.info prec_sum_1
t.rast.aggregate --o --v input=precip_abs1 output=precip_abs2 base=prec_sum granularity=4 method=minimum sampling=start,during
t.info type=strds input=precip_abs2
r.info prec_sum_0
r.info prec_sum_1
r.info prec_sum_2
r.info prec_sum_3
t.rast.aggregate --o --v input=precip_abs1 output=precip_abs2 base=prec_sum granularity=5 method=sum sampling=start,during
t.info type=strds input=precip_abs2
r.info prec_sum_0
r.info prec_sum_1
r.info prec_sum_2
r.info prec_sum_3

t.unregister type=rast maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6
t.remove type=strds input=precip_abs1,precip_abs2

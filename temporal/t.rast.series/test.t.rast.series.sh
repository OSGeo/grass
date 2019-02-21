#!/bin/sh
# Simple r.series wrapper
# We need to set a specific region in the
# @preprocess step of this test. 
# The region setting should work for UTM and LL test locations
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 res3=10 -p3

r.mapcalc --o expr="prec_1 = 100"
r.mapcalc --o expr="prec_2 = 200"
r.mapcalc --o expr="prec_3 = 300"
r.mapcalc --o expr="prec_4 = 400"
r.mapcalc --o expr="prec_5 = 500"
r.mapcalc --o expr="prec_6 = 600"

# @test
t.create --o type=strds temporaltype=absolute output=precip_abs title="A test" descr="A test"

t.register type=raster input=precip_abs maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6 start="2001-01-01" increment="1 months"
t.rast.list precip_abs

t.rast.series --o -t input=precip_abs method=average,range output=prec_average,prec_range where="start_time > '2001-03-01'"
t.rast.series --o -t input=precip_abs method=average output=prec_average where="start_time > '2001-03-01'"
t.rast.series --o    input=precip_abs method=maximum output=prec_max order=start_time
t.rast.series        input=precip_abs method=sum output=prec_sum
t.rast.series --o    input=precip_abs method=sum output=prec_sum
# This test should raise an error
t.rast.series        input=precip_abs method=sum output=prec_sum

r.info prec_average
r.info prec_max
r.info prec_sum

t.unregister type=raster maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6,prec_sum,prec_max
t.remove type=strds input=precip_abs

g.remove -f type=raster name=prec_sum,prec_max,prec_average,prec_range

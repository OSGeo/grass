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

t.register --o type=raster input=precip_abs maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6 start="2001-01-01" increment="1 months"

t.rast.colors input=precip_abs color=random
r.colors.out map=prec_1
t.rast.colors input=precip_abs color=grey.eq
r.colors.out map=prec_2
t.rast.colors input=precip_abs color=grey.log
r.colors.out map=prec_3


t.unregister type=raster maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6
t.remove type=strds input=precip_abs

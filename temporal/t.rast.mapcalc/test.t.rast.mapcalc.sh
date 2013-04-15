#!/bin/sh
# Test for t.rast.mapcalc 

# We need to set a specific region in the
# @preprocess step of this test. We generate
# raster with r.mapcalc and create several space time raster datasets
# The region setting should work for UTM and LL test locations
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 res3=10 -p3

r.mapcalc --o expr="prec_1 = rand(0, 550)"
r.mapcalc --o expr="prec_2 = rand(0, 450)"
r.mapcalc --o expr="prec_3 = rand(0, 320)"
r.mapcalc --o expr="prec_4 = rand(0, 510)"
r.mapcalc --o expr="prec_5 = rand(0, 300)"
r.mapcalc --o expr="prec_6 = rand(0, 650)"

t.create --o type=strds temporaltype=absolute output=precip_abs1 title="A test" descr="A test"
t.create --o type=strds temporaltype=absolute output=precip_abs2 title="A test" descr="A test"
t.register -i --o type=rast input=precip_abs1 maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6 start="2001-01-01" increment="3 months"
t.register --o type=rast input=precip_abs2 maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6

t.info precip_abs1
t.info precip_abs2

# The first @test
t.rast.mapcalc --o -n inputs=precip_abs1,precip_abs2 output=precip_abs3 \
           expression=" precip_abs1 + precip_abs2" base=new_prec \
           method=equal nprocs=5
t.info type=strds input=precip_abs3

t.rast.mapcalc --o -s inputs=precip_abs1,precip_abs2,precip_abs3 output=precip_abs4 \
           expression=" (precip_abs1 + precip_abs2) / precip_abs2" base=new_prec \
           method=equal nprocs=5
t.info type=strds input=precip_abs4

t.rast.mapcalc --o -s inputs=precip_abs1,precip_abs2 output=precip_abs4 \
           expression=" (precip_abs1 + precip_abs2) * null()" base=new_prec \
           method=equal nprocs=5
t.info type=strds input=precip_abs4

t.rast.mapcalc --o -sn inputs=precip_abs1,precip_abs2 output=precip_abs4 \
           expression=" (precip_abs1 + precip_abs2) * null()" base=new_prec \
           method=equal nprocs=5
t.info type=strds input=precip_abs4

# @postprocess
t.unregister type=rast maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6
t.unregister type=rast maps=new_prec_1,new_prec_2,new_prec_3,new_prec_4,new_prec_5,new_prec_6
t.remove type=strds input=precip_abs1,precip_abs2,precip_abs3,precip_abs4
